#ifndef HALIDE_HWKERNEL_OPT_H
#define HALIDE_HWKERNEL_OPT_H

/** \file
 *
 * Defines the streaming optimization pass
 */

#include "IR.h"
#include "ExtractHWKernel.h"

namespace Halide {
namespace Internal {

/** Perform streaming and double buffering optimization
 */
Stmt hwkernel_opt(Stmt s, const HWKernelDAG &dag);

}
}

#endif
