#include "HWKernelOpt.h"
#include "IRMutator.h"
#include "IROperator.h"
#include "IREquality.h"
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

namespace {

class ExpandExpr : public IRMutator {
    using IRMutator::visit;
    const Scope<Expr> &scope;

    void visit(const Variable *var) {
        if (scope.contains(var->name)) {
            // xuan TODO check if remove likely_if_innermost is ok
            //expr = scope.get(var->name);
            expr = mutate(scope.get(var->name));
            debug(4) << "Fully expanded " << var->name << " -> " << expr << "\n";
        } else {
            expr = var;
        }
    }
    
    void visit(const Call *op) {
        if (op->is_intrinsic(Call::likely) ||
                   op->is_intrinsic(Call::likely_if_innermost)) {
            assert(op->args.size() == 1);
            expr = mutate(op->args[0]);
        } else {
            IRMutator::visit(op); 
        }
    }  

public:
    ExpandExpr(const Scope<Expr> &s) : scope(s) {}

};

// Perform all the substitutions in a scope
Expr expand_expr(Expr e, const Scope<Expr> &scope) {
    ExpandExpr ee(scope);
    Expr result = ee.mutate(e);
    debug(4) << "Expanded " << e << " into " << result << "\n";
    return result;
}

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

class OuterReductionLoops : public IRVisitor {
    string kernel_name;
    vector<string> outer_rvars;

    using IRVisitor::visit;
    void visit(const For *op) {
        for(const string &s : outer_rvars) {
            if(starts_with(op->name, kernel_name) && ends_with(op->name, s)) {
                Expr v = Variable::make(Int(32), op->name);
                outer_rloops.push_back(v);
                break;
            }
        }

        op->body.accept(this);
    }

public:
    vector<Expr> outer_rloops;

    OuterReductionLoops(string kernel_name, vector<string>outer_rvars) :
        kernel_name(kernel_name), outer_rvars(outer_rvars) {}
};

vector<Expr> outer_reduction_loops(string kernel_name, vector<string> outer_rvars, Stmt s) {
    OuterReductionLoops orl(kernel_name, outer_rvars);
    s.accept(&orl);
    return orl.outer_rloops;
}
}

Stmt add_acc_init(string acc_name, InitInfo init_info) {
    return Provide::make(acc_name, init_info.op_values, {0});
}


Stmt add_acc_consumer(string kernel_name, string acc_name, Type acc_type, vector<Expr> outer_rloops, vector<Expr>func_args) {
    bool first = true;
    Expr cond;
    Expr zero = 0;
    for (const Expr &e : outer_rloops) {
        Expr eq = e == 0; //EQ::make(e, zero);
        if (first)
            cond = eq;
        else
            cond = And::make(cond, eq);
        first = false;
    }

    Expr res_var = Variable::make(Handle(), kernel_name);
    Expr acc_call = Call::make(acc_type, acc_name, {0}, Call::Intrinsic);

    Stmt then_case = Provide::make(kernel_name, {acc_call} , func_args);

    Expr value = Add::make(Call::make(acc_type, kernel_name, func_args, Call::Halide), acc_call);
    Stmt else_case = Provide::make(kernel_name, {value}, func_args);

    if (outer_rloops.size()) {
        return IfThenElse::make(cond, then_case, else_case);
    } else {
        return then_case;
    }
}

class ReplaceReferenceWithAcc : public IRMutator {
    const HWKernel &kernel;
    const HWKernelDAG &dag;
    const vector<Expr> &outer_rloops;
    const vector<string> &pure_args;
    const InitInfo &init_info;

    Scope<Expr> scope;
    string innermost_pure_arg;
    string acc_name;
    Type acc_type;
    vector<Expr> func_args;

    using IRMutator::visit;

