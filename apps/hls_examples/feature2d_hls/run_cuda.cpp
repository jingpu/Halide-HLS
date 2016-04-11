#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>
#include <algorithm>
#include <vector>
#include <iostream>
#include <sys/time.h>
//#include <omp.h>

#include "pipeline_native.h"
#include "pipeline_corners.h"
#include "pipeline_cuda.h"

#include "benchmark.h"
#include "halide_image.h"
#include "halide_image_io.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/calib3d/calib3d.hpp"

using namespace Halide::Tools;
using namespace cv;

extern "C" void halide_print(void *user_context, const char *msg) {
   printf("test");
   printf("%s\n", msg);
}


struct KeypointResponseGreater
{
    inline bool operator()(const KeyPoint& kp1, const KeyPoint& kp2) const
    {
        return kp1.response > kp2.response;
    }
};


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
    desc[16] = (uchar)(((SMOOTHED(-9, 1) < SMOOTHED(-18, 0)) << 7) + ((SMOOTHED(4, 0) < SMOOTHED(1, 12)) << 6) + ((SMOOTHED(0, 9) < SMOOTHED(-14, -10)) << 5) + ((SMOOTHED(-13, -9) < SMOOTHED(-2, 6)) << 4) + ((SMOOTHED(1, 5) < SMOOTHED(10, 10)) << 3) + ((SMOOTHED(-3, -6) < SMOOTHED(-16, -5)) << 2) + ((SMOOTHED(11, 6) < SMOOTHED(-5, 0)) << 1) + ((SMOOTHED(-23, 10) < SMOOTHED(1, 2)) << 0));
    desc[17] = (uchar)(((SMOOTHED(13, -5) < SMOOTHED(-3, 9)) << 7) + ((SMOOTHED(-4, -1) < SMOOTHED(-13, -5)) << 6) + ((SMOOTHED(10, 13) < SMOOTHED(-11, 8)) << 5) + ((SMOOTHED(19, 20) < SMOOTHED(-9, 2)) << 4) + ((SMOOTHED(4, -8) < SMOOTHED(0, -9)) << 3) + ((SMOOTHED(-14, 10) < SMOOTHED(15, 19)) << 2) + ((SMOOTHED(-14, -12) < SMOOTHED(-10, -3)) << 1) + ((SMOOTHED(-23, -3) < SMOOTHED(17, -2)) << 0));
    desc[18] = (uchar)(((SMOOTHED(-3, -11) < SMOOTHED(6, -14)) << 7) + ((SMOOTHED(19, -2) < SMOOTHED(-4, 2)) << 6) + ((SMOOTHED(-5, 5) < SMOOTHED(3, -13)) << 5) + ((SMOOTHED(2, -2) < SMOOTHED(-5, 4)) << 4) + ((SMOOTHED(17, 4) < SMOOTHED(17, -11)) << 3) + ((SMOOTHED(-7, -2) < SMOOTHED(1, 23)) << 2) + ((SMOOTHED(8, 13) < SMOOTHED(1, -16)) << 1) + ((SMOOTHED(-13, -5) < SMOOTHED(1, -17)) << 0));
    desc[19] = (uchar)(((SMOOTHED(4, 6) < SMOOTHED(-8, -3)) << 7) + ((SMOOTHED(-5, -9) < SMOOTHED(-2, -10)) << 6) + ((SMOOTHED(-9, 0) < SMOOTHED(-7, -2)) << 5) + ((SMOOTHED(5, 0) < SMOOTHED(5, 2)) << 4) + ((SMOOTHED(-4, -16) < SMOOTHED(6, 3)) << 3) + ((SMOOTHED(2, -15) < SMOOTHED(-2, 12)) << 2) + ((SMOOTHED(4, -1) < SMOOTHED(6, 2)) << 1) + ((SMOOTHED(1, 1) < SMOOTHED(-2, -8)) << 0));
    desc[20] = (uchar)(((SMOOTHED(-2, 12) < SMOOTHED(-5, -2)) << 7) + ((SMOOTHED(-8, 8) < SMOOTHED(-9, 9)) << 6) + ((SMOOTHED(2, -10) < SMOOTHED(3, 1)) << 5) + ((SMOOTHED(-4, 10) < SMOOTHED(-9, 4)) << 4) + ((SMOOTHED(6, 12) < SMOOTHED(2, 5)) << 3) + ((SMOOTHED(-3, -8) < SMOOTHED(0, 5)) << 2) + ((SMOOTHED(-13, 1) < SMOOTHED(-7, 2)) << 1) + ((SMOOTHED(-1, -10) < SMOOTHED(7, -18)) << 0));
    desc[21] = (uchar)(((SMOOTHED(-1, 8) < SMOOTHED(-9, -10)) << 7) + ((SMOOTHED(-23, -1) < SMOOTHED(6, 2)) << 6) + ((SMOOTHED(-5, -3) < SMOOTHED(3, 2)) << 5) + ((SMOOTHED(0, 11) < SMOOTHED(-4, -7)) << 4) + ((SMOOTHED(15, 2) < SMOOTHED(-10, -3)) << 3) + ((SMOOTHED(-20, -8) < SMOOTHED(-13, 3)) << 2) + ((SMOOTHED(-19, -12) < SMOOTHED(5, -11)) << 1) + ((SMOOTHED(-17, -13) < SMOOTHED(-3, 2)) << 0));
    desc[22] = (uchar)(((SMOOTHED(7, 4) < SMOOTHED(-12, 0)) << 7) + ((SMOOTHED(5, -1) < SMOOTHED(-14, -6)) << 6) + ((SMOOTHED(-4, 11) < SMOOTHED(0, -4)) << 5) + ((SMOOTHED(3, 10) < SMOOTHED(7, -3)) << 4) + ((SMOOTHED(13, 21) < SMOOTHED(-11, 6)) << 3) + ((SMOOTHED(-12, 24) < SMOOTHED(-7, -4)) << 2) + ((SMOOTHED(4, 16) < SMOOTHED(3, -14)) << 1) + ((SMOOTHED(-3, 5) < SMOOTHED(-7, -12)) << 0));
    desc[23] = (uchar)(((SMOOTHED(0, -4) < SMOOTHED(7, -5)) << 7) + ((SMOOTHED(-17, -9) < SMOOTHED(13, -7)) << 6) + ((SMOOTHED(22, -6) < SMOOTHED(-11, 5)) << 5) + ((SMOOTHED(2, -8) < SMOOTHED(23, -11)) << 4) + ((SMOOTHED(7, -10) < SMOOTHED(-1, 14)) << 3) + ((SMOOTHED(-3, -10) < SMOOTHED(8, 3)) << 2) + ((SMOOTHED(-13, 1) < SMOOTHED(-6, 0)) << 1) + ((SMOOTHED(-7, -21) < SMOOTHED(6, -14)) << 0));
    desc[24] = (uchar)(((SMOOTHED(18, 19) < SMOOTHED(-4, -6)) << 7) + ((SMOOTHED(10, 7) < SMOOTHED(-1, -4)) << 6) + ((SMOOTHED(-1, 21) < SMOOTHED(1, -5)) << 5) + ((SMOOTHED(-10, 6) < SMOOTHED(-11, -2)) << 4) + ((SMOOTHED(18, -3) < SMOOTHED(-1, 7)) << 3) + ((SMOOTHED(-3, -9) < SMOOTHED(-5, 10)) << 2) + ((SMOOTHED(-13, 14) < SMOOTHED(17, -3)) << 1) + ((SMOOTHED(11, -19) < SMOOTHED(-1, -18)) << 0));
    desc[25] = (uchar)(((SMOOTHED(8, -2) < SMOOTHED(-18, -23)) << 7) + ((SMOOTHED(0, -5) < SMOOTHED(-2, -9)) << 6) + ((SMOOTHED(-4, -11) < SMOOTHED(2, -8)) << 5) + ((SMOOTHED(14, 6) < SMOOTHED(-3, -6)) << 4) + ((SMOOTHED(-3, 0) < SMOOTHED(-15, 0)) << 3) + ((SMOOTHED(-9, 4) < SMOOTHED(-15, -9)) << 2) + ((SMOOTHED(-1, 11) < SMOOTHED(3, 11)) << 1) + ((SMOOTHED(-10, -16) < SMOOTHED(-7, 7)) << 0));
    desc[26] = (uchar)(((SMOOTHED(-2, -10) < SMOOTHED(-10, -2)) << 7) + ((SMOOTHED(-5, -3) < SMOOTHED(5, -23)) << 6) + ((SMOOTHED(13, -8) < SMOOTHED(-15, -11)) << 5) + ((SMOOTHED(-15, 11) < SMOOTHED(6, -6)) << 4) + ((SMOOTHED(-16, -3) < SMOOTHED(-2, 2)) << 3) + ((SMOOTHED(6, 12) < SMOOTHED(-16, 24)) << 2) + ((SMOOTHED(-10, 0) < SMOOTHED(8, 11)) << 1) + ((SMOOTHED(-7, 7) < SMOOTHED(-19, -7)) << 0));
    desc[27] = (uchar)(((SMOOTHED(5, 16) < SMOOTHED(9, -3)) << 7) + ((SMOOTHED(9, 7) < SMOOTHED(-7, -16)) << 6) + ((SMOOTHED(3, 2) < SMOOTHED(-10, 9)) << 5) + ((SMOOTHED(21, 1) < SMOOTHED(8, 7)) << 4) + ((SMOOTHED(7, 0) < SMOOTHED(1, 17)) << 3) + ((SMOOTHED(-8, 12) < SMOOTHED(9, 6)) << 2) + ((SMOOTHED(11, -7) < SMOOTHED(-8, -6)) << 1) + ((SMOOTHED(19, 0) < SMOOTHED(9, 3)) << 0));
    desc[28] = (uchar)(((SMOOTHED(1, -7) < SMOOTHED(-5, -11)) << 7) + ((SMOOTHED(0, 8) < SMOOTHED(-2, 14)) << 6) + ((SMOOTHED(12, -2) < SMOOTHED(-15, -6)) << 5) + ((SMOOTHED(4, 12) < SMOOTHED(0, -21)) << 4) + ((SMOOTHED(17, -4) < SMOOTHED(-6, -7)) << 3) + ((SMOOTHED(-10, -9) < SMOOTHED(-14, -7)) << 2) + ((SMOOTHED(-15, -10) < SMOOTHED(-15, -14)) << 1) + ((SMOOTHED(-7, -5) < SMOOTHED(5, -12)) << 0));
    desc[29] = (uchar)(((SMOOTHED(-4, 0) < SMOOTHED(15, -4)) << 7) + ((SMOOTHED(5, 2) < SMOOTHED(-6, -23)) << 6) + ((SMOOTHED(-4, -21) < SMOOTHED(-6, 4)) << 5) + ((SMOOTHED(-10, 5) < SMOOTHED(-15, 6)) << 4) + ((SMOOTHED(4, -3) < SMOOTHED(-1, 5)) << 3) + ((SMOOTHED(-4, 19) < SMOOTHED(-23, -4)) << 2) + ((SMOOTHED(-4, 17) < SMOOTHED(13, -11)) << 1) + ((SMOOTHED(1, 12) < SMOOTHED(4, -14)) << 0));
    desc[30] = (uchar)(((SMOOTHED(-11, -6) < SMOOTHED(-20, 10)) << 7) + ((SMOOTHED(4, 5) < SMOOTHED(3, 20)) << 6) + ((SMOOTHED(-8, -20) < SMOOTHED(3, 1)) << 5) + ((SMOOTHED(-19, 9) < SMOOTHED(9, -3)) << 4) + ((SMOOTHED(18, 15) < SMOOTHED(11, -4)) << 3) + ((SMOOTHED(12, 16) < SMOOTHED(8, 7)) << 2) + ((SMOOTHED(-14, -8) < SMOOTHED(-3, 9)) << 1) + ((SMOOTHED(-6, 0) < SMOOTHED(2, -4)) << 0));
    desc[31] = (uchar)(((SMOOTHED(1, -10) < SMOOTHED(-1, 2)) << 7) + ((SMOOTHED(8, -7) < SMOOTHED(-6, 18)) << 6) + ((SMOOTHED(9, 12) < SMOOTHED(-7, -23)) << 5) + ((SMOOTHED(8, -6) < SMOOTHED(5, 2)) << 4) + ((SMOOTHED(-9, 6) < SMOOTHED(-12, -7)) << 3) + ((SMOOTHED(-1, -2) < SMOOTHED(-7, 2)) << 2) + ((SMOOTHED(9, 9) < SMOOTHED(7, 15)) << 1) + ((SMOOTHED(6, 2) < SMOOTHED(-6, 6)) << 0));
    #undef SMOOTHED
}

