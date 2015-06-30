#include "pipeline_native.h"
#include "pipeline_hls.h"
#include "../support/static_image.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    Image<uint8_t> in(256, 256, 3);

    for (int y = 0; y < in.height(); y++) {
        for (int x = 0; x < in.width(); x++) {
	    for (int c = 0; c < in.channels(); c++) {
	        in(x, y, c) = (uint8_t)rand();
	    }
        }
    }

    Image<uint8_t> out_native(256, 256, 3);
    Image<uint8_t> out_c(256, 256, 3);

    pipeline_native(in, out_native);

    pipeline_c(in, out_c);

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
