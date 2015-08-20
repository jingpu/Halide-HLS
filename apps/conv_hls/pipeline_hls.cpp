#include <iostream>
#include <math.h>
#include <float.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <hls_stream.h>
#include "stencil.h"
#ifndef HALIDE_ATTRIBUTE_ALIGN
  #ifdef _MSC_VER
    #define HALIDE_ATTRIBUTE_ALIGN(x) __declspec(align(x))
  #else
    #define HALIDE_ATTRIBUTE_ALIGN(x) __attribute__((aligned(x)))
  #endif
#endif
#ifndef BUFFER_T_DEFINED
#define BUFFER_T_DEFINED
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

inline float maxval_f32() {return FLT_MAX;}
inline float minval_f32() {return -FLT_MAX;}
inline double maxval_f64() {return DBL_MAX;}
inline double minval_f64() {return -DBL_MAX;}
inline uint8_t maxval_u8() {return 0xff;}
inline uint8_t minval_u8() {return 0;}
inline uint16_t maxval_u16() {return 0xffff;}
inline uint16_t minval_u16() {return 0;}
inline uint32_t maxval_u32() {return 0xffffffff;}
inline uint32_t minval_u32() {return 0;}
inline uint64_t maxval_u64() {return 0xffffffffffffffff;}
inline uint64_t minval_u64() {return 0;}
inline int8_t maxval_s8() {return 0x7f;}
inline int8_t minval_s8() {return 0x80;}
inline int16_t maxval_s16() {return 0x7fff;}
inline int16_t minval_s16() {return 0x8000;}
inline int32_t maxval_s32() {return 0x7fffffff;}
inline int32_t minval_s32() {return 0x80000000;}
inline int64_t maxval_s64() {return 0x7fffffffffffffff;}
inline int64_t minval_s64() {return 0x8000000000000000;}

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

