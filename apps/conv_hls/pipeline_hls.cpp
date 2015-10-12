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

inline int8_t abs_i8(int8_t a) {return a >= 0 ? a : -a;}
inline int16_t abs_i16(int16_t a) {return a >= 0 ? a : -a;}
inline int32_t abs_i32(int32_t a) {return a >= 0 ? a : -a;}
inline int64_t abs_i64(int64_t a) {return a >= 0 ? a : -a;}
inline float abs_f32(float a) {return fabsf(a);}
inline double abs_f64(double a) {return fabs(a);}

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
template<typename T> T smod(T a, T b) {T result = a % b; if (result < 0) result += b < 0 ? -b : b; return result;}
template<typename T> T sdiv(T a, T b) {T q = a / b; T r = a - q*b; int bs = b >> (8*sizeof(T) - 1); int rs = r >> (8*sizeof(T) - 1); return q - (rs & bs) + (rs & ~bs);}
template<typename A, typename B> A reinterpret(B b) {A a; memcpy(&a, &b, sizeof(a)); return a;}

static bool halide_rewrite_buffer(buffer_t *b, int32_t elem_size,
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

static int __pipeline_hls(buffer_t *_input_buffer, buffer_t *_weight_buffer, const uint8_t _p2___bias, buffer_t *_output__2_buffer) HALIDE_FUNCTION_ATTRS {
 uint8_t *_input = (uint8_t *)(_input_buffer->host);
 (void)_input;
 const bool _input_host_and_dev_are_null = (_input_buffer->host == NULL) && (_input_buffer->dev == 0);
 (void)_input_host_and_dev_are_null;
 const int32_t _input_min_0 = _input_buffer->min[0];
 (void)_input_min_0;
 const int32_t _input_min_1 = _input_buffer->min[1];
 (void)_input_min_1;
 const int32_t _input_min_2 = _input_buffer->min[2];
 (void)_input_min_2;
 const int32_t _input_min_3 = _input_buffer->min[3];
 (void)_input_min_3;
 const int32_t _input_extent_0 = _input_buffer->extent[0];
 (void)_input_extent_0;
 const int32_t _input_extent_1 = _input_buffer->extent[1];
 (void)_input_extent_1;
 const int32_t _input_extent_2 = _input_buffer->extent[2];
 (void)_input_extent_2;
 const int32_t _input_extent_3 = _input_buffer->extent[3];
 (void)_input_extent_3;
 const int32_t _input_stride_0 = _input_buffer->stride[0];
 (void)_input_stride_0;
 const int32_t _input_stride_1 = _input_buffer->stride[1];
 (void)_input_stride_1;
 const int32_t _input_stride_2 = _input_buffer->stride[2];
 (void)_input_stride_2;
 const int32_t _input_stride_3 = _input_buffer->stride[3];
 (void)_input_stride_3;
 const int32_t _input_elem_size = _input_buffer->elem_size;
 (void)_input_elem_size;
 uint8_t *_weight = (uint8_t *)(_weight_buffer->host);
 (void)_weight;
 const bool _weight_host_and_dev_are_null = (_weight_buffer->host == NULL) && (_weight_buffer->dev == 0);
 (void)_weight_host_and_dev_are_null;
 const int32_t _weight_min_0 = _weight_buffer->min[0];
 (void)_weight_min_0;
 const int32_t _weight_min_1 = _weight_buffer->min[1];
 (void)_weight_min_1;
 const int32_t _weight_min_2 = _weight_buffer->min[2];
 (void)_weight_min_2;
 const int32_t _weight_min_3 = _weight_buffer->min[3];
 (void)_weight_min_3;
 const int32_t _weight_extent_0 = _weight_buffer->extent[0];
 (void)_weight_extent_0;
 const int32_t _weight_extent_1 = _weight_buffer->extent[1];
 (void)_weight_extent_1;
 const int32_t _weight_extent_2 = _weight_buffer->extent[2];
 (void)_weight_extent_2;
 const int32_t _weight_extent_3 = _weight_buffer->extent[3];
 (void)_weight_extent_3;
 const int32_t _weight_stride_0 = _weight_buffer->stride[0];
 (void)_weight_stride_0;
 const int32_t _weight_stride_1 = _weight_buffer->stride[1];
 (void)_weight_stride_1;
 const int32_t _weight_stride_2 = _weight_buffer->stride[2];
 (void)_weight_stride_2;
 const int32_t _weight_stride_3 = _weight_buffer->stride[3];
 (void)_weight_stride_3;
 const int32_t _weight_elem_size = _weight_buffer->elem_size;
 (void)_weight_elem_size;
 uint8_t *_output__2 = (uint8_t *)(_output__2_buffer->host);
 (void)_output__2;
 const bool _output__2_host_and_dev_are_null = (_output__2_buffer->host == NULL) && (_output__2_buffer->dev == 0);
 (void)_output__2_host_and_dev_are_null;
 const int32_t _output__2_min_0 = _output__2_buffer->min[0];
 (void)_output__2_min_0;
 const int32_t _output__2_min_1 = _output__2_buffer->min[1];
 (void)_output__2_min_1;
 const int32_t _output__2_min_2 = _output__2_buffer->min[2];
 (void)_output__2_min_2;
 const int32_t _output__2_min_3 = _output__2_buffer->min[3];
 (void)_output__2_min_3;
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
 const int32_t _output__2_elem_size = _output__2_buffer->elem_size;
 (void)_output__2_elem_size;
 int32_t _2 = _output__2_min_0 + _output__2_extent_0;
 int32_t _3 = _2 + 3;
 int32_t _4 = _input_min_0 + _input_extent_0;
 int32_t _5 = _4 + -1;
 int32_t _6 = min(_3, _5);
 int32_t _7 = max(_6, _input_min_0);
 int32_t _8 = _output__2_min_0 + -4;
 int32_t _9 = min(_8, _5);
 int32_t _10 = max(_9, _input_min_0);
 int32_t _11 = _7 - _10;
 int32_t _12 = _output__2_min_1 + _output__2_extent_1;
 int32_t _13 = _12 + 3;
 int32_t _14 = _input_min_1 + _input_extent_1;
 int32_t _15 = _14 + -1;
 int32_t _16 = min(_13, _15);
 int32_t _17 = max(_16, _input_min_1);
 int32_t _18 = _output__2_min_1 + -4;
 int32_t _19 = min(_18, _15);
 int32_t _20 = max(_19, _input_min_1);
 int32_t _21 = _17 - _20;
 int32_t _22 = _output__2_min_2 + _output__2_extent_2;
 int32_t _23 = _input_min_2 + _input_extent_2;
 int32_t _24 = min(_22, _23);
 int32_t _25 = _24 + -1;
 int32_t _26 = max(_25, _input_min_2);
 int32_t _27 = _23 + -1;
 int32_t _28 = min(_output__2_min_2, _27);
 int32_t _29 = max(_28, _input_min_2);
 int32_t _30 = _26 - _29;
 int32_t _31 = _11 + 1;
 int32_t _32 = _21 + 1;
 int32_t _33 = _31 * _32;
 int32_t _34 = _output__2_extent_0 + -1;
 int32_t _35 = _34 >> 8;
 int32_t _36 = _35 * 256;
 int32_t _37 = _36 + _output__2_min_0;
 int32_t _38 = _37 + 255;
 int32_t _39 = _2 + -1;
 int32_t _40 = min(_38, _39);
 int32_t _41 = _2 + -256;
 int32_t _42 = min(_output__2_min_0, _41);
 int32_t _43 = _40 - _42;
 int32_t _44 = _output__2_extent_1 + -1;
 int32_t _45 = _44 >> 8;
 int32_t _46 = _45 * 256;
 int32_t _47 = _46 + _output__2_min_1;
 int32_t _48 = _47 + 255;
 int32_t _49 = _12 + -1;
 int32_t _50 = min(_48, _49);
 int32_t _51 = _12 + -256;
 int32_t _52 = min(_output__2_min_1, _51);
 int32_t _53 = _50 - _52;
 int32_t _54 = _43 + 1;
 int32_t _55 = _53 + 1;
 int32_t _56 = _54 * _55;
 if (_input_host_and_dev_are_null)
 {
  int32_t _57 = _11 + 1;
  int32_t _58 = _21 + 1;
  int32_t _59 = _30 + 1;
  bool _60 = halide_rewrite_buffer(_input_buffer, 1, _10, _57, 1, _20, _58, _57, _29, _59, _33, 0, 0, 0);
  (void)_60;
 } // if _input_host_and_dev_are_null
 if (_output__2_host_and_dev_are_null)
 {
  int32_t _61 = _43 + 1;
  int32_t _62 = _53 + 1;
  bool _63 = halide_rewrite_buffer(_output__2_buffer, 1, _42, _61, 1, _52, _62, _61, _output__2_min_2, _output__2_extent_2, _56, 0, 0, 0);
  (void)_63;
 } // if _output__2_host_and_dev_are_null
 if (_weight_host_and_dev_are_null)
 {
  bool _64 = halide_rewrite_buffer(_weight_buffer, 1, 0, 5, 1, 0, 5, 5, 0, 0, 0, 0, 0, 0);
  (void)_64;
 } // if _weight_host_and_dev_are_null
 bool _65 = _input_host_and_dev_are_null || _output__2_host_and_dev_are_null;
 bool _66 = _65 || _weight_host_and_dev_are_null;
 bool _67 = !(_66);
 if (_67)
 {
  bool _68 = _input_elem_size == 1;
  if (!_68)   {
   int32_t _69 = halide_error_bad_elem_size(NULL, "Input buffer input", "uint8", _input_elem_size, 1);
   return _69;
  }
  bool _70 = _output__2_elem_size == 1;
  if (!_70)   {
   int32_t _71 = halide_error_bad_elem_size(NULL, "Output buffer output$2", "uint8", _output__2_elem_size, 1);
   return _71;
  }
  bool _72 = _weight_elem_size == 1;
  if (!_72)   {
   int32_t _73 = halide_error_bad_elem_size(NULL, "Input buffer weight", "uint8", _weight_elem_size, 1);
   return _73;
  }
  bool _74 = _input_min_0 <= _10;
  int32_t _75 = _10 + _11;
  int32_t _76 = _75 - _input_extent_0;
  int32_t _77 = _76 + 1;
  bool _78 = _77 <= _input_min_0;
  bool _79 = _74 && _78;
  if (!_79)   {
   int32_t _80 = _10 + _11;
   int32_t _81 = _input_min_0 + _input_extent_0;
   int32_t _82 = _81 + -1;
   int32_t _83 = halide_error_access_out_of_bounds(NULL, "Input buffer input", 0, _10, _80, _input_min_0, _82);
   return _83;
  }
  bool _84 = _input_min_1 <= _20;
  int32_t _85 = _20 + _21;
  int32_t _86 = _85 - _input_extent_1;
  int32_t _87 = _86 + 1;
  bool _88 = _87 <= _input_min_1;
  bool _89 = _84 && _88;
  if (!_89)   {
   int32_t _90 = _20 + _21;
   int32_t _91 = _input_min_1 + _input_extent_1;
   int32_t _92 = _91 + -1;
   int32_t _93 = halide_error_access_out_of_bounds(NULL, "Input buffer input", 1, _20, _90, _input_min_1, _92);
   return _93;
  }
  bool _94 = _input_min_2 <= _29;
  int32_t _95 = _29 + _30;
  int32_t _96 = _95 - _input_extent_2;
  int32_t _97 = _96 + 1;
  bool _98 = _97 <= _input_min_2;
  bool _99 = _94 && _98;
  if (!_99)   {
   int32_t _100 = _29 + _30;
   int32_t _101 = _input_min_2 + _input_extent_2;
   int32_t _102 = _101 + -1;
   int32_t _103 = halide_error_access_out_of_bounds(NULL, "Input buffer input", 2, _29, _100, _input_min_2, _102);
   return _103;
  }
  bool _104 = _output__2_min_0 <= _42;
  int32_t _105 = _42 + _43;
  int32_t _106 = _105 - _output__2_extent_0;
  int32_t _107 = _106 + 1;
  bool _108 = _107 <= _output__2_min_0;
  bool _109 = _104 && _108;
  if (!_109)   {
   int32_t _110 = _42 + _43;
   int32_t _111 = _output__2_min_0 + _output__2_extent_0;
   int32_t _112 = _111 + -1;
   int32_t _113 = halide_error_access_out_of_bounds(NULL, "Output buffer output$2", 0, _42, _110, _output__2_min_0, _112);
   return _113;
  }
  bool _114 = _output__2_min_1 <= _52;
  int32_t _115 = _52 + _53;
  int32_t _116 = _115 - _output__2_extent_1;
  int32_t _117 = _116 + 1;
  bool _118 = _117 <= _output__2_min_1;
  bool _119 = _114 && _118;
  if (!_119)   {
   int32_t _120 = _52 + _53;
   int32_t _121 = _output__2_min_1 + _output__2_extent_1;
   int32_t _122 = _121 + -1;
   int32_t _123 = halide_error_access_out_of_bounds(NULL, "Output buffer output$2", 1, _52, _120, _output__2_min_1, _122);
   return _123;
  }
  bool _124 = _weight_min_0 <= 0;
  int32_t _125 = 5 - _weight_extent_0;
  bool _126 = _125 <= _weight_min_0;
  bool _127 = _124 && _126;
  if (!_127)   {
   int32_t _128 = _weight_min_0 + _weight_extent_0;
   int32_t _129 = _128 + -1;
   int32_t _130 = halide_error_access_out_of_bounds(NULL, "Input buffer weight", 0, 0, 4, _weight_min_0, _129);
   return _130;
  }
  bool _131 = _weight_min_1 <= 0;
  int32_t _132 = 5 - _weight_extent_1;
  bool _133 = _132 <= _weight_min_1;
  bool _134 = _131 && _133;
  if (!_134)   {
   int32_t _135 = _weight_min_1 + _weight_extent_1;
   int32_t _136 = _135 + -1;
   int32_t _137 = halide_error_access_out_of_bounds(NULL, "Input buffer weight", 1, 0, 4, _weight_min_1, _136);
   return _137;
  }
  bool _138 = _input_stride_0 == 1;
  if (!_138)   {
   int32_t _139 = halide_error_constraint_violated(NULL, "input.stride.0", _input_stride_0, "1", 1);
   return _139;
  }
  bool _140 = _output__2_stride_0 == 1;
  if (!_140)   {
   int32_t _141 = halide_error_constraint_violated(NULL, "output$2.stride.0", _output__2_stride_0, "1", 1);
   return _141;
  }
  bool _142 = _weight_stride_0 == 1;
  if (!_142)   {
   int32_t _143 = halide_error_constraint_violated(NULL, "weight.stride.0", _weight_stride_0, "1", 1);
   return _143;
  }
  bool _144 = _weight_min_0 == 0;
  if (!_144)   {
   int32_t _145 = halide_error_constraint_violated(NULL, "weight.min.0", _weight_min_0, "0", 0);
   return _145;
  }
  bool _146 = _weight_extent_0 == 5;
  if (!_146)   {
   int32_t _147 = halide_error_constraint_violated(NULL, "weight.extent.0", _weight_extent_0, "5", 5);
   return _147;
  }
  bool _148 = _weight_stride_1 == 5;
  if (!_148)   {
   int32_t _149 = halide_error_constraint_violated(NULL, "weight.stride.1", _weight_stride_1, "5", 5);
   return _149;
  }
  bool _150 = _weight_min_1 == 0;
  if (!_150)   {
   int32_t _151 = halide_error_constraint_violated(NULL, "weight.min.1", _weight_min_1, "0", 0);
   return _151;
  }
  bool _152 = _weight_extent_1 == 5;
  if (!_152)   {
   int32_t _153 = halide_error_constraint_violated(NULL, "weight.extent.1", _weight_extent_1, "5", 5);
   return _153;
  }
  int64_t _154 = (int64_t)(_input_extent_1);
  int64_t _155 = (int64_t)(_input_extent_0);
  int64_t _156 = _154 * _155;
  int64_t _157 = (int64_t)(_output__2_extent_1);
  int64_t _158 = (int64_t)(_output__2_extent_0);
  int64_t _159 = _157 * _158;
  int64_t _160 = (int64_t)(2147483647);
  bool _161 = _155 <= _160;
  if (!_161)   {
   int64_t _162 = (int64_t)(_input_extent_0);
   int64_t _163 = (int64_t)(2147483647);
   int32_t _164 = halide_error_buffer_allocation_too_large(NULL, "input", _162, _163);
   return _164;
  }
  int64_t _165 = (int64_t)(_input_extent_1);
  int64_t _166 = (int64_t)(_input_stride_1);
  int64_t _167 = _165 * _166;
  int64_t _168 = (int64_t)(2147483647);
  bool _169 = _167 <= _168;
  if (!_169)   {
   int64_t _170 = (int64_t)(_input_extent_1);
   int64_t _171 = (int64_t)(_input_stride_1);
   int64_t _172 = _170 * _171;
   int64_t _173 = (int64_t)(2147483647);
   int32_t _174 = halide_error_buffer_allocation_too_large(NULL, "input", _172, _173);
   return _174;
  }
  int64_t _175 = (int64_t)(2147483647);
  bool _176 = _156 <= _175;
  if (!_176)   {
   int64_t _177 = (int64_t)(2147483647);
   int32_t _178 = halide_error_buffer_extents_too_large(NULL, "input", _156, _177);
   return _178;
  }
  int64_t _179 = (int64_t)(_input_extent_2);
  int64_t _180 = (int64_t)(_input_stride_2);
  int64_t _181 = _179 * _180;
  int64_t _182 = (int64_t)(2147483647);
  bool _183 = _181 <= _182;
  if (!_183)   {
   int64_t _184 = (int64_t)(_input_extent_2);
   int64_t _185 = (int64_t)(_input_stride_2);
   int64_t _186 = _184 * _185;
   int64_t _187 = (int64_t)(2147483647);
   int32_t _188 = halide_error_buffer_allocation_too_large(NULL, "input", _186, _187);
   return _188;
  }
  int64_t _189 = (int64_t)(_input_extent_2);
  int64_t _190 = _189 * _156;
  int64_t _191 = (int64_t)(2147483647);
  bool _192 = _190 <= _191;
  if (!_192)   {
   int64_t _193 = (int64_t)(_input_extent_2);
   int64_t _194 = _193 * _156;
   int64_t _195 = (int64_t)(2147483647);
   int32_t _196 = halide_error_buffer_extents_too_large(NULL, "input", _194, _195);
   return _196;
  }
  int64_t _197 = (int64_t)(_output__2_extent_0);
  int64_t _198 = (int64_t)(2147483647);
  bool _199 = _197 <= _198;
  if (!_199)   {
   int64_t _200 = (int64_t)(_output__2_extent_0);
   int64_t _201 = (int64_t)(2147483647);
   int32_t _202 = halide_error_buffer_allocation_too_large(NULL, "output$2", _200, _201);
   return _202;
  }
  int64_t _203 = (int64_t)(_output__2_extent_1);
  int64_t _204 = (int64_t)(_output__2_stride_1);
  int64_t _205 = _203 * _204;
  int64_t _206 = (int64_t)(2147483647);
  bool _207 = _205 <= _206;
  if (!_207)   {
   int64_t _208 = (int64_t)(_output__2_extent_1);
   int64_t _209 = (int64_t)(_output__2_stride_1);
   int64_t _210 = _208 * _209;
   int64_t _211 = (int64_t)(2147483647);
   int32_t _212 = halide_error_buffer_allocation_too_large(NULL, "output$2", _210, _211);
   return _212;
  }
  int64_t _213 = (int64_t)(2147483647);
  bool _214 = _159 <= _213;
  if (!_214)   {
   int64_t _215 = (int64_t)(2147483647);
   int32_t _216 = halide_error_buffer_extents_too_large(NULL, "output$2", _159, _215);
   return _216;
  }
  int64_t _217 = (int64_t)(_output__2_extent_2);
  int64_t _218 = (int64_t)(_output__2_stride_2);
  int64_t _219 = _217 * _218;
  int64_t _220 = (int64_t)(2147483647);
  bool _221 = _219 <= _220;
  if (!_221)   {
   int64_t _222 = (int64_t)(_output__2_extent_2);
   int64_t _223 = (int64_t)(_output__2_stride_2);
   int64_t _224 = _222 * _223;
   int64_t _225 = (int64_t)(2147483647);
   int32_t _226 = halide_error_buffer_allocation_too_large(NULL, "output$2", _224, _225);
   return _226;
  }
  int64_t _227 = (int64_t)(_output__2_extent_2);
  int64_t _228 = _227 * _159;
  int64_t _229 = (int64_t)(2147483647);
  bool _230 = _228 <= _229;
  if (!_230)   {
   int64_t _231 = (int64_t)(_output__2_extent_2);
   int64_t _232 = _231 * _159;
   int64_t _233 = (int64_t)(2147483647);
   int32_t _234 = halide_error_buffer_extents_too_large(NULL, "output$2", _232, _233);
   return _234;
  }
  // produce output$2
  for (int _output__2_s0_c = _output__2_min_2; _output__2_s0_c < _output__2_min_2 + _output__2_extent_2; _output__2_s0_c++)
  {
   int32_t _235 = _output__2_extent_1 + 255;
   int32_t _236 = _235 >> 8;
   for (int _output__2_s0_y_yo = 0; _output__2_s0_y_yo < 0 + _236; _output__2_s0_y_yo++)
   {
    int32_t _237 = _output__2_s0_y_yo * 256;
    int32_t _238 = _237 + _output__2_min_1;
    int32_t _239 = _output__2_min_1 + _output__2_extent_1;
    int32_t _240 = _239 + -256;
    int32_t _241 = min(_238, _240);
    int32_t _242 = _output__2_extent_0 + 255;
    int32_t _243 = _242 >> 8;
    for (int _output__2_s0_x_xo = 0; _output__2_s0_x_xo < 0 + _243; _output__2_s0_x_xo++)
    {
     int32_t _244 = _output__2_s0_x_xo * 256;
     int32_t _245 = _244 + _output__2_min_0;
     int32_t _246 = _output__2_min_0 + _output__2_extent_0;
     int32_t _247 = _246 + -256;
     int32_t _248 = min(_245, _247);
     {
      hls::stream<Stencil<uint8_t, 1, 1, 1> > _repeat_edge__2_stencil_update_stream;
      // produce repeat_edge$2.stencil_update.stream
      for (int _repeat_edge__2_scan_update__1 = 0; _repeat_edge__2_scan_update__1 < 0 + 264; _repeat_edge__2_scan_update__1++)
      {
       int32_t _249 = _input_min_0 - _248;
       int32_t _250 = _249 + 4;
       int32_t _251 = min(_250, 264);
       int32_t _252 = max(_251, 0);
       int32_t _253 = _input_min_0 + _input_extent_0;
       int32_t _254 = _253 - _248;
       int32_t _255 = _254 + 4;
       int32_t _256 = min(_255, 264);
       int32_t _257 = max(_256, _252);
       for (int _repeat_edge__2_scan_update__0 = 0; _repeat_edge__2_scan_update__0 < 0 + _252; _repeat_edge__2_scan_update__0++)
       {
        {
         Stencil<uint8_t, 1, 1, 1> _repeat_edge__2_stencil;
         // produce repeat_edge$2.stencil
         int32_t _258 = _248 + _repeat_edge__2_scan_update__0;
         int32_t _259 = _258 + -4;
         int32_t _260 = _input_min_0 + _input_extent_0;
         int32_t _261 = _260 + -1;
         int32_t _262 = min(_259, _261);
         int32_t _263 = max(_262, _input_min_0);
         int32_t _264 = _241 + _repeat_edge__2_scan_update__1;
         int32_t _265 = _264 + -4;
         int32_t _266 = _input_min_1 + _input_extent_1;
         int32_t _267 = _266 + -1;
         int32_t _268 = min(_265, _267);
         int32_t _269 = max(_268, _input_min_1);
         int32_t _270 = _269 * _input_stride_1;
         int32_t _271 = _263 + _270;
         int32_t _272 = _input_min_2 + _input_extent_2;
         int32_t _273 = _272 + -1;
         int32_t _274 = min(_output__2_s0_c, _273);
         int32_t _275 = max(_274, _input_min_2);
         int32_t _276 = _275 * _input_stride_2;
         int32_t _277 = _271 + _276;
         int32_t _278 = _input_min_1 * _input_stride_1;
         int32_t _279 = _input_min_0 + _278;
         int32_t _280 = _input_min_2 * _input_stride_2;
         int32_t _281 = _279 + _280;
         int32_t _282 = _277 - _281;
         uint8_t _283 = _input[_282];
         _repeat_edge__2_stencil(0, 0, 0) = _283;
         // consume repeat_edge$2.stencil
         _repeat_edge__2_stencil_update_stream.write(_repeat_edge__2_stencil);
         (void)0;
        } // realize _repeat_edge__2_stencil
       } // for _repeat_edge__2_scan_update__0
       int32_t _284 = _257 - _252;
       for (int _repeat_edge__2_scan_update__0 = _252; _repeat_edge__2_scan_update__0 < _252 + _284; _repeat_edge__2_scan_update__0++)
       {
        {
         Stencil<uint8_t, 1, 1, 1> _repeat_edge__2_stencil;
         // produce repeat_edge$2.stencil
         int32_t _285 = _248 + _repeat_edge__2_scan_update__0;
         int32_t _286 = _241 + _repeat_edge__2_scan_update__1;
         int32_t _287 = _286 + -4;
         int32_t _288 = _input_min_1 + _input_extent_1;
         int32_t _289 = _288 + -1;
         int32_t _290 = min(_287, _289);
         int32_t _291 = max(_290, _input_min_1);
         int32_t _292 = _291 * _input_stride_1;
         int32_t _293 = _285 + _292;
         int32_t _294 = _input_min_2 + _input_extent_2;
         int32_t _295 = _294 + -1;
         int32_t _296 = min(_output__2_s0_c, _295);
         int32_t _297 = max(_296, _input_min_2);
         int32_t _298 = _297 * _input_stride_2;
         int32_t _299 = _293 + _298;
         int32_t _300 = _input_min_1 * _input_stride_1;
         int32_t _301 = _input_min_0 + _300;
         int32_t _302 = _input_min_2 * _input_stride_2;
         int32_t _303 = _301 + _302;
         int32_t _304 = _299 - _303;
         int32_t _305 = _304 + -4;
         uint8_t _306 = _input[_305];
         _repeat_edge__2_stencil(0, 0, 0) = _306;
         // consume repeat_edge$2.stencil
         _repeat_edge__2_stencil_update_stream.write(_repeat_edge__2_stencil);
         (void)0;
        } // realize _repeat_edge__2_stencil
       } // for _repeat_edge__2_scan_update__0
       int32_t _307 = 264 - _257;
       for (int _repeat_edge__2_scan_update__0 = _257; _repeat_edge__2_scan_update__0 < _257 + _307; _repeat_edge__2_scan_update__0++)
       {
        {
         Stencil<uint8_t, 1, 1, 1> _repeat_edge__2_stencil;
         // produce repeat_edge$2.stencil
         int32_t _308 = _input_min_0 + _input_extent_0;
         int32_t _309 = _308 + -1;
         int32_t _310 = max(_309, _input_min_0);
         int32_t _311 = _241 + _repeat_edge__2_scan_update__1;
         int32_t _312 = _311 + -4;
         int32_t _313 = _input_min_1 + _input_extent_1;
         int32_t _314 = _313 + -1;
         int32_t _315 = min(_312, _314);
         int32_t _316 = max(_315, _input_min_1);
         int32_t _317 = _316 * _input_stride_1;
         int32_t _318 = _310 + _317;
         int32_t _319 = _input_min_2 + _input_extent_2;
         int32_t _320 = _319 + -1;
         int32_t _321 = min(_output__2_s0_c, _320);
         int32_t _322 = max(_321, _input_min_2);
         int32_t _323 = _322 * _input_stride_2;
         int32_t _324 = _318 + _323;
         int32_t _325 = _input_min_1 * _input_stride_1;
         int32_t _326 = _input_min_0 + _325;
         int32_t _327 = _input_min_2 * _input_stride_2;
         int32_t _328 = _326 + _327;
         int32_t _329 = _324 - _328;
         uint8_t _330 = _input[_329];
         _repeat_edge__2_stencil(0, 0, 0) = _330;
         // consume repeat_edge$2.stencil
         _repeat_edge__2_stencil_update_stream.write(_repeat_edge__2_stencil);
         (void)0;
        } // realize _repeat_edge__2_stencil
       } // for _repeat_edge__2_scan_update__0
      } // for _repeat_edge__2_scan_update__1
      // consume repeat_edge$2.stencil_update.stream
      {
       hls::stream<Stencil<uint8_t, 1, 1, 1> > _f3_stencil_stream;
       {
        Stencil<uint8_t, 5, 5> _weight_stencil;
        // produce weight.stencil
        buffer_to_stencil(_weight_buffer, _weight_stencil);
        (void)0;
        // consume weight.stencil
        // produce _hls_target.f3.stencil.stream
        _hls_target_f3_stencil_stream(_f3_stencil_stream, _p2___bias, _repeat_edge__2_stencil_update_stream, _weight_stencil);
        // consume _hls_target.f3.stencil.stream
        for (int _output__2_s0_y_yi = 0; _output__2_s0_y_yi < 0 + 256; _output__2_s0_y_yi++)
        {
         for (int _output__2_s0_x_xi = 0; _output__2_s0_x_xi < 0 + 256; _output__2_s0_x_xi++)
         {
          {
           Stencil<uint8_t, 1, 1, 1> _f3_stencil;
           // produce f3.stencil
           _f3_stencil = _f3_stencil_stream.read();
           (void)0;
           // consume f3.stencil
           int32_t _357 = _248 + _output__2_s0_x_xi;
           int32_t _358 = _241 + _output__2_s0_y_yi;
           int32_t _359 = _358 * _output__2_stride_1;
           int32_t _360 = _357 + _359;
           int32_t _361 = _output__2_s0_c * _output__2_stride_2;
           int32_t _362 = _360 + _361;
           int32_t _363 = _output__2_min_1 * _output__2_stride_1;
           int32_t _364 = _output__2_min_0 + _363;
           int32_t _365 = _output__2_min_2 * _output__2_stride_2;
           int32_t _366 = _364 + _365;
           int32_t _367 = _362 - _366;
           uint8_t _368 = _f3_stencil(0, 0, 0);
           _output__2[_367] = _368;
          } // realize _f3_stencil
         } // for _output__2_s0_x_xi
        } // for _output__2_s0_y_yi
       } // realize _weight_stencil
      } // realize _f3_stencil_stream
     } // realize _repeat_edge__2_stencil_update_stream
    } // for _output__2_s0_x_xo
   } // for _output__2_s0_y_yo
  } // for _output__2_s0_c
  // consume output$2
 } // if _67
 return 0;
}


int pipeline_hls(buffer_t *_input_buffer, buffer_t *_weight_buffer, const uint8_t _p2___bias, buffer_t *_output__2_buffer) HALIDE_FUNCTION_ATTRS {
 uint8_t *_input = (uint8_t *)(_input_buffer->host);
 (void)_input;
 const bool _input_host_and_dev_are_null = (_input_buffer->host == NULL) && (_input_buffer->dev == 0);
 (void)_input_host_and_dev_are_null;
 const int32_t _input_min_0 = _input_buffer->min[0];
 (void)_input_min_0;
 const int32_t _input_min_1 = _input_buffer->min[1];
 (void)_input_min_1;
 const int32_t _input_min_2 = _input_buffer->min[2];
 (void)_input_min_2;
 const int32_t _input_min_3 = _input_buffer->min[3];
 (void)_input_min_3;
 const int32_t _input_extent_0 = _input_buffer->extent[0];
 (void)_input_extent_0;
 const int32_t _input_extent_1 = _input_buffer->extent[1];
 (void)_input_extent_1;
 const int32_t _input_extent_2 = _input_buffer->extent[2];
 (void)_input_extent_2;
 const int32_t _input_extent_3 = _input_buffer->extent[3];
 (void)_input_extent_3;
 const int32_t _input_stride_0 = _input_buffer->stride[0];
 (void)_input_stride_0;
 const int32_t _input_stride_1 = _input_buffer->stride[1];
 (void)_input_stride_1;
 const int32_t _input_stride_2 = _input_buffer->stride[2];
 (void)_input_stride_2;
 const int32_t _input_stride_3 = _input_buffer->stride[3];
 (void)_input_stride_3;
 const int32_t _input_elem_size = _input_buffer->elem_size;
 (void)_input_elem_size;
 uint8_t *_weight = (uint8_t *)(_weight_buffer->host);
 (void)_weight;
 const bool _weight_host_and_dev_are_null = (_weight_buffer->host == NULL) && (_weight_buffer->dev == 0);
 (void)_weight_host_and_dev_are_null;
 const int32_t _weight_min_0 = _weight_buffer->min[0];
 (void)_weight_min_0;
 const int32_t _weight_min_1 = _weight_buffer->min[1];
 (void)_weight_min_1;
 const int32_t _weight_min_2 = _weight_buffer->min[2];
 (void)_weight_min_2;
 const int32_t _weight_min_3 = _weight_buffer->min[3];
 (void)_weight_min_3;
 const int32_t _weight_extent_0 = _weight_buffer->extent[0];
 (void)_weight_extent_0;
 const int32_t _weight_extent_1 = _weight_buffer->extent[1];
 (void)_weight_extent_1;
 const int32_t _weight_extent_2 = _weight_buffer->extent[2];
 (void)_weight_extent_2;
 const int32_t _weight_extent_3 = _weight_buffer->extent[3];
 (void)_weight_extent_3;
 const int32_t _weight_stride_0 = _weight_buffer->stride[0];
 (void)_weight_stride_0;
 const int32_t _weight_stride_1 = _weight_buffer->stride[1];
 (void)_weight_stride_1;
 const int32_t _weight_stride_2 = _weight_buffer->stride[2];
 (void)_weight_stride_2;
 const int32_t _weight_stride_3 = _weight_buffer->stride[3];
 (void)_weight_stride_3;
 const int32_t _weight_elem_size = _weight_buffer->elem_size;
 (void)_weight_elem_size;
 uint8_t *_output__2 = (uint8_t *)(_output__2_buffer->host);
 (void)_output__2;
 const bool _output__2_host_and_dev_are_null = (_output__2_buffer->host == NULL) && (_output__2_buffer->dev == 0);
 (void)_output__2_host_and_dev_are_null;
 const int32_t _output__2_min_0 = _output__2_buffer->min[0];
 (void)_output__2_min_0;
 const int32_t _output__2_min_1 = _output__2_buffer->min[1];
 (void)_output__2_min_1;
 const int32_t _output__2_min_2 = _output__2_buffer->min[2];
 (void)_output__2_min_2;
 const int32_t _output__2_min_3 = _output__2_buffer->min[3];
 (void)_output__2_min_3;
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
 const int32_t _output__2_elem_size = _output__2_buffer->elem_size;
 (void)_output__2_elem_size;
 int32_t _369 = __pipeline_hls(_input_buffer, _weight_buffer, _p2___bias, _output__2_buffer);
 bool _370 = _369 == 0;
 if (!_370)  {
  return _369;
 }
 return 0;
}
#ifdef __cplusplus
}  // extern "C"
#endif
