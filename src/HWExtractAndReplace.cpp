#include "HWExtractAndReplace.h"
#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Simplify.h"

namespace Halide {
namespace Internal {

class HWExtractAndReplace : public IRMutator {
    using IRVisitor::visit;
    Scope<Expr> &hw_replacement;
    void visit(const LetStmt *op) {
        if(ends_with(op->name, "loop_extent")){
            Expr value = op->value;
            const Div *div = value.as<Div>();
            // This is the situation we want to fix in Simplify
            if(div && !is_const(div->a)){
                hw_replacement.push(op->name, op->value);
                Stmt new_body = mutate(op->body);
                Expr new_value = Div::make(div->a, div->b); // should be careful with this
                stmt = LetStmt::make(op->name, new_value, new_body);
            }else{
                IRVisitor::visit(op);
            }
        }else{
            IRVisitor::visit(op);
        }
    }

public:
    HWExtractAndReplace(Scope<Expr> &s) : hw_replacement(s) {}
};

Stmt hwextract_and_replace(Stmt s, Scope<Expr> &hw_replacemnt) {
    HWExtractAndReplace hw_ear(hw_replacemnt);
    Stmt result = hw_ear.mutate(s);
    return result;
}

}
}