static int __pipeline_hls(buffer_t *_input_buffer, buffer_t *_output_buffer) HALIDE_FUNCTION_ATTRS {
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
 uint8_t *_output = (uint8_t *)(_output_buffer->host);
 (void)_output;
 const bool _output_host_and_dev_are_null = (_output_buffer->host == NULL) && (_output_buffer->dev == 0);
 (void)_output_host_and_dev_are_null;
 const int32_t _output_min_0 = _output_buffer->min[0];
 (void)_output_min_0;
 const int32_t _output_min_1 = _output_buffer->min[1];
 (void)_output_min_1;
 const int32_t _output_min_2 = _output_buffer->min[2];
 (void)_output_min_2;
 const int32_t _output_min_3 = _output_buffer->min[3];
 (void)_output_min_3;
 const int32_t _output_extent_0 = _output_buffer->extent[0];
 (void)_output_extent_0;
 const int32_t _output_extent_1 = _output_buffer->extent[1];
 (void)_output_extent_1;
 const int32_t _output_extent_2 = _output_buffer->extent[2];
 (void)_output_extent_2;
 const int32_t _output_extent_3 = _output_buffer->extent[3];
 (void)_output_extent_3;
 const int32_t _output_stride_0 = _output_buffer->stride[0];
 (void)_output_stride_0;
 const int32_t _output_stride_1 = _output_buffer->stride[1];
 (void)_output_stride_1;
 const int32_t _output_stride_2 = _output_buffer->stride[2];
 (void)_output_stride_2;
 const int32_t _output_stride_3 = _output_buffer->stride[3];
 (void)_output_stride_3;
 const int32_t _output_elem_size = _output_buffer->elem_size;
 (void)_output_elem_size;
 int32_t _1 = _output_min_0 + _output_extent_0;
 int32_t _2 = _1 + 3;
 int32_t _3 = _input_min_0 + _input_extent_0;
 int32_t _4 = _3 + -1;
 int32_t _5 = min(_2, _4);
 int32_t _6 = max(_5, _input_min_0);
 int32_t _7 = _output_min_0 + -4;
 int32_t _8 = min(_7, _4);
 int32_t _9 = max(_8, _input_min_0);
 int32_t _10 = _6 - _9;
 int32_t _11 = _output_min_1 + _output_extent_1;
 int32_t _12 = _11 + 3;
 int32_t _13 = _input_min_1 + _input_extent_1;
 int32_t _14 = _13 + -1;
 int32_t _15 = min(_12, _14);
 int32_t _16 = max(_15, _input_min_1);
 int32_t _17 = _output_min_1 + -4;
 int32_t _18 = min(_17, _14);
 int32_t _19 = max(_18, _input_min_1);
 int32_t _20 = _16 - _19;
 int32_t _21 = _output_min_2 + _output_extent_2;
 int32_t _22 = _input_min_2 + _input_extent_2;
 int32_t _23 = min(_21, _22);
 int32_t _24 = _23 + -1;
 int32_t _25 = max(_24, _input_min_2);
 int32_t _26 = _22 + -1;
 int32_t _27 = min(_output_min_2, _26);
 int32_t _28 = max(_27, _input_min_2);
 int32_t _29 = _25 - _28;
 int32_t _30 = _10 + 1;
 int32_t _31 = _20 + 1;
 int32_t _32 = _30 * _31;
 int32_t _33 = _output_extent_0 + -1;
 int32_t _34 = _33 >> 6;
 int32_t _35 = _34 * 64;
 int32_t _36 = _35 + _output_min_0;
 int32_t _37 = _36 + 63;
 int32_t _38 = _1 + -1;
 int32_t _39 = min(_37, _38);
 int32_t _40 = _1 + -64;
 int32_t _41 = min(_output_min_0, _40);
 int32_t _42 = _39 - _41;
 int32_t _43 = _output_extent_1 + -1;
 int32_t _44 = _43 >> 6;
 int32_t _45 = _44 * 64;
 int32_t _46 = _45 + _output_min_1;
 int32_t _47 = _46 + 63;
 int32_t _48 = _11 + -1;
 int32_t _49 = min(_47, _48);
 int32_t _50 = _11 + -64;
 int32_t _51 = min(_output_min_1, _50);
 int32_t _52 = _49 - _51;
 int32_t _53 = _42 + 1;
 int32_t _54 = _52 + 1;
 int32_t _55 = _53 * _54;
 if (_input_host_and_dev_are_null)
 {
  int32_t _56 = _10 + 1;
  int32_t _57 = _20 + 1;
  int32_t _58 = _29 + 1;
  bool _59 = halide_rewrite_buffer(_input_buffer, 1, _9, _56, 1, _19, _57, _56, _28, _58, _32, 0, 0, 0);
  (void)_59;
 } // if _input_host_and_dev_are_null
 if (_output_host_and_dev_are_null)
 {
  int32_t _60 = _42 + 1;
  int32_t _61 = _52 + 1;
  bool _62 = halide_rewrite_buffer(_output_buffer, 1, _41, _60, 1, _51, _61, _60, _output_min_2, _output_extent_2, _55, 0, 0, 0);
  (void)_62;
 } // if _output_host_and_dev_are_null
 bool _63 = _input_host_and_dev_are_null || _output_host_and_dev_are_null;
 bool _64 = !(_63);
 if (_64)
 {
  bool _65 = _input_elem_size == 1;
  if (!_65)   {
   int32_t _66 = halide_error_bad_elem_size(NULL, "Input buffer input", "uint8", _input_elem_size, 1);
   return _66;
  }
  bool _67 = _output_elem_size == 1;
  if (!_67)   {
   int32_t _68 = halide_error_bad_elem_size(NULL, "Output buffer output", "uint8", _output_elem_size, 1);
   return _68;
  }
  bool _69 = _input_min_0 <= _9;
  int32_t _70 = _9 + _10;
  int32_t _71 = _70 - _input_extent_0;
  int32_t _72 = _71 + 1;
  bool _73 = _72 <= _input_min_0;
  bool _74 = _69 && _73;
  if (!_74)   {
   int32_t _75 = _9 + _10;
   int32_t _76 = _input_min_0 + _input_extent_0;
   int32_t _77 = _76 + -1;
   int32_t _78 = halide_error_access_out_of_bounds(NULL, "Input buffer input", 0, _9, _75, _input_min_0, _77);
   return _78;
  }
  bool _79 = _input_min_1 <= _19;
  int32_t _80 = _19 + _20;
  int32_t _81 = _80 - _input_extent_1;
  int32_t _82 = _81 + 1;
  bool _83 = _82 <= _input_min_1;
  bool _84 = _79 && _83;
  if (!_84)   {
   int32_t _85 = _19 + _20;
   int32_t _86 = _input_min_1 + _input_extent_1;
   int32_t _87 = _86 + -1;
   int32_t _88 = halide_error_access_out_of_bounds(NULL, "Input buffer input", 1, _19, _85, _input_min_1, _87);
   return _88;
  }
  bool _89 = _input_min_2 <= _28;
  int32_t _90 = _28 + _29;
  int32_t _91 = _90 - _input_extent_2;
  int32_t _92 = _91 + 1;
  bool _93 = _92 <= _input_min_2;
  bool _94 = _89 && _93;
  if (!_94)   {
   int32_t _95 = _28 + _29;
   int32_t _96 = _input_min_2 + _input_extent_2;
   int32_t _97 = _96 + -1;
   int32_t _98 = halide_error_access_out_of_bounds(NULL, "Input buffer input", 2, _28, _95, _input_min_2, _97);
   return _98;
  }
  bool _99 = _output_min_0 <= _41;
  int32_t _100 = _41 + _42;
  int32_t _101 = _100 - _output_extent_0;
  int32_t _102 = _101 + 1;
  bool _103 = _102 <= _output_min_0;
  bool _104 = _99 && _103;
  if (!_104)   {
   int32_t _105 = _41 + _42;
   int32_t _106 = _output_min_0 + _output_extent_0;
   int32_t _107 = _106 + -1;
   int32_t _108 = halide_error_access_out_of_bounds(NULL, "Output buffer output", 0, _41, _105, _output_min_0, _107);
   return _108;
  }
  bool _109 = _output_min_1 <= _51;
  int32_t _110 = _51 + _52;
  int32_t _111 = _110 - _output_extent_1;
  int32_t _112 = _111 + 1;
  bool _113 = _112 <= _output_min_1;
  bool _114 = _109 && _113;
  if (!_114)   {
   int32_t _115 = _51 + _52;
   int32_t _116 = _output_min_1 + _output_extent_1;
   int32_t _117 = _116 + -1;
   int32_t _118 = halide_error_access_out_of_bounds(NULL, "Output buffer output", 1, _51, _115, _output_min_1, _117);
   return _118;
  }
  bool _119 = _input_stride_0 == 1;
  if (!_119)   {
   int32_t _120 = halide_error_constraint_violated(NULL, "input.stride.0", _input_stride_0, "1", 1);
   return _120;
  }
  bool _121 = _output_stride_0 == 1;
  if (!_121)   {
   int32_t _122 = halide_error_constraint_violated(NULL, "output.stride.0", _output_stride_0, "1", 1);
   return _122;
  }
  int64_t _123 = (int64_t)(_input_extent_1);
  int64_t _124 = (int64_t)(_input_extent_0);
  int64_t _125 = _123 * _124;
  int64_t _126 = (int64_t)(_output_extent_1);
  int64_t _127 = (int64_t)(_output_extent_0);
  int64_t _128 = _126 * _127;
  int64_t _129 = (int64_t)(2147483647);
  bool _130 = _124 <= _129;
  if (!_130)   {
   int64_t _131 = (int64_t)(_input_extent_0);
   int64_t _132 = (int64_t)(2147483647);
   int32_t _133 = halide_error_buffer_allocation_too_large(NULL, "input", _131, _132);
   return _133;
  }
  int64_t _134 = (int64_t)(_input_extent_1);
  int64_t _135 = (int64_t)(_input_stride_1);
  int64_t _136 = _134 * _135;
  int64_t _137 = (int64_t)(2147483647);
  bool _138 = _136 <= _137;
  if (!_138)   {
   int64_t _139 = (int64_t)(_input_extent_1);
   int64_t _140 = (int64_t)(_input_stride_1);
   int64_t _141 = _139 * _140;
   int64_t _142 = (int64_t)(2147483647);
   int32_t _143 = halide_error_buffer_allocation_too_large(NULL, "input", _141, _142);
   return _143;
  }
  int64_t _144 = (int64_t)(2147483647);
  bool _145 = _125 <= _144;
  if (!_145)   {
   int64_t _146 = (int64_t)(2147483647);
   int32_t _147 = halide_error_buffer_extents_too_large(NULL, "input", _125, _146);
   return _147;
  }
  int64_t _148 = (int64_t)(_input_extent_2);
  int64_t _149 = (int64_t)(_input_stride_2);
  int64_t _150 = _148 * _149;
  int64_t _151 = (int64_t)(2147483647);
  bool _152 = _150 <= _151;
  if (!_152)   {
   int64_t _153 = (int64_t)(_input_extent_2);
   int64_t _154 = (int64_t)(_input_stride_2);
   int64_t _155 = _153 * _154;
   int64_t _156 = (int64_t)(2147483647);
   int32_t _157 = halide_error_buffer_allocation_too_large(NULL, "input", _155, _156);
   return _157;
  }
  int64_t _158 = (int64_t)(_input_extent_2);
  int64_t _159 = _158 * _125;
  int64_t _160 = (int64_t)(2147483647);
  bool _161 = _159 <= _160;
  if (!_161)   {
   int64_t _162 = (int64_t)(_input_extent_2);
   int64_t _163 = _162 * _125;
   int64_t _164 = (int64_t)(2147483647);
   int32_t _165 = halide_error_buffer_extents_too_large(NULL, "input", _163, _164);
   return _165;
  }
  int64_t _166 = (int64_t)(_output_extent_0);
  int64_t _167 = (int64_t)(2147483647);
  bool _168 = _166 <= _167;
  if (!_168)   {
   int64_t _169 = (int64_t)(_output_extent_0);
   int64_t _170 = (int64_t)(2147483647);
   int32_t _171 = halide_error_buffer_allocation_too_large(NULL, "output", _169, _170);
   return _171;
  }
  int64_t _172 = (int64_t)(_output_extent_1);
  int64_t _173 = (int64_t)(_output_stride_1);
  int64_t _174 = _172 * _173;
  int64_t _175 = (int64_t)(2147483647);
  bool _176 = _174 <= _175;
  if (!_176)   {
   int64_t _177 = (int64_t)(_output_extent_1);
   int64_t _178 = (int64_t)(_output_stride_1);
   int64_t _179 = _177 * _178;
   int64_t _180 = (int64_t)(2147483647);
   int32_t _181 = halide_error_buffer_allocation_too_large(NULL, "output", _179, _180);
   return _181;
  }
  int64_t _182 = (int64_t)(2147483647);
  bool _183 = _128 <= _182;
  if (!_183)   {
   int64_t _184 = (int64_t)(2147483647);
   int32_t _185 = halide_error_buffer_extents_too_large(NULL, "output", _128, _184);
   return _185;
  }
  int64_t _186 = (int64_t)(_output_extent_2);
  int64_t _187 = (int64_t)(_output_stride_2);
  int64_t _188 = _186 * _187;
  int64_t _189 = (int64_t)(2147483647);
  bool _190 = _188 <= _189;
  if (!_190)   {
   int64_t _191 = (int64_t)(_output_extent_2);
   int64_t _192 = (int64_t)(_output_stride_2);
   int64_t _193 = _191 * _192;
   int64_t _194 = (int64_t)(2147483647);
   int32_t _195 = halide_error_buffer_allocation_too_large(NULL, "output", _193, _194);
   return _195;
  }
  int64_t _196 = (int64_t)(_output_extent_2);
  int64_t _197 = _196 * _128;
  int64_t _198 = (int64_t)(2147483647);
  bool _199 = _197 <= _198;
  if (!_199)   {
   int64_t _200 = (int64_t)(_output_extent_2);
   int64_t _201 = _200 * _128;
   int64_t _202 = (int64_t)(2147483647);
   int32_t _203 = halide_error_buffer_extents_too_large(NULL, "output", _201, _202);
   return _203;
  }
  int32_t _204 = _output_min_1 + _output_extent_1;
  int32_t _205 = _204 + 3;
  int32_t _206 = _output_extent_1 + -1;
  int32_t _207 = _206 >> 6;
  int32_t _208 = _207 * 64;
  int32_t _209 = _208 + _output_min_1;
  int32_t _210 = _209 + 63;
  int32_t _211 = _204 + -1;
  int32_t _212 = min(_210, _211);
  int32_t _213 = _212 + 4;
  int32_t _214 = max(_205, _213);
  int32_t _215 = _output_min_0 + _output_extent_0;
  int32_t _216 = _215 + 3;
  int32_t _217 = _output_extent_0 + -1;
  int32_t _218 = _217 >> 6;
  int32_t _219 = _218 * 64;
  int32_t _220 = _219 + _output_min_0;
  int32_t _221 = _220 + 63;
  int32_t _222 = _215 + -1;
  int32_t _223 = min(_221, _222);
  int32_t _224 = _223 + 4;
  int32_t _225 = max(_216, _224);
  int32_t _226 = _225 - _41;
  int32_t _227 = _226 + 5;
  int32_t _228 = _214 - _51;
  int32_t _229 = _228 + 5;
  int32_t _230 = _227 * _229;
  {
   int32_t _231 = _225 - _41;
   int32_t _232 = _231 + 5;
   int64_t _233 = _232;
   int32_t _234 = _214 - _51;
   int32_t _235 = _234 + 5;
   int64_t _236 = _233 * _235;
   int64_t _237 = (_236 > ((int64_t(1) << 31) - 1)) ? _236 : (_236 * _output_extent_2);
   if ((_237 > ((int64_t(1) << 31) - 1)) || ((_237 * sizeof(uint8_t)) > ((int64_t(1) << 31) - 1)))
   {
    halide_error(NULL, "32-bit signed overflow computing size of allocation repeat_edge\n");
    return -1;
   } // overflow test repeat_edge
   int64_t _238 = _237;
   uint8_t *_repeat_edge = (uint8_t *)halide_malloc(NULL, sizeof(uint8_t)*_238);
   // produce repeat_edge
   for (int _repeat_edge_s0__2 = _output_min_2; _repeat_edge_s0__2 < _output_min_2 + _output_extent_2; _repeat_edge_s0__2++)
   {
    int32_t _239 = _output_min_1 + -4;
    int32_t _240 = _output_extent_1 + 8;
    for (int _repeat_edge_s0__1 = _239; _repeat_edge_s0__1 < _239 + _240; _repeat_edge_s0__1++)
    {
     int32_t _241 = _output_min_0 + _output_extent_0;
     int32_t _242 = _241 + 4;
     int32_t _243 = min(_input_min_0, _242);
     int32_t _244 = _output_min_0 + -4;
     int32_t _245 = max(_243, _244);
     int32_t _246 = _input_min_0 + _input_extent_0;
     int32_t _247 = min(_246, _242);
     int32_t _248 = max(_247, _245);
     int32_t _249 = _245 - _output_min_0;
     int32_t _250 = _249 + 4;
     for (int _repeat_edge_s0__0 = _244; _repeat_edge_s0__0 < _244 + _250; _repeat_edge_s0__0++)
     {
      int32_t _251 = _repeat_edge_s0__0 - _41;
      int32_t _252 = _repeat_edge_s0__1 - _51;
      int32_t _253 = _252 + 4;
      int32_t _254 = _225 - _41;
      int32_t _255 = _254 + 5;
      int32_t _256 = _253 * _255;
      int32_t _257 = _251 + _256;
      int32_t _258 = _repeat_edge_s0__2 - _output_min_2;
      int32_t _259 = _258 * _230;
      int32_t _260 = _257 + _259;
      int32_t _261 = _260 + 4;
      int32_t _262 = _input_min_0 + _input_extent_0;
      int32_t _263 = _262 + -1;
      int32_t _264 = min(_repeat_edge_s0__0, _263);
      int32_t _265 = max(_264, _input_min_0);
      int32_t _266 = _input_min_1 + _input_extent_1;
      int32_t _267 = _266 + -1;
      int32_t _268 = min(_repeat_edge_s0__1, _267);
      int32_t _269 = max(_268, _input_min_1);
      int32_t _270 = _269 * _input_stride_1;
      int32_t _271 = _265 + _270;
      int32_t _272 = _input_min_2 + _input_extent_2;
      int32_t _273 = _272 + -1;
      int32_t _274 = min(_repeat_edge_s0__2, _273);
      int32_t _275 = max(_274, _input_min_2);
      int32_t _276 = _275 * _input_stride_2;
      int32_t _277 = _271 + _276;
      int32_t _278 = _input_min_1 * _input_stride_1;
      int32_t _279 = _input_min_0 + _278;
      int32_t _280 = _input_min_2 * _input_stride_2;
      int32_t _281 = _279 + _280;
      int32_t _282 = _277 - _281;
      uint8_t _283 = _input[_282];
      _repeat_edge[_261] = _283;
     } // for _repeat_edge_s0__0
     int32_t _284 = _248 - _245;
     for (int _repeat_edge_s0__0 = _245; _repeat_edge_s0__0 < _245 + _284; _repeat_edge_s0__0++)
     {
      int32_t _285 = _repeat_edge_s0__0 - _41;
      int32_t _286 = _repeat_edge_s0__1 - _51;
      int32_t _287 = _286 + 4;
      int32_t _288 = _225 - _41;
      int32_t _289 = _288 + 5;
      int32_t _290 = _287 * _289;
      int32_t _291 = _285 + _290;
      int32_t _292 = _repeat_edge_s0__2 - _output_min_2;
      int32_t _293 = _292 * _230;
      int32_t _294 = _291 + _293;
      int32_t _295 = _294 + 4;
      int32_t _296 = _input_min_1 + _input_extent_1;
      int32_t _297 = _296 + -1;
      int32_t _298 = min(_repeat_edge_s0__1, _297);
      int32_t _299 = max(_298, _input_min_1);
      int32_t _300 = _299 * _input_stride_1;
      int32_t _301 = _repeat_edge_s0__0 + _300;
      int32_t _302 = _input_min_2 + _input_extent_2;
      int32_t _303 = _302 + -1;
      int32_t _304 = min(_repeat_edge_s0__2, _303);
      int32_t _305 = max(_304, _input_min_2);
      int32_t _306 = _305 * _input_stride_2;
      int32_t _307 = _301 + _306;
      int32_t _308 = _input_min_1 * _input_stride_1;
      int32_t _309 = _input_min_0 + _308;
      int32_t _310 = _input_min_2 * _input_stride_2;
      int32_t _311 = _309 + _310;
      int32_t _312 = _307 - _311;
      uint8_t _313 = _input[_312];
      _repeat_edge[_295] = _313;
     } // for _repeat_edge_s0__0
     int32_t _314 = _output_min_0 + _output_extent_0;
     int32_t _315 = _314 - _248;
     int32_t _316 = _315 + 4;
     for (int _repeat_edge_s0__0 = _248; _repeat_edge_s0__0 < _248 + _316; _repeat_edge_s0__0++)
     {
      int32_t _317 = _repeat_edge_s0__0 - _41;
      int32_t _318 = _repeat_edge_s0__1 - _51;
      int32_t _319 = _318 + 4;
      int32_t _320 = _225 - _41;
      int32_t _321 = _320 + 5;
      int32_t _322 = _319 * _321;
      int32_t _323 = _317 + _322;
      int32_t _324 = _repeat_edge_s0__2 - _output_min_2;
      int32_t _325 = _324 * _230;
      int32_t _326 = _323 + _325;
      int32_t _327 = _326 + 4;
      int32_t _328 = _input_min_0 + _input_extent_0;
      int32_t _329 = _328 + -1;
      int32_t _330 = max(_329, _input_min_0);
      int32_t _331 = _input_min_1 + _input_extent_1;
      int32_t _332 = _331 + -1;
      int32_t _333 = min(_repeat_edge_s0__1, _332);
      int32_t _334 = max(_333, _input_min_1);
      int32_t _335 = _334 * _input_stride_1;
      int32_t _336 = _330 + _335;
      int32_t _337 = _input_min_2 + _input_extent_2;
      int32_t _338 = _337 + -1;
      int32_t _339 = min(_repeat_edge_s0__2, _338);
      int32_t _340 = max(_339, _input_min_2);
      int32_t _341 = _340 * _input_stride_2;
      int32_t _342 = _336 + _341;
      int32_t _343 = _input_min_1 * _input_stride_1;
      int32_t _344 = _input_min_0 + _343;
      int32_t _345 = _input_min_2 * _input_stride_2;
      int32_t _346 = _344 + _345;
      int32_t _347 = _342 - _346;
      uint8_t _348 = _input[_347];
      _repeat_edge[_327] = _348;
     } // for _repeat_edge_s0__0
    } // for _repeat_edge_s0__1
   } // for _repeat_edge_s0__2
   // consume repeat_edge
   // produce output
   for (int _output_s0_c = _output_min_2; _output_s0_c < _output_min_2 + _output_extent_2; _output_s0_c++)
   {
    int32_t _349 = _output_extent_1 + 63;
    int32_t _350 = _349 >> 6;
    for (int _output_s0_y_yo = 0; _output_s0_y_yo < 0 + _350; _output_s0_y_yo++)
    {
     int32_t _351 = _output_s0_y_yo * 64;
     int32_t _352 = _351 + _output_min_1;
     int32_t _353 = _output_min_1 + _output_extent_1;
     int32_t _354 = _353 + -64;
     int32_t _355 = min(_352, _354);
     int32_t _356 = _output_extent_0 >> 6;
     int32_t _357 = max(_356, 0);
     for (int _output_s0_x_xo = 0; _output_s0_x_xo < 0 + _357; _output_s0_x_xo++)
     {
      {
       hls::stream<Stencil<uint8_t, 5, 5, 1> > _buffered_stencil_stream;
       {
        hls::stream<Stencil<uint16_t, 5, 5, 1> > _conv1_stencil_stream;
        // produce conv1.stencil.stream
        {
         hls::stream<Stencil<uint16_t, 1, 1, 1> > _conv1_stencil_update_stream;
         // produce buffered.stencil.stream
         {
          hls::stream<Stencil<uint8_t, 1, 1, 1> > _buffered_stencil_update_stream;
          // produce buffered.stencil_update.stream
          for (int _buffered_scan_update_y = 0; _buffered_scan_update_y < 0 + 72; _buffered_scan_update_y++)
          {
           for (int _buffered_scan_update_x = 0; _buffered_scan_update_x < 0 + 72; _buffered_scan_update_x++)
           {
            {
             Stencil<uint8_t, 1, 1, 1> _buffered_stencil_update;
             // produce buffered.stencil_update
             int32_t _358 = _output_s0_x_xo * 64;
             int32_t _359 = _358 + _output_min_0;
             int32_t _360 = _359 + _buffered_scan_update_x;
             int32_t _361 = _360 - _41;
             int32_t _362 = _355 + _buffered_scan_update_y;
             int32_t _363 = _362 - _51;
             int32_t _364 = _225 - _41;
             int32_t _365 = _364 + 5;
             int32_t _366 = _363 * _365;
             int32_t _367 = _361 + _366;
             int32_t _368 = _output_s0_c - _output_min_2;
             int32_t _369 = _368 * _230;
             int32_t _370 = _367 + _369;
             uint8_t _371 = _repeat_edge[_370];
             _buffered_stencil_update(0, 0, 0) = _371;
             // consume buffered.stencil_update
             _buffered_stencil_update_stream.write(_buffered_stencil_update);
            } // realize _buffered_stencil_update
           } // for _buffered_scan_update_x
          } // for _buffered_scan_update_y
          // consume buffered.stencil_update.stream
          linebuffer<72, 72, 1>(_buffered_stencil_update_stream, _buffered_stencil_stream);
         } // realize _buffered_stencil_update_stream
         // consume buffered.stencil.stream
         // produce conv1.stencil_update.stream
         for (int _conv1_scan_update_y = 0; _conv1_scan_update_y < 0 + 68; _conv1_scan_update_y++)
         {
          for (int _conv1_scan_update_x = 0; _conv1_scan_update_x < 0 + 68; _conv1_scan_update_x++)
          {
           {
            Stencil<uint8_t, 5, 5, 1> _buffered_stencil;
            // produce buffered.stencil
            _buffered_stencil = _buffered_stencil_stream.read();
            // consume buffered.stencil
            {
             Stencil<uint16_t, 1, 1, 1> _conv1_stencil_update;
             // produce conv1.stencil_update
             uint8_t _372 = _buffered_stencil(0, 0, 0);
             uint16_t _373 = (uint16_t)(_372);
             uint8_t _374 = _buffered_stencil(1, 0, 0);
             uint16_t _375 = (uint16_t)(_374);
             uint16_t _376 = (uint16_t)(3);
             uint16_t _377 = _375 * _376;
             uint16_t _378 = _373 + _377;
             uint8_t _379 = _buffered_stencil(2, 0, 0);
             uint16_t _380 = (uint16_t)(_379);
             uint16_t _381 = (uint16_t)(6);
             uint16_t _382 = _380 * _381;
             uint16_t _383 = _378 + _382;
             uint8_t _384 = _buffered_stencil(3, 0, 0);
             uint16_t _385 = (uint16_t)(_384);
             uint16_t _386 = _385 * _376;
             uint16_t _387 = _383 + _386;
             uint8_t _388 = _buffered_stencil(4, 0, 0);
             uint16_t _389 = (uint16_t)(_388);
             uint16_t _390 = _387 + _389;
             uint8_t _391 = _buffered_stencil(0, 1, 0);
             uint16_t _392 = (uint16_t)(_391);
             uint16_t _393 = _392 * _376;
             uint16_t _394 = _390 + _393;
             uint8_t _395 = _buffered_stencil(1, 1, 0);
             uint16_t _396 = (uint16_t)(_395);
             uint16_t _397 = (uint16_t)(15);
             uint16_t _398 = _396 * _397;
             uint16_t _399 = _394 + _398;
             uint8_t _400 = _buffered_stencil(2, 1, 0);
             uint16_t _401 = (uint16_t)(_400);
             uint16_t _402 = (uint16_t)(25);
             uint16_t _403 = _401 * _402;
             uint16_t _404 = _399 + _403;
             uint8_t _405 = _buffered_stencil(3, 1, 0);
             uint16_t _406 = (uint16_t)(_405);
             uint16_t _407 = _406 * _397;
             uint16_t _408 = _404 + _407;
             uint8_t _409 = _buffered_stencil(4, 1, 0);
             uint16_t _410 = (uint16_t)(_409);
             uint16_t _411 = _410 * _376;
             uint16_t _412 = _408 + _411;
             uint8_t _413 = _buffered_stencil(0, 2, 0);
             uint16_t _414 = (uint16_t)(_413);
             uint16_t _415 = _414 * _381;
             uint16_t _416 = _412 + _415;
             uint8_t _417 = _buffered_stencil(1, 2, 0);
             uint16_t _418 = (uint16_t)(_417);
             uint16_t _419 = _418 * _402;
             uint16_t _420 = _416 + _419;
             uint8_t _421 = _buffered_stencil(2, 2, 0);
             uint16_t _422 = (uint16_t)(_421);
             uint16_t _423 = (uint16_t)(44);
             uint16_t _424 = _422 * _423;
             uint16_t _425 = _420 + _424;
             uint8_t _426 = _buffered_stencil(3, 2, 0);
             uint16_t _427 = (uint16_t)(_426);
             uint16_t _428 = _427 * _402;
             uint16_t _429 = _425 + _428;
             uint8_t _430 = _buffered_stencil(4, 2, 0);
             uint16_t _431 = (uint16_t)(_430);
             uint16_t _432 = _431 * _381;
             uint16_t _433 = _429 + _432;
             uint8_t _434 = _buffered_stencil(0, 3, 0);
             uint16_t _435 = (uint16_t)(_434);
             uint16_t _436 = _435 * _376;
             uint16_t _437 = _433 + _436;
             uint8_t _438 = _buffered_stencil(1, 3, 0);
             uint16_t _439 = (uint16_t)(_438);
             uint16_t _440 = _439 * _397;
             uint16_t _441 = _437 + _440;
             uint8_t _442 = _buffered_stencil(2, 3, 0);
             uint16_t _443 = (uint16_t)(_442);
             uint16_t _444 = _443 * _402;
             uint16_t _445 = _441 + _444;
             uint8_t _446 = _buffered_stencil(3, 3, 0);
             uint16_t _447 = (uint16_t)(_446);
             uint16_t _448 = _447 * _397;
             uint16_t _449 = _445 + _448;
             uint8_t _450 = _buffered_stencil(4, 3, 0);
             uint16_t _451 = (uint16_t)(_450);
             uint16_t _452 = _451 * _376;
             uint16_t _453 = _449 + _452;
             uint8_t _454 = _buffered_stencil(0, 4, 0);
             uint16_t _455 = (uint16_t)(_454);
             uint16_t _456 = _453 + _455;
             uint8_t _457 = _buffered_stencil(1, 4, 0);
             uint16_t _458 = (uint16_t)(_457);
             uint16_t _459 = _458 * _376;
             uint16_t _460 = _456 + _459;
             uint8_t _461 = _buffered_stencil(2, 4, 0);
             uint16_t _462 = (uint16_t)(_461);
             uint16_t _463 = _462 * _381;
             uint16_t _464 = _460 + _463;
             uint8_t _465 = _buffered_stencil(3, 4, 0);
             uint16_t _466 = (uint16_t)(_465);
             uint16_t _467 = _466 * _376;
             uint16_t _468 = _464 + _467;
             uint8_t _469 = _buffered_stencil(4, 4, 0);
             uint16_t _470 = (uint16_t)(_469);
             uint16_t _471 = _468 + _470;
             uint16_t _472 = _471 >> 8;
             _conv1_stencil_update(0, 0, 0) = _472;
             // consume conv1.stencil_update
             _conv1_stencil_update_stream.write(_conv1_stencil_update);
            } // realize _conv1_stencil_update
           } // realize _buffered_stencil
          } // for _conv1_scan_update_x
         } // for _conv1_scan_update_y
         // consume conv1.stencil_update.stream
         linebuffer<68, 68, 1>(_conv1_stencil_update_stream, _conv1_stencil_stream);
        } // realize _conv1_stencil_update_stream
        // consume conv1.stencil.stream
        for (int _output_s0_y_yi = 0; _output_s0_y_yi < 0 + 64; _output_s0_y_yi++)
        {
         for (int _output_s0_x_xi = 0; _output_s0_x_xi < 0 + 64; _output_s0_x_xi++)
         {
          {
           Stencil<uint16_t, 5, 5, 1> _conv1_stencil;
           // produce conv1.stencil
           _conv1_stencil = _conv1_stencil_stream.read();
           // consume conv1.stencil
           int32_t _473 = _output_s0_x_xo * 64;
           int32_t _474 = _473 + _output_min_0;
           int32_t _475 = _474 + _output_s0_x_xi;
           int32_t _476 = _355 + _output_s0_y_yi;
           int32_t _477 = _476 * _output_stride_1;
           int32_t _478 = _475 + _477;
           int32_t _479 = _output_s0_c * _output_stride_2;
           int32_t _480 = _478 + _479;
           int32_t _481 = _output_min_1 * _output_stride_1;
           int32_t _482 = _output_min_0 + _481;
           int32_t _483 = _output_min_2 * _output_stride_2;
           int32_t _484 = _482 + _483;
           int32_t _485 = _480 - _484;
           uint16_t _486 = _conv1_stencil(0, 0, 0);
           uint16_t _487 = _conv1_stencil(1, 0, 0);
           uint16_t _488 = (uint16_t)(3);
           uint16_t _489 = _487 * _488;
           uint16_t _490 = _486 + _489;
           uint16_t _491 = _conv1_stencil(2, 0, 0);
           uint16_t _492 = (uint16_t)(6);
           uint16_t _493 = _491 * _492;
           uint16_t _494 = _490 + _493;
           uint16_t _495 = _conv1_stencil(3, 0, 0);
           uint16_t _496 = _495 * _488;
           uint16_t _497 = _494 + _496;
           uint16_t _498 = _conv1_stencil(4, 0, 0);
           uint16_t _499 = _497 + _498;
           uint16_t _500 = _conv1_stencil(0, 1, 0);
           uint16_t _501 = _500 * _488;
           uint16_t _502 = _499 + _501;
           uint16_t _503 = _conv1_stencil(1, 1, 0);
           uint16_t _504 = (uint16_t)(15);
           uint16_t _505 = _503 * _504;
           uint16_t _506 = _502 + _505;
           uint16_t _507 = _conv1_stencil(2, 1, 0);
           uint16_t _508 = (uint16_t)(25);
           uint16_t _509 = _507 * _508;
           uint16_t _510 = _506 + _509;
           uint16_t _511 = _conv1_stencil(3, 1, 0);
           uint16_t _512 = _511 * _504;
           uint16_t _513 = _510 + _512;
           uint16_t _514 = _conv1_stencil(4, 1, 0);
           uint16_t _515 = _514 * _488;
           uint16_t _516 = _513 + _515;
           uint16_t _517 = _conv1_stencil(0, 2, 0);
           uint16_t _518 = _517 * _492;
           uint16_t _519 = _516 + _518;
           uint16_t _520 = _conv1_stencil(1, 2, 0);
           uint16_t _521 = _520 * _508;
           uint16_t _522 = _519 + _521;
           uint16_t _523 = _conv1_stencil(2, 2, 0);
           uint16_t _524 = (uint16_t)(44);
           uint16_t _525 = _523 * _524;
           uint16_t _526 = _522 + _525;
           uint16_t _527 = _conv1_stencil(3, 2, 0);
           uint16_t _528 = _527 * _508;
           uint16_t _529 = _526 + _528;
           uint16_t _530 = _conv1_stencil(4, 2, 0);
           uint16_t _531 = _530 * _492;
           uint16_t _532 = _529 + _531;
           uint16_t _533 = _conv1_stencil(0, 3, 0);
           uint16_t _534 = _533 * _488;
           uint16_t _535 = _532 + _534;
           uint16_t _536 = _conv1_stencil(1, 3, 0);
           uint16_t _537 = _536 * _504;
           uint16_t _538 = _535 + _537;
           uint16_t _539 = _conv1_stencil(2, 3, 0);
           uint16_t _540 = _539 * _508;
           uint16_t _541 = _538 + _540;
           uint16_t _542 = _conv1_stencil(3, 3, 0);
           uint16_t _543 = _542 * _504;
           uint16_t _544 = _541 + _543;
           uint16_t _545 = _conv1_stencil(4, 3, 0);
           uint16_t _546 = _545 * _488;
           uint16_t _547 = _544 + _546;
           uint16_t _548 = _conv1_stencil(0, 4, 0);
           uint16_t _549 = _547 + _548;
           uint16_t _550 = _conv1_stencil(1, 4, 0);
           uint16_t _551 = _550 * _488;
           uint16_t _552 = _549 + _551;
           uint16_t _553 = _conv1_stencil(2, 4, 0);
           uint16_t _554 = _553 * _492;
           uint16_t _555 = _552 + _554;
           uint16_t _556 = _conv1_stencil(3, 4, 0);
           uint16_t _557 = _556 * _488;
           uint16_t _558 = _555 + _557;
           uint16_t _559 = _conv1_stencil(4, 4, 0);
           uint16_t _560 = _558 + _559;
           uint16_t _561 = _560 >> 8;
           uint8_t _562 = (uint8_t)(_561);
           _output[_485] = _562;
          } // realize _conv1_stencil
         } // for _output_s0_x_xi
        } // for _output_s0_y_yi
       } // realize _conv1_stencil_stream
      } // realize _buffered_stencil_stream
     } // for _output_s0_x_xo
     int32_t _563 = _output_extent_0 + 63;
     int32_t _564 = _563 >> 6;
     int32_t _565 = _564 - _357;
     for (int _output_s0_x_xo = _357; _output_s0_x_xo < _357 + _565; _output_s0_x_xo++)
     {
      {
       hls::stream<Stencil<uint8_t, 5, 5, 1> > _buffered_stencil_stream;
       {
        hls::stream<Stencil<uint16_t, 5, 5, 1> > _conv1_stencil_stream;
        // produce conv1.stencil.stream
        {
         hls::stream<Stencil<uint16_t, 1, 1, 1> > _conv1_stencil_update_stream;
         // produce buffered.stencil.stream
         {
          hls::stream<Stencil<uint8_t, 1, 1, 1> > _buffered_stencil_update_stream;
          // produce buffered.stencil_update.stream
          for (int _buffered_scan_update_y = 0; _buffered_scan_update_y < 0 + 72; _buffered_scan_update_y++)
          {
           for (int _buffered_scan_update_x = 0; _buffered_scan_update_x < 0 + 72; _buffered_scan_update_x++)
           {
            {
             Stencil<uint8_t, 1, 1, 1> _buffered_stencil_update;
             // produce buffered.stencil_update
             int32_t _566 = _output_min_0 + _output_extent_0;
             int32_t _567 = _566 + _buffered_scan_update_x;
             int32_t _568 = _567 - _41;
             int32_t _569 = _355 + _buffered_scan_update_y;
             int32_t _570 = _569 - _51;
             int32_t _571 = _225 - _41;
             int32_t _572 = _571 + 5;
             int32_t _573 = _570 * _572;
             int32_t _574 = _568 + _573;
             int32_t _575 = _output_s0_c - _output_min_2;
             int32_t _576 = _575 * _230;
             int32_t _577 = _574 + _576;
             int32_t _578 = _577 + -64;
             uint8_t _579 = _repeat_edge[_578];
             _buffered_stencil_update(0, 0, 0) = _579;
             // consume buffered.stencil_update
             _buffered_stencil_update_stream.write(_buffered_stencil_update);
            } // realize _buffered_stencil_update
           } // for _buffered_scan_update_x
          } // for _buffered_scan_update_y
          // consume buffered.stencil_update.stream
          linebuffer<72, 72, 1>(_buffered_stencil_update_stream, _buffered_stencil_stream);
         } // realize _buffered_stencil_update_stream
         // consume buffered.stencil.stream
         // produce conv1.stencil_update.stream
         for (int _conv1_scan_update_y = 0; _conv1_scan_update_y < 0 + 68; _conv1_scan_update_y++)
         {
          for (int _conv1_scan_update_x = 0; _conv1_scan_update_x < 0 + 68; _conv1_scan_update_x++)
          {
           {
            Stencil<uint8_t, 5, 5, 1> _buffered_stencil;
            // produce buffered.stencil
            _buffered_stencil = _buffered_stencil_stream.read();
            // consume buffered.stencil
            {
             Stencil<uint16_t, 1, 1, 1> _conv1_stencil_update;
             // produce conv1.stencil_update
             uint8_t _580 = _buffered_stencil(0, 0, 0);
             uint16_t _581 = (uint16_t)(_580);
             uint8_t _582 = _buffered_stencil(1, 0, 0);
             uint16_t _583 = (uint16_t)(_582);
             uint16_t _584 = (uint16_t)(3);
             uint16_t _585 = _583 * _584;
             uint16_t _586 = _581 + _585;
             uint8_t _587 = _buffered_stencil(2, 0, 0);
             uint16_t _588 = (uint16_t)(_587);
             uint16_t _589 = (uint16_t)(6);
             uint16_t _590 = _588 * _589;
             uint16_t _591 = _586 + _590;
             uint8_t _592 = _buffered_stencil(3, 0, 0);
             uint16_t _593 = (uint16_t)(_592);
             uint16_t _594 = _593 * _584;
             uint16_t _595 = _591 + _594;
             uint8_t _596 = _buffered_stencil(4, 0, 0);
             uint16_t _597 = (uint16_t)(_596);
             uint16_t _598 = _595 + _597;
             uint8_t _599 = _buffered_stencil(0, 1, 0);
             uint16_t _600 = (uint16_t)(_599);
             uint16_t _601 = _600 * _584;
             uint16_t _602 = _598 + _601;
             uint8_t _603 = _buffered_stencil(1, 1, 0);
             uint16_t _604 = (uint16_t)(_603);
             uint16_t _605 = (uint16_t)(15);
             uint16_t _606 = _604 * _605;
             uint16_t _607 = _602 + _606;
             uint8_t _608 = _buffered_stencil(2, 1, 0);
             uint16_t _609 = (uint16_t)(_608);
             uint16_t _610 = (uint16_t)(25);
             uint16_t _611 = _609 * _610;
             uint16_t _612 = _607 + _611;
             uint8_t _613 = _buffered_stencil(3, 1, 0);
             uint16_t _614 = (uint16_t)(_613);
             uint16_t _615 = _614 * _605;
             uint16_t _616 = _612 + _615;
             uint8_t _617 = _buffered_stencil(4, 1, 0);
             uint16_t _618 = (uint16_t)(_617);
             uint16_t _619 = _618 * _584;
             uint16_t _620 = _616 + _619;
             uint8_t _621 = _buffered_stencil(0, 2, 0);
             uint16_t _622 = (uint16_t)(_621);
             uint16_t _623 = _622 * _589;
             uint16_t _624 = _620 + _623;
             uint8_t _625 = _buffered_stencil(1, 2, 0);
             uint16_t _626 = (uint16_t)(_625);
             uint16_t _627 = _626 * _610;
             uint16_t _628 = _624 + _627;
             uint8_t _629 = _buffered_stencil(2, 2, 0);
             uint16_t _630 = (uint16_t)(_629);
             uint16_t _631 = (uint16_t)(44);
             uint16_t _632 = _630 * _631;
             uint16_t _633 = _628 + _632;
             uint8_t _634 = _buffered_stencil(3, 2, 0);
             uint16_t _635 = (uint16_t)(_634);
             uint16_t _636 = _635 * _610;
             uint16_t _637 = _633 + _636;
             uint8_t _638 = _buffered_stencil(4, 2, 0);
             uint16_t _639 = (uint16_t)(_638);
             uint16_t _640 = _639 * _589;
             uint16_t _641 = _637 + _640;
             uint8_t _642 = _buffered_stencil(0, 3, 0);
             uint16_t _643 = (uint16_t)(_642);
             uint16_t _644 = _643 * _584;
             uint16_t _645 = _641 + _644;
             uint8_t _646 = _buffered_stencil(1, 3, 0);
             uint16_t _647 = (uint16_t)(_646);
             uint16_t _648 = _647 * _605;
             uint16_t _649 = _645 + _648;
             uint8_t _650 = _buffered_stencil(2, 3, 0);
             uint16_t _651 = (uint16_t)(_650);
             uint16_t _652 = _651 * _610;
             uint16_t _653 = _649 + _652;
             uint8_t _654 = _buffered_stencil(3, 3, 0);
             uint16_t _655 = (uint16_t)(_654);
             uint16_t _656 = _655 * _605;
             uint16_t _657 = _653 + _656;
             uint8_t _658 = _buffered_stencil(4, 3, 0);
             uint16_t _659 = (uint16_t)(_658);
             uint16_t _660 = _659 * _584;
             uint16_t _661 = _657 + _660;
             uint8_t _662 = _buffered_stencil(0, 4, 0);
             uint16_t _663 = (uint16_t)(_662);
             uint16_t _664 = _661 + _663;
             uint8_t _665 = _buffered_stencil(1, 4, 0);
             uint16_t _666 = (uint16_t)(_665);
             uint16_t _667 = _666 * _584;
             uint16_t _668 = _664 + _667;
             uint8_t _669 = _buffered_stencil(2, 4, 0);
             uint16_t _670 = (uint16_t)(_669);
             uint16_t _671 = _670 * _589;
             uint16_t _672 = _668 + _671;
             uint8_t _673 = _buffered_stencil(3, 4, 0);
             uint16_t _674 = (uint16_t)(_673);
             uint16_t _675 = _674 * _584;
             uint16_t _676 = _672 + _675;
             uint8_t _677 = _buffered_stencil(4, 4, 0);
             uint16_t _678 = (uint16_t)(_677);
             uint16_t _679 = _676 + _678;
             uint16_t _680 = _679 >> 8;
             _conv1_stencil_update(0, 0, 0) = _680;
             // consume conv1.stencil_update
             _conv1_stencil_update_stream.write(_conv1_stencil_update);
            } // realize _conv1_stencil_update
           } // realize _buffered_stencil
          } // for _conv1_scan_update_x
         } // for _conv1_scan_update_y
         // consume conv1.stencil_update.stream
         linebuffer<68, 68, 1>(_conv1_stencil_update_stream, _conv1_stencil_stream);
        } // realize _conv1_stencil_update_stream
        // consume conv1.stencil.stream
        for (int _output_s0_y_yi = 0; _output_s0_y_yi < 0 + 64; _output_s0_y_yi++)
        {
         for (int _output_s0_x_xi = 0; _output_s0_x_xi < 0 + 64; _output_s0_x_xi++)
         {
          {
           Stencil<uint16_t, 5, 5, 1> _conv1_stencil;
           // produce conv1.stencil
           _conv1_stencil = _conv1_stencil_stream.read();
           // consume conv1.stencil
           int32_t _681 = _output_min_0 + _output_extent_0;
           int32_t _682 = _681 + _output_s0_x_xi;
           int32_t _683 = _355 + _output_s0_y_yi;
           int32_t _684 = _683 * _output_stride_1;
           int32_t _685 = _682 + _684;
           int32_t _686 = _output_s0_c * _output_stride_2;
           int32_t _687 = _685 + _686;
           int32_t _688 = _output_min_1 * _output_stride_1;
           int32_t _689 = _output_min_0 + _688;
           int32_t _690 = _output_min_2 * _output_stride_2;
           int32_t _691 = _689 + _690;
           int32_t _692 = _687 - _691;
           int32_t _693 = _692 + -64;
           uint16_t _694 = _conv1_stencil(0, 0, 0);
           uint16_t _695 = _conv1_stencil(1, 0, 0);
           uint16_t _696 = (uint16_t)(3);
           uint16_t _697 = _695 * _696;
           uint16_t _698 = _694 + _697;
           uint16_t _699 = _conv1_stencil(2, 0, 0);
           uint16_t _700 = (uint16_t)(6);
           uint16_t _701 = _699 * _700;
           uint16_t _702 = _698 + _701;
           uint16_t _703 = _conv1_stencil(3, 0, 0);
           uint16_t _704 = _703 * _696;
           uint16_t _705 = _702 + _704;
           uint16_t _706 = _conv1_stencil(4, 0, 0);
           uint16_t _707 = _705 + _706;
           uint16_t _708 = _conv1_stencil(0, 1, 0);
           uint16_t _709 = _708 * _696;
           uint16_t _710 = _707 + _709;
           uint16_t _711 = _conv1_stencil(1, 1, 0);
           uint16_t _712 = (uint16_t)(15);
           uint16_t _713 = _711 * _712;
           uint16_t _714 = _710 + _713;
           uint16_t _715 = _conv1_stencil(2, 1, 0);
           uint16_t _716 = (uint16_t)(25);
           uint16_t _717 = _715 * _716;
           uint16_t _718 = _714 + _717;
           uint16_t _719 = _conv1_stencil(3, 1, 0);
           uint16_t _720 = _719 * _712;
           uint16_t _721 = _718 + _720;
           uint16_t _722 = _conv1_stencil(4, 1, 0);
           uint16_t _723 = _722 * _696;
           uint16_t _724 = _721 + _723;
           uint16_t _725 = _conv1_stencil(0, 2, 0);
           uint16_t _726 = _725 * _700;
           uint16_t _727 = _724 + _726;
           uint16_t _728 = _conv1_stencil(1, 2, 0);
           uint16_t _729 = _728 * _716;
           uint16_t _730 = _727 + _729;
           uint16_t _731 = _conv1_stencil(2, 2, 0);
           uint16_t _732 = (uint16_t)(44);
           uint16_t _733 = _731 * _732;
           uint16_t _734 = _730 + _733;
           uint16_t _735 = _conv1_stencil(3, 2, 0);
           uint16_t _736 = _735 * _716;
           uint16_t _737 = _734 + _736;
           uint16_t _738 = _conv1_stencil(4, 2, 0);
           uint16_t _739 = _738 * _700;
           uint16_t _740 = _737 + _739;
           uint16_t _741 = _conv1_stencil(0, 3, 0);
           uint16_t _742 = _741 * _696;
           uint16_t _743 = _740 + _742;
           uint16_t _744 = _conv1_stencil(1, 3, 0);
           uint16_t _745 = _744 * _712;
           uint16_t _746 = _743 + _745;
           uint16_t _747 = _conv1_stencil(2, 3, 0);
           uint16_t _748 = _747 * _716;
           uint16_t _749 = _746 + _748;
           uint16_t _750 = _conv1_stencil(3, 3, 0);
           uint16_t _751 = _750 * _712;
           uint16_t _752 = _749 + _751;
           uint16_t _753 = _conv1_stencil(4, 3, 0);
           uint16_t _754 = _753 * _696;
           uint16_t _755 = _752 + _754;
           uint16_t _756 = _conv1_stencil(0, 4, 0);
           uint16_t _757 = _755 + _756;
           uint16_t _758 = _conv1_stencil(1, 4, 0);
           uint16_t _759 = _758 * _696;
           uint16_t _760 = _757 + _759;
           uint16_t _761 = _conv1_stencil(2, 4, 0);
           uint16_t _762 = _761 * _700;
           uint16_t _763 = _760 + _762;
           uint16_t _764 = _conv1_stencil(3, 4, 0);
           uint16_t _765 = _764 * _696;
           uint16_t _766 = _763 + _765;
           uint16_t _767 = _conv1_stencil(4, 4, 0);
           uint16_t _768 = _766 + _767;
           uint16_t _769 = _768 >> 8;
           uint8_t _770 = (uint8_t)(_769);
           _output[_693] = _770;
          } // realize _conv1_stencil
         } // for _output_s0_x_xi
        } // for _output_s0_y_yi
       } // realize _conv1_stencil_stream
      } // realize _buffered_stencil_stream
     } // for _output_s0_x_xo
    } // for _output_s0_y_yo
   } // for _output_s0_c
   halide_free(NULL, _repeat_edge);
   // consume output
  } // alloc _repeat_edge
 } // if _64
 return 0;
}


