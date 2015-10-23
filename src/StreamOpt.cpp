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
using std::pair;
using std::make_pair;

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
    const Scope<Expr> &outer_scope;  // FIXME do we need this?
    Scope<Expr> scope;

    using IRMutator::visit;

    void visit(const ProducerConsumer *op) {
        if (op->name != kernel.name) {
            IRMutator::visit(op);
        } else {
            // traverse into produce and consume node of the func,
            // replace the PC node name with the stencil name, and
            // create a realize node for the stencil object
            string stencil_name = kernel.name + ".stencil";

            Stmt produce = mutate(op->produce);
            Stmt update = mutate(op->update);
            Stmt consume = mutate(op->consume);

            Stmt pc = ProducerConsumer::make(stencil_name, produce, update, consume);

            // create a realizeation of the stencil image
            Region bounds;
            for (StencilDimSpecs dim: kernel.dims) {
                bounds.push_back(Range(0, dim.size));
            }
            stmt = Realize::make(stencil_name, kernel.func.output_types(), bounds, const_true(), pc);
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
                Expr new_arg = old_arg - kernel.dims[i].min_pos;
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

            // simplify the args
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
    CreateStencilForFunction(const HWKernel &k, const Scope<Expr> &s)
        : kernel(k), outer_scope(s) {}
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
        Expr stream_var = Variable::make(Handle(), stencil_name + ".stream");
        Expr stencil_var = Variable::make(Handle(), stencil_name);
        vector<Expr> args({stream_var, stencil_var});
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
            vector<Expr> args({stream_var, stencil_var});
            Stmt write_call = Evaluate::make(Call::make(Handle(), "write_stream", args, Call::Intrinsic));

            // realize and PC node for func.stencil
            Stmt stencil_pc = ProducerConsumer::make(stencil_name, pc->produce, pc->update, write_call);
            Stmt stencil_realize = Realize::make(stencil_name, op->types, op->bounds, op->condition, stencil_pc);

            // add read_stream for each input stencil (producers fed to func)
            for (const string& s : kernel.buffered_producers) {
                const auto it = dag.kernels.find(s);
                internal_assert(it != dag.kernels.end());
                stencil_realize = add_input_stencil(stencil_realize, it->second);
            }

            Stmt stream_consume = pc->consume;
            if (kernel.name == dag.name) {
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
    const Region& buffer_bounds;
    Scope<Expr> scope;

    using IRMutator::visit;

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
            Expr stream_var = Variable::make(Handle(), kernel.name + ".stencil_update.stream");
            Expr stencil_var = Variable::make(Handle(), kernel.name + ".stencil.stream");
            vector<Expr> args({stream_var, stencil_var});
            // extract the buffer size, and put it into args
            for(const Range &r : buffer_bounds)
                args.push_back(r.extent);
            Stmt linebuffer_call = Evaluate::make(Call::make(Handle(), "linebuffer", args, Call::Intrinsic));

            // create realize+PC nodes for func.stencil.stream
            internal_assert(!stencil_pc->update.defined());
            Stmt new_stencil_pc = ProducerConsumer::make(kernel.name + ".stencil.stream",
                                                         linebuffer_call, Stmt(), stencil_pc->consume);
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

            Expr store_extent = buffer_bounds[dim_idx].extent;
            debug(3) << "store_extent = " << store_extent << '\n';

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
    LinebufferForFunction(const HWKernel &k, const Region &b)
        : kernel(k), buffer_bounds(b) { }
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
        debug(3) << "Find " << kernel << "\n";

        Stmt new_body = op->body;

        new_body = CreateStencilForFunction(kernel, scope).mutate(new_body);
        debug(3) << "IR after CreateStencilForFunction pass on Function " << kernel.name
                 << ":\n" << new_body << '\n';

        if (kernel.is_buffered) {
            // if the HW kernel is buffered, we try to implement line buffers
            new_body = AddStreamOperationForFunction(kernel, dag).mutate(new_body);
            debug(3) << "IR after AddStreamOperationForFunction pass on Function " << kernel.name
                     << ":\n" << new_body << '\n';

            new_body = PushLoopsIntoStreamForFunction(kernel).mutate(new_body);

            debug(3) << "IR after PullUpTargetNode pass on Function " << kernel.name
                     << ":\n" << new_body << '\n';

            // check if we need a line buffer
            // if stencil step is equal to stencil size, we skip inserting the line buffer
            bool insert_linebuffer = false;
            for (const StencilDimSpecs &dimspecs : kernel.dims) {
                internal_assert(dimspecs.step <= dimspecs.size);
                if (dimspecs.step < dimspecs.size) {
                    insert_linebuffer = true;
                    break;
                }
            }

            // we always add a line buffer at the inputs to help grouping the pipeline
            if (dag.input_kernels.count(kernel.name))
                insert_linebuffer = true;

            if (insert_linebuffer) {
                new_body = LinebufferForFunction(kernel, op->bounds).mutate(new_body);
                debug(3) << "IR after LinebufferForFunction pass on Function " << kernel.name
                         << ":\n" << new_body << '\n';
            } else {
                debug(3) << "Skip inserting linebuffer for Function " << kernel.name << '\n';
            }
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

// Carve out the kernels of the HW dag.
class CarveHWPipeline : public IRMutator {
    const HWKernelDAG &dag;
    Stmt pipeline_body;

    using IRMutator::visit;
    void visit(const Realize *op) {
        if (ends_with(op->name, ".stream")) {
            // extract the kernel from the stream name
            string kernel_name;
            if (ends_with(op->name, ".stencil.stream")) {
                kernel_name = op->name.substr(0, op->name.size() - string(".stencil.stream").size());
            } else if (ends_with(op->name, ".stencil_update.stream")) {
                kernel_name = op->name.substr(0, op->name.size() - string(".stencil_update.stream").size());
            } else {
                internal_error << "unexpected name format of stream.\n";
            }

            if (dag.name == kernel_name) {
                // it is a output kernel
                internal_assert(op->name == kernel_name + ".stencil.stream")
                    << "unexpected output kernel stream name: " << op->name << "\n";
                const ProducerConsumer *pc = op->body.as<ProducerConsumer>();
                internal_assert(pc && pc->name == op->name);

                // put pc->produce into pipeline_body.
                // NOTE the realize node is not part of the pipeline
                internal_assert(!pipeline_body.defined());
                internal_assert(!pc->update.defined());
                pipeline_body = pc->produce;
                stmt = op;
            } else if ((dag.kernels.count(kernel_name) &&
                        !dag.input_kernels.count(kernel_name)) ||
                       (dag.input_kernels.count(kernel_name) &&
                        op->name == kernel_name + ".stencil.stream")) {
                // it is a non-input (or linebuffered input) kernel of dag,
                // carve it out and put it in pipeline_body
                const ProducerConsumer *pc = op->body.as<ProducerConsumer>();
                internal_assert(pc && pc->name == op->name);

                // recurse. carve out realize and pc->produce,
                // leave only the comsumer in the IR
                stmt = mutate(pc->consume);

                // put realize and pc->produce into pipeline_body
                internal_assert(!pc->update.defined());
                pipeline_body = ProducerConsumer::make(pc->name, pc->produce, Stmt(), pipeline_body);
                pipeline_body = Realize::make(op->name, op->types, op->bounds, op->condition, pipeline_body);
            } else {
                // it is a input kernel of dag, then keep it unchanged
                IRMutator::visit(op);
            }
        } else {
            IRMutator::visit(op);
        }
    }

public:
    CarveHWPipeline(const HWKernelDAG &d) : dag(d) {}

    pair<Stmt, Stmt> extract(Stmt s) {
        s = mutate(s);
        return make_pair(s, pipeline_body);
    }
};

// Insert pipeline_body into the produce node of dag_name.stencil.stream
class InsertHWPipeline : public IRMutator {
    const string &dag_name;
    Stmt pipeline_body;

    using IRMutator::visit;
    void visit(const ProducerConsumer *op) {
        if (starts_with(op->name, dag_name)) {
            internal_assert(op->name == dag_name + ".stencil.stream")
                << "unexpected output kernel stream name: " << op->name << "\n";

            internal_assert(!op->update.defined());
            stmt = ProducerConsumer::make( "_hls_target." + op->name, pipeline_body, Stmt(), op->consume);
        } else {
            IRMutator::visit(op);
        }
    }

public:
    InsertHWPipeline(const string &name, Stmt s): dag_name(name), pipeline_body(s) {}
};

Stmt group_hw_pipeline(Stmt s, const HWKernelDAG &dag) {
    // Before this pass of transformation, the IR for pipeline dag_name looks like:
    // produce func {
    //   /* some loop tiles */
    //   realize input.stencil_update.stream {
    //     produce input.stencil_update.stream {...}
    //     realize input.stencil.stream {
    //       produce input.stencil.stream {
    //         linebuffer(input.stencil_update.stream, input.stencil.stream)
    //       }
    //       realize stage1.stencil.stream {
    //         produce stage1.stencil.stream {...}
    //         realize produce stage2.stencil.stream {
    //           produce stage2.stencil.stream {...}
    //           /* the rest stages */
    //           realize dag_name.stencil.stream {
    //             produce dag_name.stencil.stream {...}
    //             func(...) = func_stream.stencil(...)
    // } } } } } }
    //
    // After the transforamtion, we want to group the computation in the hardware
    // pipeline (i.e. input.stencil.stream, stage1, stage2, and dag_name) into a sub-tree
    // of IR. input.stencil_update.stream and func are parts of SW pipeline.
    // Here we decide to group those into the produce node of ProducerConsumer(PC)
    // dag_name.stencil.stream.
    // In practice, we do it in two passes. First, we carve out the pipeline. Then, we
    // insert the pipeline into the produce node of dag_name.stencil.stream.
    //
    // The result looks like:
    // produce func {
    //   /* some loop tiles */
    //   realize input.stencil_update.stream {
    //     produce input.stencil_update.stream {...}
    //     realize dag_name.stencil.stream {
    //       produce dag_name.stencil.stream {
    //         realize input.stencil.stream {
    //           produce input.stencil.stream {...}
    //           realize stage1.stencil.stream {
    //             produce stage1.stencil.stream {...}
    //             realize produce stage2.stencil.stream {
    //               produce stage2.stencil.stream {...}
    //               /* the rest stages */
    //               /* original produce of dag_name.stencil.steam */
    //       } } } }
    //       func(...) = dag_name.stencil(...)
    // } } }

    pair<Stmt, Stmt> p = CarveHWPipeline(dag).extract(s);
    s = InsertHWPipeline(dag.name, p.second).mutate(p.first);
    return s;
}

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

Stmt stream_opt(Stmt s, const vector<HWKernelDAG> &dags) {
    for(const HWKernelDAG &dag : dags) {
        s = StreamOpt(dag).mutate(s);
        s = group_hw_pipeline(s, dag);
        s = ReplaceImageParam(dag).mutate(s);
    }
    return s;
}

}
}
