#include "Halide.h"
#include <stdio.h>

using namespace Halide;

Var x("x"), y("y"), z("z"), c("c");
Var xo("xo"), yo("yo"), xi("xi"), yi("yi");
Var xio("xio"), xii("xii");
RVar ro("ro"), ri("ri");
class MyPipeline {
public:
    ImageParam A;
    ImageParam B;

    //Func prod;
    Func A_buf_copy;
    Func B_buf_copy;
    Func output_buf;
    Func output;
    Func hw_output;
    std::vector<Argument> args;

    RDom r;

    MyPipeline()
        : A(UInt(8), 2), B(UInt(8), 2), 
          //prod("prod"),
          output("output"), hw_output("hw_output"),
          r(0, 8, 0, 8)
    {

        //Copy handler
        A_buf_copy(x, y) = A(x, y); 
        B_buf_copy(x, y) = B(x, y);

        hw_output(x, y) += cast<uint16_t>(A_buf_copy(r.y*8+r.x, x))* cast<uint16_t>(B_buf_copy(r.y*8+r.x, y)); //A transpose

        //hw_output(x, y) = output_buf(x, y);
        output(x, y) = hw_output(x, y);
        A.dim(1).set_bounds(0, 384);
        A.dim(0).set_bounds(0, 64);
        A.dim(0).set_stride(1);
        A.dim(1).set_stride(64);
        B.dim(1).set_bounds(0, 384);
        B.dim(0).set_bounds(0, 64);
        B.dim(0).set_stride(1);
        B.dim(1).set_stride(64);

        output.bound(x, 0, 384);
        output.bound(y, 0, 384);

        // Arguments
        args = {A, B};
    }

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;


        //output.tile(x, y, xo, yo, xi, yi, 4, 4);
        //output.reorder(xi, yi, xo, yo);

        /*hw_output.update().tile(x, y, xo, yo, xi, yi, 16, 16);
        hw_output.update().reorder(xi, yi, xo, yo);
        hw_output.compute_root(); //compute_at(output, yo);
        */
       
        //hw_output_buf.update().tile(x, y, xo, yo, xi, yi, 16, 16);
        //hw_output_buf.update().reorder(xi, yi, xo, yo);
        output.tile(x, y, xo, yo, xi, yi, 16, 16);
        output.reorder(xi, yi, xo, yo);
        hw_output.compute_at(output, xo);
        A_buf_copy.compute_at(output, xo);
        B_buf_copy.compute_at(output, xo);


        //output_buf.compute_at(hw_output, xi);
        //hw_output.compute_root(); //compute_at(output, yo);
        

        //prod.tile(x, y, xo, yo, xi, yi, 4, 4);
        //prod.reorder(xi, yi, xo, yo);
        //prod.compute_at(output, xi);

        //output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
        output.compile_to_c("pipeline_gemm.cpp", args, "gemm");
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
        output.print_loop_nest();
 
    }

    /*void compile_gpu() {
        std::cout << "\ncompiling gpu code..." << std::endl;
        //kernel.compute_root().gpu_tile(x, 16);
        output.gpu_tile(x, y, c, 16, 16, 1);
        gray.compute_root().gpu_tile(x, y, 16, 16);
        //blur_y.compute_root().gpu_tile(x, y, 16, 16);
        ratio.compute_root().gpu_tile(x, y, 32, 16);
        //output.print_loop_nest();
        Target target = get_target_from_environment();
        target.set_feature(Target::CUDA);
        output.compile_to_lowered_stmt("pipeline_cuda.ir.html", args, HTML, target);
        output.compile_to_header("pipeline_cuda.h", args, "pipeline_cuda", target);
        output.compile_to_object("pipeline_cuda.o", args, "pipeline_cuda", target);
    }
    */
    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;
        output.tile(x, y, xo, yo, xi, yi, 192, 192);
        output.reorder(xi, yi, xo, yo);

        hw_output.update(0).split(x, xo, xi, 6);
        hw_output.update(0).split(y, yo, yi, 4);
        hw_output.update(0).reorder(r.x, xi, yi, xo, yo, r.y);

        hw_output.compute_at(output, xo);
        A_buf_copy.compute_at(output, xo);
        B_buf_copy.compute_at(output, xo);
        //hw_output.update(0).unroll(r.x);
        //hw_output.update().tile(x, y, xo, yo, xi, yi, 16, 16);
        //hw_output.update().reorder(xi, yi, xo, yo);
        //hw_output_buf.update().tile(x, y, xo, yo, xi, yi, 16, 16);
        //hw_output_buf.update().reorder(xi, yi, xo, yo);

        //output_buf.compute_at(hw_output, xi);
        //hw_output.compute_root(); //compute_at(output, yo);

        output.accelerate({A,B}, xo, xo);
 
        Target hls_target = get_target_from_environment();
        hls_target.set_feature(Target::CPlusPlusMangling);
        output.print_loop_nest();
        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML, hls_target);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls", hls_target);
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls", hls_target);
 
    }
};

