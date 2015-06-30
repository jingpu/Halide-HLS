#ifndef STENCIL_H
#define STENCIL_H

#include <hls_stream.h>

using hls::stream;


template <typename T, unsigned STENCIL_HEIGHT>
class UpdateStencil{
public:
    T p[STENCIL_HEIGHT][1];
};


template <typename T, unsigned STENCIL_WIDTH, unsigned STENCIL_HEIGHT>
class Stencil{
public:
    T p[STENCIL_HEIGHT][STENCIL_WIDTH];

    static const int HALF_STENCIL_WIDTH = STENCIL_WIDTH/2;
    static const int HALF_STENCIL_HEIGHT = STENCIL_HEIGHT/2;

    inline void shiftAndUpdate(const UpdateStencil<T, STENCIL_HEIGHT> & in) {
#pragma HLS INLINE

	for (int col = 0; col < STENCIL_WIDTH - 1; col++)
#pragma HLS UNROLL
	    for (int row = 0; row < STENCIL_HEIGHT; row++)
#pragma HLS UNROLL
		p[row][col] = p[row][col+1];

	for (int row = 0; row < STENCIL_HEIGHT; row++)
#pragma HLS UNROLL
	    p[row][STENCIL_WIDTH - 1] = in.p[row][0];
    }

    template<unsigned IMG_WEIGHT, unsigned IMG_HEIGHT>
    static void LineBuffer( stream< T > &in_stream,
			    stream< UpdateStencil<T, STENCIL_HEIGHT> > &out_stream );
};



template <typename T, unsigned STENCIL_WIDTH, unsigned STENCIL_HEIGHT>
template<unsigned IMG_WIDTH, unsigned IMG_HEIGHT>
void Stencil<T, STENCIL_WIDTH, STENCIL_HEIGHT>::LineBuffer(  stream< T > &in_stream,
							     stream< UpdateStencil<T, STENCIL_HEIGHT> > &out_stream) {
    T LB[STENCIL_HEIGHT][IMG_WIDTH + HALF_STENCIL_WIDTH*2];

#pragma HLS ARRAY_PARTITION variable=LB complete dim=1
#pragma HLS DATA_PACK variable=LB

    //
    // initialize line buffer
    //

    for(int i = 0; i < HALF_STENCIL_HEIGHT*2; i++){
	for(int j = 0; j < IMG_WIDTH + HALF_STENCIL_WIDTH*2; j++){
#pragma HLS PIPELINE
	    LB[i][j] = in_stream.read();
	}
    }


    //
    // fetch new pixel and enqueue update_buffer
    //

    int write_x = HALF_STENCIL_HEIGHT * 2;   // the row index of the fetched pixel in LB
    for(int i = HALF_STENCIL_HEIGHT; i < IMG_HEIGHT + HALF_STENCIL_HEIGHT; i++){ 
	if (write_x >= STENCIL_HEIGHT)
	    write_x -= STENCIL_HEIGHT;
	for(int j = 0; j < IMG_WIDTH + HALF_STENCIL_WIDTH*2; j++){
#pragma HLS PIPELINE
	    // [BLOCKING OP] fetch pixel from the input image
	    LB[write_x][j] = in_stream.read();

	    // [BLOCKING OP] enqueue update_buffer
	    UpdateStencil<T, STENCIL_HEIGHT> buffer, buffer_preshuffled;
#pragma HLS DATA_PACK variable=buffer_preshuffled
#pragma HLS DATA_PACK variable=buffer

	    int xx = write_x + 1;   // the row index of the first pixel of the update stencil w.r.t. LB
	    int yy = j;   // the col index of the first pixel of the update stencil w.r.t. LB

	    for (int k = 0; k < STENCIL_HEIGHT; k++)
#pragma HLS UNROLL
		buffer_preshuffled.p[k][0] = LB[k][yy];

	    for (int k = 0; k < STENCIL_HEIGHT; k++){
#pragma HLS UNROLL
		int xk = xx + k; // the row index of the current pixel of the update stencil w.r.t. LB
		if (xk >= STENCIL_HEIGHT)
		    xk -= STENCIL_HEIGHT;
		buffer.p[k][0] = buffer_preshuffled.p[xk][0]; // FIXME: remove modular op
	    }

	    out_stream.write(buffer);
	}
	write_x++;
    }
}

#endif
