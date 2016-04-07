#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include <fcntl.h>
#include <unistd.h>
#include "pipeline_zynq.h"
#include "pipeline_native.h"

#include "benchmark.h"
#include "halide_image.h"
#include "halide_image_io.h"

using namespace Halide::Tools;

int main(int argc, char **argv) {
    if (argc < 5) {
        printf("Usage: ./run left.png left-remap.png right0224.png right-remap.png\n");
        return 0;
    }
    // Open the buffer allocation device
    int cma = open("/dev/cmabuffer0", O_RDWR);
    if(cma == -1){
        printf("Failed to open cma provider!\n");
        return(0);
    }

    // open the hardware
    int hwacc = open("/dev/hwacc0", O_RDWR);
    if(hwacc == -1) {
        printf("Failed to open hardware device!\n");
        return(0);
    }

    Image<uint8_t> left = load_image(argv[1]);
    Image<uint8_t> left_remap = load_image(argv[2]);
    Image<uint8_t> right = load_image(argv[3]);
    Image<uint8_t> right_remap = load_image(argv[4]);

    Image<uint8_t> out_native(left.width(), left.height());
    Image<uint8_t> out_zynq(600*4, 400*8);

    printf("start.\n");

    pipeline_native(right, left, right_remap, left_remap, out_native);
    save_image(out_native, "out_native.png");
    printf("cpu program results saved.\n");
    //out_native = load_image("out_native.png");
    //printf("cpu program results loaded.\n");

    pipeline_zynq(right, left, right_remap, left_remap, out_zynq, hwacc, cma);
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
    printf("\nstart timing code...\n");

    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t = benchmark(1, 1, [&]() {
            pipeline_native(right, left, right_remap, left_remap, out_native);
      });
    printf("CPU program runtime: %g\n", min_t * 1e3);

    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t2 = benchmark(5, 10, [&]() {
       pipeline_zynq(right, left, right_remap, left_remap, out_zynq, hwacc, cma);
      });
    printf("accelerator program runtime: %g\n", min_t2 * 1e3);

    close(hwacc);
    close(cma);
    return 0;
}
