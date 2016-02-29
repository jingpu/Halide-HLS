#include "hls_video.h"

#define HEIGHT 1080
#define WIDTH 1920

typedef hls::stream<ap_axiu<32,1,1,1> > AXI_STREAM;
typedef hls::Mat<HEIGHT, WIDTH, HLS_8UC1> MY_IMAGE;
//typedef hls::Mat<HEIGHT, WIDTH, HLS_32FC1> MY_IMAGE;


void image_filter(AXI_STREAM& INPUT_STREAM,
                  AXI_STREAM& OUTPUT_STREAM,
                  float k, int threshold) {
#pragma HLS Dataflow
#pragma HLS INTERFACE s_axilite port=return bundle=config
#pragma HLS INTERFACE s_axilite port=k bundle=config
#pragma HLS INTERFACE s_axilite port=threshold bundle=config
#pragma HLS INTERFACE axis register port=INPUT_STREAM
#pragma HLS INTERFACE axis register port=OUTPUT_STREAM

    //Create AXI streaming interfaces for the core
    MY_IMAGE img_0(HEIGHT, WIDTH);
    MY_IMAGE img_1(HEIGHT, WIDTH);
    // Convert AXI4 Stream data to hls::mat format
    hls::AXIvideo2Mat(INPUT_STREAM, img_0);
    hls::Harris<3, 3>(img_0, img_1, k, threshold);

    // Convert the hls::mat format to AXI4 Stream format
    hls::Mat2AXIvideo(img_1, OUTPUT_STREAM);
}
