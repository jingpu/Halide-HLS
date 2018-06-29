#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include "pipeline_hls_seedark.h"
#include "seedark_pipeline.h"

#include "HalideBuffer.h"
#include "halide_image_io.h"

using namespace Halide::Runtime;
using namespace Halide::Tools;

int main(int argc, char **argv) {
    Buffer<uint8_t> input1 = load_image(argv[1]);
    Buffer<uint8_t> input2 = load_image(argv[2]);
    Buffer<uint8_t> weight(2, 1);
    weight(0) = 5;
    weight(1) = 95;
    
    Buffer<uint8_t> out_native(input1.width(), input1.height(), 3);
    Buffer<uint8_t> out_hls(input1.width(), input1.height(), 3);

    printf("start.\n");

    blend_cpu(input1, input2, weight, out_native);
    save_image(out_native, "out.png");

    printf("finish running native code\n");
    pipeline_hls(input1, input2, weight, out_hls);

    printf("finish running HLS code\n");

    bool pass = true;
    for (int y = 0; y < out_hls.height(); y++) {
        for (int x = 0; x < out_hls.width(); x++) {
            for (int c = 0; c < out_hls.channels(); c++) {
                if (out_native(x, y, c) != out_hls(x, y, c)) {
                    printf("out_native(%d, %d, %d) = %d, but out_c(%d, %d, %d) = %d\n",
                           x, y, c, out_native(x, y, c),
                           x, y, c, out_hls(x, y, c));
                    pass = false;
                }
          }
	}
    }
    if (pass) {
        printf("passed.\n");
        return 0;
    } else  {
        printf("failed.\n");
        return 1;
    }
    return 0;
}