    void visit(const For *op) {
        bool is_pure = false;
        for (const string &arg : pure_args) {
            if (starts_with(op->name, kernel.name) &&
               (ends_with(op->name, "." + arg))) {
                is_pure = true;
                break;
            }
        }
        if (is_pure) {
            Stmt new_body;
            debug(0) << "replace loop var " << op->name << "\n";
            // replace the loop var over the dimensions of the original function
            // realization with the loop var over the stencil dimension.
            // e.g. funcA.s0.x -> funcA.stencil.s0.x
            //      funcA.s1.x -> funcA.stencil.s1.x
            string old_var_name = op->name;
            vector<string> v = split_string(old_var_name, ".");
            string stage_name = v[1];
            string dim_name = v[2];
            //string stage_dim_name = op->name.substr(kernel.name.size()+1, old_var_name.size() - kernel.name.size());
            string new_var_name = kernel.name + "." + stage_name + ".stencil." + dim_name;
            Expr new_var = Variable::make(Int(32), new_var_name);
            Expr new_min = 0;
            //Expr new_extent = kernel.dims[dim_idx].step;

            // create a let statement for the old_loop_var
            Expr old_min = op->min;
            Expr old_var_value = new_var + old_min;

            // traversal down into the body
            scope.push(old_var_name, simplify(expand_expr(old_var_value, scope)));

            if (ends_with(op->name, "." + innermost_pure_arg)) {
                debug(0) << "build new for loop\n";
                acc_name = kernel.name + ".acc";
                /*Expr res_var = Variable::make(Handle(), kernel.name);
                Expr acc_var = Variable::make(Handle(), acc_name);*/

                Stmt init = add_acc_init(acc_name, init_info);
                Stmt produce = ProducerConsumer::make(acc_name, true, Block::make(init,  mutate(op->body)));
                Stmt acc_consumer = add_acc_consumer(kernel.name, acc_name, acc_type, outer_rloops, func_args);
                Stmt consume = ProducerConsumer::make(acc_name, false, acc_consumer);

                Stmt pc = Block::make(produce, consume);//(acc_name, produce, Stmt(), consume);

                //FIXME bounds should be extracted from touched region by this for loops
                Region acc_bounds;
                acc_bounds.push_back(Range(0, 1));
                new_body = Realize::make(acc_name, kernel.func.output_types(), acc_bounds, const_true(), pc);
            } else {
                new_body = mutate(op->body);
            }

            scope.pop(old_var_name);

            new_body = LetStmt::make(old_var_name, old_var_value, new_body);
            stmt = For::make(new_var_name, new_min, op->extent, op->for_type, op->device_api, new_body);
        } else {
            IRMutator::visit(op);
        }
    }


    void visit(const Call *op) {
        if(op->name  == kernel.name) {
            vector<Expr> new_args;
            new_args.push_back(0);
            acc_type = op->type;
            expr = Call::make(op->type, acc_name, new_args, Call::Intrinsic);
        } else if (std::find(kernel.input_streams.begin(), kernel.input_streams.end(),
                     op->name) != kernel.input_streams.end() // call to a input stencil)
            ) {
           debug(0) << "call to input " << op->name << "\n";
           const auto it = dag.kernels.find(op->name);
           internal_assert(it != dag.kernels.end());
           const HWKernel &stencil_kernel = it->second;

           vector<Expr> new_args(op->args.size());
            for (size_t i = 0; i < op->args.size(); i++) {
                Expr old_arg = mutate(op->args[i]);
                Expr offset;
                // This is call to input stencil
                // we use the min_pos stored in in_kernel.consumer_stencils
                const auto it = stencil_kernel.consumer_stencils.find(kernel.name);
                internal_assert(it != kernel.consumer_stencils.end());
                offset = it->second[i].min_pos;
                Expr new_arg = old_arg;// - offset;TODO check whether this is correct
                new_arg = simplify(expand_expr(new_arg, scope));
                // TODO check if the new_arg only depends on the loop vars
                // inside the producer
                new_args[i] = new_arg;
            }
            expr = Call::make(op->type, op->name, new_args, Call::Halide);
            debug(0) << "replacing call " << Expr(op) << " with\n"
                     << "\t" << expr << "\n";
        } else {
            IRMutator::visit(op);
        }
    }

