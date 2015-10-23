#include "ReplaceImageParam.h"

#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Debug.h"

namespace Halide {
namespace Internal {

using std::string;
using std::map;
using std::vector;

class ReplaceImageParamForFunction : public IRMutator {
    using IRMutator::visit;

    // Replace calls to ImageParam with calls to Stencil
    void visit(const Call *op) {
        if( op->call_type != Call::Image ||
            !op->param.defined()) {
            IRMutator::visit(op);
        } else {
            debug(3) << "replacing " << op->name << '\n';
            internal_assert(op->param.is_buffer());
            internal_assert((size_t)op->param.dimensions() == op->args.size());
            internal_assert(op->name == op->param.name());

            if(params.count(op->name) == 0)
                params[op->name] = op->param;

            // Replace the call node of func with call node of func.stencil
            string stencil_name = op->name + ".stencil";
            vector<Expr> new_args(op->args.size());

            // Mutate the arguments.
            // The value of the new argment is the old_value - param.min_constraint()
            // b/c stencil indices always start from zero
            for (size_t i = 0; i < op->args.size(); i++) {
                Expr old_arg = op->args[i];
                Expr new_arg = old_arg - op->param.min_constraint(i);
                new_args[i] = new_arg;
            }
            expr = Call::make(op->type, stencil_name, new_args, Call::Intrinsic);
        }
    }

public:
    map<string, Parameter> params;
    ReplaceImageParamForFunction() {}
};

class ReplaceImageParam : public IRMutator {
    const HWKernelDAG &dag;

    using IRMutator::visit;

    void visit(const ProducerConsumer *op) {
        if (!starts_with(op->name, "_hls_target." + dag.name)) {
            IRMutator::visit(op);
            return;
        }

        debug(2) << "find a target function " << dag.name << '\n';
        // TODO check if the accelerator schedule is valid.
        // for example if the pipeline input specified in the schedule are valid,
        // or if the storage of the input, output, and intermediates are valid.

        internal_assert(!op->update.defined());
        ReplaceImageParamForFunction replacer;
        Stmt new_produce = replacer.mutate(op->produce);

        if (new_produce.same_as(op->produce)) {
            internal_assert(replacer.params.empty());
            stmt = op;
        } else {
            stmt = ProducerConsumer::make(op->name, new_produce, Stmt(), op->consume);

            // create the realizations of stencil type of replaced imageparams
            for(const auto &pair : replacer.params) {
                const Parameter param = pair.second;
                const string stencil_name = param.name() + ".stencil";

                Expr buffer_var = Variable::make(Handle(), param.name());
                Expr stencil_var = Variable::make(Handle(), stencil_name);
                vector<Expr> args({buffer_var, stencil_var});
                Stmt convert_call = Evaluate::make(Call::make(Handle(), "buffer_to_stencil", args, Call::Intrinsic));
                stmt = ProducerConsumer::make(stencil_name, convert_call, Stmt(), stmt);

                // create a realizeation of the stencil image
                Region bounds;
                for (int i = 0; i < param.dimensions(); i++) {
                    Expr extent = param.extent_constraint(i);
                    user_assert(is_const(extent)) << "ImageParam used by hardware pipeline must set constant extents.";
                    bounds.push_back(Range(0, extent));
                }
                stmt = Realize::make(stencil_name, {param.type()}, bounds, const_true(), stmt);
            }
        }

    }
public:
    ReplaceImageParam(const HWKernelDAG &d) : dag(d) {}
};

Stmt replace_image_param(Stmt s, const HWKernelDAG &dag) {
    s = ReplaceImageParam(dag).mutate(s);
    return s;
}

}
}
