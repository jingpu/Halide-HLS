#include "Halide.h"
#include <stdint.h>

using namespace Halide;

Var x("x"), y("y"), tx("tx"), ty("ty"), c("c"), xi("xi"), yi("yi"), z("z");
Var xo("xo"), yo("yo");
Var x_grid("x_grid"), y_grid("y_grid"), x_in("x_in"), y_in("y_in");

// Average two positive values rounding up
Expr avg(Expr a, Expr b) {
    Type wider = a.type().with_bits(a.type().bits() * 2);
    return cast(a.type(), (cast(wider, a) + b + 1)/2);
}

Func hot_pixel_suppression(Func input) {
    Expr a = max(max(input(x-2, y), input(x+2, y)),
                 max(input(x, y-2), input(x, y+2)));
    Expr b = min(min(input(x-2, y), input(x+2, y)),
                 min(input(x, y-2), input(x, y+2)));

    Func denoised("denoised");
    denoised(x, y) = clamp(input(x, y), b, a);

    return denoised;
}

class MyPipeline {
    int schedule;
    ImageParam input;

    std::vector<Argument> args;

    Func shifted;
    Func processed;
    Func denoised;
    Func deinterleaved;
    Func demosaiced;
    Func corrected;
    Func hw_output;
    Func curve;
    Func rgb;

    Func interleave_x(Func a, Func b) {
        Func out;
        out(x, y) = select((x%2)==0, a(x/2, y), b(x/2, y));
        return out;
    }

    Func interleave_y(Func a, Func b) {
        Func out;
        out(x, y) = select((y%2)==0, a(x, y/2), b(x, y/2));
        return out;
    }

    Func deinterleave(Func raw) {
        // Deinterleave the color channels
        Func deinterleaved("deinterleaved");

        deinterleaved(x, y, c) = select(c == 0, raw(2*x, 2*y),
                                        select(c == 1, raw(2*x+1, 2*y),
                                               select(c == 2, raw(2*x, 2*y+1),
                                                      raw(2*x+1, 2*y+1))));
        return deinterleaved;
    }

