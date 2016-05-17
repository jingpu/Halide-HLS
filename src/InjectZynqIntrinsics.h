#ifndef HALIDE_INJECT_ZYNQ_INTRINSICS_H
#define HALIDE_INJECT_ZYNQ_INTRINSICS_H

/** \file
 * Defines the lowering pass that injects zynq platform related intrinsics
 */

#include <map>

#include "IR.h"

namespace Halide {
namespace Internal {

/** Inject Zynq platform specific allocation call for buffers shared
 * between FPGA and CPU. */
Stmt inject_zynq_intrinsics(Stmt s,
                            const std::map<std::string, Function> &env);
}
}

#endif
