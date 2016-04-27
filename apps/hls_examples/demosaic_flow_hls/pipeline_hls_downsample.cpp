#include <iostream>
#include <math.h>
#include <float.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <hls_stream.h>
#include "Stencil.h"
#include "hls_target_downsample.h"
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

#ifdef _WIN32
float roundf(float);
double round(double);
#else
inline float asinh_f32(float x) {return asinhf(x);}
inline float acosh_f32(float x) {return acoshf(x);}
inline float atanh_f32(float x) {return atanhf(x);}
inline double asinh_f64(double x) {return asinh(x);}
inline double acosh_f64(double x) {return acosh(x);}
inline double atanh_f64(double x) {return atanh(x);}
#endif
inline float sqrt_f32(float x) {return sqrtf(x);}
inline float sin_f32(float x) {return sinf(x);}
inline float asin_f32(float x) {return asinf(x);}
inline float cos_f32(float x) {return cosf(x);}
inline float acos_f32(float x) {return acosf(x);}
inline float tan_f32(float x) {return tanf(x);}
inline float atan_f32(float x) {return atanf(x);}
inline float sinh_f32(float x) {return sinhf(x);}
inline float cosh_f32(float x) {return coshf(x);}
inline float tanh_f32(float x) {return tanhf(x);}
inline float hypot_f32(float x, float y) {return hypotf(x, y);}
inline float exp_f32(float x) {return expf(x);}
inline float log_f32(float x) {return logf(x);}
inline float pow_f32(float x, float y) {return powf(x, y);}
inline float floor_f32(float x) {return floorf(x);}
inline float ceil_f32(float x) {return ceilf(x);}
inline float round_f32(float x) {return roundf(x);}

inline double sqrt_f64(double x) {return sqrt(x);}
inline double sin_f64(double x) {return sin(x);}
inline double asin_f64(double x) {return asin(x);}
inline double cos_f64(double x) {return cos(x);}
inline double acos_f64(double x) {return acos(x);}
inline double tan_f64(double x) {return tan(x);}
inline double atan_f64(double x) {return atan(x);}
inline double sinh_f64(double x) {return sinh(x);}
inline double cosh_f64(double x) {return cosh(x);}
inline double tanh_f64(double x) {return tanh(x);}
inline double hypot_f64(double x, double y) {return hypot(x, y);}
inline double exp_f64(double x) {return exp(x);}
inline double log_f64(double x) {return log(x);}
inline double pow_f64(double x, double y) {return pow(x, y);}
inline double floor_f64(double x) {return floor(x);}
inline double ceil_f64(double x) {return ceil(x);}
inline double round_f64(double x) {return round(x);}

inline float nan_f32() {return NAN;}
inline float neg_inf_f32() {return -INFINITY;}
inline float inf_f32() {return INFINITY;}
inline bool is_nan_f32(float x) {return x != x;}
inline bool is_nan_f64(double x) {return x != x;}
inline float float_from_bits(uint32_t bits) {
 union {
  uint32_t as_uint;
  float as_float;
 } u;
 u.as_uint = bits;
 return u.as_float;
}
inline int64_t make_int64(int32_t hi, int32_t lo) {
    return (((int64_t)hi) << 32) | (uint32_t)lo;
}
inline double make_float64(int32_t i0, int32_t i1) {
    union {
        int32_t as_int32[2];
        double as_double;
    } u;
    u.as_int32[0] = i0;
    u.as_int32[1] = i1;
    return u.as_double;
}

template<typename T> T max(T a, T b) {if (a > b) return a; return b;}
template<typename T> T min(T a, T b) {if (a < b) return a; return b;}
template<typename A, typename B> A reinterpret(B b) {A a; memcpy(&a, &b, sizeof(a)); return a;}