    void visit(const Provide *op) {
        if(op->name  == kernel.name) {
           /*for(size_t i = 0; i < op->args.size(); i++) {
               provide_args.push_back(op->args[i]);
               debug(0) << " provide arg " << i << ": " << op->args[i] << "\n";
           }*/
            func_args = op->args;

            // Replace the arguments. e.g.
            //   func.s0.x -> func.stencil.x + x,min
            for (size_t i = 0; i < kernel.func.args().size(); i++) {
                //func_args[i] = simplify(expand_expr(mutate(op->args[i]) - kernel.dims[i].min_pos, scope));
                func_args[i] = simplify(expand_expr(mutate(op->args[i]), scope));
            }

           //TODO new args
            vector<Expr> new_args;
            new_args.push_back(0);
            vector<Expr> new_values(op->values.size());
            for (size_t i = 0; i < op->values.size(); i++) {
                new_values[i] = mutate(op->values[i]);
            }

            stmt = Provide::make(acc_name, new_values, new_args);
        } else {
            IRMutator::visit(op);
        }

    }

public:
    ReplaceReferenceWithAcc(const HWKernel &k, const HWKernelDAG &d, const vector<Expr> &v, const vector<string> &p, const InitInfo &ii)
        : kernel(k), dag(d), outer_rloops(v), pure_args(p), init_info(ii){
        innermost_pure_arg = pure_args.back();
        debug(0) << "innermost pure arg " << innermost_pure_arg << "\n";
    } 
};


class PushInitIntoUpdate : public IRMutator {
    const HWKernelDAG &dag;

    HWKernel kernel;
    vector<Expr> outer_rloops;
    vector<string> pure_args;  
    using IRMutator::visit;

    
    void visit(const ProducerConsumer *op) {    
        if(dag.kernels.count(op->name) &&
           op->is_producer){
            Stmt body = op->body;
            const Block *body_block = body.as<Block>();
            // The following if finds the producer with update
            if(body_block
               && body_block->first.defined()
               && body_block->rest.defined()){
                /*debug(0) << "First: \n" << body_block->first << "\n";
                debug(0) << "Rest: \n" << body_block->rest << "\n";*/
                kernel = dag.kernels.find(op->name)->second;
                const StageSchedule &s = kernel.func.update_schedule(0);
        
                int innermost_pure_arg_index = (int)s.dims().size() - 1;
                map<string, int> rvar_loop_index;
                for (int i = (int)s.dims().size() - 1; i >= 0; i--) {
                    const Dim &dim = s.dims()[i];
                    debug(0) << "update dim " << i << ": " << dim.var << "\n";
                    bool is_pure = true;
                    for (const ReductionVariable &r : s.rvars()) {
                        if (dim.var.compare(r.var) == 0) {
                            rvar_loop_index[r.var] = i;
                            is_pure = false;
                            break;
                        }
                    }
                    if (is_pure) {
                        pure_args.push_back(dim.var);
                        innermost_pure_arg_index = i;
                    }
                }
        
        
                vector<string> outer_rvars;
                for(auto const& x : rvar_loop_index) {
                    if (x.second > innermost_pure_arg_index) {
                        outer_rvars.push_back(x.first);
                    }
                }
                outer_rloops = outer_reduction_loops(kernel.name, outer_rvars, body_block->rest);
        
        
                debug(0) << "pure arg innermost" << innermost_pure_arg_index << "\n";
                InitInfo init_info = extract_init_info(body_block->first);
                //body = insert_init(body_block->rest, init_info);
                Stmt produce = ReplaceReferenceWithAcc(kernel, dag, outer_rloops, pure_args, init_info).mutate(body_block->rest);
                stmt = ProducerConsumer::make(op->name, op->is_producer, produce);
                //TODO support stencil partial output, currently only support single partial output
            /*} else{
                IRMutator::visit(op);
            }*/
            /*if (body.same_as(op->body)) {
                stmt = op;
            } else {*/
            //if (!body.same_as(op->body)) {
                //stmt = ProducerConsumer::make(op->name, op->is_producer, body);
                //debug(0) << "Modified: \n" << stmt << "\n";
            }else { 
                IRMutator::visit(op);
            }
        }else {
            debug(0) << "rest: "<< op->name << "\n";
            IRMutator::visit(op);
        }
    }

public:
    PushInitIntoUpdate(const HWKernelDAG &dag) : dag(dag) {}
};


