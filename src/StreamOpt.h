#ifndef HALIDE_STREAM_OPT_H
#define HALIDE_STREAM_OPT_H

/** \file
 *
 * Defines the streaming optimization pass
 */

#include "IR.h"
#include "ExtractHWKernelDAG.h"

namespace Halide {
namespace Internal {

/** Perform streaming optimization
 */
Stmt stream_opt(Stmt s, const HWKernelDAG &dag);

}
}

#endif
