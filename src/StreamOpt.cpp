#include "StreamOpt.h"
#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Debug.h"
#include "Substitute.h"
#include "IRPrinter.h"
#include "Simplify.h"
#include "Bounds.h"

#include <iostream>
#include <algorithm>
using std::ostream;

namespace Halide {
namespace Internal {

using std::string;
using std::map;
using std::set;
using std::pair;
using std::vector;

namespace {

class ExpandExpr : public IRMutator {
    using IRMutator::visit;
    const Scope<Expr> &scope;

    void visit(const Variable *var) {
        if (scope.contains(var->name)) {
            expr = scope.get(var->name);
            debug(4) << "Fully expanded " << var->name << " -> " << expr << "\n";
        } else {
            expr = var;
        }
    }

public:
    ExpandExpr(const Scope<Expr> &s) : scope(s) {}

};

// Perform all the substitutions in a scope
Expr expand_expr(Expr e, const Scope<Expr> &scope) {
    ExpandExpr ee(scope);
    Expr result = ee.mutate(e);
    debug(4) << "Expanded " << e << " into " << result << "\n";
    return result;
}


}


class ReplaceReferencesWithStencil : public IRMutator {
    const HWKernel &kernel;
    const HWKernelDAG &dag;  // FIXME not needed
    Scope<Expr> scope;

    using IRMutator::visit;

    void visit(const For *op) {
        if (!starts_with(op->name, kernel.name)) {
            // try to simplify trivial reduction loops
            // TODO add assertions to check loop type
            Expr loop_extent = simplify(expand_expr(op->extent, scope));
            if (is_one(loop_extent)) {
                scope.push(op->name, simplify(expand_expr(op->min, scope)));
                Stmt body = mutate(op->body);
                scope.pop(op->name);
                stmt = LetStmt::make(op->name, op->min, body);
            } else {
                Stmt body = mutate(op->body);
                stmt = For::make(op->name, op->min, op->extent, op->for_type, op->device_api, body);
            }
        } else {
            // replace the loop var over the dimensions of the original function
            // realization with the loop var over the stencil dimension.
            // e.g. funcA.s0.x -> funcA.stencil.s0.x
            //      funcA.s1.x -> funcA.stencil.s1.x
            string old_var_name = op->name;
            string stage_dim_name = op->name.substr(kernel.name.size()+1, old_var_name.size() - kernel.name.size());
            string new_var_name = kernel.name + ".stencil." + stage_dim_name;
            Expr new_var = Variable::make(Int(32), new_var_name);

            // find the stencil dimension given dim_name
            int dim_idx = -1;
            for(size_t i = 0; i < kernel.func.args().size(); i++)
                if(ends_with(stage_dim_name, kernel.func.args()[i])) {
                    dim_idx = i;
                    break;
                }
            if (dim_idx == -1) {
                // it is a loop over reduction domain, and we keep it
                // TODO add an assertion
                IRMutator::visit(op);
                return;
            }
            Expr new_min = 0;
            Expr new_extent = kernel.dims[dim_idx].step;

            // create a let statement for the old_loop_var
            Expr old_min = op->min;
            Expr old_var_value = new_var + old_min;

            // traversal down into the body
            scope.push(old_var_name, simplify(expand_expr(old_var_value, scope)));
            Stmt new_body = mutate(op->body);
            scope.pop(old_var_name);

            new_body = LetStmt::make(old_var_name, old_var_value, new_body);
            stmt = For::make(new_var_name, new_min, new_extent, op->for_type, op->device_api, new_body);
        }
    }

    void visit(const Provide *op) {
        if(op->name != kernel.name) {
            IRMutator::visit(op);
        } else {
            // Replace the provide node of func with provide node of func.stencil
            string stencil_name = kernel.name + ".stencil";
            vector<Expr> new_args(op->args.size());

            // Replace the arguments. e.g.
            //   func.s0.x -> func.stencil.x
            for (size_t i = 0; i < kernel.func.args().size(); i++) {
                new_args[i] = simplify(expand_expr(mutate(op->args[i]) - kernel.dims[i].min_pos, scope));
            }

            vector<Expr> new_values(op->values.size());
            for (size_t i = 0; i < op->values.size(); i++) {
                new_values[i] = mutate(op->values[i]);
            }

            stmt = Provide::make(stencil_name, new_values, new_args);
        }
    }

