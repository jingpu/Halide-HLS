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

    int hwacc = open("/dev/hwacc0", O_RDWR);
    if(hwacc == -1) {
        printf("Failed to open hardware device!\\n");
        return(0);
    }

    Image<uint8_t> input = load_image(argv[1]);
    Image<uint8_t> out_native(480*4, 640*4);
    Image<uint8_t> out_zynq(480*4, 640*4);

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
            if (out_native(x, y) != out_zynq(x, y)) {
                printf("out_native(%d, %d) = %d, but out_zynq(%d, %d) = %d\n",
                       x, y, out_native(x, y),
                       x, y, out_zynq(x, y));
		fails++;
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
    double min_t = benchmark(3, 10, [&]() {
        pipeline_native(input, out_native);
      });
    printf("CPU program runtime: %g\n", min_t * 1e3);

    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t2 = benchmark(10, 20, [&]() {
        pipeline_zynq(input, out_zynq, hwacc, cma);
      });
    printf("accelerator program runtime: %g\n", min_t2 * 1e3);

    close(hwacc);
    close(cma);
    return 0;
}
