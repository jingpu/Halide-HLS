// double buffer implementation for Catapult HLS
#include "ac_channel.h"
#include "Stencil_catapult.h"

template<typename T, int N>
struct chanStruct{
  T data[N];
 };


template<int ID,typename DTYPE,int NUM_REGS> 
void fifo(DTYPE din, DTYPE &dout) {
  static DTYPE regs[NUM_REGS];
  /*
  if (NUM_REGS==1) {
    dout = din;
  } else {
    dout = regs[NUM_REGS-2];
  }
  */
#pragma hls_unroll yes
SHIFT:for(int i=NUM_REGS-1; i>=0; i--) {
    if (i==0) {
      regs[i] = din;
    } else {
      regs[i] = regs[i-1];
    }
 }

  dout = regs[NUM_REGS-1];
}

#pragma hls_design
#pragma hls_pipeline_init_interval 1
template <typename DTYPE, int X_TILE, int Y_TILE, int NUM_ITER>
void WRITE_BLOCK_OUTPUT(ac_channel<PackedStencil<DTYPE, X_TILE,1,1> > &din,
                        ac_channel<chanStruct<PackedStencil<DTYPE,X_TILE>,Y_TILE> > &dout) {

#pragma hls_pipeline_init_interval 1
for (int iter = 0; iter < NUM_ITER; iter++) {
  chanStruct<PackedStencil<DTYPE,X_TILE>,Y_TILE> tmp;    //temporary array inside struct
 WRITE:for (int y_idx = 0; y_idx < 0 + Y_TILE; y_idx++)
    {
      /*
     for (int x_idx = 0; x_idx < 0 + X_TILE; x_idx++)
     {
      int y_pos_tile    = y_idx * X_TILE;
      int xy_pos_tile   = x_idx + y_pos_tile;
      DTYPE data_in         = output_row(x_idx,0,0,0);
      tmp.data[xy_pos_tile] = data_in;
     } // for x_idx
      */
      
      PackedStencil<DTYPE, X_TILE,1,1> output_row = din.read();
      tmp.data[y_idx] = output_row;

    } // for y_idx

  dout.write(tmp);//Memory channel write
}
}

#pragma hls_design
#pragma hls_pipeline_init_interval 1
template <typename DTYPE, int X_TILE, int Y_TILE, int NUM_ITER>
void READ_BLOCK_OUTPUT(ac_channel<chanStruct<PackedStencil<DTYPE,X_TILE>,Y_TILE> > &din,
                       ac_channel<PackedStencil<DTYPE,X_TILE> > &dout){

for (int iter = 0; iter < NUM_ITER; iter++) {
  chanStruct<PackedStencil<DTYPE,X_TILE>, Y_TILE> tmp;    //temporary array inside struct
  tmp = din.read();                       // Single Memory channel read

#pragma hls_pipeline_init_interval 1
 READ:
  //for (int y_idx = Y_TILE-1; y_idx >= 0; y_idx--)
  for (int y_idx=0; y_idx < Y_TILE; y_idx++)
    {
      /*
     for (int x_idx = X_TILE-1; x_idx >= 0; x_idx--)
     {
      int y_pos_tile  = y_idx * X_TILE;
      int xy_pos_tile = x_idx + y_pos_tile;
      DTYPE data_out      = tmp.data[xy_pos_tile];
      dout.write(data_out);
      } // for x_idx
      */
      PackedStencil<DTYPE, X_TILE> dout_struct;
      dout_struct = tmp.data[y_idx];
      dout.write(dout_struct);

    } // for y_idx
 }
}

#pragma hls_design
#pragma hls_pipeline_init_interval 1
template <typename DTYPE, int X_TILE, int Y_TILE, int NUM_ITER>
void double_buffer_output(ac_channel<PackedStencil<DTYPE, X_TILE,1,1> > &din, 
                          ac_channel<PackedStencil<DTYPE,X_TILE> > &dout) {

  static ac_channel<chanStruct<PackedStencil<DTYPE,X_TILE,1,1>,Y_TILE> > shr_mem;//Static memory channel

    WRITE_BLOCK_OUTPUT<DTYPE, X_TILE, Y_TILE, NUM_ITER>(din, shr_mem);
    READ_BLOCK_OUTPUT<DTYPE, X_TILE, Y_TILE, NUM_ITER>(shr_mem, dout);
}

