#include "Halide.h"
#include <stdio.h>

using namespace Halide;

Var x("x"), y("y"), z("z"), c("c");
Var x_grid("x_grid"), y_grid("y_grid"), xo("xo"), yo("yo"), x_in("x_in"), y_in("y_in");

int windowR = 4;
int searchR = 64;
//int windowR = 8;
//int searchR = 120;

Func rectify_float(Func img, Func remap) {
    Expr offsetX = cast<float>(cast<int16_t>(remap(x, y, 0)) - cast<int16_t>(128)) / 16.0f;
    Expr offsetY = cast<float>(cast<int16_t>(remap(x, y, 1)) - cast<int16_t>(128)) / 16.0f;

    Expr targetX = cast<int>(floor(offsetX));
    Expr targetY = cast<int>(floor(offsetY));

    Expr wx = offsetX - targetX;
    Expr wy = offsetY - targetY;

    Func interpolated("interpolated");
    interpolated(x, y) = lerp(lerp(img(x+targetX, y+targetY, 1), img(x+targetX+1, y+targetY, 1), wx),
                              lerp(img(x+targetX, y+targetY+1, 1), img(x+targetX+1, y+targetY+1, 1), wx), wy);

    return interpolated;
}

Func rectify_int(Func img, Func remap) {
    Expr targetX = (cast<int16_t>(remap(x, y, 0)) - 128) / 16;
    Expr targetY = (cast<int16_t>(remap(x, y, 1)) - 128) / 16;

    // wx and wy are equal to wx*256 and wy*256 in rectify_float()
    Expr wx = cast<uint8_t>((cast<int16_t>(remap(x, y, 0)) * 16) - (128 * 16) - (targetX * 256));
    Expr wy = cast<uint8_t>((cast<int16_t>(remap(x, y, 1)) * 16) - (128 * 16) - (targetY * 256));

    Func interpolated("interpolated");
    interpolated(x, y) = lerp(lerp(img(x+targetX, y+targetY, 1), img(x+targetX+1, y+targetY, 1), wx),
                              lerp(img(x+targetX, y+targetY+1, 1), img(x+targetX+1, y+targetY+1, 1), wx), wy);

    return interpolated;
}

Func rectify_noop(Func img, Func remap) {
    Func pass("pass");
    pass(x, y) = img(x, y, 1);
    return pass;
}

class MyPipeline {
public:
    ImageParam left, right, left_remap, right_remap;
    Func left_padded, right_padded, left_remap_padded, right_remap_padded;
    Func left_remapped, right_remapped;
    Func SAD, offset, output, hw_output;
    RDom win, search;
    std::vector<Argument> args;

    MyPipeline()
        : left(UInt(8), 3), right(UInt(8), 3),
          left_remap(UInt(8), 3), right_remap(UInt(8), 3),
          SAD("SAD"), offset("offset"), output("output"), hw_output("hw_output"),
          win(-windowR, windowR*2, -windowR, windowR*2),
          search(0, searchR)
    {
        right_padded = BoundaryConditions::constant_exterior(right, 0);
        left_padded = BoundaryConditions::constant_exterior(left, 0);
        right_remap_padded = BoundaryConditions::constant_exterior(right_remap, 128);
        left_remap_padded = BoundaryConditions::constant_exterior(left_remap, 128);

        right_remapped = rectify_noop(right_padded, right_remap_padded);
        left_remapped = rectify_noop(left_padded, left_remap_padded);

        SAD(x, y, c) += cast<uint16_t>(absd(right_remapped(x+win.x, y+win.y),
                                            left_remapped(x+win.x+20+c, y+win.y)));

        //offset(x, y) = argmin(SAD(x, y, search.x));
        // avoid using the form of the inlined reduction function of "argmin",
        // so that we can get a handle for scheduling
        offset(x, y) = {cast<int8_t>(0), cast<uint16_t>(65535)};
        offset(x, y) = {select(SAD(x, y, search.x) < offset(x, y)[1],
                               cast<int8_t>(search.x),
                               offset(x, y)[0]),
                        min(SAD(x, y, search.x), offset(x, y)[1])};

        hw_output(x, y) = cast<uint8_t>(cast<uint16_t>(offset(x, y)[0]) * 255 / searchR);
        output(x, y) = hw_output(x, y);

        // all inputs has three channels
        right.set_bounds(2, 0, 3);
        left.set_bounds(2, 0, 3);
        right_remap.set_bounds(2, 0, 3);
        left_remap.set_bounds(2, 0, 3);

        // Arguments
        args = {right, left, right_remap, left_remap};
    }

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;

        output.tile(x, y, xo, yo, x_in, y_in, 256, 64);
        output.fuse(xo, yo, xo).parallel(xo);
        //output.split(y, yo, y_in, 64).parallel(yo);

