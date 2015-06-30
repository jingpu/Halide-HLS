#ifndef STENCIL_H
#define STENCIL_H

#include <hls_stream.h>
#include "helper.h"
using hls::stream;


template <typename T, unsigned n>
class UpdateStencil{
public:
	T p[n][1];
};


template <typename T, unsigned n>
class Stencil{
public:
	T p[n][n];

	static const int STENCIL_WIDTH = n;
	static const int STENCIL_HEIGHT = n;
	static const int HALF_STENCIL_WIDTH = STENCIL_WIDTH/2;
	static const int HALF_STENCIL_HEIGHT = STENCIL_HEIGHT/2;

	inline void shiftAndUpdate(const UpdateStencil<T, n> & in) {
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

	static void LineBuffer( stream< T > &in_stream,
			stream< UpdateStencil<T, n> > &out_stream );
};



template <typename T, unsigned n>
void Stencil<T, n>::LineBuffer(  stream< T > &in_stream,
		stream< UpdateStencil<T, n> > &out_stream) {

	T LB[STENCIL_HEIGHT][IMG_WIDTH + HALF_STENCIL_WIDTH*2];
#pragma HLS ARRAY_PARTITION variable=LB complete dim=1
#pragma HLS DATA_PACK variable=LB


	//
	// initialize line buffer
	//

	for(int i = -HALF_STENCIL_HEIGHT; i < HALF_STENCIL_HEIGHT; i++){  // i and j is the indices of the input image
		int write_x = i + HALF_STENCIL_HEIGHT;   // the row index of the fetched pixel in LB
		for(int j = -HALF_STENCIL_WIDTH; j < IMG_WIDTH + HALF_STENCIL_WIDTH; j++){
#pragma HLS PIPELINE
			int write_y = j + HALF_STENCIL_WIDTH;   // the col index of the fetched pixel in LB
			if ( i < 0 || i >= IMG_HEIGHT || j <0 || j >= IMG_WIDTH) { // if the position is outside of the input image
				// fill zeros
				LB[write_x][write_y] = T(0);
			} else {
				//  [BLOCKING OP] fetch pixel from the input image
				LB[write_x][write_y] = in_stream.read();
			}
		}
	}


	//
	// fetch new pixel and enqueue update_buffer
	//

	int write_x = HALF_STENCIL_HEIGHT * 2;   // the row index of the fetched pixel in LB
	for(int i = HALF_STENCIL_HEIGHT; i < IMG_HEIGHT + HALF_STENCIL_HEIGHT; i++){  // i and j is the indices of the input image
		if (write_x >= STENCIL_HEIGHT)
			write_x -= STENCIL_HEIGHT;
		for(int j = -HALF_STENCIL_WIDTH; j < IMG_WIDTH + HALF_STENCIL_WIDTH; j++){
#pragma HLS PIPELINE
			int write_y = j + HALF_STENCIL_WIDTH;   // the col index of the fetched pixel in LB
			if ( i >= IMG_HEIGHT || j <0 || j >= IMG_WIDTH) { // if the position is outside of the input image
				// fill zeros
				LB[write_x][write_y] = T(0);
			} else {
				// [BLOCKING OP] fetch pixel from the input image
				LB[write_x][write_y] = in_stream.read();
			}

			// [BLOCKING OP] enqueue update_buffer
			UpdateStencil<T, n> buffer, buffer_preshuffled;
#pragma HLS DATA_PACK variable=buffer_preshuffled
#pragma HLS DATA_PACK variable=buffer

			// #pragma scalarize buffer_preshuffled yes
			int xx = write_x + 1;   // the row index of the first pixel of the update stencil w.r.t. LB
			int yy = write_y;   // the col index of the first pixel of the update stencil w.r.t. LB

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

			// put(): Waits for stream to be empty, and then queues data.
			out_stream.write(buffer);
		}
		write_x++;
	}
}

#endif
