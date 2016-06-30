#include "Halide.h"
#include <stdio.h>

using namespace Halide;

Var x("x"), y("y"), c("c");
Var xo("xo"), yo("yo"), xi("xi"), yi("yi");

uint8_t phase = 3;

int windowR = 2;

Expr clampcast_u8(Expr in) {
    return cast<uint8_t>(clamp(in, 0, 255));
}

Expr clampcast_i8(Expr in) {
    return cast<int8_t>(clamp(in, -127, 127));
}

Expr clampcast_i16(Expr in) {
    return cast<int16_t>(clamp(in, -32767, 32767));
}

Func invert2x2 (Func matrix) {
    Func res("invert"), denorm, det;
    denorm(x, y) = matrix(x, y)[0] * matrix(x, y)[3] - matrix(x, y)[1] * matrix(x, y)[2];
    det(x, y) = select(denorm(x, y) != 0, 1/denorm(x, y), 0);
    res(x, y) = {det(x, y) * matrix(x, y)[3],
                 -det(x, y) * matrix(x, y)[1],
                 -det(x, y) * matrix(x, y)[2],
                 det(x, y) * matrix(x, y)[0]};
    return res;
}

Func invert2x2 (Func elem00, Func elem01, Func elem10, Func elem11) {
    Func res("invert"), denorm, det;
    denorm(x, y) = cast<float>(elem00(x, y) * elem11(x, y) - elem01(x, y) * elem01(x, y));
    det(x, y) = select(denorm(x, y) != 0, 1/denorm(x, y), 0);
    res(x, y) = {det(x, y) * elem11(x, y),
                 -det(x, y) * elem01(x, y),
                 -det(x, y) * elem10(x, y),
                 det(x, y) * elem00(x, y)};
    return res;
}

Func demosaic(Func raw) {
    Func rgb("rgb");

    Func raw16;
    Func neswNeighbors, diagNeighbors, vNeighbors, hNeighbors;
    Func green, red, blue;
    raw16(x, y) = cast<uint16_t>(raw(x, y));
    neswNeighbors(x, y) = cast<uint8_t>((raw16(x-1, y) + raw16(x+1, y) +
                                         raw16(x, y-1) + raw16(x, y+1))/4);
    diagNeighbors(x, y) = cast<uint8_t>((raw16(x-1, y-1) + raw16(x+1, y-1) +
                                         raw16(x-1, y+1) + raw16(x+1, y+1))/4);

    vNeighbors(x, y) = cast<uint8_t>((raw16(x, y-1) + raw16(x, y+1))/2);
    hNeighbors(x, y) = cast<uint8_t>((raw16(x-1, y) + raw16(x+1, y))/2);

    green(x, y) = select((y % 2) == (phase / 2),
                         select((x % 2) == (phase % 2), neswNeighbors(x, y), raw(x, y)), // First row, RG
                         select((x % 2) == (phase % 2), raw(x, y), neswNeighbors(x, y))); // Second row, GB

    red(x, y) = select((y % 2) == (phase / 2),
                       select((x % 2) == (phase % 2), raw(x, y), hNeighbors(x, y)), // First row, RG
                       select((x % 2) == (phase % 2), vNeighbors(x, y), diagNeighbors(x, y))); // Second row, GB

    blue(x, y) = select((y % 2) == (phase / 2),
                        select((x % 2) == (phase % 2), diagNeighbors(x, y), vNeighbors(x, y)), // First row, RG
                        select((x % 2) == (phase % 2), hNeighbors(x, y), raw(x, y))); // Second row, GB

    rgb(c, x, y) = cast<uint8_t>(select(c == 0, red(x, y),
                                        c == 1, green(x, y),
                                        blue(x, y)));

    return rgb;
}


