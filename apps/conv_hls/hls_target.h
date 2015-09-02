#ifndef HALIDE_CODEGEN_HLS_TARGET_HLS_TARGET_H
#define HALIDE_CODEGEN_HLS_TARGET_HLS_TARGET_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <hls_stream.h>
#include "stencil.h"

void _hls_target_f3_stream_stencil_stream(
hls::stream<Stencil<uint8_t, 1, 1, 1> > &_clamped_stream_stencil_stream,
hls::stream<Stencil<uint8_t, 1, 1, 1> > &_f3_stream_stencil_stream,
uint8_t _p2___bias,
Stencil<uint8_t, 5, 5> _weight_stencil);

#endif

