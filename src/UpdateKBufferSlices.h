#ifndef HALIDE_UPDATE_KBUFFER_SLICES_H
#define HALIDE_UPDATE_KBUFFER_SLICES_H

/** \file
 *
 * Defines the tranformation pass that updates the storage domains
 * of kernel buffer slices.
 */

#include "IR.h"

namespace Halide {
namespace Internal {

/** Update the min positions of the storage domains of kernel buffer
 * slices given the min of kernel buffer allocations
 */
Stmt update_kbuffer_slices(Stmt s);

}
}

#endif
