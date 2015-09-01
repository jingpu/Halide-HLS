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

static int __pipeline_hls(buffer_t *_input_buffer, buffer_t *_f3_buffer) HALIDE_FUNCTION_ATTRS {
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
 int32_t _995 = _f3_extent_0 + -1;
 int32_t _996 = _995 >> 8;
 int32_t _997 = _996 * 256;
 int32_t _998 = _997 + _f3_min_0;
 int32_t _999 = _998 + 255;
 int32_t _1000 = _f3_min_0 + _f3_extent_0;
 int32_t _1001 = _1000 + -1;
 int32_t _1002 = min(_999, _1001);
 int32_t _1003 = _1000 + -256;
 int32_t _1004 = min(_f3_min_0, _1003);
 int32_t _1005 = _1002 - _1004;
 int32_t _1006 = _f3_extent_1 + -1;
 int32_t _1007 = _1006 >> 8;
 int32_t _1008 = _1007 * 256;
 int32_t _1009 = _1008 + _f3_min_1;
 int32_t _1010 = _1009 + 255;
 int32_t _1011 = _f3_min_1 + _f3_extent_1;
 int32_t _1012 = _1011 + -1;
 int32_t _1013 = min(_1010, _1012);
 int32_t _1014 = _1011 + -256;
 int32_t _1015 = min(_f3_min_1, _1014);
 int32_t _1016 = _1013 - _1015;
 int32_t _1017 = _1005 + 1;
 int32_t _1018 = _1016 + 1;
 int32_t _1019 = _1017 * _1018;
 int32_t _1020 = _1000 + 3;
 int32_t _1021 = _input_min_0 + _input_extent_0;
 int32_t _1022 = _1021 + -1;
 int32_t _1023 = min(_1020, _1022);
 int32_t _1024 = max(_1023, _input_min_0);
 int32_t _1025 = _f3_min_0 + -4;
 int32_t _1026 = min(_1025, _1022);
 int32_t _1027 = max(_1026, _input_min_0);
 int32_t _1028 = _1024 - _1027;
 int32_t _1029 = _1011 + 3;
 int32_t _1030 = _input_min_1 + _input_extent_1;
 int32_t _1031 = _1030 + -1;
 int32_t _1032 = min(_1029, _1031);
 int32_t _1033 = max(_1032, _input_min_1);
 int32_t _1034 = _f3_min_1 + -4;
 int32_t _1035 = min(_1034, _1031);
 int32_t _1036 = max(_1035, _input_min_1);
 int32_t _1037 = _1033 - _1036;
 int32_t _1038 = _f3_min_2 + _f3_extent_2;
 int32_t _1039 = _input_min_2 + _input_extent_2;
 int32_t _1040 = min(_1038, _1039);
 int32_t _1041 = _1040 + -1;
 int32_t _1042 = max(_1041, _input_min_2);
 int32_t _1043 = _1039 + -1;
 int32_t _1044 = min(_f3_min_2, _1043);
 int32_t _1045 = max(_1044, _input_min_2);
 int32_t _1046 = _1042 - _1045;
 int32_t _1047 = _1028 + 1;
 int32_t _1048 = _1037 + 1;
 int32_t _1049 = _1047 * _1048;
 if (_f3_host_and_dev_are_null)
 {
  int32_t _1050 = _1005 + 1;
  int32_t _1051 = _1016 + 1;
  bool _1052 = halide_rewrite_buffer(_f3_buffer, 1, _1004, _1050, 1, _1015, _1051, _1050, _f3_min_2, _f3_extent_2, _1019, 0, 0, 0);
  (void)_1052;
 } // if _f3_host_and_dev_are_null
 if (_input_host_and_dev_are_null)
 {
  int32_t _1053 = _1028 + 1;
  int32_t _1054 = _1037 + 1;
  int32_t _1055 = _1046 + 1;
  bool _1056 = halide_rewrite_buffer(_input_buffer, 1, _1027, _1053, 1, _1036, _1054, _1053, _1045, _1055, _1049, 0, 0, 0);
  (void)_1056;
 } // if _input_host_and_dev_are_null
 bool _1057 = _f3_host_and_dev_are_null || _input_host_and_dev_are_null;
 bool _1058 = !(_1057);
 if (_1058)
 {
  bool _1059 = _f3_elem_size == 1;
  if (!_1059)   {
   int32_t _1060 = halide_error_bad_elem_size(NULL, "Output buffer f3", "uint8", _f3_elem_size, 1);
   return _1060;
  }
  bool _1061 = _input_elem_size == 1;
  if (!_1061)   {
   int32_t _1062 = halide_error_bad_elem_size(NULL, "Input buffer input", "uint8", _input_elem_size, 1);
   return _1062;
  }
  bool _1063 = _f3_min_0 <= _1004;
  int32_t _1064 = _1004 + _1005;
  int32_t _1065 = _1064 - _f3_extent_0;
  int32_t _1066 = _1065 + 1;
  bool _1067 = _1066 <= _f3_min_0;
  bool _1068 = _1063 && _1067;
  if (!_1068)   {
   int32_t _1069 = _1004 + _1005;
   int32_t _1070 = _f3_min_0 + _f3_extent_0;
   int32_t _1071 = _1070 + -1;
   int32_t _1072 = halide_error_access_out_of_bounds(NULL, "Output buffer f3", 0, _1004, _1069, _f3_min_0, _1071);
   return _1072;
  }
  bool _1073 = _f3_min_1 <= _1015;
  int32_t _1074 = _1015 + _1016;
  int32_t _1075 = _1074 - _f3_extent_1;
  int32_t _1076 = _1075 + 1;
  bool _1077 = _1076 <= _f3_min_1;
  bool _1078 = _1073 && _1077;
  if (!_1078)   {
   int32_t _1079 = _1015 + _1016;
   int32_t _1080 = _f3_min_1 + _f3_extent_1;
   int32_t _1081 = _1080 + -1;
   int32_t _1082 = halide_error_access_out_of_bounds(NULL, "Output buffer f3", 1, _1015, _1079, _f3_min_1, _1081);
   return _1082;
  }
  bool _1083 = _input_min_0 <= _1027;
  int32_t _1084 = _1027 + _1028;
  int32_t _1085 = _1084 - _input_extent_0;
  int32_t _1086 = _1085 + 1;
  bool _1087 = _1086 <= _input_min_0;
  bool _1088 = _1083 && _1087;
  if (!_1088)   {
   int32_t _1089 = _1027 + _1028;
   int32_t _1090 = _input_min_0 + _input_extent_0;
   int32_t _1091 = _1090 + -1;
   int32_t _1092 = halide_error_access_out_of_bounds(NULL, "Input buffer input", 0, _1027, _1089, _input_min_0, _1091);
   return _1092;
  }
  bool _1093 = _input_min_1 <= _1036;
  int32_t _1094 = _1036 + _1037;
  int32_t _1095 = _1094 - _input_extent_1;
  int32_t _1096 = _1095 + 1;
  bool _1097 = _1096 <= _input_min_1;
  bool _1098 = _1093 && _1097;
  if (!_1098)   {
   int32_t _1099 = _1036 + _1037;
   int32_t _1100 = _input_min_1 + _input_extent_1;
   int32_t _1101 = _1100 + -1;
   int32_t _1102 = halide_error_access_out_of_bounds(NULL, "Input buffer input", 1, _1036, _1099, _input_min_1, _1101);
   return _1102;
  }
  bool _1103 = _input_min_2 <= _1045;
  int32_t _1104 = _1045 + _1046;
  int32_t _1105 = _1104 - _input_extent_2;
  int32_t _1106 = _1105 + 1;
  bool _1107 = _1106 <= _input_min_2;
  bool _1108 = _1103 && _1107;
  if (!_1108)   {
   int32_t _1109 = _1045 + _1046;
   int32_t _1110 = _input_min_2 + _input_extent_2;
   int32_t _1111 = _1110 + -1;
   int32_t _1112 = halide_error_access_out_of_bounds(NULL, "Input buffer input", 2, _1045, _1109, _input_min_2, _1111);
   return _1112;
  }
  bool _1113 = _f3_stride_0 == 1;
  if (!_1113)   {
   int32_t _1114 = halide_error_constraint_violated(NULL, "f3.stride.0", _f3_stride_0, "1", 1);
   return _1114;
  }
  bool _1115 = _input_stride_0 == 1;
  if (!_1115)   {
   int32_t _1116 = halide_error_constraint_violated(NULL, "input.stride.0", _input_stride_0, "1", 1);
   return _1116;
  }
  int64_t _1117 = (int64_t)(_f3_extent_1);
  int64_t _1118 = (int64_t)(_f3_extent_0);
  int64_t _1119 = _1117 * _1118;
  int64_t _1120 = (int64_t)(_input_extent_1);
  int64_t _1121 = (int64_t)(_input_extent_0);
  int64_t _1122 = _1120 * _1121;
  int64_t _1123 = (int64_t)(2147483647);
  bool _1124 = _1118 <= _1123;
  if (!_1124)   {
   int64_t _1125 = (int64_t)(_f3_extent_0);
   int64_t _1126 = (int64_t)(2147483647);
   int32_t _1127 = halide_error_buffer_allocation_too_large(NULL, "f3", _1125, _1126);
   return _1127;
  }
  int64_t _1128 = (int64_t)(_f3_extent_1);
  int64_t _1129 = (int64_t)(_f3_stride_1);
  int64_t _1130 = _1128 * _1129;
  int64_t _1131 = (int64_t)(2147483647);
  bool _1132 = _1130 <= _1131;
  if (!_1132)   {
   int64_t _1133 = (int64_t)(_f3_extent_1);
   int64_t _1134 = (int64_t)(_f3_stride_1);
   int64_t _1135 = _1133 * _1134;
   int64_t _1136 = (int64_t)(2147483647);
   int32_t _1137 = halide_error_buffer_allocation_too_large(NULL, "f3", _1135, _1136);
   return _1137;
  }
  int64_t _1138 = (int64_t)(2147483647);
  bool _1139 = _1119 <= _1138;
  if (!_1139)   {
   int64_t _1140 = (int64_t)(2147483647);
   int32_t _1141 = halide_error_buffer_extents_too_large(NULL, "f3", _1119, _1140);
   return _1141;
  }
  int64_t _1142 = (int64_t)(_f3_extent_2);
  int64_t _1143 = (int64_t)(_f3_stride_2);
  int64_t _1144 = _1142 * _1143;
  int64_t _1145 = (int64_t)(2147483647);
  bool _1146 = _1144 <= _1145;
  if (!_1146)   {
   int64_t _1147 = (int64_t)(_f3_extent_2);
   int64_t _1148 = (int64_t)(_f3_stride_2);
   int64_t _1149 = _1147 * _1148;
   int64_t _1150 = (int64_t)(2147483647);
   int32_t _1151 = halide_error_buffer_allocation_too_large(NULL, "f3", _1149, _1150);
   return _1151;
  }
  int64_t _1152 = (int64_t)(_f3_extent_2);
  int64_t _1153 = _1152 * _1119;
  int64_t _1154 = (int64_t)(2147483647);
  bool _1155 = _1153 <= _1154;
  if (!_1155)   {
   int64_t _1156 = (int64_t)(_f3_extent_2);
   int64_t _1157 = _1156 * _1119;
   int64_t _1158 = (int64_t)(2147483647);
   int32_t _1159 = halide_error_buffer_extents_too_large(NULL, "f3", _1157, _1158);
   return _1159;
  }
  int64_t _1160 = (int64_t)(_input_extent_0);
  int64_t _1161 = (int64_t)(2147483647);
  bool _1162 = _1160 <= _1161;
  if (!_1162)   {
   int64_t _1163 = (int64_t)(_input_extent_0);
   int64_t _1164 = (int64_t)(2147483647);
   int32_t _1165 = halide_error_buffer_allocation_too_large(NULL, "input", _1163, _1164);
   return _1165;
  }
  int64_t _1166 = (int64_t)(_input_extent_1);
  int64_t _1167 = (int64_t)(_input_stride_1);
  int64_t _1168 = _1166 * _1167;
  int64_t _1169 = (int64_t)(2147483647);
  bool _1170 = _1168 <= _1169;
  if (!_1170)   {
   int64_t _1171 = (int64_t)(_input_extent_1);
   int64_t _1172 = (int64_t)(_input_stride_1);
   int64_t _1173 = _1171 * _1172;
   int64_t _1174 = (int64_t)(2147483647);
   int32_t _1175 = halide_error_buffer_allocation_too_large(NULL, "input", _1173, _1174);
   return _1175;
  }
  int64_t _1176 = (int64_t)(2147483647);
  bool _1177 = _1122 <= _1176;
  if (!_1177)   {
   int64_t _1178 = (int64_t)(2147483647);
   int32_t _1179 = halide_error_buffer_extents_too_large(NULL, "input", _1122, _1178);
   return _1179;
  }
  int64_t _1180 = (int64_t)(_input_extent_2);
  int64_t _1181 = (int64_t)(_input_stride_2);
  int64_t _1182 = _1180 * _1181;
  int64_t _1183 = (int64_t)(2147483647);
  bool _1184 = _1182 <= _1183;
  if (!_1184)   {
   int64_t _1185 = (int64_t)(_input_extent_2);
   int64_t _1186 = (int64_t)(_input_stride_2);
   int64_t _1187 = _1185 * _1186;
   int64_t _1188 = (int64_t)(2147483647);
   int32_t _1189 = halide_error_buffer_allocation_too_large(NULL, "input", _1187, _1188);
   return _1189;
  }
  int64_t _1190 = (int64_t)(_input_extent_2);
  int64_t _1191 = _1190 * _1122;
  int64_t _1192 = (int64_t)(2147483647);
  bool _1193 = _1191 <= _1192;
  if (!_1193)   {
   int64_t _1194 = (int64_t)(_input_extent_2);
   int64_t _1195 = _1194 * _1122;
   int64_t _1196 = (int64_t)(2147483647);
   int32_t _1197 = halide_error_buffer_extents_too_large(NULL, "input", _1195, _1196);
   return _1197;
  }
  int32_t _1198 = _f3_extent_1 + -1;
  int32_t _1199 = _1198 >> 8;
  int32_t _1200 = _1199 * 256;
  int32_t _1201 = _1200 + _f3_min_1;
  int32_t _1202 = _1201 + 255;
  int32_t _1203 = _f3_min_1 + _f3_extent_1;
  int32_t _1204 = _1203 + -1;
  int32_t _1205 = min(_1202, _1204);
  int32_t _1206 = _1205 + 4;
  int32_t _1207 = _1203 + 3;
  int32_t _1208 = max(_1206, _1207);
  int32_t _1209 = _f3_extent_0 + -1;
  int32_t _1210 = _1209 >> 8;
  int32_t _1211 = _1210 * 256;
  int32_t _1212 = _1211 + _f3_min_0;
  int32_t _1213 = _1212 + 255;
  int32_t _1214 = _f3_min_0 + _f3_extent_0;
  int32_t _1215 = _1214 + -1;
  int32_t _1216 = min(_1213, _1215);
  int32_t _1217 = _1216 + 4;
  int32_t _1218 = _1214 + 3;
  int32_t _1219 = max(_1217, _1218);
  int32_t _1220 = _1219 - _1004;
  int32_t _1221 = _1220 + 5;
  int32_t _1222 = _1208 - _1015;
  int32_t _1223 = _1222 + 5;
  int32_t _1224 = _1221 * _1223;
  {
   int32_t _1225 = _1219 - _1004;
   int32_t _1226 = _1225 + 5;
   int64_t _1227 = _1226;
   int32_t _1228 = _1208 - _1015;
   int32_t _1229 = _1228 + 5;
   int64_t _1230 = _1227 * _1229;
   int64_t _1231 = (_1230 > ((int64_t(1) << 31) - 1)) ? _1230 : (_1230 * _f3_extent_2);
   if ((_1231 > ((int64_t(1) << 31) - 1)) || ((_1231 * sizeof(uint8_t)) > ((int64_t(1) << 31) - 1)))
   {
    halide_error(NULL, "32-bit signed overflow computing size of allocation clamped_buf\n");
    return -1;
   } // overflow test clamped_buf
   int64_t _1232 = _1231;
   uint8_t *_clamped_buf = (uint8_t *)halide_malloc(NULL, sizeof(uint8_t)*_1232);
   // produce clamped_buf
   for (int _clamped_buf_s0__2 = _f3_min_2; _clamped_buf_s0__2 < _f3_min_2 + _f3_extent_2; _clamped_buf_s0__2++)
   {
    int32_t _1233 = _f3_min_1 + -4;
    int32_t _1234 = _f3_extent_1 + 8;
    for (int _clamped_buf_s0__1 = _1233; _clamped_buf_s0__1 < _1233 + _1234; _clamped_buf_s0__1++)
    {
     int32_t _1235 = _f3_min_0 + _f3_extent_0;
     int32_t _1236 = _1235 + 4;
     int32_t _1237 = min(_input_min_0, _1236);
     int32_t _1238 = _f3_min_0 + -4;
     int32_t _1239 = max(_1237, _1238);
     int32_t _1240 = _input_min_0 + _input_extent_0;
     int32_t _1241 = min(_1240, _1236);
     int32_t _1242 = max(_1241, _1239);
     int32_t _1243 = _1239 - _f3_min_0;
     int32_t _1244 = _1243 + 4;
     for (int _clamped_buf_s0__0 = _1238; _clamped_buf_s0__0 < _1238 + _1244; _clamped_buf_s0__0++)
     {
      int32_t _1245 = _clamped_buf_s0__0 - _1004;
      int32_t _1246 = _clamped_buf_s0__1 - _1015;
      int32_t _1247 = _1246 + 4;
      int32_t _1248 = _1219 - _1004;
      int32_t _1249 = _1248 + 5;
      int32_t _1250 = _1247 * _1249;
      int32_t _1251 = _1245 + _1250;
      int32_t _1252 = _clamped_buf_s0__2 - _f3_min_2;
      int32_t _1253 = _1252 * _1224;
      int32_t _1254 = _1251 + _1253;
      int32_t _1255 = _1254 + 4;
      int32_t _1256 = _input_min_0 + _input_extent_0;
      int32_t _1257 = _1256 + -1;
      int32_t _1258 = min(_clamped_buf_s0__0, _1257);
      int32_t _1259 = max(_1258, _input_min_0);
      int32_t _1260 = _input_min_1 + _input_extent_1;
      int32_t _1261 = _1260 + -1;
      int32_t _1262 = min(_clamped_buf_s0__1, _1261);
      int32_t _1263 = max(_1262, _input_min_1);
      int32_t _1264 = _1263 * _input_stride_1;
      int32_t _1265 = _1259 + _1264;
      int32_t _1266 = _input_min_2 + _input_extent_2;
      int32_t _1267 = _1266 + -1;
      int32_t _1268 = min(_clamped_buf_s0__2, _1267);
      int32_t _1269 = max(_1268, _input_min_2);
      int32_t _1270 = _1269 * _input_stride_2;
      int32_t _1271 = _1265 + _1270;
      int32_t _1272 = _input_min_1 * _input_stride_1;
      int32_t _1273 = _input_min_0 + _1272;
      int32_t _1274 = _input_min_2 * _input_stride_2;
      int32_t _1275 = _1273 + _1274;
      int32_t _1276 = _1271 - _1275;
      uint8_t _1277 = _input[_1276];
      _clamped_buf[_1255] = _1277;
     } // for _clamped_buf_s0__0
     int32_t _1278 = _1242 - _1239;
     for (int _clamped_buf_s0__0 = _1239; _clamped_buf_s0__0 < _1239 + _1278; _clamped_buf_s0__0++)
     {
      int32_t _1279 = _clamped_buf_s0__0 - _1004;
      int32_t _1280 = _clamped_buf_s0__1 - _1015;
      int32_t _1281 = _1280 + 4;
      int32_t _1282 = _1219 - _1004;
      int32_t _1283 = _1282 + 5;
      int32_t _1284 = _1281 * _1283;
      int32_t _1285 = _1279 + _1284;
      int32_t _1286 = _clamped_buf_s0__2 - _f3_min_2;
      int32_t _1287 = _1286 * _1224;
      int32_t _1288 = _1285 + _1287;
      int32_t _1289 = _1288 + 4;
      int32_t _1290 = _input_min_1 + _input_extent_1;
      int32_t _1291 = _1290 + -1;
      int32_t _1292 = min(_clamped_buf_s0__1, _1291);
      int32_t _1293 = max(_1292, _input_min_1);
      int32_t _1294 = _1293 * _input_stride_1;
      int32_t _1295 = _clamped_buf_s0__0 + _1294;
      int32_t _1296 = _input_min_2 + _input_extent_2;
      int32_t _1297 = _1296 + -1;
      int32_t _1298 = min(_clamped_buf_s0__2, _1297);
      int32_t _1299 = max(_1298, _input_min_2);
      int32_t _1300 = _1299 * _input_stride_2;
      int32_t _1301 = _1295 + _1300;
      int32_t _1302 = _input_min_1 * _input_stride_1;
      int32_t _1303 = _input_min_0 + _1302;
      int32_t _1304 = _input_min_2 * _input_stride_2;
      int32_t _1305 = _1303 + _1304;
      int32_t _1306 = _1301 - _1305;
      uint8_t _1307 = _input[_1306];
      _clamped_buf[_1289] = _1307;
     } // for _clamped_buf_s0__0
     int32_t _1308 = _f3_min_0 + _f3_extent_0;
     int32_t _1309 = _1308 - _1242;
     int32_t _1310 = _1309 + 4;
     for (int _clamped_buf_s0__0 = _1242; _clamped_buf_s0__0 < _1242 + _1310; _clamped_buf_s0__0++)
     {
      int32_t _1311 = _clamped_buf_s0__0 - _1004;
      int32_t _1312 = _clamped_buf_s0__1 - _1015;
      int32_t _1313 = _1312 + 4;
      int32_t _1314 = _1219 - _1004;
      int32_t _1315 = _1314 + 5;
      int32_t _1316 = _1313 * _1315;
      int32_t _1317 = _1311 + _1316;
      int32_t _1318 = _clamped_buf_s0__2 - _f3_min_2;
      int32_t _1319 = _1318 * _1224;
      int32_t _1320 = _1317 + _1319;
      int32_t _1321 = _1320 + 4;
      int32_t _1322 = _input_min_0 + _input_extent_0;
      int32_t _1323 = _1322 + -1;
      int32_t _1324 = max(_1323, _input_min_0);
      int32_t _1325 = _input_min_1 + _input_extent_1;
      int32_t _1326 = _1325 + -1;
      int32_t _1327 = min(_clamped_buf_s0__1, _1326);
      int32_t _1328 = max(_1327, _input_min_1);
      int32_t _1329 = _1328 * _input_stride_1;
      int32_t _1330 = _1324 + _1329;
      int32_t _1331 = _input_min_2 + _input_extent_2;
      int32_t _1332 = _1331 + -1;
      int32_t _1333 = min(_clamped_buf_s0__2, _1332);
      int32_t _1334 = max(_1333, _input_min_2);
      int32_t _1335 = _1334 * _input_stride_2;
      int32_t _1336 = _1330 + _1335;
      int32_t _1337 = _input_min_1 * _input_stride_1;
      int32_t _1338 = _input_min_0 + _1337;
      int32_t _1339 = _input_min_2 * _input_stride_2;
      int32_t _1340 = _1338 + _1339;
      int32_t _1341 = _1336 - _1340;
      uint8_t _1342 = _input[_1341];
      _clamped_buf[_1321] = _1342;
     } // for _clamped_buf_s0__0
    } // for _clamped_buf_s0__1
   } // for _clamped_buf_s0__2
   // consume clamped_buf
   // produce f3
   for (int _f3_s0_c = _f3_min_2; _f3_s0_c < _f3_min_2 + _f3_extent_2; _f3_s0_c++)
   {
    int32_t _1343 = _f3_extent_1 + 255;
    int32_t _1344 = _1343 >> 8;
    for (int _f3_s0_y_yo = 0; _f3_s0_y_yo < 0 + _1344; _f3_s0_y_yo++)
    {
     int32_t _1345 = _f3_s0_y_yo * 256;
     int32_t _1346 = _1345 + _f3_min_1;
     int32_t _1347 = _f3_min_1 + _f3_extent_1;
     int32_t _1348 = _1347 + -256;
     int32_t _1349 = min(_1346, _1348);
     int32_t _1350 = _f3_extent_0 + 255;
     int32_t _1351 = _1350 >> 8;
     for (int _f3_s0_x_xo = 0; _f3_s0_x_xo < 0 + _1351; _f3_s0_x_xo++)
     {
      int32_t _1352 = _f3_s0_x_xo * 256;
      int32_t _1353 = _1352 + _f3_min_0;
      int32_t _1354 = _f3_min_0 + _f3_extent_0;
      int32_t _1355 = _1354 + -256;
      int32_t _1356 = min(_1353, _1355);
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
           int32_t _1357 = _1356 + _clamped_stream_scan_update__0;
           int32_t _1358 = _1357 - _1004;
           int32_t _1359 = _1349 + _clamped_stream_scan_update__1;
           int32_t _1360 = _1359 - _1015;
           int32_t _1361 = _1219 - _1004;
           int32_t _1362 = _1361 + 5;
           int32_t _1363 = _1360 * _1362;
           int32_t _1364 = _1358 + _1363;
           int32_t _1365 = _f3_s0_c - _f3_min_2;
           int32_t _1366 = _1365 * _1224;
           int32_t _1367 = _1364 + _1366;
           uint8_t _1368 = _clamped_buf[_1367];
           _clamped_stream_stencil_update(0, 0, 0) = _1368;
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
        // produce f3_stream.stencil.stream
        hls_target_f3_stream_stencil_stream(_clamped_stream_stencil_stream, _f3_stream_stencil_stream);
        // consume f3_stream.stencil.stream
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
           int32_t _1574 = _1356 + _f3_s0_x_xi;
           int32_t _1575 = _1349 + _f3_s0_y_yi;
           int32_t _1576 = _1575 * _f3_stride_1;
           int32_t _1577 = _1574 + _1576;
           int32_t _1578 = _f3_s0_c * _f3_stride_2;
           int32_t _1579 = _1577 + _1578;
           int32_t _1580 = _f3_min_1 * _f3_stride_1;
           int32_t _1581 = _f3_min_0 + _1580;
           int32_t _1582 = _f3_min_2 * _f3_stride_2;
           int32_t _1583 = _1581 + _1582;
           int32_t _1584 = _1579 - _1583;
           uint8_t _1585 = _f3_stream_stencil(0, 0, 0);
           _f3[_1584] = _1585;
          } // realize _f3_stream_stencil
         } // for _f3_s0_x_xi
        } // for _f3_s0_y_yi
       } // realize _f3_stream_stencil_stream
      } // realize _clamped_stream_stencil_stream
     } // for _f3_s0_x_xo
    } // for _f3_s0_y_yo
   } // for _f3_s0_c
   halide_free(NULL, _clamped_buf);
   // consume f3
  } // alloc _clamped_buf
 } // if _1058
 return 0;
}


int pipeline_hls(buffer_t *_input_buffer, buffer_t *_f3_buffer) HALIDE_FUNCTION_ATTRS {
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
 int32_t _1586 = __pipeline_hls(_input_buffer, _f3_buffer);
 bool _1587 = _1586 == 0;
 if (!_1587)  {
  return _1586;
 }
 return 0;
}
#ifdef __cplusplus
}  // extern "C"
#endif
