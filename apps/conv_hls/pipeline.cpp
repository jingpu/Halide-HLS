#include "Halide.h"
#include <string.h>

using namespace Halide;

const unsigned char gaussian2d[5][5] = {
    {1,     3,     6,     3,     1},
    {3,    15,    25,    15,     3},
    {6,    25,    44,    25,     6},
    {3,    15,    25,    15,     3},
    {1,     3,     6,     3,     1}
};

int main(int argc, char **argv) {
    ImageParam input(UInt(8), 3, "input");
    Var x("x"), y("y"), c("c");

    Func clamped, buffered("buffered");
    clamped = BoundaryConditions::repeat_edge(input);
    buffered(x, y, c) = clamped(x, y, c);

    Func input_16("input_16");
    input_16(x, y, c) = cast<uint16_t>(buffered(x, y, c));

    Func conv1("conv1"), conv2("conv2");
    conv1(x, y, c) = (input_16(x-2, y-2, c) * gaussian2d[0][0]
                      + input_16(x-1, y-2, c) * gaussian2d[0][1]
                      + input_16(x  , y-2, c) * gaussian2d[0][2]
                      + input_16(x+1, y-2, c) * gaussian2d[0][3]
                      + input_16(x+2, y-2, c) * gaussian2d[0][4]
                      + input_16(x-2, y-1, c) * gaussian2d[1][0]
                      + input_16(x-1, y-1, c) * gaussian2d[1][1]
                      + input_16(x  , y-1, c) * gaussian2d[1][2]
                      + input_16(x+1, y-1, c) * gaussian2d[1][3]
                      + input_16(x+2, y-1, c) * gaussian2d[1][4]
                      + input_16(x-2, y  , c) * gaussian2d[2][0]
                      + input_16(x-1, y  , c) * gaussian2d[2][1]
                      + input_16(x  , y  , c) * gaussian2d[2][2]
                      + input_16(x+1, y  , c) * gaussian2d[2][3]
                      + input_16(x+2, y  , c) * gaussian2d[2][4]
                      + input_16(x-2, y+1, c) * gaussian2d[3][0]
                      + input_16(x-1, y+1, c) * gaussian2d[3][1]
                      + input_16(x  , y+1, c) * gaussian2d[3][2]
                      + input_16(x+1, y+1, c) * gaussian2d[3][3]
                      + input_16(x+2, y+1, c) * gaussian2d[3][4]
                      + input_16(x-2, y+2, c) * gaussian2d[4][0]
                      + input_16(x-1, y+2, c) * gaussian2d[4][1]
                      + input_16(x  , y+2, c) * gaussian2d[4][2]
                      + input_16(x+1, y+2, c) * gaussian2d[4][3]
                      + input_16(x+2, y+2, c) * gaussian2d[4][4]) >> 8;


    conv2(x, y, c) = (conv1(x-2, y-2, c) * gaussian2d[0][0]
                      + conv1(x-1, y-2, c) * gaussian2d[0][1]
                      + conv1(x  , y-2, c) * gaussian2d[0][2]
                      + conv1(x+1, y-2, c) * gaussian2d[0][3]
                      + conv1(x+2, y-2, c) * gaussian2d[0][4]
                      + conv1(x-2, y-1, c) * gaussian2d[1][0]
                      + conv1(x-1, y-1, c) * gaussian2d[1][1]
                      + conv1(x  , y-1, c) * gaussian2d[1][2]
                      + conv1(x+1, y-1, c) * gaussian2d[1][3]
                      + conv1(x+2, y-1, c) * gaussian2d[1][4]
                      + conv1(x-2, y  , c) * gaussian2d[2][0]
                      + conv1(x-1, y  , c) * gaussian2d[2][1]
                      + conv1(x  , y  , c) * gaussian2d[2][2]
                      + conv1(x+1, y  , c) * gaussian2d[2][3]
                      + conv1(x+2, y  , c) * gaussian2d[2][4]
                      + conv1(x-2, y+1, c) * gaussian2d[3][0]
                      + conv1(x-1, y+1, c) * gaussian2d[3][1]
                      + conv1(x  , y+1, c) * gaussian2d[3][2]
                      + conv1(x+1, y+1, c) * gaussian2d[3][3]
                      + conv1(x+2, y+1, c) * gaussian2d[3][4]
                      + conv1(x-2, y+2, c) * gaussian2d[4][0]
                      + conv1(x-1, y+2, c) * gaussian2d[4][1]
                      + conv1(x  , y+2, c) * gaussian2d[4][2]
                      + conv1(x+1, y+2, c) * gaussian2d[4][3]
                      + conv1(x+2, y+2, c) * gaussian2d[4][4]) >> 8;

    // Convert back to 8-bit RGB.
    Func output("output");
    output(x, y, c) = cast<uint8_t>(conv2(x*1, y*1, c)); // uint8

    std::vector<Argument> args;
    args.push_back(input);

    // generate hls version if the run argument is '1'
    bool gen_hls = argc == 2
        && strcmp(argv[1], "1") == 0;

    if (gen_hls) {
        // HLS Schedule
        Var xo("xo"), xi("xi"), yi("yi"), yo("yo");
        output.split(x, xo, xi, 64);
        output.split(y, yo, yi, 64);
        output.reorder(xi, yi, xo, yo, c);

        //conv2.store_at(output, xo).compute_at(output, xi);
        conv1.store_at(output, xo).compute_at(output, xi).stream();
        buffered.store_at(output, xo).compute_at(output, xi).stream();
        clamped.compute_root();

        //output.compile_to_lowered_stmt("pipeline_hls.ir", args);
        //output.compile_to_lowered_stmt("pipeline_c.ir.html", args, HTML);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls");
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls");

    } else {
        //output.compile_to_c("pipeline_c.c", args, "pipeline_c");
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
    }

    output.print_loop_nest();
    return 0;
}
