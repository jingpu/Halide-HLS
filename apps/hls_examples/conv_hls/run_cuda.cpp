#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include "benchmark.h"
#include "halide_image.h"
#include "halide_image_io.h"

#include "pipeline_native.h"
#include "pipeline_cuda.h"

#include "opencv2/gpu/gpu.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace Halide::Tools;
using namespace cv;

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

    /*
    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t2 = benchmark(iter, 10, [&]() {
        pipeline_cuda(input, weight, out_cuda);
      });
    printf("Halide CUDA program runtime: %g\n", min_t2 * 1e3);
    save_image(out_cuda, "out_cuda.png");
    */

    Mat cv_input = imread(argv[1], CV_LOAD_IMAGE_COLOR);
    Mat cv_input_rgba; // gpu::filter2D only support 1 or 4 channels
    cvtColor(cv_input, cv_input_rgba, CV_BGR2RGBA, 4);
    gpu::GpuMat d_input(cv_input_rgba);
    gpu::GpuMat d_out(d_input.size(), d_input.type());
    gpu::GpuMat d_out2(d_input.size(), d_input.type());

    Mat kernel = getGaussianKernel(5, -1, CV_32F);
    double min_t3 = benchmark(iter, 10, [&]() {
            gpu::filter2D(d_input, d_out, -1, kernel);
            gpu::filter2D(d_out, d_out2, -1, kernel);
      });
    printf("OpenCV CUDA program runtime: %g\n", min_t3 * 1e3);

    return 0;
}
