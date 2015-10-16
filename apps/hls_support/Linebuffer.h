#ifndef LINEBUFFER_H
#define LINEBUFFER_H

#include "Stencil.h"

#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <hls_stream.h>

using hls::stream;

template <size_t IMG_EXTENT_0, size_t EXTENT_1, size_t EXTENT_2, size_t EXTENT_3,
	  size_t IN_EXTENT_0,  size_t OUT_EXTENT_0, typename T>
void linebuffer_1D(stream<PackedStencil<T, IN_EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> > &in_stream,
		   stream<PackedStencil<T, OUT_EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> > &out_stream) {
    static_assert(IMG_EXTENT_0 > OUT_EXTENT_0, "image extent not is larger than output.");
    static_assert(OUT_EXTENT_0 > IN_EXTENT_0, "input extent is larger than output."); // TODO handle this situation.
    static_assert(IMG_EXTENT_0 % IN_EXTENT_0 == 0, "image extent is not divisible by input."); // TODO handle this situation.
    static_assert(OUT_EXTENT_0 % IN_EXTENT_0 == 0, "output extent is not divisible by input."); // TODO handle this situation.
#pragma HLS INLINE

    Stencil<T, OUT_EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> buffer;
#pragma HLS ARRAY_PARTITION variable=buffer.value complete dim=0

    // initialize buffer
    for(size_t i = 0; i <= OUT_EXTENT_0 - IN_EXTENT_0*2; i += IN_EXTENT_0) {
#pragma HLS PIPELINE
	Stencil<T, IN_EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> in_stencil = in_stream.read();
	buffer.write_stencil_at(in_stencil, i);
    }

    // produce one output per input
    const size_t NUM_OF_OUTPUT_0 = (IMG_EXTENT_0 - OUT_EXTENT_0) / IN_EXTENT_0 + 1;
    for(size_t n = 0; n < NUM_OF_OUTPUT_0; n++) {
#pragma HLS PIPELINE
	Stencil<T, IN_EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> in_stencil = in_stream.read();

	buffer.write_stencil_at(in_stencil, OUT_EXTENT_0 - IN_EXTENT_0);
	out_stream.write(buffer);

	// shift the buffer elements by IN_EXTENT
	// TODO coalesce the loads and stores on dim 1, dim 2 and dim 3
	for(size_t i = 0; i < OUT_EXTENT_0 - IN_EXTENT_0; i++)
	    for(size_t st_idx_1 = 0; st_idx_1 < EXTENT_1; st_idx_1++)
		for(size_t st_idx_2 = 0; st_idx_2 < EXTENT_2; st_idx_2++)
		    for(size_t st_idx_3 = 0; st_idx_3 < EXTENT_3; st_idx_3++)
		    buffer(i, st_idx_1, st_idx_2, st_idx_3) =
			buffer(i + IN_EXTENT_0, st_idx_1, st_idx_2, st_idx_3);
    }
}

// An overloaded (trivial) 1D line buffer, where all the input dimensions and
// the output dimensions are the same size
template <size_t IMG_EXTENT_0,
          size_t EXTENT_0, size_t EXTENT_1, size_t EXTENT_2, size_t EXTENT_3, typename T>
void linebuffer_1D(stream<PackedStencil<T, EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> > &in_stream,
                   stream<PackedStencil<T, EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> > &out_stream) {
#pragma HLS INLINE
    for(size_t idx_0 = 0; idx_0 < IMG_EXTENT_0; idx_0 += EXTENT_0) {
#pragma HLS PIPELINE
        out_stream.write(in_stream.read());
    }
}




template <size_t IMG_EXTENT_0, size_t IMG_EXTENT_1, size_t EXTENT_2, size_t EXTENT_3,
	  size_t IN_EXTENT_0, size_t IN_EXTENT_1,
	  size_t OUT_EXTENT_1, typename T>
