#ifndef HALIDE_HALIDERUNTIMEZYNQ_H
#define HALIDE_HALIDERUNTIMEZYNQ_H

#include "HalideRuntime.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file
 *  Routines specific to the Zynq runtime.
 */

/**
 * Userspace buffer
 */
#ifndef _UBUFFER_H_
#define _UBUFFER_H_

#ifdef __KERNEL__
#include <linux/types.h>
#define U32_TYPE    u32
#else
#include <stdint.h>
#define U32_TYPE    uint32_t
#endif /* __KERNEL__ */

/* user buffer declaration */
typedef struct UBuffer {
    U32_TYPE id;        // ID flag for internal use
    U32_TYPE offset;    // used for slicing purposes
                        // this is the offset in bytes
                        // from the beginning of root buffer
    U32_TYPE width;     // width of the image
    U32_TYPE height;    // height of the image
    U32_TYPE stride;    // stride of the image
    U32_TYPE depth;     // byte-depth of the image
} UBuffer;

#endif /* _UBUFFER_H_ */

/**
 * Set Zynq runtime by providing char driver file descriptor
 */
extern int halide_zynq_set_fd(int hwacc, int cma);

/** Initialize Zynq runtime environment and must be called
    before any other function from the runtime API. */
extern int halide_zynq_init();

/** A special free function used in Zynq Target. It is emtpy. */
extern void halide_zynq_free(void *user_context, void *ptr);

/** Allocate and free a UBuffer. Fields of the UBuffer
 * are set up according to the buffer_t argument. A pointer to
 * the UBuffer is stored in buf->dev. And buf->dev is reset
 * to zero after the UBuffer is freed.
 */
// @{
extern int halide_zynq_cma_alloc(struct halide_buffer_t *buf);
extern int halide_zynq_cma_free(struct halide_buffer_t *buf);
// @}

/** Create a new UBuffer representing a sub-image tile of IMAGE
 * buffer. The sub-image tile starts at the user space address
 * ADDRESS_OF_SUBIMAGE_ORIGIN, and is WIDTH wide and HEIGHT tall.
 * The function only calcuates the meta-data and re-use the memory
 * buffer, so no copying happens.
 */
extern int halide_zynq_subimage(const struct halide_buffer_t* image, struct UBuffer* subimage, void *address_of_subimage_origin, int width, int height);

/** Launch a hardware accelerator run. BUFS stores the inputs and
 * output (sub-)image tiles used by DMAs.
 * The function returns immediately (non-blocking) with a task_id,
 * which can be used in other synchronization (blocking) functions.
 */
extern int halide_zynq_hwacc_launch(struct UBuffer bufs[]);

/** Block inside the function until the accelerator run with
 * TASK_ID finishes. */
extern int halide_zynq_hwacc_sync(int task_id);

#ifdef __cplusplus
} // End extern "C"
#endif

#endif // HALIDE_HALIDERUNTIMEZYNQ_H
