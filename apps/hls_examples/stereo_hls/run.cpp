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
    if (argc < 5) {
        printf("Usage: ./run left.png left-remap.png right0224.png right-remap.png\n");
        return 0;
    }

    Buffer<uint8_t> left = load_image(argv[1]);
    Buffer<uint8_t> left_remap = load_image(argv[2]);
    Buffer<uint8_t> right = load_image(argv[3]);
    Buffer<uint8_t> right_remap = load_image(argv[4]);

    Buffer<uint8_t> out_native(left.width(), left.height());
    Buffer<uint8_t> out_hls(64, 64);

    printf("start.\n");

    pipeline_native(right, left, right_remap, left_remap, out_native);
    save_image(out_native, "out.png");

    printf("finish running native code\n");

    pipeline_hls(right, left, right_remap, left_remap, out_hls);
    save_image(out_hls, "out_hls.png");

    printf("finish running HLS code\n");

    int fails = 0;
    for (int y = 0; y < out_hls.height(); y++) {
        for (int x = 0; x < out_hls.width(); x++) {
            if (out_native(x, y) != out_hls(x, y)) {
                printf("out_native(%d, %d) = %d, but out_c(%d, %d) = %d\n",
                       x, y, out_native(x, y),
                       x, y, out_hls(x, y));
                fails++;
            }
	}
    }
    if (fails) {
        printf("%d fails.\n", fails);
        return 1;
    } else {
        printf("passed.\n");
        return 0;
    }
}