    Func demosaic(Func deinterleaved) {
        // These are the values we already know from the input
        // x_y = the value of channel x at a site in the input of channel y
        // gb refers to green sites in the blue rows
        // gr refers to green sites in the red rows

        // Give more convenient names to the four channels we know
        Func r_r, g_gr, g_gb, b_b;
        g_gr(x, y) = deinterleaved(x, y, 0);
        r_r(x, y)  = deinterleaved(x, y, 1);
        b_b(x, y)  = deinterleaved(x, y, 2);
        g_gb(x, y) = deinterleaved(x, y, 3);


        // These are the ones we need to interpolate
        Func b_r, g_r, b_gr, r_gr, b_gb, r_gb, r_b, g_b;

        // First calculate green at the red and blue sites

        // Try interpolating vertically and horizontally. Also compute
        // differences vertically and horizontally. Use interpolation in
        // whichever direction had the smallest difference.
        Expr gv_r  = avg(g_gb(x, y-1), g_gb(x, y));
        Expr gvd_r = absd(g_gb(x, y-1), g_gb(x, y));
        Expr gh_r  = avg(g_gr(x+1, y), g_gr(x, y));
        Expr ghd_r = absd(g_gr(x+1, y), g_gr(x, y));

        g_r(x, y)  = select(ghd_r < gvd_r, gh_r, gv_r);

        Expr gv_b  = avg(g_gr(x, y+1), g_gr(x, y));
        Expr gvd_b = absd(g_gr(x, y+1), g_gr(x, y));
        Expr gh_b  = avg(g_gb(x-1, y), g_gb(x, y));
        Expr ghd_b = absd(g_gb(x-1, y), g_gb(x, y));

        g_b(x, y)  = select(ghd_b < gvd_b, gh_b, gv_b);

        // Next interpolate red at gr by first interpolating, then
        // correcting using the error green would have had if we had
        // interpolated it in the same way (i.e. add the second derivative
        // of the green channel at the same place).
        Expr correction;
        correction = g_gr(x, y) - avg(g_r(x, y), g_r(x-1, y));
        r_gr(x, y) = correction + avg(r_r(x-1, y), r_r(x, y));

        // Do the same for other reds and blues at green sites
        correction = g_gr(x, y) - avg(g_b(x, y), g_b(x, y-1));
        b_gr(x, y) = correction + avg(b_b(x, y), b_b(x, y-1));

        correction = g_gb(x, y) - avg(g_r(x, y), g_r(x, y+1));
        r_gb(x, y) = correction + avg(r_r(x, y), r_r(x, y+1));

        correction = g_gb(x, y) - avg(g_b(x, y), g_b(x+1, y));
        b_gb(x, y) = correction + avg(b_b(x, y), b_b(x+1, y));

        // Now interpolate diagonally to get red at blue and blue at
        // red. Hold onto your hats; this gets really fancy. We do the
        // same thing as for interpolating green where we try both
        // directions (in this case the positive and negative diagonals),
        // and use the one with the lowest absolute difference. But we
        // also use the same trick as interpolating red and blue at green
        // sites - we correct our interpolations using the second
        // derivative of green at the same sites.

        correction = g_b(x, y)  - avg(g_r(x, y), g_r(x-1, y+1));
        Expr rp_b  = correction + avg(r_r(x, y), r_r(x-1, y+1));
        Expr rpd_b = absd(r_r(x, y), r_r(x-1, y+1));

        correction = g_b(x, y)  - avg(g_r(x-1, y), g_r(x, y+1));
        Expr rn_b  = correction + avg(r_r(x-1, y), r_r(x, y+1));
        Expr rnd_b = absd(r_r(x-1, y), r_r(x, y+1));

        r_b(x, y)  = select(rpd_b < rnd_b, rp_b, rn_b);


        // Same thing for blue at red
        correction = g_r(x, y)  - avg(g_b(x, y), g_b(x+1, y-1));
        Expr bp_r  = correction + avg(b_b(x, y), b_b(x+1, y-1));
        Expr bpd_r = absd(b_b(x, y), b_b(x+1, y-1));

        correction = g_r(x, y)  - avg(g_b(x+1, y), g_b(x, y-1));
        Expr bn_r  = correction + avg(b_b(x+1, y), b_b(x, y-1));
        Expr bnd_r = absd(b_b(x+1, y), b_b(x, y-1));

        b_r(x, y)  =  select(bpd_r < bnd_r, bp_r, bn_r);

        // Interleave the resulting channels
        Func r = interleave_y(interleave_x(r_gr, r_r),
                              interleave_x(r_b, r_gb));
        Func g = interleave_y(interleave_x(g_gr, g_r),
                              interleave_x(g_b, g_gb));
        Func b = interleave_y(interleave_x(b_gr, b_r),
                              interleave_x(b_b, b_gb));

        Func output("demosaiced");
        output(x, y, c) = select(c == 0, r(x, y),
                                 c == 1, g(x, y),
                                 b(x, y));


        /* THE SCHEDULE */
        if (schedule == 1) {
            // optimized for ARM
            // Compute these in chunks over tiles, vectorized by 8
            g_r.compute_at(processed, tx).vectorize(x, 8);
            g_b.compute_at(processed, tx).vectorize(x, 8);
            r_gr.compute_at(processed, tx).vectorize(x, 8);
            b_gr.compute_at(processed, tx).vectorize(x, 8);
            r_gb.compute_at(processed, tx).vectorize(x, 8);
            b_gb.compute_at(processed, tx).vectorize(x, 8);
            r_b.compute_at(processed, tx).vectorize(x, 8);
            b_r.compute_at(processed, tx).vectorize(x, 8);
            // These interleave in y, so unrolling them in y helps
            output.compute_at(processed, tx)
                .vectorize(x, 8)
                .unroll(y, 2)
                .reorder(c, x, y).bound(c, 0, 3).unroll(c);

        } else if (schedule == 2) {
            // acelerator schedule
            g_r.linebuffer();
            g_b.linebuffer();

        } else if (schedule == 3) {
            // CUDA
            output.compute_root().unroll(y, 2).unroll(x, 2)
                .reorder(c, x, y).bound(c, 0, 3).unroll(c)
                .gpu_tile(x, y, 32, 16);;

        } else {
            // optimized for X86
            // Don't vectorize, because sse is bad at 16-bit interleaving
            g_r.compute_at(processed, tx);
            g_b.compute_at(processed, tx);
            r_gr.compute_at(processed, tx);
            b_gr.compute_at(processed, tx);
            r_gb.compute_at(processed, tx);
            b_gb.compute_at(processed, tx);
            r_b.compute_at(processed, tx);
            b_r.compute_at(processed, tx);
            // These interleave in x and y, so unrolling them helps
            output.compute_at(processed, tx).unroll(x, 2).unroll(y, 2)
                .reorder(c, x, y).bound(c, 0, 3).unroll(c);
        }

        return output;
    }


