#include "StreamOpt.h"
#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Debug.h"
#include "Substitute.h"
#include "IRPrinter.h"
#include "Simplify.h"
#include "Derivative.h"
#include "Bounds.h"

#include <iostream>
#include <algorithm>
using std::ostream;

namespace Halide {
namespace Internal {

using std::string;
using std::map;
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


// create a stencil object for a function, which holds the required
// values for its consumers. And replace the references to the function
// image buffer (Provide and Call nodes) with references to the stencil object
class CreateStencilForFunction : public IRMutator {
    const HWKernel &kernel;
    const HWKernelDAG &dag;
    const Scope<Expr> &outer_scope;  // FIXME do we need this?
    Scope<Expr> scope;
    string in_kernel; // the hardware kernel currently in

    using IRMutator::visit;

    void visit(const ProducerConsumer *op) {
        if (op->name == kernel.name) {
            // traverse into produce and consume node of the func,
            // replace the PC node name with the stencil name, and
            // create a realize node for the stencil object
            string stencil_name = kernel.name + ".stencil";

            internal_assert(in_kernel == "");
            in_kernel = op->name;
            Stmt produce = mutate(op->produce);
            Stmt update = mutate(op->update);
            in_kernel = "";

            Stmt consume = mutate(op->consume);

            Stmt pc = ProducerConsumer::make(stencil_name, produce, update, consume);

            // create a realizeation of the stencil image
            Region bounds;
            for (StencilDimSpecs dim: kernel.dims) {
                bounds.push_back(Range(0, dim.size));
            }
            stmt = Realize::make(stencil_name, kernel.func.output_types(), bounds, const_true(), pc);
        } else {
            string kernel_name = op->name;
            // erase the ".stencil" suffix
            if(ends_with(kernel_name, ".stencil")){
                kernel_name = kernel_name.substr(0, kernel_name.size() - 8);
            }
            const auto it = dag.kernels.find(kernel_name);
            if (it != dag.kernels.end() &&
                !it->second.is_inlined) {
                // if we traverse into a produce node of non-inlined kernel, record the name
                internal_assert(in_kernel == "");
                in_kernel = kernel_name;
                Stmt produce = mutate(op->produce);
                Stmt update = mutate(op->update);
                in_kernel = "";

                Stmt consume = mutate(op->consume);
                if (produce.same_as(op->produce) &&
                    update.same_as(op->update) &&
                    consume.same_as(op->consume)) {
                    stmt = op;
                } else {
                    stmt = ProducerConsumer::make(op->name, produce, update, consume);
                }
            } else {
                IRMutator::visit(op);
            }
        }
    }


    void visit(const For *op) {
        if (!starts_with(op->name, kernel.name)) {
            IRMutator::visit(op);
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
            Expr new_extent = kernel.dims[dim_idx].size;

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
                new_args[i] = simplify(expand_expr(op->args[i] - kernel.dims[i].min_pos, scope));
            }

            vector<Expr> new_values(op->values.size());
            for (size_t i = 0; i < op->values.size(); i++) {
                new_values[i] = mutate(op->values[i]);
            }

            stmt = Provide::make(stencil_name, new_values, new_args);
        }
    }

