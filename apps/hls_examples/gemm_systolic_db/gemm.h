//
// Copyright 2003-2015 Mentor Graphics Corporation
//
// All Rights Reserved.
//
// THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE PROPERTY OF 
// MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT TO LICENSE TERMS.
// 

#ifndef _GLOBAL_SIMPLE_H
#define _GLOBAL_SIMPLE_H
#define SC_INCLUDE_FX
#include "ac_int.h"
#include "ac_fixed.h"
#include <ac_channel.h>
#include "Stencil_catapult.h"

#define AROW       128
#define ACOL       4
#define BROW       4
#define BCOL       128

#define BLOCKSIZE  4

//#define X_TILE 4
//#define Y_TILE 4
//#define R_TILE 4
//#define NUM_ITER 1

typedef ac_int<16> DTYPE;

void gemm(ac_channel<PackedStencil<DTYPE,ACOL> > &input,
          ac_channel<PackedStencil<DTYPE,BLOCKSIZE> > &weight, 
          ac_channel<PackedStencil<DTYPE,BLOCKSIZE> > &output);

void systolic_array(ac_channel<PackedStencil<DTYPE, ACOL,1,1> > &input, 
                    ac_channel<PackedStencil<DTYPE, ACOL*BLOCKSIZE,1,1> > &weight, 
                    ac_channel<PackedStencil<DTYPE, BLOCKSIZE,1,1> > &output);


#endif

