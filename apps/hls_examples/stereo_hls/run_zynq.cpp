#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include <fcntl.h>
#include <unistd.h>
#include "pipeline_zynq.h"
#include "pipeline_native.h"

#include "HalideBuffer.h"
#include "halide_image_io.h"

using namespace Halide::Tools;
using namespace Halide::Runtime;

extern "C" {
extern int halide_zynq_init();
}

int main(int argc, char **argv) {
    if (argc < 5) {
        printf("Usage: ./run left.png left-remap.png right0224.png right-remap.png\n");
        return 0;
    }
    // Initialize zynq runtime
    halide_zynq_init();

    Buffer<uint8_t> left = load_image(argv[1]);
    Buffer<uint8_t> left_remap = load_image(argv[2]);
    Buffer<uint8_t> right = load_image(argv[3]);
    Buffer<uint8_t> right_remap = load_image(argv[4]);

    Buffer<uint8_t> out_native(left.width(), left.height());
    Buffer<uint8_t> out_zynq(600*4, 400*8);

    printf("start.\n");

    pipeline_native(right, left, right_remap, left_remap, out_native);
    save_image(out_native, "out_native.png");
    printf("cpu program results saved.\n");
    //out_native = load_image("out_native.png");
    //printf("cpu program results loaded.\n");

    pipeline_zynq(right, left, right_remap, left_remap, out_zynq);
    save_image(out_zynq, "out_zynq.png");
    printf("accelerator program results saved.\n");

    printf("checking results...\n");
    int fails = 0;
    for (int y = 0; y < out_zynq.height(); y++) {
        for (int x = 0; x < out_zynq.width(); x++) {
            if (out_native(x, y) != 0 &&
                out_native(x, y) != out_zynq(x, y)) {
                fails++;
            }
	}
    }
    if (fails > 0) {
      printf("find %d fails.\n", fails);
    } else {
      printf("passed.\n");
    }

    return 0;
}
