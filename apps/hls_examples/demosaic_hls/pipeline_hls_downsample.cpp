#include <iostream>
#include <math.h>
#include <float.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <hls_stream.h>
#include "Stencil.h"
#include "hls_target.h"
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

#ifndef HALIDE_FUNCTION_ATTRS
#define HALIDE_FUNCTION_ATTRS
#endif
#ifdef __cplusplus
extern "C" {
#endif

int32_t halide_error_bad_elem_size(void *, const char *, const char *, int32_t, int32_t);
int32_t halide_error_access_out_of_bounds(void *, const char *, int32_t, int32_t, int32_t, int32_t, int32_t);
int32_t halide_error_constraint_violated(void *, const char *, int32_t, const char *, int32_t);
int32_t halide_error_buffer_allocation_too_large(void *, const char *, int64_t, int64_t);
int32_t halide_error_buffer_extents_too_large(void *, const char *, int64_t, int64_t);
int32_t halide_error_explicit_bounds_too_small(void *, const char *, const char *, int32_t, int32_t, int32_t, int32_t);

static int __pipeline_hls(buffer_t *_p2_input_buffer, buffer_t *_output_2_buffer) HALIDE_FUNCTION_ATTRS {
 uint8_t *_p2_input = (uint8_t *)(_p2_input_buffer->host);
 (void)_p2_input;
 const bool _p2_input_host_and_dev_are_null = (_p2_input_buffer->host == NULL) && (_p2_input_buffer->dev == 0);
 (void)_p2_input_host_and_dev_are_null;
 const int32_t _p2_input_min_0 = _p2_input_buffer->min[0];
 (void)_p2_input_min_0;
 const int32_t _p2_input_min_1 = _p2_input_buffer->min[1];
 (void)_p2_input_min_1;
 const int32_t _p2_input_min_2 = _p2_input_buffer->min[2];
 (void)_p2_input_min_2;
 const int32_t _p2_input_min_3 = _p2_input_buffer->min[3];
 (void)_p2_input_min_3;
 const int32_t _p2_input_extent_0 = _p2_input_buffer->extent[0];
 (void)_p2_input_extent_0;
 const int32_t _p2_input_extent_1 = _p2_input_buffer->extent[1];
 (void)_p2_input_extent_1;
 const int32_t _p2_input_extent_2 = _p2_input_buffer->extent[2];
 (void)_p2_input_extent_2;
 const int32_t _p2_input_extent_3 = _p2_input_buffer->extent[3];
 (void)_p2_input_extent_3;
 const int32_t _p2_input_stride_0 = _p2_input_buffer->stride[0];
 (void)_p2_input_stride_0;
 const int32_t _p2_input_stride_1 = _p2_input_buffer->stride[1];
 (void)_p2_input_stride_1;
 const int32_t _p2_input_stride_2 = _p2_input_buffer->stride[2];
 (void)_p2_input_stride_2;
 const int32_t _p2_input_stride_3 = _p2_input_buffer->stride[3];
 (void)_p2_input_stride_3;
 const int32_t _p2_input_elem_size = _p2_input_buffer->elem_size;
 (void)_p2_input_elem_size;
 uint8_t *_output_2 = (uint8_t *)(_output_2_buffer->host);
 (void)_output_2;
 const bool _output_2_host_and_dev_are_null = (_output_2_buffer->host == NULL) && (_output_2_buffer->dev == 0);
 (void)_output_2_host_and_dev_are_null;
 const int32_t _output_2_min_0 = _output_2_buffer->min[0];
 (void)_output_2_min_0;
 const int32_t _output_2_min_1 = _output_2_buffer->min[1];
 (void)_output_2_min_1;
 const int32_t _output_2_min_2 = _output_2_buffer->min[2];
 (void)_output_2_min_2;
 const int32_t _output_2_min_3 = _output_2_buffer->min[3];
 (void)_output_2_min_3;
 const int32_t _output_2_extent_0 = _output_2_buffer->extent[0];
 (void)_output_2_extent_0;
 const int32_t _output_2_extent_1 = _output_2_buffer->extent[1];
 (void)_output_2_extent_1;
 const int32_t _output_2_extent_2 = _output_2_buffer->extent[2];
 (void)_output_2_extent_2;
 const int32_t _output_2_extent_3 = _output_2_buffer->extent[3];
 (void)_output_2_extent_3;
 const int32_t _output_2_stride_0 = _output_2_buffer->stride[0];
 (void)_output_2_stride_0;
 const int32_t _output_2_stride_1 = _output_2_buffer->stride[1];
 (void)_output_2_stride_1;
 const int32_t _output_2_stride_2 = _output_2_buffer->stride[2];
 (void)_output_2_stride_2;
 const int32_t _output_2_stride_3 = _output_2_buffer->stride[3];
 (void)_output_2_stride_3;
 const int32_t _output_2_elem_size = _output_2_buffer->elem_size;
 (void)_output_2_elem_size;
 {
  {
   int64_t _110 = 1389609;
   uint8_t *__auto_insert__padded_2 = (uint8_t *)halide_malloc(NULL, sizeof(uint8_t)*_110);
   // produce __auto_insert__padded$2
   for (int __auto_insert__padded_2_s0_y = -1; __auto_insert__padded_2_s0_y < -1 + 962; __auto_insert__padded_2_s0_y++)
   {
    for (int __auto_insert__padded_2_s0_x = -1; __auto_insert__padded_2_s0_x < -1 + 1442; __auto_insert__padded_2_s0_x++)
    {
     int32_t _111 = __auto_insert__padded_2_s0_y * 1443;
     int32_t _112 = __auto_insert__padded_2_s0_x + _111;
     int32_t _113 = _112 + 2888;
     int32_t _114 = __auto_insert__padded_2_s0_y + 40;
     int32_t _115 = _114 * _p2_input_stride_1;
     int32_t _116 = __auto_insert__padded_2_s0_x + _115;
     int32_t _117 = _p2_input_min_1 * _p2_input_stride_1;
     int32_t _118 = _p2_input_min_0 + _117;
     int32_t _119 = _116 - _118;
     int32_t _120 = _119 + 200;
     uint8_t _121 = _p2_input[_120];
     __auto_insert__padded_2[_113] = _121;
    } // for __auto_insert__padded_2_s0_x
   } // for __auto_insert__padded_2_s0_y
   // consume __auto_insert__padded$2
   {
    int64_t _122 = 4147200;
    uint8_t *_hw_output_2 = (uint8_t *)halide_malloc(NULL, sizeof(uint8_t)*_122);
    // produce hw_output$2
    hls::stream<AxiPackedStencil<uint8_t, 1, 1> > _padded_2_stencil_update_stream;
    (void)0;
    (void)0;
    // produce padded$2.stencil_update.stream
    for (int _padded_2_scan_update_y = 0; _padded_2_scan_update_y < 0 + 963; _padded_2_scan_update_y++)
    {
     for (int _padded_2_scan_update_x = 0; _padded_2_scan_update_x < 0 + 1443; _padded_2_scan_update_x++)
     {
      Stencil<uint8_t, 1, 1> _padded_2_stencil;
      // produce padded$2.stencil
      int32_t _123 = _padded_2_scan_update_y * 1443;
      int32_t _124 = _padded_2_scan_update_x + _123;
      uint8_t _125 = __auto_insert__padded_2[_124];
      _padded_2_stencil(0, 0) = _125;
      // consume padded$2.stencil
      _padded_2_stencil_update_stream.write(_padded_2_stencil);
      (void)0;
     } // for _padded_2_scan_update_x
    } // for _padded_2_scan_update_y
    // consume padded$2.stencil_update.stream
    hls::stream<AxiPackedStencil<uint8_t, 3, 1, 1> > __auto_insert__hw_output_2_stencil_stream;
    (void)0;
    (void)0;
    // produce _hls_target.__auto_insert__hw_output$2.stencil.stream
    hls_target(__auto_insert__hw_output_2_stencil_stream, _padded_2_stencil_update_stream);
    // consume hw_output$2
    // produce output$2
    for (int _output_2_s0_y = 0; _output_2_s0_y < 0 + 480; _output_2_s0_y++)
    {
     for (int _output_2_s0_x = 0; _output_2_s0_x < 0 + 720; _output_2_s0_x++)
     {
      Stencil<uint8_t, 3, 1, 1> __auto_insert__hw_output_2_stencil;
      __auto_insert__hw_output_2_stencil = __auto_insert__hw_output_2_stencil_stream.read();
      for (int _output_2_s0_c = 0; _output_2_s0_c < 0 + 3; _output_2_s0_c++)
      {
       int32_t _309 = _output_2_s0_y * _output_2_stride_1;
       int32_t _310 = _output_2_s0_x + _309;
       int32_t _311 = _output_2_s0_c * _output_2_stride_2;
       int32_t _312 = _310 + _311;
       int32_t _313 = _output_2_min_1 * _output_2_stride_1;
       int32_t _314 = _output_2_min_0 + _313;
       int32_t _315 = _output_2_min_2 * _output_2_stride_2;
       int32_t _316 = _314 + _315;
       int32_t _317 = _312 - _316;
       uint8_t _323 = __auto_insert__hw_output_2_stencil(_output_2_s0_c, 0, 0);
       _output_2[_317] = _323;
      } // for _output_2_s0_x
     } // for _output_2_s0_y
    } // for _output_2_s0_c
    // consume output$2
    halide_free(NULL, _hw_output_2);
   } // alloc _hw_output_2
   halide_free(NULL, __auto_insert__padded_2);
  } // alloc __auto_insert__padded_2
 } // if _3
 return 0;
}


int pipeline_hls(buffer_t *_p2_input_buffer, buffer_t *_output_2_buffer) HALIDE_FUNCTION_ATTRS {
 uint8_t *_p2_input = (uint8_t *)(_p2_input_buffer->host);
 (void)_p2_input;
 const bool _p2_input_host_and_dev_are_null = (_p2_input_buffer->host == NULL) && (_p2_input_buffer->dev == 0);
 (void)_p2_input_host_and_dev_are_null;
 const int32_t _p2_input_min_0 = _p2_input_buffer->min[0];
 (void)_p2_input_min_0;
 const int32_t _p2_input_min_1 = _p2_input_buffer->min[1];
 (void)_p2_input_min_1;
 const int32_t _p2_input_min_2 = _p2_input_buffer->min[2];
 (void)_p2_input_min_2;
 const int32_t _p2_input_min_3 = _p2_input_buffer->min[3];
 (void)_p2_input_min_3;
 const int32_t _p2_input_extent_0 = _p2_input_buffer->extent[0];
 (void)_p2_input_extent_0;
 const int32_t _p2_input_extent_1 = _p2_input_buffer->extent[1];
 (void)_p2_input_extent_1;
 const int32_t _p2_input_extent_2 = _p2_input_buffer->extent[2];
 (void)_p2_input_extent_2;
 const int32_t _p2_input_extent_3 = _p2_input_buffer->extent[3];
 (void)_p2_input_extent_3;
 const int32_t _p2_input_stride_0 = _p2_input_buffer->stride[0];
 (void)_p2_input_stride_0;
 const int32_t _p2_input_stride_1 = _p2_input_buffer->stride[1];
 (void)_p2_input_stride_1;
 const int32_t _p2_input_stride_2 = _p2_input_buffer->stride[2];
 (void)_p2_input_stride_2;
 const int32_t _p2_input_stride_3 = _p2_input_buffer->stride[3];
 (void)_p2_input_stride_3;
 const int32_t _p2_input_elem_size = _p2_input_buffer->elem_size;
 (void)_p2_input_elem_size;
 uint8_t *_output_2 = (uint8_t *)(_output_2_buffer->host);
 (void)_output_2;
 const bool _output_2_host_and_dev_are_null = (_output_2_buffer->host == NULL) && (_output_2_buffer->dev == 0);
 (void)_output_2_host_and_dev_are_null;
 const int32_t _output_2_min_0 = _output_2_buffer->min[0];
 (void)_output_2_min_0;
 const int32_t _output_2_min_1 = _output_2_buffer->min[1];
 (void)_output_2_min_1;
 const int32_t _output_2_min_2 = _output_2_buffer->min[2];
 (void)_output_2_min_2;
 const int32_t _output_2_min_3 = _output_2_buffer->min[3];
 (void)_output_2_min_3;
 const int32_t _output_2_extent_0 = _output_2_buffer->extent[0];
 (void)_output_2_extent_0;
 const int32_t _output_2_extent_1 = _output_2_buffer->extent[1];
 (void)_output_2_extent_1;
 const int32_t _output_2_extent_2 = _output_2_buffer->extent[2];
 (void)_output_2_extent_2;
 const int32_t _output_2_extent_3 = _output_2_buffer->extent[3];
 (void)_output_2_extent_3;
 const int32_t _output_2_stride_0 = _output_2_buffer->stride[0];
 (void)_output_2_stride_0;
 const int32_t _output_2_stride_1 = _output_2_buffer->stride[1];
 (void)_output_2_stride_1;
 const int32_t _output_2_stride_2 = _output_2_buffer->stride[2];
 (void)_output_2_stride_2;
 const int32_t _output_2_stride_3 = _output_2_buffer->stride[3];
 (void)_output_2_stride_3;
 const int32_t _output_2_elem_size = _output_2_buffer->elem_size;
 (void)_output_2_elem_size;
 int32_t _324 = __pipeline_hls(_p2_input_buffer, _output_2_buffer);
 bool _325 = _324 == 0;
 if (!_325)  {
  return _324;
 }
 return 0;
}
#ifdef __cplusplus
}  // extern "C"
#endif
