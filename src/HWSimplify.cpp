#include "HWSimplify.h"
#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Simplify.h"

namespace Halide {
namespace Internal {

class HWSimplify : public IRMutator {
    using IRMutator::visit;
    struct VarInfo {
        Expr replacement;
        int old_uses, new_uses;
    };
    Scope<VarInfo> var_info;

    void visit(const Variable *op) {
        if (var_info.contains(op->name)) {
            VarInfo &info = var_info.ref(op->name);

            // if replacement is defined, we should substitute it in (unless
            // it's a var that has been hidden by a nested scope).
            if (info.replacement.defined()) {
                internal_assert(info.replacement.type() == op->type) << "Cannot replace variable " << op->name
                    << " of type " << op->type << " with expression of type " << info.replacement.type() << "\n";
                expr = info.replacement;
                info.new_uses++;
            } else {
                // This expression was not something deemed
                // substitutable - no replacement is defined.
                expr = op;
                info.old_uses++;
            }
        } else {
            // We never encountered a let that defines this var. Must
            // be a uniform. Don't touch it.
            expr = op;
        }
    }
    void visit(const LetStmt *op) {
        if(ends_with(op->name, ".base")) {
            stmt = simplify_let<LetStmt, Stmt>(op);
        }else{
            IRMutator::visit(op);
        }
    }

    template<typename T, typename Body>
    Body simplify_let(const T *op) {
        internal_assert(!var_info.contains(op->name))
            << "Simplify only works on code where every name is unique. Repeated name: " << op->name << "\n";

        Expr value = mutate(op->value);
        Body body = op->body;

        Expr replacement = value;

        VarInfo info;
        info.old_uses = 0;
        info.new_uses = 0;
        info.replacement = replacement;

        var_info.push(op->name, info);

        body = mutate(body);

        info = var_info.get(op->name);
        var_info.pop(op->name);

        Body result = body;

        if (info.old_uses > 0) {
            // The old name is still in use. We'd better keep it as well.
            result = T::make(op->name, value, result);
        }


        return result;

    }
};

Stmt hwsimplify(Stmt s) {
    Stmt result = HWSimplify().mutate(s);
    return result;
}

}
}