void linebuffer_2D_col(stream<PackedStencil<T, IN_EXTENT_0, IN_EXTENT_1, EXTENT_2, EXTENT_3> > &in_stream,
		       stream<PackedStencil<T, IN_EXTENT_0, OUT_EXTENT_1, EXTENT_2, EXTENT_3> > &out_stream) {

    Stencil<T, IMG_EXTENT_0, OUT_EXTENT_1, EXTENT_2, EXTENT_3> linebuffer;
#pragma HLS ARRAY_PARTITION variable=linebuffer.value complete dim=1
#pragma HLS ARRAY_PARTITION variable=linebuffer.value complete dim=2
#pragma HLS ARRAY_PARTITION variable=linebuffer.value complete dim=3

    // initialize linebuffer, i.e. fill first (OUT_EXTENT_1 - IN_EXTENT_1) lines
    size_t idx_1 = 0;  // the line index of coming stencil in the linebuffer
    while(idx_1 <= OUT_EXTENT_1 - IN_EXTENT_1*2) {
	for(size_t idx_0 = 0; idx_0 < IMG_EXTENT_0; idx_0 += IN_EXTENT_0)  {
#pragma HLS PIPELINE
	    Stencil<T, IN_EXTENT_0, IN_EXTENT_1, EXTENT_2, EXTENT_3> in_stencil = in_stream.read();
	    linebuffer.write_stencil_at(in_stencil, idx_0, idx_1);
	}
	idx_1 += IN_EXTENT_1;
    }

    // produce a [IN_EXTENT_0, OUT_EXTENT_1] column stencil per input
    const size_t NUM_OF_OUTPUT_1 = (IMG_EXTENT_1 - OUT_EXTENT_1) / IN_EXTENT_1 + 1;
    for (size_t n1 = 0; n1 < NUM_OF_OUTPUT_1; n1++) {
#pragma HLS loop_flatten off
	if (idx_1 >= OUT_EXTENT_1)
	    idx_1 -= OUT_EXTENT_1;

	for(size_t idx_0 = 0; idx_0 < IMG_EXTENT_0; idx_0 += IN_EXTENT_0)  {
#pragma HLS PIPELINE
	    Stencil<T, IN_EXTENT_0, IN_EXTENT_1, EXTENT_2, EXTENT_3> in_stencil = in_stream.read();
	    linebuffer.write_stencil_at(in_stencil, idx_0, idx_1);

	    Stencil<T, IN_EXTENT_0, OUT_EXTENT_1, EXTENT_2, EXTENT_3> col_buf, col_buf_preshuffled;
#pragma HLS ARRAY_PARTITION variable=col_buf.value complete dim=3
#pragma HLS ARRAY_PARTITION variable=col_buf_preshuffled.value complete dim=3

	    linebuffer.read_stencil_at(col_buf_preshuffled, idx_0, 0);

	    // shuffle the buffer
	    // TODO coalesce the loads and stores on dim 2 and dim 3
	    for(size_t st_idx_3 = 0; st_idx_3 < EXTENT_3; st_idx_3++)
		for(size_t st_idx_2 = 0; st_idx_2 < EXTENT_2; st_idx_2++)
		    for(size_t st_idx_1 = 0; st_idx_1 < OUT_EXTENT_1; st_idx_1++) {
		    // the line index of the current output pixel in the linebuffer
			size_t st_idx_1_in_pre = st_idx_1 + idx_1 + IN_EXTENT_1;
			if(st_idx_1_in_pre >= OUT_EXTENT_1)
			    st_idx_1_in_pre -= OUT_EXTENT_1;

			for(size_t st_idx_0 = 0; st_idx_0 < IN_EXTENT_0; st_idx_0++)
			    col_buf(st_idx_0, st_idx_1, st_idx_2, st_idx_3) =
				col_buf_preshuffled(st_idx_0, st_idx_1_in_pre, st_idx_2, st_idx_3);
		}
	    out_stream.write(col_buf);
	}
	idx_1 += IN_EXTENT_1;  // increase the line address
    }
}


template <size_t IMG_EXTENT_0, size_t IMG_EXTENT_1, size_t EXTENT_2, size_t EXTENT_3,
	  size_t IN_EXTENT_0, size_t IN_EXTENT_1,
	  size_t OUT_EXTENT_0, size_t OUT_EXTENT_1, typename T>
