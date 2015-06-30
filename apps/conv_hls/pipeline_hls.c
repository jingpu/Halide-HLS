#include <math.h>
#include <float.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include <hls_stream.h>
#include "pipeline_hw.h"

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
uint64_t halide_profiling_timer(void *ctx);
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

static int __pipeline_c(buffer_t *_input_buffer, buffer_t *_output_buffer) HALIDE_FUNCTION_ATTRS {
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
 int32_t _2 = _1 + 1;
 int32_t _3 = _input_min_0 + _input_extent_0;
 int32_t _4 = _3 + -1;
 int32_t _5 = min(_2, _4);
 int32_t _6 = max(_5, _input_min_0);
 int32_t _7 = _output_min_0 + -2;
 int32_t _8 = min(_7, _4);
 int32_t _9 = max(_8, _input_min_0);
 int32_t _10 = _6 - _9;
 int32_t _11 = _output_min_1 + _output_extent_1;
 int32_t _12 = _11 + 1;
 int32_t _13 = _input_min_1 + _input_extent_1;
 int32_t _14 = _13 + -1;
 int32_t _15 = min(_12, _14);
 int32_t _16 = max(_15, _input_min_1);
 int32_t _17 = _output_min_1 + -2;
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
 if (_input_host_and_dev_are_null)
 {
  int32_t _43 = _10 + 1;
  int32_t _44 = _20 + 1;
  int32_t _45 = _29 + 1;
  bool _46 = halide_rewrite_buffer(_input_buffer, 1, _9, _43, 1, _19, _44, _43, _28, _45, _32, 0, 0, 0);
  (void)_46;
 } // if _input_host_and_dev_are_null
 if (_output_host_and_dev_are_null)
 {
  int32_t _47 = _42 + 1;
  int32_t _48 = _47 * _output_extent_1;
  bool _49 = halide_rewrite_buffer(_output_buffer, 1, _41, _47, 1, _output_min_1, _output_extent_1, _47, _output_min_2, _output_extent_2, _48, 0, 0, 0);
  (void)_49;
 } // if _output_host_and_dev_are_null
 bool _50 = _input_host_and_dev_are_null || _output_host_and_dev_are_null;
 bool _51 = !(_50);
 if (_51)
 {
  bool _52 = _input_elem_size == 1;
  if (!_52)   {
   int32_t _53 = halide_error_bad_elem_size(NULL, "Input buffer input", "uint8", _input_elem_size, 1);
   return _53;
  }
  bool _54 = _output_elem_size == 1;
  if (!_54)   {
   int32_t _55 = halide_error_bad_elem_size(NULL, "Output buffer output", "uint8", _output_elem_size, 1);
   return _55;
  }
  bool _56 = _input_min_0 <= _9;
  int32_t _57 = _9 + _10;
  int32_t _58 = _57 - _input_extent_0;
  int32_t _59 = _58 + 1;
  bool _60 = _59 <= _input_min_0;
  bool _61 = _56 && _60;
  if (!_61)   {
   int32_t _62 = _9 + _10;
   int32_t _63 = _input_min_0 + _input_extent_0;
   int32_t _64 = _63 + -1;
   int32_t _65 = halide_error_access_out_of_bounds(NULL, "Input buffer input", 0, _9, _62, _input_min_0, _64);
   return _65;
  }
  bool _66 = _input_min_1 <= _19;
  int32_t _67 = _19 + _20;
  int32_t _68 = _67 - _input_extent_1;
  int32_t _69 = _68 + 1;
  bool _70 = _69 <= _input_min_1;
  bool _71 = _66 && _70;
  if (!_71)   {
   int32_t _72 = _19 + _20;
   int32_t _73 = _input_min_1 + _input_extent_1;
   int32_t _74 = _73 + -1;
   int32_t _75 = halide_error_access_out_of_bounds(NULL, "Input buffer input", 1, _19, _72, _input_min_1, _74);
   return _75;
  }
  bool _76 = _input_min_2 <= _28;
  int32_t _77 = _28 + _29;
  int32_t _78 = _77 - _input_extent_2;
  int32_t _79 = _78 + 1;
  bool _80 = _79 <= _input_min_2;
  bool _81 = _76 && _80;
  if (!_81)   {
   int32_t _82 = _28 + _29;
   int32_t _83 = _input_min_2 + _input_extent_2;
   int32_t _84 = _83 + -1;
   int32_t _85 = halide_error_access_out_of_bounds(NULL, "Input buffer input", 2, _28, _82, _input_min_2, _84);
   return _85;
  }
  bool _86 = _output_min_0 <= _41;
  int32_t _87 = _41 + _42;
  int32_t _88 = _87 - _output_extent_0;
  int32_t _89 = _88 + 1;
  bool _90 = _89 <= _output_min_0;
  bool _91 = _86 && _90;
  if (!_91)   {
   int32_t _92 = _41 + _42;
   int32_t _93 = _output_min_0 + _output_extent_0;
   int32_t _94 = _93 + -1;
   int32_t _95 = halide_error_access_out_of_bounds(NULL, "Output buffer output", 0, _41, _92, _output_min_0, _94);
   return _95;
  }
  bool _96 = _input_stride_0 == 1;
  if (!_96)   {
   int32_t _97 = halide_error_constraint_violated(NULL, "input.stride.0", _input_stride_0, "1", 1);
   return _97;
  }
  bool _98 = _output_stride_0 == 1;
  if (!_98)   {
   int32_t _99 = halide_error_constraint_violated(NULL, "output.stride.0", _output_stride_0, "1", 1);
   return _99;
  }
  int64_t _100 = (int64_t)(_input_extent_1);
  int64_t _101 = (int64_t)(_input_extent_0);
  int64_t _102 = _100 * _101;
  int64_t _103 = (int64_t)(_output_extent_1);
  int64_t _104 = (int64_t)(_output_extent_0);
  int64_t _105 = _103 * _104;
  int64_t _106 = (int64_t)(2147483647);
  bool _107 = _101 <= _106;
  if (!_107)   {
   int64_t _108 = (int64_t)(_input_extent_0);
   int64_t _109 = (int64_t)(2147483647);
   int32_t _110 = halide_error_buffer_allocation_too_large(NULL, "input", _108, _109);
   return _110;
  }
  int64_t _111 = (int64_t)(_input_extent_1);
  int64_t _112 = (int64_t)(_input_stride_1);
  int64_t _113 = _111 * _112;
  int64_t _114 = (int64_t)(2147483647);
  bool _115 = _113 <= _114;
  if (!_115)   {
   int64_t _116 = (int64_t)(_input_extent_1);
   int64_t _117 = (int64_t)(_input_stride_1);
   int64_t _118 = _116 * _117;
   int64_t _119 = (int64_t)(2147483647);
   int32_t _120 = halide_error_buffer_allocation_too_large(NULL, "input", _118, _119);
   return _120;
  }
  int64_t _121 = (int64_t)(2147483647);
  bool _122 = _102 <= _121;
  if (!_122)   {
   int64_t _123 = (int64_t)(2147483647);
   int32_t _124 = halide_error_buffer_extents_too_large(NULL, "input", _102, _123);
   return _124;
  }
  int64_t _125 = (int64_t)(_input_extent_2);
  int64_t _126 = (int64_t)(_input_stride_2);
  int64_t _127 = _125 * _126;
  int64_t _128 = (int64_t)(2147483647);
  bool _129 = _127 <= _128;
  if (!_129)   {
   int64_t _130 = (int64_t)(_input_extent_2);
   int64_t _131 = (int64_t)(_input_stride_2);
   int64_t _132 = _130 * _131;
   int64_t _133 = (int64_t)(2147483647);
   int32_t _134 = halide_error_buffer_allocation_too_large(NULL, "input", _132, _133);
   return _134;
  }
  int64_t _135 = (int64_t)(_input_extent_2);
  int64_t _136 = _135 * _102;
  int64_t _137 = (int64_t)(2147483647);
  bool _138 = _136 <= _137;
  if (!_138)   {
   int64_t _139 = (int64_t)(_input_extent_2);
   int64_t _140 = _139 * _102;
   int64_t _141 = (int64_t)(2147483647);
   int32_t _142 = halide_error_buffer_extents_too_large(NULL, "input", _140, _141);
   return _142;
  }
  int64_t _143 = (int64_t)(_output_extent_0);
  int64_t _144 = (int64_t)(2147483647);
  bool _145 = _143 <= _144;
  if (!_145)   {
   int64_t _146 = (int64_t)(_output_extent_0);
   int64_t _147 = (int64_t)(2147483647);
   int32_t _148 = halide_error_buffer_allocation_too_large(NULL, "output", _146, _147);
   return _148;
  }
  int64_t _149 = (int64_t)(_output_extent_1);
  int64_t _150 = (int64_t)(_output_stride_1);
  int64_t _151 = _149 * _150;
  int64_t _152 = (int64_t)(2147483647);
  bool _153 = _151 <= _152;
  if (!_153)   {
   int64_t _154 = (int64_t)(_output_extent_1);
   int64_t _155 = (int64_t)(_output_stride_1);
   int64_t _156 = _154 * _155;
   int64_t _157 = (int64_t)(2147483647);
   int32_t _158 = halide_error_buffer_allocation_too_large(NULL, "output", _156, _157);
   return _158;
  }
  int64_t _159 = (int64_t)(2147483647);
  bool _160 = _105 <= _159;
  if (!_160)   {
   int64_t _161 = (int64_t)(2147483647);
   int32_t _162 = halide_error_buffer_extents_too_large(NULL, "output", _105, _161);
   return _162;
  }
  int64_t _163 = (int64_t)(_output_extent_2);
  int64_t _164 = (int64_t)(_output_stride_2);
  int64_t _165 = _163 * _164;
  int64_t _166 = (int64_t)(2147483647);
  bool _167 = _165 <= _166;
  if (!_167)   {
   int64_t _168 = (int64_t)(_output_extent_2);
   int64_t _169 = (int64_t)(_output_stride_2);
   int64_t _170 = _168 * _169;
   int64_t _171 = (int64_t)(2147483647);
   int32_t _172 = halide_error_buffer_allocation_too_large(NULL, "output", _170, _171);
   return _172;
  }
  int64_t _173 = (int64_t)(_output_extent_2);
  int64_t _174 = _173 * _105;
  int64_t _175 = (int64_t)(2147483647);
  bool _176 = _174 <= _175;
  if (!_176)   {
   int64_t _177 = (int64_t)(_output_extent_2);
   int64_t _178 = _177 * _105;
   int64_t _179 = (int64_t)(2147483647);
   int32_t _180 = halide_error_buffer_extents_too_large(NULL, "output", _178, _179);
   return _180;
  }
  int32_t _181 = _output_min_0 + _output_extent_0;
  int32_t _182 = _181 + 1;
  int32_t _183 = _output_extent_0 + -1;
  int32_t _184 = _183 >> 6;
  int32_t _185 = _184 * 64;
  int32_t _186 = _185 + _output_min_0;
  int32_t _187 = _186 + 63;
  int32_t _188 = _181 + -1;
  int32_t _189 = min(_187, _188);
  int32_t _190 = _189 + 2;
  int32_t _191 = max(_182, _190);
  int32_t _192 = _191 - _41;
  int32_t _193 = _192 + 3;
  int32_t _194 = _output_extent_1 + 4;
  int32_t _195 = _193 * _194;
  {
   int32_t _196 = _191 - _41;
   int32_t _197 = _196 + 3;
   int64_t _198 = _197;
   int32_t _199 = _output_extent_1 + 4;
   int64_t _200 = _198 * _199;
   int64_t _201 = (_200 > ((int64_t(1) << 31) - 1)) ? _200 : (_200 * _output_extent_2);
   if ((_201 > ((int64_t(1) << 31) - 1)) || ((_201 * sizeof(uint8_t)) > ((int64_t(1) << 31) - 1)))
   {
    halide_error(NULL, "32-bit signed overflow computing size of allocation repeat_edge\n");
    return -1;
   } // overflow test repeat_edge
   int64_t _202 = _201;
   uint8_t *_repeat_edge = (uint8_t *)halide_malloc(NULL, sizeof(uint8_t)*_202);
   // produce repeat_edge
   for (int _repeat_edge_s0__2 = _output_min_2; _repeat_edge_s0__2 < _output_min_2 + _output_extent_2; _repeat_edge_s0__2++)
   {
    int32_t _203 = _output_min_1 + -2;
    int32_t _204 = _output_extent_1 + 4;
    for (int _repeat_edge_s0__1 = _203; _repeat_edge_s0__1 < _203 + _204; _repeat_edge_s0__1++)
    {
     int32_t _205 = _output_min_0 + _output_extent_0;
     int32_t _206 = _205 + 2;
     int32_t _207 = min(_input_min_0, _206);
     int32_t _208 = _output_min_0 + -2;
     int32_t _209 = max(_207, _208);
     int32_t _210 = _input_min_0 + _input_extent_0;
     int32_t _211 = min(_210, _206);
     int32_t _212 = max(_211, _209);
     int32_t _213 = _209 - _output_min_0;
     int32_t _214 = _213 + 2;
     for (int _repeat_edge_s0__0 = _208; _repeat_edge_s0__0 < _208 + _214; _repeat_edge_s0__0++)
     {
      int32_t _215 = _repeat_edge_s0__0 - _41;
      int32_t _216 = _repeat_edge_s0__1 - _output_min_1;
      int32_t _217 = _216 + 2;
      int32_t _218 = _191 - _41;
      int32_t _219 = _218 + 3;
      int32_t _220 = _217 * _219;
      int32_t _221 = _215 + _220;
      int32_t _222 = _repeat_edge_s0__2 - _output_min_2;
      int32_t _223 = _222 * _195;
      int32_t _224 = _221 + _223;
      int32_t _225 = _224 + 2;
      int32_t _226 = _input_min_0 + _input_extent_0;
      int32_t _227 = _226 + -1;
      int32_t _228 = min(_repeat_edge_s0__0, _227);
      int32_t _229 = max(_228, _input_min_0);
      int32_t _230 = _input_min_1 + _input_extent_1;
      int32_t _231 = _230 + -1;
      int32_t _232 = min(_repeat_edge_s0__1, _231);
      int32_t _233 = max(_232, _input_min_1);
      int32_t _234 = _233 * _input_stride_1;
      int32_t _235 = _229 + _234;
      int32_t _236 = _input_min_2 + _input_extent_2;
      int32_t _237 = _236 + -1;
      int32_t _238 = min(_repeat_edge_s0__2, _237);
      int32_t _239 = max(_238, _input_min_2);
      int32_t _240 = _239 * _input_stride_2;
      int32_t _241 = _235 + _240;
      int32_t _242 = _input_min_1 * _input_stride_1;
      int32_t _243 = _input_min_0 + _242;
      int32_t _244 = _input_min_2 * _input_stride_2;
      int32_t _245 = _243 + _244;
      int32_t _246 = _241 - _245;
      uint8_t _247 = _input[_246];
      _repeat_edge[_225] = _247;
     } // for _repeat_edge_s0__0
     int32_t _248 = _212 - _209;
     for (int _repeat_edge_s0__0 = _209; _repeat_edge_s0__0 < _209 + _248; _repeat_edge_s0__0++)
     {
      int32_t _249 = _repeat_edge_s0__0 - _41;
      int32_t _250 = _repeat_edge_s0__1 - _output_min_1;
      int32_t _251 = _250 + 2;
      int32_t _252 = _191 - _41;
      int32_t _253 = _252 + 3;
      int32_t _254 = _251 * _253;
      int32_t _255 = _249 + _254;
      int32_t _256 = _repeat_edge_s0__2 - _output_min_2;
      int32_t _257 = _256 * _195;
      int32_t _258 = _255 + _257;
      int32_t _259 = _258 + 2;
      int32_t _260 = _input_min_1 + _input_extent_1;
      int32_t _261 = _260 + -1;
      int32_t _262 = min(_repeat_edge_s0__1, _261);
      int32_t _263 = max(_262, _input_min_1);
      int32_t _264 = _263 * _input_stride_1;
      int32_t _265 = _repeat_edge_s0__0 + _264;
      int32_t _266 = _input_min_2 + _input_extent_2;
      int32_t _267 = _266 + -1;
      int32_t _268 = min(_repeat_edge_s0__2, _267);
      int32_t _269 = max(_268, _input_min_2);
      int32_t _270 = _269 * _input_stride_2;
      int32_t _271 = _265 + _270;
      int32_t _272 = _input_min_1 * _input_stride_1;
      int32_t _273 = _input_min_0 + _272;
      int32_t _274 = _input_min_2 * _input_stride_2;
      int32_t _275 = _273 + _274;
      int32_t _276 = _271 - _275;
      uint8_t _277 = _input[_276];
      _repeat_edge[_259] = _277;
     } // for _repeat_edge_s0__0
     int32_t _278 = _output_min_0 + _output_extent_0;
     int32_t _279 = _278 - _212;
     int32_t _280 = _279 + 2;
     for (int _repeat_edge_s0__0 = _212; _repeat_edge_s0__0 < _212 + _280; _repeat_edge_s0__0++)
     {
      int32_t _281 = _repeat_edge_s0__0 - _41;
      int32_t _282 = _repeat_edge_s0__1 - _output_min_1;
      int32_t _283 = _282 + 2;
      int32_t _284 = _191 - _41;
      int32_t _285 = _284 + 3;
      int32_t _286 = _283 * _285;
      int32_t _287 = _281 + _286;
      int32_t _288 = _repeat_edge_s0__2 - _output_min_2;
      int32_t _289 = _288 * _195;
      int32_t _290 = _287 + _289;
      int32_t _291 = _290 + 2;
      int32_t _292 = _input_min_0 + _input_extent_0;
      int32_t _293 = _292 + -1;
      int32_t _294 = max(_293, _input_min_0);
      int32_t _295 = _input_min_1 + _input_extent_1;
      int32_t _296 = _295 + -1;
      int32_t _297 = min(_repeat_edge_s0__1, _296);
      int32_t _298 = max(_297, _input_min_1);
      int32_t _299 = _298 * _input_stride_1;
      int32_t _300 = _294 + _299;
      int32_t _301 = _input_min_2 + _input_extent_2;
      int32_t _302 = _301 + -1;
      int32_t _303 = min(_repeat_edge_s0__2, _302);
      int32_t _304 = max(_303, _input_min_2);
      int32_t _305 = _304 * _input_stride_2;
      int32_t _306 = _300 + _305;
      int32_t _307 = _input_min_1 * _input_stride_1;
      int32_t _308 = _input_min_0 + _307;
      int32_t _309 = _input_min_2 * _input_stride_2;
      int32_t _310 = _308 + _309;
      int32_t _311 = _306 - _310;
      uint8_t _312 = _input[_311];
      _repeat_edge[_291] = _312;
     } // for _repeat_edge_s0__0
    } // for _repeat_edge_s0__1
   } // for _repeat_edge_s0__2
   // consume repeat_edge
   // produce output
   for (int _output_s0_c = _output_min_2; _output_s0_c < _output_min_2 + _output_extent_2; _output_s0_c++)
   {
    int32_t _313 = _output_extent_0 + 63;
    int32_t _314 = _313 >> 6;
    for (int _output_s0_x_xo = 0; _output_s0_x_xo < 0 + _314; _output_s0_x_xo++)
    {
     int32_t _315 = _output_s0_x_xo * 64;
     int32_t _316 = _315 + _output_min_0;
     int32_t _317 = _output_min_0 + _output_extent_0;
     int32_t _318 = _317 + -64;
     int32_t _319 = min(_316, _318); //0
     {
      hls::stream<uint8_t> to_LB;
      hls::stream<uint8_t> LB_to_ST;
      hls::stream<uint8_t> to_output;
      //int32_t _320 = min(_output_extent_1, 1); // 1
      //int32_t _321 = _320 + _output_min_1; // 1
      int32_t _322 = 0; //max(_321, _output_min_1);  // 2
      //int32_t _323 = _322 - _output_min_1;  // 1
      //for (int _output_s0_y = _output_min_1; _output_s0_y < _output_min_1 + _323; _output_s0_y++))
      // produce buffered
      //int32_t _324 = _output_s0_y + -2;  // -2
      //for (int _buffered_s0_y = -2; _buffered_s0_y < -2 + 5; _buffered_s0_y++)

      int32_t _494 = _output_extent_1 + _output_min_1; // y max
      int32_t _495 = _494 - _322;

      for (int _output_s0_y = -4; _output_s0_y < _322 + _495; _output_s0_y++){
	  // produce buffered
	  int32_t _496 = _319 + -2;
	  for (int _buffered_s0_x = _496; _buffered_s0_x < _496 + 68; _buffered_s0_x++){
	      int32_t _503 = _buffered_s0_x - _41;
	      int32_t _504 = _output_s0_y - _output_min_1;
	      int32_t _505 = _504 + 4;
	      int32_t _506 = _191 - _41;
	      int32_t _507 = _506 + 3;
	      int32_t _508 = _505 * _507;
	      int32_t _509 = _503 + _508;
	      int32_t _510 = _output_s0_c - _output_min_2;
	      int32_t _511 = _510 * _195;
	      int32_t _512 = _509 + _511;
	      int32_t _513 = _512 + 2;
	      //uint8_t _514 = _repeat_edge[_513];
	      to_LB.write(_repeat_edge[_513]);
	  } // for _buffered_s0_x
      }

      //
      // interface to LB
      // 

      bool use_hw = true;
      
      if (use_hw) {
	  convolve55_stream(to_LB, to_output);
      } else {
      uint8_t _buffered[544]; // line buffer 64x8
      for(int _buffered_s0_y = -2; _buffered_s0_y < -2 + 4; _buffered_s0_y++){
	  int32_t _325 = _319 + -2; // -2
	  for (int _buffered_s0_x = _325; _buffered_s0_x < _325 + 68; _buffered_s0_x++){
	      int32_t _326 = _buffered_s0_x - _319;
	      int32_t _327 = _buffered_s0_y & 7;
	      int32_t _328 = _327 * 68;
	      int32_t _329 = _326 + _328;
	      int32_t _330 = _329 + 2; // index in _bufferred[]
	      _buffered[_330] = to_LB.read();
	  } // for _buffered_s0_x
      } // for _buffered_s0_y

      for (int _output_s0_y = _322; _output_s0_y < _322 + _495; _output_s0_y++){
	  // produce buffered
	  int32_t _496 = _319 + -2;
	  for (int _buffered_s0_x = _496; _buffered_s0_x < _496 + 68; _buffered_s0_x++){
	      int32_t _497 = _buffered_s0_x - _319;
	      int32_t _498 = _output_s0_y + 2;  //  y+2 line
	      int32_t _499 = _498 & 7;
	      int32_t _500 = _499 * 68;
	      int32_t _501 = _497 + _500;
	      int32_t _502 = _501 + 2; // index in _bufferred[]
	      _buffered[_502] = to_LB.read();
	  } // for _buffered_s0_x
       // consume buffered
       uint8_t stencil[5][5]; // stencil
       uint16_t kernel[5][5] = {
	   {1,     3,     6,     3,     1},
	   {3,    15,    25,    15,     3},
	   {6,    25,    44,    25,     6},
	   {3,    15,    25,    15,     3},
	   {1,     3,     6,     3,     1}
       };

       for (int _output_s0_x_xi = 0; _output_s0_x_xi < 0 + 64; _output_s0_x_xi++) {
	   for(int r = 0; r < 5; r++)
	       for(int c = 0; c < 5; c++) {
		   int idx = ((_output_s0_y + -2 + r) & 7) * 68
		       + _output_s0_x_xi + c;
		   stencil[r][c] = _buffered[idx];
	       }

	   uint16_t acc = 0;
	   for(int r = 0; r < 5; r++)
	       for(int c = 0; c < 5; c++)
		   acc += stencil[r][c] * kernel[r][c];

	   acc >>= 8;
	   to_output.write(acc);
       } // for _output_s0_x_xi
      } // for _output_s0_y
      }

      //
      // interface to output buffer
      //

      for (int _output_s0_y = _322; _output_s0_y < _322 + _495; _output_s0_y++) {
	  for (int _output_s0_x_xi = 0; _output_s0_x_xi < 0 + 64; _output_s0_x_xi++) {
	      int32_t _515 = _319 + _output_s0_x_xi;
	      int32_t _516 = _output_s0_y * _output_stride_1;
	      int32_t _517 = _515 + _516;
	      int32_t _518 = _output_s0_c * _output_stride_2;
	      int32_t _519 = _517 + _518;
	      int32_t _520 = _output_min_1 * _output_stride_1;
	      int32_t _521 = _output_min_0 + _520;
	      int32_t _522 = _output_min_2 * _output_stride_2;
	      int32_t _523 = _521 + _522;
	      int32_t _524 = _519 - _523;  // output index
	      _output[_524] = to_output.read();
	  } // for _output_s0_x_xi
      } // for _output_s0_y

     } // alloc _buffered
    } // for _output_s0_x_xo
   } // for _output_s0_c
   halide_free(NULL, _repeat_edge);
   // consume output
  } // alloc _repeat_edge
 } // if _51
 return 0;
}


int pipeline_c(buffer_t *_input_buffer, buffer_t *_output_buffer) HALIDE_FUNCTION_ATTRS {
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
 int32_t _666 = __pipeline_c(_input_buffer, _output_buffer);
 bool _667 = _666 == 0;
 if (!_667)  {
  return _666;
 }
 return 0;
}
#ifdef __cplusplus
}  // extern "C"
#endif
