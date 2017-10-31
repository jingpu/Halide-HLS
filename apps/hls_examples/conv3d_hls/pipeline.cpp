#include "Halide.h"
#include <string.h>

using namespace Halide;
using std::string;


Var x("x"), y("y"), c("c"), k("k");
Var xo("xo"), xi("xi"), yi("yi"), yo("yo"), co("co"), ci("ci");

class MyPipeline {
public:
    ImageParam input;
    ImageParam weight;
    Param<uint8_t> bias;
    Func clamped;
    Func clamped_buf_copy;
    Func weight_buf_copy;
    //Func conv1;
    Func output;
    Func hw_output;
    std::vector<Argument> args;

    Func conv1;
    RDom r;
 
    MyPipeline() : input(UInt(8), 3, "input"), weight(UInt(8), 4, "weight"),
                   clamped("clamped"), output("output"), //hw_output("hw_output"),
                   conv1("conv1"), r(-2, 5, -2, 5, 0, 3, 0, 4) {
        
        // define the algorithm
        //clamped = BoundaryConditions::repeat_edge(input);
        clamped(x, y, c) = input(x+2, y+2, c);
        weight_buf_copy(x, y, c, k) = weight(x, y, c, k);       
        clamped_buf_copy(x, y, c) = clamped(x, y, c); 
        //conv1(x, y, c) += (clamped_buf_copy(x+r.x, y+r.y, r.z+r.w*3)) * weight_buf_copy(r.x+2, r.y+2, r.z+r.w*3, c);
        conv1(x, y, c) += (clamped_buf_copy(x+r.x, y+r.y, r.z+r.w*3)) * weight_buf_copy(c, r.x+2, r.y+2, r.z+r.w*3);
        //hw_output(x, y, c) = conv1(x, y, c);
        //hw_output = convolve55_rd(clamped);
        output(x, y, c) = conv1(x, y, c); //hw_output(x, y, c);
 
        conv1.update(0).unroll(r.x).unroll(r.y).unroll(r.z);
        // define common schedule: tile output, and linebuffer the intermediate
        output.tile(x, y, xo, yo, xi, yi, 64, 64).reorder(xi, yi, c, xo, yo);

        // restrict arguments
        weight.dim(0).set_bounds(0, 5);
        weight.dim(1).set_bounds(0, 5);
        weight.dim(2).set_bounds(0, 12);
        weight.dim(3).set_bounds(0, 2);
        weight.dim(0).set_stride(1);
        weight.dim(1).set_stride(5);
        weight.dim(2).set_stride(25);
        weight.dim(3).set_stride(300);

        output.bound(x, 0, 256);
        output.bound(y, 0, 256);
        output.bound(c, 0, 2);

        args.push_back(input);
        args.push_back(weight);
        args.push_back(bias);
    }

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;

        clamped.compute_root();

        //hw_output.tile(x, y, xo, yo, xi, yi, 64, 64).reorder(xi, yi, c, xo, yo);
        //hw_output.compute_root();
        //hw_output.compute_at(output, xo);//.tile(x, y, xo, yo, xi, yi, 64, 64).reorder(xi, yi, c, xo, yo);
  
        clamped_buf_copy.compute_at(conv1, c); 
        weight_buf_copy.compute_at(conv1, c); 

        conv1.update(0).unroll(r.x).unroll(r.y).unroll(r.z).unroll(r.w);
        //conv1.in().tile(x, y, xo, yo, xi, yi, 64, 64).reorder(xi, yi, c, xo, yo);
        //conv1.update(0).reorder(x, y, r.w, c); //.tile(x, y, xo, yo, xi, yi, 64, 64).reorder(xi, yi, r.w, c, xo, yo);
        conv1.update(0).reorder(x, y, c); //.tile(x, y, xo, yo, xi, yi, 64, 64).reorder(xi, yi, r.w, c, xo, yo);
        //conv1.compute_at(conv1.in(), c);
        //conv1.in().compute_at(hw_output, xo);
        //conv1.compute_at(hw_output, xo);
        conv1.compute_at(output, xo);

        //clamped.compute_root();
        //output.tile(x, y, xo, yo, xi, yi, 256, 256).reorder(c, xi, yi, xo, yo);
        //hw_output.compute_at(output, xi).store_at(output, xo);
        //output.fuse(xo, yo, xo).parallel(xo);
        //hw_output.unroll(c);

