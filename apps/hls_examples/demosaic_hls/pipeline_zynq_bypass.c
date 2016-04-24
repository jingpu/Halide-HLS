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
#ifndef KBUF_T_DEFINED
#define KBUF_T_DEFINED

typedef struct kbuf_t{
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
#endif
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

#ifdef __cplusplus
extern "C" {
#endif
#ifndef _IOCTL_CMDS_H_
#define _IOCTL_CMDS_H_
#define GET_BUFFER 1000 // Get an unused buffer
#define GRAB_IMAGE 1001 // Acquire image from camera
#define FREE_IMAGE 1002 // Release buffer
#define PROCESS_IMAGE 1003 // Push to stencil path
#define PEND_PROCESSED 1004 // Retreive from stencil path
#endif
static int halide_slice_kbuf(kbuf_t* src, kbuf_t* des, int x, int y, int width, int height) {
 *des = *src; // copy depth, stride, data, etc.
 des->width = width;
 des->height = height;
 des->phys_addr += src->depth * (y * src->stride + x);
 des->mmap_offset += src->depth * (y * src->stride + x);
 return 0;
}
static int halide_process_image(int fd, kbuf_t* ptr) {
 return ioctl(fd, PROCESS_IMAGE, (long unsigned int)ptr);
}
static int halide_pend_processed(int fd, int id) {
 return ioctl(fd, PEND_PROCESSED, (long unsigned int)id);
}


int pipeline_zynq(buffer_t *_p2___input_buffer, buffer_t *_output__2_buffer, const int32_t __hwacc, const int32_t __cma) {
 kbuf_t input_kbuf = {0};
 input_kbuf.width = 1920;
 input_kbuf.height = 1080;
 input_kbuf.depth = 1;
 input_kbuf.stride = 2048;
 input_kbuf.phys_addr = _p2___input_buffer->dev;
 kbuf_t output_kbuf = {0};
 output_kbuf.width = 720;
 output_kbuf.height = 480;
 output_kbuf.depth = 3;
 output_kbuf.stride = 720;
 output_kbuf.phys_addr = _output__2_buffer->dev;

 kbuf_t _slice_padded__2;
 halide_slice_kbuf(&input_kbuf, &_slice_padded__2, 200-2, 40-2, 1443, 963);
 kbuf_t _kbufs[2];
 _kbufs[0] = _slice_padded__2;
 _kbufs[1] = output_kbuf;
 int _process_id = halide_process_image(__hwacc, _kbufs);
 halide_pend_processed(__hwacc, _process_id);
 return 0;
}

#ifdef __cplusplus
}  // extern "C"
#endif