    void visit(const Call *op) {
        if(op->name == kernel.name) {
            // check assumptions
            internal_assert(op->func.same_as(kernel.func));
            internal_assert(op->call_type == Call::Halide);
            internal_assert(op->args.size() == kernel.func.args().size());

            // Replace the call node of func with call node of func.stencil
            string stencil_name = kernel.name + ".stencil";
            vector<Expr> new_args(op->args.size());

            // Mutate the arguments.
            // The value of the new argment is the old_value - stencil.min_pos.
            // The new value shouldn't refer to old loop vars any more
            for (size_t i = 0; i < op->args.size(); i++) {
                Expr old_arg = mutate(op->args[i]);
                Expr offset;
                if (kernel.name == in_kernel ||
                    kernel.is_output) {
                    // The call is in an update definition of the kernel itself,
                    // or it is the output kernel
                    offset = kernel.dims[i].min_pos;
                } else if (kernel.consumer_stencils.size() > 0) {
                    // This is call to kernel in the expr of other kernels (in_kernel),
                    // which is the normal cas.
                    // we use the min_pos stored in in_kernel.consumer_stencils
                    const auto it = kernel.consumer_stencils.find(in_kernel);
                    internal_assert(it != kernel.consumer_stencils.end())
                        << "Cannot find consumer stencil of " << in_kernel
                        << " in kernel " << kernel.name << "\n";
                    offset = it->second[i].min_pos;
                }
                internal_assert(offset.defined());

                Expr new_arg = old_arg - offset;
                new_arg = simplify(expand_expr(new_arg, scope));
                // TODO check if the new_arg only depends on the loop vars
                // inside the producer
                new_args[i] = new_arg;
            }
            debug(3) << "replacing call " << Expr(op) << " with\n";
            expr = Call::make(op->type, stencil_name, new_args, Call::Intrinsic);
            debug(3) << "  " << expr << "\n";
        } else if (ends_with(op->name, ".stencil")) {
            // if it is a call to a stencil, we try to simplify the args again
            vector<Expr > new_args(op->args.size());
            bool changed = false;

            // simplify the args. apply aggressive expansion and simplification on expr
            for (size_t i = 0; i < op->args.size(); i++) {
                Expr old_arg = mutate(op->args[i]);
                Expr new_arg = simplify(expand_expr(old_arg, scope));
                if (!new_arg.same_as(op->args[i])) changed = true;
                new_args[i] = new_arg;
            }

            if (!changed) {
                expr = op;
            } else {
                debug(3) << "simplify call " << Expr(op) << " to\n";
                expr = Call::make(op->type, op->name, new_args, op->call_type,
                                  op->func, op->value_index, op->image, op->param);
                debug(3) << "  " << expr << "\n";
            }
        } else {
            IRMutator::visit(op);
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
        scope.push(op->name, simplify(expand_expr(op->value, scope)));
        Stmt new_body = mutate(op->body);
        if (new_body.same_as(op->body)) {
            stmt = op;
        } else {
            stmt = LetStmt::make(op->name, op->value, new_body);
        }
        scope.pop(op->name);
    }

public:
    CreateStencilForFunction(const HWKernel &k, const HWKernelDAG &d, const Scope<Expr> &s)
        : kernel(k), dag(d), outer_scope(s) {}
};


// Replace the stencil PC node with stencil.stream PC node
// add calls to read/write the stencil.stream.
class AddStreamOperationForFunction : public IRMutator {
    const HWKernel &kernel;
    const HWKernelDAG &dag;

    using IRMutator::visit;

    // Add realize and read_stream calls arround IR s
    Stmt add_input_stencil(Stmt s, const HWKernel &input) {
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

    // At the realize node of kernel stencil, we create a realize of the kernel stencil stream,
    // add the write_stream call for it, and add read_stream call for all its inputs
    void visit(const Realize * op) {
        string stencil_name = kernel.name + ".stencil";
        if (op->name != stencil_name) {
            IRMutator::visit(op);
        } else {
            // Before mutation:
            //     realize func.stencil {
            //       produce func.stencil {...}
            //       consume func.stencil
            //
            // After mutation:
            //     realize func.stencil.stream {
            //       produce func.stencil.stream {
            //         realize input1.stencil {
            //           produce input1.stencil {
            //             read_stream(input1.stencil.stream, input1.stencil)
            //           }
            //           ...
            //           realize func.stencil {
            //             produce func.stencil {...}
            //             write_stream(func.stencil.stream, func.stencil)
            //       } } }
            //       consume func.stencil
            //     }

            // check if the body of Realize node is a PC node
            const ProducerConsumer *pc = op->body.as<ProducerConsumer>();
            internal_assert(pc && pc->name == stencil_name);

            // create the write stream call
            string stream_name = kernel.name + ".stencil.stream";
            Expr stream_var = Variable::make(Handle(), stream_name);
            Expr stencil_var = Variable::make(Handle(), stencil_name);

            // syntax for write_stream()
            // write_stream(des_stream, src_stencil)
            vector<Expr> write_args({stream_var, stencil_var});
            Stmt write_call = Evaluate::make(Call::make(Handle(), "write_stream", write_args, Call::Intrinsic));

            // realize and PC node for func.stencil
            Stmt stencil_pc = ProducerConsumer::make(stencil_name, pc->produce, pc->update, write_call);
            Stmt stencil_realize = Realize::make(stencil_name, op->types, op->bounds, op->condition, stencil_pc);

            // add read_stream for each input stencil (producers fed to func)
            for (const string& s : kernel.input_streams) {
                const auto it = dag.kernels.find(s);
                internal_assert(it != dag.kernels.end());
                stencil_realize = add_input_stencil(stencil_realize, it->second);
            }

            Stmt stream_consume = pc->consume;
            if (kernel.is_output) {
                // Adhoc for the dag output kernel
                // we need to insert read_stream for the dag here
                // because it doesn't have any consumer
                stream_consume = add_input_stencil(stream_consume, kernel);
            }

            // create the PC node for stream
            Stmt stream_pc = ProducerConsumer::make(stream_name, stencil_realize, Stmt(), stream_consume);

            // create a realizeation of the stencil stream
            Region bounds;
            for (StencilDimSpecs dim: kernel.dims) {
                bounds.push_back(Range(0, dim.size));
            }
            stmt = Realize::make(stream_name, kernel.func.output_types(), bounds, const_true(), stream_pc);
        }
    }

public:
    AddStreamOperationForFunction(const HWKernel &k, const HWKernelDAG &d) : kernel(k), dag(d) {}
};


/** Search a certain type of IR node with the name.
 */
template <typename IR_NODE_TYPE>
class SearchIRNode : public IRVisitor {
    string name_;
    bool found_;