        //output.vectorize(xi, 8);
        /*conv1.compute_at(hw_output, xi).store_at(hw_output, xo); 

        clamped.compute_at(output, xo);

        hw_output.compute_at(output, xo).tile(x, y, xo, yo, xi, yi, 256, 256);


        hw_output.store_at(output, xo).compute_at(output, xi);
        */
        output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        //output.compile_to_object("pipeline_native.o", args, "pipeline_native");
        output.compile_to_c("pipeline_native.cpp", args, "pipeline_native");
    }

    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;

        // HLS schedule: make a hw pipeline producing 'hw_output', taking
        // inputs of 'clamped', buffering intermediates at (output, xo) loop
        // level
        //clamped.compute_at(output, xo);
        clamped.compute_root();

        //hw_output.tile(x, y, xo, yo, xi, yi, 64, 64).reorder(xi, yi, c, xo, yo);
        //hw_output.compute_at(output, xo); // compute_root(); //TODO outer loop scheduled by hw side

        clamped_buf_copy.compute_at(conv1, r.w);  
        weight_buf_copy.compute_at(conv1, r.w);  

        conv1.update(0).reorder(x, y, r.w, c);//.tile(x, y, xo, yo, xi, yi, 64, 64).reorder(xi, yi, r.w, c, xo, yo);
        //conv1.compute_at(hw_output, xo);//.store_at(hw_output, xo);
        conv1.compute_at(output, xo);

        //hw_output.accelerate({clamped}, xo, xo);
        output.accelerate({clamped}, xo, xo);
        //hw_output.store_at(output, xo).compute_at(output, xo);
        
        //clamped.linebuffer();

        //output.print_loop_nest();
        Target hls_target = get_target_from_environment();
        hls_target.set_feature(Target::CPlusPlusMangling);

        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML, hls_target);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls", hls_target);
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls", hls_target);
    }

private:
    Func convolve55_rd(Func in) {
        Func local_sum, res;
        RDom r(-2, 5, -2, 5, 0, 3, 0, 4);

        //local_sum(x, y, c) = cast<uint16_t>(bias);
        local_sum(x, y, c) += cast<uint16_t>(in(x+r.x, y+r.y, r.z+r.w*3)) * weight(r.x+2, r.y+2, r.z+r.w*3, c);
        res(x, y, c) = cast<uint8_t>(local_sum(x, y, c) >> 8);

        // unroll the reduction
        local_sum.update(0).unroll(r.x).unroll(r.y);
        local_sum.update(0).reorder(x, y, r.w, c);

        return res;
    }
};

class MyPipelineGemm {
public:
    ImageParam input;
    ImageParam weight;
    Param<uint8_t> bias;
    Func in;
    Func clamped;
    Func clamped_buf_copy;
    Func weight_buf_copy;
    //Func conv1;
    Func output;
    Func hw_output;
    std::vector<Argument> args;

    Func conv1;
    RDom r;
 
    MyPipelineGemm() : input(UInt(8), 3, "input"), weight(UInt(8), 4, "weight"),
                   in("in"), clamped("clamped"), output("output"), //hw_output("hw_output"),
                   conv1("conv1"), r(0, 8, 0, 3, 0, 3, 0, 4) {
        
        // define the algorithm
        //clamped = BoundaryConditions::repeat_edge(input);
        //clamped(x, y, c) = input(x+1, y+1, c);
        //clamped = BoundaryConditions::constant_exterior(input, 0, {{0, 260}, {0,260}, {0, 12}});
        clamped_buf_copy = BoundaryConditions::constant_exterior(input, 0, {{0, 124}, {0, 32}, {0, 32}});
        //clamped_buf_copy(x, y, c) = clamped(x, y, c); 
        weight_buf_copy(x, y, c, k) = weight(x, y, c, k); 
        conv1(c, x, y) += cast<uint16_t>(clamped_buf_copy(r.x+r.w*8, x+r.y-1, y+r.z-1)) * cast<uint16_t>(weight_buf_copy(r.x+r.w*8, r.y, r.z, c));
        //hw_output(x, y, c) = conv1(x, y, c);
        //hw_output = convolve55_rd(clamped);
        output(c, x, y) = conv1(c, x, y); //hw_output(x, y, c);
 
        //conv1.update(0).unroll(r.x).unroll(r.y).unroll(r.z);
        // define common schedule: tile output, and linebuffer the intermediate
        output.tile(x, y, xo, yo, xi, yi, 62, 16);  
        output.split(c, co, ci, 16);
        output.reorder(ci, xi, yi, co, xo, yo);

        // restrict arguments
        weight.dim(0).set_bounds(0, 32);
        weight.dim(1).set_bounds(0, 3);
        weight.dim(2).set_bounds(0, 3);
        weight.dim(3).set_bounds(0, 32);
        weight.dim(0).set_stride(1);
        weight.dim(1).set_stride(32);
        weight.dim(2).set_stride(96);
        weight.dim(3).set_stride(288);
        
        output.bound(c, 0, 32);
        output.bound(x, 0, 124);
        output.bound(y, 0, 32);

        args.push_back(input);
        args.push_back(weight);
        args.push_back(bias);
    }