        right_padded.compute_at(output, xo).vectorize(_0, 16);
        left_padded.compute_at(output, xo).vectorize(_0, 16);
        right_remap_padded.compute_at(output, xo).vectorize(_0, 16);
        left_remap_padded.compute_at(output, xo).vectorize(_0, 16);

        //right_remapped.compute_at(output, yo);
        //left_remapped.compute_at(output, yo);

        SAD.compute_at(output, x_in);
        SAD.unroll(c);
        SAD.update(0).vectorize(c, 8).unroll(win.x).unroll(win.y);

        //output.print_loop_nest();

        output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
    }

    void compile_gpu() {
        std::cout << "\ncompiling gpu code..." << std::endl;

        right_padded.compute_root().gpu_tile(_0, _1, _2, 8, 8, 1);
        right_remap_padded.compute_root().gpu_tile(_0, _1, _2, 8, 8, 1);
        left_padded.compute_root().gpu_tile(_0, _1, _2, 8, 8, 1);
        left_remap_padded.compute_root().gpu_tile(_0, _1, _2, 8, 8, 1);

        right_remapped.compute_root().gpu_tile(x, y, 8, 8);
        left_remapped.compute_root().gpu_tile(x, y, 8, 8);


        output.gpu_tile(x, y, 16, 1);
        SAD.compute_at(output, Var::gpu_blocks()).gpu_threads(c);
        SAD.update(0).gpu_threads(c).unroll(win.x);//.unroll(win.y);

        //output.print_loop_nest();

        Target target = get_target_from_environment();
        target.set_feature(Target::CUDA);
        output.compile_to_lowered_stmt("pipeline_cuda.ir.html", args, HTML, target);
        output.compile_to_header("pipeline_cuda.h", args, "pipeline_cuda", target);
        output.compile_to_object("pipeline_cuda.o", args, "pipeline_cuda", target);
    }

    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;

        right_padded.compute_root();
        left_padded.compute_root();
        right_remap_padded.compute_root();
        left_remap_padded.compute_root();

        hw_output.compute_root();
        hw_output.tile(x, y, xo, yo, x_in, y_in, 256, 256);
        hw_output.accelerate({right_remapped, left_remapped}, x_in, xo);
        right_remapped.compute_root();
        left_remapped.compute_root();

        RVar so("so"), si("si");
        //offset.update(0).unroll(search.x, 16); // the unrolling doesn's generate code that balances the computation, creating a long critical path
        //offset.update(0).split(search.x, so, si, 16);
        //SAD.compute_at(offset, so);
        SAD.unroll(c);
        SAD.update(0).unroll(win.x).unroll(win.y).unroll(c);

        //output.print_loop_nest();

        // Create the target for HLS simulation
        Target hls_target = get_target_from_environment();
        hls_target.set_feature(Target::CPlusPlusMangling);
        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML, hls_target);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls", hls_target);
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls", hls_target);
    }
};

