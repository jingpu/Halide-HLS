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
using std::ostream;

namespace Halide {
namespace Internal {

using std::string;
using std::map;
using std::vector;

namespace {

// Does an expression depend on a particular variable?
class ExprDependsOnVar : public IRVisitor {
    using IRVisitor::visit;

    void visit(const Variable *op) {
        if (op->name == var) result = true;
    }

    void visit(const Let *op) {
        op->value.accept(this);
        // The name might be hidden within the body of the let, in
        // which case there's no point descending.
        if (op->name != var) {
            op->body.accept(this);
        }
    }
public:

    bool result;
    string var;

    ExprDependsOnVar(string v) : result(false), var(v) {
    }
};

bool expr_depends_on_var(Expr e, string v) {
    ExprDependsOnVar depends(v);
    e.accept(&depends);
    return depends.result;
}


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

struct StencilDimSpecs {
    int size;  // stencil window size
    int step;     // stencil window shifting step
    Expr min_pos; // stencil origin position w.r.t. the original image buffer
    string loop_var;  // outer loop var that shifts this dimensions
};


struct StencilSpecs {
    Function func;
    bool is_valid;
    vector<StencilDimSpecs> dims;
};

ostream &operator<<(ostream &out, const StencilDimSpecs dim) {
    out << "[" << dim.min_pos << ", "
        << dim.size << ", looping over "<< dim.loop_var << " step " << dim.step << "]";
    return out;
}

ostream &operator<<(ostream &out, const StencilSpecs st) {
    if (!st.is_valid)
        out << "Realization of function " << st.func.name() << " is not a valid stencil.\n";
    else {
        out << "Realization of function " << st.func.name() << " is a valid stencil.\n";
        for(size_t i = 0; i < st.dims.size(); i++)
            out << "dim " << st.func.args()[i] << ": " << st.dims[i] << '\n';
    }
    return out;
}


/** Check whether the function is a stencil function,
 *  and return the stencil specs.
 */
class AnalysisStencilFunction : public IRVisitor {
    Function func;
    StencilSpecs specs;
    Scope<Expr> scope;
    vector<string> loop_vars;

    using IRVisitor::visit;

    /** return the index of the loop_var in LOOP_VARS each STENCIL dimension depends on.
     * return a index of -1 if the dimension doesn't depend on any loop_var
     * return a index of -2 if the dimension is not valid
     */
    vector<int> get_loop_var_for_stencil(Box stencil) {
        vector<int> ret(stencil.size(), -1);

        // a valid stencil dimension can only depend on one or zero loop_var in loop_vars.
        // any two dimensions can not depend on the same var
        std::set<size_t> used_var;
        for(size_t i = 0; i < stencil.size(); i++) {
            bool find_one_var = false;
            Expr min = simplify(expand_expr(stencil[i].min, scope));

            for(size_t j = 0; j < loop_vars.size(); j++){
                if(expr_depends_on_var(min, loop_vars[j])) {
                    if(find_one_var) {
                        // now find two, invalid dimension
                        ret[i] = -2;
                        break;
                    } else {
                        // find the first var
                        if(used_var.count(j) != 0) {
                            ret[i] = -2;
                            break;
                        } else {
                            ret[i] = j;
                            find_one_var = true;
                            used_var.insert(j);
                        }
                    }
                }
            }
        }
        return ret;
    }