int pipeline_hls(buffer_t *_input_buffer, buffer_t *_output_buffer) HALIDE_FUNCTION_ATTRS {
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
 uint8_t *_output = (uint8_t *)(_output_buffer->host);
 (void)_output;
 const bool _output_host_and_dev_are_null = (_output_buffer->host == NULL) && (_output_buffer->dev == 0);
 (void)_output_host_and_dev_are_null;
 const int32_t _output_min_0 = _output_buffer->min[0];
 (void)_output_min_0;
 const int32_t _output_min_1 = _output_buffer->min[1];
 (void)_output_min_1;
 const int32_t _output_min_2 = _output_buffer->min[2];
 (void)_output_min_2;
 const int32_t _output_min_3 = _output_buffer->min[3];
 (void)_output_min_3;
 const int32_t _output_extent_0 = _output_buffer->extent[0];
 (void)_output_extent_0;
 const int32_t _output_extent_1 = _output_buffer->extent[1];
 (void)_output_extent_1;
 const int32_t _output_extent_2 = _output_buffer->extent[2];
 (void)_output_extent_2;
 const int32_t _output_extent_3 = _output_buffer->extent[3];
 (void)_output_extent_3;
 const int32_t _output_stride_0 = _output_buffer->stride[0];
 (void)_output_stride_0;
 const int32_t _output_stride_1 = _output_buffer->stride[1];
 (void)_output_stride_1;
 const int32_t _output_stride_2 = _output_buffer->stride[2];
 (void)_output_stride_2;
 const int32_t _output_stride_3 = _output_buffer->stride[3];
 (void)_output_stride_3;
 const int32_t _output_elem_size = _output_buffer->elem_size;
 (void)_output_elem_size;
 int32_t _771 = __pipeline_hls(_input_buffer, _output_buffer);
 bool _772 = _771 == 0;
 if (!_772)  {
  return _771;
 }
 return 0;
}
#ifdef __cplusplus
}  // extern "C"
#endif
