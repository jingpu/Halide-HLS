#include "hls_target.h"

#include "Linebuffer.h"

template<typename T> T max(T a, T b) {if (a > b) return a; return b;}
template<typename T> T min(T a, T b) {if (a < b) return a; return b;}

void p_hls_target_hw_output_2_stencil_stream(
hls::stream<PackedStencil<uint8_t, 1, 1, 1, 1> > &_hw_output_2_stencil_stream,
hls::stream<PackedStencil<uint8_t, 1, 1, 1, 1> > &_input2_shuffled_stencil_update_stream,
hls::stream<PackedStencil<uint8_t, 1, 1, 1, 1> > &_input_shuffled_stencil_update_stream)
{
#pragma HLS DATAFLOW
#pragma HLS INLINE region
#pragma HLS INTERFACE s_axilite port=return bundle=axilite_hls_target_hw_output_2_stencil_stream
#pragma HLS INTERFACE axis port=_hw_output_2_stencil_stream
#pragma HLS INTERFACE axis port=_input2_shuffled_stencil_update_stream
#pragma HLS INTERFACE axis port=_input_shuffled_stencil_update_stream

 hls::stream<PackedStencil<uint8_t, 8, 8, 1, 1> > _input_shuffled_stencil_stream;
#pragma HLS STREAM variable=_input_shuffled_stencil_stream depth=1
#pragma HLS RESOURCE variable=_input_shuffled_stencil_stream core=FIFO_SRL

 // produce input_shuffled.stencil.stream
 linebuffer<8, 8, 37, 37>(_input_shuffled_stencil_update_stream, _input_shuffled_stencil_stream);
 (void)0;
 // dispatch_stream(_input_shuffled_stencil_stream, 4, 8, 8, 8, 8, 8, 8, 1, 1, 37, 1, 1, 37, 1, "histogram$2", 1, 0, 8, 0, 8, 0, 37, 0, 37);
 hls::stream<PackedStencil<uint8_t, 8, 8, 1, 1> > &_input_shuffled_stencil_stream_to_histogram_2 = _input_shuffled_stencil_stream;
 (void)0;
 // consume input_shuffled.stencil.stream
 hls::stream<PackedStencil<uint16_t, 1, 1, 13, 2> > _histogram_2_stencil_update_stream;
#pragma HLS STREAM variable=_histogram_2_stencil_update_stream depth=1
#pragma HLS RESOURCE variable=_histogram_2_stencil_update_stream core=FIFO_SRL

 // produce histogram$2.stencil_update.stream
 for (int _histogram_2_scan_update_y = 0; _histogram_2_scan_update_y < 0 + 37; _histogram_2_scan_update_y++)
 {
  for (int _histogram_2_scan_update_x = 0; _histogram_2_scan_update_x < 0 + 37; _histogram_2_scan_update_x++)
  {
   Stencil<uint8_t, 8, 8, 1, 1> _input_shuffled_stencil;
#pragma HLS ARRAY_PARTITION variable=_input_shuffled_stencil.value complete dim=0

   // produce input_shuffled.stencil
   _input_shuffled_stencil = _input_shuffled_stencil_stream_to_histogram_2.read();
   (void)0;
   // consume input_shuffled.stencil
   Stencil<uint16_t, 1, 1, 13, 2> _histogram_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_histogram_2_stencil.value complete dim=0

   // produce histogram$2.stencil
   uint16_t _152 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 0, 0) = _152;
   uint16_t _153 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 0, 1) = _153;
   uint16_t _154 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 1, 0) = _154;
   uint16_t _155 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 1, 1) = _155;
   uint16_t _156 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 2, 0) = _156;
   uint16_t _157 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 2, 1) = _157;
   uint16_t _158 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 3, 0) = _158;
   uint16_t _159 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 3, 1) = _159;
   uint16_t _160 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 4, 0) = _160;
   uint16_t _161 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 4, 1) = _161;
   uint16_t _162 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 5, 0) = _162;
   uint16_t _163 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 5, 1) = _163;
   uint16_t _164 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 6, 0) = _164;
   uint16_t _165 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 6, 1) = _165;
   uint16_t _166 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 7, 0) = _166;
   uint16_t _167 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 7, 1) = _167;
   uint16_t _168 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 8, 0) = _168;
   uint16_t _169 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 8, 1) = _169;
   uint16_t _170 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 9, 0) = _170;
   uint16_t _171 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 9, 1) = _171;
   uint16_t _172 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 10, 0) = _172;
   uint16_t _173 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 10, 1) = _173;
   uint16_t _174 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 11, 0) = _174;
   uint16_t _175 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 11, 1) = _175;
   uint16_t _176 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 12, 0) = _176;
   uint16_t _177 = (uint16_t)(0);
   _histogram_2_stencil(0, 0, 12, 1) = _177;
   // update histogram$2.stencil
   for (int _histogram_2_s1_p2_r_y_r = 0; _histogram_2_s1_p2_r_y_r < 0 + 8; _histogram_2_s1_p2_r_y_r++)
   {
    for (int _histogram_2_s1_p2_r_x_r = 0; _histogram_2_s1_p2_r_x_r < 0 + 8; _histogram_2_s1_p2_r_x_r++)
    {
#pragma HLS PIPELINE
     uint8_t _178 = _input_shuffled_stencil(_histogram_2_s1_p2_r_x_r, _histogram_2_s1_p2_r_y_r, 0, 0);
     uint8_t _179 = (uint8_t)(16);
     uint8_t _180 = _178 + _179;
     uint8_t _181 = _180 >> 5;
     int32_t _182 = (int32_t)(_181);
     int32_t _183 = _182 + 2;
     uint16_t _184 = _histogram_2_stencil(0, 0, _183, 0);
     uint8_t _185 = _178 >> 4;
     uint16_t _186 = (uint16_t)(_185);
     uint16_t _187 = _184 + _186;
     _histogram_2_stencil(0, 0, _183, 0) = _187;
     uint8_t _188 = _input_shuffled_stencil(_histogram_2_s1_p2_r_x_r, _histogram_2_s1_p2_r_y_r, 0, 0);
     uint8_t _189 = (uint8_t)(16);
     uint8_t _190 = _188 + _189;
     uint8_t _191 = _190 >> 5;
     int32_t _192 = (int32_t)(_191);
     int32_t _193 = _192 + 2;
     uint16_t _194 = _histogram_2_stencil(0, 0, _193, 1);
     uint16_t _195 = (uint16_t)(4);
     uint16_t _196 = _194 + _195;
     _histogram_2_stencil(0, 0, _193, 1) = _196;
    } // for _histogram_2_s1_p2_r_x_r
   } // for _histogram_2_s1_p2_r_y_r
   // consume histogram$2.stencil
   _histogram_2_stencil_update_stream.write(_histogram_2_stencil);
   (void)0;
  } // for _histogram_2_scan_update_x
 } // for _histogram_2_scan_update_y
 // consume histogram$2.stencil_update.stream
 hls::stream<PackedStencil<uint16_t, 1, 1, 13, 2> > _histogram_2_stencil_stream;
