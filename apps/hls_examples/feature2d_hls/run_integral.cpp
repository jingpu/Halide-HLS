#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include "halide_image.h"
#include "halide_image_io.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace Halide::Tools;
using namespace cv;

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: ./run_integral input.png\n");
        return 0;
    }

    Mat cv_input = imread( argv[1], CV_LOAD_IMAGE_GRAYSCALE );
    Mat sum;

    integral(cv_input, sum, CV_32S);

    imwrite("gray_integral.ppm", sum);

    return 0;
}
