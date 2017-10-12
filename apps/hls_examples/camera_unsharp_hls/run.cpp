#include <arpa/inet.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include "pipeline_hls.h"
#include "pipeline_native.h"

#include "BufferMinimal.h"
#include "halide_image_io.h"

using Halide::Runtime::HLS::BufferMinimal;
using namespace Halide::Tools;

int main(int argc, char **argv) {
    BufferMinimal<uint16_t> input = load_image(argv[1]);
    fprintf(stderr, "%d %d\n", input.width(), input.height());
    BufferMinimal<uint8_t> out_native(2560, 1920, 3);
    BufferMinimal<uint8_t> out_hls(64*1, 64*1, 3);  // Cropped

    printf("start.\n");
    pipeline_native(input, out_native);
    save_image(out_native, "out.png");
    printf("finish running native code\n");

    pipeline_hls(input, out_hls);
    save_image(out_hls, "out_hls.png");

    printf("finish running HLS code\n");

    bool pass = true;
    for (int y = 0; y < out_hls.height(); y++) {
        for (int x = 0; x < out_hls.width(); x++) {
            for (int c = 0; c < 3; c++) {
                if (out_native(x, y, c) != out_hls(x, y, c)) {
                    printf("out_native(%d, %d, %d) = %d, but out_hls(%d, %d, %d) = %d\n",
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
}
