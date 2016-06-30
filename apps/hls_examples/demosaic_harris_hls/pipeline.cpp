#include "Halide.h"
#include <stdio.h>

using namespace Halide;

Var x("x"), y("y"), c("c");
Var xo("xo"), yo("yo"), xi("xi"), yi("yi");

uint8_t phase = 0;

int blockSize = 3;
int Ksize = 3;

float k = 0.04;
float threshold = 100;

class MyPipeline {
public:
    ImageParam input;
    std::vector<Argument> args;
    //Param<uint8_t> phase; // One bit each for x and y phase
    Func padded;
    Func red, green, blue;
    Func demosaic, gray, lowpass_x, lowpass_y, downsample;

    Func grad_x, grad_y;
    Func grad_xx, grad_yy, grad_xy;
    Func grad_gx, grad_gy, grad_gxy;
    Func cim, corners, harris;

    Func output, hw_output;
    Func neswNeighbors, diagNeighbors, vNeighbors, hNeighbors;
    RDom box, maxWin;

    MyPipeline()
        : input(UInt(8), 2), padded("padded"),
          demosaic("demosaic"), gray("gray"), lowpass_x("lowpass_x"),
          lowpass_y("lowpass_y"), downsample("downsample"),
          output("output"), hw_output("hw_output"),
          box(-blockSize/2, blockSize, -blockSize/2, blockSize),
          maxWin(-1, 3, -1, 3)
    {
        //padded = BoundaryConditions::constant_exterior(input, 0);
        //padded(x, y) = input(x+240, y+60);
        padded(x, y) = input(x+201, y+41);

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

        // downsample
        downsample(c, x, y) = lowpass_x(c, x*2, y*2);

        // harris corner detector
        {
            gray(x, y) = cast<uint8_t>((77 * cast<uint16_t>(downsample(0, x, y))
                                        + 150 * cast<uint16_t>(downsample(1, x, y))
                                        + 29 * cast<uint16_t>(downsample(2, x, y))) >> 8);

            Func gray16;
            gray16(x,y) = cast<int16_t>(gray(x,y));
            grad_x(x, y) = cast<int16_t>(-gray16(x-1,y-1) + gray16(x+1,y-1)
                                         -2*gray16(x-1,y) + 2*gray16(x+1,y)
                                         -gray16(x-1,y+1) + gray16(x+1,y+1));
            grad_y(x, y) = cast<int16_t>(gray16(x-1,y+1) - gray16(x-1,y-1) +
                                         2*gray16(x,y+1) - 2*gray16(x,y-1) +
                                         gray16(x+1,y+1) - gray16(x+1,y-1));

            grad_xx(x, y) = cast<int32_t>(grad_x(x,y)) * cast<int32_t>(grad_x(x,y));
            grad_yy(x, y) = cast<int32_t>(grad_y(x,y)) * cast<int32_t>(grad_y(x,y));
            grad_xy(x, y) = cast<int32_t>(grad_x(x,y)) * cast<int32_t>(grad_y(x,y));

            // box filter (i.e. windowed sum)
            grad_gx(x, y) += grad_xx(x+box.x, y+box.y);
            grad_gy(x, y) += grad_yy(x+box.x, y+box.y);
            grad_gxy(x, y) += grad_xy(x+box.x, y+box.y);

            // calculate Cim
            int scale = (1 << (Ksize-1)) * blockSize;
            Expr lgx = cast<float>(grad_gx(x, y) / scale / scale);
            Expr lgy = cast<float>(grad_gy(x, y) / scale / scale);
            Expr lgxy = cast<float>(grad_gxy(x, y) / scale / scale);
            Expr det = lgx*lgy - lgxy*lgxy;
            Expr trace = lgx + lgy;
            cim(x, y) = det - k*trace*trace;

            // Perform non-maximal suppression
            Expr is_max = cim(x, y) > cim(x-1, y-1) && cim(x, y) > cim(x, y-1) &&
                cim(x, y) > cim(x+1, y-1) && cim(x, y) > cim(x-1, y) &&
                cim(x, y) > cim(x+1, y) && cim(x, y) > cim(x-1, y+1) &&
                cim(x, y) > cim(x, y+1) && cim(x, y) > cim(x+1, y+1);
            //harris(x, y) = select( is_max && (cim(x, y) >= threshold), cast<uint8_t>(255), 0);
            corners(x, y) = is_max && (cim(x, y) >= threshold);
            harris(c, x, y) = select( corners(x, y),
                                      select(c == 1, cast<uint8_t>(255), 0),  // green dots on corners
                                      downsample(c, x, y));
        }

        //output(x, y) = harris(x, y);
        hw_output(c, x, y) = harris(c, x, y);
        output(x, y, c) = hw_output(c, x, y);

        // common constraints
        // We can generate slightly better code if we know the output is a whole number of tiles.
        output.bound(x, 0, 720).bound(y, 0, 480).bound(c, 0, 3);

        // Arguments
        args = {input};
    }

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;

