//
// Copyright 2003-2015 Mentor Graphics Corporation
//
// All Rights Reserved.
//
// THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE PROPERTY OF 
// MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT TO LICENSE TERMS.
// 


#include "Stencil_catapult.h"
//#include "simple_ref.h"

#include <mc_scverify.h>
#include "gemm.h"

#define DEBUG

CCS_MAIN(int argc, char *argv[]) 
{
  
  DTYPE input[ACOL][BLOCKSIZE]; // R_TILE=ACOL, Y_TILE=BLOCKSIZE
  DTYPE weight[ACOL][BLOCKSIZE]; // R_TILE=ACOL, X_TILE=BLOCKSIZE

  //DTYPE output[BLOCKSIZE][BLOCKSIZE];
  //DTYPE output_ref[SIZE][LEN];
static ac_channel<PackedStencil<DTYPE, ACOL> > input_stream;
static ac_channel<PackedStencil<DTYPE, BLOCKSIZE> > weight_stream;
static ac_channel<PackedStencil<DTYPE, BLOCKSIZE> > output_stream;

//  static ac_channel<DTYPE> input_stream;
//  static ac_channel<DTYPE> weight_stream;
//  static ac_channel<DTYPE> output_stream;

  int errCnt = 0;
  printf("Input\n");

  for ( int j = 0; j < BLOCKSIZE; j++ ){
    PackedStencil<DTYPE, ACOL> input_col;
    for ( int i = 0; i < ACOL; i++ ){
      printf("inputting %d on index %d\n",j+1, i);
      input[i][j] = j+1;
      input_col(j+1, i,0,0,0);

      //input_stream.write(input[i][j]);
      }  
      input_stream.write(input_col);
    }

    printf("Weight\n");
    PackedStencil<DTYPE, BLOCKSIZE> weight_row;
    for ( int i = 0; i < ACOL; i++ ){
      for ( int j = 0; j < BLOCKSIZE; j++ ){
        weight[i][j] = i;  
        //weight_stream.write(weight[i][j]);
        printf("weight=%d on index %d,%d\n",i, i,j);
        weight_row(weight[i][j], j,0,0,0);
      }
      weight_stream.write(weight_row);
    }
    //weight_stream.write(weight_tile);
    printf("finished weights\n");

    // Main function call
    //CCS_DESIGN(hls_target)(input, weight, output);        
    CCS_DESIGN(gemm)(input_stream,weight_stream,output_stream);
    //simple_ref(input, weight, output_ref);          

    printf("\nOutput\n\n"); 

    //PackedStencil<DTYPE, BLOCKSIZE,1,1> output_col = output_stream.read();
    for (int j = 0; j < BLOCKSIZE; j++) {
      /*
      for (int i = 0; i < ACOL; i++ ){
        //printf("output[%d][%d] = %d\n", j, i, (int)output_col(j,0,0));
        DTYPE output_value = output_stream.read();
        printf("output[%d][%d] = %d\n", i, j, output_value);
      }
      */


    }
    //printf("output = %d\n", (int)output);
    printf("\nThere were %d errors\n",errCnt);
    CCS_RETURN(0);
}