Func downsample(Func input) {
    Func downsampled("downsampled");
    Func lowpass_y, lowpass_x;
    // lowpass filter before downsample
    lowpass_y(c, x, y) = cast<uint8_t>((cast<uint16_t>(input(c, x, y)) +
                                        cast<uint16_t>(input(c, x+1, y)))/2);
    lowpass_x(c, x, y) = cast<uint8_t>((cast<uint16_t>(lowpass_y(c, x, y)) +
                                        cast<uint16_t>(lowpass_y(c, x, y+1)))/2);

    // downsample
    downsampled(c, x, y) = lowpass_x(c, x*2, y*2);
    return downsampled;
}

Func gray(Func input) {
    Func func_gray("gray");
    func_gray(x, y) = cast<uint8_t>((77 * cast<uint16_t>(input(0, x, y))
                                     + 150 * cast<uint16_t>(input(1, x, y))
                                     + 29 * cast<uint16_t>(input(2, x, y))) >> 8);
    return func_gray;
}


class MyPipeline {
public:
    ImageParam input1, input2;
    Func padded1, rgb1, small_rgb1, gray1;
    Func padded2, rgb2, small_rgb2, gray2;
    Func Ix, Iy, It;
    Func A00, A01, A10, A11, Ainv;
    Func b0, b1;
    Func u, v;
    Func output, hw_output;
    RDom win;
    std::vector<Argument> args;


    MyPipeline()
        : input1(UInt(8), 2), input2(UInt(8), 2),
          padded1("padded1"), padded2("padded2"),
          Ix("Ix"), Iy("Iy"), It("It"),
          A00("A00"), A01("A01"), A10("A10"), A11("A11"), Ainv("Ainv"),
          u("u"), v("v"),
          output("output"), hw_output("hw_output"),
          win(-windowR, windowR*2 + 1, -windowR, windowR*2 + 1)
    {
        //padded = BoundaryConditions::constant_exterior(input, 0);
        padded1(x, y) = input1(x+201, y+41);
        padded2(x, y) = input2(x+201, y+41);

        rgb1 = demosaic(padded1);
        rgb2 = demosaic(padded2);

        small_rgb1 = downsample(rgb1);
        small_rgb2 = downsample(rgb2);

        gray1 = gray(small_rgb1);
        gray2 = gray(small_rgb2);

        Func frame1_f, frame2_f;
        frame1_f(x, y) = cast<int16_t>(gray1(x, y));
        frame2_f(x, y) = cast<int16_t>(gray2(x, y));

        // calculate Ix, Iy, It
        Ix(x, y) = cast<int16_t>((frame1_f(x+1, y) - frame1_f(x-1, y))); // -127:127
        Iy(x, y) = cast<int16_t>((frame1_f(x, y+1) - frame1_f(x, y-1))); // -127:127
        It(x, y) = cast<int16_t>((frame2_f(x, y) - frame1_f(x, y))); // -127:127

        // calculate A
        A00(x, y) += cast<int32_t>(Ix(x+win.x, y+win.y)) * cast<int32_t>(Ix(x+win.x, y+win.y));
        A01(x, y) += cast<int32_t>(Ix(x+win.x, y+win.y)) * cast<int32_t>(Iy(x+win.x, y+win.y));
        A10(x, y) += cast<int32_t>(Ix(x+win.x, y+win.y)) * cast<int32_t>(Iy(x+win.x, y+win.y));
        A11(x, y) += cast<int32_t>(Iy(x+win.x, y+win.y)) * cast<int32_t>(Iy(x+win.x, y+win.y));

        Ainv = invert2x2(A00, A01, A10, A11);

        // calculate b
        b0(x, y) += cast<int32_t>(Ix(x+win.x, y+win.y)) * cast<int32_t>(It(x+win.x, y+win.y));
        b1(x, y) += cast<int32_t>(Iy(x+win.x, y+win.y)) * cast<int32_t>(It(x+win.x, y+win.y));

        // optical flow field
        u(x, y) = Ainv(x, y)[0]*(-b0(x, y)) + Ainv(x, y)[1]*(-b1(x, y));
        v(x, y) = Ainv(x, y)[2]*(-b0(x, y)) + Ainv(x, y)[3]*(-b1(x, y));

        // overlap flow field on the grayscale version of input frame 2
        Expr red, green, blue;
        Expr u_render, v_render;
        int scale_factor = 2;
        u_render =  clampcast_u8(abs(cast<int32_t>(u(x, y))) * scale_factor);  // may overlow here
        v_render =  clampcast_u8(abs(cast<int32_t>(v(x, y))) * scale_factor);
        red = clampcast_u8(cast<uint16_t>(gray2(x, y)/2) + u_render);
        green = clampcast_u8(cast<uint16_t>(gray2(x, y)/2) + v_render);
        blue = clampcast_u8(cast<uint16_t>(gray2(x, y)/2) + u_render);
        hw_output(c, x, y) = select(c == 0, red,
                                    select(c == 1, green,
                                    blue));

        output(x, y, c) = hw_output(c, x, y);

        // common constraints
        // We can generate slightly better code if we know the output is a whole number of tiles.
        output.bound(x, 0, 720).bound(y, 0, 480).bound(c, 0, 3);

        // Arguments
        args = {input1, input2};
    }

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;

