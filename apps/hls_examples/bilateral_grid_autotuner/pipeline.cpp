#include "Halide.h"
#include <stdio.h>

using namespace Halide;

#include "benchmark.h"
#include "halide_image_io.h"
using namespace Halide::Tools;

Var x("x"), y("y"), z("z"), c("c");
Var x_grid("x_grid"), y_grid("y_grid"), xo("xo"), yo("yo"), x_in("x_in"), y_in("y_in");
Var xi, yi;
uint8_t r_sigma = 32;
int s_sigma = 8;

class MyPipeline {
public:
  Image<uint8_t> input;
    RDom r;
    Func clamped, input2;
    Func histogram, downsampled;
    Func blurx, blury, blurz;
    Func output, hw_output;
    std::vector<Argument> args;

    MyPipeline(Image<uint8_t> in)
        : input(in), r(0, s_sigma, 0, s_sigma),
          input2("input2"), histogram("histogram"), downsampled("downsampled"),
          blurx("blurx"), blury("blury"), blurz("blurz"),
          output("output"), hw_output("hw_output")
    {
        // Add a boundary condition
        clamped = BoundaryConditions::repeat_edge(input);
        //clamped(x, y) = input(x+40, y+40);

        // Construct the bilateral grid
        Expr val = clamped(x * s_sigma + r.x - s_sigma/2, y * s_sigma + r.y - s_sigma/2);
        Expr zi = cast<int>((val + r_sigma/2) / r_sigma);
        histogram(x, y, z, c) = cast<uint16_t>(0);
        histogram(x, y, zi, c) += select(c == 0, val/16, 64/16);

        // Blur the grid using a five-tap filter
        blurz(x, y, z, c) = cast<uint16_t>(histogram(x, y, z-2, c) +
                             histogram(x, y, z-1, c)*4 +
                             histogram(x, y, z  , c)*6 +
                             histogram(x, y, z+1, c)*4 +
                             histogram(x, y, z+2, c)) / 16;
        blurx(x, y, z, c) = cast<uint16_t>(blurz(x-2, y, z, c) +
                             blurz(x-1, y, z, c)*4 +
                             blurz(x  , y, z, c)*6 +
                             blurz(x+1, y, z, c)*4 +
                             blurz(x+2, y, z, c)) / 16;
        blury(x, y, z, c) = cast<uint16_t>(blurx(x, y-2, z, c) +
                             blurx(x, y-1, z, c)*4 +
                             blurx(x, y  , z, c)*6 +
                             blurx(x, y+1, z, c)*4 +
                             blurx(x, y+2, z, c)) / 16;


        // Take trilinear samples to compute the output
        input2(x, y) = input(x, y);
        zi = cast<int>(cast<uint16_t>(input2(x, y)) / r_sigma);
        Expr zf = cast<uint16_t>((cast<uint16_t>(input2(x, y)) % r_sigma)  * (65536 / r_sigma));
        Expr xf = cast<uint16_t>((x % s_sigma) * (65536 / s_sigma));
        Expr yf = cast<uint16_t>((y % s_sigma) * (65536 / s_sigma));
        Expr xi = x/s_sigma;
        Expr yi = y/s_sigma;
        Func interpolated("interpolated");
        interpolated(x, y, c) =
            lerp(lerp(lerp(blury(xi, yi, zi, c), blury(xi+1, yi, zi, c), xf),
                      lerp(blury(xi, yi+1, zi, c), blury(xi+1, yi+1, zi, c), xf), yf),
                 lerp(lerp(blury(xi, yi, zi+1, c), blury(xi+1, yi, zi+1, c), xf),
                      lerp(blury(xi, yi+1, zi+1, c), blury(xi+1, yi+1, zi+1, c), xf), yf), zf);

        // Normalize
        val = interpolated(x, y, 0);
        Expr weight = max(interpolated(x, y, 1), 1); // to avoid underflow
        hw_output(x, y) = cast<uint8_t>(clamp(val * 64 / weight, 0, 255));
        output(x, y) = hw_output(x, y);

        // The comment constraints and schedules.
        //output.bound(x, 0, 1024).bound(y, 0, 1024);

        // Arguments
        args = {input};
    }