void linebuffer_2D(stream<PackedStencil<T, IN_EXTENT_0, IN_EXTENT_1, EXTENT_2, EXTENT_3> > &in_stream,
		stream<PackedStencil<T, OUT_EXTENT_0, OUT_EXTENT_1, EXTENT_2, EXTENT_3> > &out_stream) {
    static_assert(IMG_EXTENT_1 > OUT_EXTENT_1, "output extent is larger than image.");
    static_assert(OUT_EXTENT_1 > IN_EXTENT_1, "input extent is larger than output."); // TODO handle this situation.
    static_assert(IMG_EXTENT_1 % IN_EXTENT_1 == 0, "image extent is not divisible by input."); // TODO handle this situation.
    static_assert(OUT_EXTENT_1 % IN_EXTENT_1 == 0, "output extent is not divisible by input."); // TODO handle this situation.
#pragma HLS INLINE
    // the following pragmas will cause writes to linebuffer (in linebuffer_2D_col()) disappear
    // I believe it is a HLS compiler bug
    //#pragma HLS DATA_PACK variable=in_stream
    //#pragma HLS DATA_PACK variable=out_stream

    stream<PackedStencil<T, IN_EXTENT_0, OUT_EXTENT_1, EXTENT_2, EXTENT_3> > col_buf_stream;
#pragma HLS STREAM variable=col_buf_stream depth=1
#pragma HLS RESOURCE variable=col_buf_stream core=FIFO_SRL

    // use a 2D storage to buffer lines of image,
    // and output a column stencil per input at steady state
    linebuffer_2D_col<IMG_EXTENT_0, IMG_EXTENT_1>(in_stream, col_buf_stream);

    // feed the column stencil stream to 1D line buffer
    const size_t NUM_OF_OUTPUT_1 = (IMG_EXTENT_1 - OUT_EXTENT_1) / IN_EXTENT_1 + 1;
    for (size_t n1 = 0; n1 < NUM_OF_OUTPUT_1; n1++) {
#pragma HLS loop_flatten off
	linebuffer_1D<IMG_EXTENT_0>(col_buf_stream, out_stream);
    }
}

// An overloaded (trivial) 2D line buffer, where input dim 1 and output dim 1 are the same size
template <size_t IMG_EXTENT_0, size_t IMG_EXTENT_1,
          size_t IN_EXTENT_0, size_t OUT_EXTENT_0,
          size_t EXTENT_1, size_t EXTENT_2, size_t EXTENT_3, typename T>
void linebuffer_2D(stream<PackedStencil<T, IN_EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> > &in_stream,
                   stream<PackedStencil<T, OUT_EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> > &out_stream) {
#pragma HLS INLINE
    for(size_t idx_1 = 0; idx_1 < IMG_EXTENT_1; idx_1 += EXTENT_1) {
	linebuffer_1D<IMG_EXTENT_0>(in_stream, out_stream);
    }
}


/** A line buffer that buffers a image size [IMG_EXTENT_0, IMG_EXTENT_1, IMG_EXTENT_2].
 * The input is a stencil size [IN_EXTENT_0, IN_EXTENT_1, IN_EXTENT_2], and it traversal
 * the image along dimensiO 0 first, and then dimension 1, and so on. The step of the
 * input stencil is the same as the size of input stencil, so there is no overlapping
 * between input stencils.
 * The output is a stencil size [OUT_EXTENT_0, OUT_EXTENT_1, OUT_EXTENT_2], and it traversal
 * the image the same as input (i.e. along dimension 0 first, and then dimension 1, and so on.
 * The step of the output stencil is the same as the size of input stencil, so the
 * throughputs of the inputs and outputs are balanced at the steady state. In other words,
 * the line buffer generates one output per input at the steady state.
 */
template <size_t IMG_EXTENT_0, size_t IMG_EXTENT_1=1, size_t IMG_EXTENT_2=1, size_t IMG_EXTENT_3=1,
	  size_t IN_EXTENT_0, size_t IN_EXTENT_1, size_t IN_EXTENT_2, size_t IN_EXTENT_3,
	  size_t OUT_EXTENT_0, size_t OUT_EXTENT_1, size_t OUT_EXTENT_2, size_t OUT_EXTENT_3,
	  typename T>
