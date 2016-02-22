#include "Halide.h"
#include <stdio.h>

using namespace Halide;

Var x("x"), y("y"), c("c");
Var xo("xo"), yo("yo"), xi("xi"), yi("yi");

class MyPipeline {
public:
    ImageParam input;
    std::vector<Argument> args;
    Param<uint8_t> phase; // One bit each for x and y phase
    Func padded;
    Func red, green, blue;
    Func output, hw_output;
    Func neswNeighbors, diagNeighbors, vNeighbors, hNeighbors;

    MyPipeline()
        : input(UInt(8), 2), padded("padded"),
          output("output"), hw_output("hw_output")
    {
        padded = BoundaryConditions::constant_exterior(input, 0);

        // Now demosaic and try to get RGB back
        neswNeighbors(x, y) = cast<uint8_t>(padded(x-1, y)/4 + padded(x+1, y)/4 +
                                            padded(x, y-1)/4 + padded(x, y+1)/4);
        diagNeighbors(x, y) = cast<uint8_t>(padded(x-1, y-1)/4 + padded(x+1, y-1)/4 +
                                            padded(x-1, y+1)/4 + padded(x+1, y+1)/4);

        vNeighbors(x, y) = cast<uint8_t>(padded(x, y-1)/2 + padded(x, y+1)/2);
        hNeighbors(x, y) = cast<uint8_t>(padded(x-1, y)/2 + padded(x+1, y)/2);

        green(x, y) = select((y % 2) == (phase / 2),
             select((x % 2) == (phase % 2), neswNeighbors(x, y), padded(x, y)), // First row, RG
             select((x % 2) == (phase % 2), padded(x, y), neswNeighbors(x, y))); // Second row, GB

        red(x, y) = select((y % 2) == (phase / 2),
             select((x % 2) == (phase % 2), padded(x, y), hNeighbors(x, y)), // First row, RG
             select((x % 2) == (phase % 2), vNeighbors(x, y), diagNeighbors(x, y))); // Second row, GB

        blue(x, y) = select((y % 2) == (phase / 2),
             select((x % 2) == (phase % 2), diagNeighbors(x, y), vNeighbors(x, y)), // First row, RG
             select((x % 2) == (phase % 2), hNeighbors(x, y), padded(x, y))); // Second row, GB

        hw_output(x, y, c) = cast<uint8_t>(select(c == 0, red(x, y),
                                                  c == 1, green(x, y),
                                                  blue(x, y)));
        output(x, y, c) = hw_output(x, y, c);

        // common constraints
        output.bound(c, 0, 3);
        // We can generate slightly better code if we know the output is a whole number of tiles.
        Expr out_width = output.output_buffer().width();
        Expr out_height = output.output_buffer().height();
        output.bound(x, 0, (out_width/256)*256)
            .bound(y, 0, (out_height/256)*256);

        // Arguments
        args = {input, phase};
    }

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;
        output.tile(x, y, xo, yo, xi, yi, 256, 256);
        output.reorder(c, xi, yi, xo, yo);

        //output.print_loop_nest();
        //output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
    }

    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;

        padded.compute_root();
        hw_output.compute_root();
        hw_output.tile(x, y, xo, yo, xi, yi, 256, 256);
        hw_output.reorder(c, xi, yi, xo, yo);

        std::vector<Func> hw_bounds = hw_output.accelerate({padded}, xi, xo);
        hw_bounds[0].unroll(c);

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

    return 0;
}
