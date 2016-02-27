#include "hls_video.h"

#define HEIGHT 1080
#define WIDTH 1920

typedef hls::stream<ap_axiu<32,1,1,1> > AXI_STREAM;
typedef hls::Scalar<3, unsigned char> RGB_PIXEL;
typedef hls::Mat<HEIGHT, WIDTH, HLS_8UC3> RGB_IMAGE;

void image_filter(AXI_STREAM& INPUT_STREAM,
                  AXI_STREAM& OUTPUT_STREAM,
                  hls::Window<5, 5, ap_uint<8> > kernel) {
#pragma HLS Dataflow
#pragma HLS INTERFACE s_axilite port=return bundle=config
#pragma HLS INTERFACE s_axilite port=kernel bundle=config
#pragma HLS INTERFACE axis register port=INPUT_STREAM
#pragma HLS INTERFACE axis register port=OUTPUT_STREAM

    //Create AXI streaming interfaces for the core
    hls::Mat<HEIGHT, WIDTH, HLS_8UC3> img_0(HEIGHT, WIDTH);
    hls::Mat<HEIGHT, WIDTH, HLS_16UC3> img_1(HEIGHT, WIDTH);
    hls::Mat<HEIGHT, WIDTH, HLS_8UC3> img_2(HEIGHT, WIDTH);
    // Convert AXI4 Stream data to hls::mat format
    hls::AXIvideo2Mat(INPUT_STREAM, img_0);
    //hls::GaussianBlur<5, 5, hls::BORDER_REPLICATE>(img_0, img_1, rows, cols);
    hls::Point_<int> anchor;
    anchor.x=-1;
    anchor.y=-1;
    hls::Filter2D<hls::BORDER_REPLICATE>(img_0, img_1, kernel, anchor);

    // normalization
    for (size_t i = 0; i < HEIGHT; i++) {
        for (size_t j = 0; j < WIDTH; j++) {
#pragma HLS DEPENDENCE array inter false
#pragma HLS PIPELINE
            for (size_t k = 0; k < 3; k++) {
                HLS_TNAME(HLS_16UC3) src_temp;
                img_1.data_stream[k] >> src_temp;
                HLS_TNAME(HLS_8UC3) temp = src_temp >> 8;
                img_2.data_stream[k]<<temp;
            }
        }
    }


    // Convert the hls::mat format to AXI4 Stream format
    hls::Mat2AXIvideo(img_2, OUTPUT_STREAM);
}