void linebuffer(stream<PackedStencil<T, IN_EXTENT_0, IN_EXTENT_1, IN_EXTENT_2, IN_EXTENT_3> > &in_stream,
		stream<PackedStencil<T, OUT_EXTENT_0, OUT_EXTENT_1, OUT_EXTENT_2, OUT_EXTENT_3> > &out_stream) {
    static_assert(IMG_EXTENT_3 == IN_EXTENT_3 && IMG_EXTENT_3 == OUT_EXTENT_3,
		  "dont not support 4D line buffer yet.");
    static_assert(IMG_EXTENT_2 == IN_EXTENT_2 && IMG_EXTENT_2 == OUT_EXTENT_2,
		  "dont not support 3D line buffer yet.");
#pragma HLS INLINE
    linebuffer_2D<IMG_EXTENT_0, IMG_EXTENT_1>(in_stream, out_stream);
}


template <size_t IMG_EXTENT_0, size_t IMG_EXTENT_1=1, size_t IMG_EXTENT_2=1, size_t IMG_EXTENT_3=1,
	  size_t IN_EXTENT_0, size_t IN_EXTENT_1, size_t IN_EXTENT_2, size_t IN_EXTENT_3,
          size_t OUT_EXTENT_0, size_t OUT_EXTENT_1, size_t OUT_EXTENT_2, size_t OUT_EXTENT_3,
          typename T>
void linebuffer_ref(stream<PackedStencil<T, IN_EXTENT_0, IN_EXTENT_1, IN_EXTENT_2, IN_EXTENT_3> > &in_stream,
		    stream<PackedStencil<T, OUT_EXTENT_0, OUT_EXTENT_1, OUT_EXTENT_2, OUT_EXTENT_3> > &out_stream) {

    T buffer[IMG_EXTENT_3][IMG_EXTENT_2][IMG_EXTENT_1][IMG_EXTENT_0];

    for(size_t outer_3 = 0; outer_3 < IMG_EXTENT_3; outer_3 += IN_EXTENT_3)
    for(size_t outer_2 = 0; outer_2 < IMG_EXTENT_2; outer_2 += IN_EXTENT_2)
    for(size_t outer_1 = 0; outer_1 < IMG_EXTENT_1; outer_1 += IN_EXTENT_1)
    for(size_t outer_0 = 0; outer_0 < IMG_EXTENT_0; outer_0 += IN_EXTENT_0) {
        Stencil<T, IN_EXTENT_0, IN_EXTENT_1, IN_EXTENT_2, IN_EXTENT_3> stencil = in_stream.read();

        for(size_t inner_3 = 0; inner_3 < IN_EXTENT_3; inner_3++)
        for(size_t inner_2 = 0; inner_2 < IN_EXTENT_2; inner_2++)
        for(size_t inner_1 = 0; inner_1 < IN_EXTENT_1; inner_1++)
        for(size_t inner_0 = 0; inner_0 < IN_EXTENT_0; inner_0++)
            buffer[outer_3+inner_3][outer_2+inner_2][outer_1+inner_1][outer_0+inner_0]
                = stencil(inner_0, inner_1, inner_2, inner_3);
    }

    for(size_t outer_3 = 0; outer_3 <= IMG_EXTENT_3 - OUT_EXTENT_3; outer_3 += IN_EXTENT_3)
    for(size_t outer_2 = 0; outer_2 <= IMG_EXTENT_2 - OUT_EXTENT_2; outer_2 += IN_EXTENT_2)
    for(size_t outer_1 = 0; outer_1 <= IMG_EXTENT_1 - OUT_EXTENT_1; outer_1 += IN_EXTENT_1)
    for(size_t outer_0 = 0; outer_0 <= IMG_EXTENT_0 - OUT_EXTENT_0; outer_0 += IN_EXTENT_0) {
        Stencil<T, OUT_EXTENT_0, OUT_EXTENT_1, OUT_EXTENT_2, OUT_EXTENT_3> stencil;

        for(size_t inner_3 = 0; inner_3 < OUT_EXTENT_3; inner_3++)
        for(size_t inner_2 = 0; inner_2 < OUT_EXTENT_2; inner_2++)
        for(size_t inner_1 = 0; inner_1 < OUT_EXTENT_1; inner_1++)
        for(size_t inner_0 = 0; inner_0 < OUT_EXTENT_0; inner_0++)
            stencil(inner_0, inner_1, inner_2, inner_3)
                = buffer[outer_3+inner_3][outer_2+inner_2][outer_1+inner_1][outer_0+inner_0];

		out_stream.write(stencil);
    }
}


#endif
