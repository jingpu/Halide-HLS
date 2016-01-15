#include "Halide.h"
#include <stdio.h>

using namespace Halide;

Var x("x"), y("y"), c("c");
Var xo("xo"), yo("yo"), xi("xi"), yi("yi"), tile_index("ti");
Var xio("xio"), yio("yio"), xv("xv"), yp("yp");

bool use_threadhold = true;
uint8_t threadhold = 4;

class MyPipeline {

public:
  ImageParam input;
  std::vector<Argument> args;
  
  Func padded, paddedc;
  Func sobel3_x, sobel3_y;
  Func x2_grad, y2_grad, xy_grad;
  Func x2_grad_blur, y2_grad_blur, xy_grad_blur;
  Func det, trace, harris;
  Func harris_local, window_max, threshold_val, harris_supp;
  Func output, hw_output;

  MyPipeline()
    : input(UInt(8), 2), padded("padded"),
      output("output"), hw_output("hw_output")
  {
    padded = BoundaryConditions::repeat_edge(input);
    paddedc(x,y) = cast<int16_t>(padded(x,y));

    // Compute Harris values
    {
      // sobel filter original image
      sobel3_x(x, y) = (paddedc(x-1,y-1) - paddedc(x+1,y-1) +
			2*paddedc(x-1,y) - 2*paddedc(x+1,y) +
			paddedc(x-1,y+1) - paddedc(x+1,y+1));
      sobel3_y(x, y) = (paddedc(x-1,y+1) - paddedc(x-1,y-1) +
			2*paddedc(x,y+1) - 2*paddedc(x,y-1) +
			paddedc(x+1,y+1) - paddedc(x+1,y-1));
      // find squared values and scale by 1/100
      x2_grad(x, y) = (sobel3_x(x,y)/10 * sobel3_x(x,y)/10);
      y2_grad(x, y) = (sobel3_y(x,y)/10 * sobel3_y(x,y)/10);
      xy_grad(x, y) = (sobel3_x(x,y)/10 * sobel3_y(x,y)/10);

      // smooth with 3x3 Gaussian blur, sigma=0.75
      x2_grad_blur = gblur(x2_grad);
      y2_grad_blur = gblur(y2_grad);
      xy_grad_blur = gblur(xy_grad);

      // find harris corner values using k=0.1
      det(x,y) = ( x2_grad_blur(x,y) * y2_grad_blur(x,y) 
		   - xy_grad_blur(x,y) * xy_grad_blur(x,y) );
      trace(x,y)  = ( x2_grad_blur(x,y) + y2_grad_blur(x,y) );
      harris(x,y) = cast<int8_t>( (det(x,y) - trace(x,y)*trace(x,y) / 10 ) >> 8);

    }

    // Perform non-maximal suppression
    {
      // find windowed maxima for 7x7 windows
      RDom r7(-3, 7, -3, 7);
      window_max(x,y) = maximum(harris(x+r7.x, y+r7.y));
      harris_local(x,y) = select( harris(x,y) == window_max(x,y), harris(x,y),0);

      if (use_threadhold) {
          harris_supp(x, y) = select( harris_local(x, y) >= threadhold, cast<uint8_t>(255), 0);
      } else {
      // threshold at 1/8 of maximum value of entire image
      RDom full_range(0, input.width(), 0, input.height());
      threshold_val(x,y) = maximum(harris(x+full_range.x, y+full_range.y)) >> 3;
      harris_supp(x,y) = select( harris_local(x,y) >= threshold_val(0,0), cast<uint8_t>(255), 0 );
      }
    }

    // Output image of coners and orientation
    {
      hw_output(x,y) = harris_supp(x,y);
      output(x,y) = hw_output(x,y);
    }

    // schedule
    output.tile(x, y, xo, yo, xi, yi, 256, 256);
    padded.store_at(output, xo).compute_at(output, xi);
    harris.store_at(output, xo).compute_at(output, xi);


    /*
    harris.compute_root();
    threshold_val.compute_root();

    hw_output
      .tile(x,y,xo,yo,xi,yi,256,256)
      .fuse(xo, yo, tile_index)
      .parallel(tile_index);

    hw_output
      .tile(xi,yi,xio,yio,xv,yp,4,8)
      .vectorize(xv)
      .unroll(yp);
    */

    // Arguments
    args = {input};
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

    hw_output.store_at(output, xo).compute_at(output, xi);
    hw_output.accelerate_at(output, xo, {padded});

    //output.print_loop_nest();
    output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML);
    output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls");
    output.compile_to_header("pipeline_hls.h", args, "pipeline_hls");
  }

private:
  Func gblur(Func in) {
    Func local_sum, res;
    local_sum(x,y) = (in(x-1,y-1)*5  + in(x-1,y+1)*5  + 
		      in(x+1,y-1)*5  + in(x+1,y+1)*5  +
		      in(  x,y-1)*12 + in(x-1,  y)*12 +
		      in(x+1,  y)*12 + in(  x,y+1)*12 + 
		      in(  x,  y)*30);
    res(x,y) = cast<int32_t>(local_sum(x,y) >> 8 );
    
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