    void visit(const Call *op) {
        if(op->name == kernel.name || // call to this kernel itself (in update definition)
           std::find(kernel.input_streams.begin(), kernel.input_streams.end(),
                     op->name) != kernel.input_streams.end() // call to a input stencil
           ) {
            // check assumptions
            internal_assert(op->call_type == Call::Halide);

            const auto it = dag.kernels.find(op->name);
            internal_assert(it != dag.kernels.end());
            const HWKernel &stencil_kernel = it->second;
            internal_assert(op->args.size() == stencil_kernel.func.args().size());

            // Replace the call node of func with call node of func.stencil
            string stencil_name = stencil_kernel.name + ".stencil";
            vector<Expr> new_args(op->args.size());

            // Mutate the arguments.
            // The value of the new argment is the old_value - stencil.min_pos.
            // The new value shouldn't refer to old loop vars any more
            for (size_t i = 0; i < op->args.size(); i++) {
                Expr old_arg = mutate(op->args[i]);
                Expr offset;
                if (stencil_kernel.name == kernel.name) {
                    // The call is in an update definition of the kernel itself
                    offset = stencil_kernel.dims[i].min_pos;
                } else {
                    // This is call to input stencil
                    // we use the min_pos stored in in_kernel.consumer_stencils
                    const auto it = stencil_kernel.consumer_stencils.find(kernel.name);
                    internal_assert(it != kernel.consumer_stencils.end());
                    offset = it->second[i].min_pos;
                }

                Expr new_arg = old_arg - offset;
                new_arg = simplify(expand_expr(new_arg, scope));
                // TODO check if the new_arg only depends on the loop vars
                // inside the producer
                new_args[i] = new_arg;
            }
            expr = Call::make(op->type, stencil_name, new_args, Call::Intrinsic);
            debug(4) << "replacing call " << Expr(op) << " with\n"
                     << "\t" << expr << "\n";
        } else {
            IRMutator::visit(op);
        }
    }

    void visit(const Realize *op) {
        // this must be a realize node of a inlined function
        internal_assert(dag.kernels.count(op->name));
        internal_assert(dag.kernels.find(op->name)->second.is_inlined);
        // expand and simplify bound expressions
        // TODO this may not be needed if the letstmt in the scope is well preserved

        Region new_bounds(op->bounds.size());
        bool bounds_changed = false;

        // Mutate the bounds
        for (size_t i = 0; i < op->bounds.size(); i++) {
            Expr old_min    = op->bounds[i].min;
            Expr old_extent = op->bounds[i].extent;
            Expr new_min    = simplify(expand_expr(mutate(op->bounds[i].min), scope));
            Expr new_extent = simplify(expand_expr(mutate(op->bounds[i].extent), scope));
            if (!new_min.same_as(old_min))       bounds_changed = true;
            if (!new_extent.same_as(old_extent)) bounds_changed = true;
            new_bounds[i] = Range(new_min, new_extent);
        }

        Stmt body = mutate(op->body);
        Expr condition = mutate(op->condition);
        if (!bounds_changed &&
            body.same_as(op->body) &&
            condition.same_as(op->condition)) {
            stmt = op;
        } else {
            stmt = Realize::make(op->name, op->types, new_bounds,
                                 condition, body);
        }
    }

    void visit(const Let *op) {
        Expr new_value = simplify(expand_expr(mutate(op->value), scope));
        scope.push(op->name, new_value);
        Expr new_body = mutate(op->body);
        if (new_value.same_as(op->value) &&
            new_body.same_as(op->body)) {
            expr = op;
        } else {
            expr = Let::make(op->name, new_value, new_body);
        }
        scope.pop(op->name);
    }

