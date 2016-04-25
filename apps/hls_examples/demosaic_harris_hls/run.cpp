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
    Image<uint8_t> input = load_image(argv[1]);
    Image<uint8_t> out_native(720, 480);
    Image<uint8_t> out_hls(720, 480);


    printf("start.\n");

    pipeline_native(input, out_native);
    save_image(out_native, "out.png");

    printf("finish running native code\n");

    pipeline_hls(input, out_hls);

    printf("finish running HLS code\n");

    bool success = true;
    for (int y = 0; y < out_native.height(); y++) {
        for (int x = 0; x < out_native.width(); x++) {
            if (out_native(x, y) != out_hls(x, y)) {
                printf("out_native(%d, %d) = %d, but out_c(%d, %d) = %d\n",
                       x, y, out_native(x, y),
                       x, y, out_hls(x, y));
		success = false;
            }
	}
    }
    if (success)
        return 0;
    else return 1;

    /*
    FILE *ptr;
    ptr = fopen("raw", "wb");
    uint8_t in_pixel[1];
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 640; x++) {
            in_pixel[0] = input(x, y);
            fwrite(in_pixel, sizeof(in_pixel), 1, ptr);
        }
    }
    fclose(ptr);

    ptr = fopen("dump_ref", "wb");
    uint8_t out_pixel[4];
    for (int y = 1; y < 1 + 480; y++) {
        for (int x = 1; x < 1 + 640; x++) {
            out_pixel[0] = out_native(x, y, 0);
            out_pixel[1] = out_native(x, y, 1);
            out_pixel[2] = out_native(x, y, 2);
            out_pixel[3] = 0;
            fwrite(out_pixel, sizeof(out_pixel), 1, ptr);
        }
    }
    fclose(ptr);
    */
}
