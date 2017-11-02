#include<iostream>
#include "hls_target.h"

#define HW_COSIM


#define ROWS 32 //68
#define COLS 124 //68
#define ICH 32
#define OCH 32
#define FS 3

typedef uint8_t t;
typedef uint16_t rt;
using namespace std;

int main()
{
	static t image[(ROWS + FS - 1)*(COLS + FS - 1)*ICH];
	static rt res[ROWS * COLS * OCH];
	static t weight_0[FS*FS*ICH*OCH];

	for (int c = 0; c < ICH; c++)
	for (int j = 0; j < ROWS+FS-1; j++)
	for (int i = 0; i < COLS+FS-1; i++)
		image[c*(ROWS+FS-1)*(COLS+FS-1) + j*(COLS+FS-1) + i] = (abs(j-i)+c);


	for (int idx0 = 0; idx0 < FS; idx0++)
	for (int idx1 = 0; idx1 < FS; idx1++)
	for (int idx2 = 0; idx2 < ICH; idx2++)
	for (int idx3 = 0; idx3 < OCH; idx3++) {
		weight_0[idx3*FS*FS*ICH + idx2*FS*FS + idx1*FS + idx0] = abs(idx0-idx1);
	}


	int err_cnt = 0;

#ifdef HW_COSIM

	hls_target(image, res, weight_0, ICH, COLS, ROWS, OCH);

    static rt res_sw_0[ROWS * COLS * OCH];
    for (int i = 0 ; i < ROWS * COLS * OCH; i++)
    	res_sw_0[i] = 0;

    for (int k = 0; k < OCH; k++) {
      for (int y = 0; y < ROWS; y++) {
    	for (int x = 0; x < COLS; x++) {
    	  for (int c = 0; c < ICH; c++) {
    		for (int fy = 0; fy < FS; fy++) {
    		  for (int fx = 0; fx < FS; fx++){
    			  res_sw_0[ y*COLS*OCH+x*OCH + k] += image[(y+fy) * (COLS+FS-1)*ICH + (x+fx)*ICH + c] * weight_0[k*FS*FS*ICH + fy*FS*ICH + fx*ICH + c ];
    		  }
    		}
    	  }
    	}
      }
    }

    /*uint8_t image_1[(ROWS-2) * (COLS-2) * 4];
	for (int c = 0; c < 4; c++)
	for (int j = 0; j < ROWS-2; j++)
	for (int i = 0; i < COLS-2; i++) {
		if (i < 2 || j < 2) {
	      image_1[c*(ROWS-2)*(COLS-2) + j*(COLS-2) + i] = 0;
		} else {
		  image_1[c*(ROWS-2)*(COLS-2) + j*(COLS-2) + i] = res_sw_0[c*(ROWS-4) * (COLS-4) + (j-2)*(COLS-2) + i-2];
		}
	}*/

    /*uint8_t res_sw[(ROWS-6) * (COLS-6)*8];
    for (int i = 0 ; i < (ROWS-6) * (COLS-6) * 8; i++)
    	res_sw[i] = 0;
    for (int k = 0; k < 8; k++) {
     //for (int co = 0; co < 1; co++) {
      for (int y = 0; y < ROWS-6; y++) {
    	for (int x = 0; x < COLS-6; x++) {
    	  for (int ci = 0; ci < 4; ci++) {
    		for (int fy = 0; fy < 3; fy++) {
    		  for (int fx = 0; fx < 3; fx++){
                //res_sw[y*64+x] += (uint16_t)image[y+fy][x+fx](0,0,ci) * (uint16_t)weight(fx, fy, ci, k);
    			 // res_sw[k*((ROWS-4) * (COLS-4)) + y*(COLS-4)+x] += (uint8_t)image[co*3*ROWS*COLS + ci*ROWS*COLS + (y+fy) * COLS + (x+fx)] * (uint16_t)weight(fx, fy, ci, k);
    			  //res_sw[k*((ROWS-4) * (COLS-4)) + y*(COLS-4)+x] += image[co*3*ROWS*COLS + ci*ROWS*COLS + (y+fy) * COLS + (x+fx)] * weight[k*300 + co*75 + ci*25 + fy*5 + fx];
    			  res_sw[k*((ROWS-6) * (COLS-6)) + y*(COLS-6)+x] += res_sw_0[ ci*(ROWS-4)*(COLS-4) + (y+fy) * (COLS-4) + (x+fx)] * weight_1[k*36 + + fy*12 + fx*4 + ci ];
    		  }
    		}
    	  }
    	}
      }
    //}
    }*/

    for (int i = 0; i < ROWS * COLS * OCH; i++) {
    	if(res[i] != res_sw_0[i]) {
    		cout << "pos: " << i << " res: " << unsigned(res[i]) << " " << unsigned(res_sw_0[i]) << endl;
    		err_cnt++;
    	}
    }


   if (err_cnt)
      cout << "ERROR: " << err_cnt << " mismatches detected!" << endl;
   else
      cout << "Test passes." << endl;
#endif
   return err_cnt;

}