    void visit(const LetStmt *op) {
        Expr new_value = simplify(expand_expr(op->value, scope));
        scope.push(op->name, new_value);
        Stmt new_body = mutate(op->body);
        if (new_value.same_as(op->value) &&
            new_body.same_as(op->body)) {
            stmt = op;
        } else {
            stmt = LetStmt::make(op->name, new_value, new_body);
        }
        scope.pop(op->name);
    }

public:
    ReplaceReferencesWithStencil(const HWKernel &k, const HWKernelDAG &d,
                                 const Scope<Expr> *s = NULL)
        : kernel(k), dag(d) {
        scope.set_containing_scope(s);
    }
};


Stmt create_dispatch_call(const HWKernel& kernel, int min_fifo_depth = 0) {
    // dispatch the stream into seperate streams for each of its consumers
    // syntax:
    //   dispatch_stream(stream_name, num_of_dimensions,
    //                   stencil_size_dim_0, stencil_step_dim_0, store_extent_dim_0,
    //                   [stencil_size_dim_1, stencil_step_dim_1, store_extent_dim_1, ...]
    //                   num_of_consumers,
    //                   consumer_0_name, fifo_0_depth,
    //                   consumer_0_offset_dim_0, consumer_0_extent_dim_0,
    //                   [consumer_0_offset_dim_1, consumer_0_extent_dim_1, ...]
    //                   [consumer_1_name, ...])
    Expr stream_var = Variable::make(Handle(), kernel.name + ".stencil.stream");
    vector<Expr> dispatch_args({stream_var, (int)kernel.dims.size()});
    for (size_t i = 0; i < kernel.dims.size(); i++) {
        dispatch_args.push_back(kernel.dims[i].size);
        dispatch_args.push_back(kernel.dims[i].step);
        Expr store_extent = simplify(kernel.dims[i].store_bound.max -
                                     kernel.dims[i].store_bound.min + 1);
        internal_assert(is_const(store_extent));
        dispatch_args.push_back((int)*as_const_int(store_extent));
    }
    dispatch_args.push_back((int)kernel.consumer_stencils.size());
    for (const auto& p : kernel.consumer_stencils) {
        dispatch_args.push_back(p.first);
        internal_assert(kernel.consumer_fifo_depths.count(p.first));
        dispatch_args.push_back(std::max(min_fifo_depth, kernel.consumer_fifo_depths.find(p.first)->second));
        internal_assert(p.second.size() == kernel.dims.size());
        for (size_t i = 0; i < kernel.dims.size(); i++) {
            Expr store_offset = simplify(p.second[i].store_bound.min -
                                         kernel.dims[i].store_bound.min);
            Expr store_extent = simplify(p.second[i].store_bound.max -
                                         p.second[i].store_bound.min + 1);
            internal_assert(is_const(store_offset));
            internal_assert(is_const(store_extent));
            dispatch_args.push_back((int)*as_const_int(store_offset));
            dispatch_args.push_back((int)*as_const_int(store_extent));
        }
    }
    return Evaluate::make(Call::make(Handle(), "dispatch_stream", dispatch_args, Call::Intrinsic));
}


// Add realize and read_stream calls arround IR s
Stmt add_input_stencil(Stmt s, const HWKernel &kernel, const HWKernel &input) {
    string stencil_name = input.name + ".stencil";
    string stream_name = stencil_name + ".stream";
    Expr stream_var = Variable::make(Handle(), stream_name);
    Expr stencil_var = Variable::make(Handle(), stencil_name);

    // syntax for read_stream()
    // read_stream(src_stream, des_stencil, [consumer_name])
    vector<Expr> args({stream_var, stencil_var});
    if (input.name != kernel.name) {
        // for non-output kernel, we add an addition argument
        args.push_back(kernel.name);
    }
    Stmt read_call = Evaluate::make(Call::make(Handle(), "read_stream", args, Call::Intrinsic));
    Stmt pc = ProducerConsumer::make(stencil_name, read_call, Stmt(), s);

    // create a realizeation of the stencil image
    Region bounds;
    for (StencilDimSpecs dim: input.dims) {
        bounds.push_back(Range(0, dim.size));
    }
    s = Realize::make(stencil_name, input.func.output_types(), bounds, const_true(), pc);
    return s;
}

bool need_linebuffer(const HWKernel &kernel) {
    // check if we need a line buffer
    bool ret = false;
    for (size_t i = 0; i < kernel.dims.size(); i++) {
        if (kernel.dims[i].size != kernel.dims[i].step) {
            ret = true;
            break;
        }
    }
    return ret;
}

// IR for line buffers
// A line buffer is instantiated to buffer the stencil_update.stream and
// to generate the stencil.stream
// The former is smaller, which only consist of the new pixels
// sided in each shift of the stencil window.
Stmt add_linebuffer(Stmt s, const HWKernel &kernel) {
    Stmt ret;
    if (need_linebuffer(kernel)) {
        // Before mutation:
        //       stmt...
        //
        // After mutation:
        //       realize func.stencil.stream {
        //         linebuffer(...)
        //         dispatch(...)
        //         stmt...
        //       }
        string stream_name = kernel.name + ".stencil.stream";
        string update_stream_name = kernel.name + ".stencil_update.stream";
        Expr stream_var = Variable::make(Handle(), stream_name);
        Expr update_stream_var = Variable::make(Handle(), update_stream_name);

        vector<Expr> linebuffer_args({update_stream_var, stream_var});
        // extract the buffer size, and put it into args
        for (size_t i = 0; i < kernel.dims.size(); i++) {
            Expr store_extent = simplify(kernel.dims[i].store_bound.max -
                                         kernel.dims[i].store_bound.min + 1);
            linebuffer_args.push_back(store_extent);
        }
        Stmt linebuffer_call = Evaluate::make(Call::make(Handle(), "linebuffer", linebuffer_args, Call::Intrinsic));
        Stmt dispatch_call = create_dispatch_call(kernel);
        Stmt buffer_calls = Block::make(linebuffer_call, dispatch_call);

        // create a realization of the stencil of the window-size
        Region window_bounds;
        for (StencilDimSpecs dim: kernel.dims) {
            window_bounds.push_back(Range(0, dim.size));
        }
        ret = Realize::make(stream_name, kernel.func.output_types(), window_bounds, const_true(), Block::make(linebuffer_call, Block::make(dispatch_call, s)));
    } else {
        user_warning << "No linebuffer inserted after function " << kernel.name
                     << ".\n";

        // Before mutation:
        //       stmt...
        //
        // After mutation:
        //       dispatch(...)
        //       stmt...

        // Ad-hoc fix: place a fifo buffer at the inputs if the inputs is not linebuffered.
        // This works around the issue that read stencil call from a interface stream
        // in the compute kernel cannot fully unrolled
        int force_buffer_depth = 0;
        if (kernel.input_streams.empty() && kernel.consumer_stencils.size() == 1)
            force_buffer_depth = 1;
        Stmt dispatch_call = create_dispatch_call(kernel, force_buffer_depth);
        ret = Block::make(dispatch_call, s);
    }
    return ret;
}


Stmt transform_kernel(Stmt s, const HWKernelDAG &dag, const Scope<Expr> &scope) {
    Stmt ret;
    const ProducerConsumer *op = s.as<ProducerConsumer>();
    if (op) {
        const HWKernel &kernel = dag.kernels.find(op->name)->second;
        internal_assert(!kernel.is_output);
        if (kernel.is_inlined) {
            // if it is a function inlined into the output function,
            // skip transforming this funciton

            // FIXME it is buggy as the inlined function should really
            // nested in the scan loops of the output function
            internal_error;
        }

        // Before mutation:
        //    produce func {...}
        //    consume func
        //
        // After mutation:
        //     realize func.stencil_update.stream {
        //       produce func.stencil_update.stream {
        //         for scan_loops {
        //           realize input1.stencil {
        //             produce input1.stencil {
        //               read_stream(input1.stencil.stream, input1.stencil)
        //             }
        //             ...
        //             realize func.stencil {
        //               produce func.stencil {...}
        //               write_stream(func.stencil_update.stream, func.stencil)
        //       } } } }
        //       realize func.stencil.stream {
        //         linebuffer(...)
        //         dispatch(...)
        //         consume func.stencil.stream
        //       }
        //     }
        string stencil_name = kernel.name + ".stencil";
        string stream_name = need_linebuffer(kernel) ?
            kernel.name + ".stencil_update.stream" : kernel.name + ".stencil.stream";
        Expr stencil_var = Variable::make(Handle(), stencil_name);
        Expr stream_var = Variable::make(Handle(), stream_name);

        // replacing the references to the original realization with refences to stencils
        Stmt produce = ReplaceReferencesWithStencil(kernel, dag, &scope).mutate(op->produce);
        Stmt update = ReplaceReferencesWithStencil(kernel, dag, &scope).mutate(op->update);

        // syntax for write_stream()
        // write_stream(des_stream, src_stencil)
        vector<Expr> write_args({stream_var, stencil_var});
        Stmt write_call = Evaluate::make(Call::make(Handle(), "write_stream", write_args, Call::Intrinsic));

        // realize and PC node for func.stencil
        Stmt stencil_pc = ProducerConsumer::make(stencil_name, produce, update, write_call);

        // create a realization of the stencil of the step-size
        Region step_bounds;
        for (StencilDimSpecs dim: kernel.dims) {
            step_bounds.push_back(Range(0, dim.step));
        }
        Stmt stencil_realize = Realize::make(stencil_name, kernel.func.output_types(), step_bounds, const_true(), stencil_pc);

        // add read_stream for each input stencil (producers fed to func)
        for (const string& s : kernel.input_streams) {
            const auto it = dag.kernels.find(s);
            internal_assert(it != dag.kernels.end());
            stencil_realize = add_input_stencil(stencil_realize, kernel, it->second);
        }

        // insert scan loops
        Stmt scan_loops = stencil_realize;
        int scan_dim = 0;
        for(size_t i = 0; i < kernel.dims.size(); i++) {
            if (kernel.dims[i].loop_var == "undef" )
                continue;

            string loop_var_name = kernel.name + "." + kernel.func.args()[i]
                + ".__scan_dim_" + std::to_string(scan_dim++);

            Expr store_extent = simplify(kernel.dims[i].store_bound.max -
                                         kernel.dims[i].store_bound.min + 1);
            debug(3) << "kernel " << kernel.name << " store_extent = " << store_extent << '\n';

            // check the condition for the new loop for sliding the update stencil
            const IntImm *store_extent_int = store_extent.as<IntImm>();
            internal_assert(store_extent_int);
            if (store_extent_int->value % kernel.dims[i].step != 0) {
                // we cannot handle this scenario yet
                internal_error
                    << "Line buffer extent (" << store_extent_int->value
                    << ") is not divisible by the stencil step " << kernel.dims[i].step << '\n';
            }
            int loop_extent = store_extent_int->value / kernel.dims[i].step;

            // add letstmt to connect old loop var to new loop var_name
            // FIXME this is not correct in general
            scan_loops = LetStmt::make(kernel.dims[i].loop_var, Variable::make(Int(32), loop_var_name), scan_loops);
            scan_loops = For::make(loop_var_name, 0, loop_extent, ForType::Serial, DeviceAPI::Host, scan_loops);
        }

        // Recurse
        Stmt stream_consume = transform_kernel(op->consume, dag, scope);

        // Add line buffer and dispatcher
        Stmt stream_realize = add_linebuffer(stream_consume, kernel);

        // create the PC node for update stream
        Stmt stream_pc = ProducerConsumer::make(stream_name, scan_loops, Stmt(), stream_realize);

        // create a realizeation of the stencil stream
        ret = Realize::make(stream_name, kernel.func.output_types(), step_bounds, const_true(), stream_pc);
    } else {
        // this is the output kernel of the dag
        const HWKernel &kernel = dag.kernels.find(dag.name)->second;
        internal_assert(kernel.is_output);

        string stencil_name = kernel.name + ".stencil";
        string stream_name = kernel.name + ".stencil.stream";
        Expr stream_var = Variable::make(Handle(), stream_name);
        Expr stencil_var = Variable::make(Handle(), stencil_name);

        // replacing the references to the original realization with refences to stencils
        Stmt produce = ReplaceReferencesWithStencil(kernel, dag, &scope).mutate(s);

        // syntax for write_stream()
        // write_stream(des_stream, src_stencil)
        vector<Expr> write_args({stream_var, stencil_var});
        // for dag output kernel, we want to record the scan loop vars,
        // so that code gen knows when to assert TLAST signal
        int scan_dim = 0;
        for (size_t i = 0; i < kernel.dims.size(); i++) {
            if (kernel.dims[i].loop_var != "undef") {
                string loop_var_name = kernel.name + "." + kernel.func.args()[i]
                    + ".__scan_dim_" + std::to_string(scan_dim++);
                Expr store_extent = simplify(kernel.dims[i].store_bound.max -
                                             kernel.dims[i].store_bound.min + 1);
                const IntImm *store_extent_int = store_extent.as<IntImm>();
                internal_assert(store_extent_int);
                int loop_extent = store_extent_int->value / kernel.dims[i].step;

                Expr loop_var = Variable::make(Int(32), loop_var_name);
                Expr loop_max = make_const(Int(32), loop_extent - 1);
                write_args.push_back(loop_var);
                write_args.push_back(loop_max);
            }
        }
        Stmt write_call = Evaluate::make(Call::make(Handle(), "write_stream", write_args, Call::Intrinsic));

        Stmt stencil_pc = ProducerConsumer::make(stencil_name, produce, Stmt(), write_call);
        // create a realization of the stencil image
        Region bounds;
        for (StencilDimSpecs dim: kernel.dims) {
            bounds.push_back(Range(0, dim.step));
        }
        Stmt stencil_realize = Realize::make(stencil_name, kernel.func.output_types(), bounds, const_true(), stencil_pc);

        // add read_stream for each input stencil (producers fed to func)
        for (const string& s : kernel.input_streams) {
            const auto it = dag.kernels.find(s);
            internal_assert(it != dag.kernels.end());
            stencil_realize = add_input_stencil(stencil_realize, kernel, it->second);
        }

        // insert scan loops
        Stmt scan_loops = stencil_realize;
        scan_dim = 0;
        for(size_t i = 0; i < kernel.dims.size(); i++) {
            if (kernel.dims[i].loop_var == "undef" )
                continue;

            string loop_var_name = kernel.name + "." + kernel.func.args()[i]
                + ".__scan_dim_" + std::to_string(scan_dim++);

            Expr store_extent = simplify(kernel.dims[i].store_bound.max -
                                         kernel.dims[i].store_bound.min + 1);
            debug(3) << "kernel " << kernel.name << " store_extent = " << store_extent << '\n';

            // check the condition for the new loop for sliding the update stencil
            const IntImm *store_extent_int = store_extent.as<IntImm>();
            internal_assert(store_extent_int);
            if (store_extent_int->value % kernel.dims[i].step != 0) {
                // we cannot handle this scenario yet
                internal_error
                    << "Line buffer extent (" << store_extent_int->value
                    << ") is not divisible by the stencil step " << kernel.dims[i].step << '\n';
            }
            int loop_extent = store_extent_int->value / kernel.dims[i].step;

            // add letstmt to connect old loop var to new loop var_name
            // FIXME this is not correct in general
            scan_loops = LetStmt::make(kernel.dims[i].loop_var, Variable::make(Int(32), loop_var_name), scan_loops);
            scan_loops = For::make(loop_var_name, 0, loop_extent, ForType::Serial, DeviceAPI::Host, scan_loops);
        }

        ret = ProducerConsumer::make(stream_name, scan_loops, Stmt(), Evaluate::make(0));
    }
    return ret;
}


class TransformTapStencils : public IRMutator {
    const map<string, HWTap> &taps;

