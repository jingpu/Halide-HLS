#include "HalideRuntimeZynq.h"
#include "printer.h"

#ifndef _IOCTL_CMDS_H_
#define _IOCTL_CMDS_H_

// TODO: switch these out for "proper" mostly-system-unique ioctl numbers
#define GET_BUFFER 1000 // Get an unused buffer
#define GRAB_IMAGE 1001 // Acquire image from camera
#define FREE_IMAGE 1002 // Release buffer
#define PROCESS_IMAGE 1003 // Push to stencil path
#define PEND_PROCESSED 1004 // Retreive from stencil path

#endif

extern "C" {

// forward declarations of some POSIX APIs
/* open-only flags */
#define	O_RDONLY	0x0000		/* open for reading only */
#define	O_WRONLY	0x0001		/* open for writing only */
#define	O_RDWR		0x0002		/* open for reading and writing */
#define	O_ACCMODE	0x0003		/* mask for above modes */
/* mmap-only flags */
#define PROT_WRITE       0x2
#define MAP_SHARED       0x01
typedef int32_t off_t; // FIXME this is not actually correct
extern int open(const char *pathname, int flags, int mode);
extern int ioctl(int fd, unsigned long cmd, ...);
extern void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
extern int munmap(void *addr, size_t length);


// file descriptors of devices
static int fd_hwacc = 0;
static int fd_cma = 0;

WEAK int halide_zynq_set_fd(int hwacc, int cma) {
    if (!hwacc) {
        error(NULL) << "hwacc is uninitialized\n";
        return -1;
    }
    if (!cma) {
        error(NULL) << "cma is uninitialized\n";
        return -1;
    }
    fd_hwacc = hwacc;
    fd_cma = cma;
    return 0;
}

WEAK int halide_zynq_init() {
    debug(0) << "halide_zynq_init\n";
    if (fd_cma || fd_hwacc) {
        error(NULL) << "Zynq runtime is already initialized.\n";
        return -1;
    }
    fd_cma = open("/dev/cmabuffer0", O_RDWR, 0644);
    if(fd_cma == -1) {
        error(NULL) << "Failed to open cma provider!\n";
        fd_cma = fd_hwacc = 0;
        return -2;
    }
    fd_hwacc = open("/dev/hwacc0", O_RDWR, 0644);
    if(fd_hwacc == -1) {
        error(NULL) << "Failed to open hwacc device!\n";
        close(fd_cma);
        fd_cma = fd_hwacc = 0;
        return -2;
    }
    return 0;
}

WEAK void halide_zynq_free(void *user_context, void *ptr) {
    debug(0) << "halide_zynq_free\n";
    // do nothing
}

static int cma_get_buffer(cma_buffer_t* ptr) {
    return ioctl(fd_cma, GET_BUFFER, (long unsigned int)ptr);
}

static int cma_free_buffer(cma_buffer_t* ptr) {
    return ioctl(fd_cma, FREE_IMAGE, (long unsigned int)ptr);
}

WEAK int halide_zynq_cma_alloc(struct halide_buffer_t *buf) {
    debug(0) << "halide_zynq_cma_alloc\n";
    if (fd_cma == 0) {
        error(NULL) << "Zynq runtime is uninitialized.\n";
        return -1;
    }

    cma_buffer_t *cbuf = (cma_buffer_t *)malloc(sizeof(cma_buffer_t));
    if (cbuf == NULL) {
        error(NULL) << "malloc failed.\n";
        return -1;
    }

    // TODO check the strides of buf are monotonically increasing

    // Currently kernel buffer only supports 2-D data layout,
    // so we fold lower dimensions into the 'depth' field.
    size_t nDims = buf->dimensions;
    if (nDims < 2) {
        free(cbuf);
        error(NULL) << "buffer_t has less than 2 dimension, not supported in CMA driver.";
        return -3;
    }
    cbuf->depth = buf->type.bytes();
    if (nDims > 2) {
        for (size_t i = 0; i < nDims - 2; i++)
            cbuf->depth *= buf->dim[i].extent;
    }
    cbuf->width = buf->dim[nDims-2].extent;
    cbuf->height = buf->dim[nDims-1].extent;
    // TODO check stride of dimension are the same as width
    cbuf->stride = cbuf->width;

    int status = cma_get_buffer(cbuf);
    if (status != 0) {
        free(cbuf);
        error(NULL) << "cma_get_buffer() returned" << status << " (failed).\n";
        return -2;
    }

    buf->device = (uint64_t) cbuf;
    buf->host = (uint8_t*) mmap(NULL, cbuf->stride * cbuf->height * cbuf->depth,
                                PROT_WRITE, MAP_SHARED, fd_cma, cbuf->mmap_offset);

    if ((void *) buf->host == (void *) -1) {
        free(cbuf);
        error(NULL) << "mmap failed.\n";
        return -3;
    }
    return 0;
}

WEAK int halide_zynq_cma_free(struct halide_buffer_t *buf) {
    debug(0) << "halide_zynq_cma_free\n";
    if (fd_cma == 0) {
        error(NULL) << "Zynq runtime is uninitialized.\n";
        return -1;
    }

    cma_buffer_t *cbuf = (cma_buffer_t *)buf->device;
    munmap((void*)buf->host, cbuf->stride * cbuf->height * cbuf->depth);
    cma_free_buffer(cbuf);
    free(cbuf);
    buf->device = 0;
    return 0;
}

WEAK int halide_zynq_subimage(const struct halide_buffer_t* image, struct cma_buffer_t* subimage, void *address_of_subimage_origin, int width, int height) {
    debug(0) << "halide_zynq_subimage\n";
    *subimage = *((cma_buffer_t *)image->device); // copy depth, stride, data, etc.
    subimage->width = width;
    subimage->height = height;
    size_t offset = (uint8_t *)address_of_subimage_origin - image->host;
    subimage->phys_addr += offset;
    subimage->mmap_offset += offset;
    return 0;
}

WEAK int halide_zynq_hwacc_launch(struct cma_buffer_t bufs[]) {
    debug(0) << "halide_zynq_hwacc_launch\n";
    if (fd_hwacc == 0) {
        error(NULL) << "Zynq runtime is uninitialized.\n";
        return -1;
    }
    int res = ioctl(fd_hwacc, PROCESS_IMAGE, (long unsigned int)bufs);
    return res;
}

WEAK int halide_zynq_hwacc_sync(int task_id){
    debug(0) << "halide_zynq_hwacc_sync\n";
    if (fd_hwacc == 0) {
        error(NULL) << "Zynq runtime is uninitialized.\n";
        return -1;
    }
    int res = ioctl(fd_hwacc, PEND_PROCESSED, (long unsigned int)task_id);
    return res;
}

}