    /** On a ProducerConsumer node, retrieve the required box region of the comsumer,
     * and check wether it is a constant stencil window.
     */
    void visit(const ProducerConsumer *op) {
        if (op->name != func.name()) {
            IRVisitor::visit(op);
        } else {
            if (op->update.defined()) {
                debug(3) << "Warning: Function " << func.name() << " has update definitions" << '\n';
                internal_assert(false);
            }
            debug(3) << "\nAnalyze the consumers of function " << func.name() << '\n';
            Box stencil = box_required(op->consume, func.name());
            internal_assert(stencil.size() == func.args().size());
            vector<int> loop_var_idx = get_loop_var_for_stencil(stencil);

            bool is_valid_stencil = true;
            for (size_t i = 0; i < stencil.size(); i++) {
                StencilDimSpecs dim;

                Expr min = simplify(expand_expr(stencil[i].min, scope));
                Expr max = simplify(expand_expr(stencil[i].max, scope));
                Expr extent = simplify(max - min + 1);
                dim.min_pos = min;

                // check if the size of the window is a constant
                const IntImm *extent_int = extent.as<IntImm>();
                if(extent_int)
                    dim.size = extent_int->value;
                else
                    is_valid_stencil = false;

                // check if the windows is sliding along one and only one dimension,
                // i.e. the depending loop_var is one of the func.args
                debug(3) << "dim " << i << "(" << func.args()[i] << ") ";
                if(loop_var_idx[i] == -2) {
                    debug(3) << "is not valid\n";
                    is_valid_stencil = false;
                } else if(loop_var_idx[i] == -1) {
                    debug(3) << "does not depend on any loop vars\n";
                    dim.loop_var = "undef";
                    dim.step = dim.size;
                } else {
                    debug(3) <<" depending on Loop "<< loop_vars[loop_var_idx[i]] <<" \n";
                    dim.loop_var = loop_vars[loop_var_idx[i]];
                }
                debug(3) << "\tmin: " << min << "\tmax: " << max << '\n'
                         << "\textent: " << extent;

                // check if the step of the windows is a constant
                if (loop_var_idx[i] >= 0) {
                    Expr step = simplify(finite_difference(min, loop_vars[loop_var_idx[i]]));
                    debug(3) << "\tstep: " << step << '\n';
                    const IntImm *step_int = step.as<IntImm>();
                    if(step_int)
                        dim.step = step_int->value;
                    else
                        is_valid_stencil = false;
                } else {
                    debug(3) << '\n';
                }
                specs.dims.push_back(dim);
            }
            specs.is_valid = is_valid_stencil;
        }
    }

    void visit(const LetStmt *op) {
        scope.push(op->name, simplify(expand_expr(op->value, scope)));
        op->body.accept(this);
        scope.pop(op->name);
    }

    void visit(const For *op) {
        loop_vars.push_back(op->name);
        op->body.accept(this);
        loop_vars.pop_back();
    }

public:
    AnalysisStencilFunction(Function f) : func(f) {
        specs.func = f;
        specs.is_valid = false;
    }

    StencilSpecs getSpecs(Stmt s) {
        s.accept(this);
        return specs;
    }
};


// create a stencil object for a function, which holds the required
// values for its consumers. And replace the references to the function
// image buffer (Provide and Call nodes) with references to the stencil object
class CreateStencilForFunction : public IRMutator {
    Function func;
    StencilSpecs specs;
    Scope<Expr> scope;

    using IRMutator::visit;

    void visit(const ProducerConsumer *op) {
        if (op->name != func.name()) {
            IRMutator::visit(op);
        } else {
            // traverse into produce and consume node of the func,
            // replace the PC node name with the stencil name, and
            // create a realize node for the stencil object
            Stmt produce = mutate(op->produce);
            Stmt update = op->update;
            if(op->update.defined()) {
                // We cannot handle it yet
                internal_assert(false);
            }
            Stmt consume = mutate(op->consume);

            Stmt pc = ProducerConsumer::make(op->name+".stencil", produce, update, consume);

            // create a realizeation of the stencil image
            Region bounds;
            for (StencilDimSpecs dim: specs.dims) {
                Expr extent = make_const(Int(32), dim.size);
                bounds.push_back(Range(0, extent));
            }
            stmt = Realize::make(func.name()+".stencil", func.output_types(), bounds, const_true(), pc);
        }
    }


    void visit(const For *op) {
        string stage_name = func.name() + ".s0.";
        if (!starts_with(op->name, stage_name)) {
            IRMutator::visit(op);
        } else {
            // replace the loop var over the dimensions of the original function
            // realization with the loop var over the stencil dimension.
            // e.g. funcA.s0.x -> funcA.stencil.x
            string old_var_name = op->name;
            string dim_name = op->name.substr(stage_name.size(), old_var_name.size() - stage_name.size());
            string new_var_name = func.name() + ".stencil." + dim_name;
            Expr new_var = Variable::make(Int(32), new_var_name);

            // find the stencil dimension given dim_name
            int dim_idx = -1;
            for(size_t i = 0; i < func.args().size(); i++)
                if(dim_name == func.args()[i]) {
                    dim_idx = i;
                    break;
                }
            internal_assert(dim_idx != -1);
            Expr new_min = make_const(Int(32), 0);
            Expr new_extent = make_const(Int(32), specs.dims[dim_idx].size);

            // create a let statement for the old_loop_var
            Expr old_min = op->min;
            Expr old_var_value = new_var + old_min;

            // traversal down into the body
            scope.push(old_var_name, old_var_value);
            Stmt new_body = mutate(op->body);
            scope.pop(old_var_name);

            new_body = LetStmt::make(old_var_name, old_var_value, new_body);
            stmt = For::make(new_var_name, new_min, new_extent, op->for_type, op->device_api, new_body);
        }
    }

