#ifndef HALIDE_EXTRACT_HW_KERNEL_DAG_H
#define HALIDE_EXTRACT_HW_KERNEL_DAG_H

/** \file
 *
 * Defines the analysis pass to extract hard kernel DAG
 */


#include <set>
#include <map>

#include "IR.h"
#include "Bounds.h"

namespace Halide {
namespace Internal {

/** A simplified definition of BoundsInference::Stage
 */
struct BoundsInference_Stage {
    std::string name;
    size_t stage;
    std::vector<int> consumers;
    std::map<std::pair<std::string, int>, Box> bounds;
};



struct StencilDimSpecs {
    int size;  // stencil window size
    int step;     // stencil window shifting step
    Expr min_pos; // stencil origin position w.r.t. the original image buffer
    std::string loop_var;  // outer loop var that shifts this dimensions
    Interval store_bound;
};

struct HWKernel {
    Function func;
    std::string name;
    bool is_inlined;
    bool is_output;
    std::vector<StencilDimSpecs> dims;
    std::vector<std::string> input_streams;  // used when inserting read_stream calls
    std::map<std::string, std::vector<StencilDimSpecs> > consumer_stencils; // used for transforming call nodes and inserting dispatch calls
    std::map<std::string, int> consumer_fifo_depths;

    HWKernel() : is_inlined(false), is_output(false) {}
    HWKernel(Function f, const std::string &s)
        : func(f), name(s), is_inlined(false), is_output(false) {}
};

struct HWTap {
    std::string name;
    bool is_func;
    Function func;
    Parameter param;
    std::vector<StencilDimSpecs> dims;
};

struct HWKernelDAG {
    std::string name;
    std::map<std::string, HWKernel> kernels;
    std::map<std::string, HWTap> taps;
    std::set<std::string> input_kernels;
    std::set<std::string> loop_vars;   // FIXME we use loop_vars name to figure out the location to start Stream transformation. Need better way.
    LoopLevel compute_level, store_level;
};

std::ostream &operator<<(std::ostream &out, const HWKernel &k);
std::ostream &operator<<(std::ostream &out, const HWTap &t);

/** Perform analysis to extract hard kernel DAG
 */
Stmt extract_hw_kernel_dag(Stmt s, const std::map<std::string, Function> &env,
                           const std::vector<BoundsInference_Stage> &inlined_stages,
                           std::vector<HWKernelDAG> &dags);

}
}

#endif
