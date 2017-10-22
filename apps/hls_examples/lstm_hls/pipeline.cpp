#include "Halide.h"
#include <stdio.h>

using namespace Halide;

// Hyper Params
#define NUM_INPUT 16
#define NUM_HIDDEN 16
#define NUM_OUTPUT 16
#define BATCH_SIZE 16
#define T 4

// Vars
Var x("x"), y("y"), z("z");

class MyPipeline {
public:
    // Params
    ImageParam input;       // All inputs, (NUM_INPUT, BATCH_SIZE, T)
    ImageParam Wxh;         // Weight from input to hidden, (NUM_INPUT, 4*NUM_HIDDEN)
    ImageParam Whh;         // Recurrent weight of hidden, (NUM_HIDDEN, 4*NUM_HIDDEN)
    ImageParam b;           // Bias, (4*NUM_HIDDEN, 1)

    // Reduction domain iterator
    RDom rx;
    RDom rh;
    RDom ry;
    RDom rb;

    // Funcs
    Func input_buf_copy;                    // Buff copy for input
    Func Wxh_buf_copy;                      // Buff copy for Wxh
    Func Whh_buf_copy;                      // Buff copy for Whh
    Func b_buf_copy;                        // Buff copy for b
    Func h_init;                            // Initial state for h (4*NUM_HIDDEN, BATCH_SIZE)
    Func c_init;                            // Initial state for c, (4*NUM_HIDDEN, BATCH_SIZE)
    std::vector<Func> h;                    // All hiddens, (NUM_HIDDEN, BATCH_SIZE)
    std::vector<Func> c;                    // All cell states, (NUM_HIDDEN, BATCH_SIZE)
    Func output;                            // Output equals to all hidden units, (NUM_HIDDEN, BATCH_SIZE, T)
    
    std::vector<Argument> args;

    MyPipeline():input(Float(32),3),Wxh(Float(32),2),Whh(Float(32),2),b(Float(32),2),
        rx(0,NUM_INPUT),rh(0,NUM_HIDDEN),ry(0,NUM_OUTPUT),rb(0,BATCH_SIZE),
        output("output")
    {
        // Initiation
        input_buf_copy(x,y,z) = input(x,y,z);
        Wxh_buf_copy(x,y) = Wxh(x,y);
        Whh_buf_copy(x,y) = Whh(x,y);
        b_buf_copy(x,y) = b(x,y);
        
        for (int t = 0; t < T; t++) {
            std::string t_str = std::to_string(t);
            h.push_back(Func("h_["+t_str+"]"));
            c.push_back(Func("c_["+t_str+"]"));
        }
        h_init(x,y) = 0.0f;
        c_init(x,y) = 0.0f;

        for (int t = 0; t < T; t++) {
            Func &h_tm1 = t == 0 ? h_init : h[t - 1];
            Func &c_tm1 = t == 0 ? c_init : c[t - 1];
            Func input_t;
            input_t(x,y) = input_buf_copy(x,y,t);
            Func pre_gate_ub;
            pre_gate_ub(x,y) = sum(input_t(rx,y) * Wxh(rx,x));
            Func pre_gate_t;
            pre_gate_t(x,y) = pre_gate_ub(x,y) + b(x,0);
            Func pre_gate_h;
            pre_gate_h(x,y) = sum(h_tm1(rh,y) * Whh(rh,x));
            pre_gate_t(x, y) += pre_gate_h(x, y);
            
            // go through gates
            Func gate[4];
            gate[0](x, y) = 1.0f / (1.0f + fast_exp(-pre_gate_t(x, y)));
            if (t == 0) {
                gate[1](x, y) = 0.0f;
            } else {
                gate[1](x, y) = 1.0f / (1.0f + fast_exp(-pre_gate_t(x+NUM_HIDDEN,y)));
            }
            gate[2](x, y) = 1.0f / (1.0f + fast_exp(-pre_gate_t(x+2*NUM_HIDDEN,y)));
            gate[3](x, y) = tanh(pre_gate_t(x+3*NUM_HIDDEN,y));

            c[t](x, y) = gate[1](x, y) * c_tm1(x, y) + gate[0](x, y) * gate[3](x, y);
            h[t](x, y) = gate[2](x, y) * tanh(c[t](x, y));
        }
        output(x, y, z) = 0.0f;
        for (int t = 0; t < T; t++) {
            output(x, y, t) = h[t](x, y);
        }
        args = {input, Wxh, Whh, b};
    }

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;

        output.compile_to_c("pipeline_lstm.cpp", args, "lstm");
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
        output.print_loop_nest();
    }
    
    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;
        
        //output.accelerate({input, Wxh, Whh, b}, x, y);
 
        Target hls_target = get_target_from_environment();
        hls_target.set_feature(Target::CPlusPlusMangling);
        output.print_loop_nest();
        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML, hls_target);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls", hls_target);
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls", hls_target); 
    }
    
};

int main(int argc, char **argv) {
  
    MyPipeline p1;
    p1.compile_cpu();
    /*
    MyPipeline p2;
    p2.compile_hls();
    */
    return 0;
}