    void visit(const Provide *op) {
        if(op->name != func.name()) {
            IRMutator::visit(op);
        } else {
            // Replace the provide node of func with provide node of func.stencil
            string stencil_name = func.name() + ".stencil";
            vector<Expr> new_args(op->args.size());

            // Replace the arguments. e.g.
            //   func.s0.x -> func.stencil.x
            for (size_t i = 0; i < func.args().size(); i++) {
                string old_arg_name = func.name() + ".s0." + func.args()[i];
                string new_arg_name = func.name() + ".stencil." + func.args()[i];
                internal_assert(is_one(simplify(op->args[i] == Variable::make(Int(32), old_arg_name))));
                Expr new_arg = Variable::make(Int(32), new_arg_name);
                new_args[i] = new_arg;
            }
            stmt = Provide::make(stencil_name, op->values, new_args);
        }
    }

    void visit(const Call *op) {
        if(op->name != func.name()) {
            IRMutator::visit(op);
        } else {
            // check assumptions
            internal_assert(op->func.same_as(func));
            internal_assert(op->call_type == Call::Halide);
            internal_assert(op->args.size() == func.args().size());

            // Replace the call node of func with call node of func.stencil
            string stencil_name = func.name() + ".stencil";
            vector<Expr> new_args(op->args.size());

            // Mutate the arguments.
            // The value of the new argment is the old_value - stencil.min_pos.
            // The new value shouldn't refer to old loop vars any more
            for (size_t i = 0; i < op->args.size(); i++) {
                Expr old_arg = op->args[i];
                Expr new_arg = old_arg - specs.dims[i].min_pos;
                new_arg = simplify(expand_expr(new_arg, scope));

                // TODO check if the new_arg only depends on the loop vars
                // inside the producer
                new_args[i] = new_arg;
            }
            expr = Call::make(op->type, stencil_name, new_args, Call::Halide,
                              op->func, op->value_index, Buffer(), Parameter());
        }
    }

