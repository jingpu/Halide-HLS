// Copyright 2003-2015 Mentor Graphics Corporation
//
// All Rights Reserved.
//
// THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE PROPERTY OF 
// MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT TO LICENSE TERMS.
// 

#include "double_buffer.cpp"
#include "gemm.h"

template<typename T>
class pe_class{
  private:
    T x_reg;
    T y_reg;
  public:
    void exec(T &x_in, T &y_in, T &w, T &x_out, T &y_out) {
        y_out = x_reg * w + y_reg;
        x_out = x_reg;
        x_reg = x_in;
        y_reg = y_in; 
    }
};

#pragma hls_design
#pragma hls_pipeline_init_interval 1
template<typename DTYPE, int X_TILE, int Y_TILE, int R_TILE, int NUM_ITER>
void systolic_array(ac_channel<PackedStencil<DTYPE, R_TILE,1,1> > &input, 
                    ac_channel<PackedStencil<DTYPE, X_TILE,1,1> > &weight, 
                    ac_channel<PackedStencil<DTYPE, X_TILE,1,1> > &output) {

  static pe_class<DTYPE> pe[R_TILE][X_TILE];
  DTYPE in_tmp[R_TILE][X_TILE];
  DTYPE out_tmp[R_TILE][X_TILE];

  DTYPE tmp = 0;
//  PackedStencil<DTYPE, X_TILE*R_TILE,1,1> w_tile;
PackedStencil<DTYPE,X_TILE> w_tile[R_TILE];
  
 ITER: for (int iter=0; iter<NUM_ITER; ++iter) {

  // STEPS: for (int step=0; step<12; ++step) {//X_TILE+R_TILE+Y_TILE-1; ++step) {
 STEPS: for (int step=0; step<X_TILE+R_TILE+Y_TILE-1; ++step) {

      if (step < R_TILE && iter==0) {
        PackedStencil<DTYPE,X_TILE> w_row = weight.read();
        w_tile[step] = w_row;
#ifndef __SYNTHESIS__
        for (int col = 0; col<X_TILE; col++) {
        printf("weight=%d on row  %d, col %d\n", w_tile[step](col,0,0,0), step, col);
        }
#endif

      }

      // read input, add input to fifos, and read fifos into input buffer
      PackedStencil<DTYPE, R_TILE,1,1> in_col;
      if (step < Y_TILE) {
        in_col = input.read();
#ifndef __SYNTHESIS__
        for (int row = 0; row<R_TILE; row++) {
        printf("input=%d on row  %d, col %d\n", in_col(row,0,0,0), step, row);
        }
#endif

      }

      PackedStencil<DTYPE, R_TILE,1,1> input_buf;

      DTYPE input_fifo_0;
      fifo<60000,DTYPE,R_TILE-3>(in_col(0,0,0), input_fifo_0);
      input_buf(input_fifo_0, 0,0,0,0);
      DTYPE input_fifo_1;
      fifo<60001,DTYPE,R_TILE-2>(in_col(1,0,0), input_fifo_1);
      input_buf(input_fifo_1, 1,0,0,0);
      DTYPE input_fifo_2;
      fifo<60002,DTYPE,R_TILE-1>(in_col(2,0,0), input_fifo_2);
      input_buf(input_fifo_2, 2,0,0,0);
      DTYPE input_fifo_3;
      fifo<60003,DTYPE,R_TILE-0>(in_col(3,0,0), input_fifo_3);
      input_buf(input_fifo_3, 3,0,0,0);

#ifndef __SYNTHESIS__
      printf("starting step %d - input %d %d %d %d\n", step, input_fifo_0,input_fifo_1,input_fifo_2,input_fifo_3);
#endif


    #pragma hls_unroll yes
    COL: for (int j=0; j < X_TILE; ++j) {
    #pragma hls_unroll yes
      ROW: for (int i=0; i < R_TILE; ++i) {
          DTYPE input_value = input_buf(i,0,0);
          DTYPE weight_value = w_tile[i](j,0,0);

#ifndef __SYNTHESIS__
          //printf("weight=%d on index  %d,%d\n", weight_value, i, j);
#endif

          if(i == 0 && j == 0) {
            pe[i][j].exec(input_value, tmp, weight_value, in_tmp[i][j], out_tmp[i][j]);
          } else if (i == 0 && j != 0) {
            pe[i][j].exec(in_tmp[i][j-1], tmp, weight_value, in_tmp[i][j], out_tmp[i][j]);
          } else if (i != 0 && j == 0) {
            pe[i][j].exec(input_value, out_tmp[i-1][j], weight_value, in_tmp[i][j], out_tmp[i][j]);
          } else { // i>0 && j>0
            pe[i][j].exec(in_tmp[i][j-1], out_tmp[i-1][j], weight_value, in_tmp[i][j], out_tmp[i][j]);
          }
        }

        //output_fifo_input(out_tmp[R_TILE-1][j], 0,0,0,j);

      } //COL

        //write to fifos
        //fifo<ID,DTYPE,FIFO_SIZE>(input, output)

      PackedStencil<DTYPE, X_TILE> output_row;

      DTYPE sys_array_out_0 = out_tmp[R_TILE-1][0];       DTYPE output_fifo_0;
      fifo<0,DTYPE,X_TILE-0>(sys_array_out_0, output_fifo_0);
      output_row(output_fifo_0, 0,0,0,0);
      DTYPE sys_array_out_1 = out_tmp[R_TILE-1][1];       DTYPE output_fifo_1;
      fifo<1,DTYPE,X_TILE-1>(sys_array_out_1, output_fifo_1);
      output_row(output_fifo_1, 1,0,0,0);
      DTYPE sys_array_out_2 = out_tmp[R_TILE-1][2];       DTYPE output_fifo_2;
      fifo<2,DTYPE,X_TILE-2>(sys_array_out_2, output_fifo_2);
      output_row(output_fifo_2, 2,0,0,0);
      DTYPE sys_array_out_3 = out_tmp[R_TILE-1][3];       DTYPE output_fifo_3;
      fifo<3,DTYPE,X_TILE-3>(sys_array_out_3, output_fifo_3);
      output_row(output_fifo_3, 3,0,0,0);
      
#ifndef __SYNTHESIS__
      printf("ending step %d - output %d %d %d %d\n", step, output_fifo_0,output_fifo_1,output_fifo_2,output_fifo_3);
#endif

    // output row if one has completed
    if (step >= X_TILE+R_TILE-1) {
      output.write(output_row);
#ifndef __SYNTHESIS__
      printf("outputting this row\n");
#endif
      
    }


    } //STEPS
  } //ITER
}