    void run_cpu(Image<uint8_t> out) {
        std::cout << "\ncompiling cpu code..." << std::endl;

        // The CPU schedule for halide paper
        blurz.compute_root().reorder(c, z, x, y).parallel(y).vectorize(x, 8).unroll(c);
        histogram.compute_at(blurz, y);
        histogram.update().reorder(c, r.x, r.y, x, y).unroll(c);
        blurx.compute_root().reorder(c, x, y, z).parallel(z).vectorize(x, 8).unroll(c);
        blury.compute_root().reorder(c, x, y, z).parallel(z).vectorize(x, 8).unroll(c);
        output.compute_root().parallel(y).vectorize(x, 8);

        //output.print_loop_nest();
        std::cout << "JIT compiling..." << std::endl;
        output.compile_jit();
        std::cout << "running CPU code..." << std::endl;
        double min_t = benchmark(100, 10, [&]() {
            output.realize(out);
          });
        printf("CPU program runtime: %g\n", min_t);
    }

    void run_gpu(Image<uint8_t> out) {
        std::cout << "\ncompiling gpu code..." << std::endl;

        // The GPU schedule for halide paper
        // Schedule blurz in 8x8 tiles. This is a tile in
        // grid-space, which means it represents something like
        // 64x64 pixels in the input (if s_sigma is 8).
        blurz.compute_root().reorder(c, z, x, y).gpu_tile(x, y, 8, 8);

        // Schedule histogram to happen per-tile of blurz, with
        // intermediate results in shared memory. This means histogram
        // and blurz makes a three-stage kernel:
        // 1) Zero out the 8x8 set of histograms
        // 2) Compute those histogram by iterating over lots of the input image
        // 3) Blur the set of histograms in z
        histogram.reorder(c, z, x, y).compute_at(blurz, Var::gpu_blocks()).gpu_threads(x, y);
        histogram.update().reorder(c, r.x, r.y, x, y).gpu_threads(x, y).unroll(c);

        // An alternative schedule for histogram that doesn't use shared memory:
        // histogram.compute_root().reorder(c, z, x, y).gpu_tile(x, y, 8, 8);
        // histogram.update().reorder(c, r.x, r.y, x, y).gpu_tile(x, y, 8, 8).unroll(c);

        // Schedule the remaining blurs and the sampling at the end similarly.
        blurx.compute_root().gpu_tile(x, y, z, 8, 8, 1);
        blury.compute_root().gpu_tile(x, y, z, 8, 8, 1);
        output.compute_root().gpu_tile(x, y, s_sigma, s_sigma);

        //output.print_loop_nest();

        //Target target = get_target_from_environment();
        //target.set_feature(Target::CUDA);
        //output.compile_to_header("pipeline_cuda.h", args, "pipeline_cuda", target);
        //output.compile_to_object("pipeline_cuda.o", args, "pipeline_cuda", target);

        std::cout << "JIT compiling..." << std::endl;
        output.compile_jit();
        std::cout << "running GPU code..." << std::endl;
        double min_t = benchmark(100, 10, [&]() {
            output.realize(out);
          });
        printf("GPU program runtime: %g\n", min_t);
    }
};

class MyPipelineOpt {
public:
    Image<uint8_t> input;
    RDom r;
    Func clamped;
    Func input_shuffled, input2_shuffled, output_shuffled;
    Func histogram, downsampled;
    Func blurx, blury, blurz;
    Func output;
    std::vector<Argument> args;

