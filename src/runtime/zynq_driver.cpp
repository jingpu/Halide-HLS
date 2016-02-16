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
extern int ioctl(int fd, unsigned long cmd, ...);
extern void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
extern int munmap(void *addr, size_t length);

WEAK int halide_slice_kbuf(kbuf_t* src, kbuf_t* des, int x, int y, int width, int height) {
    debug(0) << "slicing kbuf...\n";
    /*
      if (width == 0 || height == 0) {
      printf("slice_buffer failed: width and height of slide should be non-zero.\n");
      return -1;
      }
      if (x + width > src->width || y + height > src->height) {
      printf("slice_buffer failed: slice is out of range.\n");
      return -1;
      }
    */
    *des = *src; // copy depth, stride, data, etc.
    des->width = width;
    des->height = height;
    des->phys_addr += src->depth * (y * src->stride + x);
    des->mmap_offset += src->depth * (y * src->stride + x);
    return 0;
}


WEAK int halide_alloc_kbuf(int fd, kbuf_t* ptr) {
    debug(0) << "allocating kbuf...\n";
    return ioctl(fd, GET_BUFFER, (long unsigned int)ptr);
}

WEAK int halide_free_kbuf(int fd, kbuf_t* ptr) {
    debug(0) << "freeing kbuf.\n";
    return ioctl(fd, FREE_IMAGE, (long unsigned int)ptr);
}

WEAK int halide_process_image(int fd, kbuf_t* ptr) {
    debug(0) << "start processing image.\n";
    return ioctl(fd, PROCESS_IMAGE, (long unsigned int)ptr);
}

WEAK int halide_pend_processed(int fd, int id) {
    debug(0) << "pending processing to finish...\n";
    return ioctl(fd, PEND_PROCESSED, (long unsigned int)id);
}

WEAK void *halide_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    return mmap(addr, length, prot, flags, fd, offset);
}

WEAK int halide_munmap(void *addr, size_t length) {
    return munmap(addr, length);
}

}
