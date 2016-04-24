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

#ifndef HALIDE_ATTRIBUTE_ALIGN
  #ifdef _MSC_VER
    #define HALIDE_ATTRIBUTE_ALIGN(x) __declspec(align(x))
  #else
    #define HALIDE_ATTRIBUTE_ALIGN(x) __attribute__((aligned(x)))
  #endif
#endif
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
struct halide_filter_metadata_t;
extern "C" {
void *halide_malloc(void *ctx, size_t);
void halide_free(void *ctx, void *ptr);
void *halide_print(void *ctx, const void *str);
void *halide_error(void *ctx, const void *str);
int halide_debug_to_file(void *ctx, const char *filename, void *data, int, int, int, int, int, int);
int halide_start_clock(void *ctx);
int64_t halide_current_time_ns(void *ctx);
void halide_profiler_pipeline_end(void *, void *);
}

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
int halide_alloc_kbuf(int fd, kbuf_t* ptr) {
 return ioctl(fd, GET_BUFFER, (long unsigned int)ptr);
}
int halide_free_kbuf(int fd, kbuf_t* ptr) {
 return ioctl(fd, FREE_IMAGE, (long unsigned int)ptr);
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
 //uint8_t *_output__2 = (uint8_t *)(_output__2_buffer->host);
 const int32_t _output__2_extent_0 = _output__2_buffer->extent[0];
 (void)_output__2_extent_0;
 const int32_t _output__2_extent_1 = _output__2_buffer->extent[1];
 (void)_output__2_extent_1;
 const int32_t _output__2_extent_2 = _output__2_buffer->extent[2];
 (void)_output__2_extent_2;
 const int32_t _output__2_extent_3 = _output__2_buffer->extent[3];
 (void)_output__2_extent_3;
 const int32_t _output__2_stride_0 = _output__2_buffer->stride[0];
 (void)_output__2_stride_0;
 const int32_t _output__2_stride_1 = _output__2_buffer->stride[1];
 (void)_output__2_stride_1;
 const int32_t _output__2_stride_2 = _output__2_buffer->stride[2];
 (void)_output__2_stride_2;
 const int32_t _output__2_stride_3 = _output__2_buffer->stride[3];
 (void)_output__2_stride_3;
 {
  {
      /*
   kbuf_t _kbuf_hw_output__2;
   _kbuf_hw_output__2.width = 720;
   _kbuf_hw_output__2.height = 480;
   _kbuf_hw_output__2.depth = 3;
   _kbuf_hw_output__2.stride = 720;
   int _status_get_kbuf_hw_output__2 = halide_alloc_kbuf(__cma, &_kbuf_hw_output__2);
   if (_status_get_kbuf_hw_output__2 < 0)   {
    printf("Failed to allocate kernel buffer for hw_output$2.\n");
   }
   uint8_t*_hw_output__2 = (uint8_t*) mmap(NULL, _kbuf_hw_output__2.stride * _kbuf_hw_output__2.height * _kbuf_hw_output__2.depth, PROT_WRITE, MAP_SHARED, __cma, _kbuf_hw_output__2.mmap_offset);
   // produce hw_output$2
   */
   {
    kbuf_t _slice_padded__2;
    halide_slice_kbuf(&input_kbuf, &_slice_padded__2, 200-2, 40-2, 1443, 963);
    kbuf_t _kbufs[2];
    _kbufs[0] = _slice_padded__2;
    _kbufs[1] = output_kbuf;
    int _process_id = halide_process_image(__hwacc, _kbufs);
    halide_pend_processed(__hwacc, _process_id);
   } // slice_padded$2
   // consume hw_output$2
   // produce output$2
   /*
   for (int _output__2_s0_c = 0; _output__2_s0_c < 0 + 3; _output__2_s0_c++)
   {
    for (int _output__2_s0_y = 0; _output__2_s0_y < 0 + 480; _output__2_s0_y++)
    {
     for (int _output__2_s0_x = 0; _output__2_s0_x < 0 + 720; _output__2_s0_x++)
     {
      int32_t _337 = _output__2_s0_y * _output__2_stride_1;
      int32_t _338 = _output__2_s0_x + _337;
      int32_t _339 = _output__2_s0_c * _output__2_stride_2;
      int32_t _340 = _338 + _339;
      int32_t _345 = _340 - 0;
      int32_t _346 = _output__2_s0_x * 3;
      int32_t _347 = _output__2_s0_c + _346;
      int32_t _348 = _output__2_s0_y * 3*720;
      int32_t _349 = _347 + _348;
      int32_t _350 = _349 + 0;
      uint8_t _351 = _hw_output__2[_350];
      _output__2[_345] = _351;
     } // for _output__2_s0_x
    } // for _output__2_s0_y
   } // for _output__2_s0_c
   // consume output$2
   munmap((void*)_hw_output__2, _kbuf_hw_output__2.stride * _kbuf_hw_output__2.height * _kbuf_hw_output__2.depth);
   halide_free_kbuf(__cma, &_kbuf_hw_output__2);
   */
  } // alloc _hw_output__2
 } // alloc __auto_insert__padded__2
 return 0;
}

#ifdef __cplusplus
}  // extern "C"
#endif