        output.tile(x, y, xo, yo, xi, yi, 120, 120);

        // ARM schedule use interleaved vector instructions
        gray1.compute_at(output, xo).vectorize(x, 8);
        gray2.compute_at(output, xo).vectorize(x, 8);

        Ix.compute_at(output, xo).vectorize(x, 4);
        Iy.compute_at(output, xo).vectorize(x, 4);
        It.compute_at(output, xo).vectorize(x, 4);

        A00.compute_at(output, xo);
        A01.compute_at(output, xo);
        A10.compute_at(output, xo);
        A11.compute_at(output, xo);

        u.compute_at(output, xo);
        v.compute_at(output, xo);
        output.reorder(c, xi, yi, xo, yo)
            .unroll(c);

        //output.print_loop_nest();
        output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        output.compile_to_object("pipeline_native.o", args, "pipeline_native");
    }

    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;

        output.tile(x, y, xo, yo, xi, yi, 720, 480)
            .reorder(c, xi, yi, xo, yo);
        padded1.compute_at(output, xo);
        padded2.compute_at(output, xo);
        hw_output.compute_at(output, xo);

        hw_output.tile(x, y, xo, yo, xi, yi, 720, 480);
        hw_output.accelerate({padded1, padded2}, xi, xo);

        gray1.linebuffer();
        gray2.linebuffer().fifo_depth(hw_output, 720*5);
        Ix.linebuffer();
        Iy.linebuffer();
        It.linebuffer();
        A00.linebuffer();
        A01.linebuffer();
        A10.linebuffer();
        A11.linebuffer();
        //Ainv.linebuffer();
        b0.linebuffer();
        b1.linebuffer();
        //u.linebuffer();
        //v.linebuffer();

        A00.update(0).unroll(win.x).unroll(win.y);
        A01.update(0).unroll(win.x).unroll(win.y);
        A10.update(0).unroll(win.x).unroll(win.y);
        A11.update(0).unroll(win.x).unroll(win.y);
        b0.update(0).unroll(win.x).unroll(win.y);
        b1.update(0).unroll(win.x).unroll(win.y);

        hw_output.unroll(c);

        //output.print_loop_nest();
        // Create the target for HLS simulation
        Target hls_target = get_target_from_environment();
        hls_target.set_feature(Target::CPlusPlusMangling);
        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML, hls_target);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls", hls_target);
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls", hls_target);

        std::vector<Target::Feature> features({Target::Zynq});
        Target target(Target::Linux, Target::ARM, 32, features);
        output.compile_to_zynq_c("pipeline_zynq.c", args, "pipeline_zynq", target);
        output.compile_to_header("pipeline_zynq.h", args, "pipeline_zynq", target);
        output.compile_to_lowered_stmt("pipeline_zynq.ir.html", args, HTML, target);
    }
};

int main(int argc, char **argv) {
    MyPipeline p1;
    p1.compile_cpu();

    MyPipeline p2;
    p2.compile_hls();

    return 0;
}
