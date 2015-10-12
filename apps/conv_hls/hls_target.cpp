#include "hls_target.h"

#include "Linebuffer.h"

inline int8_t abs_i8(int8_t a) {return a >= 0 ? a : -a;}
inline int16_t abs_i16(int16_t a) {return a >= 0 ? a : -a;}
inline int32_t abs_i32(int32_t a) {return a >= 0 ? a : -a;}
inline int64_t abs_i64(int64_t a) {return a >= 0 ? a : -a;}

template<typename T> T max(T a, T b) {if (a > b) return a; return b;}
template<typename T> T min(T a, T b) {if (a < b) return a; return b;}
template<typename T> T smod(T a, T b) {T result = a % b; if (result < 0) result += b < 0 ? -b : b; return result;}
template<typename T> T sdiv(T a, T b) {T q = a / b; T r = a - q*b; int bs = b >> (8*sizeof(T) - 1); int rs = r >> (8*sizeof(T) - 1); return q - (rs & bs) + (rs & ~bs);}

void _hls_target_f3_stencil_stream(
hls::stream<Stencil<uint8_t, 1, 1, 1> > &_f3_stencil_stream,
uint8_t _p2___bias,
hls::stream<Stencil<uint8_t, 1, 1, 1> > &_repeat_edge__2_stencil_update_stream,
Stencil<uint8_t, 5, 5> _weight_stencil)
{
 {
  hls::stream<Stencil<uint8_t, 5, 5, 1> > _repeat_edge__2_stencil_stream;
  // produce repeat_edge$2.stencil.stream
  linebuffer<264, 264, 1>(_repeat_edge__2_stencil_update_stream, _repeat_edge__2_stencil_stream);
  (void)0;
  // consume repeat_edge$2.stencil.stream
  {
   hls::stream<Stencil<uint8_t, 1, 1, 1> > _f2_stencil_update_stream;
   // produce f2.stencil_update.stream
   for (int _f2_scan_update_y = 0; _f2_scan_update_y < 0 + 260; _f2_scan_update_y++)
   {
    for (int _f2_scan_update_x = 0; _f2_scan_update_x < 0 + 260; _f2_scan_update_x++)
    {
     {
      Stencil<uint8_t, 5, 5, 1> _repeat_edge__2_stencil;
      // produce repeat_edge$2.stencil
      _repeat_edge__2_stencil = _repeat_edge__2_stencil_stream.read();
      (void)0;
      // consume repeat_edge$2.stencil
      {
       Stencil<uint8_t, 1, 1, 1> _f2_stencil;
       // produce f2.stencil
       {
        Stencil<uint16_t, 1, 1, 1> _local_sum__3_stencil;
        // produce local_sum$3.stencil
        uint16_t _331 = (uint16_t)(_p2___bias);
        _local_sum__3_stencil(0, 0, 0) = _331;
        // update local_sum$3.stencil
        for (int _local_sum__3_s1_r__3_y__r = -2; _local_sum__3_s1_r__3_y__r < -2 + 5; _local_sum__3_s1_r__3_y__r++)
        {
         for (int _local_sum__3_s1_r__3_x__r = -2; _local_sum__3_s1_r__3_x__r < -2 + 5; _local_sum__3_s1_r__3_x__r++)
         {
          uint16_t _332 = _local_sum__3_stencil(0, 0, 0);
          int32_t _333 = _local_sum__3_s1_r__3_x__r + 2;
          int32_t _334 = _local_sum__3_s1_r__3_y__r + 2;
          uint8_t _335 = _repeat_edge__2_stencil(_333, _334, 0);
          uint16_t _336 = (uint16_t)(_335);
          uint8_t _337 = _weight_stencil(_333, _334);
          uint16_t _338 = (uint16_t)(_337);
          uint16_t _339 = _336 * _338;
          uint16_t _340 = _332 + _339;
          _local_sum__3_stencil(0, 0, 0) = _340;
         } // for _local_sum__3_s1_r__3_x__r
        } // for _local_sum__3_s1_r__3_y__r
        // consume local_sum$3.stencil
        uint16_t _341 = _local_sum__3_stencil(0, 0, 0);
        uint16_t _342 = _341 >> 8;
        uint8_t _343 = (uint8_t)(_342);
        _f2_stencil(0, 0, 0) = _343;
       } // realize _local_sum__3_stencil
       // consume f2.stencil
       _f2_stencil_update_stream.write(_f2_stencil);
       (void)0;
      } // realize _f2_stencil
     } // realize _repeat_edge__2_stencil
    } // for _f2_scan_update_x
   } // for _f2_scan_update_y
   // consume f2.stencil_update.stream
   {
    hls::stream<Stencil<uint8_t, 5, 5, 1> > _f2_stencil_stream;
    // produce f2.stencil.stream
    linebuffer<260, 260, 1>(_f2_stencil_update_stream, _f2_stencil_stream);
    (void)0;
    // consume f2.stencil.stream
    for (int _output__2_s0_y_yi = 0; _output__2_s0_y_yi < 0 + 256; _output__2_s0_y_yi++)
    {
     for (int _output__2_s0_x_xi = 0; _output__2_s0_x_xi < 0 + 256; _output__2_s0_x_xi++)
     {
      {
       Stencil<uint8_t, 5, 5, 1> _f2_stencil;
       // produce f2.stencil
       _f2_stencil = _f2_stencil_stream.read();
       (void)0;
       // consume f2.stencil
       {
        Stencil<uint8_t, 1, 1, 1> _f3_stencil;
        // produce f3.stencil
        {
         Stencil<uint16_t, 1, 1, 1> _local_sum__4_stencil;
         // produce local_sum$4.stencil
         uint16_t _344 = (uint16_t)(_p2___bias);
         _local_sum__4_stencil(0, 0, 0) = _344;
         // update local_sum$4.stencil
         for (int _local_sum__4_s1_r__4_y__r = -2; _local_sum__4_s1_r__4_y__r < -2 + 5; _local_sum__4_s1_r__4_y__r++)
         {
          for (int _local_sum__4_s1_r__4_x__r = -2; _local_sum__4_s1_r__4_x__r < -2 + 5; _local_sum__4_s1_r__4_x__r++)
          {
           uint16_t _345 = _local_sum__4_stencil(0, 0, 0);
           int32_t _346 = _local_sum__4_s1_r__4_x__r + 2;
           int32_t _347 = _local_sum__4_s1_r__4_y__r + 2;
           uint8_t _348 = _f2_stencil(_346, _347, 0);
           uint16_t _349 = (uint16_t)(_348);
           uint8_t _350 = _weight_stencil(_346, _347);
           uint16_t _351 = (uint16_t)(_350);
           uint16_t _352 = _349 * _351;
           uint16_t _353 = _345 + _352;
           _local_sum__4_stencil(0, 0, 0) = _353;
          } // for _local_sum__4_s1_r__4_x__r
         } // for _local_sum__4_s1_r__4_y__r
         // consume local_sum$4.stencil
         uint16_t _354 = _local_sum__4_stencil(0, 0, 0);
         uint16_t _355 = _354 >> 8;
         uint8_t _356 = (uint8_t)(_355);
         _f3_stencil(0, 0, 0) = _356;
        } // realize _local_sum__4_stencil
        // consume f3.stencil
        _f3_stencil_stream.write(_f3_stencil);
        (void)0;
       } // realize _f3_stencil
      } // realize _f2_stencil
     } // for _output__2_s0_x_xi
    } // for _output__2_s0_y_yi
   } // realize _f2_stencil_stream
  } // realize _f2_stencil_update_stream
 } // realize _repeat_edge__2_stencil_stream
} // kernel hls_target_hls_target_f3_stencil_stream


