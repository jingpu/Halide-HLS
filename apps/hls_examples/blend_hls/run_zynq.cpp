#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include <fcntl.h>
#include <unistd.h>
#include "pipeline_zynq_seedark.h"
#include "seedark_pipeline.h"

#include "HalideBuffer.h"
#include "halide_image_io.h"

using namespace Halide::Tools;
using namespace Halide::Runtime;

extern "C" {
extern int halide_zynq_init();
}

using namespace Halide::Tools;

int main(int argc, char **argv) {
    // Open the buffer allocation device
    halide_zynq_init();

    Buffer<uint8_t> input1 = load_image(argv[1]);
    Buffer<uint8_t> input2 = load_image(argv[2]);
    Buffer<uint8_t> weight(2, 1);
    weight(0) = 5;
    weight(1) = 95;
    
    Buffer<uint8_t> out_native(input1.width(), input1.height(), 3);
    Buffer<uint8_t> out_zynq(input1.width(), input1.height(), 3);
    printf("start.\n");

    blend_cpu(input1, input2, weight, out_native);
    save_image(out_native, "out.png");
    printf("cpu program results saved.\n");

    pipeline_zynq(input1, input2, weight, out_zynq);
    save_image(out_zynq, "out_zynq.png");
    printf("accelerator program results saved.\n");

    printf("checking results...\n");
    unsigned fails = 0;
    for (int y = 0; y < out_zynq.height(); y++) {
        for (int x = 0; x < out_zynq.width(); x++) {
            for (int c = 0; c < out_zynq.channels(); c++) {
                if (out_native(x, y, c) != out_zynq(x, y, c)) {
                    printf("out_native(%d, %d, %d) = %d, but out_c(%d, %d, %d) = %d\n",
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

    return 0;
}
