#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include "pipeline_hls.h"
#include "pipeline_native.h"

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
    Image<uint8_t> out_hls(1024, 1024, 1);

    printf("start.\n");

    pipeline_native(input, out_native);
    save_image(out_native, argv[2]);

    printf("finish running native code\n");

    pipeline_hls(input, out_hls);

    printf("finish running HLS code\n");

    bool pass = true;
    for (int y = 0; y < out_native.height(); y++) {
        for (int x = 0; x < out_native.width(); x++) {
            if (out_native(x, y, 0) != out_hls(x, y, 0)) {
                printf("out_native(%d, %d, 0) = %d, but out_c(%d, %d, 0) = %d\n",
                       x, y, out_native(x, y, 0),
                       x, y, out_hls(x, y, 0));
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
