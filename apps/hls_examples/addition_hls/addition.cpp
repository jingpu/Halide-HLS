/* Blend two images with weighted addition
 * Takes two 24-bit RGB images in, and returns a single 24-bit RGB of the same size.
 * Steven Bell <sebell@stanford.edu>
 */

#include "Halide.h"
#include "halide_image_io.h"

using namespace Halide;

class AdditionPipe
{
public:
    ImageParam input1, input2;
    Func padded1, padded2;
    Func sum, hw_sum;
    Var x, y, c;
    Var xo, yo, xi, yi;
    std::vector<Argument> args;

    AdditionPipe() : input1(UInt(8), 3), input2(UInt(8), 3)
    {

        padded1 = BoundaryConditions::constant_exterior(input1, 0);
        padded2 = BoundaryConditions::constant_exterior(input2, 0);

        hw_sum(x,y,c) = Halide::cast<uint8_t>(padded1(x,y,c) / 2 + padded2(x,y,c) / 2);
        sum(x,y,c) = hw_sum(x,y,c);

        //Halide::Image<uint8_t> result = sum.realize(input1.width(), input1.height(), 3);
        //save_image(result, "sum.png");

        // Common schedule
        sum.tile(x, y, xo, yo, xi, yi, 256, 256);
        sum.reorder(c, xi, yi, xo, yo);

        // Unroll across color; bound this to 3 channels
        sum.bound(c, 0, 3);

        args = {input1, input2};
    }

    void compile_cpu()
    {
        std::cout << "\ncompiling cpu code..." << std::endl;
        sum.compile_to_header("pipeline_native.h", args, "pipeline_native");
        sum.compile_to_object("pipeline_native.o", args, "pipeline_native");
    }

    void compile_hls()
    {
        std::cout << "\ncompiling HLS code..." << std::endl;

        padded1.compute_root();
        padded2.compute_root();
        hw_sum.store_at(sum, xo).compute_at(sum, xi).unroll(c);
        hw_sum.accelerate({padded1, padded2}, sum, xi, xo);

        //sum.print_loop_nest();
        sum.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML);
        sum.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls");
        sum.compile_to_header("pipeline_hls.h", args, "pipeline_hls");
    }
};

int main(int argc, char* argv[])
{
    AdditionPipe p1;
    p1.compile_cpu();

    AdditionPipe p2;
    p2.compile_hls();

    return 0;
}

