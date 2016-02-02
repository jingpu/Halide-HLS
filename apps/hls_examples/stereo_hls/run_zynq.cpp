#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include "pipeline_zynq.h"
#include "pipeline_native.h"

#include "halide_image.h"
#include "halide_image_io.h"

using namespace Halide::Tools;

int main(int argc, char **argv) {
    if (argc < 5) {
        printf("Usage: ./run left.png left-remap.png right0224.png right-remap.png\n");
        return 0;
    }

    Image<uint8_t> left = load_image(argv[1]);
    Image<uint8_t> left_remap = load_image(argv[2]);
    Image<uint8_t> right = load_image(argv[3]);
    Image<uint8_t> right_remap = load_image(argv[4]);

    Image<uint8_t> out_native(left.width(), left.height());
    Image<uint8_t> out_zynq(left.width(), left.height());
    //Image<uint8_t> out_zynq(256, 256);


    printf("start.\n");

    pipeline_native(right, left, right_remap, left_remap, out_native);
    save_image(out_native, "out_native.png");

    printf("finish running native code\n");

    pipeline_zynq(right, left, right_remap, left_remap, out_zynq);
    save_image(out_zynq, "out_zynq.png");

    printf("finish running Zynq code\n");

    for (int y = 0; y < out_zynq.height(); y++) {
        for (int x = 0; x < out_zynq.width(); x++) {
            if (out_native(x, y) != out_zynq(x, y)) {
                printf("out_native(%d, %d) = %d, but out_zynq(%d, %d) = %d\n",
                       x, y, out_native(x, y),
                       x, y, out_zynq(x, y));
            }
	}
    }
    return 0;
}
