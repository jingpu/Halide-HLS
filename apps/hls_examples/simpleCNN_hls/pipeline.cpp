#include "Halide.h"
#include <string.h>

using namespace Halide;
using std::string;


Var x("x"), y("y"), c("c");
Var xo("xo"), xi("xi"), yi("yi"), yo("yo");

class MyPipeline {
public:
    ImageParam input;
    ImageParam weight;
    Param<uint8_t> bias;
    Func clamped;
    Func conv1;
    Func relu1;
    Func output;
    Func hw_output;
    std::vector<Argument> args;

    MyPipeline() : input(UInt(8), 3, "input"), weight(UInt(8), 4, "weight"),
                   conv1("conv1"), output("output"), hw_output("hw_output") {

        // define the algorithm
        clamped = BoundaryConditions::repeat_edge(input);
        conv1 = convolve(clamped);
        relu1 = relu(conv1);
        hw_output = maxPool(relu1);
        output(c, x, y) = hw_output(c, x, y);

        // define common schedule: tile output, and linebuffer the intermediate
        //output.reorder(xi, yi, xo, yo, c);

        // restrict arguments
        weight.set_bounds(0, 0, 64);
        weight.set_bounds(1, 0, 3);
        weight.set_bounds(2, 0, 3);
        weight.set_bounds(3, 0, 3);
        weight.set_stride(0, 1);
        weight.set_stride(1, 64);
        weight.set_stride(2, 192);
        weight.set_stride(3, 576);
        output.bound(c, 0, 64);

        args.push_back(input);
        args.push_back(weight);
        args.push_back(bias);
    }

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;

        output.tile(x, y, xo, yo, xi, yi, 64, 64);
        clamped.store_at(output, xo).compute_at(output, xi);

        output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
    }

    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;

        // HLS schedule: make a hw pipeline producing 'hw_output', taking
        // inputs of 'clamped', buffering intermediates at (output, xo) loop
        // level
        clamped.compute_root();
        hw_output.compute_root();
        hw_output.tile(x, y, xo, yo, xi, yi, 64, 64);
        hw_output.accelerate({clamped}, xi, xo);


        conv1.linebuffer();

        //output.print_loop_nest();
        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls");
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls");
    }

private:
    Func convolve(Func in) {
        int pad = 1;
        Func local_sum, res;
        RDom r(0, 3, 0, 3, 0, 3);

        local_sum(c, x, y) = cast<uint16_t>(bias);
        local_sum(c, x, y) += cast<uint16_t>(in(r.x, x+r.y-pad, y+r.z-pad)) * weight(c, r.x, r.y, r.z);
        res(c, x, y) = cast<uint8_t>(local_sum(c, x, y) >> 8);

        // unroll the reduction
        local_sum.update(0).unroll(r.x).unroll(r.y);
        return res;
    }

    Func relu(Func in) {
        Func res;
        res(c, x, y) = max(0, in(c, x, y));

        return res;
   }

   Func maxPool(Func in) {
        Func res;
        RDom r(0, 2, 0, 2);
        res(c, x, y) = maximum(in(c, x+r.x, y+r.y));

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
