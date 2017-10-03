#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include "pipeline_hls.h"
#include "pipeline_native.h"

#include "BufferMinimal.h"
#include "halide_image_io.h"

using Halide::Runtime::HLS::BufferMinimal;
using namespace Halide::Tools;

int main(int argc, char **argv) {
    BufferMinimal<uint8_t> input1 = load_image(argv[1]);
    BufferMinimal<uint8_t> input2 = load_image(argv[2]);
    BufferMinimal<uint8_t> out_native(720, 480, 3);
    BufferMinimal<uint8_t> out_hls(720, 480, 3);


    printf("start.\n");

    pipeline_native(input1, input2, out_native);
    save_image(out_native, "out.png");

    printf("finish running native code\n");

    pipeline_hls(input1, input2, out_hls);
    pipeline_native(input1, input2, out_native);
    save_image(out_hls, "out_hls.png");

    printf("finish running HLS code\n");

    bool success = true;
    for (int y = 0; y < out_hls.height(); y++) {
        for (int x = 0; x < out_hls.width(); x++) {
            for (int c = 0; c < out_hls.channels(); c++) {
                if (out_native(x, y, c) != out_hls(x, y, c)) {
                    printf("out_native(%d, %d, %d) = %d, but out_c(%d, %d, %d) = %d\n",
                           x, y, c, out_native(x, y, c),
                           x, y, c, out_hls(x, y, c));
                    success = false;
                }
            }
	}
    }
    if (success)
        return 0;
    else return 1;
}
