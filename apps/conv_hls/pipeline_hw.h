#ifndef PIPELINE_HW_H
#define PIPELINE_HW_H

#include <hls_stream.h>
#include<stdint.h>

void convolve55_stream(hls::stream<uint8_t>& input, hls::stream<uint8_t>& output);

#endif
