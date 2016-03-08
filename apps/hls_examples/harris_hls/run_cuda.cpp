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
    if (argc < 2) {
        printf("Usage: ./run input.png\n");
        return 0;
    }

    float k = 0.04;
    float threshold = 100;

    int iter = 5;
    Image<uint8_t> input = load_image(argv[1]);
    Image<uint8_t> out_native(input.width(), input.height());
    Image<uint8_t> out_cuda(input.width(), input.height());

    printf("\nstart timing code...\n");

    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t = benchmark(iter, 10, [&]() {
        pipeline_native(input, k, threshold, out_native);
      });
    printf("CPU program runtime: %g\n", min_t * 1e3);
    save_image(out_native, "out_native.png");

    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t2 = benchmark(iter, 10, [&]() {
        pipeline_cuda(input, k, threshold, out_cuda);
      });
    printf("CUDA program runtime: %g\n", min_t2 * 1e3);
    save_image(out_cuda, "out_cuda.png");

    return 0;
}
