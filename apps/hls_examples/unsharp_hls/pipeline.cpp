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
          kernel("kernel"), gray("gray"), blur_y("blur_y"), blur_x("blur_x"),
          sharpen("sharpen"), ratio("ratio"),
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
        in_bounded(c, x, y) = in(x+4, y+4, c);

        gray(x, y) = cast<uint8_t>((77 * cast<uint16_t>(in_bounded(0, x, y))
                                    + 150 * cast<uint16_t>(in_bounded(1, x, y))
                                    + 29 * cast<uint16_t>(in_bounded(2, x, y))) >> 8);

        blur_y(x, y) = cast<uint8_t>((kernel(0) * cast<uint16_t>(gray(x, y)) +
                                      kernel(1) * (cast<uint16_t>(gray(x, y-1)) +
                                                   cast<uint16_t>(gray(x, y+1))) +
                                      kernel(2) * (cast<uint16_t>(gray(x, y-2)) +
                                                   cast<uint16_t>(gray(x, y+2))) +
                                      kernel(3) * (cast<uint16_t>(gray(x, y-3)) +
                                                   cast<uint16_t>(gray(x, y+3))) +
                                      kernel(4) * (cast<uint16_t>(gray(x, y-4)) +
                                                   cast<uint16_t>(gray(x, y+4)))) >> 8);

        blur_x(x, y) = cast<uint8_t>((kernel(0) * cast<uint16_t>(blur_y(x, y)) +
                                      kernel(1) * (cast<uint16_t>(blur_y(x-1, y)) +
                                                   cast<uint16_t>(blur_y(x+1, y))) +
                                      kernel(2) * (cast<uint16_t>(blur_y(x-2, y)) +
                                                   cast<uint16_t>(blur_y(x+2, y))) +
                                      kernel(3) * (cast<uint16_t>(blur_y(x-3, y)) +
                                                   cast<uint16_t>(blur_y(x+3, y))) +
                                      kernel(4) * (cast<uint16_t>(blur_y(x-4, y)) +
                                                   cast<uint16_t>(blur_y(x+4, y)))) >> 8);

        sharpen(x, y) = cast<uint8_t>(clamp(2 * cast<uint16_t>(gray(x, y)) - blur_x(x, y), 0, 255));

        ratio(x, y) = cast<uint8_t>(clamp(cast<uint16_t>(sharpen(x, y)) * 32 / max(gray(x, y), 1), 0, 255));

        hw_output(c, x, y) = cast<uint8_t>(clamp(cast<uint16_t>(ratio(x, y)) * in_bounded(c, x, y) / 32, 0, 255));

        output(x, y, c) = hw_output(c, x, y);

        output.bound(c, 0, 3);

        // Arguments
        args = {in};
    }

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;
        //kernel.compute_root();

        output.tile(x, y, xo, yo, xi, yi, 256, 256);
        gray.compute_at(output, xo).vectorize(x, 8);
        blur_y.compute_at(output, xo).vectorize(x, 8);
        ratio.compute_at(output, xo).vectorize(x, 8);
        output.vectorize(xi, 8).reorder(xi, yi, c, xo, yo);

        output.fuse(xo, yo, xo).parallel(xo);

        //output.print_loop_nest();

        //output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
    }

    void compile_gpu() {
        std::cout << "\ncompiling gpu code..." << std::endl;

        if (true) {
            output.gpu_tile(x, y, c, 16, 16, 1);
            gray.compute_root().gpu_tile(x, y, 16, 16);
            blur_y.compute_root().gpu_tile(x, y, 16, 16);
            ratio.compute_root().gpu_tile(x, y, 16, 16);
        } else {
            output.gpu_tile(x, y, c, 16, 16, 1);
            gray.compute_at(output, Var::gpu_blocks()).gpu_threads(x, y);
            blur_y.compute_at(output, Var::gpu_blocks()).gpu_threads(x, y);
            ratio.compute_at(output, Var::gpu_blocks()).gpu_threads(x, y);
        }

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
        output.tile(x, y, xo, yo, xi, yi, 480, 640).reorder(c, xi, yi, xo, yo);

        hw_output.compute_at(output, xo);
        hw_output.tile(x, y, xo, yo, xi, yi, 480, 640).reorder(c, xi, yi, xo, yo);
        in_bounded.compute_at(output, xo);

        std::vector<Func> hw_bounds = hw_output.accelerate({in_bounded}, xi, xo);
        gray.linebuffer().fifo_depth(ratio, 8);
        blur_y.linebuffer();
        ratio.linebuffer();
        hw_bounds[0].unroll(c);  // hw output bound
        hw_bounds[1].fifo_depth(hw_bounds[0], 480*9); // hw input bounds

        //output.print_loop_nest();
        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls");
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls");

        std::vector<Target::Feature> features({Target::HLS});
        Target target(Target::Linux, Target::ARM, 32, features);
        output.compile_to_zynq_c("pipeline_zynq.c", args, "pipeline_zynq", target);
        output.compile_to_header("pipeline_zynq.h", args, "pipeline_zynq", target);

        in_bounded.vectorize(x, 16).unroll(c);
        output.reorder(c, xi, yi, xo, yo)
            .vectorize(xi, 16).unroll(c);
        output.fuse(xo, yo, xo).parallel(xo);

        output.compile_to_object("pipeline_zynq.o", args, "pipeline_zynq", target);
        output.compile_to_lowered_stmt("pipeline_zynq.ir.html", args, HTML, target);
        output.compile_to_assembly("pipeline_zynq.s", args, "pipeline_zynq", target);
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
