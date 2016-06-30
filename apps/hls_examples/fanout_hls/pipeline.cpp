#include "Halide.h"
#include <string.h>

using namespace Halide;
using std::string;

Var x("x"), y("y"), c("c");
Var xo("xo"), xi("xi"), yi("yi"), yo("yo");

class MyPipeline {
public:
    ImageParam input;
    Func A;
    Func B;
    Func C;
    Func hw_output;
    Func output;
    std::vector<Argument> args;

    MyPipeline() : input(UInt(8), 1, "input"),
                   A("A"), B("B"), C("C"), hw_output("hw_output")
    {
        // define the algorithm
        A = BoundaryConditions::repeat_edge(input);
        B(x) = A(x-1) + A(x);
        C(x) = A(x+1);
        hw_output(x) = A(x) + A(x+1) + B(x) + B(x+1) + C(x);
        output(x) = hw_output(x);

        // define common schedule: tile output

        args.push_back(input);
    }

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;
        //output.print_loop_nest();

        //output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
    }

    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;

        // HLS schedule: make a hw pipeline producing 'hw_output', taking
        // inputs of 'clamped', buffering intermediates at (output, xo) loop
        // level
        A.compute_root();
        hw_output.compute_root();
        hw_output.split(x, xo, xi, 64);
        hw_output.accelerate({A}, xi, xo);

        A.fifo_depth(hw_output, 4).fifo_depth(C, 2);
        B.linebuffer();
        C.linebuffer();

        //output.print_loop_nest();
        // Create the target for HLS simulation
        Target hls_target = get_target_from_environment();
        hls_target.set_feature(Target::CPlusPlusMangling);
        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML, hls_target);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls", hls_target);
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls", hls_target);
    }
};


int main(int argc, char **argv) {
    MyPipeline p1;
    p1.compile_cpu();

    MyPipeline p2;
    p2.compile_hls();

    return 0;
}
