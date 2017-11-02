#include "HWRevertLet.h"
#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Simplify.h"
#include "CodeGen_Internal.h"

namespace Halide {
namespace Internal {

using std::string;
using std::vector;
using std::pair;

class FindInputScope : public Closure {
public:
    FindInputScope(Stmt s)  {
        s.accept(this);
    }

    vector<string> arguments();

protected:
    using Closure::visit;

};


vector<string> FindInputScope::arguments() {
    vector<string> res;
    for (const pair<string, Buffer> &it : buffers) {
        res.push_back(it.first);
        debug(0) << "add " << it.first << " to scope\n";
    }
    for (const pair<string, Type> &it : vars) {
        res.push_back(it.first);
        debug(0) << "add " << it.first << " to scope\n";
    }
    return res;
}

class ExtractInputScope : public IRVisitor {
    using IRVisitor::visit;
    vector<string> input_scope;

    void visit(const ProducerConsumer *op){
        if (op->is_producer && starts_with(op->name, "_hls_target.")) {
            Stmt body = op->body;
            FindInputScope c(body);
            input_scope = c.arguments();
        }else{
            IRVisitor::visit(op);
        }
    
    }
public:
    vector<string> get_input_scope(){
        return input_scope;
    }
};

vector<string> extract_intput_scope(Stmt s){
    ExtractInputScope eis;
    s.accept(&eis);
    return eis.get_input_scope();
}

class ExtractExprScope : public IRVisitor {
    using IRVisitor::visit;
    vector<string> hidden;

    void visit(const Variable *op){
        IRVisitor::visit(op);
        hidden.push_back(op->name);
        debug(0) << "Find hidden: " << op->name << "\n";
    }
public:
    vector<string> get_hidden(){
        return hidden;
    }
};

vector<string> extract_expr_scope(Expr e){
    ExtractExprScope evs;
    e.accept(&evs);
    return evs.get_hidden();
}

struct ExprInfo {
    Expr value;
    vector<string> expr_scope;
};
class FindRevertArgs : public IRMutator {
    using IRMutator::visit;
    
    Scope<ExprInfo>& expr_info;
    
    void visit(const For *op) {
        string loop_name = op->name;
        Expr loop_extent = op->extent;
        const Variable *var = loop_extent.as<Variable>();
        if(!(is_const(loop_extent)||var)){
            debug(0) << "Find loop extent that is not a const: " << loop_name << " " << loop_extent << "\n";
            vector<string> hidden_scope = extract_expr_scope(loop_extent);
            string loop_extent_var_name = op->name + ".loop_extent";
            Expr loop_extent_var = Variable::make(loop_extent.type(), loop_extent_var_name);
            ExprInfo info;
            info.value = loop_extent;
            info.expr_scope = hidden_scope;
            expr_info.push(loop_extent_var_name, info);
            Stmt new_body = mutate(op->body);
            // To be corrected later
            stmt = For::make(op->name, op->min, loop_extent_var, op->for_type, op->device_api, new_body);
            stmt = LetStmt::make(loop_extent_var_name, loop_extent, stmt);
        }else{
            IRMutator::visit(op);
        }
    }
public:
    FindRevertArgs(Scope<ExprInfo>& s) : expr_info(s) {}
};

class VarUsed : public IRVisitor {
    using IRVisitor::visit;
    string var_to_check;
    bool used = false;

    void visit(const Variable *op){
        IRVisitor::visit(op);
        if(var_to_check == op->name){
            used = true;
        }
    }
public:
    VarUsed(string& s) : var_to_check(s) {}
    bool get_used(){
        return used;
    }
};

bool var_used(Stmt s, string var_to_check){
    VarUsed vu(var_to_check);
    s.accept(&vu);
    return vu.get_used();
}

class AddRevertLets : public IRMutator {
    using IRMutator::visit;
    Scope<int> arg_scope;
    Scope<int> used_scope;
    vector<string>& input_scope;
    Scope<ExprInfo>& expr_info;

    void visit(const Let *op) {
        Expr new_value = mutate(op->value);
        arg_scope.push(op->name, 0);
        Expr new_body = mutate(op->body);
        arg_scope.pop(op->name);

        if (new_value.same_as(op->value) &&
            new_body.same_as(op->body)) {
            expr = op;
        } else {
            expr = Let::make(op->name, new_value, new_body);
        }
    }

    void visit(const LetStmt *op) {
        Expr new_value = mutate(op->value);
        arg_scope.push(op->name, 0);
        Stmt new_body = mutate(op->body);
        arg_scope.pop(op->name);

        if (new_value.same_as(op->value) &&
            new_body.same_as(op->body)) {
            stmt = op;
        } else {
            stmt = LetStmt::make(op->name, new_value, new_body);
        }
    }

    void visit(const For *op) {
        Scope<Expr> let_to_make;
        arg_scope.push(op->name, 0);
        for(auto it=expr_info.cbegin(); it!=expr_info.cend(); ++it){
            string name = it.name();
            if(used_scope.contains(name)){
                continue;
            }
            ExprInfo info = it.value();
            bool add_here = true;
            for(auto const& value: info.expr_scope) {
                if(!arg_scope.contains(value)){
                    add_here = false;
                }
            }
            if(add_here && var_used(op->body, name)){
                let_to_make.push(name, info.value);
                used_scope.push(name, 0);
            }
        }
        Stmt new_body = mutate(op->body);
        arg_scope.pop(op->name);
        stmt = For::make(op->name, op->min, op->extent, op->for_type, op->device_api, new_body);
        for(auto it=let_to_make.cbegin(); it!=let_to_make.cend(); ++it){
            stmt = LetStmt::make(it.name(), it.value(), stmt);
        }
    }
   
public:
    AddRevertLets(vector<string>& i, Scope<ExprInfo>& s) : input_scope(i), expr_info(s) {
        for(auto const& value: input_scope) {
            arg_scope.push(value, 0);
        }
    }
};

Stmt hwrevert_let(Stmt s) {
    vector<string> input_scope = extract_intput_scope(s);
    Scope<ExprInfo> expr_info;
    FindRevertArgs fra(expr_info);
    s = fra.mutate(s);
    AddRevertLets arl(input_scope, expr_info);
    s = arl.mutate(s);
    return s;
}

}
}
