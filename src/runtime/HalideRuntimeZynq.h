#ifndef HALIDE_HALIDERUNTIMEZYNQ_H
#define HALIDE_HALIDERUNTIMEZYNQ_H

#include "HalideRuntime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  unsigned int id; // ID flag for internal use
  unsigned int width; // Width of the image
  unsigned int stride; // Stride between rows, in pixels. This must be >= width
  unsigned int height; // Height of the image
  unsigned int depth; // Byte-depth of the image
  unsigned int phys_addr; // Bus address for DMA
  void* kern_addr; // Kernel virtual address
  struct mMap* cvals;
  unsigned int mmap_offset;
} Buffer;

/* Sets a new buffer object which points to the same memory, but which has
 * a smaller shape.  The base pointers and width/height are adjusted to match.
 * Returns zero if successes.
 */
extern int slice_buffer(Buffer* src, Buffer* des, unsigned int x, unsigned int y, unsigned int width, unsigned int height);

/* system calls */
typedef int32_t off_t;
extern int ioctl(int fd, unsigned long cmd, ...);
extern void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
extern int munmap(void *addr, size_t length);

#ifdef __cplusplus
} // End extern "C"
#endif

#endif // HALIDE_HALIDERUNTIMEZYNQ_H
