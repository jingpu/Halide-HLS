#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include "benchmark.h"
#include "halide_image.h"
#include "halide_image_io.h"

#include "pipeline_native.h"
#include "pipeline_cuda.h"

using namespace Halide::Tools;

const unsigned char gaussian2d[5][5] = {
    {1,     3,     6,     3,     1},
    {3,    15,    25,    15,     3},
    {6,    25,    44,    25,     6},
    {3,    15,    25,    15,     3},
    {1,     3,     6,     3,     1}
};

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: ./run input.png\n");
        return 0;
    }

    int iter = 5;
    Image<uint8_t> input = load_image(argv[1]);
    Image<uint8_t> weight(5, 5);

    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 5; x++)
            weight(x, y) = gaussian2d[y][x];

    Image<uint8_t> out_native(input.width(), input.height(), input.channels());
    Image<uint8_t> out_cuda(input.width(), input.height(), input.channels());

    printf("\nstart timing code...\n");

    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t = benchmark(iter, 10, [&]() {
        pipeline_native(input, weight, out_native);
      });
    printf("CPU program runtime: %g\n", min_t * 1e3);
    save_image(out_native, "out_native.png");

    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t3 = benchmark(iter, 10, [&]() {
        pipeline_cuda(input, weight, out_cuda);
      });
    printf("CUDA program runtime: %g\n", min_t3 * 1e3);
    save_image(out_cuda, "out_cuda.png");

    return 0;
}
