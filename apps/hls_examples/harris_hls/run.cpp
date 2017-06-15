#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include "pipeline_hls.h"
#include "pipeline_native.h"

#include "HalideBuffer.h"
#include "halide_image_io.h"

using namespace Halide::Runtime;
using namespace Halide::Tools;

int main(int argc, char **argv) {
    //float k = 0.04;
    //float threshold = 100;

    Buffer<uint8_t> input = load_image(argv[1]);
    Buffer<uint8_t> out_native(input.width()-6, input.height()-6);
    Buffer<uint8_t> out_hls(64*2, 64*2);

    printf("start.\n");

    pipeline_native(input, out_native);
    save_image(out_native, "out.png");

    printf("finished running native code\n");

    pipeline_hls(input, out_hls);
    save_image(out_hls, "out_hls.png");

    printf("finished running HLS code\n");

    bool success = true;
    for (int y = 0; y < out_hls.height(); y++) {
        for (int x = 0; x < out_hls.width(); x++) {
            if (fabs(out_native(x, y) - out_hls(x, y)) > 1e-4) {
                printf("out_native(%d, %d) = %d, but out_c(%d, %d) = %d\n",
                       x, y, out_native(x, y),
                       x, y, out_hls(x, y));
		success = false;
            }
	}
    }
    if (success) {
        printf("passed.\n");
        return 0;
    } else {
        printf("failed.\n");
        return 1;
    }
}
