#include "MarkHWKernels.h"
#include "IRVisitor.h"

namespace Halide{
namespace Internal {

using std::set;
using std::map;
using std::vector;
using std::string;
using std::make_pair;

template <typename T>
std::ostream &operator<<(std::ostream &out, const map<string, T> &s) {
    out << "[";
    for (auto it = s.begin(); it != s.end(); ++it) {
        out << it->first;
        if (next(it) != s.end())
            out << ", ";
    }
    out << "]";
    return out;
}

std::ostream &operator<<(std::ostream &out, const set<string> &s) {
    out << "[";
    for (auto it = s.begin(); it != s.end(); ++it) {
        out << *it;
        if (next(it) != s.end())
            out << ", ";
    }
    out << "]";
    return out;
}

class GetCallees : public IRVisitor {
    using IRVisitor::visit;

    void visit(const Call *op) {
        IRVisitor::visit(op);
        if(op->call_type == Call::Halide && op->func.defined()) {
            Function f = Function(op->func);
            funcs[f.name()] = f;
        } else if (op->call_type == Call::Image) {
            internal_assert(op->param.defined());
            Parameter p = op->param;
            params[p.name()] = p;
        }
    }

public:
    map<string, Function> funcs;
    map<string, Parameter> params;
};

class MarkHWKernels {
public:
    MarkHWKernels(Function output, const set<string> &inputs, const set<string> &taps)
        : output_func(output), input_names(inputs), tap_names(taps) {}

    void mark() {
        vector<Function> path;
        dfs(output_func, path);

        internal_assert(input_names.size() == input_funcs.size())
            << "Not all input functions to accelerated function " << output_func.name()
            << " can be found.\n";

        internal_assert(tap_names.size() == tap_funcs.size())
            << "Not all tap functions to accelerated function " << output_func.name()
            << " can be found.\n";

        // save tap names in the schedule struct
        output_func.schedule().tap_funcs() = tap_funcs;
        output_func.schedule().tap_params() = tap_params;

        debug(3) << "marked inputs: " << input_funcs << "\n";
        debug(3) << "marked kernels: " << kernel_funcs << "\n";
        debug(3) << "marked tap funcs: " << tap_funcs << "\n";
        debug(3) << "marked tap params: " << tap_params << "\n";


        for (auto &&p : kernel_funcs) {
            p.second.schedule().is_hw_kernel() = true;
            p.second.schedule().accelerate_exit() = output_func.name();
        }
        for (auto &&p : input_funcs) {
            p.second.schedule().is_hw_kernel() = true;
            p.second.schedule().accelerate_exit() = output_func.name();
        }

    }

    void dfs(Function func, vector<Function> &path) {
        if (input_names.count(func.name()) > 0) {
            // visit a input function, save path and input function
            // then terminate
            for (const auto &f : path) kernel_funcs[f.name()] = f;
            input_funcs.insert(make_pair(func.name(), func));
            return;
        }

        if (tap_names.count(func.name()) > 0) {
            // visit a tap function, save path and tap function
            // then terminate
            for (const auto &f : path) kernel_funcs[f.name()] = f;
            tap_funcs.insert(make_pair(func.name(), func));
            return;
        }

        GetCallees visitor;
        func.accept(&visitor);
        map<string, Function> callees = visitor.funcs;
        callees.erase(func.name()); // in case the function is calling itself


        // Find tap params
        for (const auto &p : visitor.params) {
            // save path and current function
            for (const auto &f : path) kernel_funcs[f.name()] = f;
            kernel_funcs[func.name()] = func;
            tap_params.insert(p);
        }

        // Current function doesn't call any other functions, find a valid path
        if (callees.empty()) {
            if (visitor.params.empty()) {
                // TODO have special flag here
                user_warning << "Function " << func.name() << " can be statically evaluated. "
                             << "Consider ROM optimization by fully unrolling its dimension.\n";
            }
            for (const auto &f : path) kernel_funcs[f.name()] = f;
            kernel_funcs[func.name()] = func;
            return;
        }

        // DFS
        for (auto p : callees) {
            Function callee = p.second;
            path.push_back(func);
            dfs(callee, path);
            path.pop_back();
        }
    }
private:
    Function output_func;
    const set<string> &input_names;
    const set<string> &tap_names;
    map<string, Function> input_funcs;
    map<string, Function> kernel_funcs;
    map<string, Function> tap_funcs;
    map<string, Parameter> tap_params;
};

void mark_hw_kernels(Function output, const set<string> &inputs, const set<string> &taps) {
    MarkHWKernels(output, inputs, taps).mark();
}

}
}
