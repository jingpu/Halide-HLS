#include "Halide.h"
#include <stdio.h>

using namespace Halide;

Var x("x"), y("y"), z("z"), c("c");
Var xo("xo"), yo("yo"), xi("xi"), yi("yi");

class MyPipeline {
public:
    ImageParam in;
    Func in_bounded;
    Func kernel_f, kernel, gray, blur_y, blur_x;
    Func sharpen, ratio;
    Func output, hw_output;
    std::vector<Argument> args;

    MyPipeline()
        : in(UInt(8), 3),
          kernel("kernel"), blur_y("blur_y"), blur_x("blur_x"),
          output("output"), hw_output("hw_output")
    {
        // Define a 9x9 Gaussian Blur with a repeat-edge boundary condition.
        float sigma = 1.5f;

        kernel_f(x) = exp(-x*x/(2*sigma*sigma)) / (sqrtf(2*M_PI)*sigma);
        // normalize and convert to 8bit fixed point
        kernel(x) = cast<uint8_t>(kernel_f(x) * 255 /
                                  (kernel_f(0) + kernel_f(1)*2 + kernel_f(2)*2
                                   + kernel_f(3)*2 + kernel_f(4)*2));

        //in_bounded = BoundaryConditions::repeat_edge(in);
        in_bounded(_0, _1, _2) = in(_0, _1 + 4, _2 + 4);

        blur_y(c, x, y) = cast<uint8_t>((kernel(0) * cast<uint16_t>(in_bounded(c, x, y)) +
                                         kernel(1) * (cast<uint16_t>(in_bounded(c, x, y-1)) +
                                                      cast<uint16_t>(in_bounded(c, x, y+1))) +
                                         kernel(2) * (cast<uint16_t>(in_bounded(c, x, y-2)) +
                                                      cast<uint16_t>(in_bounded(c, x, y+2))) +
                                         kernel(3) * (cast<uint16_t>(in_bounded(c, x, y-3)) +
                                                      cast<uint16_t>(in_bounded(c, x, y+3))) +
                                         kernel(4) * (cast<uint16_t>(in_bounded(c, x, y-4)) +
                                                      cast<uint16_t>(in_bounded(c, x, y+4)))) >> 8);

        blur_x(c, x, y) = cast<uint8_t>((kernel(0) * cast<uint16_t>(blur_y(c, x, y)) +
                                         kernel(1) * (cast<uint16_t>(blur_y(c, x-1, y)) +
                                                      cast<uint16_t>(blur_y(c, x+1, y))) +
                                         kernel(2) * (cast<uint16_t>(blur_y(c, x-2, y)) +
                                                      cast<uint16_t>(blur_y(c, x+2, y))) +
                                         kernel(3) * (cast<uint16_t>(blur_y(c, x-3, y)) +
                                                      cast<uint16_t>(blur_y(c, x+3, y))) +
                                         kernel(4) * (cast<uint16_t>(blur_y(c, x-4, y)) +
                                                      cast<uint16_t>(blur_y(c, x+4, y)))) >> 8);

        hw_output(c, x, y) = blur_x(c, x, y);

        output(c, x, y) = hw_output(c, x, y);

        //output.bound(x, 0, 1536).bound(y, 0, 2560);
        output.bound(c, 0, 3);

        in.set_stride(0, 1)
            .set_stride(1, 3);
        output.output_buffer()
            .set_stride(0, 1)
            .set_stride(1, 3);

        in.set_bounds(0, 0, 3);
        output.output_buffer().set_bounds(0, 0, 3);

        // for certain size of images
        Expr out_width = output.output_buffer().extent(1);
        Expr out_height = output.output_buffer().extent(2);
        output.bound(x, 0, (out_width/256)*256);
        output.bound(y, 0, (out_height/64)*64);

        // Arguments
        args = {in};
    }

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;
        //kernel.compute_root();

        output.tile(x, y, xo, yo, xi, yi, 256, 64)
            .unroll(c).vectorize(xi, 8)
            .fuse(xo, yo, xo).parallel(xo);
        blur_y.compute_at(output, xo).vectorize(x, 8).unroll(c);

        //output.print_loop_nest();

        output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
    }

    void compile_gpu() {
        std::cout << "\ncompiling gpu code..." << std::endl;

        output.gpu_tile(x, y, 16, 16);

        //output.print_loop_nest();

        Target target = get_target_from_environment();
        target.set_feature(Target::CUDA);
        output.compile_to_lowered_stmt("pipeline_cuda.ir.html", args, HTML, target);
        output.compile_to_header("pipeline_cuda.h", args, "pipeline_cuda", target);
        output.compile_to_object("pipeline_cuda.o", args, "pipeline_cuda", target);
    }

    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;
        //kernel.compute_root();
        output.tile(x, y, xo, yo, xi, yi, 256, 64);
        in_bounded.compute_at(output, xo);

        hw_output.compute_at(output, xo)
            .tile(x, y, xo, yo, xi, yi, 256, 64);
        hw_output.unroll(xi, 2);

        std::vector<Func> hw_bounds = hw_output.accelerate({in_bounded}, xi, xo);
        hw_bounds[0].unroll(c).unroll(x).unroll(y);

        blur_y.linebuffer().unroll(c).unroll(x).unroll(y);

        //output.print_loop_nest();
        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls");
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls");

        std::vector<Target::Feature> features({Target::HLS});
        Target target(Target::Linux, Target::ARM, 32, features);
        output.compile_to_zynq_c("pipeline_zynq.c", args, "pipeline_zynq", target);
        output.compile_to_header("pipeline_zynq.h", args, "pipeline_zynq", target);

        output.vectorize(xi, 16).unroll(c);
        in_bounded.vectorize(_1, 16).unroll(_0);
        output.fuse(xo, yo, xo).parallel(xo);

        output.compile_to_object("pipeline_zynq.o", args, "pipeline_zynq", target);
        output.compile_to_lowered_stmt("pipeline_zynq.ir.html", args, HTML, target);
    }
};

int main(int argc, char **argv) {
    MyPipeline p1;
    p1.compile_cpu();

    MyPipeline p2;
    p2.compile_hls();

    MyPipeline p3;
    p3.compile_gpu();
    return 0;
}
