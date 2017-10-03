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
    /*
    Halide::Func mosaic;
    Halide::BufferMinimal<uint8_t> input = load_image("../tutorial/images/rgb.png");
    Halide::Var x, y;

    // Create an RG/GB mosaic
    mosaic(x, y) = Halide::cast<uint8_t>(Halide::select((y % 2) == 0,
        Halide::select((x % 2) == 0, input(x, y, 0), input(x, y, 1)), // First row, RG
        Halide::select((x % 2) == 0, input(x, y, 1), input(x, y, 2)))); // GB

    Halide::BufferMinimal<uint8_t> mosaic_image = mosaic.realize(input.width(), input.height());
    save_image(mosaic_image, "mosaic.png");
    */

    BufferMinimal<uint8_t> input = load_image(argv[1]);
    BufferMinimal<uint8_t> out_native(1440/2, 960/2, 3);
    BufferMinimal<uint8_t> out_hls(1440/2, 960/2, 3);


    printf("start.\n");

    pipeline_native(input, out_native);
    save_image(out_native, "out.png");

    printf("finish running native code\n");

    pipeline_hls(input, out_hls);

    printf("finish running HLS code\n");

    bool success = true;
    for (int y = 0; y < out_native.height(); y++) {
        for (int x = 0; x < out_native.width(); x++) {
            if (fabs(out_native(x, y, 0) - out_hls(x, y, 0)) > 1e-4) {
                printf("out_native(%d, %d, 0) = %d, but out_c(%d, %d, 0) = %d\n",
                       x, y, out_native(x, y, 0),
                       x, y, out_hls(x, y, 0));
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
