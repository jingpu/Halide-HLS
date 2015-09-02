#ifndef STENCIL_H
#define STENCIL_H

#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <hls_stream.h>


using hls::stream;

/** multi-dimension (up-to 4 dimensions) stencil struct
 */
// TODO support 4 dimensions
template <typename T, size_t EXTENT_0, size_t EXTENT_1 = 1, size_t EXTENT_2 = 1, size_t EXTENT_3 = 1>
struct Stencil{
public:
    T value[EXTENT_3][EXTENT_2][EXTENT_1][EXTENT_0];

    /** writer
     */
    inline T& operator()(size_t index_0, size_t index_1 = 0, size_t index_2 = 0, size_t index_3 = 0) {
#pragma HLS INLINE
	assert(index_0 < EXTENT_0 && index_1 < EXTENT_1 && index_2 < EXTENT_2 && index_3 < EXTENT_3);
	return value[index_3][index_2][index_1][index_0];
    }

    /** reader
     */
    inline const T& operator()(size_t index_0, size_t index_1 = 0, size_t index_2 = 0, size_t index_3 = 0) const {
#pragma HLS INLINE
	assert(index_0 < EXTENT_0 && index_1 < EXTENT_1 && index_2 < EXTENT_2 && index_3 < EXTENT_3);
	return value[index_3][index_2][index_1][index_0];
    }

    template <size_t ST_EXTENT_0, size_t ST_EXTENT_1 = 1, size_t ST_EXTENT_2 = 1, size_t ST_EXTENT_3 = 1>
    void write_stencil_at(const Stencil<T, ST_EXTENT_0, ST_EXTENT_1, ST_EXTENT_2, ST_EXTENT_3> &stencil,
			  size_t index_0, size_t index_1 = 0, size_t index_2 = 0, size_t index_3 = 0) {
#pragma HLS INLINE
	assert(index_0 + ST_EXTENT_0 - 1 < EXTENT_0 &&
	       index_1 + ST_EXTENT_1 - 1 < EXTENT_1 &&
	       index_2 + ST_EXTENT_2 - 1 < EXTENT_2 &&
	       index_3 + ST_EXTENT_3 - 1 < EXTENT_3);
	for(size_t st_idx_3 = 0; st_idx_3 < ST_EXTENT_3; st_idx_3++)
#pragma HLS UNROLL
	    for(size_t st_idx_2 = 0; st_idx_2 < ST_EXTENT_2; st_idx_2++)
#pragma HLS UNROLL
		for(size_t st_idx_1 = 0; st_idx_1 < ST_EXTENT_1; st_idx_1++)
#pragma HLS UNROLL
		    for(size_t st_idx_0 = 0; st_idx_0 < ST_EXTENT_0; st_idx_0++)
#pragma HLS UNROLL
			value[index_3+st_idx_3][index_2+st_idx_2][index_1+st_idx_1][index_0+st_idx_0] = stencil(st_idx_0, st_idx_1, st_idx_2, st_idx_3);
    }


    template <size_t ST_EXTENT_0, size_t ST_EXTENT_1 = 1, size_t ST_EXTENT_2 = 1, size_t ST_EXTENT_3 = 1>
    void read_stencil_at(Stencil<T, ST_EXTENT_0, ST_EXTENT_1, ST_EXTENT_2> &stencil,
			 size_t index_0, size_t index_1 = 0, size_t index_2 = 0, size_t index_3 = 0) const {
#pragma HLS INLINE
	assert(index_0 + ST_EXTENT_0 - 1 < EXTENT_0 &&
	       index_1 + ST_EXTENT_1 - 1 < EXTENT_1 &&
	       index_2 + ST_EXTENT_2 - 1 < EXTENT_2 &&
	       index_3 + ST_EXTENT_3 - 1 < EXTENT_3);
	for(size_t st_idx_3 = 0; st_idx_3 < ST_EXTENT_3; st_idx_3++)
#pragma HLS UNROLL
	    for(size_t st_idx_2 = 0; st_idx_2 < ST_EXTENT_2; st_idx_2++)
#pragma HLS UNROLL
		for(size_t st_idx_1 = 0; st_idx_1 < ST_EXTENT_1; st_idx_1++)
#pragma HLS UNROLL
		    for(size_t st_idx_0 = 0; st_idx_0 < ST_EXTENT_0; st_idx_0++)
#pragma HLS UNROLL
			stencil(st_idx_0, st_idx_1, st_idx_2, st_idx_3) = value[index_3+st_idx_3][index_2+st_idx_2][index_1+st_idx_1][index_0+st_idx_0];
    }
};