    Func color_correct(Func input, int32_t matrix[3][4]) {
        Expr ir = cast<int32_t>(input(x, y, 0));
        Expr ig = cast<int32_t>(input(x, y, 1));
        Expr ib = cast<int32_t>(input(x, y, 2));

        Expr r = matrix[0][3] + matrix[0][0] * ir + matrix[0][1] * ig + matrix[0][2] * ib;
        Expr g = matrix[1][3] + matrix[1][0] * ir + matrix[1][1] * ig + matrix[1][2] * ib;
        Expr b = matrix[2][3] + matrix[2][0] * ir + matrix[2][1] * ig + matrix[2][2] * ib;

        r = cast<int16_t>(r/256);
        g = cast<int16_t>(g/256);
        b = cast<int16_t>(b/256);
        corrected(x, y, c) = select(c == 0, r,
                                    select(c == 1, g, b));

        return corrected;
    }


    Func apply_curve(Func input, Type result_type, float gamma, float contrast) {
        // copied from FCam
        //Func curve("curve");

        Expr xf = clamp(cast<float>(x)/1024.0f, 0.0f, 1.0f);
        Expr g = pow(xf, 1.0f/gamma);
        Expr b = 2.0f - (float) pow(2.0f, contrast/100.0f);
        Expr a = 2.0f - 2.0f*b;
        Expr z = select(g > 0.5f,
                        1.0f - (a*(1.0f-g)*(1.0f-g) + b*(1.0f-g)),
                        a*g*g + b*g);

        Expr val = cast(result_type, clamp(z*256.0f, 0.0f, 255.0f));
        curve(x) = val;
        curve.compute_root(); // It's a LUT, compute it once ahead of time.

        if (schedule == 3) {
            // GPU schedule
            curve.gpu_tile(x, 256);
        }

        Func rgb("rgb");
        rgb(x, y, c) = curve(input(x, y, c));

        return rgb;
    }

