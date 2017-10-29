#ifndef HALIDE_HWREPLACE_BACK_H
#define HALIDE_HWREPLACE_BACK_H

/** \file
 * Defines the lowering pass that replaces back value of let node assigned in 
 * hwextract_and_replace pass
 */

#include "IR.h"
#include "Scope.h"

namespace Halide {
namespace Internal {

/** Replace back value of let stmt assigned in hwextract_and_replace
 */
Stmt hwreplace_back(Stmt s, const Scope<Expr> &hw_replacemnt);

}
}

#endif
