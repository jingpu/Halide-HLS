#ifndef HALIDE_PERFECT_NESTED_LOOPS_H
#define HALIDE_PERFECT_NESTED_LOOPS_H

/** \file
 *
 * Defines the tranformation pass that perfect nested loops
 * that implement HW kernels. It is helpful for HLS tool
 * to better pipeline the nested loops.
 */

#include "IR.h"

namespace Halide {
namespace Internal {

/** Transforms imperfect nested loops that implement HW kernels
 * into perfect nested loops
 */
Stmt perfect_nested_loops(Stmt s);

}
}

#endif