    Func unsharp_img(Func input) {
        Func unsharp("unsharp"), ratio("ratio"), gray("gray");
        Func kernel, kernel_f, blur_x, sum_x, sharpen;
        // Define a 9x9 Gaussian Blur with a repeat-edge boundary condition.
        float sigma = 1.5f;
        RDom win2(-4, 9, -4, 9);

        kernel_f(x) = exp(-x*x/(2*sigma*sigma)) / (sqrtf(2*M_PI)*sigma);
        // normalize and convert to 8bit fixed point
        kernel(x) = cast<uint8_t>(kernel_f(x) * 255 /
                                  (kernel_f(0) + kernel_f(1)*2 + kernel_f(2)*2
                                   + kernel_f(3)*2 + kernel_f(4)*2));

        gray(x, y) = cast<uint8_t>((77 * cast<uint16_t>(input(x, y, 0))
                                    + 150 * cast<uint16_t>(input(x, y, 1))
                                    + 29 * cast<uint16_t>(input(x, y, 2))) >> 8);


        // 2D filter: direct map
        sum_x(x, y) += cast<uint32_t>(gray(x+win2.x, y+win2.y))
                                       * kernel(win2.x) * kernel(win2.y);
        blur_x(x, y) = cast<uint8_t>(sum_x(x, y) >> 16);

        sum_x.update(0).unroll(win2.x).unroll(win2.y);

        sharpen(x, y) = cast<uint8_t>(clamp(2 * cast<uint16_t>(gray(x, y)) - blur_x(x, y), 0, 255));

        ratio(x, y) = cast<uint8_t>(clamp(cast<uint16_t>(sharpen(x, y)) * 32 / max(gray(x, y), 1), 0, 255));

        unsharp(x, y, c) = cast<uint8_t>(clamp(cast<uint16_t>(ratio(x, y)) * input(x, y, c) / 32, 0, 255));

        /* THE SCHEDULE */
        if (schedule == 1) {
            // optimized for ARM
            // Compute these in chunks over tiles, vectorized by 8
            gray.compute_at(processed, tx).vectorize(x, 8);
            ratio.compute_at(processed, tx).vectorize(x, 8);
            unsharp.compute_at(processed, tx).vectorize(x, 8);

        } else if (schedule == 2) {
            // acelerator schedule

        } else if (schedule == 3) {
            // CUDA
            gray.compute_root().gpu_tile(x, y, 16, 16);
            //blur_y.compute_root().gpu_tile(x, y, 16, 16);
            ratio.compute_root().gpu_tile(x, y, 32, 16);
            //unsharp.gpu_tile(x, y, c, 16, 16, 1);
        }


        return unsharp;
    }

public:
    MyPipeline(int _schedule, int32_t matrix[3][4], float gamma, float contrast)
        : schedule(_schedule), input(UInt(16), 2), curve("curve")
    {
        // Parameterized output type, because LLVM PTX (GPU) backend does not
        // currently allow 8-bit computations
        int bit_width = 8; //atoi(argv[1]);
        Type result_type = UInt(bit_width);

        // The camera pipe is specialized on the 2592x1968 images that
        // come in, so we'll just use an image instead of a uniform image.

        // shift things inwards to give us enough padding on the
        // boundaries so that we don't need to check bounds. We're going
        // to make a 2560x1920 output image, just like the FCam pipe, so
        // shift by 16, 12

        shifted(x, y) = input(x+16, y+12);

        denoised = hot_pixel_suppression(shifted);
        deinterleaved = deinterleave(denoised);
        demosaiced = demosaic(deinterleaved);
        corrected = color_correct(demosaiced, matrix);
        rgb = apply_curve(corrected, result_type, gamma, contrast);
        hw_output = unsharp_img(rgb);

        processed(tx, ty, c) = hw_output(tx, ty, c);

        // Schedule
        // We can generate slightly better code if we know the output is a whole number of tiles.
        Expr out_width = processed.output_buffer().width();
        Expr out_height = processed.output_buffer().height();
        processed
            .bound(tx, 0, (out_width/128)*128)
            .bound(ty, 0, (out_height/128)*128)
            .bound(c, 0, 3); // bound color loop 0-3, properly

        args = {input};
    }

