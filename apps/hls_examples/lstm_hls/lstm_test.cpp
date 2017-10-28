#include <iostream>
#include <math.h>
#include "hls_target.h"

#define HW_COSIM
// Hyper Params
#define NUM_INPUT 16
#define NUM_HIDDEN 16
#define NUM_OUTPUT 16
#define BATCH_SIZE 16
#define T 1

typedef float Weight_t;
typedef float Mat_t;

using namespace std;

int main(int argc, char **argv)
{

    static Weight_t Wxh[NUM_INPUT * 4 * NUM_HIDDEN];
    static Weight_t Whh[NUM_HIDDEN * 4 * NUM_HIDDEN];
    static Weight_t Why[NUM_HIDDEN * NUM_OUTPUT];
    static Weight_t b[4 * NUM_HIDDEN];
    static Mat_t Input[NUM_INPUT * BATCH_SIZE * T];

    for(int i = 0; i < 4 * NUM_HIDDEN; i++) {
        for(int j = 0; j < NUM_INPUT; j++) {
            Wxh[i * NUM_INPUT + j] = i + j;
        }
    }
   
    for(int i = 0; i < 4 * NUM_HIDDEN; i++) {
        for(int j = 0; j < NUM_HIDDEN; j++) {
            Whh[i * NUM_HIDDEN + j] = i + j;
        }
    }

    for(int i = 0; i < NUM_OUTPUT; i++) {
        for(int j = 0; j < NUM_HIDDEN; j++) {
            Why[i * NUM_HIDDEN + j] = i + j;
        }
    }

    for(int i = 0; i < 4 * NUM_HIDDEN; i++) {
        b[i] = i;
    }

    for(int t = 0; t < T; t++){
        for(int i = 0; i < BATCH_SIZE; i++){
            for(int j = 0; j < NUM_INPUT; j++){
                Input[t * BATCH_SIZE * NUM_INPUT + i * NUM_INPUT + j] = t + i + j;
            }
        }
    }

    static Mat_t hw_result[NUM_OUTPUT * BATCH_SIZE * T];
    for(int t = 0; t < T; t++) {
        for(int i = 0; i < BATCH_SIZE; i++) {
            for(int j = 0; j < NUM_OUTPUT; j++){
                hw_result[t * BATCH_SIZE * NUM_OUTPUT + i * NUM_OUTPUT + j] = 0;
            }
        }
    }
    static Mat_t sw_result[NUM_OUTPUT * BATCH_SIZE * T];
    for(int t = 0; t < T; t++) {
        for(int i = 0; i < BATCH_SIZE; i++) {
            for(int j = 0; j < NUM_OUTPUT; j++){
                sw_result[t * BATCH_SIZE * NUM_OUTPUT + i * NUM_OUTPUT + j] = 0;
            }
        }
    }
    int err_cnt = 0;

    // Generate the expected result
    static Mat_t h_tm1[NUM_HIDDEN * BATCH_SIZE];
    static Mat_t c_tm1[NUM_HIDDEN * BATCH_SIZE];
    static Mat_t h_t[NUM_HIDDEN * BATCH_SIZE];
    static Mat_t c_t[NUM_HIDDEN * BATCH_SIZE];
    for(int i = 0; i < BATCH_SIZE; i++){
        for(int j = 0; j < NUM_HIDDEN; j++){
            h_tm1[i * NUM_HIDDEN + j] = 0;
        }
    }
    for(int i = 0; i < BATCH_SIZE; i++){
        for(int j = 0; j < NUM_HIDDEN; j++){
            c_tm1[i * NUM_HIDDEN + j] = 0;
        }
    }
    for(int t = 0; t < T; t++) {

        // Matrix multiplication before gates
        Mat_t pregate_t[4 * NUM_HIDDEN * BATCH_SIZE];
        for(int i = 0; i < BATCH_SIZE; i++){
            for(int j = 0; j < 4 * NUM_HIDDEN; j++){
                pregate_t[i * 4 * NUM_HIDDEN + j] = 0;
                for(int k = 0; k < NUM_INPUT; k++){
                    pregate_t[i * 4 * NUM_HIDDEN + j] += Input[t * BATCH_SIZE * NUM_INPUT + i * NUM_INPUT + k] 
                        * Wxh[j * NUM_INPUT + k] + b[j];
                }
            }
        }
        Mat_t pregate_h[4 * NUM_HIDDEN * BATCH_SIZE];
        for(int i = 0; i < BATCH_SIZE; i++){
            for(int j = 0; j < 4 * NUM_HIDDEN; j++){
                pregate_h[i * 4 * NUM_HIDDEN + j] = 0;
                for(int k = 0; k < NUM_HIDDEN; k++){
                    pregate_h[i * 4 * NUM_HIDDEN + j] += h_tm1[t * BATCH_SIZE * NUM_INPUT + i * NUM_INPUT + k] 
                        * Whh[j * NUM_HIDDEN + k];
                }
            }
        }
        for(int i = 0; i < BATCH_SIZE; i++){
            for(int j = 0; j < 4 * NUM_HIDDEN; j++){
                pregate_t[i * 4 * NUM_HIDDEN + j] += pregate_h[i * 4 * NUM_HIDDEN + j];
            }
        }

        // Go through all the gates
        Mat_t gate_0[NUM_HIDDEN * BATCH_SIZE];
        Mat_t gate_1[NUM_HIDDEN * BATCH_SIZE];
        Mat_t gate_2[NUM_HIDDEN * BATCH_SIZE];
        Mat_t gate_3[NUM_HIDDEN * BATCH_SIZE];
        for(int i = 0; i < BATCH_SIZE; i++){
            for(int j = 0; j < NUM_HIDDEN; j++){
                gate_0[i * NUM_HIDDEN + j] = 1.0f / (1.0f + exp(-pregate_t[i * 4 * NUM_HIDDEN + j]));
                gate_1[i * NUM_HIDDEN + j] = 1.0f / (1.0f + exp(-pregate_t[i * 4 * NUM_HIDDEN + NUM_HIDDEN + j]));
                gate_2[i * NUM_HIDDEN + j] = 1.0f / (1.0f + exp(-pregate_t[i * 4 * NUM_HIDDEN + NUM_HIDDEN * 2 + j]));
                gate_3[i * NUM_HIDDEN + j] = tanh(pregate_t[i * 4 * NUM_HIDDEN + NUM_HIDDEN * 3 + j]);
            }
        }

        for(int i = 0; i < BATCH_SIZE; i++){
            for(int j = 0; j < NUM_HIDDEN; j++){
                c_t[i * NUM_HIDDEN + j] = gate_1[i * NUM_HIDDEN + j] * c_tm1[i * NUM_HIDDEN + j] + gate_0[i * NUM_HIDDEN + j] * gate_3[i * NUM_HIDDEN + j];
                h_t[i * NUM_HIDDEN + j] = gate_2[i * NUM_HIDDEN + j] * tanh(c_t[i * NUM_HIDDEN + j]);
            }
        }
        
        for(int i = 0; i < BATCH_SIZE; i++) {
            for(int j = 0; j < NUM_OUTPUT; j++){
                for(int k = 0; k < NUM_HIDDEN; k++){
                    sw_result[t * BATCH_SIZE * NUM_OUTPUT + i * NUM_OUTPUT + j] += h_t[i * NUM_HIDDEN + k] 
                        * Why[j * NUM_HIDDEN + k];
                }
            }
        }
#ifdef HW_COSIM
        hls_target(hw_result, Whh, Why, Wxh, b, c_tm1, h_tm1, Input+t*BATCH_SIZE*NUM_INPUT); 
#endif
        for (int i = 0; i < BATCH_SIZE; i++) {
            for (int j = 0; j < NUM_OUTPUT; j++) {
#ifdef HW_COSIM
                // Check HW result against SW
                if (hw_result[i * NUM_OUTPUT + j] != sw_result[i * NUM_OUTPUT + j]) {
                    cout << i << " " << j << " " << hw_result[i * NUM_OUTPUT + j] << " " <<  sw_result[i * NUM_OUTPUT + j] << endl;
                    err_cnt++;
                } 
#else
                cout << sw_result[i][j];
#endif
            }
        }
    }

#ifdef HW_COSIM
    if (err_cnt)
        cout << "ERROR: " << err_cnt << " mismatches detected!" << endl;
    else
        cout << "Test passes." << endl;
#endif
    return 0;
}