    using IRVisitor::visit;

    virtual void visit(const IR_NODE_TYPE *op) {
        if (op->name == name_) {
            found_ = true;
        } else {
            IRVisitor::visit(op);
        }
    }

public:
    bool search(Stmt stmt, string name) {
        name_ = name;
        found_ = false;
        if(stmt.defined()) {
            stmt.accept(this);
        }
        return found_;
    }
};


/** This class implimenets a transformation on IR which pulls
 *  the Realize and ProducerConsumer nodes for a target stream type
 *  towards the root level.
 */
class PullUpTargetNode : public IRMutator {
    string target;
    SearchIRNode<ProducerConsumer> searcher_;

    using IRMutator::visit;

    // check whether the target realize and PC is at root of IR
    bool is_target(Stmt s) {
        const Realize *realize = s.as<Realize>();
        if(realize && realize->name == target) {
            const ProducerConsumer *pc = realize->body.as<ProducerConsumer>();
            if(pc && pc->name == target)
                return true;
        }
        return false;
    }

    void visit(const Realize *op) {
        debug(3) << "visit Realize node " << op->name << '\n';

        if (op->name == target) {
            // find the target
            internal_assert(is_target(op));
            stmt = op;
        } else {
            internal_assert(ends_with(op->name, ".stream") || ends_with(op->name, ".stencil"))
                << "we don't expect to visit non-stream realize node.\n";
            const ProducerConsumer *body_pc = op->body.as<ProducerConsumer>();
            internal_assert(body_pc && body_pc->name == op->name)
                << "realize of a stream should followed by the PC of the stream.\n";

            Stmt body_before_swap = mutate(op->body);

            internal_assert(is_target(body_before_swap));
            const Realize *realize_A = body_before_swap.as<Realize>();
            const ProducerConsumer *pc_A = realize_A->body.as<ProducerConsumer>();
            internal_assert(!pc_A->update.defined()); // cannot handle this case for now

            /* After pull the target to the body, there might be two cases:
             * the old body (PC B) is pushed down either into produce of PC A
             * or into consume of PC A.
             * Next, we need to push the current realize node (realize B) into
             * PC A along the same direction.
             *
             * case1: before swap             after swap
             *        realize B                  realize A
             *            |                          |
             *        realize A                    PC A
             *            |                        /   \
             *          PC A               realize B   consume A
             *          /  \                    |
             *       PC B consume A           PC B
             *
             * case2: before swap             after swap
             *        realize B                  realize A
             *            |                          |
             *        realize A                    PC A
             *            |                        /   \
             *          PC A               produce A  realize B
             *          /  \                             |
             *   produce A  PC B                       PC B
             */
            bool in_produce = searcher_.search(pc_A->produce, op->name);

            if (in_produce) {
                // case 1
                const ProducerConsumer *pc_B = pc_A->produce.as<ProducerConsumer>();
                internal_assert(pc_B && pc_B->name == op->name);
                Stmt new_realize_B = Realize::make(op->name, op->types, op->bounds, op->condition, pc_A->produce);
                Stmt new_pc_A = ProducerConsumer::make(target, new_realize_B, Stmt(), pc_A->consume);

                stmt = Realize::make(target, realize_A->types, realize_A->bounds, realize_A->condition, new_pc_A);
            } else {
                // case 2
                const ProducerConsumer *pc_B = pc_A->consume.as<ProducerConsumer>();
                internal_assert(pc_B && pc_B->name == op->name);
                Stmt new_realize_B = Realize::make(op->name, op->types, op->bounds, op->condition, pc_A->consume);
                Stmt new_pc_A = ProducerConsumer::make(target, pc_A->produce, Stmt(), new_realize_B);

                stmt = Realize::make(target, realize_A->types, realize_A->bounds, realize_A->condition, new_pc_A);
            }
        }
    }

