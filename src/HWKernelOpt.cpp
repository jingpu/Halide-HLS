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
    s = add_func_constraints(s, dag);
    debug(3) << "after add output constraints:\n" << s << "\n";
    //s = push_init_into_update(s, dag);
    //debug(0) << "after push def into update:\n" << s << "\n";
    s = HWKernelOpt().mutate(s);
    debug(3) << s << "\n";
    return s;
}

}
}
