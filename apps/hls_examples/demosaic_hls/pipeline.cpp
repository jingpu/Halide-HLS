#include "Halide.h"
#include <stdio.h>

using namespace Halide;

Var x("x"), y("y"), c("c");
Var xo("xo"), yo("yo"), xi("xi"), yi("yi");

uint8_t phase = 0;

class MyPipeline {
public:
    ImageParam input;
    std::vector<Argument> args;
    //Param<uint8_t> phase; // One bit each for x and y phase
    Func padded;
    Func red, green, blue;
    Func demosaic, lowpass_x, lowpass_y, downsample;
    Func output, hw_output;
    Func neswNeighbors, diagNeighbors, vNeighbors, hNeighbors;

    MyPipeline()
        : input(UInt(8), 2), padded("padded"),
          demosaic("demosaic"), lowpass_x("lowpass_x"),
          lowpass_y("lowpass_y"), downsample("downsample"),
          output("output"), hw_output("hw_output")
    {
        //padded = BoundaryConditions::constant_exterior(input, 0);
        //padded(x, y) = input(x+240, y+60);
        padded(x, y) = input(x+200, y+40);

        // Now demosaic and try to get RGB back
        Func padded16;
        padded16(x, y) = cast<uint16_t>(padded(x, y));
        neswNeighbors(x, y) = cast<uint8_t>((padded16(x-1, y) + padded16(x+1, y) +
                                             padded16(x, y-1) + padded16(x, y+1))/4);
        diagNeighbors(x, y) = cast<uint8_t>((padded16(x-1, y-1) + padded16(x+1, y-1) +
                                             padded16(x-1, y+1) + padded16(x+1, y+1))/4);

        vNeighbors(x, y) = cast<uint8_t>((padded16(x, y-1) + padded16(x, y+1))/2);
        hNeighbors(x, y) = cast<uint8_t>((padded16(x-1, y) + padded16(x+1, y))/2);

        green(x, y) = select((y % 2) == (phase / 2),
             select((x % 2) == (phase % 2), neswNeighbors(x, y), padded(x, y)), // First row, RG
             select((x % 2) == (phase % 2), padded(x, y), neswNeighbors(x, y))); // Second row, GB

        red(x, y) = select((y % 2) == (phase / 2),
             select((x % 2) == (phase % 2), padded(x, y), hNeighbors(x, y)), // First row, RG
             select((x % 2) == (phase % 2), vNeighbors(x, y), diagNeighbors(x, y))); // Second row, GB

        blue(x, y) = select((y % 2) == (phase / 2),
             select((x % 2) == (phase % 2), diagNeighbors(x, y), vNeighbors(x, y)), // First row, RG
             select((x % 2) == (phase % 2), hNeighbors(x, y), padded(x, y))); // Second row, GB

        demosaic(c, x, y) = cast<uint8_t>(select(c == 0, red(x, y),
                                                 c == 1, green(x, y),
                                                 blue(x, y)));

        // lowpass filter before downsample
        lowpass_y(c, x, y) = cast<uint8_t>((cast<uint16_t>(demosaic(c, x, y)) +
                                            cast<uint16_t>(demosaic(c, x+1, y)))/2);
        lowpass_x(c, x, y) = cast<uint8_t>((cast<uint16_t>(lowpass_y(c, x, y)) +
                                            cast<uint16_t>(lowpass_y(c, x, y+1)))/2);

        //hw_output(c, x, y) = demosaic(c, x, y);
        hw_output(c, x, y) = lowpass_x(c, x, y);

        // downsample
        downsample(c, x, y) = hw_output(c, x*2, y*2);

        output(x, y, c) = downsample(c, x, y);
        //output(x, y, c) = hw_output(c, x, y);

        // common constraints
        output.bound(c, 0, 3);
        // We can generate slightly better code if we know the output is a whole number of tiles.
        output.bound(x, 0, 1440/2).bound(y, 0, 960/2);

        // Arguments
        args = {input};
    }

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;

        output.tile(x, y, xo, yo, xi, yi, 120, 120)
            .vectorize(xi, 8);

        // ARM schedule use interleaved vector instructions
        demosaic.compute_at(output, xo)
            .tile(x, y, xo, yo, xi, yi, 8, 2)
            .reorder(c, xi, yi, xo, yo)
            .vectorize(xi).unroll(yi).unroll(c);
        //demosaic.tile(x, y, xo, yo, xi, yi, 2, 2)
        //    .reorder(c, xi, yi, xo, yo).unroll(c).unroll(xi).unroll(yi);
        lowpass_x.compute_at(output, xo)
            .vectorize(x, 8).unroll(c);



        //output.print_loop_nest();
        output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
    }

    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;

        padded.compute_root();
        hw_output.compute_root();
        hw_output.tile(x, y, xo, yo, xi, yi, 1440, 960);
        hw_output.reorder(c, xi, yi, xo, yo);

        hw_output.accelerate({padded}, xi, xo);
        demosaic.linebuffer().unroll(c);
        lowpass_x.linebuffer().unroll(c);
        hw_output.unroll(c);

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
        output.compile_to_lowered_stmt("pipeline_zynq.ir.html", args, HTML, target);
    }
};

int main(int argc, char **argv) {
    MyPipeline p1;
    p1.compile_cpu();

    MyPipeline p2;
    p2.compile_hls();

    return 0;
}