    void visit(const ProducerConsumer *op) {
        debug(3) << "visit PC node " << op->name << '\n';

        bool in_produce = searcher_.search(op->produce, target);
        bool in_consume = searcher_.search(op->consume, target);

        internal_assert((in_produce && !in_consume) || (!in_produce && in_consume))
            << "the target should be either in produce or in consume but in not both.\n";

        /* Swap the current PC (B) and the target PC (A).
         * case1: before swap             after swap
         *          PC B                     realize A
         *         /    \                        |
         *   realize A  consume B              PC A
         *        |                            /  \
         *       PC A                  produce A  PC B
         *       /   \                             / \
         * produce A  consume A            comsume A  consume B
         *
         * case2: before swap             after swap
         *          PC B                     realize A
         *         /    \                        |
         *   produce B realize A                PC A
         *               |                      /  \
         *              PC A                 PC B  consume A
         *              / \                   / \
         *      produce A  consume A   produce B produce A
         */
        if (in_produce) {
            // case 1

            // pull target up to the root of the current produce
            Stmt produce_before_swap = mutate(op->produce);

            internal_assert(is_target(produce_before_swap));
            const Realize *realize = produce_before_swap.as<Realize>();
            const ProducerConsumer *pc_A = realize->body.as<ProducerConsumer>();
            internal_assert(!pc_A->update.defined()); // cannot handle this case for now

            Stmt new_pc_B = ProducerConsumer::make(op->name, pc_A->consume, op->update, op->consume);
            Stmt new_pc_A = ProducerConsumer::make(target, pc_A->produce, Stmt(), new_pc_B);

            stmt = Realize::make(target, realize->types, realize->bounds, realize->condition, new_pc_A);
        } else {
            // case 2

            // pull target up to the root of the current consumer
            Stmt consume_before_swap = mutate(op->consume);

            internal_assert(is_target(consume_before_swap));
            const Realize *realize = consume_before_swap.as<Realize>();
            const ProducerConsumer *pc_A = realize->body.as<ProducerConsumer>();
            internal_assert(!pc_A->update.defined()); // cannot handle this case for now

            Stmt new_pc_B = ProducerConsumer::make(op->name, op->produce, op->update, pc_A->produce);
            Stmt new_pc_A = ProducerConsumer::make(target, new_pc_B, Stmt(), pc_A->consume);

            stmt = Realize::make(target, realize->types, realize->bounds, realize->condition, new_pc_A);
        }
    }

