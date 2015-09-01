#include "Halide.h"
#include <string.h>

using namespace Halide;
using std::string;

const unsigned char gaussian2d[5][5] = {
    {1,     3,     6,     3,     1},
    {3,    15,    25,    15,     3},
    {6,    25,    44,    25,     6},
    {3,    15,    25,    15,     3},
    {1,     3,     6,     3,     1}
};

Var x("x"), y("y"), c("c");
Var xo("xo"), xi("xi"), yi("yi"), yo("yo");

class MyPipeline {
public:
    ImageParam input;
    Func clamped;
    Func conv1;
    Func output;

    MyPipeline() : input(UInt(8), 3, "input"), conv1("conv1"), output("output") {
        // define the algorithm
        clamped = BoundaryConditions::repeat_edge(input);
        conv1 = convolve55(clamped);
        output = convolve55(conv1);

        // define common schedule: tile output, and linebuffer the intermediate
        output.tile(x, y, xo, yo, xi, yi, 256, 256);
        conv1.store_at(output, xo).compute_at(output, xi);
        clamped.compute_root();
    }

    void compile_cpu() {
        //output.print_loop_nest();
        std::cout << "compiling cpu code..." << std::endl;
        std::vector<Argument> args;
        args.push_back(input);

        output.compile_to_c("pipeline_native.c", args, "pipeline_native");
        output.compile_to_lowered_stmt("pipeline_native.ir", args);
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
    }

    void compile_hls() {
        std::cout << "compiling HLS code..." << std::endl;

        // Define the scope of the hardware pipeline
        // Here we offload the computation of 'output' to hardware, taking the
        // the input of 'clamped'. As a result, the pipeline from 'clamped' to
        // 'output' will be instantiated in hardware.
        output.accelerate_from(clamped);

        // the following schedules should be inferred automatically from accelerate_from()
        // 1. create output buffer
        Func output_stream(output.name() + "_stream");  // the output buffer must use this name
        output.insert_buffer(output_stream);

        // 2. create input buffers
        Func clamped_buf,  clamped_stream;
        clamped.insert_buffer(clamped_stream);
        clamped_stream.insert_buffer(clamped_buf);

        // 3. schedule the newly created buffers
        output_stream.store_at(output, xo).compute_at(output, xi);
        clamped.store_at(output, xo).compute_at(output, xi);
        clamped_stream.store_at(output, xo).compute_at(output, xi);
        clamped_buf.compute_root();

        // 4. mark all functions in the hls pipeline as streams
        output_stream.stream();
        conv1.stream();
        clamped.stream();
        clamped_stream.stream();

        //output.print_loop_nest();
        std::vector<Argument> args;
        args.push_back(input);

        output.compile_to_lowered_stmt("pipeline_hls.ir", args);
        //output.compile_to_lowered_stmt("pipeline_c.ir.html", args, HTML);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls");
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls");
    }

private:
    Func convolve55(Func in) {
        Func in_16, res;
        in_16(x, y, c) = cast<uint16_t>(in(x, y, c));
        res(x, y, c) = cast<uint8_t>((in_16(x-2, y-2, c) * gaussian2d[0][0]
                                      + in_16(x-1, y-2, c) * gaussian2d[0][1]
                                      + in_16(x  , y-2, c) * gaussian2d[0][2]
                                      + in_16(x+1, y-2, c) * gaussian2d[0][3]
                                      + in_16(x+2, y-2, c) * gaussian2d[0][4]
                                      + in_16(x-2, y-1, c) * gaussian2d[1][0]
                                      + in_16(x-1, y-1, c) * gaussian2d[1][1]
                                      + in_16(x  , y-1, c) * gaussian2d[1][2]
                                      + in_16(x+1, y-1, c) * gaussian2d[1][3]
                                      + in_16(x+2, y-1, c) * gaussian2d[1][4]
                                      + in_16(x-2, y  , c) * gaussian2d[2][0]
                                      + in_16(x-1, y  , c) * gaussian2d[2][1]
                                      + in_16(x  , y  , c) * gaussian2d[2][2]
                                      + in_16(x+1, y  , c) * gaussian2d[2][3]
                                      + in_16(x+2, y  , c) * gaussian2d[2][4]
                                      + in_16(x-2, y+1, c) * gaussian2d[3][0]
                                      + in_16(x-1, y+1, c) * gaussian2d[3][1]
                                      + in_16(x  , y+1, c) * gaussian2d[3][2]
                                      + in_16(x+1, y+1, c) * gaussian2d[3][3]
                                      + in_16(x+2, y+1, c) * gaussian2d[3][4]
                                      + in_16(x-2, y+2, c) * gaussian2d[4][0]
                                      + in_16(x-1, y+2, c) * gaussian2d[4][1]
                                      + in_16(x  , y+2, c) * gaussian2d[4][2]
                                      + in_16(x+1, y+2, c) * gaussian2d[4][3]
                                      + in_16(x+2, y+2, c) * gaussian2d[4][4]) >> 8);
        return res;
    }
};


int main(int argc, char **argv) {
    MyPipeline p1;
    p1.compile_cpu();

    MyPipeline p2;
    p2.compile_hls();

    return 0;
}
