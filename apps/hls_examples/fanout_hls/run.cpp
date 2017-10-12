#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "pipeline_hls.h"
#include "pipeline_native.h"

#include "BufferMinimal.h"
#include "halide_image_io.h"

using Halide::Runtime::HLS::BufferMinimal;
using namespace Halide::Tools;


int main(int argc, char **argv) {
    BufferMinimal<uint8_t> in(200);

    BufferMinimal<uint8_t> out_native(in.width());
    BufferMinimal<uint8_t> out_hls(in.width());

    for (int x = 0; x < in.width(); x++) {
        in(x) = (uint8_t) rand();
    }

    printf("start.\n");

    pipeline_native(in, out_native);

    printf("finish running native code\n");

    pipeline_hls(in, out_hls);

    printf("finish running HLS code\n");

    bool success = true;
        for (int x = 0; x < out_native.width(); x++) {
            if (out_native(x) != out_hls(x)) {
                printf("out_native(%d) = %d, but out_c(%d) = %d\n",
                       x, out_native(x),
                       x, out_hls(x));
                success = false;
            }
        }

    if (success) {
        printf("Successed!\n");
        return 0;
    } else {
        printf("Failed!\n");
        return 1;
    }

}
