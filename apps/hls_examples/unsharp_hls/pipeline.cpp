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

    RDom win, win2;
    Func sum_y, sum_x;

    MyPipeline()
        : in(UInt(8), 3),
          kernel("kernel"), gray("gray"), blur_y("blur_y"), blur_x("blur_x"),
          sharpen("sharpen"), ratio("ratio"),
          output("output"), hw_output("hw_output"),
          win(-4, 9), win2(-4, 9, -4, 9)
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

        if (false) {
            // 2D filter: seperate x and y dim
            sum_y(x, y) += kernel(win.x) * cast<uint16_t>(gray(x, y+win.x));
            blur_y(x, y) = cast<uint8_t>(sum_y(x, y) >> 8);

            sum_x(x, y) += kernel(win.x) * cast<uint16_t>(blur_y(x+win.x, y));
            blur_x(x, y) = cast<uint8_t>(sum_x(x, y) >> 8);

            sum_y.update(0).unroll(win.x);
            sum_x.update(0).unroll(win.x);
        } else {

            // 2D filter: direct map
            sum_x(x, y) += cast<uint32_t>(gray(x+win2.x, y+win2.y)) * kernel(win2.x) * kernel(win2.y);
            blur_x(x, y) = cast<uint8_t>(sum_x(x, y) >> 16);

            sum_x.update(0).unroll(win2.x).unroll(win2.y);
        }

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

        output.tile(x, y, xo, yo, xi, yi, 240, 360);
        gray.compute_at(output, xo).vectorize(x, 8);
        //blur_y.compute_at(output, xo).vectorize(x, 8);
        ratio.compute_at(output, xo).vectorize(x, 8);
        output.vectorize(xi, 8).reorder(xi, yi, c, xo, yo);

        output.fuse(xo, yo, xo).parallel(xo);

        //output.print_loop_nest();
        output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
    }

    void compile_gpu() {
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

    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;
        //kernel.compute_root();

        output.tile(x, y, xo, yo, xi, yi, 480, 640).reorder(c, xi, yi, xo, yo);

        hw_output.compute_at(output, xo);
        hw_output.tile(x, y, xo, yo, xi, yi, 480, 640).reorder(c, xi, yi, xo, yo);
        in_bounded.compute_at(output, xo);

        hw_output.accelerate({in_bounded}, xi, xo);
        gray.linebuffer().fifo_depth(ratio, 20);
        //blur_y.linebuffer();
        ratio.linebuffer();
        hw_output.unroll(c);  // hw output bound
        in_bounded.fifo_depth(hw_output, 480*9); // hw input bounds

        //output.print_loop_nest();
        // Create the target for HLS simulation
        Target hls_target = get_target_from_environment();
        hls_target.set_feature(Target::CPlusPlusMangling);
        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML, hls_target);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls", hls_target);
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls", hls_target);

        std::vector<Target::Feature> features({Target::Zynq});
        Target target(Target::Linux, Target::ARM, 32, features);
        output.compile_to_zynq_c("pipeline_zynq.c", args, "pipeline_zynq", target);
        output.compile_to_header("pipeline_zynq.h", args, "pipeline_zynq", target);

        in_bounded.vectorize(x, 8).unroll(c);
        output.reorder(c, xi, yi, xo, yo)
            .vectorize(xi, 8).unroll(c);
        output.fuse(xo, yo, xo).parallel(xo);

        //in.set_stride(0, 3).set_stride(2, 1).set_bounds(2, 0, 3);
        output.output_buffer().set_stride(0, 3).set_stride(2, 1);

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