    void visit(const LetStmt *op) {
        debug(3) << "visit LetStmt node " << op->name << '\n';
        Stmt body_before_swap = mutate(op->body);

        internal_assert(is_target(body_before_swap));
        const Realize *realize = body_before_swap.as<Realize>();
        const ProducerConsumer *pc = realize->body.as<ProducerConsumer>();

        /* Swap the current LetSmt and the target PC.
         * e.g. before swap             after swap
         *          LetStmt                realize
         *             |                     |
         *          realize                  PC
         *             |                    /  \
         *            PC               LetStmt LetStmt
         *            / \                  |     |
         *           P   C                 P     C
         */
        internal_assert(!pc->update.defined()); // cannot handle this case for now

        Stmt new_produce = LetStmt::make(op->name, op->value, pc->produce);
        Stmt new_consume = LetStmt::make(op->name, op->value, pc->consume);

        Stmt new_pc = ProducerConsumer::make(target, new_produce, Stmt(), new_consume);
        stmt = Realize::make(target, realize->types, realize->bounds, realize->condition, new_pc);
    }

    void visit(const For *op) {
        debug(3) << "visit For node " << op->name << '\n';
        Stmt body_before_swap = mutate(op->body);

        internal_assert(is_target(body_before_swap));
        const Realize *realize = body_before_swap.as<Realize>();
        const ProducerConsumer *pc = realize->body.as<ProducerConsumer>();
        internal_assert(!pc->update.defined()); // cannot handle this case for now

        Stmt new_produce = For::make(op->name, op->min, op->extent, op->for_type,
                                     op->device_api, pc->produce);
        Stmt new_consume = For::make(op->name, op->min, op->extent, op->for_type,
                                     op->device_api, pc->consume);

        Stmt new_pc = ProducerConsumer::make(target, new_produce, Stmt(), new_consume);
        stmt = Realize::make(target, realize->types, realize->bounds, realize->condition, new_pc);
    }

    void visit(const Allocate *op) {
        debug(3) << "visit Allocate node " << op->name << '\n';
        internal_assert(false);
    }

    void visit(const IfThenElse *op) {
        // not sure if this a valid transformation across IfThenElse node
        internal_assert(false);
    }

    void visit(const Block *op) {
        // not sure if this a valid transformation across Block node
        internal_assert(false);
    }

    void visit(const AssertStmt *op) {
        // not sure if this a valid transformation across this type node
        internal_assert(false);
    }

    void visit(const Store *op) {
        // not sure if this a valid transformation across this type node
        internal_assert(false);
    }

    void visit(const Provide *op) {
        // not sure if this a valid transformation across this type node
        internal_assert(false);
    }

    void visit(const Free *op) {
        // not sure if this a valid transformation across this type node
        internal_assert(false);
    }

    void visit(const Evaluate *op) {
        // not sure if this a valid transformation across this type node
        internal_assert(false);
    }

public:
    PullUpTargetNode(string name) : target(name) {
        internal_assert(ends_with(name, ".stream"));
    }
};

// Push the scan loops into the produce and consume of a kernel stream,
// so we get a pipeline of produce loop nest and consume loop nest,
// which can run concurrently with a FIFO interface of the kernel stream
class PushLoopsIntoStreamForFunction : public IRMutator {
    const HWKernel &kernel;
    SearchIRNode<ProducerConsumer> searcher;

    using IRMutator::visit;

    void visit(const For *op) {
        // Before transformation:
        // for scan_loop_var {
        //   realize kernel.stencil.stream {
        //     produce kernel.stencil.stream {...}
        //     consume {...}
        // } }
        //
        // After transformation:
        // realize kernel.stencil.stream {
        //   produce kernel.stencil.stream {
        //     for scan_loop_var {...}
        //   }
        //   consume kernel.stencil.stream {
        //     for scan_loop_var {...}
        // } }
        string stream_name = kernel.name + ".stencil.stream";
        // find a loop that produce the kernel
        if(searcher.search(op, stream_name)) {
            PullUpTargetNode reorder(stream_name);
            stmt = reorder.mutate(op);
        } else {
            IRMutator::visit(op);
        }
    }

public:
    PushLoopsIntoStreamForFunction(const HWKernel &k) : kernel(k) {}
};

// Implements line buffers
// It replace the produce of func.stencil and func.stencil.stream with
// func.stencil_update and func.stencil_update.stream. The latters are
// smaller, which only consist of the new pixels sided in each shift of
// the stencil window.
// A line buffer is instantiated to buffer the stencil_update.stream and
// to generate the stencil.stream
class LinebufferForFunction : public IRMutator {
    const HWKernel& kernel;
    Scope<Expr> scope;

