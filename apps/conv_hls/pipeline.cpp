#include "Halide.h"

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

    Func conv("conv");
    conv(x, y, c) = (input_16(x-2, y-2, c) * gaussian2d[0][0]
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

    // Convert back to 8-bit RGB.
    Func output("output");
    output(x, y, c) = cast<uint8_t>(conv(x, y, c)); // uint8

    
    // Schedule
    Var x_outer("xo"), x_inner("xi");
    output.split(x, x_outer, x_inner, 64);
    output.reorder(x_inner, y, x_outer, c);

    buffered.store_at(output, x_outer).compute_at(output, y);
    clamped.compute_root();

    std::vector<Argument> args;
    args.push_back(input);

    output.print_loop_nest();

    output.compile_to_header("pipeline_native.h", args, "pipeline_native");
    output.compile_to_header("pipeline_c.h", args, "pipeline_c");
    output.compile_to_object("pipeline_native.o", args, "pipeline_native");
    //output.compile_to_assembly("pipeline_native.s", args, "pipeline_native");
    output.compile_to_c("pipeline_c.c", args, "pipeline_c");
    return 0;
}
