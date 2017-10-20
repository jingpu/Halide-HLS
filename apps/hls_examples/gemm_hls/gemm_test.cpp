#include <iostream>
#include "hls_target.h"

#define HW_COSIM

#define MAT_A_ROWS 384//384//16
#define MAT_A_COLS 64 //256
#define MAT_B_ROWS 384 //384 //16
#define MAT_B_COLS 64 //256


typedef uint8_t mat_a_t;
typedef uint8_t mat_b_t;

using namespace std;

int main(int argc, char **argv)
{

   static mat_a_t in_mat_a[MAT_A_ROWS * MAT_A_COLS];
   static mat_b_t in_mat_b[MAT_B_ROWS * MAT_B_COLS];

   for(int i = 0; i < MAT_A_ROWS; i++) {
      for(int j = 0; j < MAT_A_COLS; j++) {
          in_mat_a[i* MAT_A_COLS + j] = i+j;
      }
   }
   //Transpose
   for(int i = 0; i < MAT_B_ROWS; i++) {
      for(int j = 0; j < MAT_B_COLS; j++) {
          in_mat_b[i * MAT_B_COLS + j] = i+j;
      }
   }

   static uint16_t hw_result[MAT_A_ROWS * MAT_B_ROWS], sw_result[MAT_A_ROWS * MAT_B_ROWS];
   for(int i = 0; i < MAT_A_ROWS; i++) {
      for(int j = 0; j < MAT_B_ROWS; j++) {
          hw_result[j * MAT_A_ROWS + i] = 0;
      }
   }
   int err_cnt = 0;

   static uint16_t sw_result_tmp[MAT_A_ROWS * MAT_B_ROWS];
   // Generate the expected result
   // Iterate over the rows of the A matrix
   // It is actually the transpose of normal result
   for(int i = 0; i < MAT_A_ROWS; i++) {
      for(int j = 0; j < MAT_B_ROWS; j++) {
         // Iterate over the columns of the B matrix
         sw_result[i * MAT_B_ROWS + j] = 0;
         // Do the inner product of a row of A and col of B
         for(int k = 0; k < MAT_B_COLS; k++) {
            sw_result_tmp[i* MAT_B_ROWS + j] += (uint16_t)in_mat_a[i * MAT_A_COLS + k] * (uint16_t)in_mat_b[j * MAT_B_COLS + k];
            // if(i ==0 && j ==0)
            //	 cout << "sw: "<< sw_result[0] << endl;
         }
      }

   }


#ifdef HW_COSIM
   hls_target(hw_result, in_mat_a, in_mat_b, 64, 384); 
#endif

   // Print result matrix
   //cout << setw(6);
   for (int i = 0; i < MAT_B_ROWS; i++) {
      for (int j = 0; j < MAT_A_ROWS; j++) {
#ifdef HW_COSIM
         sw_result[i* MAT_B_ROWS + j] = sw_result_tmp[i* MAT_B_ROWS + j];
         // Check HW result against SW
         if (hw_result[i* MAT_B_ROWS + j] != sw_result[i* MAT_B_ROWS + j]) {
        	cout << i << " " << j << " " << unsigned(hw_result[i* MAT_B_ROWS + j]) << " " <<  unsigned(sw_result[i* MAT_B_ROWS + j]) << endl;
            err_cnt++;
         } 
         
#else
         cout << sw_result[i][j];
#endif
      }
   }

#ifdef HW_COSIM
   if (err_cnt)
      cout << "ERROR: " << err_cnt << " mismatches detected!" << endl;
   else
      cout << "Test passes." << endl;
#endif
   return err_cnt;
}

