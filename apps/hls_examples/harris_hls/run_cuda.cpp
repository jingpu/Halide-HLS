#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include "pipeline_cuda.h"
#include "pipeline_native.h"

#include "benchmark.h"
#include "halide_image.h"
#include "halide_image_io.h"

#include "opencv2/gpu/gpu.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace Halide::Tools;
using namespace cv;

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: ./run input.png\n");
        return 0;
    }

    float k = 0.04;
    //float threshold = 100;

    int iter = 5;
    Image<uint8_t> input = load_image(argv[1]);
    Image<uint8_t> out_native(2400, 3200);
    Image<uint8_t> out_cuda(2400, 3200);

    printf("\nstart timing code...\n");
    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t = benchmark(iter, 10, [&]() {
        pipeline_native(input, out_native);
      });
    printf("CPU program runtime: %g\n", min_t * 1e3);
    save_image(out_native, "out_native.png");


    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t2 = benchmark(iter, 10, [&]() {
        pipeline_cuda(input, out_cuda);
      });
    printf("Halide CUDA program runtime: %g\n", min_t2 * 1e3);
    save_image(out_cuda, "out_cuda.png");


    Mat cv_input = imread( argv[1], CV_LOAD_IMAGE_GRAYSCALE );
    gpu::GpuMat d_input(cv_input);
    gpu::GpuMat d_out(cv_input.size(), CV_8U);

    int blockSize = 3;
    int kSize = 3;
    double min_t3 = benchmark(iter, 10, [&]() {
            gpu::cornerHarris(d_input, d_out, blockSize, kSize, k);
      });
    printf("OpenCV CUDA program runtime: %g\n", min_t3 * 1e3);

    return 0;
}
