/**
 * Zynq runtime API implementation, mostly copied from src/runtime/zynq.cpp
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "HalideRuntime.h"

#ifndef HALIDE_ATTRIBUTE_ALIGN
  #ifdef _MSC_VER
    #define HALIDE_ATTRIBUTE_ALIGN(x) __declspec(align(x))
  #else
    #define HALIDE_ATTRIBUTE_ALIGN(x) __attribute__((aligned(x)))
  #endif
#endif
#ifndef BUFFER_T_DEFINED
#define BUFFER_T_DEFINED
typedef struct buffer_t {
    uint64_t dev;
    uint8_t* host;
    int32_t extent[4];
    int32_t stride[4];
    int32_t min[4];
    int32_t elem_size;
    HALIDE_ATTRIBUTE_ALIGN(1) bool host_dirty;
    HALIDE_ATTRIBUTE_ALIGN(1) bool dev_dirty;
    HALIDE_ATTRIBUTE_ALIGN(1) uint8_t _padding[10 - sizeof(void *)];
} buffer_t;
#endif

#ifndef _IOCTL_CMDS_H_
#define _IOCTL_CMDS_H_

#define MAGIC			'Z'

/* cma driver */
#define GET_BUFFER 		_IOWR(MAGIC, 0x20, void *)	 // Get an unused buffer
#define FREE_BUFFER 	_IOWR(MAGIC, 0x21, void *)	 // Release buffer

/* dma driver */
#define ENROLL_BUFFER	_IOWR(MAGIC, 0x40, void *)
#define WAIT_COMPLETE	_IOWR(MAGIC, 0x41, void *)
#define STATUS_CHECK	_IOWR(MAGIC, 0x42, void *)
#define DMA_COMPLETED	2
#define DMA_IN_PROGRESS	1

/* hwacc */
#define GRAB_IMAGE 		_IOWR(MAGIC, 0x22, void *)	// Acquire image from camera
#define FREE_IMAGE		_IOWR(MAGIC, 0x23, void *)	// Release buffer
#define PROCESS_IMAGE	_IOWR(MAGIC, 0x24, void *)	// Push to stencil path
#define PEND_PROCESSED	_IOWR(MAGIC, 0x25, void *)	// Retreive from stencil path
#define READ_TIMER		_IOWR(MAGIC, 0x26, void *)	// Retreive hw timer count

/* stream generator */
#define RD_CONFIG		_IOWR(MAGIC, 0x30, void *)
#define RD_SIZE			_IOWR(MAGIC, 0x31, void *)
#define RD_STATUS		_IOWR(MAGIC, 0x32, void *)
#define WR_CONFIG		_IOWR(MAGIC, 0x33, void *)
#define WR_CTRL_SET		_IOWR(MAGIC, 0x34, void *)
#define WR_CTRL_CLR		_IOWR(MAGIC, 0x35, void *)
#define WR_SIZE			_IOWR(MAGIC, 0x36, void *)

#endif /* _IOCTL_CMDS_H_ */

#ifndef _UBUFFER_H_
#define _UBUFFER_H_

#ifdef __KERNEL__
	#include <linux/types.h>
	#define U32_TYPE	u32
#else
	#include <stdint.h>
	#define U32_TYPE	uint32_t
#endif /* __KERNEL__ */

/* user buffer declaration */
typedef struct UBuffer {
	U32_TYPE id;		// ID flag for internal use
    U32_TYPE offset;    // used for slicing purposes
                        // this is the offset in bytes
                        // from the beginning of root buffer
	U32_TYPE width;		// width of the image
	U32_TYPE height;	// height of the image
	U32_TYPE stride;	// stride of the image
	U32_TYPE depth;		// byte-depth of the image
} UBuffer;

#endif /* _UBUFFER_H_ */

#ifndef CMA_BUFFER_T_DEFINED
#define CMA_BUFFER_T_DEFINED
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

