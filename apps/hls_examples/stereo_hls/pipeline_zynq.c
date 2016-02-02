#include <iostream>
#include <math.h>
#include <float.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "buffer.h"
#include "ioctl_cmds.h"

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

static int __pipeline_hls(buffer_t *_p5_buffer, buffer_t *_p4_buffer, buffer_t *_p7_buffer, buffer_t *_p6_buffer, buffer_t *_output_2_buffer) HALIDE_FUNCTION_ATTRS {
  // Open the hardware device
  int hwacc = open("/dev/hwacc0", O_RDWR);
  if(hwacc == -1){
    printf("Failed to open hardware device!\n");
    return(0);
  }

  // Allocate input and output buffers
  Buffer bufs[3];
  bufs[0].width = 263;
  bufs[0].height = 263;
  bufs[0].depth = 1;
  bufs[0].stride = 263;

  bufs[1].width = 326;
  bufs[1].height = 263;
  bufs[1].depth = 1;
  bufs[1].stride = 326;

  bufs[2].width = 256;
  bufs[2].height = 256;
  bufs[2].depth = 1;
  bufs[2].stride = 256;

  int ok = ioctl(hwacc, GET_BUFFER, (long unsigned int)bufs);
  if(ok < 0){
    printf("Failed to allocate buffer 0!\n");
    return(0);
  }

  ok = ioctl(hwacc, GET_BUFFER, (long unsigned int)(bufs + 1));
  if(ok < 0){
    printf("Failed to allocate buffer 1!\n");
    return(0);
  }

  ok = ioctl(hwacc, GET_BUFFER, (long unsigned int)(bufs + 2));
  if(ok < 0){
    printf("Failed to allocate buffer 2!\n");
    return(0);
  }

 uint8_t *_p5 = (uint8_t *)(_p5_buffer->host);
 (void)_p5;
 const bool _p5_host_and_dev_are_null = (_p5_buffer->host == NULL) && (_p5_buffer->dev == 0);
 (void)_p5_host_and_dev_are_null;
 const int32_t _p5_min_0 = _p5_buffer->min[0];
 (void)_p5_min_0;
 const int32_t _p5_min_1 = _p5_buffer->min[1];
 (void)_p5_min_1;
 const int32_t _p5_min_2 = _p5_buffer->min[2];
 (void)_p5_min_2;
 const int32_t _p5_min_3 = _p5_buffer->min[3];
 (void)_p5_min_3;
 const int32_t _p5_extent_0 = _p5_buffer->extent[0];
 (void)_p5_extent_0;
 const int32_t _p5_extent_1 = _p5_buffer->extent[1];
 (void)_p5_extent_1;
 const int32_t _p5_extent_2 = _p5_buffer->extent[2];
 (void)_p5_extent_2;
 const int32_t _p5_extent_3 = _p5_buffer->extent[3];
 (void)_p5_extent_3;
 const int32_t _p5_stride_0 = _p5_buffer->stride[0];
 (void)_p5_stride_0;
 const int32_t _p5_stride_1 = _p5_buffer->stride[1];
 (void)_p5_stride_1;
 const int32_t _p5_stride_2 = _p5_buffer->stride[2];
 (void)_p5_stride_2;
 const int32_t _p5_stride_3 = _p5_buffer->stride[3];
 (void)_p5_stride_3;
 const int32_t _p5_elem_size = _p5_buffer->elem_size;
 (void)_p5_elem_size;
 uint8_t *_p4 = (uint8_t *)(_p4_buffer->host);
 (void)_p4;
 const bool _p4_host_and_dev_are_null = (_p4_buffer->host == NULL) && (_p4_buffer->dev == 0);
 (void)_p4_host_and_dev_are_null;
 const int32_t _p4_min_0 = _p4_buffer->min[0];
 (void)_p4_min_0;
 const int32_t _p4_min_1 = _p4_buffer->min[1];
 (void)_p4_min_1;
 const int32_t _p4_min_2 = _p4_buffer->min[2];
 (void)_p4_min_2;
 const int32_t _p4_min_3 = _p4_buffer->min[3];
 (void)_p4_min_3;
 const int32_t _p4_extent_0 = _p4_buffer->extent[0];
 (void)_p4_extent_0;
 const int32_t _p4_extent_1 = _p4_buffer->extent[1];
 (void)_p4_extent_1;
 const int32_t _p4_extent_2 = _p4_buffer->extent[2];
 (void)_p4_extent_2;
 const int32_t _p4_extent_3 = _p4_buffer->extent[3];
 (void)_p4_extent_3;
 const int32_t _p4_stride_0 = _p4_buffer->stride[0];
 (void)_p4_stride_0;
 const int32_t _p4_stride_1 = _p4_buffer->stride[1];
 (void)_p4_stride_1;
 const int32_t _p4_stride_2 = _p4_buffer->stride[2];
 (void)_p4_stride_2;
 const int32_t _p4_stride_3 = _p4_buffer->stride[3];
 (void)_p4_stride_3;
 const int32_t _p4_elem_size = _p4_buffer->elem_size;
 (void)_p4_elem_size;
 uint8_t *_p7 = (uint8_t *)(_p7_buffer->host);
 (void)_p7;
 const bool _p7_host_and_dev_are_null = (_p7_buffer->host == NULL) && (_p7_buffer->dev == 0);
 (void)_p7_host_and_dev_are_null;
 const int32_t _p7_min_0 = _p7_buffer->min[0];
 (void)_p7_min_0;
 const int32_t _p7_min_1 = _p7_buffer->min[1];
 (void)_p7_min_1;
 const int32_t _p7_min_2 = _p7_buffer->min[2];
 (void)_p7_min_2;
 const int32_t _p7_min_3 = _p7_buffer->min[3];
 (void)_p7_min_3;
 const int32_t _p7_extent_0 = _p7_buffer->extent[0];
 (void)_p7_extent_0;
 const int32_t _p7_extent_1 = _p7_buffer->extent[1];
 (void)_p7_extent_1;
 const int32_t _p7_extent_2 = _p7_buffer->extent[2];
 (void)_p7_extent_2;
 const int32_t _p7_extent_3 = _p7_buffer->extent[3];
 (void)_p7_extent_3;
 const int32_t _p7_stride_0 = _p7_buffer->stride[0];
 (void)_p7_stride_0;
 const int32_t _p7_stride_1 = _p7_buffer->stride[1];
 (void)_p7_stride_1;
 const int32_t _p7_stride_2 = _p7_buffer->stride[2];
 (void)_p7_stride_2;
 const int32_t _p7_stride_3 = _p7_buffer->stride[3];
 (void)_p7_stride_3;
 const int32_t _p7_elem_size = _p7_buffer->elem_size;
 (void)_p7_elem_size;
 uint8_t *_p6 = (uint8_t *)(_p6_buffer->host);
 (void)_p6;
 const bool _p6_host_and_dev_are_null = (_p6_buffer->host == NULL) && (_p6_buffer->dev == 0);
 (void)_p6_host_and_dev_are_null;
 const int32_t _p6_min_0 = _p6_buffer->min[0];
 (void)_p6_min_0;
 const int32_t _p6_min_1 = _p6_buffer->min[1];
 (void)_p6_min_1;
 const int32_t _p6_min_2 = _p6_buffer->min[2];
 (void)_p6_min_2;
 const int32_t _p6_min_3 = _p6_buffer->min[3];
 (void)_p6_min_3;
 const int32_t _p6_extent_0 = _p6_buffer->extent[0];
 (void)_p6_extent_0;
 const int32_t _p6_extent_1 = _p6_buffer->extent[1];
 (void)_p6_extent_1;
 const int32_t _p6_extent_2 = _p6_buffer->extent[2];
 (void)_p6_extent_2;
 const int32_t _p6_extent_3 = _p6_buffer->extent[3];
 (void)_p6_extent_3;
 const int32_t _p6_stride_0 = _p6_buffer->stride[0];
 (void)_p6_stride_0;
 const int32_t _p6_stride_1 = _p6_buffer->stride[1];
 (void)_p6_stride_1;
 const int32_t _p6_stride_2 = _p6_buffer->stride[2];
 (void)_p6_stride_2;
 const int32_t _p6_stride_3 = _p6_buffer->stride[3];
 (void)_p6_stride_3;
 const int32_t _p6_elem_size = _p6_buffer->elem_size;
 (void)_p6_elem_size;
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
 int32_t _8 = _output_2_extent_0 + -1;
 int32_t _9 = _8 >> 8;
 int32_t _10 = _9 * 256;
 int32_t _11 = _10 + _output_2_min_0;
 int32_t _12 = _11 + 255;
 int32_t _13 = _output_2_min_0 + _output_2_extent_0;
 int32_t _14 = _13 + -1;
 int32_t _15 = min(_12, _14);
 int32_t _16 = _13 + -256;
 int32_t _17 = min(_output_2_min_0, _16);
 int32_t _18 = _15 - _17;
 int32_t _19 = _output_2_extent_1 + -1;
 int32_t _20 = _19 >> 8;
 int32_t _21 = _20 * 256;
 int32_t _22 = _21 + _output_2_min_1;
 int32_t _23 = _22 + 255;
 int32_t _24 = _output_2_min_1 + _output_2_extent_1;
 int32_t _25 = _24 + -1;
 int32_t _26 = min(_23, _25);
 int32_t _27 = _24 + -256;
 int32_t _28 = min(_output_2_min_1, _27);
 int32_t _29 = _26 - _28;
 int32_t _30 = _13 + 93;
 int32_t _31 = _p4_min_0 + _p4_extent_0;
 int32_t _32 = _31 + -1;
 int32_t _33 = min(_30, _32);
 int32_t _34 = max(_33, _p4_min_0);
 int32_t _35 = _output_2_min_0 + 8;
 int32_t _36 = min(_35, _32);
 int32_t _37 = max(_36, _p4_min_0);
 int32_t _38 = _34 - _37;
 int32_t _39 = _24 + 10;
 int32_t _40 = _p4_min_1 + _p4_extent_1;
 int32_t _41 = _40 + -1;
 int32_t _42 = min(_39, _41);
 int32_t _43 = max(_42, _p4_min_1);
 int32_t _44 = _output_2_min_1 + -12;
 int32_t _45 = min(_44, _41);
 int32_t _46 = max(_45, _p4_min_1);
 int32_t _47 = _43 - _46;
 int32_t _48 = _p4_min_2 + _p4_extent_2;
 int32_t _49 = _48 + -1;
 int32_t _50 = min(_49, 1);
 int32_t _51 = max(_50, _p4_min_2);
 int32_t _52 = _38 + 1;
 int32_t _53 = _47 + 1;
 int32_t _54 = _52 * _53;
 int32_t _55 = _13 + 10;
 int32_t _56 = _p5_min_0 + _p5_extent_0;
 int32_t _57 = _56 + -1;
 int32_t _58 = min(_55, _57);
 int32_t _59 = max(_58, _p5_min_0);
 int32_t _60 = _output_2_min_0 + -12;
 int32_t _61 = min(_60, _57);
 int32_t _62 = max(_61, _p5_min_0);
 int32_t _63 = _59 - _62;
 int32_t _64 = _p5_min_1 + _p5_extent_1;
 int32_t _65 = _64 + -1;
 int32_t _66 = min(_39, _65);
 int32_t _67 = max(_66, _p5_min_1);
 int32_t _68 = min(_44, _65);
 int32_t _69 = max(_68, _p5_min_1);
 int32_t _70 = _67 - _69;
 int32_t _71 = _p5_min_2 + _p5_extent_2;
 int32_t _72 = _71 + -1;
 int32_t _73 = min(_72, 1);
 int32_t _74 = max(_73, _p5_min_2);
 int32_t _75 = _63 + 1;
 int32_t _76 = _70 + 1;
 int32_t _77 = _75 * _76;
 int32_t _78 = _13 + 85;
 int32_t _79 = _p6_min_0 + _p6_extent_0;
 int32_t _80 = _79 + -1;
 int32_t _81 = min(_78, _80);
 int32_t _82 = max(_81, _p6_min_0);
 int32_t _83 = _output_2_min_0 + 16;
 int32_t _84 = min(_83, _80);
 int32_t _85 = max(_84, _p6_min_0);
 int32_t _86 = _82 - _85;
 int32_t _87 = _24 + 2;
 int32_t _88 = _p6_min_1 + _p6_extent_1;
 int32_t _89 = _88 + -1;
 int32_t _90 = min(_87, _89);
 int32_t _91 = max(_90, _p6_min_1);
 int32_t _92 = _output_2_min_1 + -4;
 int32_t _93 = min(_92, _89);
 int32_t _94 = max(_93, _p6_min_1);
 int32_t _95 = _91 - _94;
 int32_t _96 = _p6_min_2 + _p6_extent_2;
 int32_t _97 = _96 + -1;
 int32_t _98 = min(_97, 1);
 int32_t _99 = max(_98, _p6_min_2);
 int32_t _100 = min(_97, 0);
 int32_t _101 = max(_100, _p6_min_2);
 int32_t _102 = _99 - _101;
 int32_t _103 = _86 + 1;
 int32_t _104 = _95 + 1;
 int32_t _105 = _103 * _104;
 int32_t _106 = _13 + 2;
 int32_t _107 = _p7_min_0 + _p7_extent_0;
 int32_t _108 = _107 + -1;
 int32_t _109 = min(_106, _108);
 int32_t _110 = max(_109, _p7_min_0);
 int32_t _111 = _output_2_min_0 + -4;
 int32_t _112 = min(_111, _108);
 int32_t _113 = max(_112, _p7_min_0);
 int32_t _114 = _110 - _113;
 int32_t _115 = _p7_min_1 + _p7_extent_1;
 int32_t _116 = _115 + -1;
 int32_t _117 = min(_87, _116);
 int32_t _118 = max(_117, _p7_min_1);
 int32_t _119 = min(_92, _116);
 int32_t _120 = max(_119, _p7_min_1);
 int32_t _121 = _118 - _120;
 int32_t _122 = _p7_min_2 + _p7_extent_2;
 int32_t _123 = _122 + -1;
 int32_t _124 = min(_123, 1);
 int32_t _125 = max(_124, _p7_min_2);
 int32_t _126 = min(_123, 0);
 int32_t _127 = max(_126, _p7_min_2);
 int32_t _128 = _125 - _127;
 int32_t _129 = _114 + 1;
 int32_t _130 = _121 + 1;
 int32_t _131 = _129 * _130;
 if (_output_2_host_and_dev_are_null)
 {
  int32_t _132 = _18 + 1;
  int32_t _133 = _29 + 1;
  bool _134 = halide_rewrite_buffer(_output_2_buffer, 1, _17, _132, 1, _28, _133, _132, 0, 0, 0, 0, 0, 0);
  (void)_134;
 } // if _output_2_host_and_dev_are_null
 if (_p4_host_and_dev_are_null)
 {
  int32_t _135 = _38 + 1;
  int32_t _136 = _47 + 1;
  bool _137 = halide_rewrite_buffer(_p4_buffer, 1, _37, _135, 1, _46, _136, _135, _51, 1, _54, 0, 0, 0);
  (void)_137;
 } // if _p4_host_and_dev_are_null
 if (_p5_host_and_dev_are_null)
 {
  int32_t _138 = _63 + 1;
  int32_t _139 = _70 + 1;
  bool _140 = halide_rewrite_buffer(_p5_buffer, 1, _62, _138, 1, _69, _139, _138, _74, 1, _77, 0, 0, 0);
  (void)_140;
 } // if _p5_host_and_dev_are_null
 if (_p6_host_and_dev_are_null)
 {
  int32_t _141 = _86 + 1;
  int32_t _142 = _95 + 1;
  int32_t _143 = _102 + 1;
  bool _144 = halide_rewrite_buffer(_p6_buffer, 1, _85, _141, 1, _94, _142, _141, _101, _143, _105, 0, 0, 0);
  (void)_144;
 } // if _p6_host_and_dev_are_null
 if (_p7_host_and_dev_are_null)
 {
  int32_t _145 = _114 + 1;
  int32_t _146 = _121 + 1;
  int32_t _147 = _128 + 1;
  bool _148 = halide_rewrite_buffer(_p7_buffer, 1, _113, _145, 1, _120, _146, _145, _127, _147, _131, 0, 0, 0);
  (void)_148;
 } // if _p7_host_and_dev_are_null
 bool _149 = _output_2_host_and_dev_are_null || _p4_host_and_dev_are_null;
 bool _150 = _149 || _p5_host_and_dev_are_null;
 bool _151 = _150 || _p6_host_and_dev_are_null;
 bool _152 = _151 || _p7_host_and_dev_are_null;
 bool _153 = !(_152);
 if (_153)
 {
  bool _154 = _output_2_elem_size == 1;
  if (!_154)   {
   int32_t _155 = halide_error_bad_elem_size(NULL, "Output buffer output$2", "uint8", _output_2_elem_size, 1);
   return _155;
  }
  bool _156 = _p4_elem_size == 1;
  if (!_156)   {
   int32_t _157 = halide_error_bad_elem_size(NULL, "Input buffer p4", "uint8", _p4_elem_size, 1);
   return _157;
  }
  bool _158 = _p5_elem_size == 1;
  if (!_158)   {
   int32_t _159 = halide_error_bad_elem_size(NULL, "Input buffer p5", "uint8", _p5_elem_size, 1);
   return _159;
  }
  bool _160 = _p6_elem_size == 1;
  if (!_160)   {
   int32_t _161 = halide_error_bad_elem_size(NULL, "Input buffer p6", "uint8", _p6_elem_size, 1);
   return _161;
  }
  bool _162 = _p7_elem_size == 1;
  if (!_162)   {
   int32_t _163 = halide_error_bad_elem_size(NULL, "Input buffer p7", "uint8", _p7_elem_size, 1);
   return _163;
  }
  bool _164 = _output_2_min_0 <= _17;
  int32_t _165 = _17 + _18;
  int32_t _166 = _165 - _output_2_extent_0;
  int32_t _167 = _166 + 1;
  bool _168 = _167 <= _output_2_min_0;
  bool _169 = _164 && _168;
  if (!_169)   {
   int32_t _170 = _17 + _18;
   int32_t _171 = _output_2_min_0 + _output_2_extent_0;
   int32_t _172 = _171 + -1;
   int32_t _173 = halide_error_access_out_of_bounds(NULL, "Output buffer output$2", 0, _17, _170, _output_2_min_0, _172);
   return _173;
  }
  bool _174 = _output_2_min_1 <= _28;
  int32_t _175 = _28 + _29;
  int32_t _176 = _175 - _output_2_extent_1;
  int32_t _177 = _176 + 1;
  bool _178 = _177 <= _output_2_min_1;
  bool _179 = _174 && _178;
  if (!_179)   {
   int32_t _180 = _28 + _29;
   int32_t _181 = _output_2_min_1 + _output_2_extent_1;
   int32_t _182 = _181 + -1;
   int32_t _183 = halide_error_access_out_of_bounds(NULL, "Output buffer output$2", 1, _28, _180, _output_2_min_1, _182);
   return _183;
  }
  bool _184 = _p4_min_0 <= _37;
  int32_t _185 = _37 + _38;
  int32_t _186 = _185 - _p4_extent_0;
  int32_t _187 = _186 + 1;
  bool _188 = _187 <= _p4_min_0;
  bool _189 = _184 && _188;
  if (!_189)   {
   int32_t _190 = _37 + _38;
   int32_t _191 = _p4_min_0 + _p4_extent_0;
   int32_t _192 = _191 + -1;
   int32_t _193 = halide_error_access_out_of_bounds(NULL, "Input buffer p4", 0, _37, _190, _p4_min_0, _192);
   return _193;
  }
  bool _194 = _p4_min_1 <= _46;
  int32_t _195 = _46 + _47;
  int32_t _196 = _195 - _p4_extent_1;
  int32_t _197 = _196 + 1;
  bool _198 = _197 <= _p4_min_1;
  bool _199 = _194 && _198;
  if (!_199)   {
   int32_t _200 = _46 + _47;
   int32_t _201 = _p4_min_1 + _p4_extent_1;
   int32_t _202 = _201 + -1;
   int32_t _203 = halide_error_access_out_of_bounds(NULL, "Input buffer p4", 1, _46, _200, _p4_min_1, _202);
   return _203;
  }
  bool _204 = _p4_min_2 <= _51;
  int32_t _205 = _51 - _p4_extent_2;
  int32_t _206 = _205 + 1;
  bool _207 = _206 <= _p4_min_2;
  bool _208 = _204 && _207;
  if (!_208)   {
   int32_t _209 = _p4_min_2 + _p4_extent_2;
   int32_t _210 = _209 + -1;
   int32_t _211 = halide_error_access_out_of_bounds(NULL, "Input buffer p4", 2, _51, _51, _p4_min_2, _210);
   return _211;
  }
  bool _212 = _p5_min_0 <= _62;
  int32_t _213 = _62 + _63;
  int32_t _214 = _213 - _p5_extent_0;
  int32_t _215 = _214 + 1;
  bool _216 = _215 <= _p5_min_0;
  bool _217 = _212 && _216;
  if (!_217)   {
   int32_t _218 = _62 + _63;
   int32_t _219 = _p5_min_0 + _p5_extent_0;
   int32_t _220 = _219 + -1;
   int32_t _221 = halide_error_access_out_of_bounds(NULL, "Input buffer p5", 0, _62, _218, _p5_min_0, _220);
   return _221;
  }
  bool _222 = _p5_min_1 <= _69;
  int32_t _223 = _69 + _70;
  int32_t _224 = _223 - _p5_extent_1;
  int32_t _225 = _224 + 1;
  bool _226 = _225 <= _p5_min_1;
  bool _227 = _222 && _226;
  if (!_227)   {
   int32_t _228 = _69 + _70;
   int32_t _229 = _p5_min_1 + _p5_extent_1;
   int32_t _230 = _229 + -1;
   int32_t _231 = halide_error_access_out_of_bounds(NULL, "Input buffer p5", 1, _69, _228, _p5_min_1, _230);
   return _231;
  }
  bool _232 = _p5_min_2 <= _74;
  int32_t _233 = _74 - _p5_extent_2;
  int32_t _234 = _233 + 1;
  bool _235 = _234 <= _p5_min_2;
  bool _236 = _232 && _235;
  if (!_236)   {
   int32_t _237 = _p5_min_2 + _p5_extent_2;
   int32_t _238 = _237 + -1;
   int32_t _239 = halide_error_access_out_of_bounds(NULL, "Input buffer p5", 2, _74, _74, _p5_min_2, _238);
   return _239;
  }
  bool _240 = _p6_min_0 <= _85;
  int32_t _241 = _85 + _86;
  int32_t _242 = _241 - _p6_extent_0;
  int32_t _243 = _242 + 1;
  bool _244 = _243 <= _p6_min_0;
  bool _245 = _240 && _244;
  if (!_245)   {
   int32_t _246 = _85 + _86;
   int32_t _247 = _p6_min_0 + _p6_extent_0;
   int32_t _248 = _247 + -1;
   int32_t _249 = halide_error_access_out_of_bounds(NULL, "Input buffer p6", 0, _85, _246, _p6_min_0, _248);
   return _249;
  }
  bool _250 = _p6_min_1 <= _94;
  int32_t _251 = _94 + _95;
  int32_t _252 = _251 - _p6_extent_1;
  int32_t _253 = _252 + 1;
  bool _254 = _253 <= _p6_min_1;
  bool _255 = _250 && _254;
  if (!_255)   {
   int32_t _256 = _94 + _95;
   int32_t _257 = _p6_min_1 + _p6_extent_1;
   int32_t _258 = _257 + -1;
   int32_t _259 = halide_error_access_out_of_bounds(NULL, "Input buffer p6", 1, _94, _256, _p6_min_1, _258);
   return _259;
  }
  bool _260 = _p6_min_2 <= _101;
  int32_t _261 = _101 + _102;
  int32_t _262 = _261 - _p6_extent_2;
  int32_t _263 = _262 + 1;
  bool _264 = _263 <= _p6_min_2;
  bool _265 = _260 && _264;
  if (!_265)   {
   int32_t _266 = _101 + _102;
   int32_t _267 = _p6_min_2 + _p6_extent_2;
   int32_t _268 = _267 + -1;
   int32_t _269 = halide_error_access_out_of_bounds(NULL, "Input buffer p6", 2, _101, _266, _p6_min_2, _268);
   return _269;
  }
  bool _270 = _p7_min_0 <= _113;
  int32_t _271 = _113 + _114;
  int32_t _272 = _271 - _p7_extent_0;
  int32_t _273 = _272 + 1;
  bool _274 = _273 <= _p7_min_0;
  bool _275 = _270 && _274;
  if (!_275)   {
   int32_t _276 = _113 + _114;
   int32_t _277 = _p7_min_0 + _p7_extent_0;
   int32_t _278 = _277 + -1;
   int32_t _279 = halide_error_access_out_of_bounds(NULL, "Input buffer p7", 0, _113, _276, _p7_min_0, _278);
   return _279;
  }
  bool _280 = _p7_min_1 <= _120;
  int32_t _281 = _120 + _121;
  int32_t _282 = _281 - _p7_extent_1;
  int32_t _283 = _282 + 1;
  bool _284 = _283 <= _p7_min_1;
  bool _285 = _280 && _284;
  if (!_285)   {
   int32_t _286 = _120 + _121;
   int32_t _287 = _p7_min_1 + _p7_extent_1;
   int32_t _288 = _287 + -1;
   int32_t _289 = halide_error_access_out_of_bounds(NULL, "Input buffer p7", 1, _120, _286, _p7_min_1, _288);
   return _289;
  }
  bool _290 = _p7_min_2 <= _127;
  int32_t _291 = _127 + _128;
  int32_t _292 = _291 - _p7_extent_2;
  int32_t _293 = _292 + 1;
  bool _294 = _293 <= _p7_min_2;
  bool _295 = _290 && _294;
  if (!_295)   {
   int32_t _296 = _127 + _128;
   int32_t _297 = _p7_min_2 + _p7_extent_2;
   int32_t _298 = _297 + -1;
   int32_t _299 = halide_error_access_out_of_bounds(NULL, "Input buffer p7", 2, _127, _296, _p7_min_2, _298);
   return _299;
  }
  bool _300 = _output_2_stride_0 == 1;
  if (!_300)   {
   int32_t _301 = halide_error_constraint_violated(NULL, "output$2.stride.0", _output_2_stride_0, "1", 1);
   return _301;
  }
  bool _302 = _p4_stride_0 == 1;
  if (!_302)   {
   int32_t _303 = halide_error_constraint_violated(NULL, "p4.stride.0", _p4_stride_0, "1", 1);
   return _303;
  }
  bool _304 = _p5_stride_0 == 1;
  if (!_304)   {
   int32_t _305 = halide_error_constraint_violated(NULL, "p5.stride.0", _p5_stride_0, "1", 1);
   return _305;
  }
  bool _306 = _p6_stride_0 == 1;
  if (!_306)   {
   int32_t _307 = halide_error_constraint_violated(NULL, "p6.stride.0", _p6_stride_0, "1", 1);
   return _307;
  }
  bool _308 = _p7_stride_0 == 1;
  if (!_308)   {
   int32_t _309 = halide_error_constraint_violated(NULL, "p7.stride.0", _p7_stride_0, "1", 1);
   return _309;
  }
  int64_t _310 = (int64_t)(_output_2_extent_1);
  int64_t _311 = (int64_t)(_output_2_extent_0);
  int64_t _312 = _310 * _311;
  int64_t _313 = (int64_t)(_p4_extent_1);
  int64_t _314 = (int64_t)(_p4_extent_0);
  int64_t _315 = _313 * _314;
  int64_t _316 = (int64_t)(_p5_extent_1);
  int64_t _317 = (int64_t)(_p5_extent_0);
  int64_t _318 = _316 * _317;
  int64_t _319 = (int64_t)(_p6_extent_1);
  int64_t _320 = (int64_t)(_p6_extent_0);
  int64_t _321 = _319 * _320;
  int64_t _322 = (int64_t)(_p7_extent_1);
  int64_t _323 = (int64_t)(_p7_extent_0);
  int64_t _324 = _322 * _323;
  int64_t _325 = (int64_t)(2147483647);
  bool _326 = _311 <= _325;
  if (!_326)   {
   int64_t _327 = (int64_t)(_output_2_extent_0);
   int64_t _328 = (int64_t)(2147483647);
   int32_t _329 = halide_error_buffer_allocation_too_large(NULL, "output$2", _327, _328);
   return _329;
  }
  int64_t _330 = (int64_t)(_output_2_extent_1);
  int64_t _331 = (int64_t)(_output_2_stride_1);
  int64_t _332 = _330 * _331;
  int64_t _333 = (int64_t)(2147483647);
  bool _334 = _332 <= _333;
  if (!_334)   {
   int64_t _335 = (int64_t)(_output_2_extent_1);
   int64_t _336 = (int64_t)(_output_2_stride_1);
   int64_t _337 = _335 * _336;
   int64_t _338 = (int64_t)(2147483647);
   int32_t _339 = halide_error_buffer_allocation_too_large(NULL, "output$2", _337, _338);
   return _339;
  }
  int64_t _340 = (int64_t)(2147483647);
  bool _341 = _312 <= _340;
  if (!_341)   {
   int64_t _342 = (int64_t)(2147483647);
   int32_t _343 = halide_error_buffer_extents_too_large(NULL, "output$2", _312, _342);
   return _343;
  }
  int64_t _344 = (int64_t)(_p4_extent_0);
  int64_t _345 = (int64_t)(2147483647);
  bool _346 = _344 <= _345;
  if (!_346)   {
   int64_t _347 = (int64_t)(_p4_extent_0);
   int64_t _348 = (int64_t)(2147483647);
   int32_t _349 = halide_error_buffer_allocation_too_large(NULL, "p4", _347, _348);
   return _349;
  }
  int64_t _350 = (int64_t)(_p4_extent_1);
  int64_t _351 = (int64_t)(_p4_stride_1);
  int64_t _352 = _350 * _351;
  int64_t _353 = (int64_t)(2147483647);
  bool _354 = _352 <= _353;
  if (!_354)   {
   int64_t _355 = (int64_t)(_p4_extent_1);
   int64_t _356 = (int64_t)(_p4_stride_1);
   int64_t _357 = _355 * _356;
   int64_t _358 = (int64_t)(2147483647);
   int32_t _359 = halide_error_buffer_allocation_too_large(NULL, "p4", _357, _358);
   return _359;
  }
  int64_t _360 = (int64_t)(2147483647);
  bool _361 = _315 <= _360;
  if (!_361)   {
   int64_t _362 = (int64_t)(2147483647);
   int32_t _363 = halide_error_buffer_extents_too_large(NULL, "p4", _315, _362);
   return _363;
  }
  int64_t _364 = (int64_t)(_p4_extent_2);
  int64_t _365 = (int64_t)(_p4_stride_2);
  int64_t _366 = _364 * _365;
  int64_t _367 = (int64_t)(2147483647);
  bool _368 = _366 <= _367;
  if (!_368)   {
   int64_t _369 = (int64_t)(_p4_extent_2);
   int64_t _370 = (int64_t)(_p4_stride_2);
   int64_t _371 = _369 * _370;
   int64_t _372 = (int64_t)(2147483647);
   int32_t _373 = halide_error_buffer_allocation_too_large(NULL, "p4", _371, _372);
   return _373;
  }
  int64_t _374 = (int64_t)(_p4_extent_2);
  int64_t _375 = _374 * _315;
  int64_t _376 = (int64_t)(2147483647);
  bool _377 = _375 <= _376;
  if (!_377)   {
   int64_t _378 = (int64_t)(_p4_extent_2);
   int64_t _379 = _378 * _315;
   int64_t _380 = (int64_t)(2147483647);
   int32_t _381 = halide_error_buffer_extents_too_large(NULL, "p4", _379, _380);
   return _381;
  }
  int64_t _382 = (int64_t)(_p5_extent_0);
  int64_t _383 = (int64_t)(2147483647);
  bool _384 = _382 <= _383;
  if (!_384)   {
   int64_t _385 = (int64_t)(_p5_extent_0);
   int64_t _386 = (int64_t)(2147483647);
   int32_t _387 = halide_error_buffer_allocation_too_large(NULL, "p5", _385, _386);
   return _387;
  }
  int64_t _388 = (int64_t)(_p5_extent_1);
  int64_t _389 = (int64_t)(_p5_stride_1);
  int64_t _390 = _388 * _389;
  int64_t _391 = (int64_t)(2147483647);
  bool _392 = _390 <= _391;
  if (!_392)   {
   int64_t _393 = (int64_t)(_p5_extent_1);
   int64_t _394 = (int64_t)(_p5_stride_1);
   int64_t _395 = _393 * _394;
   int64_t _396 = (int64_t)(2147483647);
   int32_t _397 = halide_error_buffer_allocation_too_large(NULL, "p5", _395, _396);
   return _397;
  }
  int64_t _398 = (int64_t)(2147483647);
  bool _399 = _318 <= _398;
  if (!_399)   {
   int64_t _400 = (int64_t)(2147483647);
   int32_t _401 = halide_error_buffer_extents_too_large(NULL, "p5", _318, _400);
   return _401;
  }
  int64_t _402 = (int64_t)(_p5_extent_2);
  int64_t _403 = (int64_t)(_p5_stride_2);
  int64_t _404 = _402 * _403;
  int64_t _405 = (int64_t)(2147483647);
  bool _406 = _404 <= _405;
  if (!_406)   {
   int64_t _407 = (int64_t)(_p5_extent_2);
   int64_t _408 = (int64_t)(_p5_stride_2);
   int64_t _409 = _407 * _408;
   int64_t _410 = (int64_t)(2147483647);
   int32_t _411 = halide_error_buffer_allocation_too_large(NULL, "p5", _409, _410);
   return _411;
  }
  int64_t _412 = (int64_t)(_p5_extent_2);
  int64_t _413 = _412 * _318;
  int64_t _414 = (int64_t)(2147483647);
  bool _415 = _413 <= _414;
  if (!_415)   {
   int64_t _416 = (int64_t)(_p5_extent_2);
   int64_t _417 = _416 * _318;
   int64_t _418 = (int64_t)(2147483647);
   int32_t _419 = halide_error_buffer_extents_too_large(NULL, "p5", _417, _418);
   return _419;
  }
  int64_t _420 = (int64_t)(_p6_extent_0);
  int64_t _421 = (int64_t)(2147483647);
  bool _422 = _420 <= _421;
  if (!_422)   {
   int64_t _423 = (int64_t)(_p6_extent_0);
   int64_t _424 = (int64_t)(2147483647);
   int32_t _425 = halide_error_buffer_allocation_too_large(NULL, "p6", _423, _424);
   return _425;
  }
  int64_t _426 = (int64_t)(_p6_extent_1);
  int64_t _427 = (int64_t)(_p6_stride_1);
  int64_t _428 = _426 * _427;
  int64_t _429 = (int64_t)(2147483647);
  bool _430 = _428 <= _429;
  if (!_430)   {
   int64_t _431 = (int64_t)(_p6_extent_1);
   int64_t _432 = (int64_t)(_p6_stride_1);
   int64_t _433 = _431 * _432;
   int64_t _434 = (int64_t)(2147483647);
   int32_t _435 = halide_error_buffer_allocation_too_large(NULL, "p6", _433, _434);
   return _435;
  }
  int64_t _436 = (int64_t)(2147483647);
  bool _437 = _321 <= _436;
  if (!_437)   {
   int64_t _438 = (int64_t)(2147483647);
   int32_t _439 = halide_error_buffer_extents_too_large(NULL, "p6", _321, _438);
   return _439;
  }
  int64_t _440 = (int64_t)(_p6_extent_2);
  int64_t _441 = (int64_t)(_p6_stride_2);
  int64_t _442 = _440 * _441;
  int64_t _443 = (int64_t)(2147483647);
  bool _444 = _442 <= _443;
  if (!_444)   {
   int64_t _445 = (int64_t)(_p6_extent_2);
   int64_t _446 = (int64_t)(_p6_stride_2);
   int64_t _447 = _445 * _446;
   int64_t _448 = (int64_t)(2147483647);
   int32_t _449 = halide_error_buffer_allocation_too_large(NULL, "p6", _447, _448);
   return _449;
  }
  int64_t _450 = (int64_t)(_p6_extent_2);
  int64_t _451 = _450 * _321;
  int64_t _452 = (int64_t)(2147483647);
  bool _453 = _451 <= _452;
  if (!_453)   {
   int64_t _454 = (int64_t)(_p6_extent_2);
   int64_t _455 = _454 * _321;
   int64_t _456 = (int64_t)(2147483647);
   int32_t _457 = halide_error_buffer_extents_too_large(NULL, "p6", _455, _456);
   return _457;
  }
  int64_t _458 = (int64_t)(_p7_extent_0);
  int64_t _459 = (int64_t)(2147483647);
  bool _460 = _458 <= _459;
  if (!_460)   {
   int64_t _461 = (int64_t)(_p7_extent_0);
   int64_t _462 = (int64_t)(2147483647);
   int32_t _463 = halide_error_buffer_allocation_too_large(NULL, "p7", _461, _462);
   return _463;
  }
  int64_t _464 = (int64_t)(_p7_extent_1);
  int64_t _465 = (int64_t)(_p7_stride_1);
  int64_t _466 = _464 * _465;
  int64_t _467 = (int64_t)(2147483647);
  bool _468 = _466 <= _467;
  if (!_468)   {
   int64_t _469 = (int64_t)(_p7_extent_1);
   int64_t _470 = (int64_t)(_p7_stride_1);
   int64_t _471 = _469 * _470;
   int64_t _472 = (int64_t)(2147483647);
   int32_t _473 = halide_error_buffer_allocation_too_large(NULL, "p7", _471, _472);
   return _473;
  }
  int64_t _474 = (int64_t)(2147483647);
  bool _475 = _324 <= _474;
  if (!_475)   {
   int64_t _476 = (int64_t)(2147483647);
   int32_t _477 = halide_error_buffer_extents_too_large(NULL, "p7", _324, _476);
   return _477;
  }
  int64_t _478 = (int64_t)(_p7_extent_2);
  int64_t _479 = (int64_t)(_p7_stride_2);
  int64_t _480 = _478 * _479;
  int64_t _481 = (int64_t)(2147483647);
  bool _482 = _480 <= _481;
  if (!_482)   {
   int64_t _483 = (int64_t)(_p7_extent_2);
   int64_t _484 = (int64_t)(_p7_stride_2);
   int64_t _485 = _483 * _484;
   int64_t _486 = (int64_t)(2147483647);
   int32_t _487 = halide_error_buffer_allocation_too_large(NULL, "p7", _485, _486);
   return _487;
  }
  int64_t _488 = (int64_t)(_p7_extent_2);
  int64_t _489 = _488 * _324;
  int64_t _490 = (int64_t)(2147483647);
  bool _491 = _489 <= _490;
  if (!_491)   {
   int64_t _492 = (int64_t)(_p7_extent_2);
   int64_t _493 = _492 * _324;
   int64_t _494 = (int64_t)(2147483647);
   int32_t _495 = halide_error_buffer_extents_too_large(NULL, "p7", _493, _494);
   return _495;
  }
  int32_t _496 = _output_2_extent_1 + -1;
  int32_t _497 = _496 >> 8;
  int32_t _498 = _497 * 256;
  int32_t _499 = _498 + _output_2_min_1;
  int32_t _500 = _499 + 255;
  int32_t _501 = _output_2_min_1 + _output_2_extent_1;
  int32_t _502 = _501 + -1;
  int32_t _503 = min(_500, _502);
  int32_t _504 = _503 + 11;
  int32_t _505 = _501 + 10;
  int32_t _506 = max(_504, _505);
  int32_t _507 = _output_2_extent_0 + -1;
  int32_t _508 = _507 >> 8;
  int32_t _509 = _508 * 256;
  int32_t _510 = _509 + _output_2_min_0;
  int32_t _511 = _510 + 255;
  int32_t _512 = _output_2_min_0 + _output_2_extent_0;
  int32_t _513 = _512 + -1;
  int32_t _514 = min(_511, _513);
  int32_t _515 = _514 + 11;
  int32_t _516 = _512 + 10;
  int32_t _517 = max(_515, _516);
  {
   int32_t _518 = _517 - _17;
   int32_t _519 = _518 + 13;
   int64_t _520 = _519;
   int32_t _521 = _506 - _28;
   int32_t _522 = _521 + 13;
   int64_t _523 = _520 * _522;
   int64_t _524 = (_523 > ((int64_t(1) << 31) - 1)) ? _523 : (_523 * 1);
   if ((_524 > ((int64_t(1) << 31) - 1)) || ((_524 * sizeof(uint8_t)) > ((int64_t(1) << 31) - 1)))
   {
    halide_error(NULL, "32-bit signed overflow computing size of allocation constant_exterior$5\n");
    return -1;
   } // overflow test constant_exterior$5
   int64_t _525 = _524;
   uint8_t *_constant_exterior_5 = (uint8_t *)halide_malloc(NULL, sizeof(uint8_t)*_525);
   // produce constant_exterior$5
   int32_t _526 = _output_2_min_1 + -12;
   int32_t _527 = max(_p5_min_1, _526);
   int32_t _528 = _output_2_extent_1 + _output_2_min_1;
   int32_t _529 = _528 + 11;
   int32_t _530 = min(_527, _529);
   int32_t _531 = _p5_min_1 + _p5_extent_1;
   int32_t _532 = _output_2_min_1 + _output_2_extent_1;
   int32_t _533 = _532 + 11;
   int32_t _534 = min(_531, _533);
   int32_t _535 = max(_534, _530);
   int32_t _536 = _530 - _output_2_min_1;
   int32_t _537 = _536 + 12;
   for (int _constant_exterior_5_s0__1 = _526; _constant_exterior_5_s0__1 < _526 + _537; _constant_exterior_5_s0__1++)
   {
    int32_t _538 = _output_2_min_0 + -12;
    int32_t _539 = _output_2_extent_0 + 23;
    for (int _constant_exterior_5_s0__0 = _538; _constant_exterior_5_s0__0 < _538 + _539; _constant_exterior_5_s0__0++)
    {
     int32_t _540 = _constant_exterior_5_s0__0 - _17;
     int32_t _541 = _constant_exterior_5_s0__1 - _28;
     int32_t _542 = _541 + 12;
     int32_t _543 = _517 - _17;
     int32_t _544 = _543 + 13;
     int32_t _545 = _542 * _544;
     int32_t _546 = _540 + _545;
     int32_t _547 = _546 + 12;
     uint8_t _548 = (uint8_t)(0);
     int32_t _549 = _p5_min_0 + _p5_extent_0;
     int32_t _550 = _549 + -1;
     int32_t _551 = min(_constant_exterior_5_s0__0, _550);
     int32_t _552 = max(_551, _p5_min_0);
     int32_t _553 = _p5_min_1 + _p5_extent_1;
     int32_t _554 = _553 + -1;
     int32_t _555 = min(_constant_exterior_5_s0__1, _554);
     int32_t _556 = max(_555, _p5_min_1);
     int32_t _557 = _556 * _p5_stride_1;
     int32_t _558 = _552 + _557;
     int32_t _559 = _p5_min_2 + _p5_extent_2;
     int32_t _560 = _559 + -1;
     int32_t _561 = min(_560, 1);
     int32_t _562 = max(_561, _p5_min_2);
     int32_t _563 = _562 * _p5_stride_2;
     int32_t _564 = _558 + _563;
     int32_t _565 = _p5_min_1 * _p5_stride_1;
     int32_t _566 = _p5_min_0 + _565;
     int32_t _567 = _p5_min_2 * _p5_stride_2;
     int32_t _568 = _566 + _567;
     int32_t _569 = _564 - _568;
     uint8_t _570 = _p5[_569];
     bool _571 = _constant_exterior_5_s0__0 < _p5_min_0;
     bool _572 = _549 <= _constant_exterior_5_s0__0;
     bool _573 = _571 || _572;
     bool _574 = _constant_exterior_5_s0__1 < _p5_min_1;
     bool _575 = _573 || _574;
     bool _576 = _553 <= _constant_exterior_5_s0__1;
     bool _577 = _575 || _576;
     bool _578 = 1 < _p5_min_2;
     bool _579 = _577 || _578;
     bool _580 = _559 <= 1;
     bool _581 = _579 || _580;
     uint8_t _582 = (uint8_t)(_581 ? _548 : _570);
     _constant_exterior_5[_547] = _582;
    } // for _constant_exterior_5_s0__0
   } // for _constant_exterior_5_s0__1
   int32_t _583 = _535 - _530;
   for (int _constant_exterior_5_s0__1 = _530; _constant_exterior_5_s0__1 < _530 + _583; _constant_exterior_5_s0__1++)
   {
    int32_t _584 = _output_2_min_0 + -12;
    int32_t _585 = max(_p5_min_0, _584);
    int32_t _586 = _output_2_extent_0 + _output_2_min_0;
    int32_t _587 = _586 + 11;
    int32_t _588 = min(_585, _587);
    int32_t _589 = _p5_min_0 + _p5_extent_0;
    int32_t _590 = _output_2_min_0 + _output_2_extent_0;
    int32_t _591 = _590 + 11;
    int32_t _592 = min(_589, _591);
    int32_t _593 = max(_592, _588);
    int32_t _594 = _588 - _output_2_min_0;
    int32_t _595 = _594 + 12;
    for (int _constant_exterior_5_s0__0 = _584; _constant_exterior_5_s0__0 < _584 + _595; _constant_exterior_5_s0__0++)
    {
     int32_t _596 = _constant_exterior_5_s0__0 - _17;
     int32_t _597 = _constant_exterior_5_s0__1 - _28;
     int32_t _598 = _597 + 12;
     int32_t _599 = _517 - _17;
     int32_t _600 = _599 + 13;
     int32_t _601 = _598 * _600;
     int32_t _602 = _596 + _601;
     int32_t _603 = _602 + 12;
     uint8_t _604 = (uint8_t)(0);
     int32_t _605 = _p5_min_0 + _p5_extent_0;
     int32_t _606 = _605 + -1;
     int32_t _607 = min(_constant_exterior_5_s0__0, _606);
     int32_t _608 = max(_607, _p5_min_0);
     int32_t _609 = _constant_exterior_5_s0__1 * _p5_stride_1;
     int32_t _610 = _608 + _609;
     int32_t _611 = _p5_min_2 + _p5_extent_2;
     int32_t _612 = _611 + -1;
     int32_t _613 = min(_612, 1);
     int32_t _614 = max(_613, _p5_min_2);
     int32_t _615 = _614 * _p5_stride_2;
     int32_t _616 = _610 + _615;
     int32_t _617 = _p5_min_1 * _p5_stride_1;
     int32_t _618 = _p5_min_0 + _617;
     int32_t _619 = _p5_min_2 * _p5_stride_2;
     int32_t _620 = _618 + _619;
     int32_t _621 = _616 - _620;
     uint8_t _622 = _p5[_621];
     bool _623 = _constant_exterior_5_s0__0 < _p5_min_0;
     bool _624 = _605 <= _constant_exterior_5_s0__0;
     bool _625 = _623 || _624;
     bool _626 = 1 < _p5_min_2;
     bool _627 = _625 || _626;
     bool _628 = _611 <= 1;
     bool _629 = _627 || _628;
     uint8_t _630 = (uint8_t)(_629 ? _604 : _622);
     _constant_exterior_5[_603] = _630;
    } // for _constant_exterior_5_s0__0
    int32_t _631 = _593 - _588;
    for (int _constant_exterior_5_s0__0 = _588; _constant_exterior_5_s0__0 < _588 + _631; _constant_exterior_5_s0__0++)
    {
     int32_t _632 = _constant_exterior_5_s0__0 - _17;
     int32_t _633 = _constant_exterior_5_s0__1 - _28;
     int32_t _634 = _633 + 12;
     int32_t _635 = _517 - _17;
     int32_t _636 = _635 + 13;
     int32_t _637 = _634 * _636;
     int32_t _638 = _632 + _637;
     int32_t _639 = _638 + 12;
     uint8_t _640 = (uint8_t)(0);
     int32_t _641 = _constant_exterior_5_s0__1 * _p5_stride_1;
     int32_t _642 = _constant_exterior_5_s0__0 + _641;
     int32_t _643 = _p5_min_2 + _p5_extent_2;
     int32_t _644 = _643 + -1;
     int32_t _645 = min(_644, 1);
     int32_t _646 = max(_645, _p5_min_2);
     int32_t _647 = _646 * _p5_stride_2;
     int32_t _648 = _642 + _647;
     int32_t _649 = _p5_min_1 * _p5_stride_1;
     int32_t _650 = _p5_min_0 + _649;
     int32_t _651 = _p5_min_2 * _p5_stride_2;
     int32_t _652 = _650 + _651;
     int32_t _653 = _648 - _652;
     uint8_t _654 = _p5[_653];
     bool _655 = 1 < _p5_min_2;
     bool _656 = _643 <= 1;
     bool _657 = _655 || _656;
     uint8_t _658 = (uint8_t)(_657 ? _640 : _654);
     _constant_exterior_5[_639] = _658;
    } // for _constant_exterior_5_s0__0
    int32_t _659 = _output_2_min_0 + _output_2_extent_0;
    int32_t _660 = _659 - _593;
    int32_t _661 = _660 + 11;
    for (int _constant_exterior_5_s0__0 = _593; _constant_exterior_5_s0__0 < _593 + _661; _constant_exterior_5_s0__0++)
    {
     int32_t _662 = _constant_exterior_5_s0__0 - _17;
     int32_t _663 = _constant_exterior_5_s0__1 - _28;
     int32_t _664 = _663 + 12;
     int32_t _665 = _517 - _17;
     int32_t _666 = _665 + 13;
     int32_t _667 = _664 * _666;
     int32_t _668 = _662 + _667;
     int32_t _669 = _668 + 12;
     uint8_t _670 = (uint8_t)(0);
     int32_t _671 = _p5_min_0 + _p5_extent_0;
     int32_t _672 = _671 + -1;
     int32_t _673 = min(_constant_exterior_5_s0__0, _672);
     int32_t _674 = max(_673, _p5_min_0);
     int32_t _675 = _constant_exterior_5_s0__1 * _p5_stride_1;
     int32_t _676 = _674 + _675;
     int32_t _677 = _p5_min_2 + _p5_extent_2;
     int32_t _678 = _677 + -1;
     int32_t _679 = min(_678, 1);
     int32_t _680 = max(_679, _p5_min_2);
     int32_t _681 = _680 * _p5_stride_2;
     int32_t _682 = _676 + _681;
     int32_t _683 = _p5_min_1 * _p5_stride_1;
     int32_t _684 = _p5_min_0 + _683;
     int32_t _685 = _p5_min_2 * _p5_stride_2;
     int32_t _686 = _684 + _685;
     int32_t _687 = _682 - _686;
     uint8_t _688 = _p5[_687];
     bool _689 = _671 <= _constant_exterior_5_s0__0;
     bool _690 = 1 < _p5_min_2;
     bool _691 = _689 || _690;
     bool _692 = _677 <= 1;
     bool _693 = _691 || _692;
     uint8_t _694 = (uint8_t)(_693 ? _670 : _688);
     _constant_exterior_5[_669] = _694;
    } // for _constant_exterior_5_s0__0
   } // for _constant_exterior_5_s0__1
   int32_t _695 = _output_2_min_1 + _output_2_extent_1;
   int32_t _696 = _695 - _535;
   int32_t _697 = _696 + 11;
   for (int _constant_exterior_5_s0__1 = _535; _constant_exterior_5_s0__1 < _535 + _697; _constant_exterior_5_s0__1++)
   {
    int32_t _698 = _output_2_min_0 + -12;
    int32_t _699 = _output_2_extent_0 + 23;
    for (int _constant_exterior_5_s0__0 = _698; _constant_exterior_5_s0__0 < _698 + _699; _constant_exterior_5_s0__0++)
    {
     int32_t _700 = _constant_exterior_5_s0__0 - _17;
     int32_t _701 = _constant_exterior_5_s0__1 - _28;
     int32_t _702 = _701 + 12;
     int32_t _703 = _517 - _17;
     int32_t _704 = _703 + 13;
     int32_t _705 = _702 * _704;
     int32_t _706 = _700 + _705;
     int32_t _707 = _706 + 12;
     uint8_t _708 = (uint8_t)(0);
     int32_t _709 = _p5_min_0 + _p5_extent_0;
     int32_t _710 = _709 + -1;
     int32_t _711 = min(_constant_exterior_5_s0__0, _710);
     int32_t _712 = max(_711, _p5_min_0);
     int32_t _713 = _p5_min_1 + _p5_extent_1;
     int32_t _714 = _713 + -1;
     int32_t _715 = min(_constant_exterior_5_s0__1, _714);
     int32_t _716 = max(_715, _p5_min_1);
     int32_t _717 = _716 * _p5_stride_1;
     int32_t _718 = _712 + _717;
     int32_t _719 = _p5_min_2 + _p5_extent_2;
     int32_t _720 = _719 + -1;
     int32_t _721 = min(_720, 1);
     int32_t _722 = max(_721, _p5_min_2);
     int32_t _723 = _722 * _p5_stride_2;
     int32_t _724 = _718 + _723;
     int32_t _725 = _p5_min_1 * _p5_stride_1;
     int32_t _726 = _p5_min_0 + _725;
     int32_t _727 = _p5_min_2 * _p5_stride_2;
     int32_t _728 = _726 + _727;
     int32_t _729 = _724 - _728;
     uint8_t _730 = _p5[_729];
     bool _731 = _constant_exterior_5_s0__0 < _p5_min_0;
     bool _732 = _709 <= _constant_exterior_5_s0__0;
     bool _733 = _731 || _732;
     bool _734 = _713 <= _constant_exterior_5_s0__1;
     bool _735 = _733 || _734;
     bool _736 = 1 < _p5_min_2;
     bool _737 = _735 || _736;
     bool _738 = _719 <= 1;
     bool _739 = _737 || _738;
     uint8_t _740 = (uint8_t)(_739 ? _708 : _730);
     _constant_exterior_5[_707] = _740;
    } // for _constant_exterior_5_s0__0
   } // for _constant_exterior_5_s0__1
   // consume constant_exterior$5
   int32_t _741 = _output_2_extent_1 + -1;
   int32_t _742 = _741 >> 8;
   int32_t _743 = _742 * 256;
   int32_t _744 = _743 + _output_2_min_1;
   int32_t _745 = _744 + 255;
   int32_t _746 = _output_2_min_1 + _output_2_extent_1;
   int32_t _747 = _746 + -1;
   int32_t _748 = min(_745, _747);
   int32_t _749 = _748 + 3;
   int32_t _750 = _746 + 2;
   int32_t _751 = max(_749, _750);
   int32_t _752 = _output_2_extent_0 + -1;
   int32_t _753 = _752 >> 8;
   int32_t _754 = _753 * 256;
   int32_t _755 = _754 + _output_2_min_0;
   int32_t _756 = _755 + 255;
   int32_t _757 = _output_2_min_0 + _output_2_extent_0;
   int32_t _758 = _757 + -1;
   int32_t _759 = min(_756, _758);
   int32_t _760 = _759 + 3;
   int32_t _761 = _757 + 2;
   int32_t _762 = max(_760, _761);
   int32_t _763 = _762 - _17;
   int32_t _764 = _763 + 5;
   int32_t _765 = _751 - _28;
   int32_t _766 = _765 + 5;
   int32_t _767 = _764 * _766;
   {
    int32_t _768 = _762 - _17;
    int32_t _769 = _768 + 5;
    int64_t _770 = _769;
    int32_t _771 = _751 - _28;
    int32_t _772 = _771 + 5;
    int64_t _773 = _770 * _772;
    int64_t _774 = (_773 > ((int64_t(1) << 31) - 1)) ? _773 : (_773 * 2);
    if ((_774 > ((int64_t(1) << 31) - 1)) || ((_774 * sizeof(uint8_t)) > ((int64_t(1) << 31) - 1)))
    {
     halide_error(NULL, "32-bit signed overflow computing size of allocation constant_exterior$7\n");
     return -1;
    } // overflow test constant_exterior$7
    int64_t _775 = _774;
    uint8_t *_constant_exterior_7 = (uint8_t *)halide_malloc(NULL, sizeof(uint8_t)*_775);
    // produce constant_exterior$7
    int32_t _776 = max(_p7_min_2, 0);
    int32_t _777 = min(_776, 2);
    int32_t _778 = _p7_min_2 + _p7_extent_2;
    int32_t _779 = max(_778, _776);
    int32_t _780 = min(_779, 2);
    for (int _constant_exterior_7_s0__2 = 0; _constant_exterior_7_s0__2 < 0 + _777; _constant_exterior_7_s0__2++)
    {
     int32_t _781 = _output_2_min_1 + -4;
     int32_t _782 = _output_2_extent_1 + 7;
     for (int _constant_exterior_7_s0__1 = _781; _constant_exterior_7_s0__1 < _781 + _782; _constant_exterior_7_s0__1++)
     {
      int32_t _783 = _output_2_min_0 + -4;
      int32_t _784 = _output_2_extent_0 + 7;
      for (int _constant_exterior_7_s0__0 = _783; _constant_exterior_7_s0__0 < _783 + _784; _constant_exterior_7_s0__0++)
      {
       int32_t _785 = _constant_exterior_7_s0__0 - _17;
       int32_t _786 = _constant_exterior_7_s0__1 - _28;
       int32_t _787 = _786 + 4;
       int32_t _788 = _762 - _17;
       int32_t _789 = _788 + 5;
       int32_t _790 = _787 * _789;
       int32_t _791 = _785 + _790;
       int32_t _792 = _constant_exterior_7_s0__2 * _767;
       int32_t _793 = _791 + _792;
       int32_t _794 = _793 + 4;
       uint8_t _795 = (uint8_t)(128);
       int32_t _796 = _p7_min_0 + _p7_extent_0;
       int32_t _797 = _796 + -1;
       int32_t _798 = min(_constant_exterior_7_s0__0, _797);
       int32_t _799 = max(_798, _p7_min_0);
       int32_t _800 = _p7_min_1 + _p7_extent_1;
       int32_t _801 = _800 + -1;
       int32_t _802 = min(_constant_exterior_7_s0__1, _801);
       int32_t _803 = max(_802, _p7_min_1);
       int32_t _804 = _803 * _p7_stride_1;
       int32_t _805 = _799 + _804;
       int32_t _806 = _p7_min_2 + _p7_extent_2;
       int32_t _807 = _806 + -1;
       int32_t _808 = min(_constant_exterior_7_s0__2, _807);
       int32_t _809 = max(_808, _p7_min_2);
       int32_t _810 = _809 * _p7_stride_2;
       int32_t _811 = _805 + _810;
       int32_t _812 = _p7_min_1 * _p7_stride_1;
       int32_t _813 = _p7_min_0 + _812;
       int32_t _814 = _p7_min_2 * _p7_stride_2;
       int32_t _815 = _813 + _814;
       int32_t _816 = _811 - _815;
       uint8_t _817 = _p7[_816];
       bool _818 = _constant_exterior_7_s0__0 < _p7_min_0;
       bool _819 = _796 <= _constant_exterior_7_s0__0;
       bool _820 = _818 || _819;
       bool _821 = _constant_exterior_7_s0__1 < _p7_min_1;
       bool _822 = _820 || _821;
       bool _823 = _800 <= _constant_exterior_7_s0__1;
       bool _824 = _822 || _823;
       bool _825 = _constant_exterior_7_s0__2 < _p7_min_2;
       bool _826 = _824 || _825;
       bool _827 = _806 <= _constant_exterior_7_s0__2;
       bool _828 = _826 || _827;
       uint8_t _829 = (uint8_t)(_828 ? _795 : _817);
       _constant_exterior_7[_794] = _829;
      } // for _constant_exterior_7_s0__0
     } // for _constant_exterior_7_s0__1
    } // for _constant_exterior_7_s0__2
    int32_t _830 = _780 - _777;
    for (int _constant_exterior_7_s0__2 = _777; _constant_exterior_7_s0__2 < _777 + _830; _constant_exterior_7_s0__2++)
    {
     int32_t _831 = _output_2_min_1 + -4;
     int32_t _832 = max(_p7_min_1, _831);
     int32_t _833 = _output_2_extent_1 + _output_2_min_1;
     int32_t _834 = _833 + 3;
     int32_t _835 = min(_832, _834);
     int32_t _836 = _p7_min_1 + _p7_extent_1;
     int32_t _837 = _output_2_min_1 + _output_2_extent_1;
     int32_t _838 = _837 + 3;
     int32_t _839 = min(_836, _838);
     int32_t _840 = max(_839, _835);
     int32_t _841 = _835 - _output_2_min_1;
     int32_t _842 = _841 + 4;
     for (int _constant_exterior_7_s0__1 = _831; _constant_exterior_7_s0__1 < _831 + _842; _constant_exterior_7_s0__1++)
     {
      int32_t _843 = _output_2_min_0 + -4;
      int32_t _844 = _output_2_extent_0 + 7;
      for (int _constant_exterior_7_s0__0 = _843; _constant_exterior_7_s0__0 < _843 + _844; _constant_exterior_7_s0__0++)
      {
       int32_t _845 = _constant_exterior_7_s0__0 - _17;
       int32_t _846 = _constant_exterior_7_s0__1 - _28;
       int32_t _847 = _846 + 4;
       int32_t _848 = _762 - _17;
       int32_t _849 = _848 + 5;
       int32_t _850 = _847 * _849;
       int32_t _851 = _845 + _850;
       int32_t _852 = _constant_exterior_7_s0__2 * _767;
       int32_t _853 = _851 + _852;
       int32_t _854 = _853 + 4;
       uint8_t _855 = (uint8_t)(128);
       int32_t _856 = _p7_min_0 + _p7_extent_0;
       int32_t _857 = _856 + -1;
       int32_t _858 = min(_constant_exterior_7_s0__0, _857);
       int32_t _859 = max(_858, _p7_min_0);
       int32_t _860 = _p7_min_1 + _p7_extent_1;
       int32_t _861 = _860 + -1;
       int32_t _862 = min(_constant_exterior_7_s0__1, _861);
       int32_t _863 = max(_862, _p7_min_1);
       int32_t _864 = _863 * _p7_stride_1;
       int32_t _865 = _859 + _864;
       int32_t _866 = _constant_exterior_7_s0__2 * _p7_stride_2;
       int32_t _867 = _865 + _866;
       int32_t _868 = _p7_min_1 * _p7_stride_1;
       int32_t _869 = _p7_min_0 + _868;
       int32_t _870 = _p7_min_2 * _p7_stride_2;
       int32_t _871 = _869 + _870;
       int32_t _872 = _867 - _871;
       uint8_t _873 = _p7[_872];
       bool _874 = _constant_exterior_7_s0__0 < _p7_min_0;
       bool _875 = _856 <= _constant_exterior_7_s0__0;
       bool _876 = _874 || _875;
       bool _877 = _constant_exterior_7_s0__1 < _p7_min_1;
       bool _878 = _876 || _877;
       bool _879 = _860 <= _constant_exterior_7_s0__1;
       bool _880 = _878 || _879;
       uint8_t _881 = (uint8_t)(_880 ? _855 : _873);
       _constant_exterior_7[_854] = _881;
      } // for _constant_exterior_7_s0__0
     } // for _constant_exterior_7_s0__1
     int32_t _882 = _840 - _835;
     for (int _constant_exterior_7_s0__1 = _835; _constant_exterior_7_s0__1 < _835 + _882; _constant_exterior_7_s0__1++)
     {
      int32_t _883 = _output_2_min_0 + -4;
      int32_t _884 = max(_p7_min_0, _883);
      int32_t _885 = _output_2_extent_0 + _output_2_min_0;
      int32_t _886 = _885 + 3;
      int32_t _887 = min(_884, _886);
      int32_t _888 = _p7_min_0 + _p7_extent_0;
      int32_t _889 = _output_2_min_0 + _output_2_extent_0;
      int32_t _890 = _889 + 3;
      int32_t _891 = min(_888, _890);
      int32_t _892 = max(_891, _887);
      int32_t _893 = _887 - _output_2_min_0;
      int32_t _894 = _893 + 4;
      for (int _constant_exterior_7_s0__0 = _883; _constant_exterior_7_s0__0 < _883 + _894; _constant_exterior_7_s0__0++)
      {
       int32_t _895 = _constant_exterior_7_s0__0 - _17;
       int32_t _896 = _constant_exterior_7_s0__1 - _28;
       int32_t _897 = _896 + 4;
       int32_t _898 = _762 - _17;
       int32_t _899 = _898 + 5;
       int32_t _900 = _897 * _899;
       int32_t _901 = _895 + _900;
       int32_t _902 = _constant_exterior_7_s0__2 * _767;
       int32_t _903 = _901 + _902;
       int32_t _904 = _903 + 4;
       uint8_t _905 = (uint8_t)(128);
       int32_t _906 = _p7_min_0 + _p7_extent_0;
       int32_t _907 = _906 + -1;
       int32_t _908 = min(_constant_exterior_7_s0__0, _907);
       int32_t _909 = max(_908, _p7_min_0);
       int32_t _910 = _constant_exterior_7_s0__1 * _p7_stride_1;
       int32_t _911 = _909 + _910;
       int32_t _912 = _constant_exterior_7_s0__2 * _p7_stride_2;
       int32_t _913 = _911 + _912;
       int32_t _914 = _p7_min_1 * _p7_stride_1;
       int32_t _915 = _p7_min_0 + _914;
       int32_t _916 = _p7_min_2 * _p7_stride_2;
       int32_t _917 = _915 + _916;
       int32_t _918 = _913 - _917;
       uint8_t _919 = _p7[_918];
       bool _920 = _constant_exterior_7_s0__0 < _p7_min_0;
       bool _921 = _906 <= _constant_exterior_7_s0__0;
       bool _922 = _920 || _921;
       uint8_t _923 = (uint8_t)(_922 ? _905 : _919);
       _constant_exterior_7[_904] = _923;
      } // for _constant_exterior_7_s0__0
      int32_t _924 = _892 - _887;
      for (int _constant_exterior_7_s0__0 = _887; _constant_exterior_7_s0__0 < _887 + _924; _constant_exterior_7_s0__0++)
      {
       int32_t _925 = _constant_exterior_7_s0__0 - _17;
       int32_t _926 = _constant_exterior_7_s0__1 - _28;
       int32_t _927 = _926 + 4;
       int32_t _928 = _762 - _17;
       int32_t _929 = _928 + 5;
       int32_t _930 = _927 * _929;
       int32_t _931 = _925 + _930;
       int32_t _932 = _constant_exterior_7_s0__2 * _767;
       int32_t _933 = _931 + _932;
       int32_t _934 = _933 + 4;
       int32_t _935 = _constant_exterior_7_s0__1 * _p7_stride_1;
       int32_t _936 = _constant_exterior_7_s0__0 + _935;
       int32_t _937 = _constant_exterior_7_s0__2 * _p7_stride_2;
       int32_t _938 = _936 + _937;
       int32_t _939 = _p7_min_1 * _p7_stride_1;
       int32_t _940 = _p7_min_0 + _939;
       int32_t _941 = _p7_min_2 * _p7_stride_2;
       int32_t _942 = _940 + _941;
       int32_t _943 = _938 - _942;
       uint8_t _944 = _p7[_943];
       _constant_exterior_7[_934] = _944;
      } // for _constant_exterior_7_s0__0
      int32_t _945 = _output_2_min_0 + _output_2_extent_0;
      int32_t _946 = _945 - _892;
      int32_t _947 = _946 + 3;
      for (int _constant_exterior_7_s0__0 = _892; _constant_exterior_7_s0__0 < _892 + _947; _constant_exterior_7_s0__0++)
      {
       int32_t _948 = _constant_exterior_7_s0__0 - _17;
       int32_t _949 = _constant_exterior_7_s0__1 - _28;
       int32_t _950 = _949 + 4;
       int32_t _951 = _762 - _17;
       int32_t _952 = _951 + 5;
       int32_t _953 = _950 * _952;
       int32_t _954 = _948 + _953;
       int32_t _955 = _constant_exterior_7_s0__2 * _767;
       int32_t _956 = _954 + _955;
       int32_t _957 = _956 + 4;
       int32_t _958 = _p7_min_0 + _p7_extent_0;
       int32_t _959 = _958 + -1;
       int32_t _960 = min(_constant_exterior_7_s0__0, _959);
       int32_t _961 = max(_960, _p7_min_0);
       int32_t _962 = _constant_exterior_7_s0__1 * _p7_stride_1;
       int32_t _963 = _961 + _962;
       int32_t _964 = _constant_exterior_7_s0__2 * _p7_stride_2;
       int32_t _965 = _963 + _964;
       int32_t _966 = _p7_min_1 * _p7_stride_1;
       int32_t _967 = _p7_min_0 + _966;
       int32_t _968 = _p7_min_2 * _p7_stride_2;
       int32_t _969 = _967 + _968;
       int32_t _970 = _965 - _969;
       uint8_t _971 = _p7[_970];
       uint8_t _972 = (uint8_t)(128);
       bool _973 = _constant_exterior_7_s0__0 < _958;
       uint8_t _974 = (uint8_t)(_973 ? _971 : _972);
       _constant_exterior_7[_957] = _974;
      } // for _constant_exterior_7_s0__0
     } // for _constant_exterior_7_s0__1
     int32_t _975 = _output_2_min_1 + _output_2_extent_1;
     int32_t _976 = _975 - _840;
     int32_t _977 = _976 + 3;
     for (int _constant_exterior_7_s0__1 = _840; _constant_exterior_7_s0__1 < _840 + _977; _constant_exterior_7_s0__1++)
     {
      int32_t _978 = _output_2_min_0 + -4;
      int32_t _979 = _output_2_extent_0 + 7;
      for (int _constant_exterior_7_s0__0 = _978; _constant_exterior_7_s0__0 < _978 + _979; _constant_exterior_7_s0__0++)
      {
       int32_t _980 = _constant_exterior_7_s0__0 - _17;
       int32_t _981 = _constant_exterior_7_s0__1 - _28;
       int32_t _982 = _981 + 4;
       int32_t _983 = _762 - _17;
       int32_t _984 = _983 + 5;
       int32_t _985 = _982 * _984;
       int32_t _986 = _980 + _985;
       int32_t _987 = _constant_exterior_7_s0__2 * _767;
       int32_t _988 = _986 + _987;
       int32_t _989 = _988 + 4;
       uint8_t _990 = (uint8_t)(128);
       int32_t _991 = _p7_min_0 + _p7_extent_0;
       int32_t _992 = _991 + -1;
       int32_t _993 = min(_constant_exterior_7_s0__0, _992);
       int32_t _994 = max(_993, _p7_min_0);
       int32_t _995 = _p7_min_1 + _p7_extent_1;
       int32_t _996 = _995 + -1;
       int32_t _997 = min(_constant_exterior_7_s0__1, _996);
       int32_t _998 = max(_997, _p7_min_1);
       int32_t _999 = _998 * _p7_stride_1;
       int32_t _1000 = _994 + _999;
       int32_t _1001 = _constant_exterior_7_s0__2 * _p7_stride_2;
       int32_t _1002 = _1000 + _1001;
       int32_t _1003 = _p7_min_1 * _p7_stride_1;
       int32_t _1004 = _p7_min_0 + _1003;
       int32_t _1005 = _p7_min_2 * _p7_stride_2;
       int32_t _1006 = _1004 + _1005;
       int32_t _1007 = _1002 - _1006;
       uint8_t _1008 = _p7[_1007];
       bool _1009 = _constant_exterior_7_s0__0 < _p7_min_0;
       bool _1010 = _991 <= _constant_exterior_7_s0__0;
       bool _1011 = _1009 || _1010;
       bool _1012 = _995 <= _constant_exterior_7_s0__1;
       bool _1013 = _1011 || _1012;
       uint8_t _1014 = (uint8_t)(_1013 ? _990 : _1008);
       _constant_exterior_7[_989] = _1014;
      } // for _constant_exterior_7_s0__0
     } // for _constant_exterior_7_s0__1
    } // for _constant_exterior_7_s0__2
    int32_t _1015 = 2 - _780;
    for (int _constant_exterior_7_s0__2 = _780; _constant_exterior_7_s0__2 < _780 + _1015; _constant_exterior_7_s0__2++)
    {
     int32_t _1016 = _output_2_min_1 + -4;
     int32_t _1017 = _output_2_extent_1 + 7;
     for (int _constant_exterior_7_s0__1 = _1016; _constant_exterior_7_s0__1 < _1016 + _1017; _constant_exterior_7_s0__1++)
     {
      int32_t _1018 = _output_2_min_0 + -4;
      int32_t _1019 = _output_2_extent_0 + 7;
      for (int _constant_exterior_7_s0__0 = _1018; _constant_exterior_7_s0__0 < _1018 + _1019; _constant_exterior_7_s0__0++)
      {
       int32_t _1020 = _constant_exterior_7_s0__0 - _17;
       int32_t _1021 = _constant_exterior_7_s0__1 - _28;
       int32_t _1022 = _1021 + 4;
       int32_t _1023 = _762 - _17;
       int32_t _1024 = _1023 + 5;
       int32_t _1025 = _1022 * _1024;
       int32_t _1026 = _1020 + _1025;
       int32_t _1027 = _constant_exterior_7_s0__2 * _767;
       int32_t _1028 = _1026 + _1027;
       int32_t _1029 = _1028 + 4;
       uint8_t _1030 = (uint8_t)(128);
       int32_t _1031 = _p7_min_0 + _p7_extent_0;
       int32_t _1032 = _1031 + -1;
       int32_t _1033 = min(_constant_exterior_7_s0__0, _1032);
       int32_t _1034 = max(_1033, _p7_min_0);
       int32_t _1035 = _p7_min_1 + _p7_extent_1;
       int32_t _1036 = _1035 + -1;
       int32_t _1037 = min(_constant_exterior_7_s0__1, _1036);
       int32_t _1038 = max(_1037, _p7_min_1);
       int32_t _1039 = _1038 * _p7_stride_1;
       int32_t _1040 = _1034 + _1039;
       int32_t _1041 = _p7_min_2 + _p7_extent_2;
       int32_t _1042 = _1041 + -1;
       int32_t _1043 = min(_constant_exterior_7_s0__2, _1042);
       int32_t _1044 = max(_1043, _p7_min_2);
       int32_t _1045 = _1044 * _p7_stride_2;
       int32_t _1046 = _1040 + _1045;
       int32_t _1047 = _p7_min_1 * _p7_stride_1;
       int32_t _1048 = _p7_min_0 + _1047;
       int32_t _1049 = _p7_min_2 * _p7_stride_2;
       int32_t _1050 = _1048 + _1049;
       int32_t _1051 = _1046 - _1050;
       uint8_t _1052 = _p7[_1051];
       bool _1053 = _constant_exterior_7_s0__0 < _p7_min_0;
       bool _1054 = _1031 <= _constant_exterior_7_s0__0;
       bool _1055 = _1053 || _1054;
       bool _1056 = _constant_exterior_7_s0__1 < _p7_min_1;
       bool _1057 = _1055 || _1056;
       bool _1058 = _1035 <= _constant_exterior_7_s0__1;
       bool _1059 = _1057 || _1058;
       bool _1060 = _1041 <= _constant_exterior_7_s0__2;
       bool _1061 = _1059 || _1060;
       uint8_t _1062 = (uint8_t)(_1061 ? _1030 : _1052);
       _constant_exterior_7[_1029] = _1062;
      } // for _constant_exterior_7_s0__0
     } // for _constant_exterior_7_s0__1
    } // for _constant_exterior_7_s0__2
    // consume constant_exterior$7
    int32_t _1063 = _output_2_extent_0 + -1;
    int32_t _1064 = _1063 >> 8;
    int32_t _1065 = _1064 * 256;
    int32_t _1066 = _1065 + _output_2_min_0;
    int32_t _1067 = _1066 + 255;
    int32_t _1068 = _output_2_min_0 + _output_2_extent_0;
    int32_t _1069 = _1068 + -1;
    int32_t _1070 = min(_1067, _1069);
    int32_t _1071 = _1070 + 94;
    int32_t _1072 = _1068 + 93;
    int32_t _1073 = max(_1071, _1072);
    {
     int32_t _1074 = _1073 - _17;
     int32_t _1075 = _1074 + -7;
     int64_t _1076 = _1075;
     int32_t _1077 = _506 - _28;
     int32_t _1078 = _1077 + 13;
     int64_t _1079 = _1076 * _1078;
     int64_t _1080 = (_1079 > ((int64_t(1) << 31) - 1)) ? _1079 : (_1079 * 1);
     if ((_1080 > ((int64_t(1) << 31) - 1)) || ((_1080 * sizeof(uint8_t)) > ((int64_t(1) << 31) - 1)))
     {
      halide_error(NULL, "32-bit signed overflow computing size of allocation constant_exterior$6\n");
      return -1;
     } // overflow test constant_exterior$6
     int64_t _1081 = _1080;
     uint8_t *_constant_exterior_6 = (uint8_t *)halide_malloc(NULL, sizeof(uint8_t)*_1081);
     // produce constant_exterior$6
     int32_t _1082 = _output_2_min_1 + -12;
     int32_t _1083 = max(_p4_min_1, _1082);
     int32_t _1084 = _output_2_extent_1 + _output_2_min_1;
     int32_t _1085 = _1084 + 11;
     int32_t _1086 = min(_1083, _1085);
     int32_t _1087 = _p4_min_1 + _p4_extent_1;
     int32_t _1088 = _output_2_min_1 + _output_2_extent_1;
     int32_t _1089 = _1088 + 11;
     int32_t _1090 = min(_1087, _1089);
     int32_t _1091 = max(_1090, _1086);
     int32_t _1092 = _1086 - _output_2_min_1;
     int32_t _1093 = _1092 + 12;
     for (int _constant_exterior_6_s0__1 = _1082; _constant_exterior_6_s0__1 < _1082 + _1093; _constant_exterior_6_s0__1++)
     {
      int32_t _1094 = _output_2_min_0 + 8;
      int32_t _1095 = _output_2_extent_0 + 86;
      for (int _constant_exterior_6_s0__0 = _1094; _constant_exterior_6_s0__0 < _1094 + _1095; _constant_exterior_6_s0__0++)
      {
       int32_t _1096 = _constant_exterior_6_s0__0 - _17;
       int32_t _1097 = _constant_exterior_6_s0__1 - _28;
       int32_t _1098 = _1097 + 12;
       int32_t _1099 = _1073 - _17;
       int32_t _1100 = _1099 + -7;
       int32_t _1101 = _1098 * _1100;
       int32_t _1102 = _1096 + _1101;
       int32_t _1103 = _1102 + -8;
       uint8_t _1104 = (uint8_t)(0);
       int32_t _1105 = _p4_min_0 + _p4_extent_0;
       int32_t _1106 = _1105 + -1;
       int32_t _1107 = min(_constant_exterior_6_s0__0, _1106);
       int32_t _1108 = max(_1107, _p4_min_0);
       int32_t _1109 = _p4_min_1 + _p4_extent_1;
       int32_t _1110 = _1109 + -1;
       int32_t _1111 = min(_constant_exterior_6_s0__1, _1110);
       int32_t _1112 = max(_1111, _p4_min_1);
       int32_t _1113 = _1112 * _p4_stride_1;
       int32_t _1114 = _1108 + _1113;
       int32_t _1115 = _p4_min_2 + _p4_extent_2;
       int32_t _1116 = _1115 + -1;
       int32_t _1117 = min(_1116, 1);
       int32_t _1118 = max(_1117, _p4_min_2);
       int32_t _1119 = _1118 * _p4_stride_2;
       int32_t _1120 = _1114 + _1119;
       int32_t _1121 = _p4_min_1 * _p4_stride_1;
       int32_t _1122 = _p4_min_0 + _1121;
       int32_t _1123 = _p4_min_2 * _p4_stride_2;
       int32_t _1124 = _1122 + _1123;
       int32_t _1125 = _1120 - _1124;
       uint8_t _1126 = _p4[_1125];
       bool _1127 = _constant_exterior_6_s0__0 < _p4_min_0;
       bool _1128 = _1105 <= _constant_exterior_6_s0__0;
       bool _1129 = _1127 || _1128;
       bool _1130 = _constant_exterior_6_s0__1 < _p4_min_1;
       bool _1131 = _1129 || _1130;
       bool _1132 = _1109 <= _constant_exterior_6_s0__1;
       bool _1133 = _1131 || _1132;
       bool _1134 = 1 < _p4_min_2;
       bool _1135 = _1133 || _1134;
       bool _1136 = _1115 <= 1;
       bool _1137 = _1135 || _1136;
       uint8_t _1138 = (uint8_t)(_1137 ? _1104 : _1126);
       _constant_exterior_6[_1103] = _1138;
      } // for _constant_exterior_6_s0__0
     } // for _constant_exterior_6_s0__1
     int32_t _1139 = _1091 - _1086;
     for (int _constant_exterior_6_s0__1 = _1086; _constant_exterior_6_s0__1 < _1086 + _1139; _constant_exterior_6_s0__1++)
     {
      int32_t _1140 = _output_2_min_0 + 8;
      int32_t _1141 = max(_p4_min_0, _1140);
      int32_t _1142 = _output_2_extent_0 + _output_2_min_0;
      int32_t _1143 = _1142 + 94;
      int32_t _1144 = min(_1141, _1143);
      int32_t _1145 = _p4_min_0 + _p4_extent_0;
      int32_t _1146 = _output_2_min_0 + _output_2_extent_0;
      int32_t _1147 = _1146 + 94;
      int32_t _1148 = min(_1145, _1147);
      int32_t _1149 = max(_1148, _1144);
      int32_t _1150 = _1144 - _output_2_min_0;
      int32_t _1151 = _1150 + -8;
      for (int _constant_exterior_6_s0__0 = _1140; _constant_exterior_6_s0__0 < _1140 + _1151; _constant_exterior_6_s0__0++)
      {
       int32_t _1152 = _constant_exterior_6_s0__0 - _17;
       int32_t _1153 = _constant_exterior_6_s0__1 - _28;
       int32_t _1154 = _1153 + 12;
       int32_t _1155 = _1073 - _17;
       int32_t _1156 = _1155 + -7;
       int32_t _1157 = _1154 * _1156;
       int32_t _1158 = _1152 + _1157;
       int32_t _1159 = _1158 + -8;
       uint8_t _1160 = (uint8_t)(0);
       int32_t _1161 = _p4_min_0 + _p4_extent_0;
       int32_t _1162 = _1161 + -1;
       int32_t _1163 = min(_constant_exterior_6_s0__0, _1162);
       int32_t _1164 = max(_1163, _p4_min_0);
       int32_t _1165 = _constant_exterior_6_s0__1 * _p4_stride_1;
       int32_t _1166 = _1164 + _1165;
       int32_t _1167 = _p4_min_2 + _p4_extent_2;
       int32_t _1168 = _1167 + -1;
       int32_t _1169 = min(_1168, 1);
       int32_t _1170 = max(_1169, _p4_min_2);
       int32_t _1171 = _1170 * _p4_stride_2;
       int32_t _1172 = _1166 + _1171;
       int32_t _1173 = _p4_min_1 * _p4_stride_1;
       int32_t _1174 = _p4_min_0 + _1173;
       int32_t _1175 = _p4_min_2 * _p4_stride_2;
       int32_t _1176 = _1174 + _1175;
       int32_t _1177 = _1172 - _1176;
       uint8_t _1178 = _p4[_1177];
       bool _1179 = _constant_exterior_6_s0__0 < _p4_min_0;
       bool _1180 = _1161 <= _constant_exterior_6_s0__0;
       bool _1181 = _1179 || _1180;
       bool _1182 = 1 < _p4_min_2;
       bool _1183 = _1181 || _1182;
       bool _1184 = _1167 <= 1;
       bool _1185 = _1183 || _1184;
       uint8_t _1186 = (uint8_t)(_1185 ? _1160 : _1178);
       _constant_exterior_6[_1159] = _1186;
      } // for _constant_exterior_6_s0__0
      int32_t _1187 = _1149 - _1144;
      for (int _constant_exterior_6_s0__0 = _1144; _constant_exterior_6_s0__0 < _1144 + _1187; _constant_exterior_6_s0__0++)
      {
       int32_t _1188 = _constant_exterior_6_s0__0 - _17;
       int32_t _1189 = _constant_exterior_6_s0__1 - _28;
       int32_t _1190 = _1189 + 12;
       int32_t _1191 = _1073 - _17;
       int32_t _1192 = _1191 + -7;
       int32_t _1193 = _1190 * _1192;
       int32_t _1194 = _1188 + _1193;
       int32_t _1195 = _1194 + -8;
       uint8_t _1196 = (uint8_t)(0);
       int32_t _1197 = _constant_exterior_6_s0__1 * _p4_stride_1;
       int32_t _1198 = _constant_exterior_6_s0__0 + _1197;
       int32_t _1199 = _p4_min_2 + _p4_extent_2;
       int32_t _1200 = _1199 + -1;
       int32_t _1201 = min(_1200, 1);
       int32_t _1202 = max(_1201, _p4_min_2);
       int32_t _1203 = _1202 * _p4_stride_2;
       int32_t _1204 = _1198 + _1203;
       int32_t _1205 = _p4_min_1 * _p4_stride_1;
       int32_t _1206 = _p4_min_0 + _1205;
       int32_t _1207 = _p4_min_2 * _p4_stride_2;
       int32_t _1208 = _1206 + _1207;
       int32_t _1209 = _1204 - _1208;
       uint8_t _1210 = _p4[_1209];
       bool _1211 = 1 < _p4_min_2;
       bool _1212 = _1199 <= 1;
       bool _1213 = _1211 || _1212;
       uint8_t _1214 = (uint8_t)(_1213 ? _1196 : _1210);
       _constant_exterior_6[_1195] = _1214;
      } // for _constant_exterior_6_s0__0
      int32_t _1215 = _output_2_min_0 + _output_2_extent_0;
      int32_t _1216 = _1215 - _1149;
      int32_t _1217 = _1216 + 94;
      for (int _constant_exterior_6_s0__0 = _1149; _constant_exterior_6_s0__0 < _1149 + _1217; _constant_exterior_6_s0__0++)
      {
       int32_t _1218 = _constant_exterior_6_s0__0 - _17;
       int32_t _1219 = _constant_exterior_6_s0__1 - _28;
       int32_t _1220 = _1219 + 12;
       int32_t _1221 = _1073 - _17;
       int32_t _1222 = _1221 + -7;
       int32_t _1223 = _1220 * _1222;
       int32_t _1224 = _1218 + _1223;
       int32_t _1225 = _1224 + -8;
       uint8_t _1226 = (uint8_t)(0);
       int32_t _1227 = _p4_min_0 + _p4_extent_0;
       int32_t _1228 = _1227 + -1;
       int32_t _1229 = min(_constant_exterior_6_s0__0, _1228);
       int32_t _1230 = max(_1229, _p4_min_0);
       int32_t _1231 = _constant_exterior_6_s0__1 * _p4_stride_1;
       int32_t _1232 = _1230 + _1231;
       int32_t _1233 = _p4_min_2 + _p4_extent_2;
       int32_t _1234 = _1233 + -1;
       int32_t _1235 = min(_1234, 1);
       int32_t _1236 = max(_1235, _p4_min_2);
       int32_t _1237 = _1236 * _p4_stride_2;
       int32_t _1238 = _1232 + _1237;
       int32_t _1239 = _p4_min_1 * _p4_stride_1;
       int32_t _1240 = _p4_min_0 + _1239;
       int32_t _1241 = _p4_min_2 * _p4_stride_2;
       int32_t _1242 = _1240 + _1241;
       int32_t _1243 = _1238 - _1242;
       uint8_t _1244 = _p4[_1243];
       bool _1245 = _1227 <= _constant_exterior_6_s0__0;
       bool _1246 = 1 < _p4_min_2;
       bool _1247 = _1245 || _1246;
       bool _1248 = _1233 <= 1;
       bool _1249 = _1247 || _1248;
       uint8_t _1250 = (uint8_t)(_1249 ? _1226 : _1244);
       _constant_exterior_6[_1225] = _1250;
      } // for _constant_exterior_6_s0__0
     } // for _constant_exterior_6_s0__1
     int32_t _1251 = _output_2_min_1 + _output_2_extent_1;
     int32_t _1252 = _1251 - _1091;
     int32_t _1253 = _1252 + 11;
     for (int _constant_exterior_6_s0__1 = _1091; _constant_exterior_6_s0__1 < _1091 + _1253; _constant_exterior_6_s0__1++)
     {
      int32_t _1254 = _output_2_min_0 + 8;
      int32_t _1255 = _output_2_extent_0 + 86;
      for (int _constant_exterior_6_s0__0 = _1254; _constant_exterior_6_s0__0 < _1254 + _1255; _constant_exterior_6_s0__0++)
      {
       int32_t _1256 = _constant_exterior_6_s0__0 - _17;
       int32_t _1257 = _constant_exterior_6_s0__1 - _28;
       int32_t _1258 = _1257 + 12;
       int32_t _1259 = _1073 - _17;
       int32_t _1260 = _1259 + -7;
       int32_t _1261 = _1258 * _1260;
       int32_t _1262 = _1256 + _1261;
       int32_t _1263 = _1262 + -8;
       uint8_t _1264 = (uint8_t)(0);
       int32_t _1265 = _p4_min_0 + _p4_extent_0;
       int32_t _1266 = _1265 + -1;
       int32_t _1267 = min(_constant_exterior_6_s0__0, _1266);
       int32_t _1268 = max(_1267, _p4_min_0);
       int32_t _1269 = _p4_min_1 + _p4_extent_1;
       int32_t _1270 = _1269 + -1;
       int32_t _1271 = min(_constant_exterior_6_s0__1, _1270);
       int32_t _1272 = max(_1271, _p4_min_1);
       int32_t _1273 = _1272 * _p4_stride_1;
       int32_t _1274 = _1268 + _1273;
       int32_t _1275 = _p4_min_2 + _p4_extent_2;
       int32_t _1276 = _1275 + -1;
       int32_t _1277 = min(_1276, 1);
       int32_t _1278 = max(_1277, _p4_min_2);
       int32_t _1279 = _1278 * _p4_stride_2;
       int32_t _1280 = _1274 + _1279;
       int32_t _1281 = _p4_min_1 * _p4_stride_1;
       int32_t _1282 = _p4_min_0 + _1281;
       int32_t _1283 = _p4_min_2 * _p4_stride_2;
       int32_t _1284 = _1282 + _1283;
       int32_t _1285 = _1280 - _1284;
       uint8_t _1286 = _p4[_1285];
       bool _1287 = _constant_exterior_6_s0__0 < _p4_min_0;
       bool _1288 = _1265 <= _constant_exterior_6_s0__0;
       bool _1289 = _1287 || _1288;
       bool _1290 = _1269 <= _constant_exterior_6_s0__1;
       bool _1291 = _1289 || _1290;
       bool _1292 = 1 < _p4_min_2;
       bool _1293 = _1291 || _1292;
       bool _1294 = _1275 <= 1;
       bool _1295 = _1293 || _1294;
       uint8_t _1296 = (uint8_t)(_1295 ? _1264 : _1286);
       _constant_exterior_6[_1263] = _1296;
      } // for _constant_exterior_6_s0__0
     } // for _constant_exterior_6_s0__1
     // consume constant_exterior$6
     int32_t _1297 = _output_2_extent_0 + -1;
     int32_t _1298 = _1297 >> 8;
     int32_t _1299 = _1298 * 256;
     int32_t _1300 = _1299 + _output_2_min_0;
     int32_t _1301 = _1300 + 255;
     int32_t _1302 = _output_2_min_0 + _output_2_extent_0;
     int32_t _1303 = _1302 + -1;
     int32_t _1304 = min(_1301, _1303);
     int32_t _1305 = _1304 + 86;
     int32_t _1306 = _1302 + 85;
     int32_t _1307 = max(_1305, _1306);
     int32_t _1308 = _1307 - _17;
     int32_t _1309 = _1308 + -15;
     int32_t _1310 = _751 - _28;
     int32_t _1311 = _1310 + 5;
     int32_t _1312 = _1309 * _1311;
     {
      int32_t _1313 = _1307 - _17;
      int32_t _1314 = _1313 + -15;
      int64_t _1315 = _1314;
      int32_t _1316 = _751 - _28;
      int32_t _1317 = _1316 + 5;
      int64_t _1318 = _1315 * _1317;
      int64_t _1319 = (_1318 > ((int64_t(1) << 31) - 1)) ? _1318 : (_1318 * 2);
      if ((_1319 > ((int64_t(1) << 31) - 1)) || ((_1319 * sizeof(uint8_t)) > ((int64_t(1) << 31) - 1)))
      {
       halide_error(NULL, "32-bit signed overflow computing size of allocation constant_exterior$8\n");
       return -1;
      } // overflow test constant_exterior$8
      int64_t _1320 = _1319;
      uint8_t *_constant_exterior_8 = (uint8_t *)halide_malloc(NULL, sizeof(uint8_t)*_1320);
      // produce constant_exterior$8
      int32_t _1321 = max(_p6_min_2, 0);
      int32_t _1322 = min(_1321, 2);
      int32_t _1323 = _p6_min_2 + _p6_extent_2;
      int32_t _1324 = max(_1323, _1321);
      int32_t _1325 = min(_1324, 2);
      for (int _constant_exterior_8_s0__2 = 0; _constant_exterior_8_s0__2 < 0 + _1322; _constant_exterior_8_s0__2++)
      {
       int32_t _1326 = _output_2_min_1 + -4;
       int32_t _1327 = _output_2_extent_1 + 7;
       for (int _constant_exterior_8_s0__1 = _1326; _constant_exterior_8_s0__1 < _1326 + _1327; _constant_exterior_8_s0__1++)
       {
        int32_t _1328 = _output_2_min_0 + 16;
        int32_t _1329 = _output_2_extent_0 + 70;
        for (int _constant_exterior_8_s0__0 = _1328; _constant_exterior_8_s0__0 < _1328 + _1329; _constant_exterior_8_s0__0++)
        {
         int32_t _1330 = _constant_exterior_8_s0__0 - _17;
         int32_t _1331 = _constant_exterior_8_s0__1 - _28;
         int32_t _1332 = _1331 + 4;
         int32_t _1333 = _1307 - _17;
         int32_t _1334 = _1333 + -15;
         int32_t _1335 = _1332 * _1334;
         int32_t _1336 = _1330 + _1335;
         int32_t _1337 = _constant_exterior_8_s0__2 * _1312;
         int32_t _1338 = _1336 + _1337;
         int32_t _1339 = _1338 + -16;
         uint8_t _1340 = (uint8_t)(128);
         int32_t _1341 = _p6_min_0 + _p6_extent_0;
         int32_t _1342 = _1341 + -1;
         int32_t _1343 = min(_constant_exterior_8_s0__0, _1342);
         int32_t _1344 = max(_1343, _p6_min_0);
         int32_t _1345 = _p6_min_1 + _p6_extent_1;
         int32_t _1346 = _1345 + -1;
         int32_t _1347 = min(_constant_exterior_8_s0__1, _1346);
         int32_t _1348 = max(_1347, _p6_min_1);
         int32_t _1349 = _1348 * _p6_stride_1;
         int32_t _1350 = _1344 + _1349;
         int32_t _1351 = _p6_min_2 + _p6_extent_2;
         int32_t _1352 = _1351 + -1;
         int32_t _1353 = min(_constant_exterior_8_s0__2, _1352);
         int32_t _1354 = max(_1353, _p6_min_2);
         int32_t _1355 = _1354 * _p6_stride_2;
         int32_t _1356 = _1350 + _1355;
         int32_t _1357 = _p6_min_1 * _p6_stride_1;
         int32_t _1358 = _p6_min_0 + _1357;
         int32_t _1359 = _p6_min_2 * _p6_stride_2;
         int32_t _1360 = _1358 + _1359;
         int32_t _1361 = _1356 - _1360;
         uint8_t _1362 = _p6[_1361];
         bool _1363 = _constant_exterior_8_s0__0 < _p6_min_0;
         bool _1364 = _1341 <= _constant_exterior_8_s0__0;
         bool _1365 = _1363 || _1364;
         bool _1366 = _constant_exterior_8_s0__1 < _p6_min_1;
         bool _1367 = _1365 || _1366;
         bool _1368 = _1345 <= _constant_exterior_8_s0__1;
         bool _1369 = _1367 || _1368;
         bool _1370 = _constant_exterior_8_s0__2 < _p6_min_2;
         bool _1371 = _1369 || _1370;
         bool _1372 = _1351 <= _constant_exterior_8_s0__2;
         bool _1373 = _1371 || _1372;
         uint8_t _1374 = (uint8_t)(_1373 ? _1340 : _1362);
         _constant_exterior_8[_1339] = _1374;
        } // for _constant_exterior_8_s0__0
       } // for _constant_exterior_8_s0__1
      } // for _constant_exterior_8_s0__2
      int32_t _1375 = _1325 - _1322;
      for (int _constant_exterior_8_s0__2 = _1322; _constant_exterior_8_s0__2 < _1322 + _1375; _constant_exterior_8_s0__2++)
      {
       int32_t _1376 = _output_2_min_1 + -4;
       int32_t _1377 = max(_p6_min_1, _1376);
       int32_t _1378 = _output_2_extent_1 + _output_2_min_1;
       int32_t _1379 = _1378 + 3;
       int32_t _1380 = min(_1377, _1379);
       int32_t _1381 = _p6_min_1 + _p6_extent_1;
       int32_t _1382 = _output_2_min_1 + _output_2_extent_1;
       int32_t _1383 = _1382 + 3;
       int32_t _1384 = min(_1381, _1383);
       int32_t _1385 = max(_1384, _1380);
       int32_t _1386 = _1380 - _output_2_min_1;
       int32_t _1387 = _1386 + 4;
       for (int _constant_exterior_8_s0__1 = _1376; _constant_exterior_8_s0__1 < _1376 + _1387; _constant_exterior_8_s0__1++)
       {
        int32_t _1388 = _output_2_min_0 + 16;
        int32_t _1389 = _output_2_extent_0 + 70;
        for (int _constant_exterior_8_s0__0 = _1388; _constant_exterior_8_s0__0 < _1388 + _1389; _constant_exterior_8_s0__0++)
        {
         int32_t _1390 = _constant_exterior_8_s0__0 - _17;
         int32_t _1391 = _constant_exterior_8_s0__1 - _28;
         int32_t _1392 = _1391 + 4;
         int32_t _1393 = _1307 - _17;
         int32_t _1394 = _1393 + -15;
         int32_t _1395 = _1392 * _1394;
         int32_t _1396 = _1390 + _1395;
         int32_t _1397 = _constant_exterior_8_s0__2 * _1312;
         int32_t _1398 = _1396 + _1397;
         int32_t _1399 = _1398 + -16;
         uint8_t _1400 = (uint8_t)(128);
         int32_t _1401 = _p6_min_0 + _p6_extent_0;
         int32_t _1402 = _1401 + -1;
         int32_t _1403 = min(_constant_exterior_8_s0__0, _1402);
         int32_t _1404 = max(_1403, _p6_min_0);
         int32_t _1405 = _p6_min_1 + _p6_extent_1;
         int32_t _1406 = _1405 + -1;
         int32_t _1407 = min(_constant_exterior_8_s0__1, _1406);
         int32_t _1408 = max(_1407, _p6_min_1);
         int32_t _1409 = _1408 * _p6_stride_1;
         int32_t _1410 = _1404 + _1409;
         int32_t _1411 = _constant_exterior_8_s0__2 * _p6_stride_2;
         int32_t _1412 = _1410 + _1411;
         int32_t _1413 = _p6_min_1 * _p6_stride_1;
         int32_t _1414 = _p6_min_0 + _1413;
         int32_t _1415 = _p6_min_2 * _p6_stride_2;
         int32_t _1416 = _1414 + _1415;
         int32_t _1417 = _1412 - _1416;
         uint8_t _1418 = _p6[_1417];
         bool _1419 = _constant_exterior_8_s0__0 < _p6_min_0;
         bool _1420 = _1401 <= _constant_exterior_8_s0__0;
         bool _1421 = _1419 || _1420;
         bool _1422 = _constant_exterior_8_s0__1 < _p6_min_1;
         bool _1423 = _1421 || _1422;
         bool _1424 = _1405 <= _constant_exterior_8_s0__1;
         bool _1425 = _1423 || _1424;
         uint8_t _1426 = (uint8_t)(_1425 ? _1400 : _1418);
         _constant_exterior_8[_1399] = _1426;
        } // for _constant_exterior_8_s0__0
       } // for _constant_exterior_8_s0__1
       int32_t _1427 = _1385 - _1380;
       for (int _constant_exterior_8_s0__1 = _1380; _constant_exterior_8_s0__1 < _1380 + _1427; _constant_exterior_8_s0__1++)
       {
        int32_t _1428 = _output_2_min_0 + 16;
        int32_t _1429 = max(_p6_min_0, _1428);
        int32_t _1430 = _output_2_extent_0 + _output_2_min_0;
        int32_t _1431 = _1430 + 86;
        int32_t _1432 = min(_1429, _1431);
        int32_t _1433 = _p6_min_0 + _p6_extent_0;
        int32_t _1434 = _output_2_min_0 + _output_2_extent_0;
        int32_t _1435 = _1434 + 86;
        int32_t _1436 = min(_1433, _1435);
        int32_t _1437 = max(_1436, _1432);
        int32_t _1438 = _1432 - _output_2_min_0;
        int32_t _1439 = _1438 + -16;
        for (int _constant_exterior_8_s0__0 = _1428; _constant_exterior_8_s0__0 < _1428 + _1439; _constant_exterior_8_s0__0++)
        {
         int32_t _1440 = _constant_exterior_8_s0__0 - _17;
         int32_t _1441 = _constant_exterior_8_s0__1 - _28;
         int32_t _1442 = _1441 + 4;
         int32_t _1443 = _1307 - _17;
         int32_t _1444 = _1443 + -15;
         int32_t _1445 = _1442 * _1444;
         int32_t _1446 = _1440 + _1445;
         int32_t _1447 = _constant_exterior_8_s0__2 * _1312;
         int32_t _1448 = _1446 + _1447;
         int32_t _1449 = _1448 + -16;
         uint8_t _1450 = (uint8_t)(128);
         int32_t _1451 = _p6_min_0 + _p6_extent_0;
         int32_t _1452 = _1451 + -1;
         int32_t _1453 = min(_constant_exterior_8_s0__0, _1452);
         int32_t _1454 = max(_1453, _p6_min_0);
         int32_t _1455 = _constant_exterior_8_s0__1 * _p6_stride_1;
         int32_t _1456 = _1454 + _1455;
         int32_t _1457 = _constant_exterior_8_s0__2 * _p6_stride_2;
         int32_t _1458 = _1456 + _1457;
         int32_t _1459 = _p6_min_1 * _p6_stride_1;
         int32_t _1460 = _p6_min_0 + _1459;
         int32_t _1461 = _p6_min_2 * _p6_stride_2;
         int32_t _1462 = _1460 + _1461;
         int32_t _1463 = _1458 - _1462;
         uint8_t _1464 = _p6[_1463];
         bool _1465 = _constant_exterior_8_s0__0 < _p6_min_0;
         bool _1466 = _1451 <= _constant_exterior_8_s0__0;
         bool _1467 = _1465 || _1466;
         uint8_t _1468 = (uint8_t)(_1467 ? _1450 : _1464);
         _constant_exterior_8[_1449] = _1468;
        } // for _constant_exterior_8_s0__0
        int32_t _1469 = _1437 - _1432;
        for (int _constant_exterior_8_s0__0 = _1432; _constant_exterior_8_s0__0 < _1432 + _1469; _constant_exterior_8_s0__0++)
        {
         int32_t _1470 = _constant_exterior_8_s0__0 - _17;
         int32_t _1471 = _constant_exterior_8_s0__1 - _28;
         int32_t _1472 = _1471 + 4;
         int32_t _1473 = _1307 - _17;
         int32_t _1474 = _1473 + -15;
         int32_t _1475 = _1472 * _1474;
         int32_t _1476 = _1470 + _1475;
         int32_t _1477 = _constant_exterior_8_s0__2 * _1312;
         int32_t _1478 = _1476 + _1477;
         int32_t _1479 = _1478 + -16;
         int32_t _1480 = _constant_exterior_8_s0__1 * _p6_stride_1;
         int32_t _1481 = _constant_exterior_8_s0__0 + _1480;
         int32_t _1482 = _constant_exterior_8_s0__2 * _p6_stride_2;
         int32_t _1483 = _1481 + _1482;
         int32_t _1484 = _p6_min_1 * _p6_stride_1;
         int32_t _1485 = _p6_min_0 + _1484;
         int32_t _1486 = _p6_min_2 * _p6_stride_2;
         int32_t _1487 = _1485 + _1486;
         int32_t _1488 = _1483 - _1487;
         uint8_t _1489 = _p6[_1488];
         _constant_exterior_8[_1479] = _1489;
        } // for _constant_exterior_8_s0__0
        int32_t _1490 = _output_2_min_0 + _output_2_extent_0;
        int32_t _1491 = _1490 - _1437;
        int32_t _1492 = _1491 + 86;
        for (int _constant_exterior_8_s0__0 = _1437; _constant_exterior_8_s0__0 < _1437 + _1492; _constant_exterior_8_s0__0++)
        {
         int32_t _1493 = _constant_exterior_8_s0__0 - _17;
         int32_t _1494 = _constant_exterior_8_s0__1 - _28;
         int32_t _1495 = _1494 + 4;
         int32_t _1496 = _1307 - _17;
         int32_t _1497 = _1496 + -15;
         int32_t _1498 = _1495 * _1497;
         int32_t _1499 = _1493 + _1498;
         int32_t _1500 = _constant_exterior_8_s0__2 * _1312;
         int32_t _1501 = _1499 + _1500;
         int32_t _1502 = _1501 + -16;
         int32_t _1503 = _p6_min_0 + _p6_extent_0;
         int32_t _1504 = _1503 + -1;
         int32_t _1505 = min(_constant_exterior_8_s0__0, _1504);
         int32_t _1506 = max(_1505, _p6_min_0);
         int32_t _1507 = _constant_exterior_8_s0__1 * _p6_stride_1;
         int32_t _1508 = _1506 + _1507;
         int32_t _1509 = _constant_exterior_8_s0__2 * _p6_stride_2;
         int32_t _1510 = _1508 + _1509;
         int32_t _1511 = _p6_min_1 * _p6_stride_1;
         int32_t _1512 = _p6_min_0 + _1511;
         int32_t _1513 = _p6_min_2 * _p6_stride_2;
         int32_t _1514 = _1512 + _1513;
         int32_t _1515 = _1510 - _1514;
         uint8_t _1516 = _p6[_1515];
         uint8_t _1517 = (uint8_t)(128);
         bool _1518 = _constant_exterior_8_s0__0 < _1503;
         uint8_t _1519 = (uint8_t)(_1518 ? _1516 : _1517);
         _constant_exterior_8[_1502] = _1519;
        } // for _constant_exterior_8_s0__0
       } // for _constant_exterior_8_s0__1
       int32_t _1520 = _output_2_min_1 + _output_2_extent_1;
       int32_t _1521 = _1520 - _1385;
       int32_t _1522 = _1521 + 3;
       for (int _constant_exterior_8_s0__1 = _1385; _constant_exterior_8_s0__1 < _1385 + _1522; _constant_exterior_8_s0__1++)
       {
        int32_t _1523 = _output_2_min_0 + 16;
        int32_t _1524 = _output_2_extent_0 + 70;
        for (int _constant_exterior_8_s0__0 = _1523; _constant_exterior_8_s0__0 < _1523 + _1524; _constant_exterior_8_s0__0++)
        {
         int32_t _1525 = _constant_exterior_8_s0__0 - _17;
         int32_t _1526 = _constant_exterior_8_s0__1 - _28;
         int32_t _1527 = _1526 + 4;
         int32_t _1528 = _1307 - _17;
         int32_t _1529 = _1528 + -15;
         int32_t _1530 = _1527 * _1529;
         int32_t _1531 = _1525 + _1530;
         int32_t _1532 = _constant_exterior_8_s0__2 * _1312;
         int32_t _1533 = _1531 + _1532;
         int32_t _1534 = _1533 + -16;
         uint8_t _1535 = (uint8_t)(128);
         int32_t _1536 = _p6_min_0 + _p6_extent_0;
         int32_t _1537 = _1536 + -1;
         int32_t _1538 = min(_constant_exterior_8_s0__0, _1537);
         int32_t _1539 = max(_1538, _p6_min_0);
         int32_t _1540 = _p6_min_1 + _p6_extent_1;
         int32_t _1541 = _1540 + -1;
         int32_t _1542 = min(_constant_exterior_8_s0__1, _1541);
         int32_t _1543 = max(_1542, _p6_min_1);
         int32_t _1544 = _1543 * _p6_stride_1;
         int32_t _1545 = _1539 + _1544;
         int32_t _1546 = _constant_exterior_8_s0__2 * _p6_stride_2;
         int32_t _1547 = _1545 + _1546;
         int32_t _1548 = _p6_min_1 * _p6_stride_1;
         int32_t _1549 = _p6_min_0 + _1548;
         int32_t _1550 = _p6_min_2 * _p6_stride_2;
         int32_t _1551 = _1549 + _1550;
         int32_t _1552 = _1547 - _1551;
         uint8_t _1553 = _p6[_1552];
         bool _1554 = _constant_exterior_8_s0__0 < _p6_min_0;
         bool _1555 = _1536 <= _constant_exterior_8_s0__0;
         bool _1556 = _1554 || _1555;
         bool _1557 = _1540 <= _constant_exterior_8_s0__1;
         bool _1558 = _1556 || _1557;
         uint8_t _1559 = (uint8_t)(_1558 ? _1535 : _1553);
         _constant_exterior_8[_1534] = _1559;
        } // for _constant_exterior_8_s0__0
       } // for _constant_exterior_8_s0__1
      } // for _constant_exterior_8_s0__2
      int32_t _1560 = 2 - _1325;
      for (int _constant_exterior_8_s0__2 = _1325; _constant_exterior_8_s0__2 < _1325 + _1560; _constant_exterior_8_s0__2++)
      {
       int32_t _1561 = _output_2_min_1 + -4;
       int32_t _1562 = _output_2_extent_1 + 7;
       for (int _constant_exterior_8_s0__1 = _1561; _constant_exterior_8_s0__1 < _1561 + _1562; _constant_exterior_8_s0__1++)
       {
        int32_t _1563 = _output_2_min_0 + 16;
        int32_t _1564 = _output_2_extent_0 + 70;
        for (int _constant_exterior_8_s0__0 = _1563; _constant_exterior_8_s0__0 < _1563 + _1564; _constant_exterior_8_s0__0++)
        {
         int32_t _1565 = _constant_exterior_8_s0__0 - _17;
         int32_t _1566 = _constant_exterior_8_s0__1 - _28;
         int32_t _1567 = _1566 + 4;
         int32_t _1568 = _1307 - _17;
         int32_t _1569 = _1568 + -15;
         int32_t _1570 = _1567 * _1569;
         int32_t _1571 = _1565 + _1570;
         int32_t _1572 = _constant_exterior_8_s0__2 * _1312;
         int32_t _1573 = _1571 + _1572;
         int32_t _1574 = _1573 + -16;
         uint8_t _1575 = (uint8_t)(128);
         int32_t _1576 = _p6_min_0 + _p6_extent_0;
         int32_t _1577 = _1576 + -1;
         int32_t _1578 = min(_constant_exterior_8_s0__0, _1577);
         int32_t _1579 = max(_1578, _p6_min_0);
         int32_t _1580 = _p6_min_1 + _p6_extent_1;
         int32_t _1581 = _1580 + -1;
         int32_t _1582 = min(_constant_exterior_8_s0__1, _1581);
         int32_t _1583 = max(_1582, _p6_min_1);
         int32_t _1584 = _1583 * _p6_stride_1;
         int32_t _1585 = _1579 + _1584;
         int32_t _1586 = _p6_min_2 + _p6_extent_2;
         int32_t _1587 = _1586 + -1;
         int32_t _1588 = min(_constant_exterior_8_s0__2, _1587);
         int32_t _1589 = max(_1588, _p6_min_2);
         int32_t _1590 = _1589 * _p6_stride_2;
         int32_t _1591 = _1585 + _1590;
         int32_t _1592 = _p6_min_1 * _p6_stride_1;
         int32_t _1593 = _p6_min_0 + _1592;
         int32_t _1594 = _p6_min_2 * _p6_stride_2;
         int32_t _1595 = _1593 + _1594;
         int32_t _1596 = _1591 - _1595;
         uint8_t _1597 = _p6[_1596];
         bool _1598 = _constant_exterior_8_s0__0 < _p6_min_0;
         bool _1599 = _1576 <= _constant_exterior_8_s0__0;
         bool _1600 = _1598 || _1599;
         bool _1601 = _constant_exterior_8_s0__1 < _p6_min_1;
         bool _1602 = _1600 || _1601;
         bool _1603 = _1580 <= _constant_exterior_8_s0__1;
         bool _1604 = _1602 || _1603;
         bool _1605 = _1586 <= _constant_exterior_8_s0__2;
         bool _1606 = _1604 || _1605;
         uint8_t _1607 = (uint8_t)(_1606 ? _1575 : _1597);
         _constant_exterior_8[_1574] = _1607;
        } // for _constant_exterior_8_s0__0
       } // for _constant_exterior_8_s0__1
      } // for _constant_exterior_8_s0__2
      // consume constant_exterior$8
      // produce output$2
      int32_t _1608 = _output_2_extent_1 + 255;
      int32_t _1609 = _1608 >> 8;
      for (int _output_2_s0_y_yo = 0; _output_2_s0_y_yo < 0 + _1609; _output_2_s0_y_yo++)
      {
       int32_t _1610 = _output_2_s0_y_yo * 256;
       int32_t _1611 = _1610 + _output_2_min_1;
       int32_t _1612 = _output_2_min_1 + _output_2_extent_1;
       int32_t _1613 = _1612 + -256;
       int32_t _1614 = min(_1611, _1613);
       int32_t _1615 = _output_2_extent_0 + 255;
       int32_t _1616 = _1615 >> 8;
       for (int _output_2_s0_x_xo = 0; _output_2_s0_x_xo < 0 + _1616; _output_2_s0_x_xo++)
       {
        int32_t _1617 = _output_2_s0_x_xo * 256;
        int32_t _1618 = _1617 + _output_2_min_0;
        int32_t _1619 = _output_2_min_0 + _output_2_extent_0;
        int32_t _1620 = _1619 + -256;
        int32_t _1621 = min(_1618, _1620);

        uint8_t* in0_data = (uint8_t*) mmap(NULL, bufs[0].stride * bufs[0].height * bufs[0].depth,
                                            PROT_WRITE, MAP_SHARED, hwacc, bufs[0].mmap_offset);
        if(in0_data == MAP_FAILED){
          printf("mmap 0 failed!\n");
          return(0);
        }
        // produce interpolated$3.stencil_update.stream
        for (int _interpolated_3_scan_update_y = 0; _interpolated_3_scan_update_y < 0 + 263; _interpolated_3_scan_update_y++)
        {
         for (int _interpolated_3_scan_update_x = 0; _interpolated_3_scan_update_x < 0 + 263; _interpolated_3_scan_update_x++)
         {
          int32_t _1622 = _1621 + _interpolated_3_scan_update_x;
          int32_t _1623 = _1622 - _17;
          int32_t _1624 = _1614 + _interpolated_3_scan_update_y;
          int32_t _1625 = _1624 - _28;
          int32_t _1626 = _762 - _17;
          int32_t _1627 = _1626 + 5;
          int32_t _1628 = _1625 * _1627;
          int32_t _1629 = _1623 + _1628;
          uint8_t _1630 = _constant_exterior_7[_1629];
          int16_t _1631 = (int16_t)(_1630);
          int16_t _1632 = (int16_t)(-128);
          int16_t _1633 = _1631 + _1632;
          float _1634 = (float)(_1633);
          float _1635 = _1634 * float_from_bits(1031798784 /* 0.0625 */);
          float _1636 = floor_f32(_1635);
          int32_t _1637 = (int32_t)(_1636);
          int32_t _1638 = _1629 + _767;
          uint8_t _1639 = _constant_exterior_7[_1638];
          int16_t _1640 = (int16_t)(_1639);
          int16_t _1641 = _1640 + _1632;
          float _1642 = (float)(_1641);
          float _1643 = _1642 * float_from_bits(1031798784 /* 0.0625 */);
          float _1644 = floor_f32(_1643);
          int32_t _1645 = (int32_t)(_1644);
          float _1646 = (float)(_1637);
          float _1647 = _1635 - _1646;
          int32_t _1648 = _1622 + _1637;
          int32_t _1649 = _1648 - _17;
          int32_t _1650 = _1624 + _1645;
          int32_t _1651 = _1650 - _28;
          int32_t _1652 = _1651 + 8;
          int32_t _1653 = _517 - _17;
          int32_t _1654 = _1653 + 13;
          int32_t _1655 = _1652 * _1654;
          int32_t _1656 = _1649 + _1655;
          int32_t _1657 = _1651 + 9;
          int32_t _1658 = _1657 * _1654;
          int32_t _1659 = _1649 + _1658;
          int32_t _1660 = _1656 + 8;
          uint8_t _1661 = _constant_exterior_5[_1660];
          uint16_t _1662 = (uint16_t)(_1661);
          uint8_t _1663 = (uint8_t)(255);
          float _1664 = _1647 * float_from_bits(1132396544 /* 255 */);
          uint8_t _1665 = (uint8_t)(_1664);
          uint8_t _1666 = _1663 - _1665;
          uint16_t _1667 = (uint16_t)(_1666);
          uint16_t _1668 = _1662 * _1667;
          int32_t _1669 = _1656 + 9;
          uint8_t _1670 = _constant_exterior_5[_1669];
          uint16_t _1671 = (uint16_t)(_1670);
          uint16_t _1672 = (uint16_t)(_1665);
          uint16_t _1673 = _1671 * _1672;
          uint16_t _1674 = _1668 + _1673;
          uint16_t _1675 = (uint16_t)(128);
          uint16_t _1676 = _1674 + _1675;
          uint16_t _1677 = _1676 >> 8;
          uint16_t _1678 = _1677 + _1674;
          uint16_t _1679 = _1678 + _1675;
          uint16_t _1680 = _1679 >> 8;
          uint8_t _1681 = (uint8_t)(_1680);
          uint8_t _1682 = _1681;
          uint16_t _1683 = (uint16_t)(_1682);
          float _1684 = (float)(_1645);
          float _1685 = _1643 - _1684;
          float _1686 = _1685 * float_from_bits(1132396544 /* 255 */);
          uint8_t _1687 = (uint8_t)(_1686);
          uint8_t _1688 = _1663 - _1687;
          uint16_t _1689 = (uint16_t)(_1688);
          uint16_t _1690 = _1683 * _1689;
          int32_t _1691 = _1659 + 8;
          uint8_t _1692 = _constant_exterior_5[_1691];
          uint16_t _1693 = (uint16_t)(_1692);
          uint16_t _1694 = _1693 * _1667;
          int32_t _1695 = _1659 + 9;
          uint8_t _1696 = _constant_exterior_5[_1695];
          uint16_t _1697 = (uint16_t)(_1696);
          uint16_t _1698 = _1697 * _1672;
          uint16_t _1699 = _1694 + _1698;
          uint16_t _1700 = _1699 + _1675;
          uint16_t _1701 = _1700 >> 8;
          uint16_t _1702 = _1701 + _1699;
          uint16_t _1703 = _1702 + _1675;
          uint16_t _1704 = _1703 >> 8;
          uint8_t _1705 = (uint8_t)(_1704);
          uint8_t _1706 = _1705;
          uint16_t _1707 = (uint16_t)(_1706);
          uint16_t _1708 = (uint16_t)(_1687);
          uint16_t _1709 = _1707 * _1708;
          uint16_t _1710 = _1690 + _1709;
          uint16_t _1711 = _1710 + _1675;
          uint16_t _1712 = _1711 >> 8;
          uint16_t _1713 = _1712 + _1710;
          uint16_t _1714 = _1713 + _1675;
          uint16_t _1715 = _1714 >> 8;
          uint8_t _1716 = (uint8_t)(_1715);
          uint8_t _1717 = _1716;
          in0_data[_interpolated_3_scan_update_y*bufs[0].stride + _interpolated_3_scan_update_x] = _1717;
          (void)0;
         } // for _interpolated_3_scan_update_x
        } // for _interpolated_3_scan_update_y
        // consume interpolated$3.stencil_update.stream
        munmap((void*)in0_data, bufs[0].stride * bufs[0].height * bufs[0].depth);

        uint8_t *in1_data = (uint8_t*) mmap(NULL, bufs[1].stride * bufs[1].height * bufs[1].depth,
                               PROT_WRITE, MAP_SHARED, hwacc, bufs[1].mmap_offset);
        if(in1_data == MAP_FAILED){
          printf("mmap 1 failed!\n");
          return(0);
        }
        // produce interpolated$4.stencil_update.stream
        for (int _interpolated_4_scan_update_y = 0; _interpolated_4_scan_update_y < 0 + 263; _interpolated_4_scan_update_y++)
        {
         for (int _interpolated_4_scan_update_x = 0; _interpolated_4_scan_update_x < 0 + 326; _interpolated_4_scan_update_x++)
         {
          // produce interpolated$4.stencil
          int32_t _1718 = _1621 + _interpolated_4_scan_update_x;
          int32_t _1719 = _1718 - _17;
          int32_t _1720 = _1614 + _interpolated_4_scan_update_y;
          int32_t _1721 = _1720 - _28;
          int32_t _1722 = _1307 - _17;
          int32_t _1723 = _1722 + -15;
          int32_t _1724 = _1721 * _1723;
          int32_t _1725 = _1719 + _1724;
          uint8_t _1726 = _constant_exterior_8[_1725];
          int16_t _1727 = (int16_t)(_1726);
          int16_t _1728 = (int16_t)(-128);
          int16_t _1729 = _1727 + _1728;
          float _1730 = (float)(_1729);
          float _1731 = _1730 * float_from_bits(1031798784 /* 0.0625 */);
          float _1732 = floor_f32(_1731);
          int32_t _1733 = (int32_t)(_1732);
          int32_t _1734 = _1725 + _1312;
          uint8_t _1735 = _constant_exterior_8[_1734];
          int16_t _1736 = (int16_t)(_1735);
          int16_t _1737 = _1736 + _1728;
          float _1738 = (float)(_1737);
          float _1739 = _1738 * float_from_bits(1031798784 /* 0.0625 */);
          float _1740 = floor_f32(_1739);
          int32_t _1741 = (int32_t)(_1740);
          float _1742 = (float)(_1733);
          float _1743 = _1731 - _1742;
          int32_t _1744 = _1718 + _1733;
          int32_t _1745 = _1744 - _17;
          int32_t _1746 = _1720 + _1741;
          int32_t _1747 = _1746 - _28;
          int32_t _1748 = _1747 + 8;
          int32_t _1749 = _1073 - _17;
          int32_t _1750 = _1749 + -7;
          int32_t _1751 = _1748 * _1750;
          int32_t _1752 = _1745 + _1751;
          int32_t _1753 = _1747 + 9;
          int32_t _1754 = _1753 * _1750;
          int32_t _1755 = _1745 + _1754;
          int32_t _1756 = _1752 + 8;
          uint8_t _1757 = _constant_exterior_6[_1756];
          uint16_t _1758 = (uint16_t)(_1757);
          uint8_t _1759 = (uint8_t)(255);
          float _1760 = _1743 * float_from_bits(1132396544 /* 255 */);
          uint8_t _1761 = (uint8_t)(_1760);
          uint8_t _1762 = _1759 - _1761;
          uint16_t _1763 = (uint16_t)(_1762);
          uint16_t _1764 = _1758 * _1763;
          int32_t _1765 = _1752 + 9;
          uint8_t _1766 = _constant_exterior_6[_1765];
          uint16_t _1767 = (uint16_t)(_1766);
          uint16_t _1768 = (uint16_t)(_1761);
          uint16_t _1769 = _1767 * _1768;
          uint16_t _1770 = _1764 + _1769;
          uint16_t _1771 = (uint16_t)(128);
          uint16_t _1772 = _1770 + _1771;
          uint16_t _1773 = _1772 >> 8;
          uint16_t _1774 = _1773 + _1770;
          uint16_t _1775 = _1774 + _1771;
          uint16_t _1776 = _1775 >> 8;
          uint8_t _1777 = (uint8_t)(_1776);
          uint8_t _1778 = _1777;
          uint16_t _1779 = (uint16_t)(_1778);
          float _1780 = (float)(_1741);
          float _1781 = _1739 - _1780;
          float _1782 = _1781 * float_from_bits(1132396544 /* 255 */);
          uint8_t _1783 = (uint8_t)(_1782);
          uint8_t _1784 = _1759 - _1783;
          uint16_t _1785 = (uint16_t)(_1784);
          uint16_t _1786 = _1779 * _1785;
          int32_t _1787 = _1755 + 8;
          uint8_t _1788 = _constant_exterior_6[_1787];
          uint16_t _1789 = (uint16_t)(_1788);
          uint16_t _1790 = _1789 * _1763;
          int32_t _1791 = _1755 + 9;
          uint8_t _1792 = _constant_exterior_6[_1791];
          uint16_t _1793 = (uint16_t)(_1792);
          uint16_t _1794 = _1793 * _1768;
          uint16_t _1795 = _1790 + _1794;
          uint16_t _1796 = _1795 + _1771;
          uint16_t _1797 = _1796 >> 8;
          uint16_t _1798 = _1797 + _1795;
          uint16_t _1799 = _1798 + _1771;
          uint16_t _1800 = _1799 >> 8;
          uint8_t _1801 = (uint8_t)(_1800);
          uint8_t _1802 = _1801;
          uint16_t _1803 = (uint16_t)(_1802);
          uint16_t _1804 = (uint16_t)(_1783);
          uint16_t _1805 = _1803 * _1804;
          uint16_t _1806 = _1786 + _1805;
          uint16_t _1807 = _1806 + _1771;
          uint16_t _1808 = _1807 >> 8;
          uint16_t _1809 = _1808 + _1806;
          uint16_t _1810 = _1809 + _1771;
          uint16_t _1811 = _1810 >> 8;
          uint8_t _1812 = (uint8_t)(_1811);
          uint8_t _1813 = _1812;
          in1_data[_interpolated_4_scan_update_y*bufs[1].stride + _interpolated_4_scan_update_x] = _1813;
          (void)0;
         } // for _interpolated_4_scan_update_x
        } // for _interpolated_4_scan_update_y
        munmap((void*)in1_data, bufs[1].stride * bufs[1].height * bufs[1].depth);
        // consume interpolated$4.stencil_update.stream
        ioctl(hwacc, PROCESS_IMAGE, (long unsigned int)bufs);
        ioctl(hwacc, PEND_PROCESSED, NULL);
        // consume _hls_target.hw_output$2.stencil.stream

        uint8_t *out_data = (uint8_t*) mmap(NULL, bufs[2].stride * bufs[2].height * bufs[2].depth,
                                            PROT_READ, MAP_SHARED, hwacc, bufs[2].mmap_offset);
        if(out_data == MAP_FAILED){
          printf("mmap 2 failed!\n");
          return(0);
        }
        for (int _output_2_s0_y_y_in = 0; _output_2_s0_y_y_in < 0 + 256; _output_2_s0_y_y_in++)
        {
         for (int _output_2_s0_x_x_in = 0; _output_2_s0_x_x_in < 0 + 256; _output_2_s0_x_x_in++)
         {
          // consume hw_output$2.stencil
          int32_t _14127 = _1621 + _output_2_s0_x_x_in;
          int32_t _14128 = _1614 + _output_2_s0_y_y_in;
          int32_t _14129 = _14128 * _output_2_stride_1;
          int32_t _14130 = _14127 + _14129;
          int32_t _14131 = _output_2_min_1 * _output_2_stride_1;
          int32_t _14132 = _output_2_min_0 + _14131;
          int32_t _14133 = _14130 - _14132;
          uint8_t _14134 = out_data[_output_2_s0_y_y_in*bufs[2].stride + _output_2_s0_x_x_in];
          _output_2[_14133] = _14134;
         } // for _output_2_s0_x_x_in
        } // for _output_2_s0_y_y_in
        munmap((void*)out_data, bufs[2].stride * bufs[2].height * bufs[2].depth);
       } // for _output_2_s0_x_xo
      } // for _output_2_s0_y_yo
      halide_free(NULL, _constant_exterior_5);
      halide_free(NULL, _constant_exterior_7);
      halide_free(NULL, _constant_exterior_6);
      halide_free(NULL, _constant_exterior_8);
      // consume output$2
     } // alloc _constant_exterior_8
    } // alloc _constant_exterior_6
   } // alloc _constant_exterior_7
  } // alloc _constant_exterior_5
 } // if _153
 close(hwacc);
 return 0;
}


