#ifndef HALIDE_HWEXTRACT_AND_REPLACE_H
#define HALIDE_HWEXTRACT_AND_REPLACE_H

/** \file
 * Defines the lowering pass that extracts and replaces some let node that
 * we do not want to simplify
 */

#include "IR.h"
#include "Scope.h"

namespace Halide {
namespace Internal {

/** Find let node about loop_extent that is not cheap to compute in HLS setting
 * and replace them so that they will not be simplified by simplify function
 */
Stmt hwextract_and_replace(Stmt s, Scope<Expr> &hw_replacemnt);

}
}

#endif
