#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

//#include "pipeline_hls.h"
#include "pipeline_native.h"
#include "pipeline_corners.h"

#include "halide_image.h"
#include "halide_image_io.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/features2d/features2d.hpp"

using namespace Halide::Tools;
using namespace cv;

inline int smoothedSum(const Mat& sum, const KeyPoint& pt, int y, int x) 
{
    static const int HALF_KERNEL = 9 / 2;
    const int img_y = (int)(pt.pt.y + 0.5) + y;
    const int img_x = (int)(pt.pt.x + 0.5) + x;
    return   sum.at<int32_t>(img_y + HALF_KERNEL + 1, img_x + HALF_KERNEL + 1)
           - sum.at<int32_t>(img_y + HALF_KERNEL + 1, img_x - HALF_KERNEL)
           - sum.at<int32_t>(img_y - HALF_KERNEL, img_x + HALF_KERNEL + 1)
           + sum.at<int32_t>(img_y - HALF_KERNEL, img_x - HALF_KERNEL);
}
inline void compute_desc(const Mat& sum, const KeyPoint& pt, uchar* desc)  
{
    #define SMOOTHED(y,x) smoothedSum(sum, pt, y, x)
        desc[0] = (uchar)(((SMOOTHED(-2, -1) < SMOOTHED(7, -1)) << 7) + ((SMOOTHED(-14, -1) < SMOOTHED(-3, 3)) << 6) + ((SMOOTHED(1, -2) < SMOOTHED(11, 2)) << 5) + ((SMOOTHED(1, 6) < SMOOTHED(-10, -7)) << 4) + ((SMOOTHED(13, 2) < SMOOTHED(-1, 0)) << 3) + ((SMOOTHED(-14, 5) < SMOOTHED(5, -3)) << 2) + ((SMOOTHED(-2, 8) < SMOOTHED(2, 4)) << 1) + ((SMOOTHED(-11, 8) < SMOOTHED(-15, 5)) << 0));
        desc[1] = (uchar)(((SMOOTHED(-6, -23) < SMOOTHED(8, -9)) << 7) + ((SMOOTHED(-12, 6) < SMOOTHED(-10, 8)) << 6) + ((SMOOTHED(-3, -1) < SMOOTHED(8, 1)) << 5) + ((SMOOTHED(3, 6) < SMOOTHED(5, 6)) << 4) + ((SMOOTHED(-7, -6) < SMOOTHED(5, -5)) << 3) + ((SMOOTHED(22, -2) < SMOOTHED(-11, -8)) << 2) + ((SMOOTHED(14, 7) < SMOOTHED(8, 5)) << 1) + ((SMOOTHED(-1, 14) < SMOOTHED(-5, -14)) << 0));
        desc[2] = (uchar)(((SMOOTHED(-14, 9) < SMOOTHED(2, 0)) << 7) + ((SMOOTHED(7, -3) < SMOOTHED(22, 6)) << 6) + ((SMOOTHED(-6, 6) < SMOOTHED(-8, -5)) << 5) + ((SMOOTHED(-5, 9) < SMOOTHED(7, -1)) << 4) + ((SMOOTHED(-3, -7) < SMOOTHED(-10, -18)) << 3) + ((SMOOTHED(4, -5) < SMOOTHED(0, 11)) << 2) + ((SMOOTHED(2, 3) < SMOOTHED(9, 10)) << 1) + ((SMOOTHED(-10, 3) < SMOOTHED(4, 9)) << 0));
        desc[3] = (uchar)(((SMOOTHED(0, 12) < SMOOTHED(-3, 19)) << 7) + ((SMOOTHED(1, 15) < SMOOTHED(-11, -5)) << 6) + ((SMOOTHED(14, -1) < SMOOTHED(7, 8)) << 5) + ((SMOOTHED(7, -23) < SMOOTHED(-5, 5)) << 4) + ((SMOOTHED(0, -6) < SMOOTHED(-10, 17)) << 3) + ((SMOOTHED(13, -4) < SMOOTHED(-3, -4)) << 2) + ((SMOOTHED(-12, 1) < SMOOTHED(-12, 2)) << 1) + ((SMOOTHED(0, 8) < SMOOTHED(3, 22)) << 0));
        desc[4] = (uchar)(((SMOOTHED(-13, 13) < SMOOTHED(3, -1)) << 7) + ((SMOOTHED(-16, 17) < SMOOTHED(6, 10)) << 6) + ((SMOOTHED(7, 15) < SMOOTHED(-5, 0)) << 5) + ((SMOOTHED(2, -12) < SMOOTHED(19, -2)) << 4) + ((SMOOTHED(3, -6) < SMOOTHED(-4, -15)) << 3) + ((SMOOTHED(8, 3) < SMOOTHED(0, 14)) << 2) + ((SMOOTHED(4, -11) < SMOOTHED(5, 5)) << 1) + ((SMOOTHED(11, -7) < SMOOTHED(7, 1)) << 0));
        desc[5] = (uchar)(((SMOOTHED(6, 12) < SMOOTHED(21, 3)) << 7) + ((SMOOTHED(-3, 2) < SMOOTHED(14, 1)) << 6) + ((SMOOTHED(5, 1) < SMOOTHED(-5, 11)) << 5) + ((SMOOTHED(3, -17) < SMOOTHED(-6, 2)) << 4) + ((SMOOTHED(6, 8) < SMOOTHED(5, -10)) << 3) + ((SMOOTHED(-14, -2) < SMOOTHED(0, 4)) << 2) + ((SMOOTHED(5, -7) < SMOOTHED(-6, 5)) << 1) + ((SMOOTHED(10, 4) < SMOOTHED(4, -7)) << 0));
        desc[6] = (uchar)(((SMOOTHED(22, 0) < SMOOTHED(7, -18)) << 7) + ((SMOOTHED(-1, -3) < SMOOTHED(0, 18)) << 6) + ((SMOOTHED(-4, 22) < SMOOTHED(-5, 3)) << 5) + ((SMOOTHED(1, -7) < SMOOTHED(2, -3)) << 4) + ((SMOOTHED(19, -20) < SMOOTHED(17, -2)) << 3) + ((SMOOTHED(3, -10) < SMOOTHED(-8, 24)) << 2) + ((SMOOTHED(-5, -14) < SMOOTHED(7, 5)) << 1) + ((SMOOTHED(-2, 12) < SMOOTHED(-4, -15)) << 0));
        desc[7] = (uchar)(((SMOOTHED(4, 12) < SMOOTHED(0, -19)) << 7) + ((SMOOTHED(20, 13) < SMOOTHED(3, 5)) << 6) + ((SMOOTHED(-8, -12) < SMOOTHED(5, 0)) << 5) + ((SMOOTHED(-5, 6) < SMOOTHED(-7, -11)) << 4) + ((SMOOTHED(6, -11) < SMOOTHED(-3, -22)) << 3) + ((SMOOTHED(15, 4) < SMOOTHED(10, 1)) << 2) + ((SMOOTHED(-7, -4) < SMOOTHED(15, -6)) << 1) + ((SMOOTHED(5, 10) < SMOOTHED(0, 24)) << 0));
        desc[8] = (uchar)(((SMOOTHED(3, 6) < SMOOTHED(22, -2)) << 7) + ((SMOOTHED(-13, 14) < SMOOTHED(4, -4)) << 6) + ((SMOOTHED(-13, 8) < SMOOTHED(-18, -22)) << 5) + ((SMOOTHED(-1, -1) < SMOOTHED(-7, 3)) << 4) + ((SMOOTHED(-19, -12) < SMOOTHED(4, 3)) << 3) + ((SMOOTHED(8, 10) < SMOOTHED(13, -2)) << 2) + ((SMOOTHED(-6, -1) < SMOOTHED(-6, -5)) << 1) + ((SMOOTHED(2, -21) < SMOOTHED(-3, 2)) << 0));
        desc[9] = (uchar)(((SMOOTHED(4, -7) < SMOOTHED(0, 16)) << 7) + ((SMOOTHED(-6, -5) < SMOOTHED(-12, -1)) << 6) + ((SMOOTHED(1, -1) < SMOOTHED(9, 18)) << 5) + ((SMOOTHED(-7, 10) < SMOOTHED(-11, 6)) << 4) + ((SMOOTHED(4, 3) < SMOOTHED(19, -7)) << 3) + ((SMOOTHED(-18, 5) < SMOOTHED(-4, 5)) << 2) + ((SMOOTHED(4, 0) < SMOOTHED(-20, 4)) << 1) + ((SMOOTHED(7, -11) < SMOOTHED(18, 12)) << 0));
        desc[10] = (uchar)(((SMOOTHED(-20, 17) < SMOOTHED(-18, 7)) << 7) + ((SMOOTHED(2, 15) < SMOOTHED(19, -11)) << 6) + ((SMOOTHED(-18, 6) < SMOOTHED(-7, 3)) << 5) + ((SMOOTHED(-4, 1) < SMOOTHED(-14, 13)) << 4) + ((SMOOTHED(17, 3) < SMOOTHED(2, -8)) << 3) + ((SMOOTHED(-7, 2) < SMOOTHED(1, 6)) << 2) + ((SMOOTHED(17, -9) < SMOOTHED(-2, 8)) << 1) + ((SMOOTHED(-8, -6) < SMOOTHED(-1, 12)) << 0));
        desc[11] = (uchar)(((SMOOTHED(-2, 4) < SMOOTHED(-1, 6)) << 7) + ((SMOOTHED(-2, 7) < SMOOTHED(6, 8)) << 6) + ((SMOOTHED(-8, -1) < SMOOTHED(-7, -9)) << 5) + ((SMOOTHED(8, -9) < SMOOTHED(15, 0)) << 4) + ((SMOOTHED(0, 22) < SMOOTHED(-4, -15)) << 3) + ((SMOOTHED(-14, -1) < SMOOTHED(3, -2)) << 2) + ((SMOOTHED(-7, -4) < SMOOTHED(17, -7)) << 1) + ((SMOOTHED(-8, -2) < SMOOTHED(9, -4)) << 0));
        desc[12] = (uchar)(((SMOOTHED(5, -7) < SMOOTHED(7, 7)) << 7) + ((SMOOTHED(-5, 13) < SMOOTHED(-8, 11)) << 6) + ((SMOOTHED(11, -4) < SMOOTHED(0, 8)) << 5) + ((SMOOTHED(5, -11) < SMOOTHED(-9, -6)) << 4) + ((SMOOTHED(2, -6) < SMOOTHED(3, -20)) << 3) + ((SMOOTHED(-6, 2) < SMOOTHED(6, 10)) << 2) + ((SMOOTHED(-6, -6) < SMOOTHED(-15, 7)) << 1) + ((SMOOTHED(-6, -3) < SMOOTHED(2, 1)) << 0));
        desc[13] = (uchar)(((SMOOTHED(11, 0) < SMOOTHED(-3, 2)) << 7) + ((SMOOTHED(7, -12) < SMOOTHED(14, 5)) << 6) + ((SMOOTHED(0, -7) < SMOOTHED(-1, -1)) << 5) + ((SMOOTHED(-16, 0) < SMOOTHED(6, 8)) << 4) + ((SMOOTHED(22, 11) < SMOOTHED(0, -3)) << 3) + ((SMOOTHED(19, 0) < SMOOTHED(5, -17)) << 2) + ((SMOOTHED(-23, -14) < SMOOTHED(-13, -19)) << 1) + ((SMOOTHED(-8, 10) < SMOOTHED(-11, -2)) << 0));
        desc[14] = (uchar)(((SMOOTHED(-11, 6) < SMOOTHED(-10, 13)) << 7) + ((SMOOTHED(1, -7) < SMOOTHED(14, 0)) << 6) + ((SMOOTHED(-12, 1) < SMOOTHED(-5, -5)) << 5) + ((SMOOTHED(4, 7) < SMOOTHED(8, -1)) << 4) + ((SMOOTHED(-1, -5) < SMOOTHED(15, 2)) << 3) + ((SMOOTHED(-3, -1) < SMOOTHED(7, -10)) << 2) + ((SMOOTHED(3, -6) < SMOOTHED(10, -18)) << 1) + ((SMOOTHED(-7, -13) < SMOOTHED(-13, 10)) << 0));
        desc[15] = (uchar)(((SMOOTHED(1, -1) < SMOOTHED(13, -10)) << 7) + ((SMOOTHED(-19, 14) < SMOOTHED(8, -14)) << 6) + ((SMOOTHED(-4, -13) < SMOOTHED(7, 1)) << 5) + ((SMOOTHED(1, -2) < SMOOTHED(12, -7)) << 4) + ((SMOOTHED(3, -5) < SMOOTHED(1, -5)) << 3) + ((SMOOTHED(-2, -2) < SMOOTHED(8, -10)) << 2) + ((SMOOTHED(2, 14) < SMOOTHED(8, 7)) << 1) + ((SMOOTHED(3, 9) < SMOOTHED(8, 2)) << 0));
    //printf("middle: %d\n", sum.at<int32_t>((int)(pt.pt.y+0.5)+3, (int)(pt.pt.x+0.5)+4));
    #undef SMOOTHED
}