    using IRMutator::visit;

    // Replace calls to ImageParam with calls to Stencil
    void visit(const Call *op) {
        if (taps.count(op->name) == 0) {
            IRMutator::visit(op);
        } else if (op->call_type == Call::Image || op->call_type == Call::Halide) {
            debug(3) << "replacing " << op->name << '\n';
            const HWTap &tap = taps.find(op->name)->second;

            // Replace the call node of func with call node of func.tap.stencil
            string stencil_name = op->name + ".tap.stencil";
            vector<Expr> new_args(op->args.size());

            // Mutate the arguments.
            // The value of the new argment is the old_value - min_pos
            // b/c stencil indices always start from zero
            internal_assert(tap.dims.size() == op->args.size());
            for (size_t i = 0; i < op->args.size(); i++) {
                 new_args[i] = op->args[i]- tap.dims[i].min_pos;
            }
            expr = Call::make(op->type, stencil_name, new_args, Call::Intrinsic);
        } else {
            internal_error << "unexpected call_type\n";
        }
    }

public:
    TransformTapStencils(const map<string, HWTap> &t) : taps(t) {}
};

// Perform streaming optimization for all functions
class StreamOpt : public IRMutator {
    const HWKernelDAG &dag;
    Scope<Expr> scope;

    using IRMutator::visit;

