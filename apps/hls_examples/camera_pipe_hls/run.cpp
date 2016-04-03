#include <arpa/inet.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include "pipeline_hls.h"
#include "pipeline_native.h"

#include "halide_image.h"
#include "halide_image_io.h"

using namespace Halide::Tools;

int main(int argc, char **argv) {
    Image<uint16_t> input = load_image(argv[1]);
    fprintf(stderr, "%d %d\n", input.width(), input.height());
    Image<uint8_t> out_native(2560, 1920, 3);
    Image<uint8_t> out_hls(3, 640*2, 480*2);

    printf("start.\n");
    pipeline_native(input, out_native);
    save_image(out_native, "out.png");
    printf("finish running native code\n");

    pipeline_hls(input, out_hls);

    printf("finish running HLS code\n");

    for (int y = 0; y < out_hls.extent(2); y++) {
        for (int x = 0; x < out_hls.extent(1); x++) {
            for (int c = 0; c < out_hls.extent(0); c++) {
                if (out_native(x, y, c) != out_hls(c, x, y)) {
                    printf("out_native(%d, %d, %d) = %d, but out_c(%d, %d, %d) = %d\n",
                           x, y, c, out_native(x, y, c),
                           c, x, y, out_hls(c, x, y));
                }
            }
	}
    }


    return 0;
}