// Optimize for hls code generation
// We change the algorithm to generated reduction tree for unrolling argmin
class MyPipelineOpt {
public:
ImageParam left, right, left_remap, right_remap;
Func left_padded, right_padded, left_remap_padded, right_remap_padded;
Func left_remapped, right_remapped;
Func SAD, offset, offset_l1, output, hw_output;
RDom win, search_l1, search;
std::vector<Argument> args;

MyPipelineOpt()
    : left(UInt(8), 3), right(UInt(8), 3),
      left_remap(UInt(8), 3), right_remap(UInt(8), 3),
      SAD("SAD"), offset("offset"), output("output"), hw_output("hw_output"),
      win(-windowR, windowR*2, -windowR, windowR*2),
      search_l1(0, 4), search(0, searchR/4)
{
    right_padded = BoundaryConditions::constant_exterior(right, 0);
    left_padded = BoundaryConditions::constant_exterior(left, 0);
    right_remap_padded = BoundaryConditions::constant_exterior(right_remap, 128);
    left_remap_padded = BoundaryConditions::constant_exterior(left_remap, 128);

    right_remapped = rectify_noop(right_padded, right_remap_padded);
    left_remapped = rectify_noop(left_padded, left_remap_padded);

    SAD(x, y, c) += cast<uint16_t>(absd(right_remapped(x+win.x, y+win.y),
                                        left_remapped(x+win.x+20+c, y+win.y)));

    /*
    offset(x, y) = argmin(SAD(x, y, search.x));
    */
    // offset_l1 caculates {minarg, minval} betwen SAD(x, y, c*4) and SAD(x, y, c*4+3)
    offset_l1(x, y, c) = {cast<int8_t>(0), cast<uint16_t>(65535)};
    offset_l1(x, y, c) = {select(SAD(x, y, c*4 + search_l1.x) < offset_l1(x, y, c)[1],
                                 cast<int8_t>(c*4 + search_l1.x),
                                      offset_l1(x, y, c)[0]),
                                 min(SAD(x, y, c*4 + search_l1.x), offset_l1(x, y, c)[1])};

    offset(x, y) = {cast<int8_t>(0), cast<uint16_t>(65535)};
    offset(x, y) = {select(offset_l1(x, y, search.x)[1] < offset(x, y)[1],
                           offset_l1(x, y, search.x)[0],
                           offset(x, y)[0]),
                           min(offset_l1(x, y, search.x)[1], offset(x, y)[1])};

    hw_output(x, y) = cast<uint8_t>(cast<uint16_t>(offset(x, y)[0]) * 255 / searchR);
    output(x, y) = hw_output(x, y);

    // The comment constraints and schedules.
    // all inputs has three channels
    right.set_bounds(2, 0, 3);
    left.set_bounds(2, 0, 3);
    right_remap.set_bounds(2, 0, 3);
    left_remap.set_bounds(2, 0, 3);

    /*
    Expr out_width = output.output_buffer().width();
    Expr out_height = output.output_buffer().height();
    output
        .bound(x, 0, (out_width/600)*600)
        .bound(y, 0, (out_height/400)*400);
    */

    // Arguments
    args = {right, left, right_remap, left_remap};
}


void compile_cpu() {
    std::cout << "\ncompiling cpu code..." << std::endl;
    //output.print_loop_nest();

    right_remapped.compute_root();
    left_remapped.compute_root();

    right_padded.compute_root();
    left_padded.compute_root();
    right_remap_padded.compute_root();
    left_remap_padded.compute_root();

    //output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
    output.compile_to_header("pipeline_native.h", args, "pipeline_native");
    output.compile_to_object("pipeline_native.o", args, "pipeline_native");
}

void compile_hls() {
    std::cout << "\ncompiling HLS code..." << std::endl;

    output.tile(x, y, xo, yo, x_in, y_in, 64, 64);
    right_remapped.compute_at(output, xo);
    left_remapped.compute_at(output, xo);
    //right_padded.compute_at(hw_output, xo);
    //left_padded.compute_at(hw_output, xo);
    //right_remap_padded.compute_at(hw_output, xo);
    //left_remap_padded.compute_at(hw_output, xo);

    hw_output.compute_at(output, xo);
    hw_output.tile(x, y, xo, yo, x_in, y_in, 64, 64);

    // TODO(jingpu) use rfactor() schedule instead.
    //offset.update(0).unroll(search.x, 4);
    RVar search_xo, search_xi;
    offset.update(0).split(search.x, search_xo, search_xi, 4);
    offset.update(0).unroll(search_xi);
    SAD.compute_at(offset_l1, Var::outermost());
    SAD.unroll(c);
    SAD.update(0).unroll(win.x).unroll(win.y).unroll(c);
    offset_l1.compute_at(offset, search_xo);
    offset_l1.unroll(c);
    offset_l1.update(0).unroll(c).unroll(search_l1.x);

    hw_output.accelerate({right_remapped, left_remapped}, x_in, xo);
    //hw_output.accelerate({right_padded, left_padded, right_remap_padded, left_remap_padded}, x_in, xo);
    //right_remapped.linebuffer();
    //left_remapped.linebuffer();
    //output.print_loop_nest();
    // Create the target for HLS simulation
    Target hls_target = get_target_from_environment();
    hls_target.set_feature(Target::CPlusPlusMangling);
    output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML, hls_target);
    output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls", hls_target);
    output.compile_to_header("pipeline_hls.h", args, "pipeline_hls", hls_target);

    // Create the Zynq platform target
    std::vector<Target::Feature> features({Target::Zynq});
    // use 64 bit since Zynq-ultrascale is 64bit
    Target target(Target::Linux, Target::ARM, 64, features);
    output.compile_to_zynq_c("pipeline_zynq.c", args, "pipeline_zynq", target);
    output.compile_to_header("pipeline_zynq.h", args, "pipeline_zynq", target);

    // Vectorization and Parallelization Schedules (only work with LLVM codegen)
    //right_remapped.vectorize(x, 4);  // vectorizing input works poorly on ZYNQ
    //left_remapped.vectorize(x, 4);
    output.vectorize(x_in, 16);
    output.fuse(xo, yo, xo).parallel(xo);

    //output.print_loop_nest();
    output.compile_to_object("pipeline_zynq.o", args, "pipeline_zynq", target);
    output.compile_to_assembly("pipeline_zynq.s", args, "pipeline_zynq", target);
    output.compile_to_lowered_stmt("pipeline_zynq.ir.html", args, HTML, target);
}
};


int main(int argc, char **argv) {
    MyPipeline p1, p3;
    MyPipelineOpt p2;
    p1.compile_cpu();
    p2.compile_hls();
    p3.compile_gpu();
    return 0;
}
