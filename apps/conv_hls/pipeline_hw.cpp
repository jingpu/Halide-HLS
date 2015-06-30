#include "pipeline_hw.h"

#include "stencil.h"
using hls::stream;

const unsigned IMG_WIDTH = 64;
const unsigned IMG_HEIGHT = 256;

void convolve55_stream(hls::stream<uint8_t>& input, hls::stream<uint8_t>& output)
{
#pragma HLS DATAFLOW
#pragma HLS DATA_PACK variable=input
#pragma HLS DATA_PACK variable=output

    stream< UpdateStencil<uint8_t, 5> > LB_to_stencil;

    //#pragma HLS DATA_PACK variable=input_stream
    //#pragma HLS DATA_PACK variable=LB_to_stencil

    uint16_t coef[5][5] = {
	{1,     3,     6,     3,     1},
	{3,    15,    25,    15,     3},
	{6,    25,    44,    25,     6},
	{3,    15,    25,    15,     3},
	{1,     3,     6,     3,     1}
    };
#pragma HLS ARRAY_PARTITION variable=coef complete dim=0

    // instantiate a line buffer
    Stencil<uint8_t, 5, 5>::LineBuffer<IMG_WIDTH, IMG_HEIGHT>(input, LB_to_stencil);

    // do convolution
    Stencil<uint8_t, 5, 5> stencil;
#pragma HLS ARRAY_PARTITION variable=stencil complete dim=0

    for(int i = 0; i < IMG_HEIGHT; i++) {// i and j is the indices of the output image
	// [BLOCKING OP] initialize stencil -- get 4 cols of update stencils
	for(int j = 0; j < 4; j++){
	    UpdateStencil<uint8_t, 5> buffer = LB_to_stencil.read();
	    stencil.shiftAndUpdate(buffer);
	}
    Stencil_Op:for(int j = 0; j < IMG_WIDTH; j++){
#pragma HLS PIPELINE
	    // [BLOCKING OP] do one convolution per update stencil
	    UpdateStencil<uint8_t, 5> buffer = LB_to_stencil.read();
	    stencil.shiftAndUpdate(buffer);

	    uint16_t val = 0;
	    // apply gaussian 2d filter over channel k
	Row_Loop: for(int l = 0; l < 5; l++)
#pragma HLS UNROLL
	    Col_Loop: for(int m = 0; m < 5; m++)
#pragma HLS UNROLL
		    val += stencil.p[l][m] * coef[l][m];

	    uint8_t output_pixel = val >> 8; // back to 8bit
	    output.write(output_pixel);
	}
    }


}