    void compile_cpu() {
        assert(schedule == 1);

        std::cout << "\ncompiling cpu code..." << std::endl;

        // Compute in chunks over tiles, vectorized by 8
        denoised.compute_at(processed, tx).vectorize(x, 8);
        deinterleaved.compute_at(processed, tx).vectorize(x, 8).reorder(c, x, y).unroll(c);
        corrected.compute_at(processed, tx).vectorize(x, 4).reorder(c, x, y).unroll(c);
        processed.tile(tx, ty, xi, yi, 32, 32).reorder(xi, yi, c, tx, ty);
        processed.parallel(ty);

        //processed.print_loop_nest();
        processed.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
        processed.compile_to_file("pipeline_native", args);
    }

    void compile_gpu() {
        assert(schedule == 3);
        std::cout << "\ncompiling gpu code..." << std::endl;

        rgb.compute_root().gpu_tile(x, y, c, 32, 16, 1);
        denoised.compute_root().gpu_tile(x, y, 32, 16);
        processed.gpu_tile(tx, ty, c, 16, 16, 1);
        /*
        deinterleaved.compute_root()
            .reorder(c, x, y).unroll(c)
            .gpu_tile(x, y, 32, 16);
        */

        //processed.print_loop_nest();
        Target target = get_target_from_environment();
        target.set_feature(Target::CUDA);
        processed.compile_to_lowered_stmt("pipeline_cuda.ir.html", args, HTML, target);
        processed.compile_to_header("pipeline_cuda.h", args, "pipeline_cuda", target);
        processed.compile_to_object("pipeline_cuda.o", args, "pipeline_cuda", target);
    }

};


class MyPipelineOpt {
    int schedule;
    ImageParam input;

    std::vector<Argument> args;

    Func shifted;
    Func processed;
    Func denoised;
    Func deinterleaved;
    Func demosaiced;
    Func corrected;
    Func hw_output;
    Func curve;
    Func rgb;

    Func interleave_x(Func a, Func b) {
        Func out;
        out(x, y) = select((x%2)==0, a(x, y), b(x-1, y));
        return out;
    }

    Func interleave_y(Func a, Func b) {
        Func out;
        out(x, y) = select((y%2)==0, a(x, y), b(x, y-1));
        return out;
    }

