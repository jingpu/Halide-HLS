#include <iostream>
#include <math.h>
#include <float.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <hls_stream.h>
#include "stencil.h"
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

static int __pipeline_hls(buffer_t *_input_buffer, buffer_t *_weight_buffer, const uint8_t _p2___bias, buffer_t *_f3_buffer) HALIDE_FUNCTION_ATTRS {
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
 uint8_t *_f3 = (uint8_t *)(_f3_buffer->host);
 (void)_f3;
 const bool _f3_host_and_dev_are_null = (_f3_buffer->host == NULL) && (_f3_buffer->dev == 0);
 (void)_f3_host_and_dev_are_null;
 const int32_t _f3_min_0 = _f3_buffer->min[0];
 (void)_f3_min_0;
 const int32_t _f3_min_1 = _f3_buffer->min[1];
 (void)_f3_min_1;
 const int32_t _f3_min_2 = _f3_buffer->min[2];
 (void)_f3_min_2;
 const int32_t _f3_min_3 = _f3_buffer->min[3];
 (void)_f3_min_3;
 const int32_t _f3_extent_0 = _f3_buffer->extent[0];
 (void)_f3_extent_0;
 const int32_t _f3_extent_1 = _f3_buffer->extent[1];
 (void)_f3_extent_1;
 const int32_t _f3_extent_2 = _f3_buffer->extent[2];
 (void)_f3_extent_2;
 const int32_t _f3_extent_3 = _f3_buffer->extent[3];
 (void)_f3_extent_3;
 const int32_t _f3_stride_0 = _f3_buffer->stride[0];
 (void)_f3_stride_0;
 const int32_t _f3_stride_1 = _f3_buffer->stride[1];
 (void)_f3_stride_1;
 const int32_t _f3_stride_2 = _f3_buffer->stride[2];
 (void)_f3_stride_2;
 const int32_t _f3_stride_3 = _f3_buffer->stride[3];
 (void)_f3_stride_3;
 const int32_t _f3_elem_size = _f3_buffer->elem_size;
 (void)_f3_elem_size;
 int32_t _561 = _f3_extent_0 + -1;
 int32_t _562 = _561 >> 8;
 int32_t _563 = _562 * 256;
 int32_t _564 = _563 + _f3_min_0;
 int32_t _565 = _564 + 255;
 int32_t _566 = _f3_min_0 + _f3_extent_0;
 int32_t _567 = _566 + -1;
 int32_t _568 = min(_565, _567);
 int32_t _569 = _566 + -256;
 int32_t _570 = min(_f3_min_0, _569);
 int32_t _571 = _568 - _570;
 int32_t _572 = _f3_extent_1 + -1;
 int32_t _573 = _572 >> 8;
 int32_t _574 = _573 * 256;
 int32_t _575 = _574 + _f3_min_1;
 int32_t _576 = _575 + 255;
 int32_t _577 = _f3_min_1 + _f3_extent_1;
 int32_t _578 = _577 + -1;
 int32_t _579 = min(_576, _578);
 int32_t _580 = _577 + -256;
 int32_t _581 = min(_f3_min_1, _580);
 int32_t _582 = _579 - _581;
 int32_t _583 = _571 + 1;
 int32_t _584 = _582 + 1;
 int32_t _585 = _583 * _584;
 int32_t _586 = _566 + 3;
 int32_t _587 = _input_min_0 + _input_extent_0;
 int32_t _588 = _587 + -1;
 int32_t _589 = min(_586, _588);
 int32_t _590 = max(_589, _input_min_0);
 int32_t _591 = _f3_min_0 + -4;
 int32_t _592 = min(_591, _588);
 int32_t _593 = max(_592, _input_min_0);
 int32_t _594 = _590 - _593;
 int32_t _595 = _577 + 3;
 int32_t _596 = _input_min_1 + _input_extent_1;
 int32_t _597 = _596 + -1;
 int32_t _598 = min(_595, _597);
 int32_t _599 = max(_598, _input_min_1);
 int32_t _600 = _f3_min_1 + -4;
 int32_t _601 = min(_600, _597);
 int32_t _602 = max(_601, _input_min_1);
 int32_t _603 = _599 - _602;
 int32_t _604 = _f3_min_2 + _f3_extent_2;
 int32_t _605 = _input_min_2 + _input_extent_2;
 int32_t _606 = min(_604, _605);
 int32_t _607 = _606 + -1;
 int32_t _608 = max(_607, _input_min_2);
 int32_t _609 = _605 + -1;
 int32_t _610 = min(_f3_min_2, _609);
 int32_t _611 = max(_610, _input_min_2);
 int32_t _612 = _608 - _611;
 int32_t _613 = _594 + 1;
 int32_t _614 = _603 + 1;
 int32_t _615 = _613 * _614;
 if (_f3_host_and_dev_are_null)
 {
  int32_t _616 = _571 + 1;
  int32_t _617 = _582 + 1;
  bool _618 = halide_rewrite_buffer(_f3_buffer, 1, _570, _616, 1, _581, _617, _616, _f3_min_2, _f3_extent_2, _585, 0, 0, 0);
  (void)_618;
 } // if _f3_host_and_dev_are_null
 if (_input_host_and_dev_are_null)
 {
  int32_t _619 = _594 + 1;
  int32_t _620 = _603 + 1;
  int32_t _621 = _612 + 1;
  bool _622 = halide_rewrite_buffer(_input_buffer, 1, _593, _619, 1, _602, _620, _619, _611, _621, _615, 0, 0, 0);
  (void)_622;
 } // if _input_host_and_dev_are_null
 if (_weight_host_and_dev_are_null)
 {
  bool _623 = halide_rewrite_buffer(_weight_buffer, 1, 0, 5, 1, 0, 5, 5, 0, 0, 0, 0, 0, 0);
  (void)_623;
 } // if _weight_host_and_dev_are_null
 bool _624 = _f3_host_and_dev_are_null || _input_host_and_dev_are_null;
 bool _625 = _624 || _weight_host_and_dev_are_null;
 bool _626 = !(_625);
 if (_626)
 {
  bool _627 = _f3_elem_size == 1;
  if (!_627)   {
   int32_t _628 = halide_error_bad_elem_size(NULL, "Output buffer f3", "uint8", _f3_elem_size, 1);
   return _628;
  }
  bool _629 = _input_elem_size == 1;
  if (!_629)   {
   int32_t _630 = halide_error_bad_elem_size(NULL, "Input buffer input", "uint8", _input_elem_size, 1);
   return _630;
  }
  bool _631 = _weight_elem_size == 1;
  if (!_631)   {
   int32_t _632 = halide_error_bad_elem_size(NULL, "Input buffer weight", "uint8", _weight_elem_size, 1);
   return _632;
  }
  bool _633 = _f3_min_0 <= _570;
  int32_t _634 = _570 + _571;
  int32_t _635 = _634 - _f3_extent_0;
  int32_t _636 = _635 + 1;
  bool _637 = _636 <= _f3_min_0;
  bool _638 = _633 && _637;
  if (!_638)   {
   int32_t _639 = _570 + _571;
   int32_t _640 = _f3_min_0 + _f3_extent_0;
   int32_t _641 = _640 + -1;
   int32_t _642 = halide_error_access_out_of_bounds(NULL, "Output buffer f3", 0, _570, _639, _f3_min_0, _641);
   return _642;
  }
  bool _643 = _f3_min_1 <= _581;
  int32_t _644 = _581 + _582;
  int32_t _645 = _644 - _f3_extent_1;
  int32_t _646 = _645 + 1;
  bool _647 = _646 <= _f3_min_1;
  bool _648 = _643 && _647;
  if (!_648)   {
   int32_t _649 = _581 + _582;
   int32_t _650 = _f3_min_1 + _f3_extent_1;
   int32_t _651 = _650 + -1;
   int32_t _652 = halide_error_access_out_of_bounds(NULL, "Output buffer f3", 1, _581, _649, _f3_min_1, _651);
   return _652;
  }
  bool _653 = _input_min_0 <= _593;
  int32_t _654 = _593 + _594;
  int32_t _655 = _654 - _input_extent_0;
  int32_t _656 = _655 + 1;
  bool _657 = _656 <= _input_min_0;
  bool _658 = _653 && _657;
  if (!_658)   {
   int32_t _659 = _593 + _594;
   int32_t _660 = _input_min_0 + _input_extent_0;
   int32_t _661 = _660 + -1;
   int32_t _662 = halide_error_access_out_of_bounds(NULL, "Input buffer input", 0, _593, _659, _input_min_0, _661);
   return _662;
  }
  bool _663 = _input_min_1 <= _602;
  int32_t _664 = _602 + _603;
  int32_t _665 = _664 - _input_extent_1;
  int32_t _666 = _665 + 1;
  bool _667 = _666 <= _input_min_1;
  bool _668 = _663 && _667;
  if (!_668)   {
   int32_t _669 = _602 + _603;
   int32_t _670 = _input_min_1 + _input_extent_1;
   int32_t _671 = _670 + -1;
   int32_t _672 = halide_error_access_out_of_bounds(NULL, "Input buffer input", 1, _602, _669, _input_min_1, _671);
   return _672;
  }
  bool _673 = _input_min_2 <= _611;
  int32_t _674 = _611 + _612;
  int32_t _675 = _674 - _input_extent_2;
  int32_t _676 = _675 + 1;
  bool _677 = _676 <= _input_min_2;
  bool _678 = _673 && _677;
  if (!_678)   {
   int32_t _679 = _611 + _612;
   int32_t _680 = _input_min_2 + _input_extent_2;
   int32_t _681 = _680 + -1;
   int32_t _682 = halide_error_access_out_of_bounds(NULL, "Input buffer input", 2, _611, _679, _input_min_2, _681);
   return _682;
  }
  bool _683 = _weight_min_0 <= 0;
  int32_t _684 = 5 - _weight_extent_0;
  bool _685 = _684 <= _weight_min_0;
  bool _686 = _683 && _685;
  if (!_686)   {
   int32_t _687 = _weight_min_0 + _weight_extent_0;
   int32_t _688 = _687 + -1;
   int32_t _689 = halide_error_access_out_of_bounds(NULL, "Input buffer weight", 0, 0, 4, _weight_min_0, _688);
   return _689;
  }
  bool _690 = _weight_min_1 <= 0;
  int32_t _691 = 5 - _weight_extent_1;
  bool _692 = _691 <= _weight_min_1;
  bool _693 = _690 && _692;
  if (!_693)   {
   int32_t _694 = _weight_min_1 + _weight_extent_1;
   int32_t _695 = _694 + -1;
   int32_t _696 = halide_error_access_out_of_bounds(NULL, "Input buffer weight", 1, 0, 4, _weight_min_1, _695);
   return _696;
  }
  bool _697 = _f3_stride_0 == 1;
  if (!_697)   {
   int32_t _698 = halide_error_constraint_violated(NULL, "f3.stride.0", _f3_stride_0, "1", 1);
   return _698;
  }
  bool _699 = _input_stride_0 == 1;
  if (!_699)   {
   int32_t _700 = halide_error_constraint_violated(NULL, "input.stride.0", _input_stride_0, "1", 1);
   return _700;
  }
  bool _701 = _weight_stride_0 == 1;
  if (!_701)   {
   int32_t _702 = halide_error_constraint_violated(NULL, "weight.stride.0", _weight_stride_0, "1", 1);
   return _702;
  }
  bool _703 = _weight_min_0 == 0;
  if (!_703)   {
   int32_t _704 = halide_error_constraint_violated(NULL, "weight.min.0", _weight_min_0, "0", 0);
   return _704;
  }
  bool _705 = _weight_extent_0 == 5;
  if (!_705)   {
   int32_t _706 = halide_error_constraint_violated(NULL, "weight.extent.0", _weight_extent_0, "5", 5);
   return _706;
  }
  bool _707 = _weight_stride_1 == 5;
  if (!_707)   {
   int32_t _708 = halide_error_constraint_violated(NULL, "weight.stride.1", _weight_stride_1, "5", 5);
   return _708;
  }
  bool _709 = _weight_min_1 == 0;
  if (!_709)   {
   int32_t _710 = halide_error_constraint_violated(NULL, "weight.min.1", _weight_min_1, "0", 0);
   return _710;
  }
  bool _711 = _weight_extent_1 == 5;
  if (!_711)   {
   int32_t _712 = halide_error_constraint_violated(NULL, "weight.extent.1", _weight_extent_1, "5", 5);
   return _712;
  }
  int64_t _713 = (int64_t)(_f3_extent_1);
  int64_t _714 = (int64_t)(_f3_extent_0);
  int64_t _715 = _713 * _714;
  int64_t _716 = (int64_t)(_input_extent_1);
  int64_t _717 = (int64_t)(_input_extent_0);
  int64_t _718 = _716 * _717;
  int64_t _719 = (int64_t)(2147483647);
  bool _720 = _714 <= _719;
  if (!_720)   {
   int64_t _721 = (int64_t)(_f3_extent_0);
   int64_t _722 = (int64_t)(2147483647);
   int32_t _723 = halide_error_buffer_allocation_too_large(NULL, "f3", _721, _722);
   return _723;
  }
  int64_t _724 = (int64_t)(_f3_extent_1);
  int64_t _725 = (int64_t)(_f3_stride_1);
  int64_t _726 = _724 * _725;
  int64_t _727 = (int64_t)(2147483647);
  bool _728 = _726 <= _727;
  if (!_728)   {
   int64_t _729 = (int64_t)(_f3_extent_1);
   int64_t _730 = (int64_t)(_f3_stride_1);
   int64_t _731 = _729 * _730;
   int64_t _732 = (int64_t)(2147483647);
   int32_t _733 = halide_error_buffer_allocation_too_large(NULL, "f3", _731, _732);
   return _733;
  }
  int64_t _734 = (int64_t)(2147483647);
  bool _735 = _715 <= _734;
  if (!_735)   {
   int64_t _736 = (int64_t)(2147483647);
   int32_t _737 = halide_error_buffer_extents_too_large(NULL, "f3", _715, _736);
   return _737;
  }
  int64_t _738 = (int64_t)(_f3_extent_2);
  int64_t _739 = (int64_t)(_f3_stride_2);
  int64_t _740 = _738 * _739;
  int64_t _741 = (int64_t)(2147483647);
  bool _742 = _740 <= _741;
  if (!_742)   {
   int64_t _743 = (int64_t)(_f3_extent_2);
   int64_t _744 = (int64_t)(_f3_stride_2);
   int64_t _745 = _743 * _744;
   int64_t _746 = (int64_t)(2147483647);
   int32_t _747 = halide_error_buffer_allocation_too_large(NULL, "f3", _745, _746);
   return _747;
  }
  int64_t _748 = (int64_t)(_f3_extent_2);
  int64_t _749 = _748 * _715;
  int64_t _750 = (int64_t)(2147483647);
  bool _751 = _749 <= _750;
  if (!_751)   {
   int64_t _752 = (int64_t)(_f3_extent_2);
   int64_t _753 = _752 * _715;
   int64_t _754 = (int64_t)(2147483647);
   int32_t _755 = halide_error_buffer_extents_too_large(NULL, "f3", _753, _754);
   return _755;
  }
  int64_t _756 = (int64_t)(_input_extent_0);
  int64_t _757 = (int64_t)(2147483647);
  bool _758 = _756 <= _757;
  if (!_758)   {
   int64_t _759 = (int64_t)(_input_extent_0);
   int64_t _760 = (int64_t)(2147483647);
   int32_t _761 = halide_error_buffer_allocation_too_large(NULL, "input", _759, _760);
   return _761;
  }
  int64_t _762 = (int64_t)(_input_extent_1);
  int64_t _763 = (int64_t)(_input_stride_1);
  int64_t _764 = _762 * _763;
  int64_t _765 = (int64_t)(2147483647);
  bool _766 = _764 <= _765;
  if (!_766)   {
   int64_t _767 = (int64_t)(_input_extent_1);
   int64_t _768 = (int64_t)(_input_stride_1);
   int64_t _769 = _767 * _768;
   int64_t _770 = (int64_t)(2147483647);
   int32_t _771 = halide_error_buffer_allocation_too_large(NULL, "input", _769, _770);
   return _771;
  }
  int64_t _772 = (int64_t)(2147483647);
  bool _773 = _718 <= _772;
  if (!_773)   {
   int64_t _774 = (int64_t)(2147483647);
   int32_t _775 = halide_error_buffer_extents_too_large(NULL, "input", _718, _774);
   return _775;
  }
  int64_t _776 = (int64_t)(_input_extent_2);
  int64_t _777 = (int64_t)(_input_stride_2);
  int64_t _778 = _776 * _777;
  int64_t _779 = (int64_t)(2147483647);
  bool _780 = _778 <= _779;
  if (!_780)   {
   int64_t _781 = (int64_t)(_input_extent_2);
   int64_t _782 = (int64_t)(_input_stride_2);
   int64_t _783 = _781 * _782;
   int64_t _784 = (int64_t)(2147483647);
   int32_t _785 = halide_error_buffer_allocation_too_large(NULL, "input", _783, _784);
   return _785;
  }
  int64_t _786 = (int64_t)(_input_extent_2);
  int64_t _787 = _786 * _718;
  int64_t _788 = (int64_t)(2147483647);
  bool _789 = _787 <= _788;
  if (!_789)   {
   int64_t _790 = (int64_t)(_input_extent_2);
   int64_t _791 = _790 * _718;
   int64_t _792 = (int64_t)(2147483647);
   int32_t _793 = halide_error_buffer_extents_too_large(NULL, "input", _791, _792);
   return _793;
  }
  int64_t _794 = (int64_t)(5);
  int64_t _795 = (int64_t)(2147483647);
  bool _796 = _794 <= _795;
  if (!_796)   {
   int64_t _797 = (int64_t)(5);
   int64_t _798 = (int64_t)(2147483647);
   int32_t _799 = halide_error_buffer_allocation_too_large(NULL, "weight", _797, _798);
   return _799;
  }
  int64_t _800 = (int64_t)(5);
  int64_t _801 = _800 * _800;
  int64_t _802 = (int64_t)(2147483647);
  bool _803 = _801 <= _802;
  if (!_803)   {
   int64_t _804 = (int64_t)(5);
   int64_t _805 = _804 * _804;
   int64_t _806 = (int64_t)(2147483647);
   int32_t _807 = halide_error_buffer_allocation_too_large(NULL, "weight", _805, _806);
   return _807;
  }
  int64_t _808 = (int64_t)(5);
  int64_t _809 = _808 * _808;
  int64_t _810 = (int64_t)(2147483647);
  bool _811 = _809 <= _810;
  if (!_811)   {
   int64_t _812 = (int64_t)(5);
   int64_t _813 = _812 * _812;
   int64_t _814 = (int64_t)(2147483647);
   int32_t _815 = halide_error_buffer_extents_too_large(NULL, "weight", _813, _814);
   return _815;
  }
  int32_t _816 = _f3_extent_1 + -1;
  int32_t _817 = _816 >> 8;
  int32_t _818 = _817 * 256;
  int32_t _819 = _818 + _f3_min_1;
  int32_t _820 = _819 + 255;
  int32_t _821 = _f3_min_1 + _f3_extent_1;
  int32_t _822 = _821 + -1;
  int32_t _823 = min(_820, _822);
  int32_t _824 = _823 + 4;
  int32_t _825 = _821 + 3;
  int32_t _826 = max(_824, _825);
  int32_t _827 = _f3_extent_0 + -1;
  int32_t _828 = _827 >> 8;
  int32_t _829 = _828 * 256;
  int32_t _830 = _829 + _f3_min_0;
  int32_t _831 = _830 + 255;
  int32_t _832 = _f3_min_0 + _f3_extent_0;
  int32_t _833 = _832 + -1;
  int32_t _834 = min(_831, _833);
  int32_t _835 = _834 + 4;
  int32_t _836 = _832 + 3;
  int32_t _837 = max(_835, _836);
  int32_t _838 = _837 - _570;
  int32_t _839 = _838 + 5;
  int32_t _840 = _826 - _581;
  int32_t _841 = _840 + 5;
  int32_t _842 = _839 * _841;
  {
   int32_t _843 = _837 - _570;
   int32_t _844 = _843 + 5;
   int64_t _845 = _844;
   int32_t _846 = _826 - _581;
   int32_t _847 = _846 + 5;
   int64_t _848 = _845 * _847;
   int64_t _849 = (_848 > ((int64_t(1) << 31) - 1)) ? _848 : (_848 * _f3_extent_2);
   if ((_849 > ((int64_t(1) << 31) - 1)) || ((_849 * sizeof(uint8_t)) > ((int64_t(1) << 31) - 1)))
   {
    halide_error(NULL, "32-bit signed overflow computing size of allocation clamped_buf\n");
    return -1;
   } // overflow test clamped_buf
   int64_t _850 = _849;
   uint8_t *_clamped_buf = (uint8_t *)halide_malloc(NULL, sizeof(uint8_t)*_850);
   // produce clamped_buf
   for (int _clamped_buf_s0__2 = _f3_min_2; _clamped_buf_s0__2 < _f3_min_2 + _f3_extent_2; _clamped_buf_s0__2++)
   {
    int32_t _851 = _f3_min_1 + -4;
    int32_t _852 = _f3_extent_1 + 8;
    for (int _clamped_buf_s0__1 = _851; _clamped_buf_s0__1 < _851 + _852; _clamped_buf_s0__1++)
    {
     int32_t _853 = _f3_min_0 + _f3_extent_0;
     int32_t _854 = _853 + 4;
     int32_t _855 = min(_input_min_0, _854);
     int32_t _856 = _f3_min_0 + -4;
     int32_t _857 = max(_855, _856);
     int32_t _858 = _input_min_0 + _input_extent_0;
     int32_t _859 = min(_858, _854);
     int32_t _860 = max(_859, _857);
     int32_t _861 = _857 - _f3_min_0;
     int32_t _862 = _861 + 4;
     for (int _clamped_buf_s0__0 = _856; _clamped_buf_s0__0 < _856 + _862; _clamped_buf_s0__0++)
     {
      int32_t _863 = _clamped_buf_s0__0 - _570;
      int32_t _864 = _clamped_buf_s0__1 - _581;
      int32_t _865 = _864 + 4;
      int32_t _866 = _837 - _570;
      int32_t _867 = _866 + 5;
      int32_t _868 = _865 * _867;
      int32_t _869 = _863 + _868;
      int32_t _870 = _clamped_buf_s0__2 - _f3_min_2;
      int32_t _871 = _870 * _842;
      int32_t _872 = _869 + _871;
      int32_t _873 = _872 + 4;
      int32_t _874 = _input_min_0 + _input_extent_0;
      int32_t _875 = _874 + -1;
      int32_t _876 = min(_clamped_buf_s0__0, _875);
      int32_t _877 = max(_876, _input_min_0);
      int32_t _878 = _input_min_1 + _input_extent_1;
      int32_t _879 = _878 + -1;
      int32_t _880 = min(_clamped_buf_s0__1, _879);
      int32_t _881 = max(_880, _input_min_1);
      int32_t _882 = _881 * _input_stride_1;
      int32_t _883 = _877 + _882;
      int32_t _884 = _input_min_2 + _input_extent_2;
      int32_t _885 = _884 + -1;
      int32_t _886 = min(_clamped_buf_s0__2, _885);
      int32_t _887 = max(_886, _input_min_2);
      int32_t _888 = _887 * _input_stride_2;
      int32_t _889 = _883 + _888;
      int32_t _890 = _input_min_1 * _input_stride_1;
      int32_t _891 = _input_min_0 + _890;
      int32_t _892 = _input_min_2 * _input_stride_2;
      int32_t _893 = _891 + _892;
      int32_t _894 = _889 - _893;
      uint8_t _895 = _input[_894];
      _clamped_buf[_873] = _895;
     } // for _clamped_buf_s0__0
     int32_t _896 = _860 - _857;
     for (int _clamped_buf_s0__0 = _857; _clamped_buf_s0__0 < _857 + _896; _clamped_buf_s0__0++)
     {
      int32_t _897 = _clamped_buf_s0__0 - _570;
      int32_t _898 = _clamped_buf_s0__1 - _581;
      int32_t _899 = _898 + 4;
      int32_t _900 = _837 - _570;
      int32_t _901 = _900 + 5;
      int32_t _902 = _899 * _901;
      int32_t _903 = _897 + _902;
      int32_t _904 = _clamped_buf_s0__2 - _f3_min_2;
      int32_t _905 = _904 * _842;
      int32_t _906 = _903 + _905;
      int32_t _907 = _906 + 4;
      int32_t _908 = _input_min_1 + _input_extent_1;
      int32_t _909 = _908 + -1;
      int32_t _910 = min(_clamped_buf_s0__1, _909);
      int32_t _911 = max(_910, _input_min_1);
      int32_t _912 = _911 * _input_stride_1;
      int32_t _913 = _clamped_buf_s0__0 + _912;
      int32_t _914 = _input_min_2 + _input_extent_2;
      int32_t _915 = _914 + -1;
      int32_t _916 = min(_clamped_buf_s0__2, _915);
      int32_t _917 = max(_916, _input_min_2);
      int32_t _918 = _917 * _input_stride_2;
      int32_t _919 = _913 + _918;
      int32_t _920 = _input_min_1 * _input_stride_1;
      int32_t _921 = _input_min_0 + _920;
      int32_t _922 = _input_min_2 * _input_stride_2;
      int32_t _923 = _921 + _922;
      int32_t _924 = _919 - _923;
      uint8_t _925 = _input[_924];
      _clamped_buf[_907] = _925;
     } // for _clamped_buf_s0__0
     int32_t _926 = _f3_min_0 + _f3_extent_0;
     int32_t _927 = _926 - _860;
     int32_t _928 = _927 + 4;
     for (int _clamped_buf_s0__0 = _860; _clamped_buf_s0__0 < _860 + _928; _clamped_buf_s0__0++)
     {
      int32_t _929 = _clamped_buf_s0__0 - _570;
      int32_t _930 = _clamped_buf_s0__1 - _581;
      int32_t _931 = _930 + 4;
      int32_t _932 = _837 - _570;
      int32_t _933 = _932 + 5;
      int32_t _934 = _931 * _933;
      int32_t _935 = _929 + _934;
      int32_t _936 = _clamped_buf_s0__2 - _f3_min_2;
      int32_t _937 = _936 * _842;
      int32_t _938 = _935 + _937;
      int32_t _939 = _938 + 4;
      int32_t _940 = _input_min_0 + _input_extent_0;
      int32_t _941 = _940 + -1;
      int32_t _942 = max(_941, _input_min_0);
      int32_t _943 = _input_min_1 + _input_extent_1;
      int32_t _944 = _943 + -1;
      int32_t _945 = min(_clamped_buf_s0__1, _944);
      int32_t _946 = max(_945, _input_min_1);
      int32_t _947 = _946 * _input_stride_1;
      int32_t _948 = _942 + _947;
      int32_t _949 = _input_min_2 + _input_extent_2;
      int32_t _950 = _949 + -1;
      int32_t _951 = min(_clamped_buf_s0__2, _950);
      int32_t _952 = max(_951, _input_min_2);
      int32_t _953 = _952 * _input_stride_2;
      int32_t _954 = _948 + _953;
      int32_t _955 = _input_min_1 * _input_stride_1;
      int32_t _956 = _input_min_0 + _955;
      int32_t _957 = _input_min_2 * _input_stride_2;
      int32_t _958 = _956 + _957;
      int32_t _959 = _954 - _958;
      uint8_t _960 = _input[_959];
      _clamped_buf[_939] = _960;
     } // for _clamped_buf_s0__0
    } // for _clamped_buf_s0__1
   } // for _clamped_buf_s0__2
   // consume clamped_buf
   // produce f3
   {
    Stencil<uint8_t, 5, 5> _weight_stencil;
    // produce weight.stencil
    buffer_to_stencil(_weight_buffer, _weight_stencil);
    (void)0;
    // consume weight.stencil
    for (int _f3_s0_c = _f3_min_2; _f3_s0_c < _f3_min_2 + _f3_extent_2; _f3_s0_c++)
    {
     int32_t _961 = _f3_extent_1 + 255;
     int32_t _962 = _961 >> 8;
     for (int _f3_s0_y_yo = 0; _f3_s0_y_yo < 0 + _962; _f3_s0_y_yo++)
     {
      int32_t _963 = _f3_s0_y_yo * 256;
      int32_t _964 = _963 + _f3_min_1;
      int32_t _965 = _f3_min_1 + _f3_extent_1;
      int32_t _966 = _965 + -256;
      int32_t _967 = min(_964, _966);
      int32_t _968 = _f3_extent_0 + 255;
      int32_t _969 = _968 >> 8;
      for (int _f3_s0_x_xo = 0; _f3_s0_x_xo < 0 + _969; _f3_s0_x_xo++)
      {
       int32_t _970 = _f3_s0_x_xo * 256;
       int32_t _971 = _970 + _f3_min_0;
       int32_t _972 = _f3_min_0 + _f3_extent_0;
       int32_t _973 = _972 + -256;
       int32_t _974 = min(_971, _973);
       {
        hls::stream<Stencil<uint8_t, 1, 1, 1> > _clamped_stream_stencil_stream;
        // produce clamped_stream.stencil.stream
        {
         hls::stream<Stencil<uint8_t, 1, 1, 1> > _clamped_stream_stencil_update_stream;
         // produce clamped_stream.stencil_update.stream
         for (int _clamped_stream_scan_update__1 = 0; _clamped_stream_scan_update__1 < 0 + 264; _clamped_stream_scan_update__1++)
         {
          for (int _clamped_stream_scan_update__0 = 0; _clamped_stream_scan_update__0 < 0 + 264; _clamped_stream_scan_update__0++)
          {
           {
            Stencil<uint8_t, 1, 1, 1> _clamped_stream_stencil_update;
            // produce clamped_stream.stencil_update
            int32_t _975 = _974 + _clamped_stream_scan_update__0;
            int32_t _976 = _975 - _570;
            int32_t _977 = _967 + _clamped_stream_scan_update__1;
            int32_t _978 = _977 - _581;
            int32_t _979 = _837 - _570;
            int32_t _980 = _979 + 5;
            int32_t _981 = _978 * _980;
            int32_t _982 = _976 + _981;
            int32_t _983 = _f3_s0_c - _f3_min_2;
            int32_t _984 = _983 * _842;
            int32_t _985 = _982 + _984;
            uint8_t _986 = _clamped_buf[_985];
            _clamped_stream_stencil_update(0, 0, 0) = _986;
            // consume clamped_stream.stencil_update
            _clamped_stream_stencil_update_stream.write(_clamped_stream_stencil_update);
            (void)0;
           } // realize _clamped_stream_stencil_update
          } // for _clamped_stream_scan_update__0
         } // for _clamped_stream_scan_update__1
         // consume clamped_stream.stencil_update.stream
         linebuffer<264, 264, 1>(_clamped_stream_stencil_update_stream, _clamped_stream_stencil_stream);
         (void)0;
        } // realize _clamped_stream_stencil_update_stream
        // consume clamped_stream.stencil.stream
        {
         hls::stream<Stencil<uint8_t, 1, 1, 1> > _f3_stream_stencil_stream;
         // produce _hls_target.f3_stream.stencil.stream
         _hls_target_f3_stream_stencil_stream(_clamped_stream_stencil_stream, _f3_stream_stencil_stream, _p2___bias, _weight_stencil);
         // consume _hls_target.f3_stream.stencil.stream
         for (int _f3_s0_y_yi = 0; _f3_s0_y_yi < 0 + 256; _f3_s0_y_yi++)
         {
          for (int _f3_s0_x_xi = 0; _f3_s0_x_xi < 0 + 256; _f3_s0_x_xi++)
          {
           {
            Stencil<uint8_t, 1, 1, 1> _f3_stream_stencil;
            // produce f3_stream.stencil
            _f3_stream_stencil = _f3_stream_stencil_stream.read();
            (void)0;
            // consume f3_stream.stencil
            int32_t _1014 = _974 + _f3_s0_x_xi;
            int32_t _1015 = _967 + _f3_s0_y_yi;
            int32_t _1016 = _1015 * _f3_stride_1;
            int32_t _1017 = _1014 + _1016;
            int32_t _1018 = _f3_s0_c * _f3_stride_2;
            int32_t _1019 = _1017 + _1018;
            int32_t _1020 = _f3_min_1 * _f3_stride_1;
            int32_t _1021 = _f3_min_0 + _1020;
            int32_t _1022 = _f3_min_2 * _f3_stride_2;
            int32_t _1023 = _1021 + _1022;
            int32_t _1024 = _1019 - _1023;
            uint8_t _1025 = _f3_stream_stencil(0, 0, 0);
            _f3[_1024] = _1025;
           } // realize _f3_stream_stencil
          } // for _f3_s0_x_xi
         } // for _f3_s0_y_yi
        } // realize _f3_stream_stencil_stream
       } // realize _clamped_stream_stencil_stream
      } // for _f3_s0_x_xo
     } // for _f3_s0_y_yo
    } // for _f3_s0_c
   } // realize _weight_stencil
   halide_free(NULL, _clamped_buf);
   // consume f3
  } // alloc _clamped_buf
 } // if _626
 return 0;
}