#pragma hls_design top
#pragma hls_pipeline_init_interval 1
void gemm(ac_channel<PackedStencil<DTYPE,ACOL> > &input, 
          ac_channel<PackedStencil<DTYPE, BLOCKSIZE> > &weight, 
          ac_channel<PackedStencil<DTYPE, BLOCKSIZE> > &output) {

  //  for (int yo=0; yo<BCOL/BLOCKSIZE; yo++) {
  //    for (int xo=0; xo<AROW/BLOCKSIZE; xo++) {
  static ac_channel<PackedStencil<DTYPE, ACOL,1,1> > input_copy;

  double_buffer_input<DTYPE,BLOCKSIZE,BLOCKSIZE,ACOL,1>(input, input_copy);

  static ac_channel<PackedStencil<DTYPE, BLOCKSIZE,1,1> > weight_copy;

  double_buffer_weights<DTYPE,BLOCKSIZE,BLOCKSIZE,ACOL,1>(weight, weight_copy);

  static ac_channel<PackedStencil<DTYPE, BLOCKSIZE,1,1> > output_copy;

  //systolic_array(DTYPE in[R_TILE], DTYPE weight[R_TILE], DTYPE out[Y_TILE])
  systolic_array<DTYPE,BLOCKSIZE,BLOCKSIZE,ACOL,1>(input_copy, weight_copy, output_copy);

  double_buffer_output<DTYPE,BLOCKSIZE,BLOCKSIZE,1>(output_copy,output);

      //    }    
      //  }
}

