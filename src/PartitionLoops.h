#ifndef PARTITION_LOOPS_H
#define PARTITION_LOOPS_H

/** \file
 * Defines a lowering pass that partitions loop bodies into three
 * to handle boundary conditions: A prologue, a simplified
 * steady-stage, and an epilogue.
 */
#include <map>
#include "IR.h"

namespace Halide {
namespace Internal {

/** Partitions loop bodies into a prologue, a steady state, and an
 * epilogue. Finds the steady state by hunting for use of clamped
 * ramps, or the 'likely' intrinsic. */
EXPORT Stmt partition_loops(Stmt s, const std::map<std::string, Function> &env);

}
}

#endif
