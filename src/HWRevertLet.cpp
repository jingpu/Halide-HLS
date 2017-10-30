#include "HWRevertLet.h"
#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Simplify.h"

namespace Halide {
namespace Internal {

using std::string;
using std::vector;

class ExtractExprScope : public IRMutator {
    using IRVisitor::visit;
    vector<string> hidden;

    void visit(const Variable *op){
        hidden.push_back(op->name);
        debug(0) << "Find hidden: " << op->name << "\n";
        IRVisitor::visit(op);
    }
public:
    vector<string> get_hidden(){
        return hidden;
    }
}

vector<string> extract_expr_scope(Expr e){
    ExtractExprScope evs;
    evs.mutate(e);
    return evs.get_hidden();
}

class FindRevertArgs : public IRMutator {
    using IRMutator::visit;
    struct ExprInfo {
        Expr value;
        vector<string> expr_scope;
    };
    Scope<ExprInfo> expr_info;
    
    void visit(const For *op) {
        string loop_name = op->name;
        Expr loop_extent = op->extent;
        const Variable *var = loop_extent.as<Variable>();
        if(!(is_const(loop_extent)||var)){
            debug(0) << "Find loop extent that is not a const: " << loop_name << " " << loop_extent << "\n";
            vector<string> hidden_scope = extract_expr_scope(loop_extent);
            string loop_extent_var_name = op->name + ".loop_extent";
            Expr loop_extent_var = Variable::make(loop_extent.type().element_of(), loop_extent_var_name);
            ExprInfo info;
            info.value = loop_extent;
            info.expr_scope = hidden_scope;
            expr_info.push(loop_extent_var_name, info);
            Stmt new_body = mutate(op->body);
            // To be corrected later
            stmt = For::make(op->name, op->min, op->extent, op->for_type, op->device_api, new_body);
        }else{
            IRMutator::visit(op);
        }
    }
};

Stmt hwrevert_let(Stmt s) {
    Stmt result = FindRevertArgs().mutate(s);
    return result;
}

}
}