    MyPipelineOpt(Image<uint8_t> in)
        : input(in), r(0, s_sigma, 0, s_sigma),
          input_shuffled("input_shuffled"), input2_shuffled("input2_shuffled"),
          output_shuffled("output_shuffled"),
          histogram("histogram"), downsampled("downsampled"),
          blurx("blurx"), blury("blury"), blurz("blurz"),
          output("output")
    {
        // Add a boundary condition
        //clamped = BoundaryConditions::repeat_edge(input);
        clamped(x, y) = input(x+40, y+40);

        // shuffle the input
        input_shuffled(x_in, y_in, x_grid, y_grid)
            = clamped(x_grid*s_sigma + x_in - s_sigma/2, y_grid*s_sigma + y_in - s_sigma/2);

        // Construct the bilateral grid
        Expr val = input_shuffled(r.x, r.y, x, y);
        val = clamp(val, 0, 255);
        Expr zi = cast<int>((val + r_sigma/2) / r_sigma);
        histogram(x, y, z, c) = cast<uint16_t>(0);
        histogram(x, y, zi, c) += select(c == 0, val/16, 64/16);

        // Blur the grid using a five-tap filter
        blurz(x, y, z, c) = cast<uint16_t>(histogram(x, y, z-2, c) +
                             histogram(x, y, z-1, c)*4 +
                             histogram(x, y, z  , c)*6 +
                             histogram(x, y, z+1, c)*4 +
                             histogram(x, y, z+2, c)) / 16;
        blurx(x, y, z, c) = cast<uint16_t>(blurz(x-2, y, z, c) +
                             blurz(x-1, y, z, c)*4 +
                             blurz(x  , y, z, c)*6 +
                             blurz(x+1, y, z, c)*4 +
                             blurz(x+2, y, z, c)) / 16;
        blury(x, y, z, c) = cast<uint16_t>(blurx(x, y-2, z, c) +
                             blurx(x, y-1, z, c)*4 +
                             blurx(x, y  , z, c)*6 +
                             blurx(x, y+1, z, c)*4 +
                             blurx(x, y+2, z, c)) / 16;


        // Take trilinear samples to compute the output

        // shuffle the input
        input2_shuffled(x_in, y_in, x_grid, y_grid)
            = input(x_grid*s_sigma + x_in, y_grid*s_sigma + y_in);

        Expr sample_val = cast<uint16_t>(input2_shuffled(x_in, y_in, x_grid, y_grid));
        zi = cast<int>(sample_val / r_sigma);
        Expr zf = cast<uint16_t>((sample_val % r_sigma) * (65536 / r_sigma));
        Expr xf = cast<uint16_t>(x_in * (65536 / s_sigma));
        Expr yf = cast<uint16_t>(y_in * (65536 / s_sigma));
        Expr xi = x_grid;
        Expr yi = y_grid;
        Expr value, weight;
        value =
            lerp(lerp(lerp(blury(xi, yi, zi, 0), blury(xi+1, yi, zi, 0), xf),
                      lerp(blury(xi, yi+1, zi, 0), blury(xi+1, yi+1, zi, 0), xf), yf),
                 lerp(lerp(blury(xi, yi, zi+1, 0), blury(xi+1, yi, zi+1, 0), xf),
                      lerp(blury(xi, yi+1, zi+1, 0), blury(xi+1, yi+1, zi+1, 0), xf), yf), zf);
        weight =
            lerp(lerp(lerp(blury(xi, yi, zi, 1), blury(xi+1, yi, zi, 1), xf),
                      lerp(blury(xi, yi+1, zi, 1), blury(xi+1, yi+1, zi, 1), xf), yf),
                 lerp(lerp(blury(xi, yi, zi+1, 1), blury(xi+1, yi, zi+1, 1), xf),
                      lerp(blury(xi, yi+1, zi+1, 1), blury(xi+1, yi+1, zi+1, 1), xf), yf), zf);
        weight = max(weight, 1); // to avoid underflow

        // Normalize
        output_shuffled(x_in, y_in, x_grid, y_grid) = cast<uint8_t>(clamp(value * 64 / weight, 0, 255));

        // shuffle the output
        output(x, y) = output_shuffled(x%s_sigma, y%s_sigma, x/s_sigma, y/s_sigma);

        // The comment constraints and schedules.
        Expr out_width = output.output_buffer().width();
        Expr out_height = output.output_buffer().height();
        output
            .bound(x, 0, (out_width/480)*480)
            .bound(y, 0, (out_height/640)*640);

        // Arguments
        args = {input};
    }