    // The demosaic algorithm is optimized for HLS schedule
    // such that the bound analysis can derive a constant window
    // and shift step without needed to unroll 'demosaic' into
    // a 2x2 grid.
    //
    // The chances made from the original is that there is no
    // explict downsample and upsample in 'deinterleave' and
    // 'interleave', respectively.
    // All the intermediate functions are the same size as the
    // raw image although only pixels at even coordinates are used.
    Func demosaic(Func raw) {

        Func r_r, g_gr, g_gb, b_b;
        g_gr(x, y) = raw(x, y);//deinterleaved(x, y, 0);
        r_r(x, y)  = raw(x+1, y);//deinterleaved(x, y, 1);
        b_b(x, y)  = raw(x, y+1);//deinterleaved(x, y, 2);
        g_gb(x, y) = raw(x+1, y+1);//deinterleaved(x, y, 3);


        // These are the ones we need to interpolate
        Func b_r, g_r, b_gr, r_gr, b_gb, r_gb, r_b, g_b;

        Expr gv_r  = avg(g_gb(x, y-2), g_gb(x, y));
        Expr gvd_r = absd(g_gb(x, y-2), g_gb(x, y));
        Expr gh_r  = avg(g_gr(x+2, y), g_gr(x, y));
        Expr ghd_r = absd(g_gr(x+2, y), g_gr(x, y));

        g_r(x, y)  = select(ghd_r < gvd_r, gh_r, gv_r);

        Expr gv_b  = avg(g_gr(x, y+2), g_gr(x, y));
        Expr gvd_b = absd(g_gr(x, y+2), g_gr(x, y));
        Expr gh_b  = avg(g_gb(x-2, y), g_gb(x, y));
        Expr ghd_b = absd(g_gb(x-2, y), g_gb(x, y));

        g_b(x, y)  = select(ghd_b < gvd_b, gh_b, gv_b);

        Expr correction;
        correction = g_gr(x, y) - avg(g_r(x, y), g_r(x-2, y));
        r_gr(x, y) = correction + avg(r_r(x-2, y), r_r(x, y));

        // Do the same for other reds and blues at green sites
        correction = g_gr(x, y) - avg(g_b(x, y), g_b(x, y-2));
        b_gr(x, y) = correction + avg(b_b(x, y), b_b(x, y-2));

        correction = g_gb(x, y) - avg(g_r(x, y), g_r(x, y+2));
        r_gb(x, y) = correction + avg(r_r(x, y), r_r(x, y+2));

        correction = g_gb(x, y) - avg(g_b(x, y), g_b(x+2, y));
        b_gb(x, y) = correction + avg(b_b(x, y), b_b(x+2, y));

        correction = g_b(x, y)  - avg(g_r(x, y), g_r(x-2, y+2));
        Expr rp_b  = correction + avg(r_r(x, y), r_r(x-2, y+2));
        Expr rpd_b = absd(r_r(x, y), r_r(x-2, y+2));

        correction = g_b(x, y)  - avg(g_r(x-2, y), g_r(x, y+2));
        Expr rn_b  = correction + avg(r_r(x-2, y), r_r(x, y+2));
        Expr rnd_b = absd(r_r(x-2, y), r_r(x, y+2));

        r_b(x, y)  = select(rpd_b < rnd_b, rp_b, rn_b);

        // Same thing for blue at red
        correction = g_r(x, y)  - avg(g_b(x, y), g_b(x+2, y-2));
        Expr bp_r  = correction + avg(b_b(x, y), b_b(x+2, y-2));
        Expr bpd_r = absd(b_b(x, y), b_b(x+2, y-2));

        correction = g_r(x, y)  - avg(g_b(x+2, y), g_b(x, y-2));
        Expr bn_r  = correction + avg(b_b(x+2, y), b_b(x, y-2));
        Expr bnd_r = absd(b_b(x+2, y), b_b(x, y-2));

        b_r(x, y)  =  select(bpd_r < bnd_r, bp_r, bn_r);

        // Interleave the resulting channels
        Func r = interleave_y(interleave_x(r_gr, r_r),
                              interleave_x(r_b, r_gb));
        Func g = interleave_y(interleave_x(g_gr, g_r),
                              interleave_x(g_b, g_gb));
        Func b = interleave_y(interleave_x(b_gr, b_r),
                              interleave_x(b_b, b_gb));


        Func output("demosaiced");
        output(x, y, c) = select(c == 0, r(x, y),
                                 c == 1, g(x, y),
                                 b(x, y));

        return output;
    }


    Func color_correct(Func input, int32_t matrix[3][4]) {
        Expr ir = cast<int32_t>(input(x, y, 0));
        Expr ig = cast<int32_t>(input(x, y, 1));
        Expr ib = cast<int32_t>(input(x, y, 2));

        Expr r = matrix[0][3] + matrix[0][0] * ir + matrix[0][1] * ig + matrix[0][2] * ib;
        Expr g = matrix[1][3] + matrix[1][0] * ir + matrix[1][1] * ig + matrix[1][2] * ib;
        Expr b = matrix[2][3] + matrix[2][0] * ir + matrix[2][1] * ig + matrix[2][2] * ib;

        r = cast<int16_t>(r/256);
        g = cast<int16_t>(g/256);
        b = cast<int16_t>(b/256);
        corrected(x, y, c) = select(c == 0, r,
                                    select(c == 1, g, b));

        return corrected;
    }