#pragma HLS STREAM variable=_histogram_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_histogram_2_stencil_stream core=FIFO_SRL

 // produce histogram$2.stencil.stream
 linebuffer<37, 37, 13, 2>(_histogram_2_stencil_update_stream, _histogram_2_stencil_stream);
 (void)0;
 // dispatch_stream(_histogram_2_stencil_stream, 4, 1, 1, 37, 1, 1, 37, 13, 13, 13, 2, 2, 2, 1, "blurz$2", 1, 0, 37, 0, 37, 0, 13, 0, 2);
 hls::stream<PackedStencil<uint16_t, 1, 1, 13, 2> > &_histogram_2_stencil_stream_to_blurz_2 = _histogram_2_stencil_stream;
 (void)0;
 // consume histogram$2.stencil.stream
 hls::stream<PackedStencil<uint16_t, 1, 1, 9, 2> > _blurz_2_stencil_update_stream;
#pragma HLS STREAM variable=_blurz_2_stencil_update_stream depth=1
#pragma HLS RESOURCE variable=_blurz_2_stencil_update_stream core=FIFO_SRL

 // produce blurz$2.stencil_update.stream
 for (int _blurz_2_scan_update_y = 0; _blurz_2_scan_update_y < 0 + 37; _blurz_2_scan_update_y++)
 {
  for (int _blurz_2_scan_update_x = 0; _blurz_2_scan_update_x < 0 + 37; _blurz_2_scan_update_x++)
  {
   Stencil<uint16_t, 1, 1, 13, 2> _histogram_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_histogram_2_stencil.value complete dim=0

   // produce histogram$2.stencil
   _histogram_2_stencil = _histogram_2_stencil_stream_to_blurz_2.read();
   (void)0;
   // consume histogram$2.stencil
   Stencil<uint16_t, 1, 1, 9, 2> _blurz_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_blurz_2_stencil.value complete dim=0

   // produce blurz$2.stencil
   for (int _blurz_2_stencil_s0_c = 0; _blurz_2_stencil_s0_c < 0 + 2; _blurz_2_stencil_s0_c++)
   {
    for (int _blurz_2_stencil_s0_z = 0; _blurz_2_stencil_s0_z < 0 + 9; _blurz_2_stencil_s0_z++)
    {
#pragma HLS PIPELINE
     uint16_t _197 = _histogram_2_stencil(0, 0, _blurz_2_stencil_s0_z, _blurz_2_stencil_s0_c);
     int32_t _198 = _blurz_2_stencil_s0_z + 1;
     uint16_t _199 = _histogram_2_stencil(0, 0, _198, _blurz_2_stencil_s0_c);
     uint16_t _200 = (uint16_t)(4);
     uint16_t _201 = _199 * _200;
     uint16_t _202 = _197 + _201;
     int32_t _203 = _blurz_2_stencil_s0_z + 2;
     uint16_t _204 = _histogram_2_stencil(0, 0, _203, _blurz_2_stencil_s0_c);
     uint16_t _205 = (uint16_t)(6);
     uint16_t _206 = _204 * _205;
     uint16_t _207 = _202 + _206;
     int32_t _208 = _blurz_2_stencil_s0_z + 3;
     uint16_t _209 = _histogram_2_stencil(0, 0, _208, _blurz_2_stencil_s0_c);
     uint16_t _210 = _209 * _200;
     uint16_t _211 = _207 + _210;
     int32_t _212 = _blurz_2_stencil_s0_z + 4;
     uint16_t _213 = _histogram_2_stencil(0, 0, _212, _blurz_2_stencil_s0_c);
     uint16_t _214 = _211 + _213;
     uint16_t _215 = _214 >> 4;
     _blurz_2_stencil(0, 0, _blurz_2_stencil_s0_z, _blurz_2_stencil_s0_c) = _215;
    } // for _blurz_2_stencil_s0_z
   } // for _blurz_2_stencil_s0_c
   // consume blurz$2.stencil
   _blurz_2_stencil_update_stream.write(_blurz_2_stencil);
   (void)0;
  } // for _blurz_2_scan_update_x
 } // for _blurz_2_scan_update_y
 // consume blurz$2.stencil_update.stream
 hls::stream<PackedStencil<uint16_t, 5, 1, 9, 2> > _blurz_2_stencil_stream;