class MyPipelineConfigurable{

public:
    ImageParam A;
    ImageParam B;
    Param<int32_t> x_len;
    Param<int32_t> y_len;
    Param<int32_t> r_len;
    //Func prod;
    Func A_buf_copy;
    Func B_buf_copy;
    Func output_buf;
    Func output;
    Func hw_output;
    std::vector<Argument> args;

    RDom r;

    MyPipelineConfigurable()
    : A(UInt(8), 2), B(UInt(8), 2), 
    x_len("x_len"), y_len("y_len"), r_len("r_len"),
    output("output"), hw_output("hw_output"),
    r(0, 8, 0, r_len)
    {

    //Copy handler
    A_buf_copy(x, y) = A(x, y); 
    B_buf_copy(x, y) = B(x, y);

    hw_output(x, y) += cast<uint16_t>(A_buf_copy(r.y*8+r.x, x))* cast<uint16_t>(B_buf_copy(r.y*8+r.x, y)); //A transpose

    //hw_output(x, y) = output_buf(x, y);
    output(x, y) = hw_output(x, y);
    A.dim(1).set_bounds(0, x_len);
    A.dim(0).set_bounds(0, r_len * 8);
    A.dim(0).set_stride(1);
    A.dim(1).set_stride(r_len*8);
    B.dim(1).set_bounds(0, y_len);
    B.dim(0).set_bounds(0, r_len * 8);
    B.dim(0).set_stride(1);
    B.dim(1).set_stride(r_len * 8);

    output.bound(x, 0, x_len);
    output.bound(y, 0, y_len);

    // Arguments
    args = {A, B, x_len, y_len, r_len};
    }

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;


        //output.tile(x, y, xo, yo, xi, yi, 4, 4);
        //output.reorder(xi, yi, xo, yo);

        /*hw_output.update().tile(x, y, xo, yo, xi, yi, 16, 16);
        hw_output.update().reorder(xi, yi, xo, yo);
        hw_output.compute_root(); //compute_at(output, yo);
        */
       
        //hw_output_buf.update().tile(x, y, xo, yo, xi, yi, 16, 16);
        //hw_output_buf.update().reorder(xi, yi, xo, yo);
        output.tile(x, y, xo, yo, xi, yi, 16, 16);
        output.reorder(xi, yi, xo, yo);
        hw_output.compute_at(output, xo);
        A_buf_copy.compute_at(output, xo);
        B_buf_copy.compute_at(output, xo);


        //output_buf.compute_at(hw_output, xi);
        //hw_output.compute_root(); //compute_at(output, yo);
        

        //prod.tile(x, y, xo, yo, xi, yi, 4, 4);
        //prod.reorder(xi, yi, xo, yo);
        //prod.compute_at(output, xi);

        //output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
        output.compile_to_c("pipeline_gemm.cpp", args, "gemm");
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
        output.print_loop_nest();
 
    }

    /*void compile_gpu() {
        std::cout << "\ncompiling gpu code..." << std::endl;
        //kernel.compute_root().gpu_tile(x, 16);
        output.gpu_tile(x, y, c, 16, 16, 1);
        gray.compute_root().gpu_tile(x, y, 16, 16);
        //blur_y.compute_root().gpu_tile(x, y, 16, 16);
        ratio.compute_root().gpu_tile(x, y, 32, 16);
        //output.print_loop_nest();
        Target target = get_target_from_environment();
        target.set_feature(Target::CUDA);
        output.compile_to_lowered_stmt("pipeline_cuda.ir.html", args, HTML, target);
        output.compile_to_header("pipeline_cuda.h", args, "pipeline_cuda", target);
        output.compile_to_object("pipeline_cuda.o", args, "pipeline_cuda", target);
    }
    */
    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;
        output.tile(x, y, xo, yo, xi, yi, 192, 192);
        output.reorder(xi, yi, xo, yo);

        hw_output.update(0).split(x, xo, xi, 6);
        hw_output.update(0).split(y, yo, yi, 4);
        hw_output.update(0).reorder(r.x, xi, yi, xo, yo, r.y);

        hw_output.compute_at(output, xo);
        A_buf_copy.compute_at(hw_output, r.y);
        B_buf_copy.compute_at(hw_output, r.y);
        //hw_output.update(0).unroll(r.x);
        //hw_output.update().tile(x, y, xo, yo, xi, yi, 16, 16);
        //hw_output.update().reorder(xi, yi, xo, yo);
        //hw_output_buf.update().tile(x, y, xo, yo, xi, yi, 16, 16);
        //hw_output_buf.update().reorder(xi, yi, xo, yo);

        //output_buf.compute_at(hw_output, xi);
        //hw_output.compute_root(); //compute_at(output, yo);

        output.accelerate({A,B}, xo, xo);
 
        Target hls_target = get_target_from_environment();
        hls_target.set_feature(Target::CPlusPlusMangling);
        output.print_loop_nest();
        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML, hls_target);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls", hls_target);
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls", hls_target);
 
    }
};


int main(int argc, char **argv) {
    /*
    MyPipeline p1;
    p1.compile_cpu();

    MyPipeline p2;
    p2.compile_hls();
    */
    MyPipelineConfigurable p3;
    p3.compile_cpu();
    
    MyPipelineConfigurable p4;
    p4.compile_hls();
    
    return 0;
}