template <size_t IMG_EXTENT_0, size_t EXTENT_1, size_t EXTENT_2, size_t EXTENT_3,
	  size_t IN_EXTENT_0,  size_t OUT_EXTENT_0, typename T>
void linebuffer_1D(stream< Stencil<T, IN_EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> > &in_stream,
		   stream< Stencil<T, OUT_EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> > &out_stream) {
    static_assert(IMG_EXTENT_0 > OUT_EXTENT_0, "output extent is larger than image.");
    static_assert(OUT_EXTENT_0 > IN_EXTENT_0, "input extent is larger than output."); // TODO handle this situation.
    static_assert(IMG_EXTENT_0 % IN_EXTENT_0 == 0, "image extent is not divisible by input."); // TODO handle this situation.
    static_assert(OUT_EXTENT_0 % IN_EXTENT_0 == 0, "output extent is not divisible by input."); // TODO handle this situation.
#pragma HLS INLINE

    Stencil<T, OUT_EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> buffer;
#pragma HLS ARRAY_PARTITION variable=buffer complete dim=0

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


template <size_t IMG_EXTENT_0, size_t IMG_EXTENT_1, size_t EXTENT_2, size_t EXTENT_3,
	  size_t IN_EXTENT_0, size_t IN_EXTENT_1,
	  size_t OUT_EXTENT_1, typename T>
void linebuffer_2D_col(stream< Stencil<T, IN_EXTENT_0, IN_EXTENT_1, EXTENT_2, EXTENT_3> > &in_stream,
		       stream< Stencil<T, IN_EXTENT_0, OUT_EXTENT_1, EXTENT_2, EXTENT_3> > &out_stream) {

    Stencil<T, IMG_EXTENT_0, OUT_EXTENT_1, EXTENT_2, EXTENT_3> linebuffer;
#pragma HLS ARRAY_PARTITION variable=linebuffer complete dim=3

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
	if (idx_1 >= OUT_EXTENT_1)
	    idx_1 -= OUT_EXTENT_1;

	for(size_t idx_0 = 0; idx_0 < IMG_EXTENT_0; idx_0 += IN_EXTENT_0)  {
#pragma HLS PIPELINE
	    Stencil<T, IN_EXTENT_0, IN_EXTENT_1, EXTENT_2, EXTENT_3> in_stencil = in_stream.read();
	    linebuffer.write_stencil_at(in_stencil, idx_0, idx_1);

	    Stencil<T, IN_EXTENT_0, OUT_EXTENT_1, EXTENT_2, EXTENT_3> col_buf, col_buf_preshuffled;
#pragma HLS ARRAY_PARTITION variable=col_buf complete dim=3
#pragma HLS ARRAY_PARTITION variable=col_buf_preshuffled complete dim=3

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
void linebuffer_2D(stream< Stencil<T, IN_EXTENT_0, IN_EXTENT_1, EXTENT_2, EXTENT_3> > &in_stream,
		stream< Stencil<T, OUT_EXTENT_0, OUT_EXTENT_1, EXTENT_2, EXTENT_3> > &out_stream) {
    static_assert(IMG_EXTENT_1 > OUT_EXTENT_1, "output extent is larger than image.");
    static_assert(OUT_EXTENT_1 > IN_EXTENT_1, "input extent is larger than output."); // TODO handle this situation.
    static_assert(IMG_EXTENT_1 % IN_EXTENT_1 == 0, "image extent is not divisible by input."); // TODO handle this situation.
    static_assert(OUT_EXTENT_1 % IN_EXTENT_1 == 0, "output extent is not divisible by input."); // TODO handle this situation.
#pragma HLS INLINE
    // the following pragmas will cause writes to linebuffer (in linebuffer_2D_col()) disappear
    // I believe it is a HLS compiler bug
    //#pragma HLS DATA_PACK variable=in_stream
    //#pragma HLS DATA_PACK variable=out_stream

    stream< Stencil<T, IN_EXTENT_0, OUT_EXTENT_1, EXTENT_2, EXTENT_3> > col_buf_stream;

    // use a 2D storage to buffer lines of image,
    // and output a column stencil per input at steady state
    linebuffer_2D_col<IMG_EXTENT_0, IMG_EXTENT_1>(in_stream, col_buf_stream);

    // feed the column stencil stream to 1D line buffer
    const size_t NUM_OF_OUTPUT_1 = (IMG_EXTENT_1 - OUT_EXTENT_1) / IN_EXTENT_1 + 1;
    for (size_t n1 = 0; n1 < NUM_OF_OUTPUT_1; n1++)
	linebuffer_1D<IMG_EXTENT_0>(col_buf_stream, out_stream);
}

// A trivial 2D line buffer where input and output are the same size
template <size_t IMG_EXTENT_0, size_t IMG_EXTENT_1,
          size_t EXTENT_0, size_t EXTENT_1, size_t EXTENT_2, size_t EXTENT_3, typename T>
void linebuffer_2D(stream< Stencil<T, EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> > &in_stream,
                   stream< Stencil<T, EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> > &out_stream) {
#pragma HLS INLINE
    for(size_t idx_0 = 0; idx_0 < IMG_EXTENT_0; idx_0 += EXTENT_0)
        for(size_t idx_1 = 0; idx_1 < IMG_EXTENT_1; idx_1 += EXTENT_1) {
#pragma HLS PIPELINE
          Stencil<T, EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> s = in_stream.read();
          out_stream.write(s);
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
template <size_t IMG_EXTENT_0, size_t IMG_EXTENT_1, size_t IMG_EXTENT_2,
	  size_t IN_EXTENT_0, size_t IN_EXTENT_1, size_t IN_EXTENT_2,
	  size_t OUT_EXTENT_0, size_t OUT_EXTENT_1, size_t OUT_EXTENT_2,
	  typename T>
void linebuffer(stream< Stencil<T, IN_EXTENT_0, IN_EXTENT_1, IN_EXTENT_2> > &in_stream,
		stream< Stencil<T, OUT_EXTENT_0, OUT_EXTENT_1, OUT_EXTENT_2> > &out_stream) {
    static_assert(IMG_EXTENT_2 == IN_EXTENT_2 && IMG_EXTENT_2 == OUT_EXTENT_2,
		  "dont not support 3D line buffer yet.");
#pragma HLS INLINE
    linebuffer_2D<IMG_EXTENT_0, IMG_EXTENT_1>(in_stream, out_stream);
}

template <typename T,
	  size_t IMG_EXTENT_0, size_t IMG_EXTENT_1, size_t IMG_EXTENT_2,
	  size_t IN_EXTENT_0, size_t IN_EXTENT_1, size_t IN_EXTENT_2,
	  size_t OUT_EXTENT_0, size_t OUT_EXTENT_1, size_t OUT_EXTENT_2>
void linebuffer_ref(stream< Stencil<T, IN_EXTENT_0, IN_EXTENT_1, IN_EXTENT_2> > &in_stream,
		    stream< Stencil<T, OUT_EXTENT_0, OUT_EXTENT_1, OUT_EXTENT_2> > &out_stream) {

    T buffer[IMG_EXTENT_2][IMG_EXTENT_1][IMG_EXTENT_0];

    for(size_t in_outer_2 = 0; in_outer_2 < IMG_EXTENT_2; in_outer_2 += IN_EXTENT_2)
	for(size_t in_outer_1 = 0; in_outer_1 < IMG_EXTENT_1; in_outer_1 += IN_EXTENT_1)
	    for(size_t in_outer_0 = 0; in_outer_0 < IMG_EXTENT_0; in_outer_0 += IN_EXTENT_0) {
		Stencil<T, IN_EXTENT_0, IN_EXTENT_1, IN_EXTENT_2> in_stencil = in_stream.read();

		for(size_t in_inner_2 = 0; in_inner_2 < IN_EXTENT_2; in_inner_2++)
		    for(size_t in_inner_1 = 0; in_inner_1 < IN_EXTENT_1; in_inner_1++)
			for(size_t in_inner_0 = 0; in_inner_0 < IN_EXTENT_0; in_inner_0++)
			    buffer[in_outer_2+in_inner_2][in_outer_1+in_inner_1][in_outer_0+in_inner_0] = in_stencil(in_inner_0, in_inner_1, in_inner_2);
	    }

    for(size_t out_outer_2 = 0; out_outer_2 <= IMG_EXTENT_2 - OUT_EXTENT_2; out_outer_2 += IN_EXTENT_2)
	for(size_t out_outer_1 = 0; out_outer_1 <= IMG_EXTENT_1 - OUT_EXTENT_1; out_outer_1 += IN_EXTENT_1)
	    for(size_t out_outer_0 = 0; out_outer_0 <= IMG_EXTENT_0 - OUT_EXTENT_0; out_outer_0 += IN_EXTENT_0) {
		Stencil<T, OUT_EXTENT_0, OUT_EXTENT_1, OUT_EXTENT_2> out_stencil;

		for(size_t out_inner_2 = 0; out_inner_2 < OUT_EXTENT_2; out_inner_2++)
		    for(size_t out_inner_1 = 0; out_inner_1 < OUT_EXTENT_1; out_inner_1++)
			for(size_t out_inner_0 = 0; out_inner_0 < OUT_EXTENT_0; out_inner_0++)
			    out_stencil(out_inner_0, out_inner_1, out_inner_2) = buffer[out_outer_2+out_inner_2][out_outer_1+out_inner_1][out_outer_0+out_inner_0];

		out_stream.write(out_stencil);
	    }
}



template <size_t OUT_EXTENT_0, size_t OUT_EXTENT_1, size_t OUT_EXTENT_2,
	  typename IN_TYPE, typename OUT_TYPE>
void convolve55_stream(stream< Stencil<IN_TYPE, 5, 5, 1> > &in_stream,
		       stream< Stencil<OUT_TYPE, 1, 1, 1> > &out_stream)
{
    const uint16_t coef[5][5] = {
	{1,     3,     6,     3,     1},
	{3,    15,    25,    15,     3},
	{6,    25,    44,    25,     6},
	{3,    15,    25,    15,     3},
	{1,     3,     6,     3,     1}
    };

    for(size_t outer_2 = 0; outer_2 < OUT_EXTENT_2; outer_2++)
	for(size_t outer_1 = 0; outer_1 < OUT_EXTENT_1; outer_1++)
	    for(size_t outer_0 = 0; outer_0 < OUT_EXTENT_0; outer_0++) {
		Stencil<IN_TYPE, 5, 5, 1> in_stencil = in_stream.read();
		Stencil<OUT_TYPE, 1, 1, 1> out_stencil;

		uint16_t val = 0;
		for(int l = 0; l < 5; l++)
		    for(int m = 0; m < 5; m++)
			val += in_stencil.value[0][l][m] * coef[l][m];

		uint8_t output = val >> 8; // back to 8bit
		out_stencil.value[0][0][0] = (OUT_TYPE) output;
		out_stream.write(out_stencil);
	    }
}

#ifndef HALIDE_ATTRIBUTE_ALIGN
  #ifdef _MSC_VER
    #define HALIDE_ATTRIBUTE_ALIGN(x) __declspec(align(x))
  #else
    #define HALIDE_ATTRIBUTE_ALIGN(x) __attribute__((aligned(x)))
  #endif
#endif
#ifndef BUFFER_T_DEFINED
#define BUFFER_T_DEFINED
#include <stdbool.h>
#include <stdint.h>
typedef struct buffer_t {
    uint64_t dev;
    uint8_t* host;
    int32_t extent[4];
    int32_t stride[4];
    int32_t min[4];
    int32_t elem_size;
    HALIDE_ATTRIBUTE_ALIGN(1) bool host_dirty;
    HALIDE_ATTRIBUTE_ALIGN(1) bool dev_dirty;
    HALIDE_ATTRIBUTE_ALIGN(1) uint8_t _padding[10 - sizeof(void *)];
} buffer_t;
#endif


template <typename T, size_t EXTENT_0, size_t EXTENT_1, size_t EXTENT_2, size_t EXTENT_3>
void buffer_to_stencil(const buffer_t *buffer,
                       Stencil<T, EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> &stencil) {
    assert(EXTENT_0 == buffer->extent[0]);
    assert((EXTENT_1 == buffer->extent[1]) || (EXTENT_1 == 1 && buffer->extent[1] == 0));
    assert((EXTENT_2 == buffer->extent[2]) || (EXTENT_2 == 1 && buffer->extent[2] == 0));
    assert((EXTENT_3 == buffer->extent[3]) || (EXTENT_3 == 1 && buffer->extent[3] == 0));
    assert(sizeof(T) == buffer->elem_size);

    for(size_t idx_3 = 0; idx_3 < EXTENT_3; idx_3++)
        for(size_t idx_2 = 0; idx_2 < EXTENT_2; idx_2++)
            for(size_t idx_1 = 0; idx_1 < EXTENT_1; idx_1++)
                for(size_t idx_0 = 0; idx_0 < EXTENT_0; idx_0++) {
                    const uint8_t *ptr = buffer->host;
                    size_t offset = idx_0 * buffer->stride[0] +
                        idx_1 * buffer->stride[1] +
                        idx_2 * buffer->stride[2] +
                        idx_3 * buffer->stride[3];
                    const T *address =  (T *)(ptr + offset * buffer->elem_size);
                    stencil(idx_0, idx_1, idx_2, idx_3) = *address;
                }
}
#endif
