#include "pipeline_native.h"
//#include "pipeline_c.h"
#include "pipeline_hls.h"
#include "../support/static_image.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    Image<uint8_t> in(800, 800, 3);

    for (int y = 0; y < in.height(); y++) {
        for (int x = 0; x < in.width(); x++) {
	    for (int c = 0; c < in.channels(); c++) {
	        in(x, y, c) = (uint8_t) x+y;   //rand();
	    }
        }
    }

    Image<uint8_t> out_native(in.width(), in.height(), in.channels());
    Image<uint8_t> out_c(in.width(), in.height(), in.channels());

    printf("start.\n");

    pipeline_native(in, out_native);

    printf("finish running native code\n");

    pipeline_hls(in, out_c);

    printf("finish running HLS code\n");

    for (int y = 0; y < out_native.height(); y++) {
        for (int x = 0; x < out_native.width(); x++) {
	    for (int c = 0; c < out_native.channels(); c++) {

	        if (out_native(x, y, c) != out_c(x, y, c)) {
                    printf("out_native(%d, %d, %d) = %d, but out_c(%d, %d, %d) = %d\n",
			   x, y, c, out_native(x, y, c),
			   x, y, c, out_c(x, y, c));
            }
	}
        }
    }

    printf("Success!\n");
    return 0;
}
