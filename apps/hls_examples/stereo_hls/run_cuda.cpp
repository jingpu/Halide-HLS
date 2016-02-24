#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include "pipeline_cuda.h"
#include "pipeline_native.h"

#include "benchmark.h"
#include "halide_image.h"
#include "halide_image_io.h"

using namespace Halide::Tools;

int main(int argc, char **argv) {
    if (argc < 5) {
        printf("Usage: ./run left.png left-remap.png right0224.png right-remap.png\n");
        return 0;
    }

    int iter = 5;
    Image<uint8_t> left = load_image(argv[1]);
    Image<uint8_t> left_remap = load_image(argv[2]);
    Image<uint8_t> right = load_image(argv[3]);
    Image<uint8_t> right_remap = load_image(argv[4]);

    Image<uint8_t> out_native(left.width(), left.height());
    Image<uint8_t> out_cuda(left.width(), left.height());

    printf("\nstart timing code...\n");

    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t = benchmark(iter, 10, [&]() {
        pipeline_native(right, left, right_remap, left_remap, out_native);
      });
    printf("CPU program runtime: %g\n", min_t * 1e3);
    save_image(out_native, "out_native.png");

    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t2 = benchmark(iter, 10, [&]() {
        pipeline_cuda(right, left, right_remap, left_remap, out_cuda);
      });
    printf(" program runtime: %g\n", min_t2 * 1e3);
    save_image(out_cuda, "out_cuda.png");

    return 0;
}