    Func apply_curve(Func input, float gamma, float contrast) {
        // copied from FCam

        //Expr xf = clamp(cast<float>(x)/1024.0f, 0.0f, 1.0f);
        Expr xf = x/1024.0f;
        Expr g = pow(xf, 1.0f/gamma);
        Expr b = 2.0f - (float) pow(2.0f, contrast/100.0f);
        Expr a = 2.0f - 2.0f*b;
        Expr val = select(g > 0.5f,
                        1.0f - (a*(1.0f-g)*(1.0f-g) + b*(1.0f-g)),
                        a*g*g + b*g);

        curve(x) = cast<uint8_t>(clamp(val*256.0f, 0.0f, 255.0f));

        Func rgb("rgb");
        Expr in_val = clamp(input(x, y, c), 0, 1023);
        //rgb(x, y, c) = curve(input(x, y, c));
        rgb(x, y, c) = select(input(x, y, c) < 0, 0,
                              select(input(x, y, c) >= 1024, 255,
                                     curve(in_val)));

        return rgb;
    }

    Func unsharp_img(Func input) {
        Func unsharp("unsharp"), ratio("ratio"), gray("gray");
        Func kernel, kernel_f, blur_x, sum_x, sharpen;
        // Define a 9x9 Gaussian Blur with a repeat-edge boundary condition.
        float sigma = 1.5f;
        RDom win2(-4, 9, -4, 9);

        kernel_f(x) = exp(-x*x/(2*sigma*sigma)) / (sqrtf(2*M_PI)*sigma);
        // normalize and convert to 8bit fixed point
        kernel(x) = cast<uint8_t>(kernel_f(x) * 255 /
                                  (kernel_f(0) + kernel_f(1)*2 + kernel_f(2)*2
                                   + kernel_f(3)*2 + kernel_f(4)*2));

        gray(x, y) = cast<uint8_t>((77 * cast<uint16_t>(input(x, y, 0))
                                    + 150 * cast<uint16_t>(input(x, y, 1))
                                    + 29 * cast<uint16_t>(input(x, y, 2))) >> 8);


        // 2D filter: direct map
        sum_x(x, y) += cast<uint32_t>(gray(x+win2.x, y+win2.y))
                                       * kernel(win2.x) * kernel(win2.y);
        blur_x(x, y) = cast<uint8_t>(sum_x(x, y) >> 16);

        sum_x.update(0).unroll(win2.x).unroll(win2.y);

        sharpen(x, y) = cast<uint8_t>(clamp(2 * cast<uint16_t>(gray(x, y)) - blur_x(x, y), 0, 255));

        ratio(x, y) = cast<uint8_t>(clamp(cast<uint16_t>(sharpen(x, y)) * 32 / max(gray(x, y), 1), 0, 255));

        unsharp(c, x, y) = cast<uint8_t>(clamp(cast<uint16_t>(ratio(x, y)) * input(x, y, c) / 32, 0, 255));

        /* THE SCHEDULE */
        gray.linebuffer();
        ratio.linebuffer();

        return unsharp;
    }

public:
    MyPipelineOpt(int _schedule, int32_t matrix[3][4], float gamma, float contrast)
        : schedule(_schedule), input(UInt(16), 2), curve("curve")
    {
        // The camera pipe is specialized on the 2592x1968 images that
        // come in, so we'll just use an image instead of a uniform image.

        // shift things inwards to give us enough padding on the
        // boundaries so that we don't need to check bounds. We're going
        // to make a 2560x1920 output image, just like the FCam pipe, so
        // shift by 16, 12

        shifted(x, y) = input(x+16, y+12);

        denoised = hot_pixel_suppression(shifted);
        demosaiced = demosaic(denoised);
        corrected = color_correct(demosaiced, matrix);
        rgb = apply_curve(corrected, gamma, contrast);
        hw_output = unsharp_img(rgb);

        processed(x, y, c) = hw_output(c, x, y);

        // Schedule
        // We can generate slightly better code if we know the output is a whole number of tiles.
        Expr out_width = processed.output_buffer().width();
        Expr out_height = processed.output_buffer().height();
        processed
            .bound(x, 0, (out_width/640)*640)
            .bound(y, 0, (out_height/480)*480)
            .bound(c, 0, 3); // bound color loop 0-3, properly

        args = {input};
    }