    void visit(const Let *op) {
        scope.push(op->name, simplify(expand_expr(op->value, scope)));
        Expr new_body = mutate(op->body);
        if (new_body.same_as(op->body)) {
            expr = op;
        } else {
            expr = Let::make(op->name, op->value, new_body);
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
    CreateStencilForFunction(Function f, StencilSpecs s) : func(f), specs(s) { internal_assert(s.is_valid); }
};


// Replace the stencil PC node with stencil.stream PC node
// add calls to read/write the stencil.stream.
// We also remove the original realize node of func.stencil because
// we push it down into produce and consume of the stencil.stream
class AddStreamOperationForFunction : public IRMutator {
    Function func;

    using IRMutator::visit;

    void visit(const Realize * op) {
        string stencil_name = func.name() + ".stencil";
        if (op->name != stencil_name) {
            IRMutator::visit(op);
        } else {
            // Before mutation:
            //     realize func.stencil {
            //       produce func.stencil {...}
            //       consume func.stencil
            //
            // After mutation:
            //     produce func.stencil.stream {
            //       realize func.stencil {
            //         produce func.stencil {...}
            //         write_stream(func.stencil.stream, func.stencil)
            //       }
            //     realize func.stencil {
            //       produce func.stencil {
            //         read_stream(func.stencil.stream, func.stencil)
            //       }
            //       consume func.stencil
            //     }

            // check if the body of Realize node is a PC node
            const ProducerConsumer *pc = op->body.as<ProducerConsumer>();
            internal_assert(pc && pc->name == stencil_name);
            internal_assert(!pc->update.defined());

            // create stream operations
            string stream_name = func.name()+".stencil.stream";
            Expr stream_var = Variable::make(Handle(), stream_name);
            Expr stencil_var = Variable::make(Handle(), stencil_name);
            vector<Expr> args({stream_var, stencil_var});
            Stmt read_call = Evaluate::make(Call::make(Handle(), "read_stream", args, Call::Intrinsic));
            Stmt write_call = Evaluate::make(Call::make(Handle(), "write_stream", args, Call::Intrinsic));

            // wrap the old produce and consume into new PC nodes
            Stmt stencil_pc_produce = ProducerConsumer::make(stencil_name, pc->produce, Stmt(), write_call);
            Stmt stencil_pc_consume = ProducerConsumer::make(stencil_name, read_call, Stmt(), pc->consume);

            // re-create Realize nodes for both new PC nodes
            Stmt realize_produce = Realize::make(op->name, op->types, op->bounds, op->condition, stencil_pc_produce);
            Stmt realize_consume = Realize::make(op->name, op->types, op->bounds, op->condition, stencil_pc_consume);

            // create the PC node for stream
            stmt = ProducerConsumer::make(stream_name, realize_produce, Stmt(), realize_consume);
        }
    }

public:
    AddStreamOperationForFunction(Function f) : func(f) {}
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
 *  a target ProducerConsumer node towards the root level.
 *  The pulling stops either at the root or at any Realize node
 */
class ReorderProducerConsumer : public IRVisitor {
    string pc_name_;
    Stmt stmt_;
    SearchIRNode<ProducerConsumer> searcher_;

    using IRVisitor::visit;

    // Cannot pull PC node up across of Realize node
    virtual void visit(const Realize *op) {
        debug(3) << "visit Realize node " << op->name << '\n';
        Stmt new_body = pull_up_pc_node(op->body);

        debug(3) << "Pulling stops at realize node " << op->name << '\n';
        stmt_ = Realize::make(op->name, op->types, op->bounds, op->condition, new_body);
    }

    virtual void visit(const ProducerConsumer *op) {
        debug(3) << "visit PC node " << op->name << '\n';
        if (op->name == pc_name_) {
            // the current node is the target pc node
            stmt_ = op;
        } else {
            bool in_produce = searcher_.search(op->produce, pc_name_);
            bool in_consume = searcher_.search(op->consume, pc_name_);

            internal_assert((in_produce && !in_consume) ||
                            (!in_produce && in_consume) );

            /* Swap the current PC (PC1) and the target PC (PC2).
             * case1: before swap             after swap
             *          PC1                     PC2
             *          / \                     / \
             *         A   PC2                 PC1 C
             *             / \                 / \
             *            B   C               A   B
             *
             * case2: before swap             after swap
             *          PC1                     PC2
             *          / \                     / \
             *        PC2  C                   A  PC1
             *        / \                         / \
             *       A   B                       B   C
             */
            if (in_consume) {
                // case 1

                // pull target up to the root of the current consumer
                Stmt consumer_before_swap = pull_up_pc_node(op->consume);

                const ProducerConsumer *old_pc2 = consumer_before_swap.as<ProducerConsumer>();
                if (old_pc2) {
                    internal_assert(old_pc2 && old_pc2->name == pc_name_);
                    internal_assert(!old_pc2->update.defined()); // cannot handle this case for now

                    Stmt new_pc1 = ProducerConsumer::make(op->name, op->produce, op->update, old_pc2->produce);
                    stmt_ = ProducerConsumer::make(pc_name_, new_pc1, Stmt(), old_pc2->consume);
                } else {
                    stmt_ = ProducerConsumer::make(op->name, op->produce, op->update, consumer_before_swap);
                }
            } else {
                // case 2

                // pull target up to the root of the current produce
                Stmt produce_before_swap = pull_up_pc_node(op->produce);

                const ProducerConsumer *old_pc2 = produce_before_swap.as<ProducerConsumer>();
                if (old_pc2) {
                    internal_assert(old_pc2 && old_pc2->name == pc_name_);
                    internal_assert(!old_pc2->update.defined()); // cannot handle this case for now

                    Stmt new_pc1 = ProducerConsumer::make(op->name, old_pc2->consume, Stmt(), op->consume);
                    stmt_ = ProducerConsumer::make(pc_name_, old_pc2->produce, op->update, new_pc1);
                } else {
                    stmt_ = ProducerConsumer::make(op->name, produce_before_swap, op->update, op->consume);
                }
            }
        }
    }


    /* The rest of Stmt nodes have only one or no child Stmt */
    virtual void visit(const LetStmt *op) {
        debug(3) << "visit LetStmt node " << op->name << '\n';
        Stmt body_before_swap = pull_up_pc_node(op->body);

        const ProducerConsumer *pc = body_before_swap.as<ProducerConsumer>();
        if (pc) {
            /* Swap the current LetSmt and the target PC.
             * e.g. before swap             after swap
             *          LetStmt                  PC
             *             |                    /  \
             *            PC               LetStmt LetStmt
             *            / \                  |     |
             *           P   C                 P     C
             */
            internal_assert(pc && pc->name == pc_name_);
            internal_assert(!pc->update.defined()); // cannot handle this case for now

            Stmt new_produce = LetStmt::make(op->name, op->value, pc->produce);
            Stmt new_consume = LetStmt::make(op->name, op->value, pc->consume);

            stmt_ = ProducerConsumer::make(pc_name_, new_produce, Stmt(), new_consume);
        } else {
            stmt_ = LetStmt::make(op->name, op->value, body_before_swap);
        }
    }


    virtual void visit(const For *op) {
        debug(3) << "visit For node " << op->name << '\n';
        Stmt body_before_swap = pull_up_pc_node(op->body);

        const ProducerConsumer *pc = body_before_swap.as<ProducerConsumer>();
        if (pc) {
            internal_assert(pc && pc->name == pc_name_);
            internal_assert(!pc->update.defined()); // cannot handle this case for now

            Stmt new_produce = For::make(op->name, op->min, op->extent, op->for_type,
                                         op->device_api, pc->produce);
            Stmt new_consume = For::make(op->name, op->min, op->extent, op->for_type,
                                         op->device_api, pc->consume);

            stmt_ = ProducerConsumer::make(pc_name_, new_produce, Stmt(), new_consume);
        } else {
            stmt_ = For::make(op->name, op->min, op->extent, op->for_type,
                              op->device_api, body_before_swap);
        }
    }

    virtual void visit(const Allocate *op) {
        debug(3) << "visit Allocate node " << op->name << '\n';
        internal_assert(false);
    }

    virtual void visit(const IfThenElse *op) {
        // not sure if this a valid transformation across IfThenElse node
        internal_assert(false);
    }

    virtual void visit(const Block *op) {
        // not sure if this a valid transformation across Block node
        internal_assert(false);
    }

    virtual void visit(const AssertStmt *op) {
        // not sure if this a valid transformation across this type node
        internal_assert(false);
    }

    virtual void visit(const Store *op) {
        // not sure if this a valid transformation across this type node
        internal_assert(false);
    }

    virtual void visit(const Provide *op) {
        // not sure if this a valid transformation across this type node
        internal_assert(false);
    }

    virtual void visit(const Free *op) {
        // not sure if this a valid transformation across this type node
        internal_assert(false);
    }

    virtual void visit(const Evaluate *op) {
        // not sure if this a valid transformation across this type node
        internal_assert(false);
    }

public:
    ReorderProducerConsumer(string name) : pc_name_(name) {}

    /** Pull up ProducerConsumer node named NAME up to root,
     *  and push the rest down into either producer or consumer branch
     */
    Stmt pull_up_pc_node(Stmt root) {
        // TODO check that the IR contains the target PC node
        if (root.defined()) {
            root.accept(this);
        } else {
            stmt_ = Stmt();
        }
        return stmt_;
    }

};

// Implements line buffers
// It replace the produce of func.stencil and func.stencil.stream with
// func.stencil_update and func.stencil_update.stream. The latters are
// smaller, which only consist of the new pixels sided in each shift of
// the stencil window.
// A line buffer is instantiated to buffer the stencil_update.stream and
// to generate the stencil.stream
class LinebufferForFunction : public IRMutator {
    Function func;
    StencilSpecs specs;
    Scope<Expr> scope;
    const Scope<Expr> &outer_scope;
    const Region& buffer_bounds;

    using IRMutator::visit;

    void visit(const ProducerConsumer *op) {
        if(op->name != func.name() + ".stencil" &&
           op->name != func.name() + ".stencil.stream") {
            IRMutator::visit(op);
        } else if (op->name == func.name() + ".stencil.stream") {
            // If it is the PC node for func.stencil.stream,
            // we traverse and mutate the produce node while keep the consume node.
            // Then, we create a PC node of func.stencil_update.stream and a line buffer
            Stmt new_produce = mutate(op->produce);

            // create call to instatiate the line buffer
            Expr stream_var = Variable::make(Handle(), func.name() + ".stencil_update.stream");
            Expr stencil_var = Variable::make(Handle(), func.name() + ".stencil.stream");
            vector<Expr> args({stream_var, stencil_var});
            // extract the buffer size, and put it into args
            for(const Range &r : buffer_bounds)
                args.push_back(r.extent);
            Stmt linebuffer_call = Evaluate::make(Call::make(Handle(), "linebuffer", args, Call::Intrinsic));

            // create a new PC node for func.stencil_update.stream
            internal_assert(!op->update.defined());
            Stmt new_pc = ProducerConsumer::make(func.name() + ".stencil_update.stream",
                                                 new_produce, Stmt(), linebuffer_call);

            // create a realizeation of the stencil_update stream
            Region bounds;
            for (StencilDimSpecs dim: specs.dims) {
                Expr extent = make_const(Int(32), dim.step);
                bounds.push_back(Range(0, extent));
            }
            stmt = Realize::make(func.name()+".stencil_update.stream", func.output_types(), bounds, const_true(), new_pc);
            stmt = ProducerConsumer::make(op->name, stmt, Stmt(), op->consume);
        } else {
            // If it is the PC node for func.stencil, then
            // replace it with a new PC node for func.stencil_update

            Stmt produce = mutate(op->produce);  // further mutate produce node
            Stmt update = op->update;
            if(op->update.defined()) {
                // We cannot handle it yet
                internal_assert(false);
                //Stmt update = mutate(op->update);
            }
            // replace the old consumer (which writes to a func.stencil.stream)
            // with a new consumer which writes to func.stencil_update.stream
            const Evaluate *old_consume = op->consume.as<Evaluate>();
            internal_assert(old_consume);
            const Call *old_write_call = old_consume->value.as<Call>();
            internal_assert(old_write_call && old_write_call->name == "write_stream");

            Expr stream_var = Variable::make(Handle(), func.name() + ".stencil_update.stream");
            Expr stencil_var = Variable::make(Handle(), func.name() + ".stencil_update");
            vector<Expr> args({stream_var, stencil_var});
            Stmt consume = Evaluate::make(Call::make(Handle(), "write_stream", args, Call::Intrinsic));

            stmt = ProducerConsumer::make(func.name() + ".stencil_update", produce, update, consume);
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
        // let store.extent = stencil.min_pos(scan.max) + stencil.size - stencil.min_pos(scan.min)
        // let scan_update.extent = store.extent / stencil.step
        // for scan_update.loop_var, 0, scan_update.extent
        //   let scan.loop_var = scan.min + scan_update.loop_var * stencil.step
        //   realize update_stencil[0, stencil.step]
        //   for stencil_update.loop_var, 0, stencil.step
        //     let stencil.loop_var = stencil_update.loop_var
        //     compute stencil_update[stencil.loop_var]


        string stencil_name = func.name() + ".stencil.";
        // check if the for loop is a outer loops (scan.loop_var) that slide
        size_t dim_idx = 0;
        while (dim_idx < specs.dims.size()) {
            if(op->name == specs.dims[dim_idx].loop_var)
                break;
            dim_idx++;
        }

        if(dim_idx < specs.dims.size()) {
            // found a proper outer scan.loop_var
            debug(3) << "find outer loop " << op->name << " to mutate.\n";

            StencilDimSpecs dimspecs = specs.dims[dim_idx];
            internal_assert(dimspecs.loop_var == op->name);
            string new_loop_var_name = func.name() + ".scan_update." + func.args()[dim_idx];
            Expr new_loop_var = Variable::make(Int(32), new_loop_var_name);


            Expr old_max = op->min + op->extent - 1;
            Expr stencil_min_at_old_max = substitute(op->name, old_max, dimspecs.min_pos);
            Expr stencil_min_at_old_min = substitute(op->name, op->min, dimspecs.min_pos);
            Expr store_extent = stencil_min_at_old_max - stencil_min_at_old_min + dimspecs.size;
            store_extent = simplify(expand_expr(store_extent, scope));
            // we need to refer out_scope because the old_extent may be a contant declared outside
            // TODO maybe we can eliminat this by doing one pass of constant substitution, which
            // currently implemented in StorageFolding.cpp
            store_extent = simplify(expand_expr(store_extent, outer_scope));
            debug(3) << "store_extent = " << store_extent << '\n';

            // check the condition for the new loop for sliding the update stencil
            const IntImm *store_extent_int = store_extent.as<IntImm>();
            internal_assert(store_extent_int);
            if (store_extent_int->value % dimspecs.step != 0) {
                // we cannot handle this scenario yet
                debug(3) << "Line buffer extent (" << store_extent_int->value
                         << ") is not divisible by the stencil step " << dimspecs.step << '\n';
                internal_assert(false);
            }
            int new_extent_int = store_extent_int->value / dimspecs.step;

            // create a let statement for the old_loop_var
            {
                // TODO fix the bug in this transformation
                // I think the tranformation doesn't works when the following conditions
                // are not satisfied.
                internal_assert(is_one(simplify(expand_expr(op->min, outer_scope) == 0)));
                internal_assert(is_one(simplify(dimspecs.step == 1)));
            }
            string old_var_name = op->name;
            Expr old_var_value = op->min + new_loop_var * dimspecs.step;

            // traversal down into the body
            scope.push(old_var_name, old_var_value);
            Stmt new_body = mutate(op->body);
            scope.pop(old_var_name);

            new_body = LetStmt::make(old_var_name, old_var_value, new_body);
            stmt = For::make(new_loop_var_name, 0, new_extent_int, op->for_type, op->device_api, new_body);
        } else if (starts_with(op->name, stencil_name)) {
            // found a inner loop
            debug(3) << "find stencil loop " << op->name << " to mutate.\n";
            string old_var_name = op->name;
            string dim_name = op->name.substr(stencil_name.size(), old_var_name.size() - stencil_name.size());
            string new_var_name = func.name() + ".stencil_update." + dim_name;
            Expr new_var = Variable::make(Int(32), new_var_name);

            // look for dim_name in stencil specs
            size_t i = 0;
            while(i < func.args().size()) {
                if(dim_name == func.args()[i])
                    break;
                i++;
            }
            internal_assert(i < specs.dims.size());
            int new_extent_int = specs.dims[i].step;

            // create a let statement for the old_loop_var
            Expr old_var_value = new_var;

            // traversal down into the body
            scope.push(old_var_name, old_var_value);
            Stmt new_body = mutate(op->body);
            scope.pop(old_var_name);

            new_body = LetStmt::make(old_var_name, old_var_value, new_body);
            stmt = For::make(new_var_name, 0, new_extent_int, op->for_type, op->device_api, new_body);
        } else {
            // otherwise, keep traversal down
            IRMutator::visit(op);
        }
    }

    void visit(const Realize *op) {
        if (op->name != func.name() + ".stencil") {
            IRMutator::visit(op);
        } else {
            // If it is the Realize node for func.stencil, then
            // replace it with a new Realize node for func.stencil_update

            Region new_bounds(op->bounds.size());
            internal_assert(op->bounds.size() == specs.dims.size());

            // Mutate the bounds
            for (size_t i = 0; i < op->bounds.size(); i++) {
                internal_assert(is_const(op->bounds[i].min, 0));
                Expr new_extent = make_const(Int(32), specs.dims[i].step);
                new_bounds[i] = Range(0, new_extent);
            }

            Stmt new_body = mutate(op->body);
            stmt = Realize::make(func.name() + ".stencil_update", op->types, new_bounds,
                                 const_true(), new_body);
        }
    }

    void visit(const Provide *op) {
        if(op->name != func.name() + ".stencil") {
            IRMutator::visit(op);
        } else {
            // If it is the provide node for func.stencil, then
            // replace it with a new provide node for func.stencil_update

            // Replace the provide node of func with provide node of func.stencil
            string new_name = func.name() + ".stencil_update";
            vector<Expr> new_args(op->args.size());

            // Replace the arguments. e.g.
            //   func.s0.x -> func.stencil.x
            for (size_t i = 0; i < func.args().size(); i++) {
                string old_arg_name = func.name() + ".stencil." + func.args()[i];
                string new_arg_name = func.name() + ".stencil_update." + func.args()[i];
                internal_assert(is_one(simplify(op->args[i] == Variable::make(Int(32), old_arg_name))));
                Expr new_arg = Variable::make(Int(32), new_arg_name);
                new_args[i] = new_arg;
            }
            stmt = Provide::make(new_name, op->values, new_args);
        }
    }

    void visit(const Let *op) {
        scope.push(op->name, simplify(expand_expr(op->value, scope)));
        Expr new_body = mutate(op->body);
        if (new_body.same_as(op->body)) {
            expr = op;
        } else {
            expr = Let::make(op->name, op->value, new_body);
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
    LinebufferForFunction(Function f, StencilSpecs s, const Scope<Expr> &sc, const Region &b)
        : func(f), specs(s), outer_scope(sc), buffer_bounds(b) { internal_assert(s.is_valid); }
};


// Perform streaming optimization for all functions
class StreamOpt : public IRMutator {
    const map<string, Function> &env;
    Scope<Expr> scope;

    using IRMutator::visit;

    void visit(const Realize *op) {
        // Find the args for this function
        map<string, Function>::const_iterator iter = env.find(op->name);

        // If it's not in the environment it's some anonymous
        // realization that we should skip (e.g. an inlined reduction)
        if (iter == env.end()) {
            IRMutator::visit(op);
            return;
        }

        // If the Function in question has the same compute_at level
        // as its store_at level, skip it.
        const Schedule &sched = iter->second.schedule();
        if (sched.compute_level() == sched.store_level()) {
            IRMutator::visit(op);
            return;
        }

        // If the Function is not scheduled as a stream, skip it
        if (!sched.is_stream()) {
            IRMutator::visit(op);
            return;
        }

        // Here we want to do post-order traversal, as opposed to pre-order in class SlidingWindow
        // because we want to transform each Func from the output of the pipeline backward
        Stmt new_body = mutate(op->body);


        debug(3) << "Doing stencil window analysis on realization of " << op->name << "\n";
        Function func = iter->second;
        AnalysisStencilFunction checker(func);
        StencilSpecs specs = checker.getSpecs(new_body);
        debug(3) <<  specs << '\n';

        if(specs.is_valid) {
            new_body = CreateStencilForFunction(func, specs).mutate(new_body);
            debug(3) << "IR after CreateStencilForFunction pass on Function " << func.name()
                     << ":\n" << new_body << '\n';

            new_body = AddStreamOperationForFunction(func).mutate(new_body);
            debug(3) << "IR after AddStreamOperationForFunction pass on Function " << func.name()
                     << ":\n" << new_body << '\n';

            ReorderProducerConsumer reorder(op->name+".stencil.stream");
            new_body = reorder.pull_up_pc_node(new_body);

            // create a realizeation of the stencil stream
            Region bounds;
            for (StencilDimSpecs dim: specs.dims) {
                Expr extent = make_const(Int(32), dim.size);
                bounds.push_back(Range(0, extent));
            }
            new_body = Realize::make(func.name()+".stencil.stream",
                                     func.output_types(), bounds, const_true(), new_body);

            debug(3) << "IR after ReorderProducerConsumer pass on Function " << func.name()
                     << ":\n" << new_body << '\n';


            new_body = LinebufferForFunction(func, specs, scope, op->bounds).mutate(new_body);
            debug(3) << "IR after LinebufferForFunction pass on Function " << func.name()
                     << ":\n" << new_body << '\n';

            // we don't need the old realization node, since the storage is implicitly in the line buffer
            stmt = new_body;

        } else {
            stmt = op;
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
    StreamOpt(const map<string, Function> &e) : env(e) {}

};



Stmt stream_opt(Stmt s, const map<string, Function> &env) {
    return StreamOpt(env).mutate(s);
}

}
}
