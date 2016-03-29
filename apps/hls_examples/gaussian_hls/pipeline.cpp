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
        // Define a 7x7 Gaussian Blur with a repeat-edge boundary condition.
        float sigma = 1.5f;

        kernel_f(x) = exp(-x*x/(2*sigma*sigma)) / (sqrtf(2*M_PI)*sigma);
        // normalize and convert to 8bit fixed point
        kernel(x) = cast<uint8_t>(kernel_f(x) * 256 /
                                  (kernel_f(0) + kernel_f(1)*2 + kernel_f(2)*3));

        in_bounded = BoundaryConditions::repeat_edge(in);

        blur_y(c, x, y) = cast<uint8_t>((kernel(0) * cast<uint16_t>(in_bounded(c, x, y)) +
                                         kernel(1) * (cast<uint16_t>(in_bounded(c, x, y-1)) +
                                                      cast<uint16_t>(in_bounded(c, x, y+1))) +
                                         kernel(2) * (cast<uint16_t>(in_bounded(c, x, y-2)) +
                                                      cast<uint16_t>(in_bounded(c, x, y+2))) +
                                         kernel(3) * (cast<uint16_t>(in_bounded(c, x, y-3)) +
                                                      cast<uint16_t>(in_bounded(c, x, y+3)))) >> 8);

        blur_x(c, x, y) = cast<uint8_t>((kernel(0) * cast<uint16_t>(blur_y(c, x, y)) +
                                         kernel(1) * (cast<uint16_t>(blur_y(c, x-1, y)) +
                                                      cast<uint16_t>(blur_y(c, x+1, y))) +
                                         kernel(2) * (cast<uint16_t>(blur_y(c, x-2, y)) +
                                                      cast<uint16_t>(blur_y(c, x+2, y))) +
                                         kernel(3) * (cast<uint16_t>(blur_y(c, x-3, y)) +
                                                      cast<uint16_t>(blur_y(c, x+3, y)))) >> 8);

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

        // Arguments
        args = {in};
    }

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;
        //kernel.compute_root();

        output.tile(x, y, xo, yo, xi, yi, 256, 256)
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

        output.gpu_tile(x, y, c, 16, 8, 3).unroll(c);

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
        output.tile(x, y, xo, yo, xi, yi, 256, 256);

        hw_output.compute_at(output, xo);
        hw_output.tile(x, y, xo, yo, xi, yi, 256, 256);
        in_bounded.compute_at(output, xo);

        std::vector<Func> hw_bounds = hw_output.accelerate({in_bounded}, xi, xo);
        hw_bounds[0].unroll(c);

        blur_y.linebuffer().unroll(c);

        //output.print_loop_nest();
        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls");
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls");
    }
};

int main(int argc, char **argv) {
    MyPipeline p1;
    p1.compile_cpu();

    MyPipeline p2;
    p2.compile_hls();

    MyPipeline p3;
    //p3.compile_gpu();
    return 0;
}