Stmt push_init_into_update(Stmt s, const HWKernelDAG &dag){
  return PushInitIntoUpdate(dag).mutate(s);
}

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
    const map<string, Function> &env;
    Scope<Expr> scope;

    using IRMutator::visit;
   void visit(const ProducerConsumer *op) {
      map<string, Function>::const_iterator it = env.find(op->name); // This is a way to hack const map key when C++ 11 is not supported
      if (it!=env.end() && it->second.schedule().is_accelerated()){
          Stmt body = mutate(op->body);
          stmt = ProducerConsumer::make("_hls_target." + op->name, true, body);
      } else {
          IRMutator::visit(op);
      }
    }

public:
    HWKernelOpt(const std::map<std::string, Function> &e)
        : env(e) {}
};

Stmt hw_kernel_opt(Stmt s, const map<std::string, Function> &env){
    return HWKernelOpt(env).mutate(s);
}  

class SubstituteLoopVar : public IRMutator {
    const map<string, Expr> &replace;
    Scope<int> hidden;

    Expr find_replacement(const string &s) {
        map<string, Expr>::const_iterator iter = replace.find(s);
        if (iter != replace.end() && !hidden.contains(s)) {
            return iter->second;
        } else {
            return Expr();
        }
    }

public:
    SubstituteLoopVar(const map<string, Expr> &m) : replace(m) {}

    using IRMutator::visit;

    void visit(const Variable *v) {
        Expr r = find_replacement(v->name);
        if (r.defined()) {
            expr = simplify(Add::make(r, v)); // Different with the Substitute class
        } else {
            expr = v;
        }
    }

    void visit(const Let *op) {
        Expr new_value = mutate(op->value);
        hidden.push(op->name, 0);
        Expr new_body = mutate(op->body);
        hidden.pop(op->name);

        if (new_value.same_as(op->value) &&
            new_body.same_as(op->body)) {
            expr = op;
        } else {
            expr = Let::make(op->name, new_value, new_body);
        }
    }

    void visit(const LetStmt *op) {
        Expr new_value = mutate(op->value);
        hidden.push(op->name, 0);
        Stmt new_body = mutate(op->body);
        hidden.pop(op->name);

        if (new_value.same_as(op->value) &&
            new_body.same_as(op->body)) {
            stmt = op;
        } else {
            stmt = LetStmt::make(op->name, new_value, new_body);
        }
    }

    void visit(const For *op) {

        Expr new_min = mutate(op->min);
        Expr new_extent = mutate(op->extent);
        hidden.push(op->name, 0);
        Stmt new_body = mutate(op->body);
        hidden.pop(op->name);

        if (new_min.same_as(op->min) &&
            new_extent.same_as(op->extent) &&
            new_body.same_as(op->body)) {
            stmt = op;
        } else {
            stmt = For::make(op->name, new_min, new_extent, op->for_type, op->device_api, new_body);
        }
    }

};

Stmt substitute_loop_var(const string &name, const Expr &replacement, const Stmt &stmt) {
    map<string, Expr> m;
    m[name] = replacement;
    SubstituteLoopVar s(m);
    return s.mutate(stmt);
}

class ShiftLoopMin : public IRMutator {

    using IRMutator::visit;

    void visit(const For *op) {
        string loop_var = op->name;
        Expr loop_min = op->min;
        Expr new_min = Expr(0);
        Stmt body = op->body;
        if(!equal(new_min, loop_min)){
            body = substitute_loop_var(loop_var, loop_min, op->body);
        }
        body = mutate(body);
        if(body.same_as(op->body)){
            stmt = op;
        }else {
            stmt = For::make(op->name, new_min, op->extent, op->for_type, op->device_api, body);
        }
    }
};

Stmt shift_loop_min(Stmt s){
    return ShiftLoopMin().mutate(s);
}  

Stmt hwkernel_opt(Stmt s, const map<std::string, Function> &env, const HWKernelDAG &dag) {
    debug(3) << s << "\n";
    s = add_func_constraints(s, dag);
    debug(3) << "after add output constraints:\n" << s << "\n";
    s = push_init_into_update(s, dag);
    debug(0) << "after push def into update:\n" << s << "\n";
    s = shift_loop_min(s);
    debug(0) << "after shift loop init:\n" << s << "\n";
    s = hw_kernel_opt(s, env);
    debug(3) << "after kernel optimization:\n" << s << "\n";
    return s;
}

}
}
