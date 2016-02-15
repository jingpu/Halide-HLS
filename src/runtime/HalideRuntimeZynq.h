#ifndef HALIDE_HALIDERUNTIMEZYNQ_H
#define HALIDE_HALIDERUNTIMEZYNQ_H

#include "HalideRuntime.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mMap;

typedef struct kbuf_t {
  unsigned int id; // ID flag for internal use
  unsigned int width; // Width of the image
  unsigned int stride; // Stride between rows, in pixels. This must be >= width
  unsigned int height; // Height of the image
  unsigned int depth; // Byte-depth of the image
  unsigned int phys_addr; // Bus address for DMA
  void* kern_addr; // Kernel virtual address
  struct mMap* cvals;
  unsigned int mmap_offset;
} kbuf_t;

/* Sets a new buffer object which points to the same memory, but which has
 * a smaller shape.  The base pointers and width/height are adjusted to match.
 * Returns zero if successes.
 */
extern int halide_slice_kbuf(kbuf_t* src, kbuf_t* des, int x, int y, int width, int height);

extern int halide_alloc_kbuf(int fd, kbuf_t* ptr);
extern int halide_free_kbuf(int fd, kbuf_t* ptr);
extern int halide_process_image(int fd, kbuf_t* ptr);
extern int halide_pend_processed(int fd);

/* system calls */
typedef int32_t off_t; // FIXME this is not actually correct
extern void *halide_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
extern int halide_munmap(void *addr, size_t length);

#ifdef __cplusplus
} // End extern "C"
#endif

#endif // HALIDE_HALIDERUNTIMEZYNQ_H
