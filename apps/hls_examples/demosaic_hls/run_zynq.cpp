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

    Image<uint8_t> input = load_image(argv[1]);
    fprintf(stderr, "%d %d\n", input.width(), input.height());
    Image<uint8_t> out_native(1440/2, 960/2, 3);
    Image<uint8_t> out_zynq(1440/2, 960/2, 3);

    printf("start.\n");

    pipeline_native(input, out_native);
    save_image(out_native, "out_native.png");
    printf("cpu program results saved.\n");
    //out_native = load_image("out_native.png");
    //printf("cpu program results loaded.\n");

    pipeline_zynq(input, out_zynq, hwacc, cma);
    save_image(out_zynq, "out_zynq.png");
    printf("accelerator program results saved.\n");

    printf("checking results...\n");

    unsigned fails = 0;
    for (int y = 0; y < out_zynq.height(); y++) {
        for (int x = 0; x < out_zynq.width(); x++) {
            for (int c = 0; c < 3; c++) {
                if (out_native(x, y, c) != out_zynq(x, y, c)) {
                    printf("out_native(%d, %d, %d) = %d, but out_zynq(%d, %d, %d) = %d\n",
                           x, y, c, out_native(x, y, c),
                           x, y, c, out_zynq(x, y, c));
                    fails++;
                }
            }
	}
    }
    if (!fails) {
        printf("passed.\n");
    } else  {
        printf("%u fails.\n", fails);
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
    double min_t2 = benchmark(5, 20, [&]() {
            pipeline_zynq(input, out_zynq, hwacc, cma);
        });
    printf("accelerator program runtime: %g\n", min_t2 * 1e3);

    close(hwacc);
    close(cma);
    return 0;
}
