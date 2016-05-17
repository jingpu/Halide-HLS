#include <iostream>
#include <math.h>
#include <float.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#ifndef HALIDE_ATTRIBUTE_ALIGN
  #ifdef _MSC_VER
    #define HALIDE_ATTRIBUTE_ALIGN(x) __declspec(align(x))
  #else
    #define HALIDE_ATTRIBUTE_ALIGN(x) __attribute__((aligned(x)))
  #endif
#endif
#ifndef BUFFER_T_DEFINED
#define BUFFER_T_DEFINED
#include <stdbool.h>
#include <stdint.h>
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

// Zynq runtime API
int halide_zynq_init();
void halide_zynq_free(void *user_context, void *ptr);
int halide_zynq_cma_alloc(struct buffer_t *buf);
int halide_zynq_cma_free(struct buffer_t *buf);
int halide_zynq_subimage(const struct buffer_t* image, struct cma_buffer_t* subimage, void *address_of_subimage_origin, int width, int height);
int halide_zynq_hwacc_launch(struct cma_buffer_t bufs[]);
int halide_zynq_hwacc_sync(int task_id);

int pipeline_zynq(buffer_t *input_buffer, buffer_t *output_buffer) {
 cma_buffer_t* output_kbuf = (cma_buffer_t *)output_buffer->dev;

 cma_buffer_t input_slice;
 uint8_t* subimage_origin = input_buffer->host
     + (201-7)*input_buffer->stride[0]
     + (41-7)*input_buffer->stride[1];
 halide_zynq_subimage(input_buffer, &input_slice, subimage_origin , 1454, 974);
 cma_buffer_t _kbufs[2];
 _kbufs[0] = input_slice;
 _kbufs[1] = *output_kbuf;
 int _process_id = halide_zynq_hwacc_launch(_kbufs);
 halide_zynq_hwacc_sync(_process_id);
 return 0;
}

#ifdef __cplusplus
}  // extern "C"
#endif
