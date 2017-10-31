#ifndef HALIDE_HWREVERT_LET_H
#define HALIDE_HWREVERT_LET_H

/** \file
 *
 * Defines the pass used in HLS optimization that revert 
 * some let stmt that we do not wnat them to be substituted
 */

#include "IR.h"

namespace Halide {
namespace Internal {

/** Revert complex arguments in For loop to let stmt again
 */
Stmt hwrevert_let(Stmt s);
}
}

#endif