    void visit(const For *op) {
        if (!dag.store_level.match(op->name) && !dag.loop_vars.count(op->name)) {
            IRMutator::visit(op);
        } else if (dag.compute_level.match(op->name)) {
            internal_assert(dag.loop_vars.count(op->name));

            // walk inside of any let statements
            Stmt body = op->body;

            vector<pair<string, Expr>> lets;
            while (const LetStmt *let = body.as<LetStmt>()) {
                body = let->body;
                scope.push(let->name, simplify(expand_expr(let->value, scope)));
                lets.push_back(make_pair(let->name, let->value));
            }

            Stmt new_body = transform_kernel(body, dag, scope);

            // insert line buffers for input streams
            for (const string &kernel_name : dag.input_kernels) {
                const HWKernel &input_kernel = dag.kernels.find(kernel_name)->second;
                new_body = add_linebuffer(new_body, input_kernel);
            }

            // Rewrap the let statements
            for (size_t i = lets.size(); i > 0; i--) {
                new_body = LetStmt::make(lets[i-1].first, lets[i-1].second, new_body);
            }

            // remove the loop statement if it is one of the scan loops
            stmt = new_body;
        } else if (dag.loop_vars.count(op->name)){
            // remove the loop statement if it is one of the scan loops
            stmt = mutate(op->body);
        } else {
            internal_assert(dag.store_level.match(op->name));
            debug(3) << "find the pipeline producing " << dag.name << "\n";

            // walk inside of any let statements
            Stmt body = op->body;

            vector<pair<string, Expr>> lets;
            while (const LetStmt *let = body.as<LetStmt>()) {
                body = let->body;
                scope.push(let->name, simplify(expand_expr(let->value, scope)));
                lets.push_back(make_pair(let->name, let->value));
            }

            Stmt new_body = mutate(body);

            //stmt = For::make(dag.name + ".accelerator", 0, 1, ForType::Serial, DeviceAPI::Host, body);
            new_body = ProducerConsumer::make("_hls_target." + dag.name, new_body, Stmt(), Evaluate::make(0));

            // add declarations of inputs and output (external) streams outside the hardware pipeline IR
            vector<string> external_streams;
            external_streams.push_back(dag.name);
            external_streams.insert(external_streams.end(), dag.input_kernels.begin(), dag.input_kernels.end());
            for (const string &name : external_streams) {
                const HWKernel kernel = dag.kernels.find(name)->second;
                string stream_name = need_linebuffer(kernel) ?
                    kernel.name + ".stencil_update.stream" : kernel.name + ".stencil.stream";

                string direction = kernel.is_output ? "stream_to_buffer" : "buffer_to_stream";
                Expr stream_var = Variable::make(Handle(), stream_name);

                // derive the coordinate of the sub-image block
                internal_assert(kernel.func.output_types().size() == 1);
                vector<Expr> image_args;
                for (size_t i = 0; i < kernel.dims.size(); i++) {
                    image_args.push_back(kernel.dims[i].store_bound.min);
                }
                Expr address_of_subimage_origin = Call::make(Handle(), Call::address_of, {Call::make(kernel.func, image_args, 0)}, Call::Intrinsic);
                Expr buffer_var = Variable::make(type_of<struct buffer_t *>(), kernel.name + ".buffer");

                // add intrinsic functions to convert memory buffers to streams
                // syntax:
                //   stream_subimage(direction, buffer_var, stream_var, address_of_subimage_origin,
                //                   dim_0_stride, dim_0_extent, ...)
                vector<Expr> stream_call_args({direction, buffer_var, stream_var, address_of_subimage_origin});
                for (size_t i = 0; i < kernel.dims.size(); i++) {
                    stream_call_args.push_back(Variable::make(Int(32), kernel.name + ".stride." + std::to_string(i)));
                    stream_call_args.push_back(simplify(kernel.dims[i].store_bound.max - kernel.dims[i].store_bound.min + 1));
                }
                Stmt stream_subimg = Evaluate::make(Call::make(Handle(), "stream_subimage", stream_call_args, Call::Intrinsic));

                Region bounds;
                for (StencilDimSpecs dim: kernel.dims) {
                    bounds.push_back(Range(0, dim.step));
                }
                new_body = Realize::make(stream_name, kernel.func.output_types(), bounds, const_true(), Block::make(stream_subimg, new_body));
            }

            // Handle tap values
            new_body = TransformTapStencils(dag.taps).mutate(new_body);

            // Declare and initialize tap stencils with buffers
            // TODO move this call out side the tile loops over the kernel launch
            for (const auto &p : dag.taps) {
                const HWTap &tap = p.second;
                const string stencil_name = tap.name + ".tap.stencil";

                Expr buffer_var = Variable::make(type_of<struct buffer_t *>(), tap.name + ".buffer");
                Expr stencil_var = Variable::make(Handle(), stencil_name);
                vector<Expr> args({buffer_var, stencil_var});
                Stmt convert_call = Evaluate::make(Call::make(Handle(), "buffer_to_stencil", args, Call::Intrinsic));

                // create a realizeation of the stencil
                Region bounds;
                for (const auto &dim : tap.dims) {
                    bounds.push_back(Range(0, dim.size));
                }
                vector<Type> types = tap.is_func ? tap.func.output_types() :
                    vector<Type>({tap.param.type()});
                new_body = Realize::make(stencil_name, types, bounds, const_true(), Block::make(convert_call, new_body));
            }

            // Rewrap the let statements
            for (size_t i = lets.size(); i > 0; i--) {
                new_body = LetStmt::make(lets[i-1].first, lets[i-1].second, new_body);
            }
            stmt = For::make(op->name, op->min, op->extent, op->for_type, op->device_api, new_body);
        }
    }

