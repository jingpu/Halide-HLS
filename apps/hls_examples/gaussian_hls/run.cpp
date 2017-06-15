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
    Buffer<uint8_t> input = load_image(argv[1]);
    Buffer<uint8_t> out_native(input.width()-8, input.height()-8);
    Buffer<uint8_t> out_hls(64, 64);

    printf("start.\n");

    pipeline_native(input, out_native);
    save_image(out_native, "out.png");

    printf("finish running native code\n");
    pipeline_hls(input, out_hls);

    printf("finish running HLS code\n");

    unsigned fails = 0;
    for (int y = 0; y < out_hls.height(); y++) {
        for (int x = 0; x < out_hls.width(); x++) {
            for (int c = 0; c < out_hls.channels(); c++) {
                if (out_native(x, y, c) != out_hls(x, y, c)) {
                    printf("out_native(%d, %d, %d) = %d, but out_c(%d, %d, %d) = %d\n",
                           x, y, c, out_native(x, y, c),
                           x, y, c, out_hls(x, y, c));
                    fails++;
                }
            }
	}
    }
    if (!fails) {
        printf("passed.\n");
        return 0;
    } else  {
        printf("%u fails.\n", fails);
        return 1;
    }
    return 0;
}
