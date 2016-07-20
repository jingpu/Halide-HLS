#include "Halide.h"
#include <stdio.h>

using namespace Halide;

Var x("x"), y("y"), z("z"), c("c");
Var x_grid("x_grid"), y_grid("y_grid"), xo("xo"), yo("yo"), x_in("x_in"), y_in("y_in");
uint8_t r_sigma = 32;
int s_sigma = 8;

class MyPipeline {
public:
    ImageParam input;
    RDom r;
    Func clamped, input2;
    Func histogram, downsampled;
    Func blurx, blury, blurz;
    Func output, hw_output;
    std::vector<Argument> args;

    MyPipeline()
        : input(UInt(8), 2), r(0, s_sigma, 0, s_sigma),
          input2("input2"), histogram("histogram"), downsampled("downsampled"),
          blurx("blurx"), blury("blury"), blurz("blurz"),
          output("output"), hw_output("hw_output")
    {
        // Add a boundary condition
        //clamped = BoundaryConditions::repeat_edge(input);
        clamped(x, y) = input(x+40, y+40);

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

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;

        // The CPU schedule for halide paper
        blurz.compute_root().reorder(c, z, x, y).parallel(y).vectorize(x, 8).unroll(c);
        histogram.compute_at(blurz, y);
        histogram.update().reorder(c, r.x, r.y, x, y).unroll(c);
        blurx.compute_root().reorder(c, x, y, z).parallel(z).vectorize(x, 8).unroll(c);
        blury.compute_root().reorder(c, x, y, z).parallel(z).vectorize(x, 8).unroll(c);
        output.compute_root().parallel(y).vectorize(x, 8);

        //output.print_loop_nest();

        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
    }

    void compile_gpu() {
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

        Target target = get_target_from_environment();
        target.set_feature(Target::CUDA);
        output.compile_to_header("pipeline_cuda.h", args, "pipeline_cuda", target);
        output.compile_to_object("pipeline_cuda.o", args, "pipeline_cuda", target);
    }

    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;

        output.tile(x, y, xo, yo, x_in, y_in, 256, 256);
        output.tile(x_in, y_in, x_grid, y_grid, x_in, y_in, 8, 8);

        blury.store_at(output, xo).compute_at(output, x_grid).reorder(x, y, z, c);
        blurx.store_at(output, xo).compute_at(output, x_grid).reorder(x, y, z, c);
        blurz.store_at(output, xo).compute_at(output, x_grid).reorder(z, x, y, c);

        histogram.store_at(output, xo).compute_at(output, x_grid).reorder(c, z, x, y).unroll(c).unroll(z);
        histogram.update().reorder(c, r.x, r.y, x, y).unroll(c);

        clamped.store_at(output, xo).compute_at(output, x_grid);
        input2.store_at(output, xo).compute_at(output, x_grid);

        hw_output.compute_at(output, xo);
        hw_output.accelerate({clamped, input2}, x_grid, xo);

        //output.print_loop_nest();

        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls");
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls");
    }
};

class MyPipelineOpt {
public:
    ImageParam input;
    RDom r;
    Func clamped;
    Func input_shuffled, input2_shuffled, output_shuffled;
    Func histogram, downsampled;
    Func blurx, blury, blurz;
    Func output;
    std::vector<Argument> args;

    MyPipelineOpt()
        : input(UInt(8), 2), r(0, s_sigma, 0, s_sigma),
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


    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;
        output.tile(x, y, xo, yo, x_in, y_in, 480, 640);

        output_shuffled.compute_at(output, xo);
        input_shuffled.compute_at(output, xo);
        input2_shuffled.compute_at(output, xo);

        output_shuffled.tile(x_grid, y_grid, xo, yo, x_grid, y_grid, 60, 80);
        output_shuffled.accelerate({input_shuffled, input2_shuffled}, x_grid, xo);

        blury.linebuffer().reorder(x, y, z, c);
        blurx.linebuffer().reorder(x, y, z, c);
        blurz.linebuffer().reorder(z, x, y, c);
        histogram.linebuffer().reorder(c, z, x, y).unroll(c).unroll(z);
        histogram.update().reorder(c, r.x, r.y, x, y).unroll(c);

        //output.print_loop_nest();

        // Create the target for HLS simulation
        Target hls_target = get_target_from_environment();
        hls_target.set_feature(Target::CPlusPlusMangling);
        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML, hls_target);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls", hls_target);
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls", hls_target);

        // Create the Zynq platform target
        std::vector<Target::Feature> features({Target::Zynq});
        Target target(Target::Linux, Target::ARM, 32, features);

        output.compile_to_zynq_c("pipeline_zynq.c", args, "pipeline_zynq", target);
        output.compile_to_header("pipeline_zynq.h", args, "pipeline_zynq", target);

        // Vectorization and Parallelization Schedules (only work with LLVM codegen)
        input_shuffled.vectorize(x_in, 8);
        input2_shuffled.vectorize(x_in, 8);
        output.vectorize(x_in, 8);

        output.fuse(xo, yo, xo).parallel(xo);

        output.compile_to_lowered_stmt("pipeline_zynq.ir.html", args, HTML, target);
        output.compile_to_object("pipeline_zynq.o", args, "pipeline_zynq", target);
        output.compile_to_assembly("pipeline_zynq.s", args, "pipeline_zynq", target);
    }
};

int main(int argc, char **argv) {
    MyPipeline p1;
    p1.compile_cpu();

    MyPipelineOpt p2;
    p2.compile_hls();

    MyPipeline p3;
    p3.compile_gpu();
    return 0;
}
