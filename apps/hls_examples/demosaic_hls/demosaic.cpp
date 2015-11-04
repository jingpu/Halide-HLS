/* Simple mosaic
 * Takes an 8-bit, 1-channel image in, and returns an 24-bit RGB output.
 * Steven Bell <sebell@stanford.edu>
 */

#include "Halide.h"

#include "halide_image_io.h"
//using namespace Halide;
using namespace Halide::Tools;

int main(int argc, char* argv[])
{
  Halide::Func mosaic;
  Halide::Image<uint8_t> input = load_image("../tutorial/images/rgb.png");
  Halide::Var x, y;

  // Create an RG/GB mosaic
  mosaic(x, y) = Halide::cast<uint8_t>(Halide::select((y % 2) == 0,
    Halide::select((x % 2) == 0, input(x, y, 0), input(x, y, 1)), // First row, RG
    Halide::select((x % 2) == 0, input(x, y, 1), input(x, y, 2)))); // GB

  Halide::Image<uint8_t> output = mosaic.realize(input.width(), input.height());
  save_image(output, "mosaic.png");

  Halide::Func padded = Halide::BoundaryConditions::constant_exterior(output, 0);

  // Now demosaic and try to get RGB back
  Halide::Func demosaic;
  Halide::Func red, green, blue;

  Halide::Func neswNeighbors, diagNeighbors, vNeighbors, hNeighbors;
  neswNeighbors(x, y) = Halide::cast<uint8_t>(0.25f * padded(x-1, y) + 0.25f * padded(x+1, y) + 0.25f * padded(x, y-1) + 0.25f * padded(x, y+1));
  diagNeighbors(x, y) = Halide::cast<uint8_t>(0.25f * padded(x-1, y-1) + 0.25f * padded(x+1, y-1) + 0.25f * padded(x-1, y+1) + 0.25f * padded(x+1, y+1));

  vNeighbors(x, y) = Halide::cast<uint8_t>(0.5f * padded(x, y-1) + 0.5f * padded(x, y+1));
  hNeighbors(x, y) = Halide::cast<uint8_t>(0.5f * padded(x-1, y) + 0.5f * padded(x+1, y));

  green(x, y) = Halide::select((y % 2) == 0,
    Halide::select((x % 2) == 0, neswNeighbors(x, y), padded(x, y)), // First row, RG
    Halide::select((x % 2) == 0, padded(x, y), neswNeighbors(x, y))); // Second row, GB

  red(x, y) = Halide::select((y % 2) == 0,
    Halide::select((x % 2) == 0, padded(x, y), hNeighbors(x, y)), // First row, RG
    Halide::select((x % 2) == 0, vNeighbors(x, y), diagNeighbors(x, y))); // Second row, GB

  blue(x, y) = Halide::select((y % 2) == 0,
    Halide::select((x % 2) == 0, diagNeighbors(x, y), vNeighbors(x, y)), // First row, RG
    Halide::select((x % 2) == 0, hNeighbors(x, y), padded(x, y))); // Second row, GB

  Halide::Var c;
  demosaic(x, y, c) = Halide::cast<uint8_t>(Halide::select(
    c == 0, red(x, y),
    c == 1, green(x, y),
    blue(x, y)));

  Halide::Image<uint8_t> rgb = demosaic.realize(input.width(), input.height(), 3);
  save_image(rgb, "demosaicked.png");

  return(0);
}

