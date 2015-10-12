#ifndef HALIDE_CODEGEN_HLS_TARGET_HLS_TARGET_H
#define HALIDE_CODEGEN_HLS_TARGET_HLS_TARGET_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <hls_stream.h>
#include "Stencil.h"

void _hls_target_hw_output__2_stencil_stream(
hls::stream<Stencil<uint8_t, 8, 8> > &_hw_output__2_stencil_stream,
hls::stream<Stencil<uint16_t, 8, 8> > &_input2__2_stencil_update_stream,
hls::stream<Stencil<uint8_t, 8, 8> > &_repeat_edge__2_stencil_update_stream);

#endif

