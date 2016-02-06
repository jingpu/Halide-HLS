#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include "pipeline_zynq.h"
#include "pipeline_native.h"

#include "benchmark.h"
#include "halide_image.h"
#include "halide_image_io.h"

using namespace Halide::Tools;

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: ./run input.png output.png\n");
        return 0;
    }

    Image<uint8_t> input = load_image(argv[1]);
    Image<uint8_t> out_native(1024, 1024, 1);
    Image<uint8_t> out_zynq(1024, 1024, 1);


    printf("start.\n");

    pipeline_native(input, out_native);
    save_image(out_native, argv[2]);
    printf("cpu program results saved.\n");
    //out_native = load_image("out_native.png");
    //printf("cpu program results loaded.\n");

    pipeline_zynq(input, out_zynq);
    save_image(out_zynq, "out_zynq.png");
    printf("accelerator program results saved.\n");

    printf("checking results...\n");
    bool pass = true;
    for (int y = 0; y < out_zynq.height(); y++) {
        for (int x = 0; x < out_zynq.width(); x++) {
            if (out_native(x, y) != out_zynq(x, y)) {
                printf("out_native(%d, %d) = %d, but out_zynq(%d, %d) = %d\n",
                       x, y, out_native(x, y),
                       x, y, out_zynq(x, y));
                pass = false;
            }
	}
    }
    if (!pass) {
      printf("failed.\n");
      return 1;
    } else {
      printf("passed.\n");
    }

    printf("\nstart timing code...\n");

    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t = benchmark(1, 10, [&]() {
        pipeline_native(input, out_native);
      });
    printf("CPU program runtime: %g\n", min_t * 1e3);

    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t2 = benchmark(1, 10, [&]() {
        pipeline_zynq(input, out_zynq);
      });
    printf("accelerator program runtime: %g\n", min_t2 * 1e3);

    return 0;
}
