#include "MarkHWKernels.h"
#include "IRVisitor.h"

namespace Halide{
namespace Internal {

using std::set;
using std::vector;
using std::string;


class MarkHWKernels : public IRVisitor {
public:
    vector<Function> marked_functions;

    using IRVisitor::visit;

    void mark_function(Function f) {
        if (f.schedule().is_hw_kernel() == false) {
            f.schedule().is_hw_kernel() = true;
            marked_functions.push_back(f);
        }
    }

    void visit(const Call *op) {
        IRVisitor::visit(op);

        if(op->call_type == Call::Halide) {
            Function f = op->func;
            mark_function(f);
        }
    }
};

// TODO check if the inputs are valid, i.e.
// the recursion always exit when hitting the inputs
void mark_hw_kernels(Function output, const set<string> &inputs) {
    internal_assert(inputs.count(output.name()) == 0)
        << "Output Function " << output.name() << " cannot be in the inputs set.\n";

    output.schedule().is_hw_kernel() = true;

    MarkHWKernels marker;
    output.accept(&marker);

    for (Function f : marker.marked_functions) {
        if (!inputs.count(f.name())) {
            // recurse if f is not in inputs
            mark_hw_kernels(f, inputs);
        }
    }
}

}
}
