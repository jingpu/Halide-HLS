/* Blend two images with weighted addition
 * Takes two 24-bit RGB images in, and returns a single 24-bit RGB of the same size.
 * Steven Bell <sebell@stanford.edu>
 * Keyi Zhang  <keyi@stanford.edu>
 */

#include "Halide.h"

using namespace Halide;

class BlendPipe
{
public:
    ImageParam input1, input2;
    ImageParam weight; // Two values, stored at 0(,0,0,0) and 1

    Func padded1, padded2;
    Func remap1, remap2;
    Func sum, hw_sum;
    Func blended;
    Var x, y, c;
    Var xo, yo, xi, yi;
    std::vector<Argument> args;

    Func blend_img(Func in1, Func in2, ImageParam &weight) {
        /**
         * Keyi: using weighted sum allows us to have more fine-grained control
         * over weights given 0-255 range constraint
         */
        Func blend{"blend"};
        blend(c, x, y) = cast<uint8_t>(clamp((cast<int16_t>(in1(c, x, y)) * weight(0) +
                         cast<int16_t>(in2(c, x, y)) * weight(1)) / (weight(0) + weight(1)),
                         0, 255));
        return blend;
    }

    BlendPipe() : input1(UInt(8), 3), input2(UInt(8), 3), weight(UInt(8), 1)
    {
        padded1 = BoundaryConditions::constant_exterior(input1, 0);
        padded2 = BoundaryConditions::constant_exterior(input2, 0);
        
        remap1(c, x, y) = padded1(x, y, c);
        remap2(c, x, y) = padded2(x, y, c);

        blended = blend_img(remap1, remap2, weight);

        hw_sum(c, x, y) = blended(c, x, y);
        sum(x, y, c) = hw_sum(c, x, y);

        // Common schedule
        sum.tile(x, y, xo, yo, xi, yi, 256, 256);
        sum.reorder(c, xi, yi, xo, yo);

        // Unroll across color; bound this to 3 channels
        sum.bound(c, 0, 3);
        hw_sum.bound(c, 0, 3);
        // set bounds for weights
        weight.dim(0).set_bounds(0, 2);

        args = {input1, input2, weight};
    }

    void build_cpulib()
    {
        padded1.store_at(sum, xo).compute_at(sum, xi);
        padded2.store_at(sum, xo).compute_at(sum, xi);
        hw_sum.store_at(sum, xo).compute_at(sum, xi).unroll(c);

        std::cout << "building pipeline for CPU as a static library..." << std::endl;
        sum.compile_to_header("seedark_pipeline.h", args, "blend_cpu");
        sum.compile_to_object("seedark_pipeline.o", args, "blend_cpu");
    }

    void generate_hls()
    {
        std::cout << "generating HLS code..." << std::endl;
        hw_sum.tile(x, y, xo, yo, xi, yi, 256, 256).reorder(c, xi, yi, xo, yo);
        hw_sum.compute_at(sum, xo);
        remap1.compute_at(sum, xo);
        remap2.compute_at(sum, xo);
        blended.linebuffer();
        // Accelerate the whole thing on the FPGA
        hw_sum.accelerate({remap1, remap2}, xi, xo);

        Target hls_target = get_target_from_environment();
        hls_target.set_feature(Target::CPlusPlusMangling);
        sum.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML, hls_target);
        sum.compile_to_hls("pipeline_hls_seedark.cpp", args, "pipeline_hls", hls_target);
        sum.compile_to_header("pipeline_hls_seedark.h", args, "pipeline_hls", hls_target);

        std::vector<Target::Feature> features({Target::Zynq});
        Target target(Target::Linux, Target::ARM, 64, features);
        sum.compile_to_zynq_c("pipeline_zynq_seedark.c", args, "pipeline_zynq", target);
        sum.compile_to_header("pipeline_zynq_seedark.h", args, "pipeline_zynq", target);
    }
};

int main(int argc, char* argv[])
{
    BlendPipe pipe_cpu, pipe_hls;
    pipe_cpu.build_cpulib();
    pipe_hls.generate_hls();
    return 0;
}

