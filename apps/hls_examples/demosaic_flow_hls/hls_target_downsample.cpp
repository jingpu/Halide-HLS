#include "hls_target_downsample.h"

#include "Linebuffer.h"
#include "halide_math.h"
void hls_target(
hls::stream<AxiPackedStencil<uint8_t, 3, 1, 1> > &arg_0,
hls::stream<AxiPackedStencil<uint8_t, 2, 1> > &arg_1,
hls::stream<AxiPackedStencil<uint8_t, 2, 1> > &arg_2)
{
#pragma HLS DATAFLOW
#pragma HLS INLINE region
#pragma HLS INTERFACE s_axilite port=return bundle=config
#pragma HLS INTERFACE axis register port=arg_0
#pragma HLS INTERFACE axis register port=arg_1
#pragma HLS INTERFACE axis register port=arg_2

 // alias the arguments
 hls::stream<AxiPackedStencil<uint8_t, 3, 1, 1> > &__auto_insert__hw_output_2_stencil_stream = arg_0;
 hls::stream<AxiPackedStencil<uint8_t, 2, 1> > &in1_stream = arg_1;
 hls::stream<AxiPackedStencil<uint8_t, 2, 1> > &in2_stream = arg_2;
 hls::stream<PackedStencil<uint8_t, 2, 2> > _padded1_2_stencil_update_stream;
 hls::stream<PackedStencil<uint8_t, 2, 2> > _padded2_2_stencil_update_stream;


 // buffer the input stream in order to fix HLS compiler issue,
 // where the loop nest for packing 2x2 stencils cannot be
 // pipelined.
 hls::stream<PackedStencil<uint8_t, 2, 1> > in1_buffer_stream;
 for (int y = 0; y < 0 + 487*2; y++) {
     for (int x = 0; x < 0 + 727; x++) {
         PackedStencil<uint8_t, 2, 1> s = in1_stream.read();
         in1_buffer_stream.write(s);
     }
 }

 hls::stream<PackedStencil<uint8_t, 2, 1> > in2_buffer_stream;
 for (int y = 0; y < 0 + 970; y++) {
     for (int x = 0; x < 0 + 725; x++) {
         PackedStencil<uint8_t, 2, 1> s = in2_stream.read();
         in2_buffer_stream.write(s);
     }
 }


 uint8_t buffer1[1454];

 for (int y = 0; y < 0 + 487*2; y++) {
     for (int x = 0; x < 0 + 727; x++) {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
         Stencil<uint8_t, 2, 1> in1_stencil = in1_buffer_stream.read();

         if (y % 2 == 0) {
             for (int i = 0; i < 2; i++) {
                 buffer1[x*2 + i] = in1_stencil(i, 0);
             }
         } else {
             Stencil<uint8_t, 2, 2> out1_stencil;
             for (int i = 0; i < 2; i++) {
                 out1_stencil(i, 0) = buffer1[x*2 + i];
             }
             for (int i = 0; i < 2; i++) {
                 out1_stencil(i, 1) = in1_stencil(i, 0);
             }
             _padded1_2_stencil_update_stream.write(out1_stencil);
         }
     }
 }
 uint8_t buffer2[1450];
 for (int y = 0; y < 0 + 970; y++) {
     for (int x = 0; x < 0 + 725; x++) {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
         Stencil<uint8_t, 2, 1> in2_stencil = in2_buffer_stream.read();

         if (y % 2 == 0) {
             for (int i = 0; i < 2; i++) {
                 buffer2[x*2 + i] = in2_stencil(i, 0);
             }
         } else {
             Stencil<uint8_t, 2, 2> out2_stencil;
             for (int i = 0; i < 2; i++) {
                 out2_stencil(i, 0) = buffer2[x*2 + i];
             }
             for (int i = 0; i < 2; i++) {
                 out2_stencil(i, 1) = in2_stencil(i, 0);
             }
             _padded2_2_stencil_update_stream.write(out2_stencil);
         }
     }
 }

 hls::stream<PackedStencil<uint8_t, 4, 4> > _padded2_2_stencil_stream;
#pragma HLS STREAM variable=_padded2_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_padded2_2_stencil_stream core=FIFO_SRL

 // produce padded2$2.stencil.stream
 linebuffer<1450, 970>(_padded2_2_stencil_update_stream, _padded2_2_stencil_stream);
 (void)0;
 // dispatch_stream(_padded2_2_stencil_stream, 2, 4, 2, 1450, 4, 2, 970, 1, "gray$4", 1, 0, 1450, 0, 970);
 hls::stream<PackedStencil<uint8_t, 4, 4> > &_padded2_2_stencil_stream_to_gray_4 = _padded2_2_stencil_stream;
 (void)0;
 // consume padded2$2.stencil.stream
 hls::stream<PackedStencil<uint8_t, 1, 1> > _gray_4_stencil_stream;
#pragma HLS STREAM variable=_gray_4_stencil_stream depth=1
#pragma HLS RESOURCE variable=_gray_4_stencil_stream core=FIFO_SRL

 // produce gray$4.stencil.stream
 for (int _gray_4_scan_y = 0; _gray_4_scan_y < 0 + 484; _gray_4_scan_y++)
 {
  for (int _gray_4_scan_x = 0; _gray_4_scan_x < 0 + 724; _gray_4_scan_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<uint8_t, 4, 4> _padded2_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_padded2_2_stencil.value complete dim=0

   _padded2_2_stencil = _padded2_2_stencil_stream_to_gray_4.read();
   (void)0;
   Stencil<uint8_t, 1, 1> _gray_4_stencil;
#pragma HLS ARRAY_PARTITION variable=_gray_4_stencil.value complete dim=0

   uint8_t _192 = _padded2_2_stencil(2, 0);
   uint8_t _193 = _padded2_2_stencil(0, 2);
   uint8_t _194 = _padded2_2_stencil(2, 2);
   uint8_t _195 = _padded2_2_stencil(2, 1);
   uint8_t _196 = _padded2_2_stencil(1, 2);
   uint8_t _197 = _padded2_2_stencil(1, 1);
   uint16_t _198 = (uint16_t)(_197);
   uint8_t _199 = _padded2_2_stencil(3, 1);
   uint16_t _200 = (uint16_t)(_199);
   uint16_t _201 = _198 + _200;
   uint8_t _202 = _padded2_2_stencil(1, 3);
   uint8_t _203 = _padded2_2_stencil(0, 0);
   uint16_t _204 = (uint16_t)(_203);
   uint16_t _205 = (uint16_t)(_192);
   uint16_t _206 = _204 + _205;
   uint16_t _207 = (uint16_t)(_193);
   uint16_t _208 = _206 + _207;
   uint16_t _209 = (uint16_t)(_194);
   uint16_t _210 = _208 + _209;
   uint16_t _211 = _210 >> 2;
   uint8_t _212 = (uint8_t)(_211);
   uint16_t _213 = (uint16_t)(_212);
   uint16_t _214 = _205 + _209;
   uint16_t _215 = _214 >> 1;
   uint8_t _216 = (uint8_t)(_215);
   uint16_t _217 = (uint16_t)(_216);
   uint16_t _218 = _213 + _217;
   uint16_t _219 = _218 >> 1;
   uint8_t _220 = (uint8_t)(_219);
   uint16_t _221 = (uint16_t)(_220);
   uint16_t _222 = _207 + _209;
   uint16_t _223 = _222 >> 1;
   uint8_t _224 = (uint8_t)(_223);
   uint16_t _225 = (uint16_t)(_224);
   uint16_t _226 = _225 + _209;
   uint16_t _227 = _226 >> 1;
   uint8_t _228 = (uint8_t)(_227);
   uint16_t _229 = (uint16_t)(_228);
   uint16_t _230 = _221 + _229;
   uint16_t _231 = _230 >> 1;
   uint8_t _232 = (uint8_t)(_231);
   uint16_t _233 = (uint16_t)(_232);
   uint16_t _234 = (uint16_t)(77);
   uint16_t _235 = _233 * _234;
   uint8_t _236 = _padded2_2_stencil(0, 1);
   uint16_t _237 = (uint16_t)(_236);
   uint16_t _238 = (uint16_t)(_195);
   uint16_t _239 = _237 + _238;
   uint8_t _240 = _padded2_2_stencil(1, 0);
   uint16_t _241 = (uint16_t)(_240);
   uint16_t _242 = _239 + _241;
   uint16_t _243 = (uint16_t)(_196);
   uint16_t _244 = _242 + _243;
   uint16_t _245 = _244 >> 2;
   uint8_t _246 = (uint8_t)(_245);
   uint16_t _247 = (uint16_t)(_246);
   uint16_t _248 = _247 + _238;
   uint16_t _249 = _248 >> 1;
   uint8_t _250 = (uint8_t)(_249);
   uint16_t _251 = (uint16_t)(_250);
   uint8_t _252 = _padded2_2_stencil(3, 2);
   uint16_t _253 = (uint16_t)(_252);
   uint16_t _254 = _243 + _253;
   uint16_t _255 = _254 + _238;
   uint8_t _256 = _padded2_2_stencil(2, 3);
   uint16_t _257 = (uint16_t)(_256);
   uint16_t _258 = _255 + _257;
   uint16_t _259 = _258 >> 2;
   uint8_t _260 = (uint8_t)(_259);
   uint16_t _261 = (uint16_t)(_260);
   uint16_t _262 = _243 + _261;
   uint16_t _263 = _262 >> 1;
   uint8_t _264 = (uint8_t)(_263);
   uint16_t _265 = (uint16_t)(_264);
   uint16_t _266 = _251 + _265;
   uint16_t _267 = _266 >> 1;
   uint8_t _268 = (uint8_t)(_267);
   uint16_t _269 = (uint16_t)(_268);
   uint16_t _270 = (uint16_t)(150);
   uint16_t _271 = _269 * _270;
   uint16_t _272 = _235 + _271;
   uint16_t _273 = _201 >> 1;
   uint8_t _274 = (uint8_t)(_273);
   uint16_t _275 = (uint16_t)(_274);
   uint16_t _276 = _198 + _275;
   uint16_t _277 = _276 >> 1;
   uint8_t _278 = (uint8_t)(_277);
   uint16_t _279 = (uint16_t)(_278);
   uint16_t _280 = (uint16_t)(_202);
   uint16_t _281 = _198 + _280;
   uint16_t _282 = _281 >> 1;
   uint8_t _283 = (uint8_t)(_282);
   uint16_t _284 = (uint16_t)(_283);
   uint16_t _285 = _201 + _280;
   uint8_t _286 = _padded2_2_stencil(3, 3);
   uint16_t _287 = (uint16_t)(_286);
   uint16_t _288 = _285 + _287;
   uint16_t _289 = _288 >> 2;
   uint8_t _290 = (uint8_t)(_289);
   uint16_t _291 = (uint16_t)(_290);
   uint16_t _292 = _284 + _291;
   uint16_t _293 = _292 >> 1;
   uint8_t _294 = (uint8_t)(_293);
   uint16_t _295 = (uint16_t)(_294);
   uint16_t _296 = _279 + _295;
   uint16_t _297 = _296 >> 1;
   uint8_t _298 = (uint8_t)(_297);
   uint16_t _299 = (uint16_t)(_298);
   uint16_t _300 = (uint16_t)(29);
   uint16_t _301 = _299 * _300;
   uint16_t _302 = _272 + _301;
   uint16_t _303 = _302 >> 8;
   uint8_t _304 = (uint8_t)(_303);
   _gray_4_stencil(0, 0) = _304;
   _gray_4_stencil_stream.write(_gray_4_stencil);
   (void)0;
  } // for _gray_4_scan_x
 } // for _gray_4_scan_y
 // dispatch_stream(_gray_4_stencil_stream, 2, 1, 1, 724, 1, 1, 484, 2, "It$2", 1, 0, 724, 0, 484, "__auto_insert__hw_output$2", 3600, 2, 720, 2, 480);
 hls::stream<PackedStencil<uint8_t, 1, 1> > _gray_4_stencil_stream_to_It_2;
#pragma HLS STREAM variable=_gray_4_stencil_stream_to_It_2 depth=1
#pragma HLS RESOURCE variable=_gray_4_stencil_stream_to_It_2 core=FIFO_SRL

 hls::stream<PackedStencil<uint8_t, 1, 1> > _gray_4_stencil_stream_to___auto_insert__hw_output_2;
#pragma HLS STREAM variable=_gray_4_stencil_stream_to___auto_insert__hw_output_2 depth=3600
 for (int _dim_1 = 0; _dim_1 <= 483; _dim_1 += 1)
 for (int _dim_0 = 0; _dim_0 <= 723; _dim_0 += 1)
 {
#pragma HLS PIPELINE
  Stencil<uint8_t, 1, 1> _tmp_stencil = _gray_4_stencil_stream.read();
#pragma HLS ARRAY_PARTITION variable=_tmp_stencil.value complete dim=0

  if (_dim_0 >= 0 && _dim_0 <= 723 && _dim_1 >= 0 && _dim_1 <= 483)
  {
   _gray_4_stencil_stream_to_It_2.write(_tmp_stencil);
  }
  if (_dim_0 >= 2 && _dim_0 <= 721 && _dim_1 >= 2 && _dim_1 <= 481)
  {
   _gray_4_stencil_stream_to___auto_insert__hw_output_2.write(_tmp_stencil);
  }
 }
 (void)0;
 // consume gray$4.stencil.stream
 hls::stream<PackedStencil<uint8_t, 4, 4> > _padded1_2_stencil_stream;
#pragma HLS STREAM variable=_padded1_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_padded1_2_stencil_stream core=FIFO_SRL

 // produce padded1$2.stencil.stream
 linebuffer<1454, 974>(_padded1_2_stencil_update_stream, _padded1_2_stencil_stream);
 (void)0;
 // dispatch_stream(_padded1_2_stencil_stream, 2, 4, 2, 1454, 4, 2, 974, 1, "gray$3", 1, 0, 1454, 0, 974);
 hls::stream<PackedStencil<uint8_t, 4, 4> > &_padded1_2_stencil_stream_to_gray_3 = _padded1_2_stencil_stream;
 (void)0;
 // consume padded1$2.stencil.stream
 hls::stream<PackedStencil<uint8_t, 1, 1> > _gray_3_stencil_update_stream;
#pragma HLS STREAM variable=_gray_3_stencil_update_stream depth=1
#pragma HLS RESOURCE variable=_gray_3_stencil_update_stream core=FIFO_SRL

 // produce gray$3.stencil_update.stream
 for (int _gray_3_scan_update_y = 0; _gray_3_scan_update_y < 0 + 486; _gray_3_scan_update_y++)
 {
  for (int _gray_3_scan_update_x = 0; _gray_3_scan_update_x < 0 + 726; _gray_3_scan_update_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<uint8_t, 4, 4> _padded1_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_padded1_2_stencil.value complete dim=0

   _padded1_2_stencil = _padded1_2_stencil_stream_to_gray_3.read();
   (void)0;
   Stencil<uint8_t, 1, 1> _gray_3_stencil;
#pragma HLS ARRAY_PARTITION variable=_gray_3_stencil.value complete dim=0

   uint8_t _305 = _padded1_2_stencil(2, 0);
   uint8_t _306 = _padded1_2_stencil(0, 2);
   uint8_t _307 = _padded1_2_stencil(2, 2);
   uint8_t _308 = _padded1_2_stencil(2, 1);
   uint8_t _309 = _padded1_2_stencil(1, 2);
   uint8_t _310 = _padded1_2_stencil(1, 1);
   uint16_t _311 = (uint16_t)(_310);
   uint8_t _312 = _padded1_2_stencil(3, 1);
   uint16_t _313 = (uint16_t)(_312);
   uint16_t _314 = _311 + _313;
   uint8_t _315 = _padded1_2_stencil(1, 3);
   uint8_t _316 = _padded1_2_stencil(0, 0);
   uint16_t _317 = (uint16_t)(_316);
   uint16_t _318 = (uint16_t)(_305);
   uint16_t _319 = _317 + _318;
   uint16_t _320 = (uint16_t)(_306);
   uint16_t _321 = _319 + _320;
   uint16_t _322 = (uint16_t)(_307);
   uint16_t _323 = _321 + _322;
   uint16_t _324 = _323 >> 2;
   uint8_t _325 = (uint8_t)(_324);
   uint16_t _326 = (uint16_t)(_325);
   uint16_t _327 = _318 + _322;
   uint16_t _328 = _327 >> 1;
   uint8_t _329 = (uint8_t)(_328);
   uint16_t _330 = (uint16_t)(_329);
   uint16_t _331 = _326 + _330;
   uint16_t _332 = _331 >> 1;
   uint8_t _333 = (uint8_t)(_332);
   uint16_t _334 = (uint16_t)(_333);
   uint16_t _335 = _320 + _322;
   uint16_t _336 = _335 >> 1;
   uint8_t _337 = (uint8_t)(_336);
   uint16_t _338 = (uint16_t)(_337);
   uint16_t _339 = _338 + _322;
   uint16_t _340 = _339 >> 1;
   uint8_t _341 = (uint8_t)(_340);
   uint16_t _342 = (uint16_t)(_341);
   uint16_t _343 = _334 + _342;
   uint16_t _344 = _343 >> 1;
   uint8_t _345 = (uint8_t)(_344);
   uint16_t _346 = (uint16_t)(_345);
   uint16_t _347 = (uint16_t)(77);
   uint16_t _348 = _346 * _347;
   uint8_t _349 = _padded1_2_stencil(0, 1);
   uint16_t _350 = (uint16_t)(_349);
   uint16_t _351 = (uint16_t)(_308);
   uint16_t _352 = _350 + _351;
   uint8_t _353 = _padded1_2_stencil(1, 0);
   uint16_t _354 = (uint16_t)(_353);
   uint16_t _355 = _352 + _354;
   uint16_t _356 = (uint16_t)(_309);
   uint16_t _357 = _355 + _356;
   uint16_t _358 = _357 >> 2;
   uint8_t _359 = (uint8_t)(_358);
   uint16_t _360 = (uint16_t)(_359);
   uint16_t _361 = _360 + _351;
   uint16_t _362 = _361 >> 1;
   uint8_t _363 = (uint8_t)(_362);
   uint16_t _364 = (uint16_t)(_363);
   uint8_t _365 = _padded1_2_stencil(3, 2);
   uint16_t _366 = (uint16_t)(_365);
   uint16_t _367 = _356 + _366;
   uint16_t _368 = _367 + _351;
   uint8_t _369 = _padded1_2_stencil(2, 3);
   uint16_t _370 = (uint16_t)(_369);
   uint16_t _371 = _368 + _370;
   uint16_t _372 = _371 >> 2;
   uint8_t _373 = (uint8_t)(_372);
   uint16_t _374 = (uint16_t)(_373);
   uint16_t _375 = _356 + _374;
   uint16_t _376 = _375 >> 1;
   uint8_t _377 = (uint8_t)(_376);
   uint16_t _378 = (uint16_t)(_377);
   uint16_t _379 = _364 + _378;
   uint16_t _380 = _379 >> 1;
   uint8_t _381 = (uint8_t)(_380);
   uint16_t _382 = (uint16_t)(_381);
   uint16_t _383 = (uint16_t)(150);
   uint16_t _384 = _382 * _383;
   uint16_t _385 = _348 + _384;
   uint16_t _386 = _314 >> 1;
   uint8_t _387 = (uint8_t)(_386);
   uint16_t _388 = (uint16_t)(_387);
   uint16_t _389 = _311 + _388;
   uint16_t _390 = _389 >> 1;
   uint8_t _391 = (uint8_t)(_390);
   uint16_t _392 = (uint16_t)(_391);
   uint16_t _393 = (uint16_t)(_315);
   uint16_t _394 = _311 + _393;
   uint16_t _395 = _394 >> 1;
   uint8_t _396 = (uint8_t)(_395);
   uint16_t _397 = (uint16_t)(_396);
   uint16_t _398 = _314 + _393;
   uint8_t _399 = _padded1_2_stencil(3, 3);
   uint16_t _400 = (uint16_t)(_399);
   uint16_t _401 = _398 + _400;
   uint16_t _402 = _401 >> 2;
   uint8_t _403 = (uint8_t)(_402);
   uint16_t _404 = (uint16_t)(_403);
   uint16_t _405 = _397 + _404;
   uint16_t _406 = _405 >> 1;
   uint8_t _407 = (uint8_t)(_406);
   uint16_t _408 = (uint16_t)(_407);
   uint16_t _409 = _392 + _408;
   uint16_t _410 = _409 >> 1;
   uint8_t _411 = (uint8_t)(_410);
   uint16_t _412 = (uint16_t)(_411);
   uint16_t _413 = (uint16_t)(29);
   uint16_t _414 = _412 * _413;
   uint16_t _415 = _385 + _414;
   uint16_t _416 = _415 >> 8;
   uint8_t _417 = (uint8_t)(_416);
   _gray_3_stencil(0, 0) = _417;
   _gray_3_stencil_update_stream.write(_gray_3_stencil);
   (void)0;
  } // for _gray_3_scan_update_x
 } // for _gray_3_scan_update_y
 // consume gray$3.stencil_update.stream
 hls::stream<PackedStencil<uint8_t, 3, 3> > _gray_3_stencil_stream;
#pragma HLS STREAM variable=_gray_3_stencil_stream depth=1
#pragma HLS RESOURCE variable=_gray_3_stencil_stream core=FIFO_SRL

 // produce gray$3.stencil.stream
 linebuffer<726, 486>(_gray_3_stencil_update_stream, _gray_3_stencil_stream);
 (void)0;
 // dispatch_stream(_gray_3_stencil_stream, 2, 3, 1, 726, 3, 1, 486, 3, "It$2", 1, 0, 726, 0, 486, "Ix$2", 1, 0, 726, 0, 486, "Iy$2", 1, 0, 726, 0, 486);
 hls::stream<PackedStencil<uint8_t, 3, 3> > _gray_3_stencil_stream_to_It_2;
#pragma HLS STREAM variable=_gray_3_stencil_stream_to_It_2 depth=1
#pragma HLS RESOURCE variable=_gray_3_stencil_stream_to_It_2 core=FIFO_SRL

 hls::stream<PackedStencil<uint8_t, 3, 3> > _gray_3_stencil_stream_to_Ix_2;
#pragma HLS STREAM variable=_gray_3_stencil_stream_to_Ix_2 depth=1
#pragma HLS RESOURCE variable=_gray_3_stencil_stream_to_Ix_2 core=FIFO_SRL

 hls::stream<PackedStencil<uint8_t, 3, 3> > _gray_3_stencil_stream_to_Iy_2;
#pragma HLS STREAM variable=_gray_3_stencil_stream_to_Iy_2 depth=1
#pragma HLS RESOURCE variable=_gray_3_stencil_stream_to_Iy_2 core=FIFO_SRL

 for (int _dim_1 = 0; _dim_1 <= 483; _dim_1 += 1)
 for (int _dim_0 = 0; _dim_0 <= 723; _dim_0 += 1)
 {
#pragma HLS PIPELINE
  Stencil<uint8_t, 3, 3> _tmp_stencil = _gray_3_stencil_stream.read();
#pragma HLS ARRAY_PARTITION variable=_tmp_stencil.value complete dim=0

  if (_dim_0 >= 0 && _dim_0 <= 723 && _dim_1 >= 0 && _dim_1 <= 483)
  {
   _gray_3_stencil_stream_to_It_2.write(_tmp_stencil);
  }
  if (_dim_0 >= 0 && _dim_0 <= 723 && _dim_1 >= 0 && _dim_1 <= 483)
  {
   _gray_3_stencil_stream_to_Ix_2.write(_tmp_stencil);
  }
  if (_dim_0 >= 0 && _dim_0 <= 723 && _dim_1 >= 0 && _dim_1 <= 483)
  {
   _gray_3_stencil_stream_to_Iy_2.write(_tmp_stencil);
  }
 }
 (void)0;
 // consume gray$3.stencil.stream
 hls::stream<PackedStencil<int16_t, 1, 1> > _Ix_2_stencil_update_stream;
#pragma HLS STREAM variable=_Ix_2_stencil_update_stream depth=1
#pragma HLS RESOURCE variable=_Ix_2_stencil_update_stream core=FIFO_SRL

 // produce Ix$2.stencil_update.stream
 for (int _Ix_2_scan_update_y = 0; _Ix_2_scan_update_y < 0 + 484; _Ix_2_scan_update_y++)
 {
  for (int _Ix_2_scan_update_x = 0; _Ix_2_scan_update_x < 0 + 724; _Ix_2_scan_update_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<uint8_t, 3, 3> _gray_3_stencil;
#pragma HLS ARRAY_PARTITION variable=_gray_3_stencil.value complete dim=0

   _gray_3_stencil = _gray_3_stencil_stream_to_Ix_2.read();
   (void)0;
   Stencil<int16_t, 1, 1> _Ix_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_Ix_2_stencil.value complete dim=0

   uint8_t _418 = _gray_3_stencil(2, 1);
   int16_t _419 = (int16_t)(_418);
   uint8_t _420 = _gray_3_stencil(0, 1);
   int16_t _421 = (int16_t)(_420);
   int16_t _422 = _419 - _421;
   _Ix_2_stencil(0, 0) = _422;
   _Ix_2_stencil_update_stream.write(_Ix_2_stencil);
   (void)0;
  } // for _Ix_2_scan_update_x
 } // for _Ix_2_scan_update_y
 // consume Ix$2.stencil_update.stream
 hls::stream<PackedStencil<int16_t, 5, 5> > _Ix_2_stencil_stream;
#pragma HLS STREAM variable=_Ix_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_Ix_2_stencil_stream core=FIFO_SRL

 // produce Ix$2.stencil.stream
 linebuffer<724, 484>(_Ix_2_stencil_update_stream, _Ix_2_stencil_stream);
 (void)0;
 // dispatch_stream(_Ix_2_stencil_stream, 2, 5, 1, 724, 5, 1, 484, 4, "A00$2", 1, 0, 724, 0, 484, "A01$2", 1, 0, 724, 0, 484, "A10$2", 1, 0, 724, 0, 484, "p2:b0", 1, 0, 724, 0, 484);
 hls::stream<PackedStencil<int16_t, 5, 5> > _Ix_2_stencil_stream_to_A00_2;
#pragma HLS STREAM variable=_Ix_2_stencil_stream_to_A00_2 depth=1
#pragma HLS RESOURCE variable=_Ix_2_stencil_stream_to_A00_2 core=FIFO_SRL

 hls::stream<PackedStencil<int16_t, 5, 5> > _Ix_2_stencil_stream_to_A01_2;
#pragma HLS STREAM variable=_Ix_2_stencil_stream_to_A01_2 depth=1
#pragma HLS RESOURCE variable=_Ix_2_stencil_stream_to_A01_2 core=FIFO_SRL

 hls::stream<PackedStencil<int16_t, 5, 5> > _Ix_2_stencil_stream_to_A10_2;
#pragma HLS STREAM variable=_Ix_2_stencil_stream_to_A10_2 depth=1
#pragma HLS RESOURCE variable=_Ix_2_stencil_stream_to_A10_2 core=FIFO_SRL

 hls::stream<PackedStencil<int16_t, 5, 5> > _Ix_2_stencil_stream_to_p2_b0;
#pragma HLS STREAM variable=_Ix_2_stencil_stream_to_p2_b0 depth=1
#pragma HLS RESOURCE variable=_Ix_2_stencil_stream_to_p2_b0 core=FIFO_SRL

 for (int _dim_1 = 0; _dim_1 <= 479; _dim_1 += 1)
 for (int _dim_0 = 0; _dim_0 <= 719; _dim_0 += 1)
 {
#pragma HLS PIPELINE
  Stencil<int16_t, 5, 5> _tmp_stencil = _Ix_2_stencil_stream.read();
#pragma HLS ARRAY_PARTITION variable=_tmp_stencil.value complete dim=0

  if (_dim_0 >= 0 && _dim_0 <= 719 && _dim_1 >= 0 && _dim_1 <= 479)
  {
   _Ix_2_stencil_stream_to_A00_2.write(_tmp_stencil);
  }
  if (_dim_0 >= 0 && _dim_0 <= 719 && _dim_1 >= 0 && _dim_1 <= 479)
  {
   _Ix_2_stencil_stream_to_A01_2.write(_tmp_stencil);
  }
  if (_dim_0 >= 0 && _dim_0 <= 719 && _dim_1 >= 0 && _dim_1 <= 479)
  {
   _Ix_2_stencil_stream_to_A10_2.write(_tmp_stencil);
  }
  if (_dim_0 >= 0 && _dim_0 <= 719 && _dim_1 >= 0 && _dim_1 <= 479)
  {
   _Ix_2_stencil_stream_to_p2_b0.write(_tmp_stencil);
  }
 }
 (void)0;
 // consume Ix$2.stencil.stream
 hls::stream<PackedStencil<int32_t, 1, 1> > _A00_2_stencil_stream;
#pragma HLS STREAM variable=_A00_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_A00_2_stencil_stream core=FIFO_SRL

 // produce A00$2.stencil.stream
 for (int _A00_2_scan_y = 0; _A00_2_scan_y < 0 + 480; _A00_2_scan_y++)
 {
  for (int _A00_2_scan_x = 0; _A00_2_scan_x < 0 + 720; _A00_2_scan_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<int16_t, 5, 5> _Ix_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_Ix_2_stencil.value complete dim=0

   _Ix_2_stencil = _Ix_2_stencil_stream_to_A00_2.read();
   (void)0;
   Stencil<int32_t, 1, 1> _A00_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_A00_2_stencil.value complete dim=0

   _A00_2_stencil(0, 0) = 0;
   int16_t _423 = _Ix_2_stencil(0, 0);
   int32_t _424 = _A00_2_stencil(0, 0);
   int32_t _425 = (int32_t)(_423);
   int32_t _426 = _425 * _425;
   int32_t _427 = _424 + _426;
   _A00_2_stencil(0, 0) = _427;
   int16_t _428 = _Ix_2_stencil(1, 0);
   int32_t _429 = _A00_2_stencil(0, 0);
   int32_t _430 = (int32_t)(_428);
   int32_t _431 = _430 * _430;
   int32_t _432 = _429 + _431;
   _A00_2_stencil(0, 0) = _432;
   int16_t _433 = _Ix_2_stencil(2, 0);
   int32_t _434 = _A00_2_stencil(0, 0);
   int32_t _435 = (int32_t)(_433);
   int32_t _436 = _435 * _435;
   int32_t _437 = _434 + _436;
   _A00_2_stencil(0, 0) = _437;
   int16_t _438 = _Ix_2_stencil(3, 0);
   int32_t _439 = _A00_2_stencil(0, 0);
   int32_t _440 = (int32_t)(_438);
   int32_t _441 = _440 * _440;
   int32_t _442 = _439 + _441;
   _A00_2_stencil(0, 0) = _442;
   int16_t _443 = _Ix_2_stencil(4, 0);
   int32_t _444 = _A00_2_stencil(0, 0);
   int32_t _445 = (int32_t)(_443);
   int32_t _446 = _445 * _445;
   int32_t _447 = _444 + _446;
   _A00_2_stencil(0, 0) = _447;
   int16_t _448 = _Ix_2_stencil(0, 1);
   int32_t _449 = _A00_2_stencil(0, 0);
   int32_t _450 = (int32_t)(_448);
   int32_t _451 = _450 * _450;
   int32_t _452 = _449 + _451;
   _A00_2_stencil(0, 0) = _452;
   int16_t _453 = _Ix_2_stencil(1, 1);
   int32_t _454 = _A00_2_stencil(0, 0);
   int32_t _455 = (int32_t)(_453);
   int32_t _456 = _455 * _455;
   int32_t _457 = _454 + _456;
   _A00_2_stencil(0, 0) = _457;
   int16_t _458 = _Ix_2_stencil(2, 1);
   int32_t _459 = _A00_2_stencil(0, 0);
   int32_t _460 = (int32_t)(_458);
   int32_t _461 = _460 * _460;
   int32_t _462 = _459 + _461;
   _A00_2_stencil(0, 0) = _462;
   int16_t _463 = _Ix_2_stencil(3, 1);
   int32_t _464 = _A00_2_stencil(0, 0);
   int32_t _465 = (int32_t)(_463);
   int32_t _466 = _465 * _465;
   int32_t _467 = _464 + _466;
   _A00_2_stencil(0, 0) = _467;
   int16_t _468 = _Ix_2_stencil(4, 1);
   int32_t _469 = _A00_2_stencil(0, 0);
   int32_t _470 = (int32_t)(_468);
   int32_t _471 = _470 * _470;
   int32_t _472 = _469 + _471;
   _A00_2_stencil(0, 0) = _472;
   int16_t _473 = _Ix_2_stencil(0, 2);
   int32_t _474 = _A00_2_stencil(0, 0);
   int32_t _475 = (int32_t)(_473);
   int32_t _476 = _475 * _475;
   int32_t _477 = _474 + _476;
   _A00_2_stencil(0, 0) = _477;
   int16_t _478 = _Ix_2_stencil(1, 2);
   int32_t _479 = _A00_2_stencil(0, 0);
   int32_t _480 = (int32_t)(_478);
   int32_t _481 = _480 * _480;
   int32_t _482 = _479 + _481;
   _A00_2_stencil(0, 0) = _482;
   int16_t _483 = _Ix_2_stencil(2, 2);
   int32_t _484 = _A00_2_stencil(0, 0);
   int32_t _485 = (int32_t)(_483);
   int32_t _486 = _485 * _485;
   int32_t _487 = _484 + _486;
   _A00_2_stencil(0, 0) = _487;
   int16_t _488 = _Ix_2_stencil(3, 2);
   int32_t _489 = _A00_2_stencil(0, 0);
   int32_t _490 = (int32_t)(_488);
   int32_t _491 = _490 * _490;
   int32_t _492 = _489 + _491;
   _A00_2_stencil(0, 0) = _492;
   int16_t _493 = _Ix_2_stencil(4, 2);
   int32_t _494 = _A00_2_stencil(0, 0);
   int32_t _495 = (int32_t)(_493);
   int32_t _496 = _495 * _495;
   int32_t _497 = _494 + _496;
   _A00_2_stencil(0, 0) = _497;
   int16_t _498 = _Ix_2_stencil(0, 3);
   int32_t _499 = _A00_2_stencil(0, 0);
   int32_t _500 = (int32_t)(_498);
   int32_t _501 = _500 * _500;
   int32_t _502 = _499 + _501;
   _A00_2_stencil(0, 0) = _502;
   int16_t _503 = _Ix_2_stencil(1, 3);
   int32_t _504 = _A00_2_stencil(0, 0);
   int32_t _505 = (int32_t)(_503);
   int32_t _506 = _505 * _505;
   int32_t _507 = _504 + _506;
   _A00_2_stencil(0, 0) = _507;
   int16_t _508 = _Ix_2_stencil(2, 3);
   int32_t _509 = _A00_2_stencil(0, 0);
   int32_t _510 = (int32_t)(_508);
   int32_t _511 = _510 * _510;
   int32_t _512 = _509 + _511;
   _A00_2_stencil(0, 0) = _512;
   int16_t _513 = _Ix_2_stencil(3, 3);
   int32_t _514 = _A00_2_stencil(0, 0);
   int32_t _515 = (int32_t)(_513);
   int32_t _516 = _515 * _515;
   int32_t _517 = _514 + _516;
   _A00_2_stencil(0, 0) = _517;
   int16_t _518 = _Ix_2_stencil(4, 3);
   int32_t _519 = _A00_2_stencil(0, 0);
   int32_t _520 = (int32_t)(_518);
   int32_t _521 = _520 * _520;
   int32_t _522 = _519 + _521;
   _A00_2_stencil(0, 0) = _522;
   int16_t _523 = _Ix_2_stencil(0, 4);
   int32_t _524 = _A00_2_stencil(0, 0);
   int32_t _525 = (int32_t)(_523);
   int32_t _526 = _525 * _525;
   int32_t _527 = _524 + _526;
   _A00_2_stencil(0, 0) = _527;
   int16_t _528 = _Ix_2_stencil(1, 4);
   int32_t _529 = _A00_2_stencil(0, 0);
   int32_t _530 = (int32_t)(_528);
   int32_t _531 = _530 * _530;
   int32_t _532 = _529 + _531;
   _A00_2_stencil(0, 0) = _532;
   int16_t _533 = _Ix_2_stencil(2, 4);
   int32_t _534 = _A00_2_stencil(0, 0);
   int32_t _535 = (int32_t)(_533);
   int32_t _536 = _535 * _535;
   int32_t _537 = _534 + _536;
   _A00_2_stencil(0, 0) = _537;
   int16_t _538 = _Ix_2_stencil(3, 4);
   int32_t _539 = _A00_2_stencil(0, 0);
   int32_t _540 = (int32_t)(_538);
   int32_t _541 = _540 * _540;
   int32_t _542 = _539 + _541;
   _A00_2_stencil(0, 0) = _542;
   int16_t _543 = _Ix_2_stencil(4, 4);
   int32_t _544 = _A00_2_stencil(0, 0);
   int32_t _545 = (int32_t)(_543);
   int32_t _546 = _545 * _545;
   int32_t _547 = _544 + _546;
   _A00_2_stencil(0, 0) = _547;
   _A00_2_stencil_stream.write(_A00_2_stencil);
   (void)0;
  } // for _A00_2_scan_x
 } // for _A00_2_scan_y
 // dispatch_stream(_A00_2_stencil_stream, 2, 1, 1, 720, 1, 1, 480, 1, "__auto_insert__hw_output$2", 1, 0, 720, 0, 480);
 hls::stream<PackedStencil<int32_t, 1, 1> > &_A00_2_stencil_stream_to___auto_insert__hw_output_2 = _A00_2_stencil_stream;
 (void)0;
 // consume A00$2.stencil.stream
 hls::stream<PackedStencil<int16_t, 1, 1> > _Iy_2_stencil_update_stream;
#pragma HLS STREAM variable=_Iy_2_stencil_update_stream depth=1
#pragma HLS RESOURCE variable=_Iy_2_stencil_update_stream core=FIFO_SRL

 // produce Iy$2.stencil_update.stream
 for (int _Iy_2_scan_update_y = 0; _Iy_2_scan_update_y < 0 + 484; _Iy_2_scan_update_y++)
 {
  for (int _Iy_2_scan_update_x = 0; _Iy_2_scan_update_x < 0 + 724; _Iy_2_scan_update_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<uint8_t, 3, 3> _gray_3_stencil;
#pragma HLS ARRAY_PARTITION variable=_gray_3_stencil.value complete dim=0

   _gray_3_stencil = _gray_3_stencil_stream_to_Iy_2.read();
   (void)0;
   Stencil<int16_t, 1, 1> _Iy_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_Iy_2_stencil.value complete dim=0

   uint8_t _548 = _gray_3_stencil(1, 2);
   int16_t _549 = (int16_t)(_548);
   uint8_t _550 = _gray_3_stencil(1, 0);
   int16_t _551 = (int16_t)(_550);
   int16_t _552 = _549 - _551;
   _Iy_2_stencil(0, 0) = _552;
   _Iy_2_stencil_update_stream.write(_Iy_2_stencil);
   (void)0;
  } // for _Iy_2_scan_update_x
 } // for _Iy_2_scan_update_y
 // consume Iy$2.stencil_update.stream
 hls::stream<PackedStencil<int16_t, 5, 5> > _Iy_2_stencil_stream;
#pragma HLS STREAM variable=_Iy_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_Iy_2_stencil_stream core=FIFO_SRL

 // produce Iy$2.stencil.stream
 linebuffer<724, 484>(_Iy_2_stencil_update_stream, _Iy_2_stencil_stream);
 (void)0;
 // dispatch_stream(_Iy_2_stencil_stream, 2, 5, 1, 724, 5, 1, 484, 4, "A01$2", 1, 0, 724, 0, 484, "A10$2", 1, 0, 724, 0, 484, "A11$2", 1, 0, 724, 0, 484, "p2:b1", 1, 0, 724, 0, 484);
 hls::stream<PackedStencil<int16_t, 5, 5> > _Iy_2_stencil_stream_to_A01_2;
#pragma HLS STREAM variable=_Iy_2_stencil_stream_to_A01_2 depth=1
#pragma HLS RESOURCE variable=_Iy_2_stencil_stream_to_A01_2 core=FIFO_SRL

 hls::stream<PackedStencil<int16_t, 5, 5> > _Iy_2_stencil_stream_to_A10_2;
#pragma HLS STREAM variable=_Iy_2_stencil_stream_to_A10_2 depth=1
#pragma HLS RESOURCE variable=_Iy_2_stencil_stream_to_A10_2 core=FIFO_SRL

 hls::stream<PackedStencil<int16_t, 5, 5> > _Iy_2_stencil_stream_to_A11_2;
#pragma HLS STREAM variable=_Iy_2_stencil_stream_to_A11_2 depth=1
#pragma HLS RESOURCE variable=_Iy_2_stencil_stream_to_A11_2 core=FIFO_SRL

 hls::stream<PackedStencil<int16_t, 5, 5> > _Iy_2_stencil_stream_to_p2_b1;
#pragma HLS STREAM variable=_Iy_2_stencil_stream_to_p2_b1 depth=1
#pragma HLS RESOURCE variable=_Iy_2_stencil_stream_to_p2_b1 core=FIFO_SRL

 for (int _dim_1 = 0; _dim_1 <= 479; _dim_1 += 1)
 for (int _dim_0 = 0; _dim_0 <= 719; _dim_0 += 1)
 {
#pragma HLS PIPELINE
  Stencil<int16_t, 5, 5> _tmp_stencil = _Iy_2_stencil_stream.read();
#pragma HLS ARRAY_PARTITION variable=_tmp_stencil.value complete dim=0

  if (_dim_0 >= 0 && _dim_0 <= 719 && _dim_1 >= 0 && _dim_1 <= 479)
  {
   _Iy_2_stencil_stream_to_A01_2.write(_tmp_stencil);
  }
  if (_dim_0 >= 0 && _dim_0 <= 719 && _dim_1 >= 0 && _dim_1 <= 479)
  {
   _Iy_2_stencil_stream_to_A10_2.write(_tmp_stencil);
  }
  if (_dim_0 >= 0 && _dim_0 <= 719 && _dim_1 >= 0 && _dim_1 <= 479)
  {
   _Iy_2_stencil_stream_to_A11_2.write(_tmp_stencil);
  }
  if (_dim_0 >= 0 && _dim_0 <= 719 && _dim_1 >= 0 && _dim_1 <= 479)
  {
   _Iy_2_stencil_stream_to_p2_b1.write(_tmp_stencil);
  }
 }
 (void)0;
 // consume Iy$2.stencil.stream
 hls::stream<PackedStencil<int32_t, 1, 1> > _A01_2_stencil_stream;
#pragma HLS STREAM variable=_A01_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_A01_2_stencil_stream core=FIFO_SRL

 // produce A01$2.stencil.stream
 for (int _A01_2_scan_y = 0; _A01_2_scan_y < 0 + 480; _A01_2_scan_y++)
 {
  for (int _A01_2_scan_x = 0; _A01_2_scan_x < 0 + 720; _A01_2_scan_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<int16_t, 5, 5> _Iy_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_Iy_2_stencil.value complete dim=0

   _Iy_2_stencil = _Iy_2_stencil_stream_to_A01_2.read();
   (void)0;
   Stencil<int16_t, 5, 5> _Ix_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_Ix_2_stencil.value complete dim=0

   _Ix_2_stencil = _Ix_2_stencil_stream_to_A01_2.read();
   (void)0;
   Stencil<int32_t, 1, 1> _A01_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_A01_2_stencil.value complete dim=0

   _A01_2_stencil(0, 0) = 0;
   int32_t _553 = _A01_2_stencil(0, 0);
   int16_t _554 = _Ix_2_stencil(0, 0);
   int32_t _555 = (int32_t)(_554);
   int16_t _556 = _Iy_2_stencil(0, 0);
   int32_t _557 = (int32_t)(_556);
   int32_t _558 = _555 * _557;
   int32_t _559 = _553 + _558;
   _A01_2_stencil(0, 0) = _559;
   int32_t _560 = _A01_2_stencil(0, 0);
   int16_t _561 = _Ix_2_stencil(1, 0);
   int32_t _562 = (int32_t)(_561);
   int16_t _563 = _Iy_2_stencil(1, 0);
   int32_t _564 = (int32_t)(_563);
   int32_t _565 = _562 * _564;
   int32_t _566 = _560 + _565;
   _A01_2_stencil(0, 0) = _566;
   int32_t _567 = _A01_2_stencil(0, 0);
   int16_t _568 = _Ix_2_stencil(2, 0);
   int32_t _569 = (int32_t)(_568);
   int16_t _570 = _Iy_2_stencil(2, 0);
   int32_t _571 = (int32_t)(_570);
   int32_t _572 = _569 * _571;
   int32_t _573 = _567 + _572;
   _A01_2_stencil(0, 0) = _573;
   int32_t _574 = _A01_2_stencil(0, 0);
   int16_t _575 = _Ix_2_stencil(3, 0);
   int32_t _576 = (int32_t)(_575);
   int16_t _577 = _Iy_2_stencil(3, 0);
   int32_t _578 = (int32_t)(_577);
   int32_t _579 = _576 * _578;
   int32_t _580 = _574 + _579;
   _A01_2_stencil(0, 0) = _580;
   int32_t _581 = _A01_2_stencil(0, 0);
   int16_t _582 = _Ix_2_stencil(4, 0);
   int32_t _583 = (int32_t)(_582);
   int16_t _584 = _Iy_2_stencil(4, 0);
   int32_t _585 = (int32_t)(_584);
   int32_t _586 = _583 * _585;
   int32_t _587 = _581 + _586;
   _A01_2_stencil(0, 0) = _587;
   int32_t _588 = _A01_2_stencil(0, 0);
   int16_t _589 = _Ix_2_stencil(0, 1);
   int32_t _590 = (int32_t)(_589);
   int16_t _591 = _Iy_2_stencil(0, 1);
   int32_t _592 = (int32_t)(_591);
   int32_t _593 = _590 * _592;
   int32_t _594 = _588 + _593;
   _A01_2_stencil(0, 0) = _594;
   int32_t _595 = _A01_2_stencil(0, 0);
   int16_t _596 = _Ix_2_stencil(1, 1);
   int32_t _597 = (int32_t)(_596);
   int16_t _598 = _Iy_2_stencil(1, 1);
   int32_t _599 = (int32_t)(_598);
   int32_t _600 = _597 * _599;
   int32_t _601 = _595 + _600;
   _A01_2_stencil(0, 0) = _601;
   int32_t _602 = _A01_2_stencil(0, 0);
   int16_t _603 = _Ix_2_stencil(2, 1);
   int32_t _604 = (int32_t)(_603);
   int16_t _605 = _Iy_2_stencil(2, 1);
   int32_t _606 = (int32_t)(_605);
   int32_t _607 = _604 * _606;
   int32_t _608 = _602 + _607;
   _A01_2_stencil(0, 0) = _608;
   int32_t _609 = _A01_2_stencil(0, 0);
   int16_t _610 = _Ix_2_stencil(3, 1);
   int32_t _611 = (int32_t)(_610);
   int16_t _612 = _Iy_2_stencil(3, 1);
   int32_t _613 = (int32_t)(_612);
   int32_t _614 = _611 * _613;
   int32_t _615 = _609 + _614;
   _A01_2_stencil(0, 0) = _615;
   int32_t _616 = _A01_2_stencil(0, 0);
   int16_t _617 = _Ix_2_stencil(4, 1);
   int32_t _618 = (int32_t)(_617);
   int16_t _619 = _Iy_2_stencil(4, 1);
   int32_t _620 = (int32_t)(_619);
   int32_t _621 = _618 * _620;
   int32_t _622 = _616 + _621;
   _A01_2_stencil(0, 0) = _622;
   int32_t _623 = _A01_2_stencil(0, 0);
   int16_t _624 = _Ix_2_stencil(0, 2);
   int32_t _625 = (int32_t)(_624);
   int16_t _626 = _Iy_2_stencil(0, 2);
   int32_t _627 = (int32_t)(_626);
   int32_t _628 = _625 * _627;
   int32_t _629 = _623 + _628;
   _A01_2_stencil(0, 0) = _629;
   int32_t _630 = _A01_2_stencil(0, 0);
   int16_t _631 = _Ix_2_stencil(1, 2);
   int32_t _632 = (int32_t)(_631);
   int16_t _633 = _Iy_2_stencil(1, 2);
   int32_t _634 = (int32_t)(_633);
   int32_t _635 = _632 * _634;
   int32_t _636 = _630 + _635;
   _A01_2_stencil(0, 0) = _636;
   int32_t _637 = _A01_2_stencil(0, 0);
   int16_t _638 = _Ix_2_stencil(2, 2);
   int32_t _639 = (int32_t)(_638);
   int16_t _640 = _Iy_2_stencil(2, 2);
   int32_t _641 = (int32_t)(_640);
   int32_t _642 = _639 * _641;
   int32_t _643 = _637 + _642;
   _A01_2_stencil(0, 0) = _643;
   int32_t _644 = _A01_2_stencil(0, 0);
   int16_t _645 = _Ix_2_stencil(3, 2);
   int32_t _646 = (int32_t)(_645);
   int16_t _647 = _Iy_2_stencil(3, 2);
   int32_t _648 = (int32_t)(_647);
   int32_t _649 = _646 * _648;
   int32_t _650 = _644 + _649;
   _A01_2_stencil(0, 0) = _650;
   int32_t _651 = _A01_2_stencil(0, 0);
   int16_t _652 = _Ix_2_stencil(4, 2);
   int32_t _653 = (int32_t)(_652);
   int16_t _654 = _Iy_2_stencil(4, 2);
   int32_t _655 = (int32_t)(_654);
   int32_t _656 = _653 * _655;
   int32_t _657 = _651 + _656;
   _A01_2_stencil(0, 0) = _657;
   int32_t _658 = _A01_2_stencil(0, 0);
   int16_t _659 = _Ix_2_stencil(0, 3);
   int32_t _660 = (int32_t)(_659);
   int16_t _661 = _Iy_2_stencil(0, 3);
   int32_t _662 = (int32_t)(_661);
   int32_t _663 = _660 * _662;
   int32_t _664 = _658 + _663;
   _A01_2_stencil(0, 0) = _664;
   int32_t _665 = _A01_2_stencil(0, 0);
   int16_t _666 = _Ix_2_stencil(1, 3);
   int32_t _667 = (int32_t)(_666);
   int16_t _668 = _Iy_2_stencil(1, 3);
   int32_t _669 = (int32_t)(_668);
   int32_t _670 = _667 * _669;
   int32_t _671 = _665 + _670;
   _A01_2_stencil(0, 0) = _671;
   int32_t _672 = _A01_2_stencil(0, 0);
   int16_t _673 = _Ix_2_stencil(2, 3);
   int32_t _674 = (int32_t)(_673);
   int16_t _675 = _Iy_2_stencil(2, 3);
   int32_t _676 = (int32_t)(_675);
   int32_t _677 = _674 * _676;
   int32_t _678 = _672 + _677;
   _A01_2_stencil(0, 0) = _678;
   int32_t _679 = _A01_2_stencil(0, 0);
   int16_t _680 = _Ix_2_stencil(3, 3);
   int32_t _681 = (int32_t)(_680);
   int16_t _682 = _Iy_2_stencil(3, 3);
   int32_t _683 = (int32_t)(_682);
   int32_t _684 = _681 * _683;
   int32_t _685 = _679 + _684;
   _A01_2_stencil(0, 0) = _685;
   int32_t _686 = _A01_2_stencil(0, 0);
   int16_t _687 = _Ix_2_stencil(4, 3);
   int32_t _688 = (int32_t)(_687);
   int16_t _689 = _Iy_2_stencil(4, 3);
   int32_t _690 = (int32_t)(_689);
   int32_t _691 = _688 * _690;
   int32_t _692 = _686 + _691;
   _A01_2_stencil(0, 0) = _692;
   int32_t _693 = _A01_2_stencil(0, 0);
   int16_t _694 = _Ix_2_stencil(0, 4);
   int32_t _695 = (int32_t)(_694);
   int16_t _696 = _Iy_2_stencil(0, 4);
   int32_t _697 = (int32_t)(_696);
   int32_t _698 = _695 * _697;
   int32_t _699 = _693 + _698;
   _A01_2_stencil(0, 0) = _699;
   int32_t _700 = _A01_2_stencil(0, 0);
   int16_t _701 = _Ix_2_stencil(1, 4);
   int32_t _702 = (int32_t)(_701);
   int16_t _703 = _Iy_2_stencil(1, 4);
   int32_t _704 = (int32_t)(_703);
   int32_t _705 = _702 * _704;
   int32_t _706 = _700 + _705;
   _A01_2_stencil(0, 0) = _706;
   int32_t _707 = _A01_2_stencil(0, 0);
   int16_t _708 = _Ix_2_stencil(2, 4);
   int32_t _709 = (int32_t)(_708);
   int16_t _710 = _Iy_2_stencil(2, 4);
   int32_t _711 = (int32_t)(_710);
   int32_t _712 = _709 * _711;
   int32_t _713 = _707 + _712;
   _A01_2_stencil(0, 0) = _713;
   int32_t _714 = _A01_2_stencil(0, 0);
   int16_t _715 = _Ix_2_stencil(3, 4);
   int32_t _716 = (int32_t)(_715);
   int16_t _717 = _Iy_2_stencil(3, 4);
   int32_t _718 = (int32_t)(_717);
   int32_t _719 = _716 * _718;
   int32_t _720 = _714 + _719;
   _A01_2_stencil(0, 0) = _720;
   int32_t _721 = _A01_2_stencil(0, 0);
   int16_t _722 = _Ix_2_stencil(4, 4);
   int32_t _723 = (int32_t)(_722);
   int16_t _724 = _Iy_2_stencil(4, 4);
   int32_t _725 = (int32_t)(_724);
   int32_t _726 = _723 * _725;
   int32_t _727 = _721 + _726;
   _A01_2_stencil(0, 0) = _727;
   _A01_2_stencil_stream.write(_A01_2_stencil);
   (void)0;
  } // for _A01_2_scan_x
 } // for _A01_2_scan_y
 // dispatch_stream(_A01_2_stencil_stream, 2, 1, 1, 720, 1, 1, 480, 1, "__auto_insert__hw_output$2", 1, 0, 720, 0, 480);
 hls::stream<PackedStencil<int32_t, 1, 1> > &_A01_2_stencil_stream_to___auto_insert__hw_output_2 = _A01_2_stencil_stream;
 (void)0;
 // consume A01$2.stencil.stream
 hls::stream<PackedStencil<int32_t, 1, 1> > _A10_2_stencil_stream;
#pragma HLS STREAM variable=_A10_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_A10_2_stencil_stream core=FIFO_SRL

 // produce A10$2.stencil.stream
 for (int _A10_2_scan_y = 0; _A10_2_scan_y < 0 + 480; _A10_2_scan_y++)
 {
  for (int _A10_2_scan_x = 0; _A10_2_scan_x < 0 + 720; _A10_2_scan_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<int16_t, 5, 5> _Iy_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_Iy_2_stencil.value complete dim=0

   _Iy_2_stencil = _Iy_2_stencil_stream_to_A10_2.read();
   (void)0;
   Stencil<int16_t, 5, 5> _Ix_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_Ix_2_stencil.value complete dim=0

   _Ix_2_stencil = _Ix_2_stencil_stream_to_A10_2.read();
   (void)0;
   Stencil<int32_t, 1, 1> _A10_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_A10_2_stencil.value complete dim=0

   _A10_2_stencil(0, 0) = 0;
   int32_t _728 = _A10_2_stencil(0, 0);
   int16_t _729 = _Ix_2_stencil(0, 0);
   int32_t _730 = (int32_t)(_729);
   int16_t _731 = _Iy_2_stencil(0, 0);
   int32_t _732 = (int32_t)(_731);
   int32_t _733 = _730 * _732;
   int32_t _734 = _728 + _733;
   _A10_2_stencil(0, 0) = _734;
   int32_t _735 = _A10_2_stencil(0, 0);
   int16_t _736 = _Ix_2_stencil(1, 0);
   int32_t _737 = (int32_t)(_736);
   int16_t _738 = _Iy_2_stencil(1, 0);
   int32_t _739 = (int32_t)(_738);
   int32_t _740 = _737 * _739;
   int32_t _741 = _735 + _740;
   _A10_2_stencil(0, 0) = _741;
   int32_t _742 = _A10_2_stencil(0, 0);
   int16_t _743 = _Ix_2_stencil(2, 0);
   int32_t _744 = (int32_t)(_743);
   int16_t _745 = _Iy_2_stencil(2, 0);
   int32_t _746 = (int32_t)(_745);
   int32_t _747 = _744 * _746;
   int32_t _748 = _742 + _747;
   _A10_2_stencil(0, 0) = _748;
   int32_t _749 = _A10_2_stencil(0, 0);
   int16_t _750 = _Ix_2_stencil(3, 0);
   int32_t _751 = (int32_t)(_750);
   int16_t _752 = _Iy_2_stencil(3, 0);
   int32_t _753 = (int32_t)(_752);
   int32_t _754 = _751 * _753;
   int32_t _755 = _749 + _754;
   _A10_2_stencil(0, 0) = _755;
   int32_t _756 = _A10_2_stencil(0, 0);
   int16_t _757 = _Ix_2_stencil(4, 0);
   int32_t _758 = (int32_t)(_757);
   int16_t _759 = _Iy_2_stencil(4, 0);
   int32_t _760 = (int32_t)(_759);
   int32_t _761 = _758 * _760;
   int32_t _762 = _756 + _761;
   _A10_2_stencil(0, 0) = _762;
   int32_t _763 = _A10_2_stencil(0, 0);
   int16_t _764 = _Ix_2_stencil(0, 1);
   int32_t _765 = (int32_t)(_764);
   int16_t _766 = _Iy_2_stencil(0, 1);
   int32_t _767 = (int32_t)(_766);
   int32_t _768 = _765 * _767;
   int32_t _769 = _763 + _768;
   _A10_2_stencil(0, 0) = _769;
   int32_t _770 = _A10_2_stencil(0, 0);
   int16_t _771 = _Ix_2_stencil(1, 1);
   int32_t _772 = (int32_t)(_771);
   int16_t _773 = _Iy_2_stencil(1, 1);
   int32_t _774 = (int32_t)(_773);
   int32_t _775 = _772 * _774;
   int32_t _776 = _770 + _775;
   _A10_2_stencil(0, 0) = _776;
   int32_t _777 = _A10_2_stencil(0, 0);
   int16_t _778 = _Ix_2_stencil(2, 1);
   int32_t _779 = (int32_t)(_778);
   int16_t _780 = _Iy_2_stencil(2, 1);
   int32_t _781 = (int32_t)(_780);
   int32_t _782 = _779 * _781;
   int32_t _783 = _777 + _782;
   _A10_2_stencil(0, 0) = _783;
   int32_t _784 = _A10_2_stencil(0, 0);
   int16_t _785 = _Ix_2_stencil(3, 1);
   int32_t _786 = (int32_t)(_785);
   int16_t _787 = _Iy_2_stencil(3, 1);
   int32_t _788 = (int32_t)(_787);
   int32_t _789 = _786 * _788;
   int32_t _790 = _784 + _789;
   _A10_2_stencil(0, 0) = _790;
   int32_t _791 = _A10_2_stencil(0, 0);
   int16_t _792 = _Ix_2_stencil(4, 1);
   int32_t _793 = (int32_t)(_792);
   int16_t _794 = _Iy_2_stencil(4, 1);
   int32_t _795 = (int32_t)(_794);
   int32_t _796 = _793 * _795;
   int32_t _797 = _791 + _796;
   _A10_2_stencil(0, 0) = _797;
   int32_t _798 = _A10_2_stencil(0, 0);
   int16_t _799 = _Ix_2_stencil(0, 2);
   int32_t _800 = (int32_t)(_799);
   int16_t _801 = _Iy_2_stencil(0, 2);
   int32_t _802 = (int32_t)(_801);
   int32_t _803 = _800 * _802;
   int32_t _804 = _798 + _803;
   _A10_2_stencil(0, 0) = _804;
   int32_t _805 = _A10_2_stencil(0, 0);
   int16_t _806 = _Ix_2_stencil(1, 2);
   int32_t _807 = (int32_t)(_806);
   int16_t _808 = _Iy_2_stencil(1, 2);
   int32_t _809 = (int32_t)(_808);
   int32_t _810 = _807 * _809;
   int32_t _811 = _805 + _810;
   _A10_2_stencil(0, 0) = _811;
   int32_t _812 = _A10_2_stencil(0, 0);
   int16_t _813 = _Ix_2_stencil(2, 2);
   int32_t _814 = (int32_t)(_813);
   int16_t _815 = _Iy_2_stencil(2, 2);
   int32_t _816 = (int32_t)(_815);
   int32_t _817 = _814 * _816;
   int32_t _818 = _812 + _817;
   _A10_2_stencil(0, 0) = _818;
   int32_t _819 = _A10_2_stencil(0, 0);
   int16_t _820 = _Ix_2_stencil(3, 2);
   int32_t _821 = (int32_t)(_820);
   int16_t _822 = _Iy_2_stencil(3, 2);
   int32_t _823 = (int32_t)(_822);
   int32_t _824 = _821 * _823;
   int32_t _825 = _819 + _824;
   _A10_2_stencil(0, 0) = _825;
   int32_t _826 = _A10_2_stencil(0, 0);
   int16_t _827 = _Ix_2_stencil(4, 2);
   int32_t _828 = (int32_t)(_827);
   int16_t _829 = _Iy_2_stencil(4, 2);
   int32_t _830 = (int32_t)(_829);
   int32_t _831 = _828 * _830;
   int32_t _832 = _826 + _831;
   _A10_2_stencil(0, 0) = _832;
   int32_t _833 = _A10_2_stencil(0, 0);
   int16_t _834 = _Ix_2_stencil(0, 3);
   int32_t _835 = (int32_t)(_834);
   int16_t _836 = _Iy_2_stencil(0, 3);
   int32_t _837 = (int32_t)(_836);
   int32_t _838 = _835 * _837;
   int32_t _839 = _833 + _838;
   _A10_2_stencil(0, 0) = _839;
   int32_t _840 = _A10_2_stencil(0, 0);
   int16_t _841 = _Ix_2_stencil(1, 3);
   int32_t _842 = (int32_t)(_841);
   int16_t _843 = _Iy_2_stencil(1, 3);
   int32_t _844 = (int32_t)(_843);
   int32_t _845 = _842 * _844;
   int32_t _846 = _840 + _845;
   _A10_2_stencil(0, 0) = _846;
   int32_t _847 = _A10_2_stencil(0, 0);
   int16_t _848 = _Ix_2_stencil(2, 3);
   int32_t _849 = (int32_t)(_848);
   int16_t _850 = _Iy_2_stencil(2, 3);
   int32_t _851 = (int32_t)(_850);
   int32_t _852 = _849 * _851;
   int32_t _853 = _847 + _852;
   _A10_2_stencil(0, 0) = _853;
   int32_t _854 = _A10_2_stencil(0, 0);
   int16_t _855 = _Ix_2_stencil(3, 3);
   int32_t _856 = (int32_t)(_855);
   int16_t _857 = _Iy_2_stencil(3, 3);
   int32_t _858 = (int32_t)(_857);
   int32_t _859 = _856 * _858;
   int32_t _860 = _854 + _859;
   _A10_2_stencil(0, 0) = _860;
   int32_t _861 = _A10_2_stencil(0, 0);
   int16_t _862 = _Ix_2_stencil(4, 3);
   int32_t _863 = (int32_t)(_862);
   int16_t _864 = _Iy_2_stencil(4, 3);
   int32_t _865 = (int32_t)(_864);
   int32_t _866 = _863 * _865;
   int32_t _867 = _861 + _866;
   _A10_2_stencil(0, 0) = _867;
   int32_t _868 = _A10_2_stencil(0, 0);
   int16_t _869 = _Ix_2_stencil(0, 4);
   int32_t _870 = (int32_t)(_869);
   int16_t _871 = _Iy_2_stencil(0, 4);
   int32_t _872 = (int32_t)(_871);
   int32_t _873 = _870 * _872;
   int32_t _874 = _868 + _873;
   _A10_2_stencil(0, 0) = _874;
   int32_t _875 = _A10_2_stencil(0, 0);
   int16_t _876 = _Ix_2_stencil(1, 4);
   int32_t _877 = (int32_t)(_876);
   int16_t _878 = _Iy_2_stencil(1, 4);
   int32_t _879 = (int32_t)(_878);
   int32_t _880 = _877 * _879;
   int32_t _881 = _875 + _880;
   _A10_2_stencil(0, 0) = _881;
   int32_t _882 = _A10_2_stencil(0, 0);
   int16_t _883 = _Ix_2_stencil(2, 4);
   int32_t _884 = (int32_t)(_883);
   int16_t _885 = _Iy_2_stencil(2, 4);
   int32_t _886 = (int32_t)(_885);
   int32_t _887 = _884 * _886;
   int32_t _888 = _882 + _887;
   _A10_2_stencil(0, 0) = _888;
   int32_t _889 = _A10_2_stencil(0, 0);
   int16_t _890 = _Ix_2_stencil(3, 4);
   int32_t _891 = (int32_t)(_890);
   int16_t _892 = _Iy_2_stencil(3, 4);
   int32_t _893 = (int32_t)(_892);
   int32_t _894 = _891 * _893;
   int32_t _895 = _889 + _894;
   _A10_2_stencil(0, 0) = _895;
   int32_t _896 = _A10_2_stencil(0, 0);
   int16_t _897 = _Ix_2_stencil(4, 4);
   int32_t _898 = (int32_t)(_897);
   int16_t _899 = _Iy_2_stencil(4, 4);
   int32_t _900 = (int32_t)(_899);
   int32_t _901 = _898 * _900;
   int32_t _902 = _896 + _901;
   _A10_2_stencil(0, 0) = _902;
   _A10_2_stencil_stream.write(_A10_2_stencil);
   (void)0;
  } // for _A10_2_scan_x
 } // for _A10_2_scan_y
 // dispatch_stream(_A10_2_stencil_stream, 2, 1, 1, 720, 1, 1, 480, 1, "__auto_insert__hw_output$2", 1, 0, 720, 0, 480);
 hls::stream<PackedStencil<int32_t, 1, 1> > &_A10_2_stencil_stream_to___auto_insert__hw_output_2 = _A10_2_stencil_stream;
 (void)0;
 // consume A10$2.stencil.stream
 hls::stream<PackedStencil<int32_t, 1, 1> > _A11_2_stencil_stream;
#pragma HLS STREAM variable=_A11_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_A11_2_stencil_stream core=FIFO_SRL

 // produce A11$2.stencil.stream
 for (int _A11_2_scan_y = 0; _A11_2_scan_y < 0 + 480; _A11_2_scan_y++)
 {
  for (int _A11_2_scan_x = 0; _A11_2_scan_x < 0 + 720; _A11_2_scan_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<int16_t, 5, 5> _Iy_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_Iy_2_stencil.value complete dim=0

   _Iy_2_stencil = _Iy_2_stencil_stream_to_A11_2.read();
   (void)0;
   Stencil<int32_t, 1, 1> _A11_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_A11_2_stencil.value complete dim=0

   _A11_2_stencil(0, 0) = 0;
   int16_t _903 = _Iy_2_stencil(0, 0);
   int32_t _904 = _A11_2_stencil(0, 0);
   int32_t _905 = (int32_t)(_903);
   int32_t _906 = _905 * _905;
   int32_t _907 = _904 + _906;
   _A11_2_stencil(0, 0) = _907;
   int16_t _908 = _Iy_2_stencil(1, 0);
   int32_t _909 = _A11_2_stencil(0, 0);
   int32_t _910 = (int32_t)(_908);
   int32_t _911 = _910 * _910;
   int32_t _912 = _909 + _911;
   _A11_2_stencil(0, 0) = _912;
   int16_t _913 = _Iy_2_stencil(2, 0);
   int32_t _914 = _A11_2_stencil(0, 0);
   int32_t _915 = (int32_t)(_913);
   int32_t _916 = _915 * _915;
   int32_t _917 = _914 + _916;
   _A11_2_stencil(0, 0) = _917;
   int16_t _918 = _Iy_2_stencil(3, 0);
   int32_t _919 = _A11_2_stencil(0, 0);
   int32_t _920 = (int32_t)(_918);
   int32_t _921 = _920 * _920;
   int32_t _922 = _919 + _921;
   _A11_2_stencil(0, 0) = _922;
   int16_t _923 = _Iy_2_stencil(4, 0);
   int32_t _924 = _A11_2_stencil(0, 0);
   int32_t _925 = (int32_t)(_923);
   int32_t _926 = _925 * _925;
   int32_t _927 = _924 + _926;
   _A11_2_stencil(0, 0) = _927;
   int16_t _928 = _Iy_2_stencil(0, 1);
   int32_t _929 = _A11_2_stencil(0, 0);
   int32_t _930 = (int32_t)(_928);
   int32_t _931 = _930 * _930;
   int32_t _932 = _929 + _931;
   _A11_2_stencil(0, 0) = _932;
   int16_t _933 = _Iy_2_stencil(1, 1);
   int32_t _934 = _A11_2_stencil(0, 0);
   int32_t _935 = (int32_t)(_933);
   int32_t _936 = _935 * _935;
   int32_t _937 = _934 + _936;
   _A11_2_stencil(0, 0) = _937;
   int16_t _938 = _Iy_2_stencil(2, 1);
   int32_t _939 = _A11_2_stencil(0, 0);
   int32_t _940 = (int32_t)(_938);
   int32_t _941 = _940 * _940;
   int32_t _942 = _939 + _941;
   _A11_2_stencil(0, 0) = _942;
   int16_t _943 = _Iy_2_stencil(3, 1);
   int32_t _944 = _A11_2_stencil(0, 0);
   int32_t _945 = (int32_t)(_943);
   int32_t _946 = _945 * _945;
   int32_t _947 = _944 + _946;
   _A11_2_stencil(0, 0) = _947;
   int16_t _948 = _Iy_2_stencil(4, 1);
   int32_t _949 = _A11_2_stencil(0, 0);
   int32_t _950 = (int32_t)(_948);
   int32_t _951 = _950 * _950;
   int32_t _952 = _949 + _951;
   _A11_2_stencil(0, 0) = _952;
   int16_t _953 = _Iy_2_stencil(0, 2);
   int32_t _954 = _A11_2_stencil(0, 0);
   int32_t _955 = (int32_t)(_953);
   int32_t _956 = _955 * _955;
   int32_t _957 = _954 + _956;
   _A11_2_stencil(0, 0) = _957;
   int16_t _958 = _Iy_2_stencil(1, 2);
   int32_t _959 = _A11_2_stencil(0, 0);
   int32_t _960 = (int32_t)(_958);
   int32_t _961 = _960 * _960;
   int32_t _962 = _959 + _961;
   _A11_2_stencil(0, 0) = _962;
   int16_t _963 = _Iy_2_stencil(2, 2);
   int32_t _964 = _A11_2_stencil(0, 0);
   int32_t _965 = (int32_t)(_963);
   int32_t _966 = _965 * _965;
   int32_t _967 = _964 + _966;
   _A11_2_stencil(0, 0) = _967;
   int16_t _968 = _Iy_2_stencil(3, 2);
   int32_t _969 = _A11_2_stencil(0, 0);
   int32_t _970 = (int32_t)(_968);
   int32_t _971 = _970 * _970;
   int32_t _972 = _969 + _971;
   _A11_2_stencil(0, 0) = _972;
   int16_t _973 = _Iy_2_stencil(4, 2);
   int32_t _974 = _A11_2_stencil(0, 0);
   int32_t _975 = (int32_t)(_973);
   int32_t _976 = _975 * _975;
   int32_t _977 = _974 + _976;
   _A11_2_stencil(0, 0) = _977;
   int16_t _978 = _Iy_2_stencil(0, 3);
   int32_t _979 = _A11_2_stencil(0, 0);
   int32_t _980 = (int32_t)(_978);
   int32_t _981 = _980 * _980;
   int32_t _982 = _979 + _981;
   _A11_2_stencil(0, 0) = _982;
   int16_t _983 = _Iy_2_stencil(1, 3);
   int32_t _984 = _A11_2_stencil(0, 0);
   int32_t _985 = (int32_t)(_983);
   int32_t _986 = _985 * _985;
   int32_t _987 = _984 + _986;
   _A11_2_stencil(0, 0) = _987;
   int16_t _988 = _Iy_2_stencil(2, 3);
   int32_t _989 = _A11_2_stencil(0, 0);
   int32_t _990 = (int32_t)(_988);
   int32_t _991 = _990 * _990;
   int32_t _992 = _989 + _991;
   _A11_2_stencil(0, 0) = _992;
   int16_t _993 = _Iy_2_stencil(3, 3);
   int32_t _994 = _A11_2_stencil(0, 0);
   int32_t _995 = (int32_t)(_993);
   int32_t _996 = _995 * _995;
   int32_t _997 = _994 + _996;
   _A11_2_stencil(0, 0) = _997;
   int16_t _998 = _Iy_2_stencil(4, 3);
   int32_t _999 = _A11_2_stencil(0, 0);
   int32_t _1000 = (int32_t)(_998);
   int32_t _1001 = _1000 * _1000;
   int32_t _1002 = _999 + _1001;
   _A11_2_stencil(0, 0) = _1002;
   int16_t _1003 = _Iy_2_stencil(0, 4);
   int32_t _1004 = _A11_2_stencil(0, 0);
   int32_t _1005 = (int32_t)(_1003);
   int32_t _1006 = _1005 * _1005;
   int32_t _1007 = _1004 + _1006;
   _A11_2_stencil(0, 0) = _1007;
   int16_t _1008 = _Iy_2_stencil(1, 4);
   int32_t _1009 = _A11_2_stencil(0, 0);
   int32_t _1010 = (int32_t)(_1008);
   int32_t _1011 = _1010 * _1010;
   int32_t _1012 = _1009 + _1011;
   _A11_2_stencil(0, 0) = _1012;
   int16_t _1013 = _Iy_2_stencil(2, 4);
   int32_t _1014 = _A11_2_stencil(0, 0);
   int32_t _1015 = (int32_t)(_1013);
   int32_t _1016 = _1015 * _1015;
   int32_t _1017 = _1014 + _1016;
   _A11_2_stencil(0, 0) = _1017;
   int16_t _1018 = _Iy_2_stencil(3, 4);
   int32_t _1019 = _A11_2_stencil(0, 0);
   int32_t _1020 = (int32_t)(_1018);
   int32_t _1021 = _1020 * _1020;
   int32_t _1022 = _1019 + _1021;
   _A11_2_stencil(0, 0) = _1022;
   int16_t _1023 = _Iy_2_stencil(4, 4);
   int32_t _1024 = _A11_2_stencil(0, 0);
   int32_t _1025 = (int32_t)(_1023);
   int32_t _1026 = _1025 * _1025;
   int32_t _1027 = _1024 + _1026;
   _A11_2_stencil(0, 0) = _1027;
   _A11_2_stencil_stream.write(_A11_2_stencil);
   (void)0;
  } // for _A11_2_scan_x
 } // for _A11_2_scan_y
 // dispatch_stream(_A11_2_stencil_stream, 2, 1, 1, 720, 1, 1, 480, 1, "__auto_insert__hw_output$2", 1, 0, 720, 0, 480);
 hls::stream<PackedStencil<int32_t, 1, 1> > &_A11_2_stencil_stream_to___auto_insert__hw_output_2 = _A11_2_stencil_stream;
 (void)0;
 // consume A11$2.stencil.stream
 hls::stream<PackedStencil<int16_t, 1, 1> > _It_2_stencil_update_stream;
#pragma HLS STREAM variable=_It_2_stencil_update_stream depth=1
#pragma HLS RESOURCE variable=_It_2_stencil_update_stream core=FIFO_SRL

 // produce It$2.stencil_update.stream
 for (int _It_2_scan_update_y = 0; _It_2_scan_update_y < 0 + 484; _It_2_scan_update_y++)
 {
  for (int _It_2_scan_update_x = 0; _It_2_scan_update_x < 0 + 724; _It_2_scan_update_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<uint8_t, 1, 1> _gray_4_stencil;
#pragma HLS ARRAY_PARTITION variable=_gray_4_stencil.value complete dim=0

   _gray_4_stencil = _gray_4_stencil_stream_to_It_2.read();
   (void)0;
   Stencil<uint8_t, 3, 3> _gray_3_stencil;
#pragma HLS ARRAY_PARTITION variable=_gray_3_stencil.value complete dim=0

   _gray_3_stencil = _gray_3_stencil_stream_to_It_2.read();
   (void)0;
   Stencil<int16_t, 1, 1> _It_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_It_2_stencil.value complete dim=0

   uint8_t _1028 = _gray_4_stencil(0, 0);
   int16_t _1029 = (int16_t)(_1028);
   uint8_t _1030 = _gray_3_stencil(1, 1);
   int16_t _1031 = (int16_t)(_1030);
   int16_t _1032 = _1029 - _1031;
   _It_2_stencil(0, 0) = _1032;
   _It_2_stencil_update_stream.write(_It_2_stencil);
   (void)0;
  } // for _It_2_scan_update_x
 } // for _It_2_scan_update_y
 // consume It$2.stencil_update.stream
 hls::stream<PackedStencil<int16_t, 5, 5> > _It_2_stencil_stream;
#pragma HLS STREAM variable=_It_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_It_2_stencil_stream core=FIFO_SRL

 // produce It$2.stencil.stream
 linebuffer<724, 484>(_It_2_stencil_update_stream, _It_2_stencil_stream);
 (void)0;
 // dispatch_stream(_It_2_stencil_stream, 2, 5, 1, 724, 5, 1, 484, 2, "p2:b0", 1, 0, 724, 0, 484, "p2:b1", 1, 0, 724, 0, 484);
 hls::stream<PackedStencil<int16_t, 5, 5> > _It_2_stencil_stream_to_p2_b0;
#pragma HLS STREAM variable=_It_2_stencil_stream_to_p2_b0 depth=1
#pragma HLS RESOURCE variable=_It_2_stencil_stream_to_p2_b0 core=FIFO_SRL

 hls::stream<PackedStencil<int16_t, 5, 5> > _It_2_stencil_stream_to_p2_b1;
#pragma HLS STREAM variable=_It_2_stencil_stream_to_p2_b1 depth=1
#pragma HLS RESOURCE variable=_It_2_stencil_stream_to_p2_b1 core=FIFO_SRL

 for (int _dim_1 = 0; _dim_1 <= 479; _dim_1 += 1)
 for (int _dim_0 = 0; _dim_0 <= 719; _dim_0 += 1)
 {
#pragma HLS PIPELINE
  Stencil<int16_t, 5, 5> _tmp_stencil = _It_2_stencil_stream.read();
#pragma HLS ARRAY_PARTITION variable=_tmp_stencil.value complete dim=0

  if (_dim_0 >= 0 && _dim_0 <= 719 && _dim_1 >= 0 && _dim_1 <= 479)
  {
   _It_2_stencil_stream_to_p2_b0.write(_tmp_stencil);
  }
  if (_dim_0 >= 0 && _dim_0 <= 719 && _dim_1 >= 0 && _dim_1 <= 479)
  {
   _It_2_stencil_stream_to_p2_b1.write(_tmp_stencil);
  }
 }
 (void)0;
 // consume It$2.stencil.stream
 hls::stream<PackedStencil<int32_t, 1, 1> > _p2_b0_stencil_stream;
#pragma HLS STREAM variable=_p2_b0_stencil_stream depth=1
#pragma HLS RESOURCE variable=_p2_b0_stencil_stream core=FIFO_SRL

 // produce p2:b0.stencil.stream
 for (int _p2_b0_scan_y = 0; _p2_b0_scan_y < 0 + 480; _p2_b0_scan_y++)
 {
  for (int _p2_b0_scan_x = 0; _p2_b0_scan_x < 0 + 720; _p2_b0_scan_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<int16_t, 5, 5> _Ix_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_Ix_2_stencil.value complete dim=0

   _Ix_2_stencil = _Ix_2_stencil_stream_to_p2_b0.read();
   (void)0;
   Stencil<int16_t, 5, 5> _It_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_It_2_stencil.value complete dim=0

   _It_2_stencil = _It_2_stencil_stream_to_p2_b0.read();
   (void)0;
   Stencil<int32_t, 1, 1> _p2_b0_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_b0_stencil.value complete dim=0

   _p2_b0_stencil(0, 0) = 0;
   int32_t _1033 = _p2_b0_stencil(0, 0);
   int16_t _1034 = _Ix_2_stencil(0, 0);
   int32_t _1035 = (int32_t)(_1034);
   int16_t _1036 = _It_2_stencil(0, 0);
   int32_t _1037 = (int32_t)(_1036);
   int32_t _1038 = _1035 * _1037;
   int32_t _1039 = _1033 + _1038;
   _p2_b0_stencil(0, 0) = _1039;
   int32_t _1040 = _p2_b0_stencil(0, 0);
   int16_t _1041 = _Ix_2_stencil(1, 0);
   int32_t _1042 = (int32_t)(_1041);
   int16_t _1043 = _It_2_stencil(1, 0);
   int32_t _1044 = (int32_t)(_1043);
   int32_t _1045 = _1042 * _1044;
   int32_t _1046 = _1040 + _1045;
   _p2_b0_stencil(0, 0) = _1046;
   int32_t _1047 = _p2_b0_stencil(0, 0);
   int16_t _1048 = _Ix_2_stencil(2, 0);
   int32_t _1049 = (int32_t)(_1048);
   int16_t _1050 = _It_2_stencil(2, 0);
   int32_t _1051 = (int32_t)(_1050);
   int32_t _1052 = _1049 * _1051;
   int32_t _1053 = _1047 + _1052;
   _p2_b0_stencil(0, 0) = _1053;
   int32_t _1054 = _p2_b0_stencil(0, 0);
   int16_t _1055 = _Ix_2_stencil(3, 0);
   int32_t _1056 = (int32_t)(_1055);
   int16_t _1057 = _It_2_stencil(3, 0);
   int32_t _1058 = (int32_t)(_1057);
   int32_t _1059 = _1056 * _1058;
   int32_t _1060 = _1054 + _1059;
   _p2_b0_stencil(0, 0) = _1060;
   int32_t _1061 = _p2_b0_stencil(0, 0);
   int16_t _1062 = _Ix_2_stencil(4, 0);
   int32_t _1063 = (int32_t)(_1062);
   int16_t _1064 = _It_2_stencil(4, 0);
   int32_t _1065 = (int32_t)(_1064);
   int32_t _1066 = _1063 * _1065;
   int32_t _1067 = _1061 + _1066;
   _p2_b0_stencil(0, 0) = _1067;
   int32_t _1068 = _p2_b0_stencil(0, 0);
   int16_t _1069 = _Ix_2_stencil(0, 1);
   int32_t _1070 = (int32_t)(_1069);
   int16_t _1071 = _It_2_stencil(0, 1);
   int32_t _1072 = (int32_t)(_1071);
   int32_t _1073 = _1070 * _1072;
   int32_t _1074 = _1068 + _1073;
   _p2_b0_stencil(0, 0) = _1074;
   int32_t _1075 = _p2_b0_stencil(0, 0);
   int16_t _1076 = _Ix_2_stencil(1, 1);
   int32_t _1077 = (int32_t)(_1076);
   int16_t _1078 = _It_2_stencil(1, 1);
   int32_t _1079 = (int32_t)(_1078);
   int32_t _1080 = _1077 * _1079;
   int32_t _1081 = _1075 + _1080;
   _p2_b0_stencil(0, 0) = _1081;
   int32_t _1082 = _p2_b0_stencil(0, 0);
   int16_t _1083 = _Ix_2_stencil(2, 1);
   int32_t _1084 = (int32_t)(_1083);
   int16_t _1085 = _It_2_stencil(2, 1);
   int32_t _1086 = (int32_t)(_1085);
   int32_t _1087 = _1084 * _1086;
   int32_t _1088 = _1082 + _1087;
   _p2_b0_stencil(0, 0) = _1088;
   int32_t _1089 = _p2_b0_stencil(0, 0);
   int16_t _1090 = _Ix_2_stencil(3, 1);
   int32_t _1091 = (int32_t)(_1090);
   int16_t _1092 = _It_2_stencil(3, 1);
   int32_t _1093 = (int32_t)(_1092);
   int32_t _1094 = _1091 * _1093;
   int32_t _1095 = _1089 + _1094;
   _p2_b0_stencil(0, 0) = _1095;
   int32_t _1096 = _p2_b0_stencil(0, 0);
   int16_t _1097 = _Ix_2_stencil(4, 1);
   int32_t _1098 = (int32_t)(_1097);
   int16_t _1099 = _It_2_stencil(4, 1);
   int32_t _1100 = (int32_t)(_1099);
   int32_t _1101 = _1098 * _1100;
   int32_t _1102 = _1096 + _1101;
   _p2_b0_stencil(0, 0) = _1102;
   int32_t _1103 = _p2_b0_stencil(0, 0);
   int16_t _1104 = _Ix_2_stencil(0, 2);
   int32_t _1105 = (int32_t)(_1104);
   int16_t _1106 = _It_2_stencil(0, 2);
   int32_t _1107 = (int32_t)(_1106);
   int32_t _1108 = _1105 * _1107;
   int32_t _1109 = _1103 + _1108;
   _p2_b0_stencil(0, 0) = _1109;
   int32_t _1110 = _p2_b0_stencil(0, 0);
   int16_t _1111 = _Ix_2_stencil(1, 2);
   int32_t _1112 = (int32_t)(_1111);
   int16_t _1113 = _It_2_stencil(1, 2);
   int32_t _1114 = (int32_t)(_1113);
   int32_t _1115 = _1112 * _1114;
   int32_t _1116 = _1110 + _1115;
   _p2_b0_stencil(0, 0) = _1116;
   int32_t _1117 = _p2_b0_stencil(0, 0);
   int16_t _1118 = _Ix_2_stencil(2, 2);
   int32_t _1119 = (int32_t)(_1118);
   int16_t _1120 = _It_2_stencil(2, 2);
   int32_t _1121 = (int32_t)(_1120);
   int32_t _1122 = _1119 * _1121;
   int32_t _1123 = _1117 + _1122;
   _p2_b0_stencil(0, 0) = _1123;
   int32_t _1124 = _p2_b0_stencil(0, 0);
   int16_t _1125 = _Ix_2_stencil(3, 2);
   int32_t _1126 = (int32_t)(_1125);
   int16_t _1127 = _It_2_stencil(3, 2);
   int32_t _1128 = (int32_t)(_1127);
   int32_t _1129 = _1126 * _1128;
   int32_t _1130 = _1124 + _1129;
   _p2_b0_stencil(0, 0) = _1130;
   int32_t _1131 = _p2_b0_stencil(0, 0);
   int16_t _1132 = _Ix_2_stencil(4, 2);
   int32_t _1133 = (int32_t)(_1132);
   int16_t _1134 = _It_2_stencil(4, 2);
   int32_t _1135 = (int32_t)(_1134);
   int32_t _1136 = _1133 * _1135;
   int32_t _1137 = _1131 + _1136;
   _p2_b0_stencil(0, 0) = _1137;
   int32_t _1138 = _p2_b0_stencil(0, 0);
   int16_t _1139 = _Ix_2_stencil(0, 3);
   int32_t _1140 = (int32_t)(_1139);
   int16_t _1141 = _It_2_stencil(0, 3);
   int32_t _1142 = (int32_t)(_1141);
   int32_t _1143 = _1140 * _1142;
   int32_t _1144 = _1138 + _1143;
   _p2_b0_stencil(0, 0) = _1144;
   int32_t _1145 = _p2_b0_stencil(0, 0);
   int16_t _1146 = _Ix_2_stencil(1, 3);
   int32_t _1147 = (int32_t)(_1146);
   int16_t _1148 = _It_2_stencil(1, 3);
   int32_t _1149 = (int32_t)(_1148);
   int32_t _1150 = _1147 * _1149;
   int32_t _1151 = _1145 + _1150;
   _p2_b0_stencil(0, 0) = _1151;
   int32_t _1152 = _p2_b0_stencil(0, 0);
   int16_t _1153 = _Ix_2_stencil(2, 3);
   int32_t _1154 = (int32_t)(_1153);
   int16_t _1155 = _It_2_stencil(2, 3);
   int32_t _1156 = (int32_t)(_1155);
   int32_t _1157 = _1154 * _1156;
   int32_t _1158 = _1152 + _1157;
   _p2_b0_stencil(0, 0) = _1158;
   int32_t _1159 = _p2_b0_stencil(0, 0);
   int16_t _1160 = _Ix_2_stencil(3, 3);
   int32_t _1161 = (int32_t)(_1160);
   int16_t _1162 = _It_2_stencil(3, 3);
   int32_t _1163 = (int32_t)(_1162);
   int32_t _1164 = _1161 * _1163;
   int32_t _1165 = _1159 + _1164;
   _p2_b0_stencil(0, 0) = _1165;
   int32_t _1166 = _p2_b0_stencil(0, 0);
   int16_t _1167 = _Ix_2_stencil(4, 3);
   int32_t _1168 = (int32_t)(_1167);
   int16_t _1169 = _It_2_stencil(4, 3);
   int32_t _1170 = (int32_t)(_1169);
   int32_t _1171 = _1168 * _1170;
   int32_t _1172 = _1166 + _1171;
   _p2_b0_stencil(0, 0) = _1172;
   int32_t _1173 = _p2_b0_stencil(0, 0);
   int16_t _1174 = _Ix_2_stencil(0, 4);
   int32_t _1175 = (int32_t)(_1174);
   int16_t _1176 = _It_2_stencil(0, 4);
   int32_t _1177 = (int32_t)(_1176);
   int32_t _1178 = _1175 * _1177;
   int32_t _1179 = _1173 + _1178;
   _p2_b0_stencil(0, 0) = _1179;
   int32_t _1180 = _p2_b0_stencil(0, 0);
   int16_t _1181 = _Ix_2_stencil(1, 4);
   int32_t _1182 = (int32_t)(_1181);
   int16_t _1183 = _It_2_stencil(1, 4);
   int32_t _1184 = (int32_t)(_1183);
   int32_t _1185 = _1182 * _1184;
   int32_t _1186 = _1180 + _1185;
   _p2_b0_stencil(0, 0) = _1186;
   int32_t _1187 = _p2_b0_stencil(0, 0);
   int16_t _1188 = _Ix_2_stencil(2, 4);
   int32_t _1189 = (int32_t)(_1188);
   int16_t _1190 = _It_2_stencil(2, 4);
   int32_t _1191 = (int32_t)(_1190);
   int32_t _1192 = _1189 * _1191;
   int32_t _1193 = _1187 + _1192;
   _p2_b0_stencil(0, 0) = _1193;
   int32_t _1194 = _p2_b0_stencil(0, 0);
   int16_t _1195 = _Ix_2_stencil(3, 4);
   int32_t _1196 = (int32_t)(_1195);
   int16_t _1197 = _It_2_stencil(3, 4);
   int32_t _1198 = (int32_t)(_1197);
   int32_t _1199 = _1196 * _1198;
   int32_t _1200 = _1194 + _1199;
   _p2_b0_stencil(0, 0) = _1200;
   int32_t _1201 = _p2_b0_stencil(0, 0);
   int16_t _1202 = _Ix_2_stencil(4, 4);
   int32_t _1203 = (int32_t)(_1202);
   int16_t _1204 = _It_2_stencil(4, 4);
   int32_t _1205 = (int32_t)(_1204);
   int32_t _1206 = _1203 * _1205;
   int32_t _1207 = _1201 + _1206;
   _p2_b0_stencil(0, 0) = _1207;
   _p2_b0_stencil_stream.write(_p2_b0_stencil);
   (void)0;
  } // for _p2_b0_scan_x
 } // for _p2_b0_scan_y
 // dispatch_stream(_p2_b0_stencil_stream, 2, 1, 1, 720, 1, 1, 480, 1, "__auto_insert__hw_output$2", 1, 0, 720, 0, 480);
 hls::stream<PackedStencil<int32_t, 1, 1> > &_p2_b0_stencil_stream_to___auto_insert__hw_output_2 = _p2_b0_stencil_stream;
 (void)0;
 // consume p2:b0.stencil.stream
 hls::stream<PackedStencil<int32_t, 1, 1> > _p2_b1_stencil_stream;
#pragma HLS STREAM variable=_p2_b1_stencil_stream depth=1
#pragma HLS RESOURCE variable=_p2_b1_stencil_stream core=FIFO_SRL

 // produce p2:b1.stencil.stream
 for (int _p2_b1_scan_y = 0; _p2_b1_scan_y < 0 + 480; _p2_b1_scan_y++)
 {
  for (int _p2_b1_scan_x = 0; _p2_b1_scan_x < 0 + 720; _p2_b1_scan_x++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<int16_t, 5, 5> _Iy_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_Iy_2_stencil.value complete dim=0

   _Iy_2_stencil = _Iy_2_stencil_stream_to_p2_b1.read();
   (void)0;
   Stencil<int16_t, 5, 5> _It_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_It_2_stencil.value complete dim=0

   _It_2_stencil = _It_2_stencil_stream_to_p2_b1.read();
   (void)0;
   Stencil<int32_t, 1, 1> _p2_b1_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_b1_stencil.value complete dim=0

   _p2_b1_stencil(0, 0) = 0;
   int32_t _1208 = _p2_b1_stencil(0, 0);
   int16_t _1209 = _Iy_2_stencil(0, 0);
   int32_t _1210 = (int32_t)(_1209);
   int16_t _1211 = _It_2_stencil(0, 0);
   int32_t _1212 = (int32_t)(_1211);
   int32_t _1213 = _1210 * _1212;
   int32_t _1214 = _1208 + _1213;
   _p2_b1_stencil(0, 0) = _1214;
   int32_t _1215 = _p2_b1_stencil(0, 0);
   int16_t _1216 = _Iy_2_stencil(1, 0);
   int32_t _1217 = (int32_t)(_1216);
   int16_t _1218 = _It_2_stencil(1, 0);
   int32_t _1219 = (int32_t)(_1218);
   int32_t _1220 = _1217 * _1219;
   int32_t _1221 = _1215 + _1220;
   _p2_b1_stencil(0, 0) = _1221;
   int32_t _1222 = _p2_b1_stencil(0, 0);
   int16_t _1223 = _Iy_2_stencil(2, 0);
   int32_t _1224 = (int32_t)(_1223);
   int16_t _1225 = _It_2_stencil(2, 0);
   int32_t _1226 = (int32_t)(_1225);
   int32_t _1227 = _1224 * _1226;
   int32_t _1228 = _1222 + _1227;
   _p2_b1_stencil(0, 0) = _1228;
   int32_t _1229 = _p2_b1_stencil(0, 0);
   int16_t _1230 = _Iy_2_stencil(3, 0);
   int32_t _1231 = (int32_t)(_1230);
   int16_t _1232 = _It_2_stencil(3, 0);
   int32_t _1233 = (int32_t)(_1232);
   int32_t _1234 = _1231 * _1233;
   int32_t _1235 = _1229 + _1234;
   _p2_b1_stencil(0, 0) = _1235;
   int32_t _1236 = _p2_b1_stencil(0, 0);
   int16_t _1237 = _Iy_2_stencil(4, 0);
   int32_t _1238 = (int32_t)(_1237);
   int16_t _1239 = _It_2_stencil(4, 0);
   int32_t _1240 = (int32_t)(_1239);
   int32_t _1241 = _1238 * _1240;
   int32_t _1242 = _1236 + _1241;
   _p2_b1_stencil(0, 0) = _1242;
   int32_t _1243 = _p2_b1_stencil(0, 0);
   int16_t _1244 = _Iy_2_stencil(0, 1);
   int32_t _1245 = (int32_t)(_1244);
   int16_t _1246 = _It_2_stencil(0, 1);
   int32_t _1247 = (int32_t)(_1246);
   int32_t _1248 = _1245 * _1247;
   int32_t _1249 = _1243 + _1248;
   _p2_b1_stencil(0, 0) = _1249;
   int32_t _1250 = _p2_b1_stencil(0, 0);
   int16_t _1251 = _Iy_2_stencil(1, 1);
   int32_t _1252 = (int32_t)(_1251);
   int16_t _1253 = _It_2_stencil(1, 1);
   int32_t _1254 = (int32_t)(_1253);
   int32_t _1255 = _1252 * _1254;
   int32_t _1256 = _1250 + _1255;
   _p2_b1_stencil(0, 0) = _1256;
   int32_t _1257 = _p2_b1_stencil(0, 0);
   int16_t _1258 = _Iy_2_stencil(2, 1);
   int32_t _1259 = (int32_t)(_1258);
   int16_t _1260 = _It_2_stencil(2, 1);
   int32_t _1261 = (int32_t)(_1260);
   int32_t _1262 = _1259 * _1261;
   int32_t _1263 = _1257 + _1262;
   _p2_b1_stencil(0, 0) = _1263;
   int32_t _1264 = _p2_b1_stencil(0, 0);
   int16_t _1265 = _Iy_2_stencil(3, 1);
   int32_t _1266 = (int32_t)(_1265);
   int16_t _1267 = _It_2_stencil(3, 1);
   int32_t _1268 = (int32_t)(_1267);
   int32_t _1269 = _1266 * _1268;
   int32_t _1270 = _1264 + _1269;
   _p2_b1_stencil(0, 0) = _1270;
   int32_t _1271 = _p2_b1_stencil(0, 0);
   int16_t _1272 = _Iy_2_stencil(4, 1);
   int32_t _1273 = (int32_t)(_1272);
   int16_t _1274 = _It_2_stencil(4, 1);
   int32_t _1275 = (int32_t)(_1274);
   int32_t _1276 = _1273 * _1275;
   int32_t _1277 = _1271 + _1276;
   _p2_b1_stencil(0, 0) = _1277;
   int32_t _1278 = _p2_b1_stencil(0, 0);
   int16_t _1279 = _Iy_2_stencil(0, 2);
   int32_t _1280 = (int32_t)(_1279);
   int16_t _1281 = _It_2_stencil(0, 2);
   int32_t _1282 = (int32_t)(_1281);
   int32_t _1283 = _1280 * _1282;
   int32_t _1284 = _1278 + _1283;
   _p2_b1_stencil(0, 0) = _1284;
   int32_t _1285 = _p2_b1_stencil(0, 0);
   int16_t _1286 = _Iy_2_stencil(1, 2);
   int32_t _1287 = (int32_t)(_1286);
   int16_t _1288 = _It_2_stencil(1, 2);
   int32_t _1289 = (int32_t)(_1288);
   int32_t _1290 = _1287 * _1289;
   int32_t _1291 = _1285 + _1290;
   _p2_b1_stencil(0, 0) = _1291;
   int32_t _1292 = _p2_b1_stencil(0, 0);
   int16_t _1293 = _Iy_2_stencil(2, 2);
   int32_t _1294 = (int32_t)(_1293);
   int16_t _1295 = _It_2_stencil(2, 2);
   int32_t _1296 = (int32_t)(_1295);
   int32_t _1297 = _1294 * _1296;
   int32_t _1298 = _1292 + _1297;
   _p2_b1_stencil(0, 0) = _1298;
   int32_t _1299 = _p2_b1_stencil(0, 0);
   int16_t _1300 = _Iy_2_stencil(3, 2);
   int32_t _1301 = (int32_t)(_1300);
   int16_t _1302 = _It_2_stencil(3, 2);
   int32_t _1303 = (int32_t)(_1302);
   int32_t _1304 = _1301 * _1303;
   int32_t _1305 = _1299 + _1304;
   _p2_b1_stencil(0, 0) = _1305;
   int32_t _1306 = _p2_b1_stencil(0, 0);
   int16_t _1307 = _Iy_2_stencil(4, 2);
   int32_t _1308 = (int32_t)(_1307);
   int16_t _1309 = _It_2_stencil(4, 2);
   int32_t _1310 = (int32_t)(_1309);
   int32_t _1311 = _1308 * _1310;
   int32_t _1312 = _1306 + _1311;
   _p2_b1_stencil(0, 0) = _1312;
   int32_t _1313 = _p2_b1_stencil(0, 0);
   int16_t _1314 = _Iy_2_stencil(0, 3);
   int32_t _1315 = (int32_t)(_1314);
   int16_t _1316 = _It_2_stencil(0, 3);
   int32_t _1317 = (int32_t)(_1316);
   int32_t _1318 = _1315 * _1317;
   int32_t _1319 = _1313 + _1318;
   _p2_b1_stencil(0, 0) = _1319;
   int32_t _1320 = _p2_b1_stencil(0, 0);
   int16_t _1321 = _Iy_2_stencil(1, 3);
   int32_t _1322 = (int32_t)(_1321);
   int16_t _1323 = _It_2_stencil(1, 3);
   int32_t _1324 = (int32_t)(_1323);
   int32_t _1325 = _1322 * _1324;
   int32_t _1326 = _1320 + _1325;
   _p2_b1_stencil(0, 0) = _1326;
   int32_t _1327 = _p2_b1_stencil(0, 0);
   int16_t _1328 = _Iy_2_stencil(2, 3);
   int32_t _1329 = (int32_t)(_1328);
   int16_t _1330 = _It_2_stencil(2, 3);
   int32_t _1331 = (int32_t)(_1330);
   int32_t _1332 = _1329 * _1331;
   int32_t _1333 = _1327 + _1332;
   _p2_b1_stencil(0, 0) = _1333;
   int32_t _1334 = _p2_b1_stencil(0, 0);
   int16_t _1335 = _Iy_2_stencil(3, 3);
   int32_t _1336 = (int32_t)(_1335);
   int16_t _1337 = _It_2_stencil(3, 3);
   int32_t _1338 = (int32_t)(_1337);
   int32_t _1339 = _1336 * _1338;
   int32_t _1340 = _1334 + _1339;
   _p2_b1_stencil(0, 0) = _1340;
   int32_t _1341 = _p2_b1_stencil(0, 0);
   int16_t _1342 = _Iy_2_stencil(4, 3);
   int32_t _1343 = (int32_t)(_1342);
   int16_t _1344 = _It_2_stencil(4, 3);
   int32_t _1345 = (int32_t)(_1344);
   int32_t _1346 = _1343 * _1345;
   int32_t _1347 = _1341 + _1346;
   _p2_b1_stencil(0, 0) = _1347;
   int32_t _1348 = _p2_b1_stencil(0, 0);
   int16_t _1349 = _Iy_2_stencil(0, 4);
   int32_t _1350 = (int32_t)(_1349);
   int16_t _1351 = _It_2_stencil(0, 4);
   int32_t _1352 = (int32_t)(_1351);
   int32_t _1353 = _1350 * _1352;
   int32_t _1354 = _1348 + _1353;
   _p2_b1_stencil(0, 0) = _1354;
   int32_t _1355 = _p2_b1_stencil(0, 0);
   int16_t _1356 = _Iy_2_stencil(1, 4);
   int32_t _1357 = (int32_t)(_1356);
   int16_t _1358 = _It_2_stencil(1, 4);
   int32_t _1359 = (int32_t)(_1358);
   int32_t _1360 = _1357 * _1359;
   int32_t _1361 = _1355 + _1360;
   _p2_b1_stencil(0, 0) = _1361;
   int32_t _1362 = _p2_b1_stencil(0, 0);
   int16_t _1363 = _Iy_2_stencil(2, 4);
   int32_t _1364 = (int32_t)(_1363);
   int16_t _1365 = _It_2_stencil(2, 4);
   int32_t _1366 = (int32_t)(_1365);
   int32_t _1367 = _1364 * _1366;
   int32_t _1368 = _1362 + _1367;
   _p2_b1_stencil(0, 0) = _1368;
   int32_t _1369 = _p2_b1_stencil(0, 0);
   int16_t _1370 = _Iy_2_stencil(3, 4);
   int32_t _1371 = (int32_t)(_1370);
   int16_t _1372 = _It_2_stencil(3, 4);
   int32_t _1373 = (int32_t)(_1372);
   int32_t _1374 = _1371 * _1373;
   int32_t _1375 = _1369 + _1374;
   _p2_b1_stencil(0, 0) = _1375;
   int32_t _1376 = _p2_b1_stencil(0, 0);
   int16_t _1377 = _Iy_2_stencil(4, 4);
   int32_t _1378 = (int32_t)(_1377);
   int16_t _1379 = _It_2_stencil(4, 4);
   int32_t _1380 = (int32_t)(_1379);
   int32_t _1381 = _1378 * _1380;
   int32_t _1382 = _1376 + _1381;
   _p2_b1_stencil(0, 0) = _1382;
   _p2_b1_stencil_stream.write(_p2_b1_stencil);
   (void)0;
  } // for _p2_b1_scan_x
 } // for _p2_b1_scan_y
 // dispatch_stream(_p2_b1_stencil_stream, 2, 1, 1, 720, 1, 1, 480, 1, "__auto_insert__hw_output$2", 1, 0, 720, 0, 480);
 hls::stream<PackedStencil<int32_t, 1, 1> > &_p2_b1_stencil_stream_to___auto_insert__hw_output_2 = _p2_b1_stencil_stream;
 (void)0;
 // consume p2:b1.stencil.stream
 // produce __auto_insert__hw_output$2.stencil.stream
 for (int _hw_output_2_s0_y_yi = 0; _hw_output_2_s0_y_yi < 0 + 480; _hw_output_2_s0_y_yi++)
 {
  for (int _hw_output_2_s0_x_xi = 0; _hw_output_2_s0_x_xi < 0 + 720; _hw_output_2_s0_x_xi++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<int32_t, 1, 1> _p2_b1_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_b1_stencil.value complete dim=0

   _p2_b1_stencil = _p2_b1_stencil_stream_to___auto_insert__hw_output_2.read();
   (void)0;
   Stencil<int32_t, 1, 1> _p2_b0_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_b0_stencil.value complete dim=0

   _p2_b0_stencil = _p2_b0_stencil_stream_to___auto_insert__hw_output_2.read();
   (void)0;
   Stencil<uint8_t, 1, 1> _gray_4_stencil;
#pragma HLS ARRAY_PARTITION variable=_gray_4_stencil.value complete dim=0

   _gray_4_stencil = _gray_4_stencil_stream_to___auto_insert__hw_output_2.read();
   (void)0;
   Stencil<int32_t, 1, 1> _A11_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_A11_2_stencil.value complete dim=0

   _A11_2_stencil = _A11_2_stencil_stream_to___auto_insert__hw_output_2.read();
   (void)0;
   Stencil<int32_t, 1, 1> _A10_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_A10_2_stencil.value complete dim=0

   _A10_2_stencil = _A10_2_stencil_stream_to___auto_insert__hw_output_2.read();
   (void)0;
   Stencil<int32_t, 1, 1> _A01_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_A01_2_stencil.value complete dim=0

   _A01_2_stencil = _A01_2_stencil_stream_to___auto_insert__hw_output_2.read();
   (void)0;
   Stencil<int32_t, 1, 1> _A00_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_A00_2_stencil.value complete dim=0

   _A00_2_stencil = _A00_2_stencil_stream_to___auto_insert__hw_output_2.read();
   (void)0;
   Stencil<uint8_t, 3, 1, 1> __auto_insert__hw_output_2_stencil;
#pragma HLS ARRAY_PARTITION variable=__auto_insert__hw_output_2_stencil.value complete dim=0

   int32_t _1383 = _A11_2_stencil(0, 0);
   int32_t _1384 = _A01_2_stencil(0, 0);
   int32_t _1385 = _A00_2_stencil(0, 0);
   int32_t _1386 = _1385 * _1383;
   int32_t _1387 = _1384 * _1384;
   int32_t _1388 = _1386 - _1387;
   float _1389 = (float)(_1388);
   bool _1390 = _1389 == float_from_bits(0 /* 0 */);
   uint8_t _1391 = _gray_4_stencil(0, 0);
   uint8_t _1392 = _1391 >> 1;
   uint16_t _1393 = (uint16_t)(_1392);
   float _1394 = float_from_bits(1065353216 /* 1 */) / _1389;
   float _1395 = (float)(_1390 ? float_from_bits(0 /* 0 */) : _1394);
   float _1396 = (float)(_1383);
   float _1397 = _1395 * _1396;
   int32_t _1398 = _p2_b0_stencil(0, 0);
   int32_t _1399 = 0 - _1398;
   float _1400 = (float)(_1399);
   float _1401 = _1397 * _1400;
   float _1402 = float_from_bits(0 /* 0 */) - _1394;
   float _1403 = (float)(_1390 ? float_from_bits(0 /* 0 */) : _1402);
   float _1404 = (float)(_1384);
   float _1405 = _1403 * _1404;
   int32_t _1406 = _p2_b1_stencil(0, 0);
   int32_t _1407 = 0 - _1406;
   float _1408 = (float)(_1407);
   float _1409 = _1405 * _1408;
   float _1410 = _1401 + _1409;
   int32_t _1411 = (int32_t)(_1410);
   int32_t _1412 = 0 - _1411;
   bool _1413 = _1411 > 0;
   int32_t _1414 = (int32_t)(_1413 ? _1411 : _1412);
   uint32_t _1415 = (uint32_t)(_1414);
   uint32_t _1416 = _1415;
   uint32_t _1417 = (uint32_t)(2);
   uint32_t _1418 = _1416 * _1417;
   uint32_t _1419 = (uint32_t)(255);
   uint32_t _1420 = min(_1418, _1419);
   uint8_t _1421 = (uint8_t)(_1420);
   uint16_t _1422 = (uint16_t)(_1421);
   uint16_t _1423 = _1393 + _1422;
   uint16_t _1424 = (uint16_t)(255);
   uint16_t _1425 = min(_1423, _1424);
   uint8_t _1426 = (uint8_t)(_1425);
   __auto_insert__hw_output_2_stencil(0, 0, 0) = _1426;
   int32_t _1427 = _A00_2_stencil(0, 0);
   int32_t _1428 = _A01_2_stencil(0, 0);
   int32_t _1429 = _A11_2_stencil(0, 0);
   int32_t _1430 = _1427 * _1429;
   int32_t _1431 = _1428 * _1428;
   int32_t _1432 = _1430 - _1431;
   float _1433 = (float)(_1432);
   float _1434 = float_from_bits(1065353216 /* 1 */) / _1433;
   bool _1435 = _1433 == float_from_bits(0 /* 0 */);
   float _1436 = (float)(_1435 ? float_from_bits(0 /* 0 */) : _1434);
   uint8_t _1437 = _gray_4_stencil(0, 0);
   uint8_t _1438 = _1437 >> 1;
   uint16_t _1439 = (uint16_t)(_1438);
   float _1440 = float_from_bits(0 /* 0 */) - _1436;
   int32_t _1441 = _A10_2_stencil(0, 0);
   float _1442 = (float)(_1441);
   float _1443 = _1440 * _1442;
   int32_t _1444 = _p2_b0_stencil(0, 0);
   int32_t _1445 = 0 - _1444;
   float _1446 = (float)(_1445);
   float _1447 = _1443 * _1446;
   float _1448 = (float)(_1427);
   float _1449 = _1436 * _1448;
   int32_t _1450 = _p2_b1_stencil(0, 0);
   int32_t _1451 = 0 - _1450;
   float _1452 = (float)(_1451);
   float _1453 = _1449 * _1452;
   float _1454 = _1447 + _1453;
   int32_t _1455 = (int32_t)(_1454);
   int32_t _1456 = 0 - _1455;
   bool _1457 = _1455 > 0;
   int32_t _1458 = (int32_t)(_1457 ? _1455 : _1456);
   uint32_t _1459 = (uint32_t)(_1458);
   uint32_t _1460 = _1459;
   uint32_t _1461 = (uint32_t)(2);
   uint32_t _1462 = _1460 * _1461;
   uint32_t _1463 = (uint32_t)(255);
   uint32_t _1464 = min(_1462, _1463);
   uint8_t _1465 = (uint8_t)(_1464);
   uint16_t _1466 = (uint16_t)(_1465);
   uint16_t _1467 = _1439 + _1466;
   uint16_t _1468 = (uint16_t)(255);
   uint16_t _1469 = min(_1467, _1468);
   uint8_t _1470 = (uint8_t)(_1469);
   __auto_insert__hw_output_2_stencil(1, 0, 0) = _1470;
   int32_t _1471 = _A11_2_stencil(0, 0);
   int32_t _1472 = _A01_2_stencil(0, 0);
   int32_t _1473 = _A00_2_stencil(0, 0);
   int32_t _1474 = _1473 * _1471;
   int32_t _1475 = _1472 * _1472;
   int32_t _1476 = _1474 - _1475;
   float _1477 = (float)(_1476);
   bool _1478 = _1477 == float_from_bits(0 /* 0 */);
   uint8_t _1479 = _gray_4_stencil(0, 0);
   uint8_t _1480 = _1479 >> 1;
   uint16_t _1481 = (uint16_t)(_1480);
   float _1482 = float_from_bits(1065353216 /* 1 */) / _1477;
   float _1483 = (float)(_1478 ? float_from_bits(0 /* 0 */) : _1482);
   float _1484 = (float)(_1471);
   float _1485 = _1483 * _1484;
   int32_t _1486 = _p2_b0_stencil(0, 0);
   int32_t _1487 = 0 - _1486;
   float _1488 = (float)(_1487);
   float _1489 = _1485 * _1488;
   float _1490 = float_from_bits(0 /* 0 */) - _1482;
   float _1491 = (float)(_1478 ? float_from_bits(0 /* 0 */) : _1490);
   float _1492 = (float)(_1472);
   float _1493 = _1491 * _1492;
   int32_t _1494 = _p2_b1_stencil(0, 0);
   int32_t _1495 = 0 - _1494;
   float _1496 = (float)(_1495);
   float _1497 = _1493 * _1496;
   float _1498 = _1489 + _1497;
   int32_t _1499 = (int32_t)(_1498);
   int32_t _1500 = 0 - _1499;
   bool _1501 = _1499 > 0;
   int32_t _1502 = (int32_t)(_1501 ? _1499 : _1500);
   uint32_t _1503 = (uint32_t)(_1502);
   uint32_t _1504 = _1503;
   uint32_t _1505 = (uint32_t)(2);
   uint32_t _1506 = _1504 * _1505;
   uint32_t _1507 = (uint32_t)(255);
   uint32_t _1508 = min(_1506, _1507);
   uint8_t _1509 = (uint8_t)(_1508);
   uint16_t _1510 = (uint16_t)(_1509);
   uint16_t _1511 = _1481 + _1510;
   uint16_t _1512 = (uint16_t)(255);
   uint16_t _1513 = min(_1511, _1512);
   uint8_t _1514 = (uint8_t)(_1513);
   __auto_insert__hw_output_2_stencil(2, 0, 0) = _1514;
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