extern "C" int brief(buffer_t *input, int col, int row, buffer_t *out) 
{
    if (input->host == nullptr ) {
   	input->min[0] = 0;
        input->min[1] = 0;
        input->extent[0] = col;
        input->extent[1] = row;
    }else {
        assert(out->host);
        uint8_t *corners = (uint8_t *)input->host;
        corners -= input->min[0] * input->stride[0];
        corners -= input->min[1] * input->stride[1];
        std::vector<KeyPoint> keypoints;
        for (int y = 0; y < row; y++) {
            for (int x = 0; x < col; x++) {
                uint8_t value = corners[x * input->stride[0] + y * input->stride[1]];
                if (value == 255) {
                    KeyPoint kpt((float)x, (float)y, 4, -1, 255.0);
                    keypoints.push_back(kpt);
                }
            }
        }
        //printf("keypoints number: %d\n", (int)keypoints.size());
       // printf("out dimension: %d\n", out->extent[1]);
        //uint8_t *gray_img = (uint8_t *)img->host;
        //printf("input_img: %u\n", gray_img[0] );       
        //gray_img -= img->min[0] * img->stride[0];
        //gray_img -= img->min[1] * img->stride[1];
        //Mat cv_input = Mat(img->extent[0], img->extent[1], CV_8UC1, gray_img);
        //Mat input_trans = cv_input.t();
        Mat cv_input = imread("../../images/gray_small.png", CV_LOAD_IMAGE_GRAYSCALE ); //TODO
        Mat sum;
        //sum.create((int)img->extent[1], (int)img->extent[0], CV_32S);
        //sum.setTo(Scalar::all(0));
        integral(cv_input, sum, CV_32S);
        //printf("input_img: %u\n", cv_input.at<uint8_t>(120, 36) );       
        //printf("input_img: %u\n", cv_input.at<uint8_t>(131, 37) );       
        //printf("input_img: %u\n", cv_input.at<uint8_t>(51, 60) );       
        //printf("integral: %d\n", sum.at<int32_t>(120, 36) );       
        //printf("integral: %d\n", sum.at<int32_t>(131, 37) );       
 
        int num_kpt = out->extent[1];
        uint8_t *dst = (uint8_t *)(out->host);
        memset(dst, 0, num_kpt*16*sizeof(uint8_t));
        //Mat descriptors;
        //descriptors.create(20, 16, CV_8U, Scalar::all(0)); //use 16 as the descriptor size
        //Matx21f R;
        for (int i = 0; i < num_kpt ; ++i) {
            //uchar* desc = descriptor.ptr(i);
            uchar* desc = dst + 16 * i;
            const KeyPoint& pt = keypoints[i];
            //printf("before %u\n", desc[0]);
            //printf("x: %d, y: %d\n", (int)pt.pt.x, (int)pt.pt.y);
            compute_desc(sum, pt, desc);  
            //printf("after %u\n", desc[0]);
        }
    }
    return 0;
} 


