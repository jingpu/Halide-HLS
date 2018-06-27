#include "HalideRuntimeZynq.h"
#include "printer.h"

#ifndef _IOCTL_CMDS_H_
#define _IOCTL_CMDS_H_

// because LLVM/Halide tries to compile cross-platform, system-depedent calls such as
// <sys/ioctl.h> can not be included normally. Copying over their macro to here
// since Zynq is only valid for Linux os.

#define _IOC_NRBITS 8
#define _IOC_TYPEBITS   8
#define _IOC_SIZEBITS   13
#define _IOC_DIRBITS    3

#define _IOC_NRMASK ((1 << _IOC_NRBITS)-1)
#define _IOC_TYPEMASK   ((1 << _IOC_TYPEBITS)-1)
#define _IOC_SIZEMASK   ((1 << _IOC_SIZEBITS)-1)
#define _IOC_DIRMASK    ((1 << _IOC_DIRBITS)-1)

#define _IOC_NRSHIFT    0
#define _IOC_TYPESHIFT  (_IOC_NRSHIFT+_IOC_NRBITS)
#define _IOC_SIZESHIFT  (_IOC_TYPESHIFT+_IOC_TYPEBITS)
#define _IOC_DIRSHIFT   (_IOC_SIZESHIFT+_IOC_SIZEBITS)

#define _IOC_NONE   1U
#define _IOC_READ   2U
#define _IOC_WRITE  4U

#define _IOC(dir,type,nr,size)          \
    ((unsigned int)             \
     (((dir)  << _IOC_DIRSHIFT) |       \
      ((type) << _IOC_TYPESHIFT) |      \
      ((nr)   << _IOC_NRSHIFT) |        \
      ((size) << _IOC_SIZESHIFT)))


#define _IOWR(type,nr,size) _IOC(_IOC_READ|_IOC_WRITE,(type),(nr),sizeof(size))

#define MAGIC           'Z'

/* cma driver */
#define GET_BUFFER      _IOWR(MAGIC, 0x20, void *)   // Get an unused buffer
#define FREE_BUFFER     _IOWR(MAGIC, 0x21, void *)   // Release buffer

/* dma driver */
#define ENROLL_BUFFER   _IOWR(MAGIC, 0x40, void *)
#define WAIT_COMPLETE   _IOWR(MAGIC, 0x41, void *)
#define STATUS_CHECK    _IOWR(MAGIC, 0x42, void *)
#define DMA_COMPLETED   2
#define DMA_IN_PROGRESS 1

/* hwacc */
#define GRAB_IMAGE      _IOWR(MAGIC, 0x22, void *)  // Acquire image from camera
#define FREE_IMAGE      _IOWR(MAGIC, 0x23, void *)  // Release buffer
#define PROCESS_IMAGE   _IOWR(MAGIC, 0x24, void *)  // Push to stencil path
#define PEND_PROCESSED  _IOWR(MAGIC, 0x25, void *)  // Retreive from stencil path
#define READ_TIMER      _IOWR(MAGIC, 0x26, void *)  // Retreive hw timer count

#endif /* _IOCTL_CMDS_H_ */

extern "C" {

// forward declarations of some POSIX APIs
/* open-only flags */
#define O_RDONLY    0x0000      /* open for reading only */
#define O_WRONLY    0x0001      /* open for writing only */
#define O_RDWR      0x0002      /* open for reading and writing */
#define O_ACCMODE   0x0003      /* mask for above modes */
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
        halide_zynq_set_fd(0, 0);
        return -2;
    }
    fd_hwacc = open("/dev/hwacc0", O_RDWR, 0644);
    if(fd_hwacc == -1) {
        error(NULL) << "Failed to open hwacc device!\n";
        close(fd_cma);
        halide_zynq_set_fd(0, 0);
        return -2;
    }
    return 0;
}

WEAK void halide_zynq_free(void *user_context, void *ptr) {
    debug(0) << "halide_zynq_free\n";
    // do nothing
}

static int cma_get_buffer(UBuffer* ptr) {
    return ioctl(fd_cma, GET_BUFFER, (long unsigned int)ptr);
}

static int cma_free_buffer(UBuffer* ptr) {
    return ioctl(fd_cma, FREE_IMAGE, (long unsigned int)ptr);
}

WEAK int halide_zynq_cma_alloc(struct halide_buffer_t *buf) {
    debug(0) << "halide_zynq_cma_alloc\n";
    if (fd_cma == 0) {
        error(NULL) << "Zynq runtime is uninitialized.\n";
        return -1;
    }

    UBuffer *cbuf = (UBuffer *)malloc(sizeof(UBuffer));
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
        error(NULL) << "buffer_t has less than 2 dimension, not supported in CMA driver.\n";
        return -3;
    }
    cbuf->depth = buf->type.bytes();
    if (nDims > 2) {
        for (size_t i = 0; i < nDims - 2; i++) {
            cbuf->depth *= buf->dim[i].extent;
        }
    }
    cbuf->width = buf->dim[nDims-2].extent;
    cbuf->height = buf->dim[nDims-1].extent;
    // TODO check stride of dimension are the same as width
    cbuf->stride = cbuf->width;
    int status = cma_get_buffer(cbuf);
    if (status != 0) {
        free(cbuf);
        error(NULL) << "cma_get_buffer() returned " << status << " (failed).\n";
        return -2;
    }
    // use mem offset filed as cambuffer ID
    // notice that since page size is 4K, the kernel will automatically
    // shift the mem offset >> by 12. Hence we need to shift left 12 bits
    uint32_t cma_buf_id = cbuf->id << 12;
    buf->device = (uint64_t) cbuf;
    buf->host = (uint8_t*) mmap(NULL, cbuf->stride * cbuf->height * cbuf->depth,
                                PROT_WRITE, MAP_SHARED, fd_cma, cma_buf_id);

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
    UBuffer *cbuf = (UBuffer *)buf->device;
    munmap((void*)buf->host, cbuf->stride * cbuf->height * cbuf->depth);
    cma_free_buffer(cbuf);
    free(cbuf);
    buf->device = 0;
    return 0;
}

WEAK int halide_zynq_subimage(const struct halide_buffer_t* image, struct UBuffer* subimage, void *address_of_subimage_origin, int width, int height) {
    debug(0) << "halide_zynq_subimage\n";
    *subimage = *((UBuffer *)image->device); // copy depth, stride, data, etc.
    subimage->width = width;
    subimage->height = height;
    size_t offset = (uint8_t *)address_of_subimage_origin - image->host;

    //subimage->phys_addr += offset;
    //subimage->mmap_offset += offset;
    subimage->offset = offset;

    return 0;
}

WEAK int halide_zynq_hwacc_launch(struct UBuffer bufs[]) {
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