    void compile_cpu() {
        std::cout << "\ncompiling cpu code..." << std::endl;

        clamped.compute_root();
        //hw_output.tile(x, y, xo, yo, xi, yi, 64, 64).reorder(xi, yi, c, xo, yo);
        //hw_output.compute_root();
        //hw_output.compute_at(output, xo);//.tile(x, y, xo, yo, xi, yi, 64, 64).reorder(xi, yi, c, xo, yo);
  
        clamped_buf_copy.compute_at(conv1, c);  
        weight_buf_copy.compute_at(conv1, c);  

        conv1.update(0).unroll(r.x).unroll(r.y).unroll(r.z).unroll(r.w);
        //conv1.in().tile(x, y, xo, yo, xi, yi, 64, 64).reorder(xi, yi, c, xo, yo);
        //conv1.update(0).reorder(x, y, r.w, c); //.tile(x, y, xo, yo, xi, yi, 64, 64).reorder(xi, yi, r.w, c, xo, yo);
        conv1.update(0).reorder(x, y, c); //.tile(x, y, xo, yo, xi, yi, 64, 64).reorder(xi, yi, r.w, c, xo, yo);
        //conv1.compute_at(conv1.in(), c);
        //conv1.in().compute_at(hw_output, xo);
        //conv1.compute_at(hw_output, xo);
        conv1.compute_at(output, xo);

        //clamped.compute_root();
        //output.tile(x, y, xo, yo, xi, yi, 256, 256).reorder(c, xi, yi, xo, yo);
        //hw_output.compute_at(output, xi).store_at(output, xo);
        //output.fuse(xo, yo, xo).parallel(xo);
        //hw_output.unroll(c);

        //output.vectorize(xi, 8);
        /*conv1.compute_at(hw_output, xi).store_at(hw_output, xo); 

        clamped.compute_at(output, xo);

        hw_output.compute_at(output, xo).tile(x, y, xo, yo, xi, yi, 256, 256);


        hw_output.store_at(output, xo).compute_at(output, xi);
        */
        output.compile_to_lowered_stmt("pipeline_native.ir.html", args, HTML);
        output.compile_to_header("pipeline_native.h", args, "pipeline_native");
        //output.compile_to_object("pipeline_native.o", args, "pipeline_native");
        output.compile_to_c("pipeline_native.cpp", args, "pipeline_native");
    }

    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;

        // HLS schedule: make a hw pipeline producing 'hw_output', taking
        // inputs of 'clamped', buffering intermediates at (output, xo) loop
        // level
        //clamped.compute_at(output, co);
        //clamped.compute_root();

        //hw_output.tile(x, y, xo, yo, xi, yi, 64, 64).reorder(xi, yi, c, xo, yo);
        //hw_output.compute_at(output, xo); // compute_root(); //TODO outer loop scheduled by hw side

        clamped_buf_copy.compute_at(output, co);  
        weight_buf_copy.compute_at(output, co);  

        conv1.update(0).split(c, co, ci, 8);
        conv1.update(0).reorder(r.x, ci, co, x, y, r.y, r.z, r.w);//.tile(x, y, xo, yo, xi, yi, 64, 64).reorder(xi, yi, r.w, c, xo, yo);
        conv1.update(0).unroll(r.x);//.unroll(ci);
        //conv1.compute_at(hw_output, xo);//.store_at(hw_output, xo);
        conv1.compute_at(output, co);

        //hw_output.accelerate({clamped}, xo, xo);
        output.accelerate({input}, xo, xo);
        //hw_output.store_at(output, xo).compute_at(output, xo);
        
        //clamped.linebuffer();

        //output.print_loop_nest();
        Target hls_target = get_target_from_environment();
        hls_target.set_feature(Target::CPlusPlusMangling);

        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML, hls_target);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls", hls_target);
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls", hls_target);
    }

};


int main(int argc, char **argv) {
    //MyPipeline p1;
    //p1.compile_cpu();

    MyPipelineGemm p2;
    p2.compile_hls();

    return 0;
}