    void compile_hls() {
        assert(schedule == 2);
        std::cout << "\ncompiling HLS code..." << std::endl;

        // Block in chunks over tiles
        processed.tile(x, y, xo, yo, xi, yi, 640, 480)
            .reorder(c, xi, yi, xo, yo);
        shifted.compute_at(processed, xo);
        hw_output.compute_at(processed, xo);

        hw_output.tile(x, y, xo, yo, xi, yi, 640, 480)
            .reorder(c, xi, yi, xo, yo).unroll(c);;
        //hw_output.unroll(xi, 2);
        hw_output.accelerate({shifted}, xi, xo);

        curve.compute_at(hw_output, xo).unroll(x);  // synthesize curve to a ROM

        denoised.linebuffer()
            .unroll(x).unroll(y);
        demosaiced.linebuffer()
            .unroll(c).unroll(x).unroll(y);
        rgb.linebuffer().unroll(c).unroll(x).unroll(y)
            .fifo_depth(hw_output, 480*9);

        //processed.print_loop_nest();
        processed.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML);
        processed.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls");
        processed.compile_to_header("pipeline_hls.h", args, "pipeline_hls");

        std::vector<Target::Feature> features({Target::Zynq});
        Target target(Target::Linux, Target::ARM, 32, features);
        processed.compile_to_zynq_c("pipeline_zynq.c", args, "pipeline_zynq", target);
        processed.compile_to_header("pipeline_zynq.h", args, "pipeline_zynq", target);

        shifted.vectorize(x, 8);
        processed
            .vectorize(xi, 16).unroll(c);
        processed.fuse(xo, yo, xo).parallel(xo);

        //shifted.set_stride(0, 3).set_stride(2, 1).set_bounds(2, 0, 3);
        processed.output_buffer().set_stride(0, 3).set_stride(2, 1);

        processed.compile_to_object("pipeline_zynq.o", args, "pipeline_zynq", target);
        processed.compile_to_lowered_stmt("pipeline_zynq.ir.html", args, HTML, target);
        processed.compile_to_assembly("pipeline_zynq.s", args, "pipeline_zynq", target);
    }
};


int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: ./pipeline color_temp gamma contrast\n"
               "e.g. ./pieline 3200 2 50");
        return 0;
    }

    float color_temp = atof(argv[1]);
    float gamma = atof(argv[2]);
    float contrast = atof(argv[3]);

    // These color matrices are for the sensor in the Nokia N900 and are
    // taken from the FCam source.
    float matrix_3200[][4] = {{ 1.6697f, -0.2693f, -0.4004f, -42.4346f},
                               {-0.3576f,  1.0615f,  1.5949f, -37.1158f},
                               {-0.2175f, -1.8751f,  6.9640f, -26.6970f}};

    float matrix_7000[][4] = {{ 2.2997f, -0.4478f,  0.1706f, -39.0923f},
                               {-0.3826f,  1.5906f, -0.2080f, -25.4311f},
                               {-0.0888f, -0.7344f,  2.2832f, -20.0826f}};


    // Get a color matrix by linearly interpolating between two
    // calibrated matrices using inverse kelvin.
    int32_t matrix[3][4];
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 4; x++) {
            float alpha = (1.0f/color_temp - 1.0f/3200) / (1.0f/7000 - 1.0f/3200);
            float val =  matrix_3200[y][x] * alpha + matrix_7000[y][x] * (1 - alpha);
            matrix[y][x] = (int32_t)(val * 256.0f); // Q8.8 fixed point
        }
    }

    MyPipeline p1(1, matrix, gamma, contrast);
    p1.compile_cpu();

    MyPipelineOpt p2(2, matrix, gamma, contrast);
    p2.compile_hls();

    MyPipeline p3(3, matrix, gamma, contrast);
    p3.compile_gpu();
    return 0;
}