inline static bool halide_rewrite_buffer(buffer_t *b, int32_t elem_size,
                           int32_t min0, int32_t extent0, int32_t stride0,
                           int32_t min1, int32_t extent1, int32_t stride1,
                           int32_t min2, int32_t extent2, int32_t stride2,
                           int32_t min3, int32_t extent3, int32_t stride3) {
 b->min[0] = min0;
 b->min[1] = min1;
 b->min[2] = min2;
 b->min[3] = min3;
 b->extent[0] = extent0;
 b->extent[1] = extent1;
 b->extent[2] = extent2;
 b->extent[3] = extent3;
 b->stride[0] = stride0;
 b->stride[1] = stride1;
 b->stride[2] = stride2;
 b->stride[3] = stride3;
 return true;
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

static int __pipeline_hls(buffer_t *_p2_input1_buffer, buffer_t *_p2_input2_buffer, buffer_t *_output_2_buffer) HALIDE_FUNCTION_ATTRS {
 uint8_t *_p2_input1 = (uint8_t *)(_p2_input1_buffer->host);
 (void)_p2_input1;
 const bool _p2_input1_host_and_dev_are_null = (_p2_input1_buffer->host == NULL) && (_p2_input1_buffer->dev == 0);
 (void)_p2_input1_host_and_dev_are_null;
 const int32_t _p2_input1_min_0 = _p2_input1_buffer->min[0];
 (void)_p2_input1_min_0;
 const int32_t _p2_input1_min_1 = _p2_input1_buffer->min[1];
 (void)_p2_input1_min_1;
 const int32_t _p2_input1_min_2 = _p2_input1_buffer->min[2];
 (void)_p2_input1_min_2;
 const int32_t _p2_input1_min_3 = _p2_input1_buffer->min[3];
 (void)_p2_input1_min_3;
 const int32_t _p2_input1_extent_0 = _p2_input1_buffer->extent[0];
 (void)_p2_input1_extent_0;
 const int32_t _p2_input1_extent_1 = _p2_input1_buffer->extent[1];
 (void)_p2_input1_extent_1;
 const int32_t _p2_input1_extent_2 = _p2_input1_buffer->extent[2];
 (void)_p2_input1_extent_2;
 const int32_t _p2_input1_extent_3 = _p2_input1_buffer->extent[3];
 (void)_p2_input1_extent_3;
 const int32_t _p2_input1_stride_0 = _p2_input1_buffer->stride[0];
 (void)_p2_input1_stride_0;
 const int32_t _p2_input1_stride_1 = _p2_input1_buffer->stride[1];
 (void)_p2_input1_stride_1;
 const int32_t _p2_input1_stride_2 = _p2_input1_buffer->stride[2];
 (void)_p2_input1_stride_2;
 const int32_t _p2_input1_stride_3 = _p2_input1_buffer->stride[3];
 (void)_p2_input1_stride_3;
 const int32_t _p2_input1_elem_size = _p2_input1_buffer->elem_size;
 (void)_p2_input1_elem_size;
 uint8_t *_p2_input2 = (uint8_t *)(_p2_input2_buffer->host);
 (void)_p2_input2;
 const bool _p2_input2_host_and_dev_are_null = (_p2_input2_buffer->host == NULL) && (_p2_input2_buffer->dev == 0);
 (void)_p2_input2_host_and_dev_are_null;
 const int32_t _p2_input2_min_0 = _p2_input2_buffer->min[0];
 (void)_p2_input2_min_0;
 const int32_t _p2_input2_min_1 = _p2_input2_buffer->min[1];
 (void)_p2_input2_min_1;
 const int32_t _p2_input2_min_2 = _p2_input2_buffer->min[2];
 (void)_p2_input2_min_2;
 const int32_t _p2_input2_min_3 = _p2_input2_buffer->min[3];
 (void)_p2_input2_min_3;
 const int32_t _p2_input2_extent_0 = _p2_input2_buffer->extent[0];
 (void)_p2_input2_extent_0;
 const int32_t _p2_input2_extent_1 = _p2_input2_buffer->extent[1];
 (void)_p2_input2_extent_1;
 const int32_t _p2_input2_extent_2 = _p2_input2_buffer->extent[2];
 (void)_p2_input2_extent_2;
 const int32_t _p2_input2_extent_3 = _p2_input2_buffer->extent[3];
 (void)_p2_input2_extent_3;
 const int32_t _p2_input2_stride_0 = _p2_input2_buffer->stride[0];
 (void)_p2_input2_stride_0;
 const int32_t _p2_input2_stride_1 = _p2_input2_buffer->stride[1];
 (void)_p2_input2_stride_1;
 const int32_t _p2_input2_stride_2 = _p2_input2_buffer->stride[2];
 (void)_p2_input2_stride_2;
 const int32_t _p2_input2_stride_3 = _p2_input2_buffer->stride[3];
 (void)_p2_input2_stride_3;
 const int32_t _p2_input2_elem_size = _p2_input2_buffer->elem_size;
 (void)_p2_input2_elem_size;
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
 if (_output_2_host_and_dev_are_null)
 {
  bool _0 = halide_rewrite_buffer(_output_2_buffer, 1, 0, 720, 1, 0, 480, 720, 0, 3, 345600, 0, 0, 0);
  (void)_0;
 } // if _output_2_host_and_dev_are_null
 if (_p2_input1_host_and_dev_are_null)
 {
  bool _1 = halide_rewrite_buffer(_p2_input1_buffer, 1, 194, 1454, 1, 34, 974, 1454, 0, 0, 0, 0, 0, 0);
  (void)_1;
 } // if _p2_input1_host_and_dev_are_null
 if (_p2_input2_host_and_dev_are_null)
 {
  bool _2 = halide_rewrite_buffer(_p2_input2_buffer, 1, 196, 1450, 1, 36, 970, 1450, 0, 0, 0, 0, 0, 0);
  (void)_2;
 } // if _p2_input2_host_and_dev_are_null
 bool _3 = _output_2_host_and_dev_are_null || _p2_input1_host_and_dev_are_null;
 bool _4 = _3 || _p2_input2_host_and_dev_are_null;
 bool _5 = !(_4);
 if (_5)
 {
  bool _6 = _output_2_elem_size == 1;
  if (!_6)   {
   int32_t _7 = halide_error_bad_elem_size(NULL, "Output buffer output$2", "uint8", _output_2_elem_size, 1);
   return _7;
  }
  bool _8 = _p2_input1_elem_size == 1;
  if (!_8)   {
   int32_t _9 = halide_error_bad_elem_size(NULL, "Input buffer p2:input1", "uint8", _p2_input1_elem_size, 1);
   return _9;
  }
  bool _10 = _p2_input2_elem_size == 1;
  if (!_10)   {
   int32_t _11 = halide_error_bad_elem_size(NULL, "Input buffer p2:input2", "uint8", _p2_input2_elem_size, 1);
   return _11;
  }
  bool _12 = _output_2_min_0 <= 0;
  int32_t _13 = 720 - _output_2_extent_0;
  bool _14 = _13 <= _output_2_min_0;
  bool _15 = _12 && _14;
  if (!_15)   {
   int32_t _16 = _output_2_min_0 + _output_2_extent_0;
   int32_t _17 = _16 + -1;
   int32_t _18 = halide_error_access_out_of_bounds(NULL, "Output buffer output$2", 0, 0, 719, _output_2_min_0, _17);
   return _18;
  }
  bool _19 = _output_2_min_1 <= 0;
  int32_t _20 = 480 - _output_2_extent_1;
  bool _21 = _20 <= _output_2_min_1;
  bool _22 = _19 && _21;
  if (!_22)   {
   int32_t _23 = _output_2_min_1 + _output_2_extent_1;
   int32_t _24 = _23 + -1;
   int32_t _25 = halide_error_access_out_of_bounds(NULL, "Output buffer output$2", 1, 0, 479, _output_2_min_1, _24);
   return _25;
  }
  bool _26 = _output_2_min_2 <= 0;
  int32_t _27 = 3 - _output_2_extent_2;
  bool _28 = _27 <= _output_2_min_2;
  bool _29 = _26 && _28;
  if (!_29)   {
   int32_t _30 = _output_2_min_2 + _output_2_extent_2;
   int32_t _31 = _30 + -1;
   int32_t _32 = halide_error_access_out_of_bounds(NULL, "Output buffer output$2", 2, 0, 2, _output_2_min_2, _31);
   return _32;
  }
  bool _33 = _p2_input1_min_0 <= 194;
  int32_t _34 = 1648 - _p2_input1_extent_0;
  bool _35 = _34 <= _p2_input1_min_0;
  bool _36 = _33 && _35;
  if (!_36)   {
   int32_t _37 = _p2_input1_min_0 + _p2_input1_extent_0;
   int32_t _38 = _37 + -1;
   int32_t _39 = halide_error_access_out_of_bounds(NULL, "Input buffer p2:input1", 0, 194, 1647, _p2_input1_min_0, _38);
   return _39;
  }
  bool _40 = _p2_input1_min_1 <= 34;
  int32_t _41 = 1008 - _p2_input1_extent_1;
  bool _42 = _41 <= _p2_input1_min_1;
  bool _43 = _40 && _42;
  if (!_43)   {
   int32_t _44 = _p2_input1_min_1 + _p2_input1_extent_1;
   int32_t _45 = _44 + -1;
   int32_t _46 = halide_error_access_out_of_bounds(NULL, "Input buffer p2:input1", 1, 34, 1007, _p2_input1_min_1, _45);
   return _46;
  }
  bool _47 = _p2_input2_min_0 <= 196;
  int32_t _48 = 1646 - _p2_input2_extent_0;
  bool _49 = _48 <= _p2_input2_min_0;
  bool _50 = _47 && _49;
  if (!_50)   {
   int32_t _51 = _p2_input2_min_0 + _p2_input2_extent_0;
   int32_t _52 = _51 + -1;
   int32_t _53 = halide_error_access_out_of_bounds(NULL, "Input buffer p2:input2", 0, 196, 1645, _p2_input2_min_0, _52);
   return _53;
  }
  bool _54 = _p2_input2_min_1 <= 36;
  int32_t _55 = 1006 - _p2_input2_extent_1;
  bool _56 = _55 <= _p2_input2_min_1;
  bool _57 = _54 && _56;
  if (!_57)   {
   int32_t _58 = _p2_input2_min_1 + _p2_input2_extent_1;
   int32_t _59 = _58 + -1;
   int32_t _60 = halide_error_access_out_of_bounds(NULL, "Input buffer p2:input2", 1, 36, 1005, _p2_input2_min_1, _59);
   return _60;
  }
  bool _61 = _output_2_stride_0 == 1;
  if (!_61)   {
   int32_t _62 = halide_error_constraint_violated(NULL, "output$2.stride.0", _output_2_stride_0, "1", 1);
   return _62;
  }
  bool _63 = _p2_input1_stride_0 == 1;
  if (!_63)   {
   int32_t _64 = halide_error_constraint_violated(NULL, "p2:input1.stride.0", _p2_input1_stride_0, "1", 1);
   return _64;
  }
  bool _65 = _p2_input2_stride_0 == 1;
  if (!_65)   {
   int32_t _66 = halide_error_constraint_violated(NULL, "p2:input2.stride.0", _p2_input2_stride_0, "1", 1);
   return _66;
  }
  int64_t _67 = (int64_t)(_output_2_extent_1);
  int64_t _68 = (int64_t)(_output_2_extent_0);
  int64_t _69 = _67 * _68;
  int64_t _70 = (int64_t)(_p2_input1_extent_1);
  int64_t _71 = (int64_t)(_p2_input1_extent_0);
  int64_t _72 = _70 * _71;
  int64_t _73 = (int64_t)(_p2_input2_extent_1);
  int64_t _74 = (int64_t)(_p2_input2_extent_0);
  int64_t _75 = _73 * _74;
  int64_t _76 = (int64_t)(2147483647);
  bool _77 = _68 <= _76;
  if (!_77)   {
   int64_t _78 = (int64_t)(_output_2_extent_0);
   int64_t _79 = (int64_t)(2147483647);
   int32_t _80 = halide_error_buffer_allocation_too_large(NULL, "output$2", _78, _79);
   return _80;
  }
  int64_t _81 = (int64_t)(_output_2_extent_1);
  int64_t _82 = (int64_t)(_output_2_stride_1);
  int64_t _83 = _81 * _82;
  int64_t _84 = (int64_t)(2147483647);
  bool _85 = _83 <= _84;
  if (!_85)   {
   int64_t _86 = (int64_t)(_output_2_extent_1);
   int64_t _87 = (int64_t)(_output_2_stride_1);
   int64_t _88 = _86 * _87;
   int64_t _89 = (int64_t)(2147483647);
   int32_t _90 = halide_error_buffer_allocation_too_large(NULL, "output$2", _88, _89);
   return _90;
  }
  int64_t _91 = (int64_t)(2147483647);
  bool _92 = _69 <= _91;
  if (!_92)   {
   int64_t _93 = (int64_t)(2147483647);
   int32_t _94 = halide_error_buffer_extents_too_large(NULL, "output$2", _69, _93);
   return _94;
  }
  int64_t _95 = (int64_t)(_output_2_extent_2);
  int64_t _96 = (int64_t)(_output_2_stride_2);
  int64_t _97 = _95 * _96;
  int64_t _98 = (int64_t)(2147483647);
  bool _99 = _97 <= _98;
  if (!_99)   {
   int64_t _100 = (int64_t)(_output_2_extent_2);
   int64_t _101 = (int64_t)(_output_2_stride_2);
   int64_t _102 = _100 * _101;
   int64_t _103 = (int64_t)(2147483647);
   int32_t _104 = halide_error_buffer_allocation_too_large(NULL, "output$2", _102, _103);
   return _104;
  }
  int64_t _105 = (int64_t)(_output_2_extent_2);
  int64_t _106 = _105 * _69;
  int64_t _107 = (int64_t)(2147483647);
  bool _108 = _106 <= _107;
  if (!_108)   {
   int64_t _109 = (int64_t)(_output_2_extent_2);
   int64_t _110 = _109 * _69;
   int64_t _111 = (int64_t)(2147483647);
   int32_t _112 = halide_error_buffer_extents_too_large(NULL, "output$2", _110, _111);
   return _112;
  }
  int64_t _113 = (int64_t)(_p2_input1_extent_0);
  int64_t _114 = (int64_t)(2147483647);
  bool _115 = _113 <= _114;
  if (!_115)   {
   int64_t _116 = (int64_t)(_p2_input1_extent_0);
   int64_t _117 = (int64_t)(2147483647);
   int32_t _118 = halide_error_buffer_allocation_too_large(NULL, "p2:input1", _116, _117);
   return _118;
  }
  int64_t _119 = (int64_t)(_p2_input1_extent_1);
  int64_t _120 = (int64_t)(_p2_input1_stride_1);
  int64_t _121 = _119 * _120;
  int64_t _122 = (int64_t)(2147483647);
  bool _123 = _121 <= _122;
  if (!_123)   {
   int64_t _124 = (int64_t)(_p2_input1_extent_1);
   int64_t _125 = (int64_t)(_p2_input1_stride_1);
   int64_t _126 = _124 * _125;
   int64_t _127 = (int64_t)(2147483647);
   int32_t _128 = halide_error_buffer_allocation_too_large(NULL, "p2:input1", _126, _127);
   return _128;
  }
  int64_t _129 = (int64_t)(2147483647);
  bool _130 = _72 <= _129;
  if (!_130)   {
   int64_t _131 = (int64_t)(2147483647);
   int32_t _132 = halide_error_buffer_extents_too_large(NULL, "p2:input1", _72, _131);
   return _132;
  }
  int64_t _133 = (int64_t)(_p2_input2_extent_0);
  int64_t _134 = (int64_t)(2147483647);
  bool _135 = _133 <= _134;
  if (!_135)   {
   int64_t _136 = (int64_t)(_p2_input2_extent_0);
   int64_t _137 = (int64_t)(2147483647);
   int32_t _138 = halide_error_buffer_allocation_too_large(NULL, "p2:input2", _136, _137);
   return _138;
  }
  int64_t _139 = (int64_t)(_p2_input2_extent_1);
  int64_t _140 = (int64_t)(_p2_input2_stride_1);
  int64_t _141 = _139 * _140;
  int64_t _142 = (int64_t)(2147483647);
  bool _143 = _141 <= _142;
  if (!_143)   {
   int64_t _144 = (int64_t)(_p2_input2_extent_1);
   int64_t _145 = (int64_t)(_p2_input2_stride_1);
   int64_t _146 = _144 * _145;
   int64_t _147 = (int64_t)(2147483647);
   int32_t _148 = halide_error_buffer_allocation_too_large(NULL, "p2:input2", _146, _147);
   return _148;
  }
  int64_t _149 = (int64_t)(2147483647);
  bool _150 = _75 <= _149;
  if (!_150)   {
   int64_t _151 = (int64_t)(2147483647);
   int32_t _152 = halide_error_buffer_extents_too_large(NULL, "p2:input2", _75, _151);
   return _152;
  }
  {
   int64_t _153 = 1406500;
   uint8_t *__auto_insert__padded2_2 = (uint8_t *)halide_malloc(NULL, sizeof(uint8_t)*_153);
   // produce __auto_insert__padded2$2
   for (int __auto_insert__padded2_2_s0_y = -5; __auto_insert__padded2_2_s0_y < -5 + 970; __auto_insert__padded2_2_s0_y++)
   {
    for (int __auto_insert__padded2_2_s0_x = -5; __auto_insert__padded2_2_s0_x < -5 + 1450; __auto_insert__padded2_2_s0_x++)
    {
     int32_t _154 = __auto_insert__padded2_2_s0_y * 1450;
     int32_t _155 = __auto_insert__padded2_2_s0_x + _154;
     int32_t _156 = _155 + 7255;
     int32_t _157 = __auto_insert__padded2_2_s0_y + 41;
     int32_t _158 = _157 * _p2_input2_stride_1;
     int32_t _159 = __auto_insert__padded2_2_s0_x + _158;
     int32_t _160 = _p2_input2_min_1 * _p2_input2_stride_1;
     int32_t _161 = _p2_input2_min_0 + _160;
     int32_t _162 = _159 - _161;
     int32_t _163 = _162 + 201;
     uint8_t _164 = _p2_input2[_163];
     __auto_insert__padded2_2[_156] = _164;
    } // for __auto_insert__padded2_2_s0_x
   } // for __auto_insert__padded2_2_s0_y
   // consume __auto_insert__padded2$2
   {
    int64_t _165 = 1416196;
    uint8_t *__auto_insert__padded1_2 = (uint8_t *)halide_malloc(NULL, sizeof(uint8_t)*_165);
    // produce __auto_insert__padded1$2
    for (int __auto_insert__padded1_2_s0_y = -7; __auto_insert__padded1_2_s0_y < -7 + 974; __auto_insert__padded1_2_s0_y++)
    {
     for (int __auto_insert__padded1_2_s0_x = -7; __auto_insert__padded1_2_s0_x < -7 + 1454; __auto_insert__padded1_2_s0_x++)
     {
      int32_t _166 = __auto_insert__padded1_2_s0_y * 1454;
      int32_t _167 = __auto_insert__padded1_2_s0_x + _166;
      int32_t _168 = _167 + 10185;
      int32_t _169 = __auto_insert__padded1_2_s0_y + 41;
      int32_t _170 = _169 * _p2_input1_stride_1;
      int32_t _171 = __auto_insert__padded1_2_s0_x + _170;
      int32_t _172 = _p2_input1_min_1 * _p2_input1_stride_1;
      int32_t _173 = _p2_input1_min_0 + _172;
      int32_t _174 = _171 - _173;
      int32_t _175 = _174 + 201;
      uint8_t _176 = _p2_input1[_175];
      __auto_insert__padded1_2[_168] = _176;
     } // for __auto_insert__padded1_2_s0_x
    } // for __auto_insert__padded1_2_s0_y
    // consume __auto_insert__padded1$2
    {
     int64_t _177 = 1036800;
     uint8_t *_hw_output_2 = (uint8_t *)halide_malloc(NULL, sizeof(uint8_t)*_177);
     // produce hw_output$2
     hls::stream<AxiPackedStencil<uint8_t, 2, 1> > _padded2_2_stencil_update_stream;
     (void)0;
     (void)0;
     // produce padded2$2.stencil_update.stream
     for (int _padded2_2_scan_update_y = 0; _padded2_2_scan_update_y < 0 + 485*2; _padded2_2_scan_update_y++)
     {
      for (int _padded2_2_scan_update_x = 0; _padded2_2_scan_update_x < 0 + 725; _padded2_2_scan_update_x++)
      {
       Stencil<uint8_t, 2, 1> _padded2_2_stencil;
       // produce padded2$2.stencil
        for (int _padded2_2_stencil_s0_x = 0; _padded2_2_stencil_s0_x < 0 + 2; _padded2_2_stencil_s0_x++)
        {
         int32_t _178 = _padded2_2_scan_update_y;
         int32_t _179 = _178;
         int32_t _180 = _padded2_2_scan_update_x * 2;
         int32_t _181 = _padded2_2_stencil_s0_x + _180;
         int32_t _182 = _179 * 1450;
         int32_t _183 = _181 + _182;
         uint8_t _184 = __auto_insert__padded2_2[_183];
         _padded2_2_stencil(_padded2_2_stencil_s0_x, 0) = _184;
        } // for _padded2_2_stencil_s0_x
       // consume padded2$2.stencil
       _padded2_2_stencil_update_stream.write(_padded2_2_stencil);
       (void)0;
      } // for _padded2_2_scan_update_x
     } // for _padded2_2_scan_update_y
     // consume padded2$2.stencil_update.stream
     hls::stream<AxiPackedStencil<uint8_t, 2, 1> > _padded1_2_stencil_update_stream;
     (void)0;
     (void)0;
     // produce padded1$2.stencil_update.stream
     for (int _padded1_2_scan_update_y = 0; _padded1_2_scan_update_y < 0 + 487*2; _padded1_2_scan_update_y++)
     {
      for (int _padded1_2_scan_update_x = 0; _padded1_2_scan_update_x < 0 + 727; _padded1_2_scan_update_x++)
      {
       Stencil<uint8_t, 2, 1> _padded1_2_stencil;
       // produce padded1$2.stencil
        for (int _padded1_2_stencil_s0_x = 0; _padded1_2_stencil_s0_x < 0 + 2; _padded1_2_stencil_s0_x++)
        {
         int32_t _185 = _padded1_2_scan_update_y;
         int32_t _186 = _185;
         int32_t _187 = _padded1_2_scan_update_x * 2;
         int32_t _188 = _padded1_2_stencil_s0_x + _187;
         int32_t _189 = _186 * 1454;
         int32_t _190 = _188 + _189;
         uint8_t _191 = __auto_insert__padded1_2[_190];
         _padded1_2_stencil(_padded1_2_stencil_s0_x, 0) = _191;
        } // for _padded1_2_stencil_s0_x
       // consume padded1$2.stencil
       _padded1_2_stencil_update_stream.write(_padded1_2_stencil);
       (void)0;
      } // for _padded1_2_scan_update_x
     } // for _padded1_2_scan_update_y
     // consume padded1$2.stencil_update.stream
     hls::stream<AxiPackedStencil<uint8_t, 3, 1, 1> > __auto_insert__hw_output_2_stencil_stream;
     (void)0;
     (void)0;
     // produce _hls_target.__auto_insert__hw_output$2.stencil.stream
     hls_target(__auto_insert__hw_output_2_stencil_stream, _padded1_2_stencil_update_stream, _padded2_2_stencil_update_stream);
     // consume _hls_target.__auto_insert__hw_output$2.stencil.stream
     for (int _hw_output_2_s0_y_yi = 0; _hw_output_2_s0_y_yi < 0 + 480; _hw_output_2_s0_y_yi++)
     {
      for (int _hw_output_2_s0_x_xi = 0; _hw_output_2_s0_x_xi < 0 + 720; _hw_output_2_s0_x_xi++)
      {
       Stencil<uint8_t, 3, 1, 1> __auto_insert__hw_output_2_stencil;
       // produce __auto_insert__hw_output$2.stencil
       __auto_insert__hw_output_2_stencil = __auto_insert__hw_output_2_stencil_stream.read();
       (void)0;
       // consume __auto_insert__hw_output$2.stencil
       for (int _hw_output_2_s0_c = 0; _hw_output_2_s0_c < 0 + 3; _hw_output_2_s0_c++)
       {
        int32_t _1472 = _hw_output_2_s0_x_xi * 3;
        int32_t _1473 = _hw_output_2_s0_c + _1472;
        int32_t _1474 = _hw_output_2_s0_y_yi * 2160;
        int32_t _1475 = _1473 + _1474;
        uint8_t _1476 = __auto_insert__hw_output_2_stencil(_hw_output_2_s0_c, 0, 0);
        _hw_output_2[_1475] = _1476;
       } // for _hw_output_2_s0_c
      } // for _hw_output_2_s0_x_xi
     } // for _hw_output_2_s0_y_yi
     // consume hw_output$2
     bool _1477 = 0 <= _output_2_min_2;
     int32_t _1478 = _output_2_min_2 + _output_2_extent_2;
     bool _1479 = _1478 <= 3;
     bool _1480 = _1477 && _1479;
     if (!_1480)      {
      int32_t _1481 = _output_2_min_2 + _output_2_extent_2;
      int32_t _1482 = _1481 + -1;
      int32_t _1483 = halide_error_explicit_bounds_too_small(NULL, "c", "output$2", 0, 2, _output_2_min_2, _1482);
      return _1483;
     }
     bool _1484 = 0 <= _output_2_min_1;
     int32_t _1485 = _output_2_min_1 + _output_2_extent_1;
     bool _1486 = _1485 <= 480;
     bool _1487 = _1484 && _1486;
     if (!_1487)      {
      int32_t _1488 = _output_2_min_1 + _output_2_extent_1;
      int32_t _1489 = _1488 + -1;
      int32_t _1490 = halide_error_explicit_bounds_too_small(NULL, "y", "output$2", 0, 479, _output_2_min_1, _1489);
      return _1490;
     }
     bool _1491 = 0 <= _output_2_min_0;
     int32_t _1492 = _output_2_min_0 + _output_2_extent_0;
     bool _1493 = _1492 <= 720;
     bool _1494 = _1491 && _1493;
     if (!_1494)      {
      int32_t _1495 = _output_2_min_0 + _output_2_extent_0;
      int32_t _1496 = _1495 + -1;
      int32_t _1497 = halide_error_explicit_bounds_too_small(NULL, "x", "output$2", 0, 719, _output_2_min_0, _1496);
      return _1497;
     }
     // produce output$2
     for (int _output_2_s0_c = 0; _output_2_s0_c < 0 + 3; _output_2_s0_c++)
     {
      for (int _output_2_s0_y = 0; _output_2_s0_y < 0 + 480; _output_2_s0_y++)
      {
       for (int _output_2_s0_x = 0; _output_2_s0_x < 0 + 720; _output_2_s0_x++)
       {
        int32_t _1498 = _output_2_s0_y * _output_2_stride_1;
        int32_t _1499 = _output_2_s0_x + _1498;
        int32_t _1500 = _output_2_s0_c * _output_2_stride_2;
        int32_t _1501 = _1499 + _1500;
        int32_t _1502 = _output_2_min_1 * _output_2_stride_1;
        int32_t _1503 = _output_2_min_0 + _1502;
        int32_t _1504 = _output_2_min_2 * _output_2_stride_2;
        int32_t _1505 = _1503 + _1504;
        int32_t _1506 = _1501 - _1505;
        int32_t _1507 = _output_2_s0_x * 3;
        int32_t _1508 = _output_2_s0_c + _1507;
        int32_t _1509 = _output_2_s0_y * 2160;
        int32_t _1510 = _1508 + _1509;
        uint8_t _1511 = _hw_output_2[_1510];
        _output_2[_1506] = _1511;
       } // for _output_2_s0_x
      } // for _output_2_s0_y
     } // for _output_2_s0_c
     // consume output$2
     halide_free(NULL, _hw_output_2);
    } // alloc _hw_output_2
    halide_free(NULL, __auto_insert__padded1_2);
   } // alloc __auto_insert__padded1_2
   halide_free(NULL, __auto_insert__padded2_2);
  } // alloc __auto_insert__padded2_2
 } // if _5
 return 0;
}


int pipeline_hls(buffer_t *_p2_input1_buffer, buffer_t *_p2_input2_buffer, buffer_t *_output_2_buffer) HALIDE_FUNCTION_ATTRS {
 uint8_t *_p2_input1 = (uint8_t *)(_p2_input1_buffer->host);
 (void)_p2_input1;
 const bool _p2_input1_host_and_dev_are_null = (_p2_input1_buffer->host == NULL) && (_p2_input1_buffer->dev == 0);
 (void)_p2_input1_host_and_dev_are_null;
 const int32_t _p2_input1_min_0 = _p2_input1_buffer->min[0];
 (void)_p2_input1_min_0;
 const int32_t _p2_input1_min_1 = _p2_input1_buffer->min[1];
 (void)_p2_input1_min_1;
 const int32_t _p2_input1_min_2 = _p2_input1_buffer->min[2];
 (void)_p2_input1_min_2;
 const int32_t _p2_input1_min_3 = _p2_input1_buffer->min[3];
 (void)_p2_input1_min_3;
 const int32_t _p2_input1_extent_0 = _p2_input1_buffer->extent[0];
 (void)_p2_input1_extent_0;
 const int32_t _p2_input1_extent_1 = _p2_input1_buffer->extent[1];
 (void)_p2_input1_extent_1;
 const int32_t _p2_input1_extent_2 = _p2_input1_buffer->extent[2];
 (void)_p2_input1_extent_2;
 const int32_t _p2_input1_extent_3 = _p2_input1_buffer->extent[3];
 (void)_p2_input1_extent_3;
 const int32_t _p2_input1_stride_0 = _p2_input1_buffer->stride[0];
 (void)_p2_input1_stride_0;
 const int32_t _p2_input1_stride_1 = _p2_input1_buffer->stride[1];
 (void)_p2_input1_stride_1;
 const int32_t _p2_input1_stride_2 = _p2_input1_buffer->stride[2];
 (void)_p2_input1_stride_2;
 const int32_t _p2_input1_stride_3 = _p2_input1_buffer->stride[3];
 (void)_p2_input1_stride_3;
 const int32_t _p2_input1_elem_size = _p2_input1_buffer->elem_size;
 (void)_p2_input1_elem_size;
 uint8_t *_p2_input2 = (uint8_t *)(_p2_input2_buffer->host);
 (void)_p2_input2;
 const bool _p2_input2_host_and_dev_are_null = (_p2_input2_buffer->host == NULL) && (_p2_input2_buffer->dev == 0);
 (void)_p2_input2_host_and_dev_are_null;
 const int32_t _p2_input2_min_0 = _p2_input2_buffer->min[0];
 (void)_p2_input2_min_0;
 const int32_t _p2_input2_min_1 = _p2_input2_buffer->min[1];
 (void)_p2_input2_min_1;
 const int32_t _p2_input2_min_2 = _p2_input2_buffer->min[2];
 (void)_p2_input2_min_2;
 const int32_t _p2_input2_min_3 = _p2_input2_buffer->min[3];
 (void)_p2_input2_min_3;
 const int32_t _p2_input2_extent_0 = _p2_input2_buffer->extent[0];
 (void)_p2_input2_extent_0;
 const int32_t _p2_input2_extent_1 = _p2_input2_buffer->extent[1];
 (void)_p2_input2_extent_1;
 const int32_t _p2_input2_extent_2 = _p2_input2_buffer->extent[2];
 (void)_p2_input2_extent_2;
 const int32_t _p2_input2_extent_3 = _p2_input2_buffer->extent[3];
 (void)_p2_input2_extent_3;
 const int32_t _p2_input2_stride_0 = _p2_input2_buffer->stride[0];
 (void)_p2_input2_stride_0;
 const int32_t _p2_input2_stride_1 = _p2_input2_buffer->stride[1];
 (void)_p2_input2_stride_1;
 const int32_t _p2_input2_stride_2 = _p2_input2_buffer->stride[2];
 (void)_p2_input2_stride_2;
 const int32_t _p2_input2_stride_3 = _p2_input2_buffer->stride[3];
 (void)_p2_input2_stride_3;
 const int32_t _p2_input2_elem_size = _p2_input2_buffer->elem_size;
 (void)_p2_input2_elem_size;
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
 int32_t _1512 = __pipeline_hls(_p2_input1_buffer, _p2_input2_buffer, _output_2_buffer);
 bool _1513 = _1512 == 0;
 if (!_1513)  {
  return _1512;
 }
 return 0;
}
#ifdef __cplusplus
}  // extern "C"
#endif