    using IRMutator::visit;

    // TODO optimization if the update stream is of the same dimension,
    // make a reference as opposed to create a linebuffer
    void visit(const Realize *op) {
        if(op->name != kernel.name + ".stencil" &&
           op->name != kernel.name + ".stencil.stream") {
            IRMutator::visit(op);
        } else if (op->name == kernel.name + ".stencil.stream") {
            // If it is the realize(+PC) node for func.stencil.stream,
            // we traverse and mutate the produce node while keep the consume node.
            // Then, we create realize+PC nodes of func.stencil_update.stream and a line buffer
            // e.g. before:
            //        realize func.stencil.stream {
            //          produce func.stencil.stream {...}
            //          consume {...}
            //        }
            //
            //      after:
            //        realize func.stencil_update.stream {
            //          produce func.stencil_update.stream {...}
            //            realize func.stencil.stream {
            //              produce func.stencil.stream {linebuffer(...)}
            //              consume {...}
            //        }   }
            const ProducerConsumer *stencil_pc = op->body.as<ProducerConsumer>();
            internal_assert(stencil_pc && stencil_pc->name == kernel.name + ".stencil.stream");
            Stmt produce_stencil_update = mutate(stencil_pc->produce);

            // create call to instatiate the line buffer
            Expr update_stream_var = Variable::make(Handle(), kernel.name + ".stencil_update.stream");
            Expr stream_var = Variable::make(Handle(), kernel.name + ".stencil.stream");
            vector<Expr> args({update_stream_var, stream_var});
            // extract the buffer size, and put it into args
            for (size_t i = 0; i < kernel.dims.size(); i++) {
                Expr store_extent = simplify(kernel.dims[i].store_bound.max -
                                             kernel.dims[i].store_bound.min + 1);
                args.push_back(store_extent);
            }
            Stmt linebuffer_call = Evaluate::make(Call::make(Handle(), "linebuffer", args, Call::Intrinsic));

            // dispatch the stream into seperate streams for each of its consumers
            // syntax:
            //   dispatch_stream(stream_name, num_of_dimensions,
            //                   stencil_size_dim_0, stencil_step_dim_0, store_extent_dim_0,
            //                   [stencil_size_dim_1, stencil_step_dim_1, store_extent_dim_1, ...]
            //                   num_of_consumers,
            //                   consumer_0_name, consumer_0_offset_dim_0, consumer_0_extent_dim_0,
            //                   [consumer_0_offset_dim_1, consumer_0_extent_dim_1, ...]
            //                   [consumer_1_name, ...])
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
            Stmt dispatch_call = Evaluate::make(Call::make(Handle(), "dispatch_stream", dispatch_args, Call::Intrinsic));

            Stmt stream_calls = Block::make(linebuffer_call, dispatch_call);

            // create realize+PC nodes for func.stencil.stream
            internal_assert(!stencil_pc->update.defined());
            Stmt new_stencil_pc = ProducerConsumer::make(kernel.name + ".stencil.stream",
                                                         stream_calls, Stmt(), stencil_pc->consume);
            Stmt new_stencil_realize = Realize::make(kernel.name + ".stencil.stream", op->types,
                                                     op->bounds, const_true(), new_stencil_pc);


            // create realize+PC nodes for func.stencil_update.stream
            Stmt new_pc = ProducerConsumer::make(kernel.name + ".stencil_update.stream",
                                                 produce_stencil_update, Stmt(), new_stencil_realize);

            // create a realizeation of the stencil_update stream
            Region bounds;
            for (StencilDimSpecs dim: kernel.dims) {
                bounds.push_back(Range(0, dim.step));
            }
            stmt = Realize::make(kernel.name + ".stencil_update.stream", op->types, bounds, const_true(), new_pc);
        } else {
            // If it is the Realize(+PC) node for func.stencil, then
            // replace it with a new Realize+PC node for smaller func.stencil
            const ProducerConsumer *stencil_pc = op->body.as<ProducerConsumer>();
            internal_assert(stencil_pc && stencil_pc->name == kernel.name + ".stencil");

            Stmt produce = mutate(stencil_pc->produce);
            Stmt update = mutate(stencil_pc->update);

            // replace the old consumer (which writes to a func.stencil.stream)
            // with a new consumer which writes to func.stencil_update.stream
            const Evaluate *old_consume = stencil_pc->consume.as<Evaluate>();
            internal_assert(old_consume);
            const Call *old_write_call = old_consume->value.as<Call>();
            internal_assert(old_write_call && old_write_call->name == "write_stream");

            Expr stream_var = Variable::make(Handle(), kernel.name + ".stencil_update.stream");
            Expr stencil_var = Variable::make(Handle(), kernel.name + ".stencil");
            Stmt consume = Evaluate::make(Call::make(Handle(), "write_stream", {stream_var, stencil_var}, Call::Intrinsic));

            Stmt new_pc = ProducerConsumer::make(kernel.name + ".stencil", produce, update, consume);

            Region new_bounds(op->bounds.size());
            internal_assert(op->bounds.size() == kernel.dims.size());
            // Mutate the bounds
            for (size_t i = 0; i < op->bounds.size(); i++) {
                new_bounds[i] = Range(0, kernel.dims[i].step);
            }

            stmt = Realize::make(kernel.name + ".stencil", op->types, new_bounds,
                                 const_true(), new_pc);
        }
    }