#pragma HLS STREAM variable=_blurz_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_blurz_2_stencil_stream core=FIFO_SRL

 // produce blurz$2.stencil.stream
 linebuffer<37, 37, 9, 2>(_blurz_2_stencil_update_stream, _blurz_2_stencil_stream);
 (void)0;
 // dispatch_stream(_blurz_2_stencil_stream, 4, 5, 1, 37, 1, 1, 37, 9, 9, 9, 2, 2, 2, 1, "blurx$2", 1, 0, 37, 0, 37, 0, 9, 0, 2);
 hls::stream<PackedStencil<uint16_t, 5, 1, 9, 2> > &_blurz_2_stencil_stream_to_blurx_2 = _blurz_2_stencil_stream;
 (void)0;
 // consume blurz$2.stencil.stream
 hls::stream<PackedStencil<uint16_t, 1, 1, 9, 2> > _blurx_2_stencil_update_stream;
#pragma HLS STREAM variable=_blurx_2_stencil_update_stream depth=1
#pragma HLS RESOURCE variable=_blurx_2_stencil_update_stream core=FIFO_SRL

 // produce blurx$2.stencil_update.stream
 for (int _blurx_2_scan_update_y = 0; _blurx_2_scan_update_y < 0 + 37; _blurx_2_scan_update_y++)
 {
  for (int _blurx_2_scan_update_x = 0; _blurx_2_scan_update_x < 0 + 33; _blurx_2_scan_update_x++)
  {
   Stencil<uint16_t, 5, 1, 9, 2> _blurz_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_blurz_2_stencil.value complete dim=0

   // produce blurz$2.stencil
   _blurz_2_stencil = _blurz_2_stencil_stream_to_blurx_2.read();
   (void)0;
   // consume blurz$2.stencil
   Stencil<uint16_t, 1, 1, 9, 2> _blurx_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_blurx_2_stencil.value complete dim=0

   // produce blurx$2.stencil
   for (int _blurx_2_stencil_s0_c = 0; _blurx_2_stencil_s0_c < 0 + 2; _blurx_2_stencil_s0_c++)
   {
    for (int _blurx_2_stencil_s0_z = 0; _blurx_2_stencil_s0_z < 0 + 9; _blurx_2_stencil_s0_z++)
    {
#pragma HLS PIPELINE
     uint16_t _216 = _blurz_2_stencil(0, 0, _blurx_2_stencil_s0_z, _blurx_2_stencil_s0_c);
     uint16_t _217 = _blurz_2_stencil(1, 0, _blurx_2_stencil_s0_z, _blurx_2_stencil_s0_c);
     uint16_t _218 = (uint16_t)(4);
     uint16_t _219 = _217 * _218;
     uint16_t _220 = _216 + _219;
     uint16_t _221 = _blurz_2_stencil(2, 0, _blurx_2_stencil_s0_z, _blurx_2_stencil_s0_c);
     uint16_t _222 = (uint16_t)(6);
     uint16_t _223 = _221 * _222;
     uint16_t _224 = _220 + _223;
     uint16_t _225 = _blurz_2_stencil(3, 0, _blurx_2_stencil_s0_z, _blurx_2_stencil_s0_c);
     uint16_t _226 = _225 * _218;
     uint16_t _227 = _224 + _226;
     uint16_t _228 = _blurz_2_stencil(4, 0, _blurx_2_stencil_s0_z, _blurx_2_stencil_s0_c);
     uint16_t _229 = _227 + _228;
     uint16_t _230 = _229 >> 4;
     _blurx_2_stencil(0, 0, _blurx_2_stencil_s0_z, _blurx_2_stencil_s0_c) = _230;
    } // for _blurx_2_stencil_s0_z
   } // for _blurx_2_stencil_s0_c
   // consume blurx$2.stencil
   _blurx_2_stencil_update_stream.write(_blurx_2_stencil);
   (void)0;
  } // for _blurx_2_scan_update_x
 } // for _blurx_2_scan_update_y
 // consume blurx$2.stencil_update.stream
 hls::stream<PackedStencil<uint16_t, 1, 5, 9, 2> > _blurx_2_stencil_stream;