#pragma hls_design
#pragma hls_pipeline_init_interval 1
template <typename DTYPE, int R_TILE, int Y_TILE, int NUM_ITER>
void WRITE_BLOCK_INPUT(ac_channel<PackedStencil<DTYPE,R_TILE> > &din,
                      ac_channel<chanStruct<PackedStencil<DTYPE,R_TILE,1,1>,Y_TILE> > &dout) {

#pragma hls_pipeline_init_interval 1
for (int iter = 0; iter < NUM_ITER; iter++) {
  chanStruct<PackedStencil<DTYPE,R_TILE,1,1>,Y_TILE> tmp;    //temporary array inside struct
 WRITE:for (int y_idx = 0; y_idx < 0 + Y_TILE; y_idx++)
    {
      /*
     for (int r_idx = 0; r_idx < 0 + R_TILE; r_idx++)
     {
       //int y_pos_tile    = y_idx * R_TILE;
       //int ry_pos_tile   = r_idx + y_pos_tile;
       DTYPE data_in         = din.read();
       column(data_in, r_idx,0,0,0);
     } // for r_idx
      */
      PackedStencil<DTYPE,R_TILE,1,1> column;
      column = din.read();
      tmp.data[y_idx] = column;
    } // for y_idx

  dout.write(tmp);//Memory channel write
 }
}


#pragma hls_design
#pragma hls_pipeline_init_interval 1
template <typename DTYPE, int X_TILE, int Y_TILE, int R_TILE, int NUM_ITER>
void READ_BLOCK_INPUT(ac_channel<chanStruct<PackedStencil<DTYPE,R_TILE,1,1>,Y_TILE> > &din,
                     ac_channel<PackedStencil<DTYPE, R_TILE,1,1> > &dout){

#pragma hls_pipeline_init_interval 1
for (int iter = 0; iter < NUM_ITER; iter++) {
  chanStruct<PackedStencil<DTYPE,R_TILE>,Y_TILE> tmp;    //temporary array inside struct
  tmp = din.read();                       // Single Memory channel read
 READ:
  //for (int y_idx = Y_TILE-1; y_idx >= 0; y_idx--)
  for (int y_idx=0; y_idx < Y_TILE; y_idx++)
    {
      //for (int x_idx = X_TILE-1; x_idx >= 0; x_idx--)
        {
          /*
#pragma hls_unroll yes       
          for (int r_idx = R_TILE-1; r_idx >=0; r_idx--)
            {

              //int y_pos_tile    = y_idx * R_TILE;
              int ry_pos_tile   = r_idx + y_pos_tile;
              DTYPE data_out    = tmp.data[ry_pos_tile];
              DTYPE data_out    = tmp.data[0];
              dout_struct(data_out, r_idx,0,0,0);
            }
          */

          PackedStencil<DTYPE, R_TILE,1,1> dout_struct;
          dout_struct = tmp.data[y_idx];
          dout.write(dout_struct);

        } // for x_idx
    } // for y_idx
 }
}

#pragma hls_design
#pragma hls_pipeline_init_interval 1
  template <typename DTYPE, int X_TILE, int Y_TILE, int R_TILE, int NUM_ITER>
void double_buffer_input(//DTYPE din[X_TILE * Y_TILE],DTYPE dout[X_TILE * Y_TILE], 
                         ac_channel<PackedStencil<DTYPE,R_TILE> > &din, 
                         ac_channel<PackedStencil<DTYPE, R_TILE,1,1> > &dout) {

    static ac_channel<chanStruct<PackedStencil<DTYPE,R_TILE,1,1>,Y_TILE> > shr_mem;//Static memory channel

  WRITE_BLOCK_INPUT<DTYPE, R_TILE, Y_TILE, NUM_ITER>(din, shr_mem);
  READ_BLOCK_INPUT<DTYPE, X_TILE, Y_TILE, R_TILE, NUM_ITER>(shr_mem, dout);
}

