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
        uint16_t _353 = (uint16_t)(_p2___bias);
        _local_sum__3_stencil(0, 0, 0) = _353;
        // update local_sum$3.stencil
        for (int _local_sum__3_s1_r__3_y__r = -2; _local_sum__3_s1_r__3_y__r < -2 + 5; _local_sum__3_s1_r__3_y__r++)
        {
         for (int _local_sum__3_s1_r__3_x__r = -2; _local_sum__3_s1_r__3_x__r < -2 + 5; _local_sum__3_s1_r__3_x__r++)
         {
          uint16_t _354 = _local_sum__3_stencil(0, 0, 0);
          int32_t _355 = _local_sum__3_s1_r__3_x__r + 2;
          int32_t _356 = _local_sum__3_s1_r__3_y__r + 2;
          uint8_t _357 = _repeat_edge__2_stencil(_355, _356, 0);
          uint16_t _358 = (uint16_t)(_357);
          uint8_t _359 = _weight_stencil(_355, _356);
          uint16_t _360 = (uint16_t)(_359);
          uint16_t _361 = _358 * _360;
          uint16_t _362 = _354 + _361;
          _local_sum__3_stencil(0, 0, 0) = _362;
         } // for _local_sum__3_s1_r__3_x__r
        } // for _local_sum__3_s1_r__3_y__r
        // consume local_sum$3.stencil
        uint16_t _363 = _local_sum__3_stencil(0, 0, 0);
        uint16_t _364 = _363 >> 8;
        uint8_t _365 = (uint8_t)(_364);
        _f2_stencil(0, 0, 0) = _365;
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
         uint16_t _366 = (uint16_t)(_p2___bias);
         _local_sum__4_stencil(0, 0, 0) = _366;
         // update local_sum$4.stencil
         for (int _local_sum__4_s1_r__4_y__r = -2; _local_sum__4_s1_r__4_y__r < -2 + 5; _local_sum__4_s1_r__4_y__r++)
         {
          for (int _local_sum__4_s1_r__4_x__r = -2; _local_sum__4_s1_r__4_x__r < -2 + 5; _local_sum__4_s1_r__4_x__r++)
          {
           uint16_t _367 = _local_sum__4_stencil(0, 0, 0);
           int32_t _368 = _local_sum__4_s1_r__4_x__r + 2;
           int32_t _369 = _local_sum__4_s1_r__4_y__r + 2;
           uint8_t _370 = _f2_stencil(_368, _369, 0);
           uint16_t _371 = (uint16_t)(_370);
           uint8_t _372 = _weight_stencil(_368, _369);
           uint16_t _373 = (uint16_t)(_372);
           uint16_t _374 = _371 * _373;
           uint16_t _375 = _367 + _374;
           _local_sum__4_stencil(0, 0, 0) = _375;
          } // for _local_sum__4_s1_r__4_x__r
         } // for _local_sum__4_s1_r__4_y__r
         // consume local_sum$4.stencil
         uint16_t _376 = _local_sum__4_stencil(0, 0, 0);
         uint16_t _377 = _376 >> 8;
         uint8_t _378 = (uint8_t)(_377);
         _f3_stencil(0, 0, 0) = _378;
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