#pragma HLS STREAM variable=_blurx_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_blurx_2_stencil_stream core=FIFO_SRL

 // produce blurx$2.stencil.stream
 linebuffer<33, 37, 9, 2>(_blurx_2_stencil_update_stream, _blurx_2_stencil_stream);
 (void)0;
 // dispatch_stream(_blurx_2_stencil_stream, 4, 1, 1, 33, 5, 1, 37, 9, 9, 9, 2, 2, 2, 1, "blury$2", 1, 0, 33, 0, 37, 0, 9, 0, 2);
 hls::stream<PackedStencil<uint16_t, 1, 5, 9, 2> > &_blurx_2_stencil_stream_to_blury_2 = _blurx_2_stencil_stream;
 (void)0;
 // consume blurx$2.stencil.stream
 hls::stream<PackedStencil<uint16_t, 1, 1, 9, 2> > _blury_2_stencil_update_stream;
#pragma HLS STREAM variable=_blury_2_stencil_update_stream depth=1
#pragma HLS RESOURCE variable=_blury_2_stencil_update_stream core=FIFO_SRL

 // produce blury$2.stencil_update.stream
 for (int _blury_2_scan_update_y = 0; _blury_2_scan_update_y < 0 + 33; _blury_2_scan_update_y++)
 {
  for (int _blury_2_scan_update_x = 0; _blury_2_scan_update_x < 0 + 33; _blury_2_scan_update_x++)
  {
   Stencil<uint16_t, 1, 5, 9, 2> _blurx_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_blurx_2_stencil.value complete dim=0

   // produce blurx$2.stencil
   _blurx_2_stencil = _blurx_2_stencil_stream_to_blury_2.read();
   (void)0;
   // consume blurx$2.stencil
   Stencil<uint16_t, 1, 1, 9, 2> _blury_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_blury_2_stencil.value complete dim=0

   // produce blury$2.stencil
   for (int _blury_2_stencil_s0_c = 0; _blury_2_stencil_s0_c < 0 + 2; _blury_2_stencil_s0_c++)
   {
    for (int _blury_2_stencil_s0_z = 0; _blury_2_stencil_s0_z < 0 + 9; _blury_2_stencil_s0_z++)
    {
#pragma HLS PIPELINE
     uint16_t _231 = _blurx_2_stencil(0, 0, _blury_2_stencil_s0_z, _blury_2_stencil_s0_c);
     uint16_t _232 = _blurx_2_stencil(0, 1, _blury_2_stencil_s0_z, _blury_2_stencil_s0_c);
     uint16_t _233 = (uint16_t)(4);
     uint16_t _234 = _232 * _233;
     uint16_t _235 = _231 + _234;
     uint16_t _236 = _blurx_2_stencil(0, 2, _blury_2_stencil_s0_z, _blury_2_stencil_s0_c);
     uint16_t _237 = (uint16_t)(6);
     uint16_t _238 = _236 * _237;
     uint16_t _239 = _235 + _238;
     uint16_t _240 = _blurx_2_stencil(0, 3, _blury_2_stencil_s0_z, _blury_2_stencil_s0_c);
     uint16_t _241 = _240 * _233;
     uint16_t _242 = _239 + _241;
     uint16_t _243 = _blurx_2_stencil(0, 4, _blury_2_stencil_s0_z, _blury_2_stencil_s0_c);
     uint16_t _244 = _242 + _243;
     uint16_t _245 = _244 >> 4;
     _blury_2_stencil(0, 0, _blury_2_stencil_s0_z, _blury_2_stencil_s0_c) = _245;
    } // for _blury_2_stencil_s0_z
   } // for _blury_2_stencil_s0_c
   // consume blury$2.stencil
   _blury_2_stencil_update_stream.write(_blury_2_stencil);
   (void)0;
  } // for _blury_2_scan_update_x
 } // for _blury_2_scan_update_y
 // consume blury$2.stencil_update.stream
 hls::stream<PackedStencil<uint16_t, 2, 2, 9, 2> > _blury_2_stencil_stream;