    void run_cpu(Image<uint8_t> out) {
        std::cout << "\ncompiling cpu code..." << std::endl;

        // The CPU schedule for halide paper
        input_shuffled.compute_root().vectorize(x_in, 8).parallel(y_grid);
        input2_shuffled.compute_root().vectorize(x_in, 8).parallel(y_grid);
        blurz.compute_root().reorder(c, z, x, y).parallel(y).vectorize(x, 8).unroll(c);
        histogram.compute_at(blurz, y);
        histogram.update().reorder(c, r.x, r.y, x, y).unroll(c);
        blurx.compute_root().reorder(c, x, y, z).parallel(z).vectorize(x, 8).unroll(c);
        blury.compute_root().reorder(c, x, y, z).parallel(z).vectorize(x, 8).unroll(c);
        output.compute_root().parallel(y).vectorize(x, 8);
        output_shuffled.compute_root().parallel(y_grid).vectorize(x_in, 8);

        Target target = get_target_from_environment();
        target.set_feature(Target::Profile);
        //output.print_loop_nest();
        std::cout << "JIT compiling PipelineOpt..." << std::endl;
        output.compile_jit(target);
        std::cout << "running CPU code..." << std::endl;
        double min_t = benchmark(1, 10, [&]() {
            output.realize(out);
          });
        printf("CPU program runtime: %g\n", min_t);
    }

    void run_hls(Image<uint8_t> out) {
        std::cout << "\ncompiling HLS code..." << std::endl;
        output.tile(x, y, xo, yo, x_in, y_in, 240, 320);
        output.fuse(xo, yo, xo).parallel(xo);

        //output_shuffled.compute_at(output, xo);
        input_shuffled.compute_at(output, xo).vectorize(x_in, 8);
        input2_shuffled.compute_at(output, xo).vectorize(x_in, 8);
        output.vectorize(x_in, 8);

        blurz.compute_at(output, xo).reorder(c, z, x, y).vectorize(x, 8).unroll(c);
        histogram.compute_at(blurz, y);
        histogram.update().reorder(c, r.x, r.y, x, y).unroll(c);
        blurx.compute_at(output, xo).reorder(c, x, y, z).vectorize(x, 8).unroll(c);
        blury.compute_at(output, xo).reorder(c, x, y, z).vectorize(x, 8).unroll(c);

        output_shuffled.compute_at(output, xo).tile(x_grid, y_grid, xo, yo, xi, yi, 30, 40);
        output_shuffled.accelerate({blury, input2_shuffled}, xi, xo);

        //blury.linebuffer().reorder(x, y, z, c);
        //blurx.linebuffer().reorder(x, y, z, c);
        //blurz.linebuffer().reorder(z, x, y, c);
        //histogram.linebuffer().reorder(c, z, x, y).unroll(c).unroll(z);
        //histogram.update().reorder(c, r.x, r.y, x, y).unroll(c);

        //output.print_loop_nest();

        // Create the target for HLS simulation
        Target target = get_target_from_environment();
        target.set_feature(Target::Zynq);
        //target.set_feature(Target::Debug);

        //output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls", target);
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls", target);

        output.compile_to_lowered_stmt("pipeline_zynq.ir.html", args, HTML, target);

        std::cout << "JIT compiling..." << std::endl;
        output.compile_jit(target);
        std::cout << "running Zynq code..." << std::endl;
        double min_t = benchmark(100, 10, [&]() {
            output.realize(out);
          });
        printf("Zynq program runtime: %g\n", min_t);

    }
};

int main(int argc, char **argv) {
    Image<uint8_t> input = load_image(argv[1]);
    Image<uint8_t> output(480*4, 640*4);

    MyPipelineOpt p1(input);
    p1.run_cpu(output);
    //save_image(output, "out.png");

    MyPipelineOpt p2(input);
    //p2.run_hls(output);

    MyPipeline p3(input);
    //p3.run_gpu(output);
    return 0;
}
