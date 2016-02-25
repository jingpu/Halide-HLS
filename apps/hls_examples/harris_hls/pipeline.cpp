#include "Halide.h"
#include <stdio.h>

using namespace Halide;

Var x("x"), y("y"), c("c");
Var xo("xo"), yo("yo"), xi("xi"), yi("yi"), tile_index("ti");
Var xio("xio"), yio("yio"), xv("xv"), yp("yp");

int blockSize = 2;
int Ksize = 3;

class MyPipeline {

public:
    ImageParam input;
    Param<float> k;
    Param<int> threshold;
    std::vector<Argument> args;

    Func padded;
    Func grad_x, grad_y;
    Func grad_xx, grad_yy, grad_xy;
    Func grad_gx, grad_gy, grad_gxy;
    Func cim;
    Func output, hw_output;
    RDom box, maxWin;

  MyPipeline()
    : input(UInt(8), 2), padded("padded"),
      grad_x("grad_x"), grad_y("grad_y"),
      grad_xx("grad_xx"), grad_yy("grad_yy"), grad_xy("grad_xy"),
      grad_gx("grad_gx"), grad_gy("grad_gy"), grad_gxy("grad_gxy"),
      output("output"), hw_output("hw_output"),
      box(-blockSize/2, blockSize, -blockSize/2, blockSize),
      maxWin(-1, 3, -1, 3) {

    padded = BoundaryConditions::repeat_edge(input);

    // sobel filter
    Func padded16;
    padded16(x,y) = cast<int16_t>(padded(x,y));
    grad_x(x, y) = cast<int16_t>(-padded16(x-1,y-1) + padded16(x+1,y-1)
                                 -2*padded16(x-1,y) + 2*padded16(x+1,y)
                                 -padded16(x-1,y+1) + padded16(x+1,y+1));
    grad_y(x, y) = cast<int16_t>(padded16(x-1,y+1) - padded16(x-1,y-1) +
                                 2*padded16(x,y+1) - 2*padded16(x,y-1) +
                                 padded16(x+1,y+1) - padded16(x+1,y-1));

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
    cim(x,y) = det - k*trace*trace;

    // Perform non-maximal suppression
    Expr is_max = cim(x,y) > cim(x-1, y-1) && cim(x, y) > cim(x, y-1) &&
        cim(x, y) > cim(x+1, y-1) && cim(x-1, y) > cim(x, y) &&
        cim(x, y) > cim(x+1, y) && cim(x, y) > cim(x-1, y+1) &&
        cim(x, y) > cim(x, y+1) && cim(x, y) > cim(x+1, y+1);
    hw_output(x, y) = select( is_max && (cim(x, y) >= threshold), cast<uint8_t>(255), 0);

    output(x,y) = hw_output(x,y);

    // Arguments
    args = {input, k, threshold};
  }

  void compile_cpu() {
    std::cout << "\ncompiling cpu code..." << std::endl;

    output.tile(x, y, xo, yo, xi, yi, 256, 256);
    grad_x.compute_at(output, xo);
    grad_y.compute_at(output, xo);
    grad_xx.compute_at(output, xo);
    grad_yy.compute_at(output, xo);
    grad_xy.compute_at(output, xo);
    grad_gx.compute_at(output, xo);
    grad_gy.compute_at(output, xo);
    grad_gxy.compute_at(output, xo);
    cim.compute_at(output, xo);

    grad_gx.update(0).unroll(box.x).unroll(box.y);
    grad_gy.update(0).unroll(box.x).unroll(box.y);
    grad_gxy.update(0).unroll(box.x).unroll(box.y);

    //output.print_loop_nest();
    //output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
    output.compile_to_header("pipeline_native.h", args, "pipeline_native");
    output.compile_to_object("pipeline_native.o", args, "pipeline_native");
  }

  void compile_hls() {
    std::cout << "\ncompiling HLS code..." << std::endl;

    hw_output.compute_root();
    hw_output.tile(x, y, xo, yo, xi, yi, 2048, 2048);
    hw_output.accelerate({padded}, xi, xo);
    grad_x.linebuffer();
    grad_y.linebuffer();
    grad_xx.linebuffer();
    grad_yy.linebuffer();
    grad_xy.linebuffer();
    grad_gx.linebuffer();
    grad_gy.linebuffer();
    grad_gxy.linebuffer();
    cim.linebuffer();

    grad_gx.update(0).unroll(box.x).unroll(box.y);
    grad_gy.update(0).unroll(box.x).unroll(box.y);
    grad_gxy.update(0).unroll(box.x).unroll(box.y);

    output.print_loop_nest();
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
