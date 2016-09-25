#include "InlineReductions.h"
#include "Func.h"
#include "Scope.h"
#include "IROperator.h"
#include "IRMutator.h"
#include "Debug.h"
#include "CSE.h"
#include "Simplify.h"

namespace Halide {

using std::string;
using std::vector;
using std::ostringstream;

namespace Internal {

class FindFreeVars : public IRMutator {
public:
    const string &name;
    vector<Var> free_vars;
    vector<Expr> call_args;
    RDom rdom;

    FindFreeVars(RDom r, const string &n) :
        name(n), rdom(r), explicit_rdom(r.defined()) {
    }

private:
    bool explicit_rdom;

    Scope<int> internal;

    using IRMutator::visit;

    void visit(const Let *op) {
        Expr value = mutate(op->value);
        internal.push(op->name, 0);
        Expr body = mutate(op->body);
        internal.pop(op->name);
        if (value.same_as(op->value) &&
            body.same_as(op->body)) {
            expr = op;
        } else {
            expr = Let::make(op->name, value, body);
        }
    }

    void visit(const Variable *v) {

        string var_name = v->name;
        expr = v;

        if (internal.contains(var_name)) {
            // Don't capture internally defined vars
            return;
        }

        if (v->reduction_domain.defined()) {
            if (explicit_rdom) {
                if (v->reduction_domain.same_as(rdom.domain())) {
                    // This variable belongs to the explicit reduction domain, so
                    // skip it.
                    return;
                } else {
                    // This should be converted to a pure variable and
                    // added to the free vars list.
                    var_name = unique_name('v');
                    expr = Variable::make(v->type, var_name);
                }
            } else {
                if (!rdom.defined()) {
                    // We're looking for a reduction domain, and this variable
                    // has one. Capture it.
                    rdom = RDom(v->reduction_domain);
                    return;
                } else if (!rdom.domain().same_as(v->reduction_domain)) {
                    // We were looking for a reduction domain, and already
                    // found one. This one is different!
                    user_error << "Inline reduction \"" << name
                               << "\" refers to reduction variables from multiple reduction domains: "
                               << v->name << ", " << rdom.x.name() << "\n";
                } else {
                    // Recapturing an already-known reduction domain
                    return;
                }
            }
        }

        if (v->param.defined()) {
            // Skip parameters
            return;
        }

        for (size_t i = 0; i < free_vars.size(); i++) {
            if (var_name == free_vars[i].name()) return;
        }

        free_vars.push_back(Var(var_name));
        call_args.push_back(v);
    }
};
}

using namespace Internal;

Expr sum(Expr e, const std::string &name) {
    return sum(RDom(), e, name);
}

Expr sum(RDom r, Expr e, const std::string &name) {
    Internal::FindFreeVars v(r, name);
    e = v.mutate(common_subexpression_elimination(e));

    user_assert(v.rdom.defined()) << "Expression passed to sum must reference a reduction domain";

    Func f(name);
    f(v.free_vars) += e;
    return f(v.call_args);
}

Expr product(Expr e, const std::string &name) {
    return product(RDom(), e, name);
}

Expr product(RDom r, Expr e, const std::string &name) {
    Internal::FindFreeVars v(r, name);
    e = v.mutate(common_subexpression_elimination(e));

    user_assert(v.rdom.defined()) << "Expression passed to product must reference a reduction domain";

    Func f(name);
    f(v.free_vars) *= e;
    return f(v.call_args);
}

Expr maximum(Expr e, const std::string &name) {
    return maximum(RDom(), e, name);
}

Expr maximum(RDom r, Expr e, const std::string &name) {
    Internal::FindFreeVars v(r, name);
    e = v.mutate(common_subexpression_elimination(e));

    user_assert(v.rdom.defined()) << "Expression passed to maximum must reference a reduction domain";

    Func f(name);
    f(v.free_vars) = e.type().min();
    f(v.free_vars) = max(f(v.free_vars), e);
    return f(v.call_args);
}

Expr minimum(Expr e, const std::string &name) {
    return minimum(RDom(), e, name);
}

Expr minimum(RDom r, Expr e, const std::string &name) {
    Internal::FindFreeVars v(r, name);
    e = v.mutate(common_subexpression_elimination(e));

    user_assert(v.rdom.defined()) << "Expression passed to minimum must reference a reduction domain";

    Func f(name);
    f(v.free_vars) = e.type().max();
    f(v.free_vars) = min(f(v.free_vars), e);
    return f(v.call_args);
}

Func argmaxmin_func(Expr e, const Internal::FindFreeVars &v, bool is_argmax) {
    Func f(v.name);

    Tuple initial_tup(vector<Expr>(v.rdom.dimensions()+1));
    Tuple update_tup(vector<Expr>(v.rdom.dimensions()+1));
    for (int i = 0; i < v.rdom.dimensions(); i++) {
        initial_tup[i] = 0;
        update_tup[i] = v.rdom[i];
    }
    int value_index = (int)initial_tup.size()-1;
    initial_tup[value_index] = is_argmax ? e.type().min() : e.type().max();
    update_tup[value_index] = e;

    f(v.free_vars) = initial_tup;
    Expr better = is_argmax ? e > f(v.free_vars)[value_index] :
        e < f(v.free_vars)[value_index];
    f(v.free_vars) = tuple_select(better, update_tup, f(v.free_vars));
    return f;
}

Tuple argmax(Expr e, const std::string &name) {
    return argmax(RDom(), e, name);
}

Tuple argmax(RDom r, Expr e, const std::string &name) {
    Internal::FindFreeVars v(r, name);
    e = v.mutate(common_subexpression_elimination(e));

    user_assert(v.rdom.defined()) << "Expression passed to argmax must reference a reduction domain";

    Func f = argmaxmin_func(e, v, true);
    return f(v.call_args);
}

Tuple argmin(Expr e, const std::string &name) {
    return argmin(RDom(), e, name);
}


Tuple argmin(RDom r, Expr e, const std::string &name) {
    Internal::FindFreeVars v(r, name);
    e = v.mutate(common_subexpression_elimination(e));

    user_assert(v.rdom.defined()) << "Expression passed to argmin must reference a reduction domain";

    Func f = argmaxmin_func(e, v, false);
    return f(v.call_args);
}

void argmaxmin_tree_split(Func f,  int radix, const Internal::FindFreeVars &v, bool is_argmax) {
    // Tree-like split-rfactor-unroll
    // split each RVar by a factor of radix, and create a tree structure
    // of imtermediate functions to calculate argmax/argmin
    // schedule the computation of funtions in BFS order

    // get the string after the last '.'
    auto get_var_name = [](const string &full_name) {
        size_t pos = full_name.find_last_of('.');
        if (pos != string::npos) {
            internal_assert(pos + 1 < full_name.size());
            return full_name.substr(pos + 1);
        } else {
            return full_name;
        }};

    vector<ReductionVariable> &rvars = f.function().update_schedule(0).rvars();
    while (true) {
        internal_assert(!rvars.empty());
        RDom rdom(rvars);
        vector<std::pair<RVar, Var>> preserved;

        // try to factor the first RVar
        if (rvars.size() == 1 && is_one(simplify(rvars[0].extent <= radix))) {
            // stop if the extent of the only RVar is not greater than radix
            break;
        } else if (is_one(simplify(rvars[0].extent > radix))) {
            // split RVar if the extent is larger than radix
            RVar r(get_var_name(rvars[0].var));
            RVar ri, ro;
            Var u;
            f.update(0).split(r, ro, ri, radix);
            preserved.push_back({ro, u});
        }

        // preserve other RVar
        for (size_t i = 1; i < rvars.size(); i++) {
            RVar r(get_var_name(rvars[i].var));
            Var u;
            preserved.push_back({r, u});
        }

        Func intm = f.update(0).argmaxmin_rfactor(preserved, is_argmax);
        // schedules on the intermediate function
        if (v.free_vars.empty()) {
            intm.compute_root();
        } else {
            intm.compute_at(f, v.free_vars.back());
        }
    }
}

Tuple argmax_tree(Expr e, int radix, const std::string &name) {
    return argmax_tree(RDom(), e, radix, name);
}

Tuple argmin_tree(Expr e, int radix, const std::string &name) {
    return argmin_tree(RDom(), e, radix, name);
}

Tuple argmax_tree(RDom r, Expr e, int radix, const std::string &name) {
    Internal::FindFreeVars v(r, name);
    e = v.mutate(common_subexpression_elimination(e));

    user_assert(v.rdom.defined()) << "Expression passed to argmax must reference a reduction domain";
    user_assert(radix > 1) << "The radix of the tree reduction should be larger than";

    Func f = argmaxmin_func(e, v, true);
    argmaxmin_tree_split(f, radix, v, true);

    return f(v.call_args);
}

Tuple argmin_tree(RDom r, Expr e, int radix, const std::string &name) {
    Internal::FindFreeVars v(r, name);
    e = v.mutate(common_subexpression_elimination(e));

    user_assert(v.rdom.defined()) << "Expression passed to argmin must reference a reduction domain";
    user_assert(radix > 1) << "The radix of the tree reduction should be larger than";

    Func f = argmaxmin_func(e, v, false);
    argmaxmin_tree_split(f, radix, v, false);

    return f(v.call_args);
}
}

