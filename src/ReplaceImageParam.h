#ifndef HALIDE_REPLACE_IMAGE_PARAM_H
#define HALIDE_REPLACE_IMAGE_PARAM_H

/** \file
 *
 * Defines the tranformation pass that replaces ImageParam
 * used in HW pipeline with stencil data type
 */

#include "IR.h"
#include "ExtractHWKernelDAG.h"

namespace Halide {
namespace Internal {

/** Replace ImageParam used in HW pipeline with stencil data type
 */
Stmt replace_image_param(Stmt s, const HWKernelDAG &dag);

}
}

#endif