        output.tile(x, y, xo, yo, xi, yi, 120, 120);

        // ARM schedule use interleaved vector instructions
        /*
        demosaic.compute_at(output, xo)
            .tile(x, y, xo, yo, xi, yi, 8, 2)
            .reorder(c, xi, yi, xo, yo)
            .vectorize(xi).unroll(yi).unroll(c);
        gray.compute_at(output, xo)
            .tile(x, y, xo, yo, xi, yi, 8, 2)
            .vectorize(xi).unroll(yi);
        lowpass_x.compute_at(output, xo)
            .vectorize(x, 8);
        */
        downsample.compute_at(output, xo).vectorize(x, 8).unroll(c);

        grad_x.compute_at(output, xo).vectorize(x, 8);
        grad_y.compute_at(output, xo).vectorize(x, 8);
        grad_xx.compute_at(output, xo).vectorize(x, 4);
        grad_yy.compute_at(output, xo).vectorize(x, 4);
        grad_xy.compute_at(output, xo).vectorize(x, 4);
        grad_gx.compute_at(output, xo).vectorize(x, 4);
        grad_gy.compute_at(output, xo).vectorize(x, 4);
        grad_gxy.compute_at(output, xo).vectorize(x, 4);
        cim.compute_at(output, xo).vectorize(x, 4);

        grad_gx.update(0).unroll(box.x).unroll(box.y);
        grad_gy.update(0).unroll(box.x).unroll(box.y);
        grad_gxy.update(0).unroll(box.x).unroll(box.y);
        output.vectorize(xi, 4);

        //output.print_loop_nest();
        output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
    }

    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;

        padded.compute_root();
        hw_output.compute_root();

        /*
        hw_output.tile(x, y, xo, yo, xi, yi, 1440, 960);

        std::vector<Func> hw_bounds = hw_output.accelerate({padded}, xi, xo);
        gray.linebuffer();
        lowpass_x.linebuffer();
        hw_bounds[0];
        */

        hw_output.tile(x, y, xo, yo, xi, yi, 720, 480)
            .reorder(c, xi, yi, xo, yo);
        hw_output.accelerate({padded}, xi, xo);

        downsample.linebuffer().unroll(c)
            .fifo_depth(hw_output, 5400);
        grad_x.linebuffer().unroll(x);
        grad_y.linebuffer().unroll(x);
        grad_xx.linebuffer().unroll(x);
        grad_yy.linebuffer().unroll(x);
        grad_xy.linebuffer().unroll(x);
        grad_gx.linebuffer().unroll(x);
        grad_gx.update(0).unroll(x);
        grad_gy.linebuffer().unroll(x);
        grad_gy.update(0).unroll(x);
        grad_gxy.linebuffer().unroll(x);
        grad_gxy.update(0).unroll(x);
        cim.linebuffer().unroll(x);
        corners.linebuffer().unroll(x);
        hw_output.unroll(xi).unroll(c);

        grad_gx.update(0).unroll(box.x).unroll(box.y);
        grad_gy.update(0).unroll(box.x).unroll(box.y);
        grad_gxy.update(0).unroll(box.x).unroll(box.y);

        //output.print_loop_nest();
        // Create the target for HLS simulation
        Target hls_target = get_target_from_environment();
        hls_target.set_feature(Target::CPlusPlusMangling);
        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML, hls_target);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls", hls_target);
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls", hls_target);

        std::vector<Target::Feature> features({Target::Zynq});
        Target target(Target::Linux, Target::ARM, 32, features);
        output.compile_to_lowered_stmt("pipeline_zynq.ir.html", args, HTML, target);
        output.compile_to_zynq_c("pipeline_zynq.c", args, "pipeline_zynq", target);
        output.compile_to_header("pipeline_zynq.h", args, "pipeline_zynq", target);
        output.compile_to_object("pipeline_zynq.o", args, "pipeline_zynq", target);
    }
};

int main(int argc, char **argv) {
    MyPipeline p1;
    p1.compile_cpu();

    MyPipeline p2;
    p2.compile_hls();

    return 0;
}