int main(int argc, char **argv) {

    //float k = 0.04;
    //float threshold = 100;

    Image<uint8_t> input = load_image(argv[1]);
    Image<uint8_t> out_native0(16, 640);
    Image<uint8_t> out_native1(16, 640);
    Image<uint8_t> out_corners0(input.width(), input.height(), input.channels());
    Image<uint8_t> out_corners1(input.width(), input.height(), input.channels());
    //Image<uint8_t> out_native(input.width(), input.height(), input.channels());
    //Image<uint8_t> out_hls(input.width(), input.height(), input.channels());

    printf("start.\n");

    pipeline_native(input, out_native0);
    pipeline_native(input, out_native1);
    pipeline_corners(input, out_corners0);
    pipeline_corners(input, out_corners1);
    //save_image(out_native, "out.png");

    printf("finished running native code\n");

    for (int y = 0; y < out_native0.height(); y++) {
        for (int x = 0; x < out_native0.width(); x++) {
            if (fabs(out_native0(x, y, 0) - out_native1(x, y, 0)) > 1e-4) {
                printf("out_native0(%d, %d, 0) = %d, but out_native1(%d, %d, 0) = %d\n",
                       x, y, out_native0(x, y, 0),
                       x, y, out_native1(x, y, 0));
                return 1;
            }
        }
    }


    std::vector<KeyPoint> keypoints0;
    std::vector<KeyPoint> keypoints1;
    for (int y = 0; y < input.height(); y++) {
        for (int x = 0; x < input.width(); x++) {
            uint8_t value0 = out_corners0(x, y, 0);
            uint8_t value1 = out_corners1(x, y, 0);
            if (value0 == 255) {
                KeyPoint kpt((float)x, (float)y, 4, -1, 255.0);
                keypoints0.push_back(kpt);
            }
            if (value1 == 255) {
                KeyPoint kpt((float)x, (float)y, 4, -1, 255.0);
                keypoints1.push_back(kpt);
            } 
        }
    }


    buffer_t* out_host0 = (buffer_t *)out_native0;
    buffer_t* out_host1 = (buffer_t *)out_native1;
    uint8_t *out0 = (uint8_t *)(out_host0->host);
    uint8_t *out1 = (uint8_t *)(out_host1->host);
    Mat desc0 = Mat(out_native0.width(), out_native0.height(), CV_8UC1, out0);
    Mat desc1 = Mat(out_native1.width(), out_native1.height(), CV_8UC1, out1);
    
    BFMatcher matcher(NORM_HAMMING);
    std::vector<DMatch> matches;
    matcher.match(desc0.t(), desc1.t(), matches);

    Mat img_matches;
    Mat cv_img = imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE);
    drawMatches(cv_img, keypoints0, cv_img, keypoints1, matches, img_matches);

    imshow("Matches", img_matches);
    waitKey(0);

/*    pipeline_hls(input, out_hls);
    save_image(out_hls, "out_hls.png");

    printf("finished running HLS code\n");

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
    else return 1;*/
    return 0;
}