    void visit(const For *op) {
        // We intend to mutate two kinds of loops.
        // 1. We replace the outer loops that slide the stencil window across the image
        //    with loops for update stencil
        //
        // 2. We replace the loops that iterate over the stencil window with loops
        //    iterating over the update stencil windows
        //
        // e.g.
        // for scan.loop_var, scan.min, scan.extent
        //   realize stencil[0, stencil.size]
        //   for stencil.loop_var, 0, stencil.size
        //     compute stencil[stencil.loop_var]
        //
        // =========
        // let scan_update.extent = store_bounds.extent / stencil.step
        // for scan_update.loop_var, scan.min, scan_update.extent
        //   let scan.loop_var = scan_update.loop_var
        //   realize update_stencil[0, stencil.step]
        //   for stencil_update.loop_var, 0, stencil.step
        //     let stencil.loop_var = stencil_update.loop_var
        //     compute stencil_update[stencil.loop_var]


        string stencil_name = kernel.name + ".stencil.";
        // check if the for loop is a outer loops (scan.loop_var) that slide
        size_t dim_idx = 0;
        while (dim_idx < kernel.dims.size()) {
            if(op->name == kernel.dims[dim_idx].loop_var)
                break;
            dim_idx++;
        }

        if(dim_idx < kernel.dims.size()) {
            // found a proper outer scan.loop_var
            debug(3) << "find outer loop " << op->name << " to mutate.\n";

            StencilDimSpecs dimspecs = kernel.dims[dim_idx];
            internal_assert(dimspecs.loop_var == op->name);
            string new_loop_var_name = kernel.name + ".scan_update." + kernel.func.args()[dim_idx];
            Expr new_loop_var = Variable::make(Int(32), new_loop_var_name);

            //Expr store_extent = buffer_bounds[dim_idx].extent;w
            Expr store_extent = simplify(kernel.dims[dim_idx].store_bound.max -
                                         kernel.dims[dim_idx].store_bound.min + 1);
            debug(3) << "kernel " << kernel.name << " store_extent = " << store_extent << '\n';

            // check the condition for the new loop for sliding the update stencil
            const IntImm *store_extent_int = store_extent.as<IntImm>();
            internal_assert(store_extent_int);
            if (store_extent_int->value % dimspecs.step != 0) {
                // we cannot handle this scenario yet
                internal_error
                    << "Line buffer extent (" << store_extent_int->value
                    << ") is not divisible by the stencil step " << dimspecs.step << '\n';
            }
            int new_extent = store_extent_int->value / dimspecs.step;

            string old_var_name = op->name;
            Expr old_var_value = new_loop_var;

            // traversal down into the body
            scope.push(old_var_name, old_var_value);
            Stmt new_body = mutate(op->body);
            scope.pop(old_var_name);

            new_body = LetStmt::make(old_var_name, old_var_value, new_body);
            stmt = For::make(new_loop_var_name, op->min, new_extent, op->for_type, op->device_api, new_body);

        } else if (starts_with(op->name, stencil_name)) {
            // found a inner loop
            debug(3) << "find stencil loop " << op->name << " to mutate.\n";
            string old_var_name = op->name;
            string stage_dim_name = op->name.substr(stencil_name.size(), old_var_name.size() - stencil_name.size());

            // look for dim_name in stencil specs
            size_t i = 0;
            while(i < kernel.func.args().size()) {
                if(ends_with(stage_dim_name, kernel.func.args()[i]))
                    break;
                i++;
            }
            internal_assert(i < kernel.dims.size());
            int new_extent = kernel.dims[i].step;

            // traversal down into the body
            Stmt new_body = mutate(op->body);

            //new_body = LetStmt::make(old_var_name, old_var_value, new_body);
            stmt = For::make(op->name, 0, new_extent, op->for_type, op->device_api, new_body);
        } else {
            // otherwise, keep traversal down
            IRMutator::visit(op);
        }
    }