    // Remove the realize node for intermediate functions in the hardware pipeline,
    // as we will create linebuffers in the pipeline to hold these values
    void visit(const Realize *op) {
        if (dag.kernels.count(op->name) &&
            !dag.kernels.find(op->name)->second.is_inlined && // is a linebuffered function
            dag.name != op->name && // not the output
            dag.input_kernels.count(op->name) == 0 // not a input
            ) {
            stmt = mutate(op->body);
            // check constraints
            /*
            for (size_t i = 0; i < kernel.dims.size(); i++) {
                Expr store_extent = simplify(kernel.dims[i].store_bound.max -
                                             kernel.dims[i].store_bound.min + 1);
                if (!is_zero(simplify(op->bounds[i].extent - store_extent))) {
                    user_error << "linebuffer (" << store_extent << ") for " << op->name
                               << " is not equal to realize extent (" << op->bounds[i].extent << ").\n";
                }
            }
            */
        } else {
            IRMutator::visit(op);
        }
    }

    void visit(const LetStmt *op) {
        Expr new_value = simplify(expand_expr(op->value, scope));
        scope.push(op->name, new_value);
        Stmt new_body = mutate(op->body);
        if (new_value.same_as(op->value) &&
            new_body.same_as(op->body)) {
            stmt = op;
        } else {
            stmt = LetStmt::make(op->name, new_value, new_body);
        }
        scope.pop(op->name);
    }

public:
    StreamOpt(const HWKernelDAG &d)
        : dag(d) {}
};

Stmt stream_opt(Stmt s, const HWKernelDAG &dag) {
    debug(3) << s << "\n";
    s = StreamOpt(dag).mutate(s);
    debug(3) << s << "\n";
    return s;
}

}
}
