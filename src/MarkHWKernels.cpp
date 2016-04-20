#include "MarkHWKernels.h"
#include "IRVisitor.h"

namespace Halide{
namespace Internal {

using std::set;
using std::vector;
using std::string;


class MarkHWKernels : public IRVisitor {
    const string &exit_name;

    using IRVisitor::visit;

    void visit(const Call *op) {
        IRVisitor::visit(op);

        if(op->call_type == Call::Halide) {
            Function f = op->func;
            mark_function(f);
        }
    }

    void mark_function(Function f) {
        if (f.schedule().is_hw_kernel() == false) {
            f.schedule().is_hw_kernel() = true;
            f.schedule().accelerate_exit() = exit_name;
            marked_functions.push_back(f);
            debug(4) << "Function " << f.name() << " is marked as hw_kernel.\n";
        }
    }

public:
    vector<Function> marked_functions;

    MarkHWKernels(const string &s) : exit_name(s) {}
};

// TODO check if the inputs are valid, i.e.
// the recursion always exit when hitting the inputs
void mark_hw_kernels_recurse(Function output, const set<string> &inputs,
                             const string &exit_name) {
    internal_assert(inputs.count(output.name()) == 0)
        << "Output Function " << output.name() << " cannot be in the inputs set.\n";

    output.schedule().is_hw_kernel() = true;
    output.schedule().accelerate_exit() = exit_name;

    MarkHWKernels marker(exit_name);
    output.accept(&marker);

    for (Function f : marker.marked_functions) {
        if (!inputs.count(f.name())) {
            // recurse if f is not in inputs
            mark_hw_kernels_recurse(f, inputs, exit_name);
        }
    }
}

void mark_hw_kernels(Function output, const set<string> &inputs) {
    mark_hw_kernels_recurse(output, inputs, output.name());
}

}
}
