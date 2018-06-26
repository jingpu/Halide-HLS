#ifndef HALIDE_HALIDERUNTIMEZYNQ_H
#define HALIDE_HALIDERUNTIMEZYNQ_H

#include "HalideRuntime.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file
 *  Routines specific to the Zynq runtime.
 */

#ifndef CMA_BUFFER_T_DEFINED
#define CMA_BUFFER_T_DEFINED
/** CMA (Continuous Memory Allocator) buffer struct passing
 * between Zynq low-level device driver calls. It stores
 * meta-data of a sub-image (2-D image tile), and the physical
 * address of the buffer used in DMA.
 *
 * It is originally defined in
 * https://github.com/stevenbell/zynqbuilder/blob/master/drivers/buffer.h
 */
struct mMap;
typedef struct cma_buffer_t {
  unsigned int id; // ID flag for internal use
  unsigned int width; // Width of the image
  unsigned int stride; // Stride between rows, in pixels. This must be >= width
  unsigned int height; // Height of the image
  unsigned int depth; // Byte-depth of the image
  unsigned int phys_addr; // Bus address for DMA
  void* kern_addr; // Kernel virtual address
  struct mMap* cvals;
  unsigned int mmap_offset;
} cma_buffer_t;
#endif


/**
 * Set Zynq runtime by providing char driver file descriptor
 */
extern int halide_zynq_set_fd(int hwacc, int cma);

/** Initialize Zynq runtime environment and must be called
    before any other function from the runtime API. */
extern int halide_zynq_init();

/** A special free function used in Zynq Target. It is emtpy. */
extern void halide_zynq_free(void *user_context, void *ptr);

/** Allocate and free a cma_buffer_t. Fields of the cma_buffer_t
 * are set up according to the buffer_t argument. A pointer to
 * the cma_buffer_t is stored in buf->dev. And buf->dev is reset
 * to zero after the cma_buffer_t is freed.
 */
// @{
extern int halide_zynq_cma_alloc(struct halide_buffer_t *buf);
extern int halide_zynq_cma_free(struct halide_buffer_t *buf);
// @}

/** Create a new cma_buffer_t representing a sub-image tile of IMAGE
 * buffer. The sub-image tile starts at the user space address
 * ADDRESS_OF_SUBIMAGE_ORIGIN, and is WIDTH wide and HEIGHT tall.
 * The function only calcuates the meta-data and re-use the memory
 * buffer, so no copying happens.
 */
extern int halide_zynq_subimage(const struct halide_buffer_t* image, struct cma_buffer_t* subimage, void *address_of_subimage_origin, int width, int height);

/** Launch a hardware accelerator run. BUFS stores the inputs and
 * output (sub-)image tiles used by DMAs.
 * The function returns immediately (non-blocking) with a task_id,
 * which can be used in other synchronization (blocking) functions.
 */
extern int halide_zynq_hwacc_launch(struct cma_buffer_t bufs[]);

/** Block inside the function until the accelerator run with
 * TASK_ID finishes. */
extern int halide_zynq_hwacc_sync(int task_id);

#ifdef __cplusplus
} // End extern "C"
#endif

#endif // HALIDE_HALIDERUNTIMEZYNQ_H
