#include "HWKernelOpt.h"
#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Debug.h"
#include "Substitute.h"
#include "IRPrinter.h"
#include "Simplify.h"
#include "Bounds.h"

#include <iostream>
#include <algorithm>
using std::ostream;

namespace Halide {
namespace Internal {

using std::string;
using std::map;
using std::set;
using std::pair;
using std::vector;

struct InitInfo {
    string op_name;
    vector<Expr> op_values;
};

class ExtractInitInfo : public IRMutator {
    using IRVisitor::visit;
    
    void visit(const Provide *op) {
        init_info.op_name = op->name;
        init_info.op_values = op->values;
        IRMutator::visit(op);
    }
public:
    InitInfo init_info;
};

InitInfo extract_init_info(Stmt s) {
    ExtractInitInfo eii;
    eii.mutate(s);
    internal_assert(eii.init_info.op_values.size() == 1);
    return eii.init_info;
}

class InsertInit : public IRMutator {
    using IRMutator::visit;
    InitInfo init_info;
    vector<Expr> op_args;
    string hw_output_name;
    string first_loop_var;
    vector<Expr> hw_output_args;
    
    void visit(const Provide *op) {
        hw_output_name = op->name;
        hw_output_args = op->args;
        vector<Expr> new_values(op->values.size());
        for (size_t i = 0; i < op->values.size(); i++) {
            Expr old_value = op->values[i];
            Expr new_value = mutate(old_value);
            new_values[i] = new_value;
        }
        stmt = Provide::make("hw_output_acc", new_values, {0});
    }

    void visit(const Call *op) {
        if(op->name == hw_output_name){
            IRMutator::visit(op);
            // TOOD: to be fixed
            // expr = Call::make(Int(16), "hw_output_acc", {0}, Call::Halide);
        }else{
            IRMutator::visit(op);
        }
    }

    void visit(const For *op) {
        if(ends_with(op->name, "r$y")){
            first_loop_var = op->name;
        }
        if(ends_with(op->name, "r$x")){
            // First we alter the loop
            Stmt new_body = mutate(op->body);
            Stmt new_for_loop = For::make(op->name, op->min, op->extent, op->for_type, op->device_api, new_body);
            // Initialize a variable to accumulate 
            Expr hw_output_acc = Variable::make(Int(16), "hw_output_acc");
            Stmt hw_output_acc_with_init = Provide::make("hw_output_acc", init_info.op_values, {0});
            stmt = Block::make(hw_output_acc_with_init, new_for_loop);
            // Add if else statement to utilize hw_output_acc
            Stmt acc_provide = Provide::make(hw_output_name, {hw_output_acc}, hw_output_args);
            Stmt else_provide = Provide::make(hw_output_name, {Add::make(hw_output_acc, Call::make(Int(16), hw_output_name, hw_output_args, Call::Halide))}, hw_output_args);
            Expr condition_var = Variable::make(Int(32), first_loop_var);
            Expr condition = EQ::make(condition_var, 0);
            Stmt ifthen = IfThenElse::make(condition, acc_provide, else_provide);
            stmt = Block::make(stmt, ifthen);
        }else{
            IRMutator::visit(op);    
        }
    }
public:
    void set_init_info(InitInfo ii){
        init_info = ii;
    }
};

Stmt insert_init(Stmt s, InitInfo init_info){
    InsertInit ini;
    ini.set_init_info(init_info);
    s = ini.mutate(s);
    return s;
}

class ResetInit : public IRMutator {
    using IRMutator::visit;
    
    void visit(const ProducerConsumer *op) {    
        if(op->is_producer){
            Stmt body = op->body;
            const Block *body_block = body.as<Block>();
            // The following if finds the producer with update
            if(body_block){
                debug(0) << op->name << "\n";
                debug(0) << "First: \n" << body_block->first << "\n";
                debug(0) << "Rest: \n" << body_block->rest << "\n";
                InitInfo init_info = extract_init_info(body_block->first);
                body = insert_init(body_block->rest, init_info);
            } else{
                IRMutator::visit(op);
            }
            if (body.same_as(op->body)) {
                stmt = op;
            } else {
                stmt = ProducerConsumer::make(op->name, op->is_producer, body);
                debug(0) << "Modified: \n" << stmt << "\n";
            }
        }else {
            IRMutator::visit(op);
        }
    }

public:
    /*ResetInit(const Scope<Expr> &s) : scope(s) {}*/
};

Stmt add_func_constraints(Stmt s, const HWKernelDAG &dag) {

    vector<pair<string, Expr>> constraints;

    for (auto &p : dag.kernels) {
        const HWKernel &kernel = p.second;
        const vector<Bound> bounds = kernel.func.schedule().bounds();
        int dim = 0;
        Expr prev_stride;
        for (auto &b : bounds) {
            //FIXME dim order  
            string min_name = kernel.name + ".min." + std::to_string(dim) + ".constrained";
            string stride_name = kernel.name + ".stride." + std::to_string(dim) + ".constrained";
            constraints.push_back(make_pair(min_name, b.min));
            if (dim == 0) {
                prev_stride = b.extent * 1;
            } else {
                constraints.push_back(make_pair(stride_name, prev_stride));
                prev_stride = prev_stride * b.extent;
            }
            dim++;
        }
    }
    // Inject the code that defines the constrained sizes.
    for (size_t i = constraints.size(); i > 0; i--) {
        s = LetStmt::make(constraints[i-1].first, constraints[i-1].second, s);
    }
    return s;
}

//Perform hw optimization for all function
class HWKernelOpt : public IRMutator {
    //const HWKernelDAG &dag;
    Scope<Expr> scope;

    using IRMutator::visit;
   void visit(const ProducerConsumer *op) {
      if (starts_with(op->name, "output")){ //FIXME accelerate_at function name
          Stmt body = mutate(op->body);
          stmt = ProducerConsumer::make("_hls_target." + op->name, true, body);
      } else {
          IRMutator::visit(op);
      }
    }

public:
    /*HWKernelOpt(const HWKernelDAG &d)
        : dag(d) {}*/
};


Stmt hwkernel_opt(Stmt s, const HWKernelDAG &dag) {
    debug(3) << s << "\n";
    // Now we find initiation before loop
    s = ResetInit().mutate(s);
    s = add_func_constraints(s, dag);
    debug(3) << "after add output constraints:\n" << s << "\n";
    //s = push_init_into_update(s, dag);
    //debug(0) << "after push def into update:\n" << s << "\n";
    s = HWKernelOpt().mutate(s);
    debug(3) << "after kernel optimization:\n" << s << "\n";
    return s;
}

}
}
