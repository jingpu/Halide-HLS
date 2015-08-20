#include "stencil.h"
//#include "hls_target.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


template <size_t N, typename T, size_t EXTENT_0, size_t EXTENT_1, size_t EXTENT_2>
void gen_inputs(stream< Stencil<T, EXTENT_0, EXTENT_1, EXTENT_2> > &in_stream_1,
		stream< Stencil<T, EXTENT_0, EXTENT_1, EXTENT_2> > &in_stream_2) {
    for (int i = 0; i < N; i++) {
	Stencil<T, EXTENT_0, EXTENT_1, EXTENT_2> input;
	for(size_t idx_2 = 0; idx_2 < EXTENT_2; idx_2++)
	    for(size_t idx_1 = 0; idx_1 < EXTENT_1; idx_1++)
		for(size_t idx_0 = 0; idx_0 < EXTENT_0; idx_0++)
		    input(idx_0, idx_1, idx_2) = (T) rand();
	in_stream_1.write(input);
	in_stream_2.write(input);
    }
}


template <size_t N, typename T, size_t EXTENT_0, size_t EXTENT_1, size_t EXTENT_2>
bool check_outputs(stream< Stencil<T, EXTENT_0, EXTENT_1, EXTENT_2> > &out_stream_1,
		   stream< Stencil<T, EXTENT_0, EXTENT_1, EXTENT_2> > &out_stream_2) {
    bool success = true;
    for (int i = 0; i < N; i++) {
	Stencil<T, EXTENT_0, EXTENT_1, EXTENT_2> output_1 = out_stream_1.read();
	Stencil<T, EXTENT_0, EXTENT_1, EXTENT_2> output_2 = out_stream_2.read();
	for(size_t idx_2 = 0; idx_2 < EXTENT_2; idx_2++)
	    for(size_t idx_1 = 0; idx_1 < EXTENT_1; idx_1++)
		for(size_t idx_0 = 0; idx_0 < EXTENT_0; idx_0++)
		    if (output_1(idx_0, idx_1, idx_2) != output_2(idx_0, idx_1, idx_2)) {
			printf("output_1(%lu, %lu, %lu) = %d, but output_2(%lu, %lu, %lu) = %d\n",
			       idx_0, idx_1, idx_2, output_1(idx_0, idx_1, idx_2),
			       idx_0, idx_1, idx_2, output_2(idx_0, idx_1, idx_2));
		        success = false;
		    }
    }
    return success;
}


void test_1D() {
    hls::stream<Stencil<uint8_t, 1, 1, 1> > input_stream;
    hls::stream<Stencil<uint8_t, 1, 1, 1> > input_ref_stream;
    hls::stream<Stencil<uint8_t, 3, 1, 1> > output_stream;
    hls::stream<Stencil<uint8_t, 3, 1, 1> > output_ref_stream;

    gen_inputs<10>(input_stream, input_ref_stream);

    printf("test linebuffer_1D()... ");
    linebuffer_1D<10>(input_stream, output_stream);
    linebuffer_ref<uint8_t, 10, 1, 1>(input_ref_stream, output_ref_stream);

    if (check_outputs<8>(output_stream, output_ref_stream))
	printf("passed!\n");
    else
	printf("failed!\n");
}


void test_2D() {
    hls::stream<Stencil<uint8_t, 1, 1, 1> > input_stream;
    hls::stream<Stencil<uint8_t, 1, 1, 1> > input_ref_stream;
    hls::stream<Stencil<uint8_t, 5, 5, 1> > output_stream;
    hls::stream<Stencil<uint8_t, 5, 5, 1> > output_ref_stream;

    gen_inputs<260*260>(input_stream, input_ref_stream);

    printf("test linebuffer_2D()... ");
    linebuffer_2D<260, 260>(input_stream, output_stream);
    //hls_target(input_stream, output_stream);
    linebuffer_ref<uint8_t, 260, 260, 1>(input_ref_stream, output_ref_stream);

    if (check_outputs<256*256>(output_stream, output_ref_stream))
	printf("passed!\n");
    else
	printf("failed!\n");
}

int main(int argc, char **argv) {
    test_1D();
    test_2D();
    return 0;
}