extern "C" int brief(buffer_t *input, buffer_t *img, int col, int row, float threshold, buffer_t *out) 
{   //struct timeval t3, t4;
    //gettimeofday(&t3, NULL);
 
    if (input->host == nullptr) {
 
   	input->min[0] = 0;
        input->min[1] = 0;
        input->extent[0] = col;
        input->extent[1] = row;


    }else {
 
        assert(out->host);
        float *corners = (float *)input->host;
        corners -= input->min[0] * input->stride[0];
        corners -= input->min[1] * input->stride[1];

        //struct timeval t1, t2;
        //gettimeofday(&t1, NULL);
        std::vector<KeyPoint> keypoints;
        keypoints.reserve(128);
        for (int y = 28; y < row-28; y++) {
            for (int x = 28; x < col-28; x++) {
                float value = corners[x * input->stride[0] + y * input->stride[1]];
                if (value >= threshold) {
                    KeyPoint kpt((float)x, (float)y, 4, -1, value);
                    keypoints.push_back(kpt);
                    x++;
                }
            }
        }
        //gettimeofday(&t2, NULL);

        //double time = (t2.tv_sec - t1.tv_sec) * 1000.0 +
        //   (t2.tv_usec - t1.tv_usec) / (1000.0);
        //printf("loop time: %f\n", time);

        //printf("keypoints number: %d\n", (int)keypoints.size());
        //printf("threshold: %u\n", (uint8_t)threshold);

        //gettimeofday(&t1, NULL);
        int num_kpt = out->extent[1];
        std::nth_element(keypoints.begin(), keypoints.begin() + num_kpt, keypoints.end(), KeypointResponseGreater());
        keypoints.resize(num_kpt);
        
        //gettimeofday(&t2, NULL);
        //time = (t2.tv_sec - t1.tv_sec) * 1000.0 +
        //   (t2.tv_usec - t1.tv_usec) / (1000.0);
        //printf("sort time: %f\n", time);

       
        //gettimeofday(&t1, NULL);
        assert(img->host);
        uint8_t *gray_img = (uint8_t *)img->host;
        gray_img -= img->min[0] * img->stride[0];
        gray_img -= img->min[1] * img->stride[1];

        Mat cv_input = Mat(img->extent[1], img->extent[0], CV_8UC1, gray_img);
 
        uint8_t *dst = (uint8_t *)(out->host);
        memset(dst, 0, num_kpt*16*sizeof(uint8_t));
        for (int i = 0; i < num_kpt ; ++i) {
            uchar* desc = dst + 16 * i;
            const KeyPoint& pt = keypoints[i];
            compute_desc(cv_input, pt, desc);  
        }
        //gettimeofday(&t2, NULL);
        //time = (t2.tv_sec - t1.tv_sec) * 1000.0 +
        //   (t2.tv_usec - t1.tv_usec) / (1000.0);
        //printf("desc time: %f\n", time);


    }        
    //gettimeofday(&t4, NULL);
    //double time = (t4.tv_sec - t3.tv_sec) * 1000.0 +
    //   (t4.tv_usec - t3.tv_usec) / (1000.0);
    //printf("total time: %f\n", time);


    return 0;
} 