#pragma HLS STREAM variable=_blury_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_blury_2_stencil_stream core=FIFO_SRL

 // produce blury$2.stencil.stream
 linebuffer<33, 33, 9, 2>(_blury_2_stencil_update_stream, _blury_2_stencil_stream);
 (void)0;
 // dispatch_stream(_blury_2_stencil_stream, 4, 2, 1, 33, 2, 1, 33, 9, 9, 9, 2, 2, 2, 1, "hw_output$2", 1, 0, 33, 0, 33, 0, 9, 0, 2);
 hls::stream<PackedStencil<uint16_t, 2, 2, 9, 2> > &_blury_2_stencil_stream_to_hw_output_2 = _blury_2_stencil_stream;
 (void)0;
 // consume blury$2.stencil.stream
 hls::stream<PackedStencil<uint8_t, 8, 8, 1, 1> > _input2_shuffled_stencil_stream;
#pragma HLS STREAM variable=_input2_shuffled_stencil_stream depth=1
#pragma HLS RESOURCE variable=_input2_shuffled_stencil_stream core=FIFO_SRL

 // produce input2_shuffled.stencil.stream
 linebuffer<8, 8, 32, 32>(_input2_shuffled_stencil_update_stream, _input2_shuffled_stencil_stream);
 (void)0;
 // dispatch_stream(_input2_shuffled_stencil_stream, 4, 8, 8, 8, 8, 8, 8, 1, 1, 32, 1, 1, 32, 1, "hw_output$2", 1, 0, 8, 0, 8, 0, 32, 0, 32);
 hls::stream<PackedStencil<uint8_t, 8, 8, 1, 1> > &_input2_shuffled_stencil_stream_to_hw_output_2 = _input2_shuffled_stencil_stream;
 (void)0;
 // consume input2_shuffled.stencil.stream
 for (int _output_shuffled_s0_y_grid_y_grid = 0; _output_shuffled_s0_y_grid_y_grid < 0 + 32; _output_shuffled_s0_y_grid_y_grid++)
 {
  for (int _output_shuffled_s0_x_grid_x_grid = 0; _output_shuffled_s0_x_grid_x_grid < 0 + 32; _output_shuffled_s0_x_grid_x_grid++)
  {
   Stencil<uint8_t, 8, 8, 1, 1> _input2_shuffled_stencil;
#pragma HLS ARRAY_PARTITION variable=_input2_shuffled_stencil.value complete dim=0

   // produce input2_shuffled.stencil
   _input2_shuffled_stencil = _input2_shuffled_stencil_stream_to_hw_output_2.read();
   (void)0;
   // consume input2_shuffled.stencil
   Stencil<uint16_t, 2, 2, 9, 2> _blury_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_blury_2_stencil.value complete dim=0

   // produce blury$2.stencil
   _blury_2_stencil = _blury_2_stencil_stream_to_hw_output_2.read();
   (void)0;
   // consume blury$2.stencil

   // produce hw_output$2.stencil
   for (int _hw_output_2_stencil_s0_y_in = 0; _hw_output_2_stencil_s0_y_in < 0 + 8; _hw_output_2_stencil_s0_y_in++)
   {
    for (int _hw_output_2_stencil_s0_x_in = 0; _hw_output_2_stencil_s0_x_in < 0 + 8; _hw_output_2_stencil_s0_x_in++)
    {
#pragma HLS PIPELINE
        Stencil<uint8_t, 1, 1, 1, 1> _hw_output_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_hw_output_2_stencil.value complete dim=0
     uint8_t _246 = _input2_shuffled_stencil(_hw_output_2_stencil_s0_x_in, _hw_output_2_stencil_s0_y_in, 0, 0);
     uint16_t _247 = (uint16_t)(_246);
     uint16_t _248 = _247 >> 5;
     int32_t _249 = (int32_t)(_248);
     uint16_t _250 = _blury_2_stencil(0, 0, _249, 1);
     uint32_t _251 = (uint32_t)(_250);
     uint16_t _252 = (uint16_t)(65535);
     int32_t _253 = _hw_output_2_stencil_s0_x_in * 8192;
     uint16_t _254 = (uint16_t)(_253);
     uint16_t _255 = _252 - _254;
     uint32_t _256 = (uint32_t)(_255);
     uint32_t _257 = _251 * _256;
     uint16_t _258 = _blury_2_stencil(1, 0, _249, 1);
     uint32_t _259 = (uint32_t)(_258);
     uint32_t _260 = (uint32_t)(_254);
     uint32_t _261 = _259 * _260;
     uint32_t _262 = _257 + _261;
     uint32_t _263 = (uint32_t)(32768);
     uint32_t _264 = _262 + _263;
     uint32_t _265 = _264 >> 16;
     uint32_t _266 = _265 + _262;
     uint32_t _267 = _266 + _263;
     uint32_t _268 = _267 >> 16;
     uint16_t _269 = (uint16_t)(_268);
     uint16_t _270 = _269;
     uint32_t _271 = (uint32_t)(_270);
     int32_t _272 = _hw_output_2_stencil_s0_y_in * 8192;
     uint16_t _273 = (uint16_t)(_272);
     uint16_t _274 = _252 - _273;
     uint32_t _275 = (uint32_t)(_274);
     uint32_t _276 = _271 * _275;
     uint16_t _277 = _blury_2_stencil(0, 1, _249, 1);
     uint32_t _278 = (uint32_t)(_277);
     uint32_t _279 = _278 * _256;
     uint16_t _280 = _blury_2_stencil(1, 1, _249, 1);
     uint32_t _281 = (uint32_t)(_280);
     uint32_t _282 = _281 * _260;
     uint32_t _283 = _279 + _282;
     uint32_t _284 = _283 + _263;
     uint32_t _285 = _284 >> 16;
     uint32_t _286 = _285 + _283;
     uint32_t _287 = _286 + _263;
     uint32_t _288 = _287 >> 16;
     uint16_t _289 = (uint16_t)(_288);
     uint16_t _290 = _289;
     uint32_t _291 = (uint32_t)(_290);
     uint32_t _292 = (uint32_t)(_273);
     uint32_t _293 = _291 * _292;
     uint32_t _294 = _276 + _293;
     uint32_t _295 = _294 + _263;
     uint32_t _296 = _295 >> 16;
     uint32_t _297 = _296 + _294;
     uint32_t _298 = _297 + _263;
     uint32_t _299 = _298 >> 16;
     uint16_t _300 = (uint16_t)(_299);
     uint16_t _301 = _300;
     uint32_t _302 = (uint32_t)(_301);
     uint16_t _303 = _247 & 31;
     uint16_t _304 = (uint16_t)(2048);
     uint16_t _305 = _303 * _304;
     uint16_t _306 = _252 - _305;
     uint32_t _307 = (uint32_t)(_306);
     uint32_t _308 = _302 * _307;
     int32_t _309 = _249 + 1;
     uint16_t _310 = _blury_2_stencil(0, 0, _309, 1);
     uint32_t _311 = (uint32_t)(_310);
     uint32_t _312 = _311 * _256;
     uint16_t _313 = _blury_2_stencil(1, 0, _309, 1);
     uint32_t _314 = (uint32_t)(_313);
     uint32_t _315 = _314 * _260;
     uint32_t _316 = _312 + _315;
     uint32_t _317 = _316 + _263;
     uint32_t _318 = _317 >> 16;
     uint32_t _319 = _318 + _316;
     uint32_t _320 = _319 + _263;
     uint32_t _321 = _320 >> 16;
     uint16_t _322 = (uint16_t)(_321);
     uint16_t _323 = _322;
     uint32_t _324 = (uint32_t)(_323);
     uint32_t _325 = _324 * _275;
     uint16_t _326 = _blury_2_stencil(0, 1, _309, 1);
     uint32_t _327 = (uint32_t)(_326);
     uint32_t _328 = _327 * _256;
     uint16_t _329 = _blury_2_stencil(1, 1, _309, 1);
     uint32_t _330 = (uint32_t)(_329);
     uint32_t _331 = _330 * _260;
     uint32_t _332 = _328 + _331;
     uint32_t _333 = _332 + _263;
     uint32_t _334 = _333 >> 16;
     uint32_t _335 = _334 + _332;
     uint32_t _336 = _335 + _263;
     uint32_t _337 = _336 >> 16;
     uint16_t _338 = (uint16_t)(_337);
     uint16_t _339 = _338;
     uint32_t _340 = (uint32_t)(_339);
     uint32_t _341 = _340 * _292;
     uint32_t _342 = _325 + _341;
     uint32_t _343 = _342 + _263;
     uint32_t _344 = _343 >> 16;
     uint32_t _345 = _344 + _342;
     uint32_t _346 = _345 + _263;
     uint32_t _347 = _346 >> 16;
     uint16_t _348 = (uint16_t)(_347);
     uint16_t _349 = _348;
     uint32_t _350 = (uint32_t)(_349);
     uint32_t _351 = (uint32_t)(_305);
     uint32_t _352 = _350 * _351;
     uint32_t _353 = _308 + _352;
     uint32_t _354 = _353 + _263;
     uint32_t _355 = _354 >> 16;
     uint32_t _356 = _355 + _353;
     uint32_t _357 = _356 + _263;
     uint32_t _358 = _357 >> 16;
     uint16_t _359 = (uint16_t)(_358);
     uint16_t _360 = _359;
     uint16_t _361 = (uint16_t)(0);
     uint16_t _362 = _blury_2_stencil(0, 0, _249, 0);
     uint32_t _363 = (uint32_t)(_362);
     uint32_t _364 = _363 * _256;
     uint16_t _365 = _blury_2_stencil(1, 0, _249, 0);
     uint32_t _366 = (uint32_t)(_365);
     uint32_t _367 = _366 * _260;
     uint32_t _368 = _364 + _367;
     uint32_t _369 = _368 + _263;
     uint32_t _370 = _369 >> 16;
     uint32_t _371 = _370 + _368;
     uint32_t _372 = _371 + _263;
     uint32_t _373 = _372 >> 16;
     uint16_t _374 = (uint16_t)(_373);
     uint16_t _375 = _374;
     uint32_t _376 = (uint32_t)(_375);
     uint32_t _377 = _376 * _275;
     uint16_t _378 = _blury_2_stencil(0, 1, _249, 0);
     uint32_t _379 = (uint32_t)(_378);
     uint32_t _380 = _379 * _256;
     uint16_t _381 = _blury_2_stencil(1, 1, _249, 0);
     uint32_t _382 = (uint32_t)(_381);
     uint32_t _383 = _382 * _260;
     uint32_t _384 = _380 + _383;
     uint32_t _385 = _384 + _263;
     uint32_t _386 = _385 >> 16;
     uint32_t _387 = _386 + _384;
     uint32_t _388 = _387 + _263;
     uint32_t _389 = _388 >> 16;
     uint16_t _390 = (uint16_t)(_389);
     uint16_t _391 = _390;
     uint32_t _392 = (uint32_t)(_391);
     uint32_t _393 = _392 * _292;
     uint32_t _394 = _377 + _393;
     uint32_t _395 = _394 + _263;
     uint32_t _396 = _395 >> 16;
     uint32_t _397 = _396 + _394;
     uint32_t _398 = _397 + _263;
     uint32_t _399 = _398 >> 16;
     uint16_t _400 = (uint16_t)(_399);
     uint16_t _401 = _400;
     uint32_t _402 = (uint32_t)(_401);
     uint32_t _403 = _402 * _307;
     uint16_t _404 = _blury_2_stencil(0, 0, _309, 0);
     uint32_t _405 = (uint32_t)(_404);
     uint32_t _406 = _405 * _256;
     uint16_t _407 = _blury_2_stencil(1, 0, _309, 0);
     uint32_t _408 = (uint32_t)(_407);
     uint32_t _409 = _408 * _260;
     uint32_t _410 = _406 + _409;
     uint32_t _411 = _410 + _263;
     uint32_t _412 = _411 >> 16;
     uint32_t _413 = _412 + _410;
     uint32_t _414 = _413 + _263;
     uint32_t _415 = _414 >> 16;
     uint16_t _416 = (uint16_t)(_415);
     uint16_t _417 = _416;
     uint32_t _418 = (uint32_t)(_417);
     uint32_t _419 = _418 * _275;
     uint16_t _420 = _blury_2_stencil(0, 1, _309, 0);
     uint32_t _421 = (uint32_t)(_420);
     uint32_t _422 = _421 * _256;
     uint16_t _423 = _blury_2_stencil(1, 1, _309, 0);
     uint32_t _424 = (uint32_t)(_423);
     uint32_t _425 = _424 * _260;
     uint32_t _426 = _422 + _425;
     uint32_t _427 = _426 + _263;
     uint32_t _428 = _427 >> 16;
     uint32_t _429 = _428 + _426;
     uint32_t _430 = _429 + _263;
     uint32_t _431 = _430 >> 16;
     uint16_t _432 = (uint16_t)(_431);
     uint16_t _433 = _432;
     uint32_t _434 = (uint32_t)(_433);
     uint32_t _435 = _434 * _292;
     uint32_t _436 = _419 + _435;
     uint32_t _437 = _436 + _263;
     uint32_t _438 = _437 >> 16;
     uint32_t _439 = _438 + _436;
     uint32_t _440 = _439 + _263;
     uint32_t _441 = _440 >> 16;
     uint16_t _442 = (uint16_t)(_441);
     uint16_t _443 = _442;
     uint32_t _444 = (uint32_t)(_443);
     uint32_t _445 = _444 * _351;
     uint32_t _446 = _403 + _445;
     uint32_t _447 = _446 + _263;
     uint32_t _448 = _447 >> 16;
     uint32_t _449 = _448 + _446;
     uint32_t _450 = _449 + _263;
     uint32_t _451 = _450 >> 16;
     uint16_t _452 = (uint16_t)(_451);
     uint16_t _453 = _452;
     uint16_t _454 = (uint16_t)(64);
     uint16_t _455 = _453 * _454;
     uint16_t _456 = _455 / _360;
     uint16_t _457 = (uint16_t)(255);
     uint16_t _458 = min(_456, _457);
     bool _459 = _360 == _361;
     uint16_t _460 = (uint16_t)(_459 ? _361 : _458);
     uint8_t _461 = (uint8_t)(_460);
     _hw_output_2_stencil(0, 0, 0, 0) = _461;
     PackedStencil<uint8_t, 1, 1, 1, 1>_hw_output_2_stencil_packed = _hw_output_2_stencil;
     if (_output_shuffled_s0_x_grid_x_grid == 31 && _output_shuffled_s0_y_grid_y_grid == 31 &&
         _hw_output_2_stencil_s0_y_in == 7 && _hw_output_2_stencil_s0_x_in == 7) {
        _hw_output_2_stencil_packed.last = 1;
    } else {
        _hw_output_2_stencil_packed.last = 0;
    }
     _hw_output_2_stencil_stream.write(_hw_output_2_stencil_packed);
    } // for _hw_output_2_stencil_s0_x_in
   } // for _hw_output_2_stencil_s0_y_in
   // consume hw_output$2.stencil
   (void)0;
  } // for _output_shuffled_s0_x_grid_x_grid
 } // for _output_shuffled_s0_y_grid_y_grid
} // kernel hls_target_hls_target_hw_output_2_stencil_stream