#pragma hls_design
#pragma hls_pipeline_init_interval 1
template <typename DTYPE, int R_TILE, int X_TILE, int NUM_ITER>
void WRITE_BLOCK_WEIGHTS(ac_channel<PackedStencil<DTYPE,X_TILE> > &din,
                         ac_channel<chanStruct<PackedStencil<DTYPE,X_TILE>, R_TILE> > &dout) {

#pragma hls_pipeline_init_interval 1
for (int iter = 0; iter < NUM_ITER; iter++) {
  chanStruct<PackedStencil<DTYPE,X_TILE>, R_TILE> tmp;    //temporary array inside struct
 WRITE:for (int r_idx = 0; r_idx < 0 + R_TILE; r_idx++)
    {
      /*
     for (int x_idx = 0; x_idx < 0 + X_TILE; x_idx++)
     {
       //int x_pos_tile    = x_idx * R_TILE;
       //int rx_pos_tile   = r_idx + x_pos_tile;
       DTYPE data_in     = din.read();
       row(data_in, x_idx,0,0,0);
     } // for x_idx
      */

      PackedStencil<DTYPE,X_TILE> row;
      row     = din.read();
      tmp.data[r_idx] = row;
    } // for r_idx

  dout.write(tmp);//Memory channel write
 }
}


#pragma hls_design
#pragma hls_pipeline_init_interval 1
template <typename DTYPE, int X_TILE, int Y_TILE, int R_TILE, int NUM_ITER>
void READ_BLOCK_WEIGHTS(ac_channel<chanStruct<PackedStencil<DTYPE,X_TILE,1,1>,R_TILE> > &din,
                        ac_channel<PackedStencil<DTYPE, X_TILE,1,1> > &dout){
#pragma hls_pipeline_init_interval 1
  for (int iter = 0; iter < NUM_ITER; iter++) {

    chanStruct<PackedStencil<DTYPE,X_TILE>,R_TILE> tmp;    //temporary array inside struct

    tmp = din.read();                       // Single Memory channel read
  READ:
    //for (int r_idx = R_TILE-1; r_idx >= 0; r_idx--)
    for (int r_idx = 0; r_idx < R_TILE; r_idx++)
      {
       
        /*
          #pragma hls_unroll yes
          for (int x_idx = X_TILE-1; x_idx >=0; x_idx--)
          {

          int x_pos_tile    = x_idx * R_TILE;
          int rx_pos_tile   = r_idx + x_pos_tile;
          DTYPE data_out    = tmp.data[rx_pos_tile];
          dout_struct(data_out, rx_pos_tile,0,0,0);
          } // for x_idx
        */
        PackedStencil<DTYPE, X_TILE,1,1> dout_struct;
        dout_struct = tmp.data[r_idx];
        dout.write(dout_struct);

      } // for r_idx

  } // for iter
}

#pragma hls_design
#pragma hls_pipeline_init_interval 1
  template <typename DTYPE, int X_TILE, int Y_TILE, int R_TILE, int NUM_ITER>
  void double_buffer_weights(ac_channel<PackedStencil<DTYPE,X_TILE> > &din, 
                             ac_channel<PackedStencil<DTYPE, X_TILE,1,1> > &dout) {

    static ac_channel<chanStruct<PackedStencil<DTYPE,X_TILE,1,1>,R_TILE> > shr_mem;//Static memory channel

  WRITE_BLOCK_WEIGHTS<DTYPE, R_TILE, X_TILE, NUM_ITER>(din, shr_mem);
  READ_BLOCK_WEIGHTS<DTYPE, X_TILE, Y_TILE, R_TILE, NUM_ITER>(shr_mem, dout);
}