#ifdef __cplusplus
extern "C" {
#endif

// file descriptors of devices
static int fd_hwacc = 0;
static int fd_cma = 0;

/* Forward declaration */
int halide_zynq_subimage(const struct halide_buffer_t* image, struct UBuffer* subimage, void *address_of_subimage_origin, int width, int height);
void halide_zynq_free(void *user_context, void *ptr);
int halide_zynq_cma_alloc_fd(struct halide_buffer_t *buf, int cma);
int halide_zynq_cma_free_fd(struct halide_buffer_t *buf, int cma);
int halide_zynq_hwacc_launch_fd(struct UBuffer bufs[], int hwacc);
int halide_zynq_hwacc_sync_fd(int task_id, int hwacc);
void buffer_to_stencil(struct halide_buffer_t* image, struct UBuffer* stencil, int cma);

void halide_zynq_free(void *user_context, void *ptr) {
    // do nothing
}

void buffer_to_stencil(struct halide_buffer_t* image, struct UBuffer* stencil, int cma) {
    size_t nDims = image->dimensions;
    if (nDims < 2) {
        printf("buffer_t has less than 2 dimension, not supported in CMA driver.");
    }
    stencil->width = image->dim[nDims - 2].extent; // length of the array
    stencil->height = image->dim[nDims - 1].extent;
    stencil->depth = (image->type.bits + 7) / 8;
    if (stencil->height != 1)
        printf("stencil height is not 1\n");
    // we assume tap values are 1D array. hence its height = 1,
    // as a result, we can use this to tell tap/stream apart
    // TODO: may consider add another field to UBuffer to indicate the value
    // currently put mem address to the id(high) and stride(low) field
    uint64_t addr = (uint64_t)image->host;
    stencil->id = 0xFFFFFFFF & (addr >> 32);
    stencil->stride = 0xFFFFFFFF & addr;
}

/*
 * actual thread-independent implementation
 */
static int cma_get_buffer_fd(struct UBuffer* ptr, int cma) {
    if (!cma) {
        printf("fd cma is 0\n");
        return -1;
    }
    return ioctl(cma, GET_BUFFER, (void *)ptr);
}

static int cma_free_buffer_fd(struct UBuffer* ptr, int cma) {
    if (!cma) {
        printf("fd cma is 0\n");
        return -1;
    }
    return ioctl(cma, FREE_IMAGE, (void *)ptr);
}

int halide_zynq_cma_alloc_fd(struct halide_buffer_t *buf, int cma) {
    if (cma == 0) {
        printf("Zynq runtime is uninitialized.\n");
        return -1;
    }

    UBuffer *cbuf = (UBuffer *)malloc(sizeof(UBuffer));
    if (cbuf == NULL) {
        printf("malloc failed.\n");
        return -1;
    }

    // TODO check the strides of buf are monotonically increasing

    // Currently kernel buffer only supports 2-D data layout,
    // so we fold lower dimensions into the 'depth' field.
    size_t nDims = buf->dimensions;
    if (nDims < 2) {
        free(cbuf);
        printf("buffer_t has less than 2 dimension, not supported in CMA driver.");
        return -3;
    }
    cbuf->depth = (buf->type.bits + 7) / 8;
    if (nDims > 2) {
        for (size_t i = 0; i < nDims - 2; i++) {
            cbuf->depth *= buf->dim[i].extent;
        }
    }
    cbuf->width = buf->dim[nDims-2].extent;
    cbuf->height = buf->dim[nDims-1].extent;
    // TODO check stride of dimension are the same as width
    cbuf->stride = cbuf->width;
    int status = cma_get_buffer_fd(cbuf, cma);
    if (status != 0) {
        free(cbuf);
        printf("cma_get_buffer() returned %d (failed).\n", status);
        return -2;
    }
    uint32_t cma_buf_id = cbuf->id << 12;
    buf->device = (uint64_t) cbuf;
    buf->host = (uint8_t*) mmap(NULL, cbuf->stride * cbuf->height * cbuf->depth,
                                PROT_WRITE, MAP_SHARED, cma, cma_buf_id);

    if ((void *) buf->host == (void *) -1) {
        free(cbuf);
        printf("mmap failed. %s\n", strerror(errno));
        return -3;
    }
    return 0;
}

int halide_zynq_cma_free_fd(struct halide_buffer_t *buf, int cma) {
    if (cma == 0) {
        printf("Zynq runtime is uninitialized.\n");
        return -1;
    }

    UBuffer *cbuf = (UBuffer *)buf->device;
    munmap((void*)buf->host, cbuf->stride * cbuf->height * cbuf->depth);
    cma_free_buffer_fd(cbuf, cma);
    free(cbuf);
    return 0;
}

int halide_zynq_subimage(const struct halide_buffer_t* image, struct UBuffer* subimage, void *address_of_subimage_origin, int width, int height) {
    *subimage = *((UBuffer *)image->device); // copy depth, stride, data, etc.
    subimage->width = width;
    subimage->height = height;
    size_t offset = (uint8_t *)address_of_subimage_origin - image->host;
    
    subimage->offset = offset; 

    return 0;
}

int halide_zynq_hwacc_launch_fd(struct UBuffer bufs[], int hwacc) {
    if (hwacc == 0) {
        printf("Zynq runtime is uninitialized.\n");
        return -1;
    }
    int res = ioctl(hwacc, PROCESS_IMAGE, (long unsigned int)bufs);
    return res;
}

int halide_zynq_hwacc_sync_fd(int task_id, int hwacc) {
    if (hwacc == 0) {
        printf("Zynq runtime is uninitialized.\n");
        return -1;
    }
    int res = ioctl(hwacc, PEND_PROCESSED, (long unsigned int)task_id);
    return res;
}


#ifdef __cplusplus
} // End extern "C"
#endif
