#include "hls_target_downsample.h"

#include "Linebuffer.h"
#include "halide_math.h"
void hls_target(
hls::stream<AxiPackedStencil<uint8_t, 1, 1> > &arg_0,
hls::stream<AxiPackedStencil<uint8_t, 2, 1> > &arg_1)
{
#pragma HLS DATAFLOW
#pragma HLS INLINE region
#pragma HLS INTERFACE s_axilite port=return bundle=config
#pragma HLS INTERFACE axis register port=arg_0
#pragma HLS INTERFACE axis register port=arg_1

 // alias the arguments
 hls::stream<AxiPackedStencil<uint8_t, 1, 1> > &__auto_insert__hw_output_2_stencil_stream = arg_0;
 hls::stream<AxiPackedStencil<uint8_t, 2, 1> > &in_stream = arg_1;
 hls::stream<PackedStencil<uint8_t, 2, 2> > _padded_2_stencil_update_stream;


 uint8_t buffer[727*2];

 for (int y = 0; y < 0 + 487*2; y++) {
     for (int x = 0; x < 0 + 727; x++) {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
         Stencil<uint8_t, 2, 1> in_stencil = in_stream.read();

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
 hls::stream<PackedStencil<uint8_t, 1, 1> > _downsample_2_stencil_update_stream;
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
   Stencil<uint8_t, 1, 1> _downsample_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_downsample_2_stencil.value complete dim=0

   uint8_t _105 = _padded_2_stencil(2, 1);
   uint8_t _106 = _padded_2_stencil(1, 2);
   uint8_t _107 = _padded_2_stencil(2, 0);
   uint8_t _108 = _padded_2_stencil(0, 2);
   uint8_t _109 = _padded_2_stencil(2, 2);
   uint8_t _110 = _padded_2_stencil(1, 1);
   uint16_t _111 = (uint16_t)(_110);
   uint8_t _112 = _padded_2_stencil(3, 1);
   uint16_t _113 = (uint16_t)(_112);
   uint16_t _114 = _111 + _113;
   uint8_t _115 = _padded_2_stencil(1, 3);
   uint16_t _116 = (uint16_t)(77);
   uint16_t _117 = _111 * _116;
   uint8_t _118 = _padded_2_stencil(0, 1);
   uint16_t _119 = (uint16_t)(_118);
   uint16_t _120 = (uint16_t)(_105);
   uint16_t _121 = _119 + _120;
   uint8_t _122 = _padded_2_stencil(1, 0);
   uint16_t _123 = (uint16_t)(_122);
   uint16_t _124 = _121 + _123;
   uint16_t _125 = (uint16_t)(_106);
   uint16_t _126 = _124 + _125;
   uint16_t _127 = _126 >> 2;
   uint8_t _128 = (uint8_t)(_127);
   uint16_t _129 = (uint16_t)(_128);
   uint16_t _130 = (uint16_t)(150);
   uint16_t _131 = _129 * _130;
   uint16_t _132 = _117 + _131;
   uint8_t _133 = _padded_2_stencil(0, 0);
   uint16_t _134 = (uint16_t)(_133);
   uint16_t _135 = (uint16_t)(_107);
   uint16_t _136 = _134 + _135;
   uint16_t _137 = (uint16_t)(_108);
   uint16_t _138 = _136 + _137;
   uint16_t _139 = (uint16_t)(_109);
   uint16_t _140 = _138 + _139;
   uint16_t _141 = _140 >> 2;
   uint8_t _142 = (uint8_t)(_141);
   uint16_t _143 = (uint16_t)(_142);
   uint16_t _144 = (uint16_t)(29);
   uint16_t _145 = _143 * _144;
   uint16_t _146 = _132 + _145;
   uint16_t _147 = _146 >> 8;
   uint8_t _148 = (uint8_t)(_147);
   uint16_t _149 = (uint16_t)(_148);
   uint16_t _150 = _114 >> 1;
   uint8_t _151 = (uint8_t)(_150);
   uint16_t _152 = (uint16_t)(_151);
   uint16_t _153 = _152 * _116;
   uint16_t _154 = _120 * _130;
   uint16_t _155 = _153 + _154;
   uint16_t _156 = _135 + _139;
   uint16_t _157 = _156 >> 1;
   uint8_t _158 = (uint8_t)(_157);
   uint16_t _159 = (uint16_t)(_158);
   uint16_t _160 = _159 * _144;
   uint16_t _161 = _155 + _160;
   uint16_t _162 = _161 >> 8;
   uint8_t _163 = (uint8_t)(_162);
   uint16_t _164 = (uint16_t)(_163);
   uint16_t _165 = _149 + _164;
   uint16_t _166 = _165 >> 1;
   uint8_t _167 = (uint8_t)(_166);
   uint16_t _168 = (uint16_t)(_167);
   uint16_t _169 = (uint16_t)(_115);
   uint16_t _170 = _111 + _169;
   uint16_t _171 = _170 >> 1;
   uint8_t _172 = (uint8_t)(_171);
   uint16_t _173 = (uint16_t)(_172);
   uint16_t _174 = _173 * _116;
   uint16_t _175 = _125 * _130;
   uint16_t _176 = _174 + _175;
   uint16_t _177 = _137 + _139;
   uint16_t _178 = _177 >> 1;
   uint8_t _179 = (uint8_t)(_178);
   uint16_t _180 = (uint16_t)(_179);
   uint16_t _181 = _180 * _144;
   uint16_t _182 = _176 + _181;
   uint16_t _183 = _182 >> 8;
   uint8_t _184 = (uint8_t)(_183);
   uint16_t _185 = (uint16_t)(_184);
   uint16_t _186 = _114 + _169;
   uint8_t _187 = _padded_2_stencil(3, 3);
   uint16_t _188 = (uint16_t)(_187);
   uint16_t _189 = _186 + _188;
   uint16_t _190 = _189 >> 2;
   uint8_t _191 = (uint8_t)(_190);
   uint16_t _192 = (uint16_t)(_191);
   uint16_t _193 = _192 * _116;
   uint8_t _194 = _padded_2_stencil(3, 2);
   uint16_t _195 = (uint16_t)(_194);
   uint16_t _196 = _125 + _195;
   uint16_t _197 = _196 + _120;
   uint8_t _198 = _padded_2_stencil(2, 3);
   uint16_t _199 = (uint16_t)(_198);
   uint16_t _200 = _197 + _199;
   uint16_t _201 = _200 >> 2;
   uint8_t _202 = (uint8_t)(_201);
   uint16_t _203 = (uint16_t)(_202);
   uint16_t _204 = _203 * _130;
   uint16_t _205 = _193 + _204;
   uint16_t _206 = _139 * _144;
   uint16_t _207 = _205 + _206;
   uint16_t _208 = _207 >> 8;
   uint8_t _209 = (uint8_t)(_208);
   uint16_t _210 = (uint16_t)(_209);
   uint16_t _211 = _185 + _210;
   uint16_t _212 = _211 >> 1;
   uint8_t _213 = (uint8_t)(_212);
   uint16_t _214 = (uint16_t)(_213);
   uint16_t _215 = _168 + _214;
   uint16_t _216 = _215 >> 1;
   uint8_t _217 = (uint8_t)(_216);
   _downsample_2_stencil(0, 0) = _217;
   _downsample_2_stencil_update_stream.write(_downsample_2_stencil);
   (void)0;
  } // for _downsample_2_scan_update_x
 } // for _downsample_2_scan_update_y
 // consume downsample$2.stencil_update.stream
 hls::stream<PackedStencil<uint8_t, 3, 3> > _downsample_2_stencil_stream;
#pragma HLS STREAM variable=_downsample_2_stencil_stream depth=1
#pragma HLS RESOURCE variable=_downsample_2_stencil_stream core=FIFO_SRL

 // produce downsample$2.stencil.stream
 linebuffer<726, 486>(_downsample_2_stencil_update_stream, _downsample_2_stencil_stream);
 (void)0;
 // dispatch_stream(_downsample_2_stencil_stream, 2, 3, 1, 726, 3, 1, 486, 2, "p2:grad_x", 1, 0, 726, 0, 486, "p2:grad_y", 1, 0, 726, 0, 486);
 hls::stream<PackedStencil<uint8_t, 3, 3> > _downsample_2_stencil_stream_to_p2_grad_x;
#pragma HLS STREAM variable=_downsample_2_stencil_stream_to_p2_grad_x depth=1
#pragma HLS RESOURCE variable=_downsample_2_stencil_stream_to_p2_grad_x core=FIFO_SRL

 hls::stream<PackedStencil<uint8_t, 3, 3> > _downsample_2_stencil_stream_to_p2_grad_y;
#pragma HLS STREAM variable=_downsample_2_stencil_stream_to_p2_grad_y depth=1
#pragma HLS RESOURCE variable=_downsample_2_stencil_stream_to_p2_grad_y core=FIFO_SRL

 for (int _dim_1 = 0; _dim_1 <= 483; _dim_1 += 1)
 for (int _dim_0 = 0; _dim_0 <= 723; _dim_0 += 1)
 {
#pragma HLS PIPELINE
  Stencil<uint8_t, 3, 3> _tmp_stencil = _downsample_2_stencil_stream.read();
#pragma HLS ARRAY_PARTITION variable=_tmp_stencil.value complete dim=0

  if (_dim_0 >= 0 && _dim_0 <= 723 && _dim_1 >= 0 && _dim_1 <= 483)
  {
   _downsample_2_stencil_stream_to_p2_grad_x.write(_tmp_stencil);
  }
  if (_dim_0 >= 0 && _dim_0 <= 723 && _dim_1 >= 0 && _dim_1 <= 483)
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
   Stencil<uint8_t, 3, 3> _downsample_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_downsample_2_stencil.value complete dim=0

   _downsample_2_stencil = _downsample_2_stencil_stream_to_p2_grad_x.read();
   (void)0;
   Stencil<int16_t, 1, 1> _p2_grad_x_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_x_stencil.value complete dim=0

   uint8_t _218 = _downsample_2_stencil(2, 0);
   int16_t _219 = (int16_t)(_218);
   uint8_t _220 = _downsample_2_stencil(0, 0);
   int16_t _221 = (int16_t)(_220);
   int16_t _222 = _219 - _221;
   uint8_t _223 = _downsample_2_stencil(0, 1);
   int16_t _224 = (int16_t)(_223);
   int16_t _225 = (int16_t)(2);
   int16_t _226 = _224 * _225;
   int16_t _227 = _222 - _226;
   uint8_t _228 = _downsample_2_stencil(2, 1);
   int16_t _229 = (int16_t)(_228);
   int16_t _230 = _229 * _225;
   int16_t _231 = _227 + _230;
   uint8_t _232 = _downsample_2_stencil(0, 2);
   int16_t _233 = (int16_t)(_232);
   int16_t _234 = _231 - _233;
   uint8_t _235 = _downsample_2_stencil(2, 2);
   int16_t _236 = (int16_t)(_235);
   int16_t _237 = _234 + _236;
   _p2_grad_x_stencil(0, 0) = _237;
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

   int16_t _238 = _p2_grad_x_stencil(0, 0);
   int32_t _239 = (int32_t)(_238);
   int32_t _240 = _239 * _239;
   _p2_grad_xx_stencil(0, 0) = _240;
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
   int32_t _241 = _p2_grad_gx_stencil(0, 0);
   int32_t _242 = _p2_grad_xx_stencil(0, 0);
   int32_t _243 = _241 + _242;
   _p2_grad_gx_stencil(0, 0) = _243;
   int32_t _244 = _p2_grad_gx_stencil(0, 0);
   int32_t _245 = _p2_grad_xx_stencil(1, 0);
   int32_t _246 = _244 + _245;
   _p2_grad_gx_stencil(0, 0) = _246;
   int32_t _247 = _p2_grad_gx_stencil(0, 0);
   int32_t _248 = _p2_grad_xx_stencil(2, 0);
   int32_t _249 = _247 + _248;
   _p2_grad_gx_stencil(0, 0) = _249;
   int32_t _250 = _p2_grad_gx_stencil(0, 0);
   int32_t _251 = _p2_grad_xx_stencil(0, 1);
   int32_t _252 = _250 + _251;
   _p2_grad_gx_stencil(0, 0) = _252;
   int32_t _253 = _p2_grad_gx_stencil(0, 0);
   int32_t _254 = _p2_grad_xx_stencil(1, 1);
   int32_t _255 = _253 + _254;
   _p2_grad_gx_stencil(0, 0) = _255;
   int32_t _256 = _p2_grad_gx_stencil(0, 0);
   int32_t _257 = _p2_grad_xx_stencil(2, 1);
   int32_t _258 = _256 + _257;
   _p2_grad_gx_stencil(0, 0) = _258;
   int32_t _259 = _p2_grad_gx_stencil(0, 0);
   int32_t _260 = _p2_grad_xx_stencil(0, 2);
   int32_t _261 = _259 + _260;
   _p2_grad_gx_stencil(0, 0) = _261;
   int32_t _262 = _p2_grad_gx_stencil(0, 0);
   int32_t _263 = _p2_grad_xx_stencil(1, 2);
   int32_t _264 = _262 + _263;
   _p2_grad_gx_stencil(0, 0) = _264;
   int32_t _265 = _p2_grad_gx_stencil(0, 0);
   int32_t _266 = _p2_grad_xx_stencil(2, 2);
   int32_t _267 = _265 + _266;
   _p2_grad_gx_stencil(0, 0) = _267;
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
   Stencil<uint8_t, 3, 3> _downsample_2_stencil;
#pragma HLS ARRAY_PARTITION variable=_downsample_2_stencil.value complete dim=0

   _downsample_2_stencil = _downsample_2_stencil_stream_to_p2_grad_y.read();
   (void)0;
   Stencil<int16_t, 1, 1> _p2_grad_y_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_grad_y_stencil.value complete dim=0

   uint8_t _268 = _downsample_2_stencil(0, 2);
   int16_t _269 = (int16_t)(_268);
   uint8_t _270 = _downsample_2_stencil(0, 0);
   int16_t _271 = (int16_t)(_270);
   int16_t _272 = _269 - _271;
   uint8_t _273 = _downsample_2_stencil(1, 2);
   int16_t _274 = (int16_t)(_273);
   int16_t _275 = (int16_t)(2);
   int16_t _276 = _274 * _275;
   int16_t _277 = _272 + _276;
   uint8_t _278 = _downsample_2_stencil(1, 0);
   int16_t _279 = (int16_t)(_278);
   int16_t _280 = _279 * _275;
   int16_t _281 = _277 - _280;
   uint8_t _282 = _downsample_2_stencil(2, 2);
   int16_t _283 = (int16_t)(_282);
   int16_t _284 = _281 + _283;
   uint8_t _285 = _downsample_2_stencil(2, 0);
   int16_t _286 = (int16_t)(_285);
   int16_t _287 = _284 - _286;
   _p2_grad_y_stencil(0, 0) = _287;
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

   int16_t _288 = _p2_grad_x_stencil(0, 0);
   int32_t _289 = (int32_t)(_288);
   int16_t _290 = _p2_grad_y_stencil(0, 0);
   int32_t _291 = (int32_t)(_290);
   int32_t _292 = _289 * _291;
   _p2_grad_xy_stencil(0, 0) = _292;
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
   int32_t _293 = _p2_grad_gxy_stencil(0, 0);
   int32_t _294 = _p2_grad_xy_stencil(0, 0);
   int32_t _295 = _293 + _294;
   _p2_grad_gxy_stencil(0, 0) = _295;
   int32_t _296 = _p2_grad_gxy_stencil(0, 0);
   int32_t _297 = _p2_grad_xy_stencil(1, 0);
   int32_t _298 = _296 + _297;
   _p2_grad_gxy_stencil(0, 0) = _298;
   int32_t _299 = _p2_grad_gxy_stencil(0, 0);
   int32_t _300 = _p2_grad_xy_stencil(2, 0);
   int32_t _301 = _299 + _300;
   _p2_grad_gxy_stencil(0, 0) = _301;
   int32_t _302 = _p2_grad_gxy_stencil(0, 0);
   int32_t _303 = _p2_grad_xy_stencil(0, 1);
   int32_t _304 = _302 + _303;
   _p2_grad_gxy_stencil(0, 0) = _304;
   int32_t _305 = _p2_grad_gxy_stencil(0, 0);
   int32_t _306 = _p2_grad_xy_stencil(1, 1);
   int32_t _307 = _305 + _306;
   _p2_grad_gxy_stencil(0, 0) = _307;
   int32_t _308 = _p2_grad_gxy_stencil(0, 0);
   int32_t _309 = _p2_grad_xy_stencil(2, 1);
   int32_t _310 = _308 + _309;
   _p2_grad_gxy_stencil(0, 0) = _310;
   int32_t _311 = _p2_grad_gxy_stencil(0, 0);
   int32_t _312 = _p2_grad_xy_stencil(0, 2);
   int32_t _313 = _311 + _312;
   _p2_grad_gxy_stencil(0, 0) = _313;
   int32_t _314 = _p2_grad_gxy_stencil(0, 0);
   int32_t _315 = _p2_grad_xy_stencil(1, 2);
   int32_t _316 = _314 + _315;
   _p2_grad_gxy_stencil(0, 0) = _316;
   int32_t _317 = _p2_grad_gxy_stencil(0, 0);
   int32_t _318 = _p2_grad_xy_stencil(2, 2);
   int32_t _319 = _317 + _318;
   _p2_grad_gxy_stencil(0, 0) = _319;
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

   int16_t _320 = _p2_grad_y_stencil(0, 0);
   int32_t _321 = (int32_t)(_320);
   int32_t _322 = _321 * _321;
   _p2_grad_yy_stencil(0, 0) = _322;
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
   int32_t _323 = _p2_grad_gy_stencil(0, 0);
   int32_t _324 = _p2_grad_yy_stencil(0, 0);
   int32_t _325 = _323 + _324;
   _p2_grad_gy_stencil(0, 0) = _325;
   int32_t _326 = _p2_grad_gy_stencil(0, 0);
   int32_t _327 = _p2_grad_yy_stencil(1, 0);
   int32_t _328 = _326 + _327;
   _p2_grad_gy_stencil(0, 0) = _328;
   int32_t _329 = _p2_grad_gy_stencil(0, 0);
   int32_t _330 = _p2_grad_yy_stencil(2, 0);
   int32_t _331 = _329 + _330;
   _p2_grad_gy_stencil(0, 0) = _331;
   int32_t _332 = _p2_grad_gy_stencil(0, 0);
   int32_t _333 = _p2_grad_yy_stencil(0, 1);
   int32_t _334 = _332 + _333;
   _p2_grad_gy_stencil(0, 0) = _334;
   int32_t _335 = _p2_grad_gy_stencil(0, 0);
   int32_t _336 = _p2_grad_yy_stencil(1, 1);
   int32_t _337 = _335 + _336;
   _p2_grad_gy_stencil(0, 0) = _337;
   int32_t _338 = _p2_grad_gy_stencil(0, 0);
   int32_t _339 = _p2_grad_yy_stencil(2, 1);
   int32_t _340 = _338 + _339;
   _p2_grad_gy_stencil(0, 0) = _340;
   int32_t _341 = _p2_grad_gy_stencil(0, 0);
   int32_t _342 = _p2_grad_yy_stencil(0, 2);
   int32_t _343 = _341 + _342;
   _p2_grad_gy_stencil(0, 0) = _343;
   int32_t _344 = _p2_grad_gy_stencil(0, 0);
   int32_t _345 = _p2_grad_yy_stencil(1, 2);
   int32_t _346 = _344 + _345;
   _p2_grad_gy_stencil(0, 0) = _346;
   int32_t _347 = _p2_grad_gy_stencil(0, 0);
   int32_t _348 = _p2_grad_yy_stencil(2, 2);
   int32_t _349 = _347 + _348;
   _p2_grad_gy_stencil(0, 0) = _349;
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

   int32_t _350 = _p2_grad_gx_stencil(0, 0);
   int32_t _351 = _p2_grad_gy_stencil(0, 0);
   int32_t _352 = _p2_grad_gxy_stencil(0, 0);
   int32_t _353 = _350 / 144;
   int32_t _354 = _350 - _353 * 144;
   int32_t _355 = 144 >> (int32_t)31;
   int32_t _356 = _354 >> (int32_t)31;
   int32_t _357 = _353 - (_356 & _355) + (_356 & ~_355);
   float _358 = (float)(_357);
   int32_t _359 = _351 / 144;
   int32_t _360 = _351 - _359 * 144;
   int32_t _361 = _360 >> (int32_t)31;
   int32_t _362 = _359 - (_361 & _355) + (_361 & ~_355);
   float _363 = (float)(_362);
   float _364 = _358 + _363;
   float _365 = _358 * _363;
   int32_t _366 = _352 / 144;
   int32_t _367 = _352 - _366 * 144;
   int32_t _368 = _367 >> (int32_t)31;
   int32_t _369 = _366 - (_368 & _355) + (_368 & ~_355);
   float _370 = (float)(_369);
   float _371 = _370 * _370;
   float _372 = _365 - _371;
   float _373 = _364 * float_from_bits(1025758986 /* 0.04 */);
   float _374 = _373 * _364;
   float _375 = _372 - _374;
   _p2_cim_stencil(0, 0) = _375;
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
 // dispatch_stream(_p2_cim_stencil_stream, 2, 3, 1, 722, 3, 1, 482, 1, "__auto_insert__hw_output$2", 1, 0, 722, 0, 482);
 hls::stream<PackedStencil<float, 3, 3> > &_p2_cim_stencil_stream_to___auto_insert__hw_output_2 = _p2_cim_stencil_stream;
 (void)0;
 // consume p2:cim.stencil.stream
 // produce __auto_insert__hw_output$2.stencil.stream
 for (int _hw_output_2_s0_y_yi = 0; _hw_output_2_s0_y_yi < 0 + 480; _hw_output_2_s0_y_yi++)
 {
  for (int _hw_output_2_s0_x_xi = 0; _hw_output_2_s0_x_xi < 0 + 720; _hw_output_2_s0_x_xi++)
  {
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=1
   Stencil<float, 3, 3> _p2_cim_stencil;
#pragma HLS ARRAY_PARTITION variable=_p2_cim_stencil.value complete dim=0

   _p2_cim_stencil = _p2_cim_stencil_stream_to___auto_insert__hw_output_2.read();
   (void)0;
   Stencil<uint8_t, 1, 1> __auto_insert__hw_output_2_stencil;
#pragma HLS ARRAY_PARTITION variable=__auto_insert__hw_output_2_stencil.value complete dim=0

   float _376 = _p2_cim_stencil(1, 1);
   uint8_t _377 = (uint8_t)(255);
   uint8_t _378 = (uint8_t)(0);
   float _379 = _p2_cim_stencil(0, 0);
   float _380 = _p2_cim_stencil(1, 0);
   float _381 = max(_379, _380);
   float _382 = _p2_cim_stencil(2, 0);
   float _383 = max(_381, _382);
   float _384 = _p2_cim_stencil(0, 1);
   float _385 = max(_383, _384);
   float _386 = _p2_cim_stencil(2, 1);
   float _387 = max(_385, _386);
   float _388 = _p2_cim_stencil(0, 2);
   float _389 = max(_387, _388);
   float _390 = _p2_cim_stencil(1, 2);
   float _391 = max(_389, _390);
   float _392 = _p2_cim_stencil(2, 2);
   float _393 = max(_391, _392);
   bool _394 = _393 < _376;
   bool _395 = float_from_bits(1120403456 /* 100 */) <= _376;
   bool _396 = _394 && _395;
   uint8_t _397 = (uint8_t)(_396 ? _377 : _378);
   __auto_insert__hw_output_2_stencil(0, 0) = _397;
   AxiPackedStencil<uint8_t, 1, 1>__auto_insert__hw_output_2_stencil_packed = __auto_insert__hw_output_2_stencil;
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


