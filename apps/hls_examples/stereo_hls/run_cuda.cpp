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
    printf("Halide CUDA program runtime: %g\n", min_t2 * 1e3);
    save_image(out_cuda, "out_cuda.png");

    Mat cv_left = imread( argv[1], CV_LOAD_IMAGE_GRAYSCALE );
    Mat cv_right = imread( argv[2], CV_LOAD_IMAGE_GRAYSCALE );

    gpu::GpuMat d_left(cv_left);
    gpu::GpuMat d_right(cv_right);
    gpu::GpuMat d_disp(cv_left.size(), CV_8U);

    int ndisparities = 16*4;   /**< Range of disparity */
    int SADWindowSize = 9; /**< Size of the block window. Must be odd */
    gpu::StereoBM_GPU bm(gpu::StereoBM_GPU::BASIC_PRESET, ndisparities, SADWindowSize);

    double min_t3 = benchmark(iter, 10, [&]() {
            bm(d_left, d_right, d_disp);
      });
    printf("OpenCV CUDA program runtime: %g\n", min_t3 * 1e3);
    //save_image(out_cuda, "out_cuda.png");

    return 0;
}