int pipeline_zynq(buffer_t *_p5_buffer, buffer_t *_p4_buffer, buffer_t *_p7_buffer, buffer_t *_p6_buffer, buffer_t *_output_2_buffer) HALIDE_FUNCTION_ATTRS {
 uint8_t *_p5 = (uint8_t *)(_p5_buffer->host);
 (void)_p5;
 const bool _p5_host_and_dev_are_null = (_p5_buffer->host == NULL) && (_p5_buffer->dev == 0);
 (void)_p5_host_and_dev_are_null;
 const int32_t _p5_min_0 = _p5_buffer->min[0];
 (void)_p5_min_0;
 const int32_t _p5_min_1 = _p5_buffer->min[1];
 (void)_p5_min_1;
 const int32_t _p5_min_2 = _p5_buffer->min[2];
 (void)_p5_min_2;
 const int32_t _p5_min_3 = _p5_buffer->min[3];
 (void)_p5_min_3;
 const int32_t _p5_extent_0 = _p5_buffer->extent[0];
 (void)_p5_extent_0;
 const int32_t _p5_extent_1 = _p5_buffer->extent[1];
 (void)_p5_extent_1;
 const int32_t _p5_extent_2 = _p5_buffer->extent[2];
 (void)_p5_extent_2;
 const int32_t _p5_extent_3 = _p5_buffer->extent[3];
 (void)_p5_extent_3;
 const int32_t _p5_stride_0 = _p5_buffer->stride[0];
 (void)_p5_stride_0;
 const int32_t _p5_stride_1 = _p5_buffer->stride[1];
 (void)_p5_stride_1;
 const int32_t _p5_stride_2 = _p5_buffer->stride[2];
 (void)_p5_stride_2;
 const int32_t _p5_stride_3 = _p5_buffer->stride[3];
 (void)_p5_stride_3;
 const int32_t _p5_elem_size = _p5_buffer->elem_size;
 (void)_p5_elem_size;
 uint8_t *_p4 = (uint8_t *)(_p4_buffer->host);
 (void)_p4;
 const bool _p4_host_and_dev_are_null = (_p4_buffer->host == NULL) && (_p4_buffer->dev == 0);
 (void)_p4_host_and_dev_are_null;
 const int32_t _p4_min_0 = _p4_buffer->min[0];
 (void)_p4_min_0;
 const int32_t _p4_min_1 = _p4_buffer->min[1];
 (void)_p4_min_1;
 const int32_t _p4_min_2 = _p4_buffer->min[2];
 (void)_p4_min_2;
 const int32_t _p4_min_3 = _p4_buffer->min[3];
 (void)_p4_min_3;
 const int32_t _p4_extent_0 = _p4_buffer->extent[0];
 (void)_p4_extent_0;
 const int32_t _p4_extent_1 = _p4_buffer->extent[1];
 (void)_p4_extent_1;
 const int32_t _p4_extent_2 = _p4_buffer->extent[2];
 (void)_p4_extent_2;
 const int32_t _p4_extent_3 = _p4_buffer->extent[3];
 (void)_p4_extent_3;
 const int32_t _p4_stride_0 = _p4_buffer->stride[0];
 (void)_p4_stride_0;
 const int32_t _p4_stride_1 = _p4_buffer->stride[1];
 (void)_p4_stride_1;
 const int32_t _p4_stride_2 = _p4_buffer->stride[2];
 (void)_p4_stride_2;
 const int32_t _p4_stride_3 = _p4_buffer->stride[3];
 (void)_p4_stride_3;
 const int32_t _p4_elem_size = _p4_buffer->elem_size;
 (void)_p4_elem_size;
 uint8_t *_p7 = (uint8_t *)(_p7_buffer->host);
 (void)_p7;
 const bool _p7_host_and_dev_are_null = (_p7_buffer->host == NULL) && (_p7_buffer->dev == 0);
 (void)_p7_host_and_dev_are_null;
 const int32_t _p7_min_0 = _p7_buffer->min[0];
 (void)_p7_min_0;
 const int32_t _p7_min_1 = _p7_buffer->min[1];
 (void)_p7_min_1;
 const int32_t _p7_min_2 = _p7_buffer->min[2];
 (void)_p7_min_2;
 const int32_t _p7_min_3 = _p7_buffer->min[3];
 (void)_p7_min_3;
 const int32_t _p7_extent_0 = _p7_buffer->extent[0];
 (void)_p7_extent_0;
 const int32_t _p7_extent_1 = _p7_buffer->extent[1];
 (void)_p7_extent_1;
 const int32_t _p7_extent_2 = _p7_buffer->extent[2];
 (void)_p7_extent_2;
 const int32_t _p7_extent_3 = _p7_buffer->extent[3];
 (void)_p7_extent_3;
 const int32_t _p7_stride_0 = _p7_buffer->stride[0];
 (void)_p7_stride_0;
 const int32_t _p7_stride_1 = _p7_buffer->stride[1];
 (void)_p7_stride_1;
 const int32_t _p7_stride_2 = _p7_buffer->stride[2];
 (void)_p7_stride_2;
 const int32_t _p7_stride_3 = _p7_buffer->stride[3];
 (void)_p7_stride_3;
 const int32_t _p7_elem_size = _p7_buffer->elem_size;
 (void)_p7_elem_size;
 uint8_t *_p6 = (uint8_t *)(_p6_buffer->host);
 (void)_p6;
 const bool _p6_host_and_dev_are_null = (_p6_buffer->host == NULL) && (_p6_buffer->dev == 0);
 (void)_p6_host_and_dev_are_null;
 const int32_t _p6_min_0 = _p6_buffer->min[0];
 (void)_p6_min_0;
 const int32_t _p6_min_1 = _p6_buffer->min[1];
 (void)_p6_min_1;
 const int32_t _p6_min_2 = _p6_buffer->min[2];
 (void)_p6_min_2;
 const int32_t _p6_min_3 = _p6_buffer->min[3];
 (void)_p6_min_3;
 const int32_t _p6_extent_0 = _p6_buffer->extent[0];
 (void)_p6_extent_0;
 const int32_t _p6_extent_1 = _p6_buffer->extent[1];
 (void)_p6_extent_1;
 const int32_t _p6_extent_2 = _p6_buffer->extent[2];
 (void)_p6_extent_2;
 const int32_t _p6_extent_3 = _p6_buffer->extent[3];
 (void)_p6_extent_3;
 const int32_t _p6_stride_0 = _p6_buffer->stride[0];
 (void)_p6_stride_0;
 const int32_t _p6_stride_1 = _p6_buffer->stride[1];
 (void)_p6_stride_1;
 const int32_t _p6_stride_2 = _p6_buffer->stride[2];
 (void)_p6_stride_2;
 const int32_t _p6_stride_3 = _p6_buffer->stride[3];
 (void)_p6_stride_3;
 const int32_t _p6_elem_size = _p6_buffer->elem_size;
 (void)_p6_elem_size;
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
 int32_t _14135 = __pipeline_hls(_p5_buffer, _p4_buffer, _p7_buffer, _p6_buffer, _output_2_buffer);
 bool _14136 = _14135 == 0;
 if (!_14136)  {
  return _14135;
 }
 return 0;
}
#ifdef __cplusplus
}  // extern "C"
#endif
