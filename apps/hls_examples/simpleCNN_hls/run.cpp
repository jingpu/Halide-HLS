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
    Buffer<uint8_t> in(3, 256, 256);
    Buffer<uint8_t> weight(64, 3, 3, 3);
    uint8_t bias = 0;

    Buffer<uint8_t> out_native(weight.width(), in.height(), in.channels());
    Buffer<uint8_t> out_hls(weight.width(), in.height(), in.channels());


    for (int y = 0; y < in.height(); y++) {
        for (int x = 0; x < in.width(); x++) {
	    for (int c = 0; c < in.channels(); c++) {
	        in(x, y, c) = (uint8_t) x+y;   //rand();
	    }
        }
    }

    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
             for (int z =0; z< weight.height(); z++) {
                for (int c = 0; c < weight.width(); c++) {
                    weight(c, z, x, y) = gaussian2d[y][x] +z+c;
                }
            }
        }
    }

    printf("start.\n");

    pipeline_native(in, weight, bias, out_native);

    printf("finish running native code\n");

    pipeline_hls(in, weight, bias, out_hls);

    printf("finish running HLS code\n");

    bool success = true;
    for (int y = 0; y < out_native.height(); y++) {
        for (int x = 0; x < out_native.width(); x++) {
	    for (int c = 0; c < out_native.channels(); c++) {
	        if (out_native(x, y, c) != out_hls(x, y, c)) {
                    printf("out_native(%d, %d, %d) = %d, but out_c(%d, %d, %d) = %d\n",
			   x, y, c, out_native(x, y, c),
			   x, y, c, out_hls(x, y, c));
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
