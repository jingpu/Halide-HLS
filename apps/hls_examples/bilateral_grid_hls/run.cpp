#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include "pipeline_hls.h"
#include "pipeline_native.h"

#include "HalideBuffer.h"
#include "halide_image_io.h"

using namespace Halide;
using namespace Halide::Tools;

int main(int argc, char **argv) {
    Image<uint8_t> input = load_image(argv[1]);
    Image<uint8_t> out_native(480*4, 640*4);
    Image<uint8_t> out_hls(480, 640);

    printf("start.\n");

    pipeline_native(input, out_native);
    save_image(out_native, "out.png");

    printf("finish running native code\n");

    pipeline_hls(input, out_hls);

    printf("finish running HLS code\n");

    bool pass = true;
    for (int y = 0; y < out_hls.height(); y++) {
        for (int x = 0; x < out_hls.width(); x++) {
            if (out_native(x, y) != out_hls(x, y)) {
                printf("out_native(%d, %d) = %d, but out_c(%d, %d) = %d\n",
                       x, y, out_native(x, y),
                       x, y, out_hls(x, y));
                pass = false;
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
}
