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
    Image<uint8_t> out_native(input.width()-8, input.height()-8, 3);
    Image<uint8_t> out_hls(480*1, 640*1, 3);

    printf("start.\n");

    pipeline_native(input, out_native);
    save_image(out_native, "out.png");

    printf("finish running native code\n");
    pipeline_hls(input, out_hls);

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
