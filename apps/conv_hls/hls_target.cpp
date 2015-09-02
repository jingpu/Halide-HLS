#include "hls_target.h"

void _hls_target_f3_stream_stencil_stream(
hls::stream<Stencil<uint8_t, 1, 1, 1> > &_clamped_stream_stencil_stream,
hls::stream<Stencil<uint8_t, 1, 1, 1> > &_f3_stream_stencil_stream,
uint8_t _p2___bias,
Stencil<uint8_t, 5, 5> _weight_stencil)
{
 {
  hls::stream<Stencil<uint8_t, 5, 5, 1> > _repeat_edge__2_stencil_stream;
  // produce repeat_edge$2.stencil.stream
  {
   hls::stream<Stencil<uint8_t, 1, 1, 1> > _repeat_edge__2_stencil_update_stream;
   // produce repeat_edge$2.stencil_update.stream
   for (int _repeat_edge__2_scan_update__1 = 0; _repeat_edge__2_scan_update__1 < 0 + 264; _repeat_edge__2_scan_update__1++)
   {
    for (int _repeat_edge__2_scan_update__0 = 0; _repeat_edge__2_scan_update__0 < 0 + 264; _repeat_edge__2_scan_update__0++)
    {
     {
      Stencil<uint8_t, 1, 1, 1> _clamped_stream_stencil;
      // produce clamped_stream.stencil
      _clamped_stream_stencil = _clamped_stream_stencil_stream.read();
      (void)0;
      // consume clamped_stream.stencil
      {
       Stencil<uint8_t, 1, 1, 1> _repeat_edge__2_stencil_update;
       // produce repeat_edge$2.stencil_update
       uint8_t _987 = _clamped_stream_stencil(0, 0, 0);
       _repeat_edge__2_stencil_update(0, 0, 0) = _987;
       // consume repeat_edge$2.stencil_update
       _repeat_edge__2_stencil_update_stream.write(_repeat_edge__2_stencil_update);
       (void)0;
      } // realize _repeat_edge__2_stencil_update
     } // realize _clamped_stream_stencil
    } // for _repeat_edge__2_scan_update__0
   } // for _repeat_edge__2_scan_update__1
   // consume repeat_edge$2.stencil_update.stream
   linebuffer<264, 264, 1>(_repeat_edge__2_stencil_update_stream, _repeat_edge__2_stencil_stream);
   (void)0;
  } // realize _repeat_edge__2_stencil_update_stream
  // consume repeat_edge$2.stencil.stream
  {
   hls::stream<Stencil<uint8_t, 5, 5, 1> > _f2_stencil_stream;
   // produce f2.stencil.stream
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
        Stencil<uint8_t, 1, 1, 1> _f2_stencil_update;
        // produce f2.stencil_update
        {
         uint16_t _local_sum__3[1];
         // produce local_sum$3
         uint16_t _988 = (uint16_t)(_p2___bias);
         _local_sum__3[0] = _988;
         // update local_sum$3
         for (int _local_sum__3_s1_r__3_y__r = -2; _local_sum__3_s1_r__3_y__r < -2 + 5; _local_sum__3_s1_r__3_y__r++)
         {
          for (int _local_sum__3_s1_r__3_x__r = -2; _local_sum__3_s1_r__3_x__r < -2 + 5; _local_sum__3_s1_r__3_x__r++)
          {
           uint16_t _989 = _local_sum__3[0];
           int32_t _990 = _local_sum__3_s1_r__3_x__r + 2;
           int32_t _991 = _local_sum__3_s1_r__3_y__r + 2;
           uint8_t _992 = _repeat_edge__2_stencil(_990, _991, 0);
           uint16_t _993 = (uint16_t)(_992);
           uint8_t _994 = _weight_stencil(_990, _991);
           uint16_t _995 = (uint16_t)(_994);
           uint16_t _996 = _993 * _995;
           uint16_t _997 = _989 + _996;
           _local_sum__3[0] = _997;
          } // for _local_sum__3_s1_r__3_x__r
         } // for _local_sum__3_s1_r__3_y__r
         // consume local_sum$3
         uint16_t _998 = _local_sum__3[0];
         uint16_t _999 = _998 >> 8;
         uint8_t _1000 = (uint8_t)(_999);
         _f2_stencil_update(0, 0, 0) = _1000;
        } // alloc _local_sum__3
        // consume f2.stencil_update
        _f2_stencil_update_stream.write(_f2_stencil_update);
        (void)0;
       } // realize _f2_stencil_update
      } // realize _repeat_edge__2_stencil
     } // for _f2_scan_update_x
    } // for _f2_scan_update_y
    // consume f2.stencil_update.stream
    linebuffer<260, 260, 1>(_f2_stencil_update_stream, _f2_stencil_stream);
    (void)0;
   } // realize _f2_stencil_update_stream
   // consume f2.stencil.stream
   {
    hls::stream<Stencil<uint8_t, 1, 1, 1> > _f3_stream_stencil_update_stream;
    // produce f3_stream.stencil_update.stream
    for (int _f3_stream_scan_update_y = 0; _f3_stream_scan_update_y < 0 + 256; _f3_stream_scan_update_y++)
    {
     for (int _f3_stream_scan_update_x = 0; _f3_stream_scan_update_x < 0 + 256; _f3_stream_scan_update_x++)
     {
      {
       Stencil<uint8_t, 5, 5, 1> _f2_stencil;
       // produce f2.stencil
       _f2_stencil = _f2_stencil_stream.read();
       (void)0;
       // consume f2.stencil
       {
        Stencil<uint8_t, 1, 1, 1> _f3_stream_stencil_update;
        // produce f3_stream.stencil_update
        {
         uint16_t _local_sum__4[1];
         // produce local_sum$4
         uint16_t _1001 = (uint16_t)(_p2___bias);
         _local_sum__4[0] = _1001;
         // update local_sum$4
         for (int _local_sum__4_s1_r__4_y__r = -2; _local_sum__4_s1_r__4_y__r < -2 + 5; _local_sum__4_s1_r__4_y__r++)
         {
          for (int _local_sum__4_s1_r__4_x__r = -2; _local_sum__4_s1_r__4_x__r < -2 + 5; _local_sum__4_s1_r__4_x__r++)
          {
           uint16_t _1002 = _local_sum__4[0];
           int32_t _1003 = _local_sum__4_s1_r__4_x__r + 2;
           int32_t _1004 = _local_sum__4_s1_r__4_y__r + 2;
           uint8_t _1005 = _f2_stencil(_1003, _1004, 0);
           uint16_t _1006 = (uint16_t)(_1005);
           uint8_t _1007 = _weight_stencil(_1003, _1004);
           uint16_t _1008 = (uint16_t)(_1007);
           uint16_t _1009 = _1006 * _1008;
           uint16_t _1010 = _1002 + _1009;
           _local_sum__4[0] = _1010;
          } // for _local_sum__4_s1_r__4_x__r
         } // for _local_sum__4_s1_r__4_y__r
         // consume local_sum$4
         uint16_t _1011 = _local_sum__4[0];
         uint16_t _1012 = _1011 >> 8;
         uint8_t _1013 = (uint8_t)(_1012);
         _f3_stream_stencil_update(0, 0, 0) = _1013;
        } // alloc _local_sum__4
        // consume f3_stream.stencil_update
        _f3_stream_stencil_update_stream.write(_f3_stream_stencil_update);
        (void)0;
       } // realize _f3_stream_stencil_update
      } // realize _f2_stencil
     } // for _f3_stream_scan_update_x
    } // for _f3_stream_scan_update_y
    // consume f3_stream.stencil_update.stream
    linebuffer<256, 256, 1>(_f3_stream_stencil_update_stream, _f3_stream_stencil_stream);
    (void)0;
   } // realize _f3_stream_stencil_update_stream
  } // realize _f2_stencil_stream
 } // realize _repeat_edge__2_stencil_stream
} // kernel hls_target_hls_target_f3_stream_stencil_stream


