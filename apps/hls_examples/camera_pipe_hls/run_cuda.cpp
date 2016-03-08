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
        printf("Usage: ./process raw.png color_temp gamma contrast timing_iterations output.png\n"
               "e.g. ./process raw.png 3200 2 50");
        return 0;
    }

    int iter = 5;
    Image<uint16_t> input = load_image(argv[1]);
    Image<uint8_t> out_native(2560, 1920, 3);
    Image<uint8_t> out_cuda(2560, 1920, 3);

    // These color matrices are for the sensor in the Nokia N900 and are
    // taken from the FCam source.
    float _matrix_3200[][4] = {{ 1.6697f, -0.2693f, -0.4004f, -42.4346f},
                                {-0.3576f,  1.0615f,  1.5949f, -37.1158f},
                                {-0.2175f, -1.8751f,  6.9640f, -26.6970f}};

    float _matrix_7000[][4] = {{ 2.2997f, -0.4478f,  0.1706f, -39.0923f},
                                {-0.3826f,  1.5906f, -0.2080f, -25.4311f},
                                {-0.0888f, -0.7344f,  2.2832f, -20.0826f}};
    Image<float> matrix_3200(4, 3), matrix_7000(4, 3);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            matrix_3200(j, i) = _matrix_3200[i][j];
            matrix_7000(j, i) = _matrix_7000[i][j];
        }
    }

    float color_temp = atof(argv[2]);
    float gamma = atof(argv[3]);
    float contrast = atof(argv[4]);

    printf("\nstart timing code...\n");

    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t = benchmark(iter, 10, [&]() {
        pipeline_native(color_temp, gamma, contrast, input, matrix_3200, matrix_7000, out_native);
      });
    printf("CPU program runtime: %g\n", min_t * 1e3);
    save_image(out_native, "out_native.png");

    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t2 = benchmark(iter, 10, [&]() {
        pipeline_cuda(color_temp, gamma, contrast, input, matrix_3200, matrix_7000, out_cuda);
      });
    printf("CUDA program runtime: %g\n", min_t2 * 1e3);
    save_image(out_cuda, "out_cuda.png");

    return 0;
}