    void visit(const LetStmt *op) {
        scope.push(op->name, simplify(expand_expr(op->value, scope)));
        Stmt new_body = mutate(op->body);
        if (new_body.same_as(op->body)) {
            stmt = op;
        } else {
            stmt = LetStmt::make(op->name, op->value, new_body);
        }
        scope.pop(op->name);
    }

public:
    LinebufferForFunction(const HWKernel &k)
        : kernel(k){ }
};


// Perform streaming optimization for all functions
class StreamOpt : public IRMutator {
    const HWKernelDAG &dag;
    Scope<Expr> scope;

    using IRMutator::visit;

    void visit(const Realize *op) {
        const auto it = dag.kernels.find(op->name);
        if (it == dag.kernels.end()) {
            IRMutator::visit(op);
            return;
        }

        const HWKernel &kernel = it->second;
        internal_assert(kernel.name == op->name);
        internal_assert(kernel.func.name() == kernel.name);

        // inlined kernels are skipped
        if (kernel.is_inlined) {
            IRMutator::visit(op);
            return;
        }
        debug(3) << "Find " << kernel << "\n";

        Stmt new_body = op->body;

        new_body = CreateStencilForFunction(kernel, dag, scope).mutate(new_body);
        debug(3) << "IR after CreateStencilForFunction pass on Function " << kernel.name
                 << ":\n" << new_body << '\n';

        // if the HW kernel is buffered, we try to implement line buffers
        new_body = AddStreamOperationForFunction(kernel, dag).mutate(new_body);
        debug(3) << "IR after AddStreamOperationForFunction pass on Function " << kernel.name
                 << ":\n" << new_body << '\n';

        new_body = PushLoopsIntoStreamForFunction(kernel).mutate(new_body);

        debug(3) << "IR after PullUpTargetNode pass on Function " << kernel.name
                 << ":\n" << new_body << '\n';

        // check if we need a line buffer
        // We skip inserting a line buffer for the output kernel
        bool insert_linebuffer = true;
        if (kernel.is_output)
            insert_linebuffer = false;

        if (insert_linebuffer) {
            new_body = LinebufferForFunction(kernel).mutate(new_body);
            debug(3) << "IR after LinebufferForFunction pass on Function " << kernel.name
                     << ":\n" << new_body << '\n';
        } else {
            debug(3) << "Skip inserting linebuffer for Function " << kernel.name << '\n';
        }

        new_body = mutate(new_body);

        // we don't need the old realization node, since the storage is implicitly in the line buffer
        stmt = new_body;
    }

    void visit(const LetStmt *op) {
        scope.push(op->name, simplify(expand_expr(op->value, scope)));
        Stmt new_body = mutate(op->body);
        if (new_body.same_as(op->body)) {
            stmt = op;
        } else {
            stmt = LetStmt::make(op->name, op->value, new_body);
        }
        scope.pop(op->name);
    }

public:
    StreamOpt(const HWKernelDAG &d)
        : dag(d) {}

};

Stmt stream_opt(Stmt s, const HWKernelDAG &dag) {
    s = StreamOpt(dag).mutate(s);
    return s;
}

}
}
