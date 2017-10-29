#include "HWReplaceBack.h"
#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Simplify.h"

namespace Halide {
namespace Internal {

class HWReplaceBack : public IRMutator {
    using IRVisitor::visit;
    const Scope<Expr> &hw_replacement;
    void visit(const LetStmt *op) {
        if(hw_replacement.contains(op->name)){
            Expr new_value = hw_replacement.get(op->name);
            Stmt new_body = mutate(op->body);
            stmt = LetStmt::make(op->name, new_value, new_body);
        }else{
            IRVisitor::visit(op);
        }
    }
public:
    HWReplaceBack(const Scope<Expr> &s) : hw_replacement(s) {}
};

Stmt hwreplace_back(Stmt s, const Scope<Expr> &hw_replacemnt) {
    HWReplaceBack hw_rb(hw_replacemnt);
    Stmt result = hw_rb.mutate(s);
    return result;
}

}
}
