#include "hls_target_downsample.h"

#include "Linebuffer.h"
#include "halide_math.h"
void hls_target(
hls::stream<AxiPackedStencil<uint8_t, 3, 1, 1> > &arg_0,
hls::stream<AxiPackedStencil<uint8_t, 2, 1> > &arg_1)
{
#pragma HLS DATAFLOW
#pragma HLS INLINE region
#pragma HLS INTERFACE s_axilite port=return bundle=config
#pragma HLS INTERFACE axis register port=arg_0
#pragma HLS INTERFACE axis register port=arg_1

 // alias the arguments
 hls::stream<AxiPackedStencil<uint8_t, 3, 1, 1> > &__auto_insert__hw_output_2_stencil_stream = arg_0;
 hls::stream<AxiPackedStencil<uint8_t, 2, 1> > &in_stream = arg_1;
 hls::stream<AxiPackedStencil<uint8_t, 2, 2> > _padded_2_stencil_update_stream;

 // buffer the input stream in order to fix HLS compiler issue,
 // where the loop nest for packing 2x2 stencils cannot be
 // pipelined.
 hls::stream<PackedStencil<uint8_t, 2, 1> > in_buffer_stream;

 for (int y = 0; y < 0 + 487*2; y++) {
     for (int x = 0; x < 0 + 727; x++) {
         PackedStencil<uint8_t, 2, 1> s = in_stream.read();
         in_buffer_stream.write(s);
     }
 }

 uint8_t buffer[727*2];

 for (int y = 0; y < 0 + 487*2; y++) {
     for (int x = 0; x < 0 + 727; x++) {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
         Stencil<uint8_t, 2, 1> in_stencil = in_buffer_stream.read();

         if (y % 2 == 0) {
             for (int i = 0; i < 2; i++)
                 buffer[x*2 + i] = in_stencil(i, 0);
         } else {
             Stencil<uint8_t, 2, 2> out_stencil;
             for (int i = 0; i < 2; i++)
                 out_stencil(i, 0) = buffer[x*2 + i];
             for (int i = 0; i < 2; i++)
                 out_stencil(i, 1) = in_stencil(i, 0);

             _padded_2_stencil_update_stream.write(out_stencil);
         }
     }
 }

 hls::stream<PackedStencil<uint8_t, 4, 4> > _padded_2_stencil_stream;
#pragma HLS STREAM variable=_padded_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_padded_2_stencil_stream core=FIFO_SRL

 // produce padded$2.stencil.stream
 linebuffer<1454, 974>(_padded_2_stencil_update_stream, _padded_2_stencil_stream);
 (void)0;
 // dispatch_stream(_padded_2_stencil_stream, 2, 4, 2, 1454, 4, 2, 974, 1, "downsample$2", 1, 0, 1454, 0, 974);
 hls::stream<PackedStencil<uint8_t, 4, 4> > &_padded_2_stencil_stream_to_downsample_2 = _padded_2_stencil_stream;
 (void)0;
 // consume padded$2.stencil.stream
 hls::stream<PackedStencil<uint8_t, 3, 1, 1> > _downsample_2_stencil_update_stream;
#pragma HLS STREAM variable=_downsample_2_stencil_update_stream depth=1
#pragma HLS RESOURCE variable=_downsample_2_stencil_update_stream core=FIFO_SRL

 // produce downsample$2.stencil_update.stream
 for (int _downsample_2_scan_update_y = 0; _downsample_2_scan_update_y < 0 + 486; _downsample_2_scan_update_y++)
 {
  for (int _downsample_2_scan_update_x = 0; _downsample_2_scan_update_x < 0 + 726; _downsample_2_scan_update_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<uint8_t, 4, 4> _padded_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_padded_2_stencil.value complete dim=0

   _padded_2_stencil = _padded_2_stencil_stream_to_downsample_2.read();
   (void)0;
   Stencil<uint8_t, 3, 1, 1> _downsample_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_downsample_2_stencil.value complete dim=0

   uint8_t _130 = _padded_2_stencil(1, 1);
   uint16_t _131 = (uint16_t)(_130);
   uint8_t _132 = _padded_2_stencil(3, 1);
   uint16_t _133 = (uint16_t)(_132);
   uint16_t _134 = _131 + _133;
   uint8_t _135 = _padded_2_stencil(1, 3);
   uint16_t _136 = _134 >> 1;
   uint8_t _137 = (uint8_t)(_136);
   uint16_t _138 = (uint16_t)(_137);
   uint16_t _139 = _131 + _138;
   uint16_t _140 = _139 >> 1;
   uint8_t _141 = (uint8_t)(_140);
   uint16_t _142 = (uint16_t)(_141);
   uint16_t _143 = (uint16_t)(_135);
   uint16_t _144 = _131 + _143;
   uint16_t _145 = _144 >> 1;
   uint8_t _146 = (uint8_t)(_145);
   uint16_t _147 = (uint16_t)(_146);
   uint16_t _148 = _134 + _143;
   uint8_t _149 = _padded_2_stencil(3, 3);
   uint16_t _150 = (uint16_t)(_149);
   uint16_t _151 = _148 + _150;
   uint16_t _152 = _151 >> 2;
   uint8_t _153 = (uint8_t)(_152);
   uint16_t _154 = (uint16_t)(_153);
   uint16_t _155 = _147 + _154;
   uint16_t _156 = _155 >> 1;
   uint8_t _157 = (uint8_t)(_156);
   uint16_t _158 = (uint16_t)(_157);
   uint16_t _159 = _142 + _158;
   uint16_t _160 = _159 >> 1;
   uint8_t _161 = (uint8_t)(_160);
   _downsample_2_stencil(0, 0, 0) = _161;
   uint8_t _162 = _padded_2_stencil(2, 1);
   uint8_t _163 = _padded_2_stencil(1, 2);
   uint8_t _164 = _padded_2_stencil(0, 1);
   uint16_t _165 = (uint16_t)(_164);
   uint16_t _166 = (uint16_t)(_162);
   uint16_t _167 = _165 + _166;
   uint8_t _168 = _padded_2_stencil(1, 0);
   uint16_t _169 = (uint16_t)(_168);
   uint16_t _170 = _167 + _169;
   uint16_t _171 = (uint16_t)(_163);
   uint16_t _172 = _170 + _171;
   uint16_t _173 = _172 >> 2;
   uint8_t _174 = (uint8_t)(_173);
   uint16_t _175 = (uint16_t)(_174);
   uint16_t _176 = _175 + _166;
   uint16_t _177 = _176 >> 1;
   uint8_t _178 = (uint8_t)(_177);
   uint16_t _179 = (uint16_t)(_178);
   uint8_t _180 = _padded_2_stencil(3, 2);
   uint16_t _181 = (uint16_t)(_180);
   uint16_t _182 = _171 + _181;
   uint16_t _183 = _182 + _166;
   uint8_t _184 = _padded_2_stencil(2, 3);
   uint16_t _185 = (uint16_t)(_184);
   uint16_t _186 = _183 + _185;
   uint16_t _187 = _186 >> 2;
   uint8_t _188 = (uint8_t)(_187);
   uint16_t _189 = (uint16_t)(_188);
   uint16_t _190 = _171 + _189;
   uint16_t _191 = _190 >> 1;
   uint8_t _192 = (uint8_t)(_191);
   uint16_t _193 = (uint16_t)(_192);
   uint16_t _194 = _179 + _193;
   uint16_t _195 = _194 >> 1;
   uint8_t _196 = (uint8_t)(_195);
   _downsample_2_stencil(1, 0, 0) = _196;
   uint8_t _197 = _padded_2_stencil(0, 2);
   uint8_t _198 = _padded_2_stencil(2, 2);
   uint8_t _199 = _padded_2_stencil(2, 0);
   uint8_t _200 = _padded_2_stencil(0, 0);
   uint16_t _201 = (uint16_t)(_200);
   uint16_t _202 = (uint16_t)(_199);
   uint16_t _203 = _201 + _202;
   uint16_t _204 = (uint16_t)(_197);
   uint16_t _205 = _203 + _204;
   uint16_t _206 = (uint16_t)(_198);
   uint16_t _207 = _205 + _206;
   uint16_t _208 = _207 >> 2;
   uint8_t _209 = (uint8_t)(_208);
   uint16_t _210 = (uint16_t)(_209);
   uint16_t _211 = _202 + _206;
   uint16_t _212 = _211 >> 1;
   uint8_t _213 = (uint8_t)(_212);
   uint16_t _214 = (uint16_t)(_213);
   uint16_t _215 = _210 + _214;
   uint16_t _216 = _215 >> 1;
   uint8_t _217 = (uint8_t)(_216);
   uint16_t _218 = (uint16_t)(_217);
   uint16_t _219 = _204 + _206;
   uint16_t _220 = _219 >> 1;
   uint8_t _221 = (uint8_t)(_220);
   uint16_t _222 = (uint16_t)(_221);
   uint16_t _223 = _222 + _206;
   uint16_t _224 = _223 >> 1;
   uint8_t _225 = (uint8_t)(_224);
   uint16_t _226 = (uint16_t)(_225);
   uint16_t _227 = _218 + _226;
   uint16_t _228 = _227 >> 1;
   uint8_t _229 = (uint8_t)(_228);
   _downsample_2_stencil(2, 0, 0) = _229;
   _downsample_2_stencil_update_stream.write(_downsample_2_stencil);
   (void)0;
  } // for _downsample_2_scan_update_x
 } // for _downsample_2_scan_update_y
 // consume downsample$2.stencil_update.stream
 hls::stream<PackedStencil<uint8_t, 3, 3, 3> > _downsample_2_stencil_stream;
#pragma HLS STREAM variable=_downsample_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_downsample_2_stencil_stream core=FIFO_SRL

 // produce downsample$2.stencil.stream
 linebuffer<3, 726, 486>(_downsample_2_stencil_update_stream, _downsample_2_stencil_stream);
 (void)0;
 // dispatch_stream(_downsample_2_stencil_stream, 3, 3, 3, 3, 3, 1, 726, 3, 1, 486, 3, "__auto_insert__hw_output$2", 5400, 0, 3, 1, 722, 1, 482, "p2:grad_x", 1, 0, 3, 0, 726, 0, 486, "p2:grad_y", 1, 0, 3, 0, 726, 0, 486);
 hls::stream<PackedStencil<uint8_t, 3, 3, 3> > _downsample_2_stencil_stream_to___auto_insert__hw_output_2;
#pragma HLS STREAM variable=_downsample_2_stencil_stream_to___auto_insert__hw_output_2 depth=5400
 hls::stream<PackedStencil<uint8_t, 3, 3, 3> > _downsample_2_stencil_stream_to_p2_grad_x;
#pragma HLS STREAM variable=_downsample_2_stencil_stream_to_p2_grad_x depth=1
#pragma HLS RESOURCE variable=_downsample_2_stencil_stream_to_p2_grad_x core=FIFO_SRL

 hls::stream<PackedStencil<uint8_t, 3, 3, 3> > _downsample_2_stencil_stream_to_p2_grad_y;
#pragma HLS STREAM variable=_downsample_2_stencil_stream_to_p2_grad_y depth=1
#pragma HLS RESOURCE variable=_downsample_2_stencil_stream_to_p2_grad_y core=FIFO_SRL

 for (int _dim_2 = 0; _dim_2 <= 483; _dim_2 += 1)
 for (int _dim_1 = 0; _dim_1 <= 723; _dim_1 += 1)
 for (int _dim_0 = 0; _dim_0 <= 0; _dim_0 += 3)
 {
#pragma HLS PIPELINE
  Stencil<uint8_t, 3, 3, 3> _tmp_stencil = _downsample_2_stencil_stream.read();
#pragma HLS ARRAY_PARTITION variable=_tmp_stencil.value complete dim=0

  if (_dim_0 >= 0 && _dim_0 <= 0 && _dim_1 >= 1 && _dim_1 <= 720 && _dim_2 >= 1 && _dim_2 <= 480)
  {
   _downsample_2_stencil_stream_to___auto_insert__hw_output_2.write(_tmp_stencil);
  }
  if (_dim_0 >= 0 && _dim_0 <= 0 && _dim_1 >= 0 && _dim_1 <= 723 && _dim_2 >= 0 && _dim_2 <= 483)
  {
   _downsample_2_stencil_stream_to_p2_grad_x.write(_tmp_stencil);
  }
  if (_dim_0 >= 0 && _dim_0 <= 0 && _dim_1 >= 0 && _dim_1 <= 723 && _dim_2 >= 0 && _dim_2 <= 483)
  {
   _downsample_2_stencil_stream_to_p2_grad_y.write(_tmp_stencil);
  }
 }
 (void)0;
 // consume downsample$2.stencil.stream
 hls::stream<PackedStencil<int16_t, 1, 1> > _p2_grad_x_stencil_stream;
#pragma HLS STREAM variable=_p2_grad_x_stencil_stream depth=1
#pragma HLS RESOURCE variable=_p2_grad_x_stencil_stream core=FIFO_SRL

 // produce p2:grad_x.stencil.stream
 for (int _p2_grad_x_scan_y = 0; _p2_grad_x_scan_y < 0 + 484; _p2_grad_x_scan_y++)
 {
  for (int _p2_grad_x_scan_x = 0; _p2_grad_x_scan_x < 0 + 724; _p2_grad_x_scan_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<uint8_t, 3, 3, 3> _downsample_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_downsample_2_stencil.value complete dim=0

   _downsample_2_stencil = _downsample_2_stencil_stream_to_p2_grad_x.read();
   (void)0;
   Stencil<int16_t, 1, 1> _p2_grad_x_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_x_stencil.value complete dim=0

   uint8_t _230 = _downsample_2_stencil(0, 2, 0);
   uint16_t _231 = (uint16_t)(_230);
   uint16_t _232 = (uint16_t)(77);
   uint16_t _233 = _231 * _232;
   uint8_t _234 = _downsample_2_stencil(1, 2, 0);
   uint16_t _235 = (uint16_t)(_234);
   uint16_t _236 = (uint16_t)(150);
   uint16_t _237 = _235 * _236;
   uint16_t _238 = _233 + _237;
   uint8_t _239 = _downsample_2_stencil(2, 2, 0);
   uint16_t _240 = (uint16_t)(_239);
   uint16_t _241 = (uint16_t)(29);
   uint16_t _242 = _240 * _241;
   uint16_t _243 = _238 + _242;
   uint16_t _244 = _243 >> 8;
   uint8_t _245 = (uint8_t)(_244);
   int16_t _246 = (int16_t)(_245);
   uint8_t _247 = _downsample_2_stencil(0, 0, 0);
   uint16_t _248 = (uint16_t)(_247);
   uint16_t _249 = _248 * _232;
   uint8_t _250 = _downsample_2_stencil(1, 0, 0);
   uint16_t _251 = (uint16_t)(_250);
   uint16_t _252 = _251 * _236;
   uint16_t _253 = _249 + _252;
   uint8_t _254 = _downsample_2_stencil(2, 0, 0);
   uint16_t _255 = (uint16_t)(_254);
   uint16_t _256 = _255 * _241;
   uint16_t _257 = _253 + _256;
   uint16_t _258 = _257 >> 8;
   uint8_t _259 = (uint8_t)(_258);
   int16_t _260 = (int16_t)(_259);
   int16_t _261 = _246 - _260;
   uint8_t _262 = _downsample_2_stencil(0, 0, 1);
   uint16_t _263 = (uint16_t)(_262);
   uint16_t _264 = _263 * _232;
   uint8_t _265 = _downsample_2_stencil(1, 0, 1);
   uint16_t _266 = (uint16_t)(_265);
   uint16_t _267 = _266 * _236;
   uint16_t _268 = _264 + _267;
   uint8_t _269 = _downsample_2_stencil(2, 0, 1);
   uint16_t _270 = (uint16_t)(_269);
   uint16_t _271 = _270 * _241;
   uint16_t _272 = _268 + _271;
   uint16_t _273 = _272 >> 8;
   uint8_t _274 = (uint8_t)(_273);
   int16_t _275 = (int16_t)(_274);
   int16_t _276 = (int16_t)(2);
   int16_t _277 = _275 * _276;
   int16_t _278 = _261 - _277;
   uint8_t _279 = _downsample_2_stencil(0, 2, 1);
   uint16_t _280 = (uint16_t)(_279);
   uint16_t _281 = _280 * _232;
   uint8_t _282 = _downsample_2_stencil(1, 2, 1);
   uint16_t _283 = (uint16_t)(_282);
   uint16_t _284 = _283 * _236;
   uint16_t _285 = _281 + _284;
   uint8_t _286 = _downsample_2_stencil(2, 2, 1);
   uint16_t _287 = (uint16_t)(_286);
   uint16_t _288 = _287 * _241;
   uint16_t _289 = _285 + _288;
   uint16_t _290 = _289 >> 8;
   uint8_t _291 = (uint8_t)(_290);
   int16_t _292 = (int16_t)(_291);
   int16_t _293 = _292 * _276;
   int16_t _294 = _278 + _293;
   uint8_t _295 = _downsample_2_stencil(0, 0, 2);
   uint16_t _296 = (uint16_t)(_295);
   uint16_t _297 = _296 * _232;
   uint8_t _298 = _downsample_2_stencil(1, 0, 2);
   uint16_t _299 = (uint16_t)(_298);
   uint16_t _300 = _299 * _236;
   uint16_t _301 = _297 + _300;
   uint8_t _302 = _downsample_2_stencil(2, 0, 2);
   uint16_t _303 = (uint16_t)(_302);
   uint16_t _304 = _303 * _241;
   uint16_t _305 = _301 + _304;
   uint16_t _306 = _305 >> 8;
   uint8_t _307 = (uint8_t)(_306);
   int16_t _308 = (int16_t)(_307);
   int16_t _309 = _294 - _308;
   uint8_t _310 = _downsample_2_stencil(0, 2, 2);
   uint16_t _311 = (uint16_t)(_310);
   uint16_t _312 = _311 * _232;
   uint8_t _313 = _downsample_2_stencil(1, 2, 2);
   uint16_t _314 = (uint16_t)(_313);
   uint16_t _315 = _314 * _236;
   uint16_t _316 = _312 + _315;
   uint8_t _317 = _downsample_2_stencil(2, 2, 2);
   uint16_t _318 = (uint16_t)(_317);
   uint16_t _319 = _318 * _241;
   uint16_t _320 = _316 + _319;
   uint16_t _321 = _320 >> 8;
   uint8_t _322 = (uint8_t)(_321);
   int16_t _323 = (int16_t)(_322);
   int16_t _324 = _309 + _323;
   _p2_grad_x_stencil(0, 0) = _324;
   _p2_grad_x_stencil_stream.write(_p2_grad_x_stencil);
   (void)0;
  } // for _p2_grad_x_scan_x
 } // for _p2_grad_x_scan_y
 // dispatch_stream(_p2_grad_x_stencil_stream, 2, 1, 1, 724, 1, 1, 484, 2, "p2:grad_xx", 1, 0, 724, 0, 484, "p2:grad_xy", 1, 0, 724, 0, 484);
 hls::stream<PackedStencil<int16_t, 1, 1> > _p2_grad_x_stencil_stream_to_p2_grad_xx;
#pragma HLS STREAM variable=_p2_grad_x_stencil_stream_to_p2_grad_xx depth=1
#pragma HLS RESOURCE variable=_p2_grad_x_stencil_stream_to_p2_grad_xx core=FIFO_SRL

 hls::stream<PackedStencil<int16_t, 1, 1> > _p2_grad_x_stencil_stream_to_p2_grad_xy;
#pragma HLS STREAM variable=_p2_grad_x_stencil_stream_to_p2_grad_xy depth=1
#pragma HLS RESOURCE variable=_p2_grad_x_stencil_stream_to_p2_grad_xy core=FIFO_SRL

 for (int _dim_1 = 0; _dim_1 <= 483; _dim_1 += 1)
 for (int _dim_0 = 0; _dim_0 <= 723; _dim_0 += 1)
 {
#pragma HLS PIPELINE
  Stencil<int16_t, 1, 1> _tmp_stencil = _p2_grad_x_stencil_stream.read();
#pragma HLS ARRAY_PARTITION variable=_tmp_stencil.value complete dim=0

  if (_dim_0 >= 0 && _dim_0 <= 723 && _dim_1 >= 0 && _dim_1 <= 483)
  {
   _p2_grad_x_stencil_stream_to_p2_grad_xx.write(_tmp_stencil);
  }
  if (_dim_0 >= 0 && _dim_0 <= 723 && _dim_1 >= 0 && _dim_1 <= 483)
  {
   _p2_grad_x_stencil_stream_to_p2_grad_xy.write(_tmp_stencil);
  }
 }
 (void)0;
 // consume p2:grad_x.stencil.stream
 hls::stream<PackedStencil<int32_t, 1, 1> > _p2_grad_xx_stencil_update_stream;
#pragma HLS STREAM variable=_p2_grad_xx_stencil_update_stream depth=1
#pragma HLS RESOURCE variable=_p2_grad_xx_stencil_update_stream core=FIFO_SRL

 // produce p2:grad_xx.stencil_update.stream
 for (int _p2_grad_xx_scan_update_y = 0; _p2_grad_xx_scan_update_y < 0 + 484; _p2_grad_xx_scan_update_y++)
 {
  for (int _p2_grad_xx_scan_update_x = 0; _p2_grad_xx_scan_update_x < 0 + 724; _p2_grad_xx_scan_update_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<int16_t, 1, 1> _p2_grad_x_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_x_stencil.value complete dim=0

   _p2_grad_x_stencil = _p2_grad_x_stencil_stream_to_p2_grad_xx.read();
   (void)0;
   Stencil<int32_t, 1, 1> _p2_grad_xx_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_xx_stencil.value complete dim=0

   int16_t _325 = _p2_grad_x_stencil(0, 0);
   int32_t _326 = (int32_t)(_325);
   int32_t _327 = _326 * _326;
   _p2_grad_xx_stencil(0, 0) = _327;
   _p2_grad_xx_stencil_update_stream.write(_p2_grad_xx_stencil);
   (void)0;
  } // for _p2_grad_xx_scan_update_x
 } // for _p2_grad_xx_scan_update_y
 // consume p2:grad_xx.stencil_update.stream
 hls::stream<PackedStencil<int32_t, 3, 3> > _p2_grad_xx_stencil_stream;
#pragma HLS STREAM variable=_p2_grad_xx_stencil_stream depth=1
#pragma HLS RESOURCE variable=_p2_grad_xx_stencil_stream core=FIFO_SRL

 // produce p2:grad_xx.stencil.stream
 linebuffer<724, 484>(_p2_grad_xx_stencil_update_stream, _p2_grad_xx_stencil_stream);
 (void)0;
 // dispatch_stream(_p2_grad_xx_stencil_stream, 2, 3, 1, 724, 3, 1, 484, 1, "p2:grad_gx", 1, 0, 724, 0, 484);
 hls::stream<PackedStencil<int32_t, 3, 3> > &_p2_grad_xx_stencil_stream_to_p2_grad_gx = _p2_grad_xx_stencil_stream;
 (void)0;
 // consume p2:grad_xx.stencil.stream
 hls::stream<PackedStencil<int32_t, 1, 1> > _p2_grad_gx_stencil_stream;
#pragma HLS STREAM variable=_p2_grad_gx_stencil_stream depth=1
#pragma HLS RESOURCE variable=_p2_grad_gx_stencil_stream core=FIFO_SRL

 // produce p2:grad_gx.stencil.stream
 for (int _p2_grad_gx_scan_y = 0; _p2_grad_gx_scan_y < 0 + 482; _p2_grad_gx_scan_y++)
 {
  for (int _p2_grad_gx_scan_x = 0; _p2_grad_gx_scan_x < 0 + 722; _p2_grad_gx_scan_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<int32_t, 3, 3> _p2_grad_xx_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_xx_stencil.value complete dim=0

   _p2_grad_xx_stencil = _p2_grad_xx_stencil_stream_to_p2_grad_gx.read();
   (void)0;
   Stencil<int32_t, 1, 1> _p2_grad_gx_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_gx_stencil.value complete dim=0

   _p2_grad_gx_stencil(0, 0) = 0;
   int32_t _328 = _p2_grad_gx_stencil(0, 0);
   int32_t _329 = _p2_grad_xx_stencil(0, 0);
   int32_t _330 = _328 + _329;
   _p2_grad_gx_stencil(0, 0) = _330;
   int32_t _331 = _p2_grad_gx_stencil(0, 0);
   int32_t _332 = _p2_grad_xx_stencil(1, 0);
   int32_t _333 = _331 + _332;
   _p2_grad_gx_stencil(0, 0) = _333;
   int32_t _334 = _p2_grad_gx_stencil(0, 0);
   int32_t _335 = _p2_grad_xx_stencil(2, 0);
   int32_t _336 = _334 + _335;
   _p2_grad_gx_stencil(0, 0) = _336;
   int32_t _337 = _p2_grad_gx_stencil(0, 0);
   int32_t _338 = _p2_grad_xx_stencil(0, 1);
   int32_t _339 = _337 + _338;
   _p2_grad_gx_stencil(0, 0) = _339;
   int32_t _340 = _p2_grad_gx_stencil(0, 0);
   int32_t _341 = _p2_grad_xx_stencil(1, 1);
   int32_t _342 = _340 + _341;
   _p2_grad_gx_stencil(0, 0) = _342;
   int32_t _343 = _p2_grad_gx_stencil(0, 0);
   int32_t _344 = _p2_grad_xx_stencil(2, 1);
   int32_t _345 = _343 + _344;
   _p2_grad_gx_stencil(0, 0) = _345;
   int32_t _346 = _p2_grad_gx_stencil(0, 0);
   int32_t _347 = _p2_grad_xx_stencil(0, 2);
   int32_t _348 = _346 + _347;
   _p2_grad_gx_stencil(0, 0) = _348;
   int32_t _349 = _p2_grad_gx_stencil(0, 0);
   int32_t _350 = _p2_grad_xx_stencil(1, 2);
   int32_t _351 = _349 + _350;
   _p2_grad_gx_stencil(0, 0) = _351;
   int32_t _352 = _p2_grad_gx_stencil(0, 0);
   int32_t _353 = _p2_grad_xx_stencil(2, 2);
   int32_t _354 = _352 + _353;
   _p2_grad_gx_stencil(0, 0) = _354;
   _p2_grad_gx_stencil_stream.write(_p2_grad_gx_stencil);
   (void)0;
  } // for _p2_grad_gx_scan_x
 } // for _p2_grad_gx_scan_y
 // dispatch_stream(_p2_grad_gx_stencil_stream, 2, 1, 1, 722, 1, 1, 482, 1, "p2:cim", 1, 0, 722, 0, 482);
 hls::stream<PackedStencil<int32_t, 1, 1> > &_p2_grad_gx_stencil_stream_to_p2_cim = _p2_grad_gx_stencil_stream;
 (void)0;
 // consume p2:grad_gx.stencil.stream
 hls::stream<PackedStencil<int16_t, 1, 1> > _p2_grad_y_stencil_stream;
#pragma HLS STREAM variable=_p2_grad_y_stencil_stream depth=1
#pragma HLS RESOURCE variable=_p2_grad_y_stencil_stream core=FIFO_SRL

 // produce p2:grad_y.stencil.stream
 for (int _p2_grad_y_scan_y = 0; _p2_grad_y_scan_y < 0 + 484; _p2_grad_y_scan_y++)
 {
  for (int _p2_grad_y_scan_x = 0; _p2_grad_y_scan_x < 0 + 724; _p2_grad_y_scan_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<uint8_t, 3, 3, 3> _downsample_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_downsample_2_stencil.value complete dim=0

   _downsample_2_stencil = _downsample_2_stencil_stream_to_p2_grad_y.read();
   (void)0;
   Stencil<int16_t, 1, 1> _p2_grad_y_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_y_stencil.value complete dim=0

   uint8_t _355 = _downsample_2_stencil(0, 0, 2);
   uint16_t _356 = (uint16_t)(_355);
   uint16_t _357 = (uint16_t)(77);
   uint16_t _358 = _356 * _357;
   uint8_t _359 = _downsample_2_stencil(1, 0, 2);
   uint16_t _360 = (uint16_t)(_359);
   uint16_t _361 = (uint16_t)(150);
   uint16_t _362 = _360 * _361;
   uint16_t _363 = _358 + _362;
   uint8_t _364 = _downsample_2_stencil(2, 0, 2);
   uint16_t _365 = (uint16_t)(_364);
   uint16_t _366 = (uint16_t)(29);
   uint16_t _367 = _365 * _366;
   uint16_t _368 = _363 + _367;
   uint16_t _369 = _368 >> 8;
   uint8_t _370 = (uint8_t)(_369);
   int16_t _371 = (int16_t)(_370);
   uint8_t _372 = _downsample_2_stencil(0, 0, 0);
   uint16_t _373 = (uint16_t)(_372);
   uint16_t _374 = _373 * _357;
   uint8_t _375 = _downsample_2_stencil(1, 0, 0);
   uint16_t _376 = (uint16_t)(_375);
   uint16_t _377 = _376 * _361;
   uint16_t _378 = _374 + _377;
   uint8_t _379 = _downsample_2_stencil(2, 0, 0);
   uint16_t _380 = (uint16_t)(_379);
   uint16_t _381 = _380 * _366;
   uint16_t _382 = _378 + _381;
   uint16_t _383 = _382 >> 8;
   uint8_t _384 = (uint8_t)(_383);
   int16_t _385 = (int16_t)(_384);
   int16_t _386 = _371 - _385;
   uint8_t _387 = _downsample_2_stencil(0, 1, 2);
   uint16_t _388 = (uint16_t)(_387);
   uint16_t _389 = _388 * _357;
   uint8_t _390 = _downsample_2_stencil(1, 1, 2);
   uint16_t _391 = (uint16_t)(_390);
   uint16_t _392 = _391 * _361;
   uint16_t _393 = _389 + _392;
   uint8_t _394 = _downsample_2_stencil(2, 1, 2);
   uint16_t _395 = (uint16_t)(_394);
   uint16_t _396 = _395 * _366;
   uint16_t _397 = _393 + _396;
   uint16_t _398 = _397 >> 8;
   uint8_t _399 = (uint8_t)(_398);
   int16_t _400 = (int16_t)(_399);
   int16_t _401 = (int16_t)(2);
   int16_t _402 = _400 * _401;
   int16_t _403 = _386 + _402;
   uint8_t _404 = _downsample_2_stencil(0, 1, 0);
   uint16_t _405 = (uint16_t)(_404);
   uint16_t _406 = _405 * _357;
   uint8_t _407 = _downsample_2_stencil(1, 1, 0);
   uint16_t _408 = (uint16_t)(_407);
   uint16_t _409 = _408 * _361;
   uint16_t _410 = _406 + _409;
   uint8_t _411 = _downsample_2_stencil(2, 1, 0);
   uint16_t _412 = (uint16_t)(_411);
   uint16_t _413 = _412 * _366;
   uint16_t _414 = _410 + _413;
   uint16_t _415 = _414 >> 8;
   uint8_t _416 = (uint8_t)(_415);
   int16_t _417 = (int16_t)(_416);
   int16_t _418 = _417 * _401;
   int16_t _419 = _403 - _418;
   uint8_t _420 = _downsample_2_stencil(0, 2, 2);
   uint16_t _421 = (uint16_t)(_420);
   uint16_t _422 = _421 * _357;
   uint8_t _423 = _downsample_2_stencil(1, 2, 2);
   uint16_t _424 = (uint16_t)(_423);
   uint16_t _425 = _424 * _361;
   uint16_t _426 = _422 + _425;
   uint8_t _427 = _downsample_2_stencil(2, 2, 2);
   uint16_t _428 = (uint16_t)(_427);
   uint16_t _429 = _428 * _366;
   uint16_t _430 = _426 + _429;
   uint16_t _431 = _430 >> 8;
   uint8_t _432 = (uint8_t)(_431);
   int16_t _433 = (int16_t)(_432);
   int16_t _434 = _419 + _433;
   uint8_t _435 = _downsample_2_stencil(0, 2, 0);
   uint16_t _436 = (uint16_t)(_435);
   uint16_t _437 = _436 * _357;
   uint8_t _438 = _downsample_2_stencil(1, 2, 0);
   uint16_t _439 = (uint16_t)(_438);
   uint16_t _440 = _439 * _361;
   uint16_t _441 = _437 + _440;
   uint8_t _442 = _downsample_2_stencil(2, 2, 0);
   uint16_t _443 = (uint16_t)(_442);
   uint16_t _444 = _443 * _366;
   uint16_t _445 = _441 + _444;
   uint16_t _446 = _445 >> 8;
   uint8_t _447 = (uint8_t)(_446);
   int16_t _448 = (int16_t)(_447);
   int16_t _449 = _434 - _448;
   _p2_grad_y_stencil(0, 0) = _449;
   _p2_grad_y_stencil_stream.write(_p2_grad_y_stencil);
   (void)0;
  } // for _p2_grad_y_scan_x
 } // for _p2_grad_y_scan_y
 // dispatch_stream(_p2_grad_y_stencil_stream, 2, 1, 1, 724, 1, 1, 484, 2, "p2:grad_xy", 1, 0, 724, 0, 484, "p2:grad_yy", 1, 0, 724, 0, 484);
 hls::stream<PackedStencil<int16_t, 1, 1> > _p2_grad_y_stencil_stream_to_p2_grad_xy;
#pragma HLS STREAM variable=_p2_grad_y_stencil_stream_to_p2_grad_xy depth=1
#pragma HLS RESOURCE variable=_p2_grad_y_stencil_stream_to_p2_grad_xy core=FIFO_SRL

 hls::stream<PackedStencil<int16_t, 1, 1> > _p2_grad_y_stencil_stream_to_p2_grad_yy;
#pragma HLS STREAM variable=_p2_grad_y_stencil_stream_to_p2_grad_yy depth=1
#pragma HLS RESOURCE variable=_p2_grad_y_stencil_stream_to_p2_grad_yy core=FIFO_SRL

 for (int _dim_1 = 0; _dim_1 <= 483; _dim_1 += 1)
 for (int _dim_0 = 0; _dim_0 <= 723; _dim_0 += 1)
 {
#pragma HLS PIPELINE
  Stencil<int16_t, 1, 1> _tmp_stencil = _p2_grad_y_stencil_stream.read();
#pragma HLS ARRAY_PARTITION variable=_tmp_stencil.value complete dim=0

  if (_dim_0 >= 0 && _dim_0 <= 723 && _dim_1 >= 0 && _dim_1 <= 483)
  {
   _p2_grad_y_stencil_stream_to_p2_grad_xy.write(_tmp_stencil);
  }
  if (_dim_0 >= 0 && _dim_0 <= 723 && _dim_1 >= 0 && _dim_1 <= 483)
  {
   _p2_grad_y_stencil_stream_to_p2_grad_yy.write(_tmp_stencil);
  }
 }
 (void)0;
 // consume p2:grad_y.stencil.stream
 hls::stream<PackedStencil<int32_t, 1, 1> > _p2_grad_xy_stencil_update_stream;
#pragma HLS STREAM variable=_p2_grad_xy_stencil_update_stream depth=1
#pragma HLS RESOURCE variable=_p2_grad_xy_stencil_update_stream core=FIFO_SRL

 // produce p2:grad_xy.stencil_update.stream
 for (int _p2_grad_xy_scan_update_y = 0; _p2_grad_xy_scan_update_y < 0 + 484; _p2_grad_xy_scan_update_y++)
 {
  for (int _p2_grad_xy_scan_update_x = 0; _p2_grad_xy_scan_update_x < 0 + 724; _p2_grad_xy_scan_update_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<int16_t, 1, 1> _p2_grad_y_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_y_stencil.value complete dim=0

   _p2_grad_y_stencil = _p2_grad_y_stencil_stream_to_p2_grad_xy.read();
   (void)0;
   Stencil<int16_t, 1, 1> _p2_grad_x_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_x_stencil.value complete dim=0

   _p2_grad_x_stencil = _p2_grad_x_stencil_stream_to_p2_grad_xy.read();
   (void)0;
   Stencil<int32_t, 1, 1> _p2_grad_xy_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_xy_stencil.value complete dim=0

   int16_t _450 = _p2_grad_x_stencil(0, 0);
   int32_t _451 = (int32_t)(_450);
   int16_t _452 = _p2_grad_y_stencil(0, 0);
   int32_t _453 = (int32_t)(_452);
   int32_t _454 = _451 * _453;
   _p2_grad_xy_stencil(0, 0) = _454;
   _p2_grad_xy_stencil_update_stream.write(_p2_grad_xy_stencil);
   (void)0;
  } // for _p2_grad_xy_scan_update_x
 } // for _p2_grad_xy_scan_update_y
 // consume p2:grad_xy.stencil_update.stream
 hls::stream<PackedStencil<int32_t, 3, 3> > _p2_grad_xy_stencil_stream;
#pragma HLS STREAM variable=_p2_grad_xy_stencil_stream depth=1
#pragma HLS RESOURCE variable=_p2_grad_xy_stencil_stream core=FIFO_SRL

 // produce p2:grad_xy.stencil.stream
 linebuffer<724, 484>(_p2_grad_xy_stencil_update_stream, _p2_grad_xy_stencil_stream);
 (void)0;
 // dispatch_stream(_p2_grad_xy_stencil_stream, 2, 3, 1, 724, 3, 1, 484, 1, "p2:grad_gxy", 1, 0, 724, 0, 484);
 hls::stream<PackedStencil<int32_t, 3, 3> > &_p2_grad_xy_stencil_stream_to_p2_grad_gxy = _p2_grad_xy_stencil_stream;
 (void)0;
 // consume p2:grad_xy.stencil.stream
 hls::stream<PackedStencil<int32_t, 1, 1> > _p2_grad_gxy_stencil_stream;
#pragma HLS STREAM variable=_p2_grad_gxy_stencil_stream depth=1
#pragma HLS RESOURCE variable=_p2_grad_gxy_stencil_stream core=FIFO_SRL

 // produce p2:grad_gxy.stencil.stream
 for (int _p2_grad_gxy_scan_y = 0; _p2_grad_gxy_scan_y < 0 + 482; _p2_grad_gxy_scan_y++)
 {
  for (int _p2_grad_gxy_scan_x = 0; _p2_grad_gxy_scan_x < 0 + 722; _p2_grad_gxy_scan_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<int32_t, 3, 3> _p2_grad_xy_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_xy_stencil.value complete dim=0

   _p2_grad_xy_stencil = _p2_grad_xy_stencil_stream_to_p2_grad_gxy.read();
   (void)0;
   Stencil<int32_t, 1, 1> _p2_grad_gxy_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_gxy_stencil.value complete dim=0

   _p2_grad_gxy_stencil(0, 0) = 0;
   int32_t _455 = _p2_grad_gxy_stencil(0, 0);
   int32_t _456 = _p2_grad_xy_stencil(0, 0);
   int32_t _457 = _455 + _456;
   _p2_grad_gxy_stencil(0, 0) = _457;
   int32_t _458 = _p2_grad_gxy_stencil(0, 0);
   int32_t _459 = _p2_grad_xy_stencil(1, 0);
   int32_t _460 = _458 + _459;
   _p2_grad_gxy_stencil(0, 0) = _460;
   int32_t _461 = _p2_grad_gxy_stencil(0, 0);
   int32_t _462 = _p2_grad_xy_stencil(2, 0);
   int32_t _463 = _461 + _462;
   _p2_grad_gxy_stencil(0, 0) = _463;
   int32_t _464 = _p2_grad_gxy_stencil(0, 0);
   int32_t _465 = _p2_grad_xy_stencil(0, 1);
   int32_t _466 = _464 + _465;
   _p2_grad_gxy_stencil(0, 0) = _466;
   int32_t _467 = _p2_grad_gxy_stencil(0, 0);
   int32_t _468 = _p2_grad_xy_stencil(1, 1);
   int32_t _469 = _467 + _468;
   _p2_grad_gxy_stencil(0, 0) = _469;
   int32_t _470 = _p2_grad_gxy_stencil(0, 0);
   int32_t _471 = _p2_grad_xy_stencil(2, 1);
   int32_t _472 = _470 + _471;
   _p2_grad_gxy_stencil(0, 0) = _472;
   int32_t _473 = _p2_grad_gxy_stencil(0, 0);
   int32_t _474 = _p2_grad_xy_stencil(0, 2);
   int32_t _475 = _473 + _474;
   _p2_grad_gxy_stencil(0, 0) = _475;
   int32_t _476 = _p2_grad_gxy_stencil(0, 0);
   int32_t _477 = _p2_grad_xy_stencil(1, 2);
   int32_t _478 = _476 + _477;
   _p2_grad_gxy_stencil(0, 0) = _478;
   int32_t _479 = _p2_grad_gxy_stencil(0, 0);
   int32_t _480 = _p2_grad_xy_stencil(2, 2);
   int32_t _481 = _479 + _480;
   _p2_grad_gxy_stencil(0, 0) = _481;
   _p2_grad_gxy_stencil_stream.write(_p2_grad_gxy_stencil);
   (void)0;
  } // for _p2_grad_gxy_scan_x
 } // for _p2_grad_gxy_scan_y
 // dispatch_stream(_p2_grad_gxy_stencil_stream, 2, 1, 1, 722, 1, 1, 482, 1, "p2:cim", 1, 0, 722, 0, 482);
 hls::stream<PackedStencil<int32_t, 1, 1> > &_p2_grad_gxy_stencil_stream_to_p2_cim = _p2_grad_gxy_stencil_stream;
 (void)0;
 // consume p2:grad_gxy.stencil.stream
 hls::stream<PackedStencil<int32_t, 1, 1> > _p2_grad_yy_stencil_update_stream;
#pragma HLS STREAM variable=_p2_grad_yy_stencil_update_stream depth=1
#pragma HLS RESOURCE variable=_p2_grad_yy_stencil_update_stream core=FIFO_SRL

 // produce p2:grad_yy.stencil_update.stream
 for (int _p2_grad_yy_scan_update_y = 0; _p2_grad_yy_scan_update_y < 0 + 484; _p2_grad_yy_scan_update_y++)
 {
  for (int _p2_grad_yy_scan_update_x = 0; _p2_grad_yy_scan_update_x < 0 + 724; _p2_grad_yy_scan_update_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<int16_t, 1, 1> _p2_grad_y_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_y_stencil.value complete dim=0

   _p2_grad_y_stencil = _p2_grad_y_stencil_stream_to_p2_grad_yy.read();
   (void)0;
   Stencil<int32_t, 1, 1> _p2_grad_yy_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_yy_stencil.value complete dim=0

   int16_t _482 = _p2_grad_y_stencil(0, 0);
   int32_t _483 = (int32_t)(_482);
   int32_t _484 = _483 * _483;
   _p2_grad_yy_stencil(0, 0) = _484;
   _p2_grad_yy_stencil_update_stream.write(_p2_grad_yy_stencil);
   (void)0;
  } // for _p2_grad_yy_scan_update_x
 } // for _p2_grad_yy_scan_update_y
 // consume p2:grad_yy.stencil_update.stream
 hls::stream<PackedStencil<int32_t, 3, 3> > _p2_grad_yy_stencil_stream;
#pragma HLS STREAM variable=_p2_grad_yy_stencil_stream depth=1
#pragma HLS RESOURCE variable=_p2_grad_yy_stencil_stream core=FIFO_SRL

 // produce p2:grad_yy.stencil.stream
 linebuffer<724, 484>(_p2_grad_yy_stencil_update_stream, _p2_grad_yy_stencil_stream);
 (void)0;
 // dispatch_stream(_p2_grad_yy_stencil_stream, 2, 3, 1, 724, 3, 1, 484, 1, "p2:grad_gy", 1, 0, 724, 0, 484);
 hls::stream<PackedStencil<int32_t, 3, 3> > &_p2_grad_yy_stencil_stream_to_p2_grad_gy = _p2_grad_yy_stencil_stream;
 (void)0;
 // consume p2:grad_yy.stencil.stream
 hls::stream<PackedStencil<int32_t, 1, 1> > _p2_grad_gy_stencil_stream;
#pragma HLS STREAM variable=_p2_grad_gy_stencil_stream depth=1
#pragma HLS RESOURCE variable=_p2_grad_gy_stencil_stream core=FIFO_SRL

 // produce p2:grad_gy.stencil.stream
 for (int _p2_grad_gy_scan_y = 0; _p2_grad_gy_scan_y < 0 + 482; _p2_grad_gy_scan_y++)
 {
  for (int _p2_grad_gy_scan_x = 0; _p2_grad_gy_scan_x < 0 + 722; _p2_grad_gy_scan_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<int32_t, 3, 3> _p2_grad_yy_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_yy_stencil.value complete dim=0

   _p2_grad_yy_stencil = _p2_grad_yy_stencil_stream_to_p2_grad_gy.read();
   (void)0;
   Stencil<int32_t, 1, 1> _p2_grad_gy_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_gy_stencil.value complete dim=0

   _p2_grad_gy_stencil(0, 0) = 0;
   int32_t _485 = _p2_grad_gy_stencil(0, 0);
   int32_t _486 = _p2_grad_yy_stencil(0, 0);
   int32_t _487 = _485 + _486;
   _p2_grad_gy_stencil(0, 0) = _487;
   int32_t _488 = _p2_grad_gy_stencil(0, 0);
   int32_t _489 = _p2_grad_yy_stencil(1, 0);
   int32_t _490 = _488 + _489;
   _p2_grad_gy_stencil(0, 0) = _490;
   int32_t _491 = _p2_grad_gy_stencil(0, 0);
   int32_t _492 = _p2_grad_yy_stencil(2, 0);
   int32_t _493 = _491 + _492;
   _p2_grad_gy_stencil(0, 0) = _493;
   int32_t _494 = _p2_grad_gy_stencil(0, 0);
   int32_t _495 = _p2_grad_yy_stencil(0, 1);
   int32_t _496 = _494 + _495;
   _p2_grad_gy_stencil(0, 0) = _496;
   int32_t _497 = _p2_grad_gy_stencil(0, 0);
   int32_t _498 = _p2_grad_yy_stencil(1, 1);
   int32_t _499 = _497 + _498;
   _p2_grad_gy_stencil(0, 0) = _499;
   int32_t _500 = _p2_grad_gy_stencil(0, 0);
   int32_t _501 = _p2_grad_yy_stencil(2, 1);
   int32_t _502 = _500 + _501;
   _p2_grad_gy_stencil(0, 0) = _502;
   int32_t _503 = _p2_grad_gy_stencil(0, 0);
   int32_t _504 = _p2_grad_yy_stencil(0, 2);
   int32_t _505 = _503 + _504;
   _p2_grad_gy_stencil(0, 0) = _505;
   int32_t _506 = _p2_grad_gy_stencil(0, 0);
   int32_t _507 = _p2_grad_yy_stencil(1, 2);
   int32_t _508 = _506 + _507;
   _p2_grad_gy_stencil(0, 0) = _508;
   int32_t _509 = _p2_grad_gy_stencil(0, 0);
   int32_t _510 = _p2_grad_yy_stencil(2, 2);
   int32_t _511 = _509 + _510;
   _p2_grad_gy_stencil(0, 0) = _511;
   _p2_grad_gy_stencil_stream.write(_p2_grad_gy_stencil);
   (void)0;
  } // for _p2_grad_gy_scan_x
 } // for _p2_grad_gy_scan_y
 // dispatch_stream(_p2_grad_gy_stencil_stream, 2, 1, 1, 722, 1, 1, 482, 1, "p2:cim", 1, 0, 722, 0, 482);
 hls::stream<PackedStencil<int32_t, 1, 1> > &_p2_grad_gy_stencil_stream_to_p2_cim = _p2_grad_gy_stencil_stream;
 (void)0;
 // consume p2:grad_gy.stencil.stream
 hls::stream<PackedStencil<float, 1, 1> > _p2_cim_stencil_update_stream;
#pragma HLS STREAM variable=_p2_cim_stencil_update_stream depth=1
#pragma HLS RESOURCE variable=_p2_cim_stencil_update_stream core=FIFO_SRL

 // produce p2:cim.stencil_update.stream
 for (int _p2_cim_scan_update_y = 0; _p2_cim_scan_update_y < 0 + 482; _p2_cim_scan_update_y++)
 {
  for (int _p2_cim_scan_update_x = 0; _p2_cim_scan_update_x < 0 + 722; _p2_cim_scan_update_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<int32_t, 1, 1> _p2_grad_gy_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_gy_stencil.value complete dim=0

   _p2_grad_gy_stencil = _p2_grad_gy_stencil_stream_to_p2_cim.read();
   (void)0;
   Stencil<int32_t, 1, 1> _p2_grad_gxy_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_gxy_stencil.value complete dim=0

   _p2_grad_gxy_stencil = _p2_grad_gxy_stencil_stream_to_p2_cim.read();
   (void)0;
   Stencil<int32_t, 1, 1> _p2_grad_gx_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_gx_stencil.value complete dim=0

   _p2_grad_gx_stencil = _p2_grad_gx_stencil_stream_to_p2_cim.read();
   (void)0;
   Stencil<float, 1, 1> _p2_cim_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_cim_stencil.value complete dim=0

   int32_t _512 = _p2_grad_gx_stencil(0, 0);
   int32_t _513 = _p2_grad_gy_stencil(0, 0);
   int32_t _514 = _p2_grad_gxy_stencil(0, 0);
   int32_t _515 = _512 / 144;
   int32_t _516 = _512 - _515 * 144;
   int32_t _517 = 144 >> (int32_t)31;
   int32_t _518 = _516 >> (int32_t)31;
   int32_t _519 = _515 - (_518 & _517) + (_518 & ~_517);
   float _520 = (float)(_519);
   int32_t _521 = _513 / 144;
   int32_t _522 = _513 - _521 * 144;
   int32_t _523 = _522 >> (int32_t)31;
   int32_t _524 = _521 - (_523 & _517) + (_523 & ~_517);
   float _525 = (float)(_524);
   float _526 = _520 + _525;
   float _527 = _520 * _525;
   int32_t _528 = _514 / 144;
   int32_t _529 = _514 - _528 * 144;
   int32_t _530 = _529 >> (int32_t)31;
   int32_t _531 = _528 - (_530 & _517) + (_530 & ~_517);
   float _532 = (float)(_531);
   float _533 = _532 * _532;
   float _534 = _527 - _533;
   float _535 = _526 * float_from_bits(1025758986 /* 0.04 */);
   float _536 = _535 * _526;
   float _537 = _534 - _536;
   _p2_cim_stencil(0, 0) = _537;
   _p2_cim_stencil_update_stream.write(_p2_cim_stencil);
   (void)0;
  } // for _p2_cim_scan_update_x
 } // for _p2_cim_scan_update_y
 // consume p2:cim.stencil_update.stream
 hls::stream<PackedStencil<float, 3, 3> > _p2_cim_stencil_stream;
#pragma HLS STREAM variable=_p2_cim_stencil_stream depth=1
#pragma HLS RESOURCE variable=_p2_cim_stencil_stream core=FIFO_SRL

 // produce p2:cim.stencil.stream
 linebuffer<722, 482>(_p2_cim_stencil_update_stream, _p2_cim_stencil_stream);
 (void)0;
 // dispatch_stream(_p2_cim_stencil_stream, 2, 3, 1, 722, 3, 1, 482, 1, "p2:corners", 1, 0, 722, 0, 482);
 hls::stream<PackedStencil<float, 3, 3> > &_p2_cim_stencil_stream_to_p2_corners = _p2_cim_stencil_stream;
 (void)0;
 // consume p2:cim.stencil.stream
 hls::stream<PackedStencil<bool, 1, 1> > _p2_corners_stencil_stream;
#pragma HLS STREAM variable=_p2_corners_stencil_stream depth=1
#pragma HLS RESOURCE variable=_p2_corners_stencil_stream core=FIFO_SRL

 // produce p2:corners.stencil.stream
 for (int _p2_corners_scan_y = 0; _p2_corners_scan_y < 0 + 480; _p2_corners_scan_y++)
 {
  for (int _p2_corners_scan_x = 0; _p2_corners_scan_x < 0 + 720; _p2_corners_scan_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<float, 3, 3> _p2_cim_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_cim_stencil.value complete dim=0

   _p2_cim_stencil = _p2_cim_stencil_stream_to_p2_corners.read();
   (void)0;
   Stencil<bool, 1, 1> _p2_corners_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_corners_stencil.value complete dim=0

   float _538 = _p2_cim_stencil(1, 1);
   float _539 = _p2_cim_stencil(0, 0);
   float _540 = _p2_cim_stencil(1, 0);
   float _541 = max(_539, _540);
   float _542 = _p2_cim_stencil(2, 0);
   float _543 = max(_541, _542);
   float _544 = _p2_cim_stencil(0, 1);
   float _545 = max(_543, _544);
   float _546 = _p2_cim_stencil(2, 1);
   float _547 = max(_545, _546);
   float _548 = _p2_cim_stencil(0, 2);
   float _549 = max(_547, _548);
   float _550 = _p2_cim_stencil(1, 2);
   float _551 = max(_549, _550);
   float _552 = _p2_cim_stencil(2, 2);
   float _553 = max(_551, _552);
   bool _554 = _553 < _538;
   bool _555 = float_from_bits(1120403456 /* 100 */) <= _538;
   bool _556 = _554 && _555;
   _p2_corners_stencil(0, 0) = _556;
   _p2_corners_stencil_stream.write(_p2_corners_stencil);
   (void)0;
  } // for _p2_corners_scan_x
 } // for _p2_corners_scan_y
 // dispatch_stream(_p2_corners_stencil_stream, 2, 1, 1, 720, 1, 1, 480, 1, "__auto_insert__hw_output$2", 1, 0, 720, 0, 480);
 hls::stream<PackedStencil<bool, 1, 1> > &_p2_corners_stencil_stream_to___auto_insert__hw_output_2 = _p2_corners_stencil_stream;
 (void)0;
 // consume p2:corners.stencil.stream
 // produce __auto_insert__hw_output$2.stencil.stream
 for (int _hw_output_2_s0_y_yi = 0; _hw_output_2_s0_y_yi < 0 + 480; _hw_output_2_s0_y_yi++)
 {
  for (int _hw_output_2_s0_x_xi = 0; _hw_output_2_s0_x_xi < 0 + 720; _hw_output_2_s0_x_xi++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<bool, 1, 1> _p2_corners_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_corners_stencil.value complete dim=0

   _p2_corners_stencil = _p2_corners_stencil_stream_to___auto_insert__hw_output_2.read();
   (void)0;
   Stencil<uint8_t, 3, 3, 3> _downsample_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_downsample_2_stencil.value complete dim=0

   _downsample_2_stencil = _downsample_2_stencil_stream_to___auto_insert__hw_output_2.read();
   (void)0;
   Stencil<uint8_t, 3, 1, 1> __auto_insert__hw_output_2_stencil;
#pragma HLS ARRAY_PARTITION variable=__auto_insert__hw_output_2_stencil.value complete dim=0

   uint8_t _557 = (uint8_t)(0);
   uint8_t _558 = _downsample_2_stencil(0, 2, 2);
   bool _559 = _p2_corners_stencil(0, 0);
   uint8_t _560 = (uint8_t)(_559 ? _557 : _558);
   __auto_insert__hw_output_2_stencil(0, 0, 0) = _560;
   uint8_t _561 = (uint8_t)(255);
   uint8_t _562 = _downsample_2_stencil(1, 2, 2);
   bool _563 = _p2_corners_stencil(0, 0);
   uint8_t _564 = (uint8_t)(_563 ? _561 : _562);
   __auto_insert__hw_output_2_stencil(1, 0, 0) = _564;
   uint8_t _565 = (uint8_t)(0);
   uint8_t _566 = _downsample_2_stencil(2, 2, 2);
   bool _567 = _p2_corners_stencil(0, 0);
   uint8_t _568 = (uint8_t)(_567 ? _565 : _566);
   __auto_insert__hw_output_2_stencil(2, 0, 0) = _568;
   AxiPackedStencil<uint8_t, 3, 1, 1>__auto_insert__hw_output_2_stencil_packed = __auto_insert__hw_output_2_stencil;
   if (_hw_output_2_s0_x_xi == 719 && _hw_output_2_s0_y_yi == 479) {
    __auto_insert__hw_output_2_stencil_packed.last = 1;
   } else {
    __auto_insert__hw_output_2_stencil_packed.last = 0;
   }
   __auto_insert__hw_output_2_stencil_stream.write(__auto_insert__hw_output_2_stencil_packed);
   (void)0;
  } // for _hw_output_2_s0_x_xi
 } // for _hw_output_2_s0_y_yi
 // consume __auto_insert__hw_output$2.stencil.stream
} // kernel hls_target_hls_target