int main(int argc, char **argv) {

    //float k = 0.04;
    //float threshold = 100;
    int nfeatures = 20;
    int iter = 1;

    Image<uint8_t> input = load_image(argv[1]);
    Image<int32_t> intg(input.width(), input.height());
    Image<uint8_t> out_native(16, nfeatures);
    Image<uint8_t> out_cuda(16, nfeatures);
    Image<float> out_corners(input.width(), input.height());

    Mat cv_img = imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE);
    Mat sum;
    integral(cv_img, sum, CV_32S);
    
    for (int y = 0; y < input.height(); y++) {
        for (int x = 0; x < input.width(); x++) {
            intg(x, y) = sum.at<int32_t>(y, x);
        }
    }

    printf("\nstart timing code ...\n");

    double min_t = benchmark(iter, 10, [&]() {
       pipeline_native(input, intg, out_native);
    });
    printf("CPU program runtime: %g\n", min_t * 1e3);

    /*double min_t1 = benchmark(iter, 10, [&]() {
       pipeline_corners(input, intg, out_corners);
    });
    printf("CPU Harris program runtime: %g\n", min_t1 * 1e3);
    */

    double min_t3 = benchmark(iter, 10, [&]() {
       pipeline_cuda(input, intg, out_cuda);
    });
    printf("CUDA program runtime: %g\n", min_t3 * 1e3);


    for (int y = 0; y < out_native.height(); y++) {
        for (int x = 0; x < out_native.width(); x++) {
            if (fabs(out_native(x, y, 0) - out_cuda(x, y, 0)) > 1e-4) {
                printf("out_native(%d, %d, 0) = %d, but out_cuda(%d, %d, 0) = %d\n",
                       x, y, out_native(x, y, 0),
                       x, y, out_cuda(x, y, 0));
                return 1;
            }
        }
    }

    return 0;
}
