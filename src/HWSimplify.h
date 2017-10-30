#ifndef HALIDE_HWSIMPLIFY_H
#define HALIDE_HWSIMPLIFY_H

/** \file
 *
 * Defines the simplification pass used in HLS optimization
 */

#include "IR.h"

namespace Halide {
namespace Internal {

/** Perform simplification optimization
 */
Stmt hwsimplify(Stmt s);
}
}

#endif