int pipeline_hls(buffer_t *_input_buffer, buffer_t *_weight_buffer, const uint8_t _p2___bias, buffer_t *_f3_buffer) HALIDE_FUNCTION_ATTRS {
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
 uint8_t *_f3 = (uint8_t *)(_f3_buffer->host);
 (void)_f3;
 const bool _f3_host_and_dev_are_null = (_f3_buffer->host == NULL) && (_f3_buffer->dev == 0);
 (void)_f3_host_and_dev_are_null;
 const int32_t _f3_min_0 = _f3_buffer->min[0];
 (void)_f3_min_0;
 const int32_t _f3_min_1 = _f3_buffer->min[1];
 (void)_f3_min_1;
 const int32_t _f3_min_2 = _f3_buffer->min[2];
 (void)_f3_min_2;
 const int32_t _f3_min_3 = _f3_buffer->min[3];
 (void)_f3_min_3;
 const int32_t _f3_extent_0 = _f3_buffer->extent[0];
 (void)_f3_extent_0;
 const int32_t _f3_extent_1 = _f3_buffer->extent[1];
 (void)_f3_extent_1;
 const int32_t _f3_extent_2 = _f3_buffer->extent[2];
 (void)_f3_extent_2;
 const int32_t _f3_extent_3 = _f3_buffer->extent[3];
 (void)_f3_extent_3;
 const int32_t _f3_stride_0 = _f3_buffer->stride[0];
 (void)_f3_stride_0;
 const int32_t _f3_stride_1 = _f3_buffer->stride[1];
 (void)_f3_stride_1;
 const int32_t _f3_stride_2 = _f3_buffer->stride[2];
 (void)_f3_stride_2;
 const int32_t _f3_stride_3 = _f3_buffer->stride[3];
 (void)_f3_stride_3;
 const int32_t _f3_elem_size = _f3_buffer->elem_size;
 (void)_f3_elem_size;
 int32_t _1026 = __pipeline_hls(_input_buffer, _weight_buffer, _p2___bias, _f3_buffer);
 bool _1027 = _1026 == 0;
 if (!_1027)  {
  return _1026;
 }
 return 0;
}
#ifdef __cplusplus
}  // extern "C"
#endif
