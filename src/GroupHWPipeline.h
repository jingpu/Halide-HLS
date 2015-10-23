#ifndef HALIDE_GROUP_HW_PIPELINE_H
#define HALIDE_GROUP_HW_PIPELINE_H

/** \file
 *
 * Defines the tranformation pass that group HW pipeline together
 */

#include "IR.h"
#include "ExtractHWKernelDAG.h"

namespace Halide {
namespace Internal {

/** Group the HW pipeline of dag into a subtree in IR
 */
Stmt group_hw_pipeline(Stmt s, const HWKernelDAG &dag);

}
}

#endif
