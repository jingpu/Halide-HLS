#include "hls_target.h"

#include "Linebuffer.h"
#include "halide_math.h"
void hls_target(
hls::stream<AxiPackedStencil<uint8_t, 3, 1, 1> > &arg_0,
hls::stream<AxiPackedStencil<uint8_t, 1, 1> > &arg_1)
{
#pragma HLS DATAFLOW
#pragma HLS INLINE region
#pragma HLS INTERFACE s_axilite port=return bundle=config
#pragma HLS INTERFACE axis register port=arg_0
#pragma HLS INTERFACE axis register port=arg_1

 // alias the arguments
 hls::stream<PackedStencil<uint8_t, 3, 1, 1> > __auto_insert__hw_output_2_stencil_stream;
 hls::stream<AxiPackedStencil<uint8_t, 3, 1, 1> > &out_stream = arg_0;
 hls::stream<AxiPackedStencil<uint8_t, 1, 1> > &_padded_2_stencil_update_stream = arg_1;

 hls::stream<PackedStencil<uint8_t, 3, 3> > _padded_2_stencil_stream;
#pragma HLS STREAM variable=_padded_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_padded_2_stencil_stream core=FIFO_SRL

 // produce padded$2.stencil.stream
 linebuffer<1443, 963>(_padded_2_stencil_update_stream, _padded_2_stencil_stream);
 (void)0;
 // dispatch_stream(_padded_2_stencil_stream, 2, 3, 1, 1443, 3, 1, 963, 1, "demosaic$2", 1, 0, 1443, 0, 963);
 hls::stream<PackedStencil<uint8_t, 3, 3> > &_padded_2_stencil_stream_to_demosaic_2 = _padded_2_stencil_stream;
 (void)0;
 // consume padded$2.stencil.stream
 hls::stream<PackedStencil<uint8_t, 3, 1, 1> > _demosaic_2_stencil_update_stream;
#pragma HLS STREAM variable=_demosaic_2_stencil_update_stream depth=1
#pragma HLS RESOURCE variable=_demosaic_2_stencil_update_stream core=FIFO_SRL

 // produce demosaic$2.stencil_update.stream
 for (int _demosaic_2_scan_update_y = 0; _demosaic_2_scan_update_y < 0 + 961; _demosaic_2_scan_update_y++)
 {
  for (int _demosaic_2_scan_update_x = 0; _demosaic_2_scan_update_x < 0 + 1441; _demosaic_2_scan_update_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<uint8_t, 3, 3> _padded_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_padded_2_stencil.value complete dim=0

   _padded_2_stencil = _padded_2_stencil_stream_to_demosaic_2.read();
   (void)0;
   Stencil<uint8_t, 3, 1, 1> _demosaic_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_demosaic_2_stencil.value complete dim=0

   int32_t _126 = _demosaic_2_scan_update_x + -1;
   int32_t _127 = _126 & 1;
   bool _128 = _127 == 0;
   uint8_t _129 = _padded_2_stencil(1, 1);
   uint8_t _130 = _padded_2_stencil(0, 1);
   uint16_t _131 = (uint16_t)(_130);
   uint8_t _132 = _padded_2_stencil(2, 1);
   uint16_t _133 = (uint16_t)(_132);
   uint16_t _134 = _131 + _133;
   uint16_t _135 = _134 >> 1;
   uint8_t _136 = (uint8_t)(_135);
   uint8_t _137 = (uint8_t)(_128 ? _129 : _136);
   uint8_t _138 = _padded_2_stencil(1, 0);
   uint16_t _139 = (uint16_t)(_138);
   uint8_t _140 = _padded_2_stencil(1, 2);
   uint16_t _141 = (uint16_t)(_140);
   uint16_t _142 = _139 + _141;
   uint16_t _143 = _142 >> 1;
   uint8_t _144 = (uint8_t)(_143);
   uint8_t _145 = _padded_2_stencil(0, 0);
   uint16_t _146 = (uint16_t)(_145);
   uint8_t _147 = _padded_2_stencil(2, 0);
   uint16_t _148 = (uint16_t)(_147);
   uint16_t _149 = _146 + _148;
   uint8_t _150 = _padded_2_stencil(0, 2);
   uint16_t _151 = (uint16_t)(_150);
   uint16_t _152 = _149 + _151;
   uint8_t _153 = _padded_2_stencil(2, 2);
   uint16_t _154 = (uint16_t)(_153);
   uint16_t _155 = _152 + _154;
   uint16_t _156 = _155 >> 2;
   uint8_t _157 = (uint8_t)(_156);
   uint8_t _158 = (uint8_t)(_128 ? _144 : _157);
   int32_t _159 = _demosaic_2_scan_update_y + -1;
   int32_t _160 = _159 & 1;
   bool _161 = _160 == 0;
   uint8_t _162 = (uint8_t)(_161 ? _137 : _158);
   _demosaic_2_stencil(0, 0, 0) = _162;
   int32_t _163 = _demosaic_2_scan_update_x + -1;
   int32_t _164 = _163 & 1;
   bool _165 = _164 == 0;
   uint8_t _166 = _padded_2_stencil(1, 1);
   uint8_t _167 = _padded_2_stencil(0, 1);
   uint16_t _168 = (uint16_t)(_167);
   uint8_t _169 = _padded_2_stencil(2, 1);
   uint16_t _170 = (uint16_t)(_169);
   uint16_t _171 = _168 + _170;
   uint8_t _172 = _padded_2_stencil(1, 0);
   uint16_t _173 = (uint16_t)(_172);
   uint16_t _174 = _171 + _173;
   uint8_t _175 = _padded_2_stencil(1, 2);
   uint16_t _176 = (uint16_t)(_175);
   uint16_t _177 = _174 + _176;
   uint16_t _178 = _177 >> 2;
   uint8_t _179 = (uint8_t)(_178);
   uint8_t _180 = (uint8_t)(_165 ? _179 : _166);
   uint8_t _181 = (uint8_t)(_165 ? _166 : _179);
   int32_t _182 = _demosaic_2_scan_update_y + -1;
   int32_t _183 = _182 & 1;
   bool _184 = _183 == 0;
   uint8_t _185 = (uint8_t)(_184 ? _180 : _181);
   _demosaic_2_stencil(1, 0, 0) = _185;
   int32_t _186 = _demosaic_2_scan_update_x + -1;
   int32_t _187 = _186 & 1;
   bool _188 = _187 == 0;
   uint8_t _189 = _padded_2_stencil(0, 0);
   uint16_t _190 = (uint16_t)(_189);
   uint8_t _191 = _padded_2_stencil(2, 0);
   uint16_t _192 = (uint16_t)(_191);
   uint16_t _193 = _190 + _192;
   uint8_t _194 = _padded_2_stencil(0, 2);
   uint16_t _195 = (uint16_t)(_194);
   uint16_t _196 = _193 + _195;
   uint8_t _197 = _padded_2_stencil(2, 2);
   uint16_t _198 = (uint16_t)(_197);
   uint16_t _199 = _196 + _198;
   uint16_t _200 = _199 >> 2;
   uint8_t _201 = (uint8_t)(_200);
   uint8_t _202 = _padded_2_stencil(1, 0);
   uint16_t _203 = (uint16_t)(_202);
   uint8_t _204 = _padded_2_stencil(1, 2);
   uint16_t _205 = (uint16_t)(_204);
   uint16_t _206 = _203 + _205;
   uint16_t _207 = _206 >> 1;
   uint8_t _208 = (uint8_t)(_207);
   uint8_t _209 = (uint8_t)(_188 ? _201 : _208);
   uint8_t _210 = _padded_2_stencil(0, 1);
   uint16_t _211 = (uint16_t)(_210);
   uint8_t _212 = _padded_2_stencil(2, 1);
   uint16_t _213 = (uint16_t)(_212);
   uint16_t _214 = _211 + _213;
   uint16_t _215 = _214 >> 1;
   uint8_t _216 = (uint8_t)(_215);
   uint8_t _217 = _padded_2_stencil(1, 1);
   uint8_t _218 = (uint8_t)(_188 ? _216 : _217);
   int32_t _219 = _demosaic_2_scan_update_y + -1;
   int32_t _220 = _219 & 1;
   bool _221 = _220 == 0;
   uint8_t _222 = (uint8_t)(_221 ? _209 : _218);
   _demosaic_2_stencil(2, 0, 0) = _222;
   _demosaic_2_stencil_update_stream.write(_demosaic_2_stencil);
   (void)0;
  } // for _demosaic_2_scan_update_x
 } // for _demosaic_2_scan_update_y
 // consume demosaic$2.stencil_update.stream
 hls::stream<PackedStencil<uint8_t, 3, 2, 2> > _demosaic_2_stencil_stream;
#pragma HLS STREAM variable=_demosaic_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_demosaic_2_stencil_stream core=FIFO_SRL

 // produce demosaic$2.stencil.stream
 linebuffer<3, 1441, 961>(_demosaic_2_stencil_update_stream, _demosaic_2_stencil_stream);
 (void)0;
 // dispatch_stream(_demosaic_2_stencil_stream, 3, 3, 3, 3, 2, 1, 1441, 2, 1, 961, 1, "lowpass_x$2", 1, 0, 3, 0, 1441, 0, 961);
 hls::stream<PackedStencil<uint8_t, 3, 2, 2> > &_demosaic_2_stencil_stream_to_lowpass_x_2 = _demosaic_2_stencil_stream;
 (void)0;
 // consume demosaic$2.stencil.stream
 hls::stream<PackedStencil<uint8_t, 3, 1, 1> > _lowpass_x_2_stencil_stream;
#pragma HLS STREAM variable=_lowpass_x_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_lowpass_x_2_stencil_stream core=FIFO_SRL

 // produce lowpass_x$2.stencil.stream
 for (int _lowpass_x_2_scan_y = 0; _lowpass_x_2_scan_y < 0 + 960; _lowpass_x_2_scan_y++)
 {
  for (int _lowpass_x_2_scan_x = 0; _lowpass_x_2_scan_x < 0 + 1440; _lowpass_x_2_scan_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<uint8_t, 3, 2, 2> _demosaic_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_demosaic_2_stencil.value complete dim=0

   _demosaic_2_stencil = _demosaic_2_stencil_stream_to_lowpass_x_2.read();
   (void)0;
   Stencil<uint8_t, 3, 1, 1> _lowpass_x_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_lowpass_x_2_stencil.value complete dim=0

   uint8_t _223 = _demosaic_2_stencil(0, 0, 0);
   uint16_t _224 = (uint16_t)(_223);
   uint8_t _225 = _demosaic_2_stencil(0, 1, 0);
   uint16_t _226 = (uint16_t)(_225);
   uint16_t _227 = _224 + _226;
   uint16_t _228 = _227 >> 1;
   uint8_t _229 = (uint8_t)(_228);
   uint16_t _230 = (uint16_t)(_229);
   uint8_t _231 = _demosaic_2_stencil(0, 0, 1);
   uint16_t _232 = (uint16_t)(_231);
   uint8_t _233 = _demosaic_2_stencil(0, 1, 1);
   uint16_t _234 = (uint16_t)(_233);
   uint16_t _235 = _232 + _234;
   uint16_t _236 = _235 >> 1;
   uint8_t _237 = (uint8_t)(_236);
   uint16_t _238 = (uint16_t)(_237);
   uint16_t _239 = _230 + _238;
   uint16_t _240 = _239 >> 1;
   uint8_t _241 = (uint8_t)(_240);
   _lowpass_x_2_stencil(0, 0, 0) = _241;
   uint8_t _242 = _demosaic_2_stencil(1, 0, 0);
   uint16_t _243 = (uint16_t)(_242);
   uint8_t _244 = _demosaic_2_stencil(1, 1, 0);
   uint16_t _245 = (uint16_t)(_244);
   uint16_t _246 = _243 + _245;
   uint16_t _247 = _246 >> 1;
   uint8_t _248 = (uint8_t)(_247);
   uint16_t _249 = (uint16_t)(_248);
   uint8_t _250 = _demosaic_2_stencil(1, 0, 1);
   uint16_t _251 = (uint16_t)(_250);
   uint8_t _252 = _demosaic_2_stencil(1, 1, 1);
   uint16_t _253 = (uint16_t)(_252);
   uint16_t _254 = _251 + _253;
   uint16_t _255 = _254 >> 1;
   uint8_t _256 = (uint8_t)(_255);
   uint16_t _257 = (uint16_t)(_256);
   uint16_t _258 = _249 + _257;
   uint16_t _259 = _258 >> 1;
   uint8_t _260 = (uint8_t)(_259);
   _lowpass_x_2_stencil(1, 0, 0) = _260;
   uint8_t _261 = _demosaic_2_stencil(2, 0, 0);
   uint16_t _262 = (uint16_t)(_261);
   uint8_t _263 = _demosaic_2_stencil(2, 1, 0);
   uint16_t _264 = (uint16_t)(_263);
   uint16_t _265 = _262 + _264;
   uint16_t _266 = _265 >> 1;
   uint8_t _267 = (uint8_t)(_266);
   uint16_t _268 = (uint16_t)(_267);
   uint8_t _269 = _demosaic_2_stencil(2, 0, 1);
   uint16_t _270 = (uint16_t)(_269);
   uint8_t _271 = _demosaic_2_stencil(2, 1, 1);
   uint16_t _272 = (uint16_t)(_271);
   uint16_t _273 = _270 + _272;
   uint16_t _274 = _273 >> 1;
   uint8_t _275 = (uint8_t)(_274);
   uint16_t _276 = (uint16_t)(_275);
   uint16_t _277 = _268 + _276;
   uint16_t _278 = _277 >> 1;
   uint8_t _279 = (uint8_t)(_278);
   _lowpass_x_2_stencil(2, 0, 0) = _279;
   _lowpass_x_2_stencil_stream.write(_lowpass_x_2_stencil);
   (void)0;
  } // for _lowpass_x_2_scan_x
 } // for _lowpass_x_2_scan_y
 // dispatch_stream(_lowpass_x_2_stencil_stream, 3, 3, 3, 3, 1, 1, 1440, 1, 1, 960, 1, "__auto_insert__hw_output$2", 1, 0, 3, 0, 1440, 0, 960);
 hls::stream<PackedStencil<uint8_t, 3, 1, 1> > &_lowpass_x_2_stencil_stream_to___auto_insert__hw_output_2 = _lowpass_x_2_stencil_stream;
 (void)0;
 // consume lowpass_x$2.stencil.stream
 // produce __auto_insert__hw_output$2.stencil.stream
 for (int _hw_output_2_s0_y_yi = 0; _hw_output_2_s0_y_yi < 0 + 960; _hw_output_2_s0_y_yi++)
 {
  for (int _hw_output_2_s0_x_xi = 0; _hw_output_2_s0_x_xi < 0 + 1440; _hw_output_2_s0_x_xi++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<uint8_t, 3, 1, 1> _lowpass_x_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_lowpass_x_2_stencil.value complete dim=0

   _lowpass_x_2_stencil = _lowpass_x_2_stencil_stream_to___auto_insert__hw_output_2.read();
   (void)0;
   Stencil<uint8_t, 3, 1, 1> __auto_insert__hw_output_2_stencil;
#pragma HLS ARRAY_PARTITION variable=__auto_insert__hw_output_2_stencil.value complete dim=0

   uint8_t _280 = _lowpass_x_2_stencil(0, 0, 0);
   __auto_insert__hw_output_2_stencil(0, 0, 0) = _280;
   uint8_t _281 = _lowpass_x_2_stencil(1, 0, 0);
   __auto_insert__hw_output_2_stencil(1, 0, 0) = _281;
   uint8_t _282 = _lowpass_x_2_stencil(2, 0, 0);
   __auto_insert__hw_output_2_stencil(2, 0, 0) = _282;
   AxiPackedStencil<uint8_t, 3, 1, 1>__auto_insert__hw_output_2_stencil_packed = __auto_insert__hw_output_2_stencil;
   __auto_insert__hw_output_2_stencil_stream.write(__auto_insert__hw_output_2_stencil_packed);
   (void)0;
  } // for _hw_output_2_s0_x_xi
 } // for _hw_output_2_s0_y_yi
 // consume __auto_insert__hw_output$2.stencil.stream
 /*
 uint8_t *buffer = (uint8_t *) malloc(3 * 1440 * 960);
 for (int y = 0; y < 0 + 960; y++)
 {
  for (int x = 0; x < 0 + 1440; x++)
  {
      Stencil<uint8_t, 3, 1, 1> stencil = __auto_insert__hw_output_2_stencil_stream.read();
      for (int c = 0; c < 3; c++) {
          buffer[c + x*3 + y*1440*3] = stencil(c, 0, 0);
      }
  }
 }
 for (int y = 0; y < 0 + 480; y++)
 {
  for (int x = 0; x < 0 + 720; x++)
  {
      Stencil<uint8_t, 3, 1, 1> stencil;
      for (int c = 0; c < 3; c++) {
          stencil(c, 0, 0) = buffer[c + x*3*2 + y*1440*3*2 + 4323];
      }
      out_stream.write(stencil);
  }
 }
 */
 for (int y = 0; y < 0 + 960; y++)
 {
  for (int x = 0; x < 0 + 1440; x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   AxiPackedStencil<uint8_t, 3, 1, 1> stencil = __auto_insert__hw_output_2_stencil_stream.read();
   if (y == 959 && x == 1439) {
       stencil.last = 1;
   } else {
       stencil.last = 0;
   }
   if (x % 2 == 1 && y % 2 == 1) {
       out_stream.write(stencil);
   }
  }
 }
} // kernel hls_target_hls_target


