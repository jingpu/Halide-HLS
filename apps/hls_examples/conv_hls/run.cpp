#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "pipeline_native.h"
#include "pipeline_hls.h"

#include "HalideBuffer.h"
#include "halide_image_io.h"

using namespace Halide::Runtime;
using namespace Halide::Tools;

const unsigned char gaussian2d[5][5] = {
    {1,     3,     6,     3,     1},
    {3,    15,    25,    15,     3},
    {6,    25,    44,    25,     6},
    {3,    15,    25,    15,     3},
    {1,     3,     6,     3,     1}
};


int main(int argc, char **argv) {
    Buffer<uint8_t> in(800, 800, 3);
    Buffer<uint8_t> weight(5, 5);

    Buffer<uint8_t> out_native(in.width(), in.height(), in.channels());
    Buffer<uint8_t> out_hls(in.width(), in.height(), in.channels());
    Buffer<uint8_t> out_hls_argv(in.width(), in.height(), in.channels());

    for (int y = 0; y < in.height(); y++) {
        for (int x = 0; x < in.width(); x++) {
	    for (int c = 0; c < in.channels(); c++) {
	        in(x, y, c) = (uint8_t) x+y;   //rand();
	    }
        }
    }

    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 5; x++)
            weight(x, y) = gaussian2d[y][x];

    printf("start.\n");

    pipeline_native(in, weight, 0, out_native);

    printf("finish running native code\n");


    pipeline_hls(in, weight, 0, out_hls);
    printf("finish running HLS code\n");

    uint16_t bias = 0;
    void* arg_values[4] = { in.raw_buffer(),
                            weight.raw_buffer(),
                            &bias,
                            out_hls_argv.raw_buffer() };
    pipeline_hls_argv(arg_values);
    printf("finish running HLS code with argv\n");

    save_png(out_native, "out.png");

    bool success = true;
    for (int y = 0; y < out_native.height(); y++) {
        for (int x = 0; x < out_native.width(); x++) {
	    for (int c = 0; c < out_native.channels(); c++) {
	        if (out_native(x, y, c) != out_hls(x, y, c) ||
                    out_hls(x, y, c) != out_hls_argv(x, y, c)) {
                    printf("Mismatch found: out_native(%d, %d, %d) = %d, "
                           "out_hls(%d, %d, %d) = %d, "
                           "out_hls_argv(%d, %d, %d) = %d\n",
			   x, y, c, out_native(x, y, c),
			   x, y, c, out_hls(x, y, c),
                           x, y, c, out_hls_argv(x, y, c));
                    success = false;
                }
            }
        }
    }

    if (success) {
        printf("Successed!\n");
        return 0;
    } else {
        printf("Failed!\n");
        return 1;
    }
}
