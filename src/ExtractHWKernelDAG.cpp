#include "ExtractHWKernelDAG.h"

#include <iostream>
#include "Func.h"
#include "IRVisitor.h"
#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Debug.h"
#include "Substitute.h"
#include "IRPrinter.h"
#include "Simplify.h"
#include "Bounds.h"
#include "ExprUsesVar.h"
#include "Var.h"

#include <algorithm>

namespace Halide {
namespace Internal {

using std::string;
using std::map;
using std::set;
using std::vector;
using std::ostream;


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

// return if the Expr is a integer multiple of a variable
bool is_varmulcont(Expr e) {
    const Mul *mul = e.as<Mul>();
    if(mul) {
        if (is_const(mul->a) || is_const(mul->b)) {
            return true;
        }
    }
    return false;
}

class SubstituteInConstants : public IRMutator {
    using IRMutator::visit;

    Scope<Expr> scope;
    void visit(const LetStmt *op) {
        Expr value = simplify(mutate(op->value));

        Stmt body;
        if (is_const(value) || is_varmulcont(value)) {
            scope.push(op->name, value);
            body = mutate(op->body);
            scope.pop(op->name);
        } else {
            body = mutate(op->body);
        }

        if (body.same_as(op->body) && value.same_as(op->value)) {
            stmt = op;
        } else {
            stmt = LetStmt::make(op->name, value, body);
        }
    }

    void visit(const Variable *op) {
        if (scope.contains(op->name)) {
            expr = scope.get(op->name);
        } else {
            expr = op;
        }
    }
};

class FiniteDifference : public IRMutator {
    Scope<Expr> scope;
    string var;

    Expr brute_force(Expr e) {
        return substitute(var, (Variable::make(Int(32), var)) + 1, e) - e;
    }

    using IRMutator::visit;

    void visit(const IntImm *op) {
        expr = make_zero(op->type);
    }

    void visit(const UIntImm *op) {
        expr = make_zero(op->type);
    }

    void visit(const FloatImm *op) {
        expr = make_zero(op->type);
    }

    void visit(const Cast *op) {
        expr = brute_force(op);
    }

    void visit(const Variable *op) {
        if (op->name == var) {
            expr = make_one(op->type);
        } else if (scope.contains(op->name)) {
            expr = scope.get(op->name);
        } else {
            expr = make_zero(op->type);
        }
    }

    void visit(const Add *op) {
        expr = mutate(op->a) + mutate(op->b);
    }

    void visit(const Sub *op) {
        expr = mutate(op->a) - mutate(op->b);
    }

    void visit(const Mul *op) {
        Expr da = mutate(op->a), db = mutate(op->b);
        expr = op->a * db + da * op->b + da * db;
    }

    void visit(const Div *op) {
        expr = brute_force(op);
    }

    void visit(const Mod *op) {
        expr = brute_force(op);
    }

    void visit(const Min *op) {
        expr = select(op->a < op->b, mutate(op->a), mutate(op->b));
    }

    void visit(const Max *op) {
        expr = select(op->a > op->b, mutate(op->a), mutate(op->b));
    }

    void visit(const Select *op) {
        expr = select(op->condition, mutate(op->true_value), mutate(op->false_value));
    }

    void visit(const Load *op) {
        expr = brute_force(op);
    }

    void visit(const Call *op) {
        expr = brute_force(op);
    }

    void visit(const Let *op) {
        scope.push(op->name, mutate(op->value));
        expr = mutate(op->body);
        scope.pop(op->name);
    }
public:
    FiniteDifference(string v) : var(v) {}
};

// TODO remove this helper function
// See change of StorageFolding.cpp in commit b89e1d6e
Expr finite_difference(Expr expr, const string &var) {
    return FiniteDifference(var).mutate(expr);
}

}

bool operator==(const StencilDimSpecs &left, const StencilDimSpecs &right) {
    return left.loop_var == right.loop_var &&
        is_one(simplify(left.min_pos == right.min_pos)) &&
        left.size == right.size &&
        left.step == right.step;
}

ostream &operator<<(ostream &out, const StencilDimSpecs &dim) {
    out << "[" << dim.min_pos << ", "
        << dim.size << ", looping "<< dim.loop_var << " step " << dim.step << "]"
        << " over " << "[" << dim.store_bound.min << ", " << dim.store_bound.max << "]\n";
    return out;
}

template <typename T>
ostream &operator<<(ostream &out, const vector<T> &v) {
    out << "{";
    for (size_t i = 0; i < v.size(); i++) {
        out << v[i];
        if (i != v.size() - 1)
            out << ", ";
    }
    out << "}";
    return out;
}

template <typename Iter>
Iter next_it(Iter iter)
{
    return ++iter;
}

template <typename T>
ostream &operator<<(ostream &out, const set<T> &s) {
    out << "{";
    for (auto it = s.begin(); it != s.end(); ++it) {
        out << *it;
        if (next_it(it) != s.end())
            out << ", ";
    }
    out << "}";
    return out;
}

ostream &operator<<(ostream &out, const HWKernel &k) {
    out << "HWKernel " << k.name << " takes inputs " << k.input_streams << "\n";
    if(k.is_inlined) {
        out << "[inlined]\n";
    }
    for (size_t i = 0; i < k.dims.size(); i++)
        out << "  dim " << k.func.args()[i] << ": " << k.dims[i] << '\n';

    if (k.consumer_stencils.size() > 0) {
        for (const auto &p : k.consumer_stencils) {
            out << "consumer " << p.first << '\n';
            for (size_t i = 0; i < p.second.size(); i++)
                out << "  dim " << k.func.args()[i] << ": " << p.second[i] << '\n';
        }
    }
    return out;
}

ostream &operator<<(ostream &out, const HWTap &t) {
    out << "HWTap " << t.name << " from " << (t.is_func ? "function" : "paramter") << "\n";
    for (size_t i = 0; i < t.dims.size(); i++)
        out << "  dim " << i << ": " << t.dims[i] << '\n';
    return out;
}

vector<StencilDimSpecs>
extract_stencil_specs(Box box, const set<string> &scan_loops,
                      const Scope<Expr> &stencil_bounds,
                      const Scope<Expr> &store_bounds) {
    vector<StencilDimSpecs> res;
    for(size_t i = 0; i < box.size(); i++) {
        StencilDimSpecs dim_specs;
        Expr min = simplify(expand_expr(box[i].min, stencil_bounds));
        Expr max = simplify(expand_expr(box[i].max, stencil_bounds));
        Expr extent = simplify(expand_expr(max - min + 1, stencil_bounds));

        dim_specs.min_pos = min;
        const IntImm *extent_int = extent.as<IntImm>();
        internal_assert(extent_int) << "stencil window extent ("
                                    << extent << ") is not a const.\n";
        dim_specs.size = extent_int->value;

        Expr store_min = simplify(expand_expr(box[i].min, store_bounds));
        Expr store_max = simplify(expand_expr(box[i].max, store_bounds));
        dim_specs.store_bound = Interval(store_min, store_max);

        dim_specs.step = dim_specs.size;
        dim_specs.loop_var = "undef";
        // look for loop var that slides along this dimensions
        //for (size_t j = 0; j < scan_loops.size(); j++) {
        for (const string& scan_loop : scan_loops) {
            if (expr_uses_var(min, scan_loop)) {
                dim_specs.loop_var = scan_loop;
                Expr step = simplify(finite_difference(min, dim_specs.loop_var));
                const IntImm *step_int = step.as<IntImm>();
                internal_assert(step_int) << "stencil window step is not a const.\n";
                dim_specs.step = step_int->value;
                break;
            }
        }
        res.push_back(dim_specs);
    }
    return res;
}

// TODO review the implementation of this function
// FIXME there is bug if the merged store bounds of input kerenel is larger than SW implementation allocates
vector<StencilDimSpecs>
merge_consumer_stencils(map<string, vector<StencilDimSpecs> > &consumer_stencils) {
    vector<StencilDimSpecs> res;
    internal_assert(consumer_stencils.size() > 0);
    const vector<StencilDimSpecs> &first_stencil = consumer_stencils.begin()->second;
    size_t num_of_dims = first_stencil.size();
    // First pass, figure out the size and min_pos of stencil windows
    // that encloses all consumer stencil windows.
    // Also does checks on assumptions regarding step and loop_var
    for (size_t i = 0; i < num_of_dims; i++) {
        StencilDimSpecs dim_specs;
        dim_specs.size = first_stencil[i].size;
        dim_specs.step = first_stencil[i].step;
        dim_specs.min_pos = first_stencil[i].min_pos;
        dim_specs.loop_var = first_stencil[i].loop_var;
        for (const auto& p : consumer_stencils) {
            internal_assert(p.second.size() == num_of_dims);
            const StencilDimSpecs &consumer_dim = p.second[i];
            // check the following stencil constraints
            // 1. the steps of all consumers are the same
            // 2. the loop vars of all consumers are the same
            internal_assert(consumer_dim.loop_var == dim_specs.loop_var);
            internal_assert(dim_specs.loop_var == "undef"
                            || consumer_dim.step == dim_specs.step); // step is valid only if loop_var is not "undef"

            // compute the max size of the stencil window
            dim_specs.size = dim_specs.size > consumer_dim.size ? dim_specs.size :
                consumer_dim.size;
            // compute the globally minimum position
            dim_specs.min_pos = simplify(min(dim_specs.min_pos, consumer_dim.min_pos));
        }
        if (dim_specs.loop_var == "undef")
            dim_specs.step = dim_specs.size;
        res.push_back(dim_specs);
    }

    // Second pass, update the min_pos and store_bounds of each consumer stencil
    // if the size of stencil windows is different
    for (auto& p : consumer_stencils) {
        for (size_t i = 0; i < num_of_dims; i++) {
            StencilDimSpecs &consumer_dim = p.second[i];
            if (consumer_dim.size != res[i].size) {
                int size_difference = res[i].size - consumer_dim.size;
                internal_assert(is_const(simplify(consumer_dim.min_pos - res[i].min_pos)));
                int pos_difference = *as_const_int(simplify(consumer_dim.min_pos - res[i].min_pos));
                internal_assert(size_difference > 0 && pos_difference >= 0);

                // we want to left shift the consumer stencil window as large as possible,
                // but the enlarged new windows at the new position still covers the original
                // window region.
                int left_shift_amount = pos_difference < size_difference ? pos_difference : size_difference;
                int right_enlarged_amount = size_difference - left_shift_amount;

                // Shift min_pos and store_bound.min
                consumer_dim.min_pos = simplify(consumer_dim.min_pos - left_shift_amount);
                consumer_dim.store_bound.min = simplify(consumer_dim.store_bound.min - left_shift_amount);

                // Shift store_bound.max
                consumer_dim.store_bound.max = simplify(consumer_dim.store_bound.max + right_enlarged_amount);
                // increase the consumer window size
                consumer_dim.size = res[i].size;
            }
        }
    }

    // Third pass, figure out the store bounds that encloses all consumer store bounds
    for (size_t i = 0; i < num_of_dims; i++) {
        StencilDimSpecs &dim_specs = res[i];
        dim_specs.store_bound = first_stencil[i].store_bound;
        for (const auto& p : consumer_stencils) {
            internal_assert(p.second.size() == num_of_dims);
            const StencilDimSpecs &consumer_dim = p.second[i];
            // compute the union of the store bounds
            Expr store_min = simplify(min(dim_specs.store_bound.min, consumer_dim.store_bound.min));
            Expr store_max = simplify(max(dim_specs.store_bound.max, consumer_dim.store_bound.max));
            dim_specs.store_bound = Interval(store_min, store_max);
        }
    }
    return res;
}

// Build calculate the input streams for each HWKernel in dag
void calculate_input_streams(HWKernelDAG &dag) {
    for (auto &p : dag.kernels) {
        const HWKernel &kernel = p.second;
        if (kernel.is_inlined) {
            // skip inlined kernels, for they are not streamed into their consumers
            continue;
        }
        // save the current kernel in the input_stream fields of all its consumers
        for (const auto &p : kernel.consumer_stencils) {
            internal_assert(dag.kernels.count(p.first));
            HWKernel &consumer = dag.kernels[p.first];
            consumer.input_streams.push_back(kernel.name);
        }
    }
}

class BuildDAGForFunction : public IRVisitor {
    Function func;
    const map<string, Function> &env;
    const vector<BoundsInference_Stage> &inlined_stages;
    const LoopLevel &compute_level;
    const LoopLevel &store_level;

    HWKernelDAG dag;
    bool is_scan_loops;
    set<string> scan_loops; // collection of loops vars that func windows scan along
    map<string, Expr> loop_mins, loop_maxes;


    using IRVisitor::visit;

    void visit(const ProducerConsumer *op) {
        // match store level at PC node in case the looplevel is outermost
        if (op->is_producer &&
            op->name == func.name() &&
            store_level.match(LoopLevel(func, Var::outermost()))) {
            is_scan_loops = true;
        }
        IRVisitor::visit(op);
    }

    void visit(const For *op) {
        // scan loops are loops between store level (exclusive) and
        // the compute level (inclusive) of the accelerated function
        if (is_scan_loops && starts_with(op->name, func.name() + ".")) {
            debug(3) << "added loop " << op->name << " to scan loops.\n";
            scan_loops.insert(op->name);
            loop_mins[op->name] = op->min;
            loop_maxes[op->name] = simplify(op->min + op->extent - 1);
        }
        if (store_level.match(op->name)) {
            is_scan_loops = true;
        }
        if (compute_level.match(op->name)) {
            is_scan_loops = false;
        }

        // Recurse
        IRVisitor::visit(op);

        if (compute_level.match(op->name)) {
            // Figure out how much of the accelerated func we're producing for each iteration
            Box box = box_provided(op->body, func.name());

            // save the bounds values in scope
            Scope<Expr> stencil_bounds, store_bounds;
            for (int i = 0; i < func.dimensions(); i++) {
                string stage_name = func.name() + ".s0." + func.args()[i];
                stencil_bounds.push(stage_name + ".min", box[i].min);
                stencil_bounds.push(stage_name + ".max", box[i].max);
                store_bounds.push(stage_name + ".min", substitute(loop_mins, box[i].min));
                store_bounds.push(stage_name + ".max", substitute(loop_maxes, box[i].max));
            }

            vector<StencilDimSpecs> dims = extract_stencil_specs(box, scan_loops, stencil_bounds, store_bounds);
            // manually compute store_bound for the output
            // kernel because extract_stencil_specs() cannot
            // extract it correctly
            for (size_t i = 0; i < box.size(); i++) {
                Expr store_min = substitute(loop_mins, box[i].min);
                Expr store_max = substitute(loop_maxes, box[i].max);
                dims[i].store_bound = Interval(store_min, store_max);
            }
            HWKernel k(func, func.name());
            k.is_output = true;
            k.dims = dims;
            dag.kernels[k.name] = k;

            debug(3) << k << "\n";

            // Figure out bounds of each tap paramter
            for (const auto &p : func.schedule().tap_params()) {
                HWTap tap;
                tap.name = p.first;
                tap.is_func = false;
                tap.param = p.second;
                internal_assert(tap.param.is_buffer());
                for (int i = 0; i < tap.param.dimensions(); i++) {
                    StencilDimSpecs dim_specs;
                    dim_specs.min_pos = tap.param.min_constraint(i);
                    Expr extent = tap.param.extent_constraint(i);
                    const IntImm *extent_int = extent.as<IntImm>();
                    internal_assert(extent_int) << "tap window extent ("
                                                << extent << ") is not a const.\n";
                    dim_specs.size = extent_int->value;
                    dim_specs.step = dim_specs.size;
                    dim_specs.loop_var = "undef";
                    tap.dims.push_back(dim_specs);
                }
                dag.taps[p.first] = tap;
            }

            // Figure out bounds of each tap function
            for (const auto &p : func.schedule().tap_funcs()) {
                // TODO check if the required function value has been fully evaluated
                HWTap tap;
                tap.name = p.first;
                tap.is_func = true;
                tap.func = p.second;
                Box box = box_required(op->body, p.first);
                tap.dims = extract_stencil_specs(box, scan_loops, stencil_bounds, store_bounds);
                dag.taps[p.first] = tap;
            }

            // Figure out how much of each func in the pipeline we're producing
            // do this from output of the pipeline to inputs
            // 'inlined_stages' is sorted, so we do it straight backward

            // first fast forward to the output stage
            int i = inlined_stages.size() - 1;
            while (i >= 0) {
                if (inlined_stages[i].name == func.name()) {
                    break;
                }
                i--;
            }
            // scan through each stage before the output stage
            i--;
            while (i >= 0) {
                const BoundsInference_Stage &stage = inlined_stages[i];
                Function cur_func = env.find(stage.name)->second;

                if (cur_func.schedule().is_hw_kernel() ) {
                    debug(3) << "func " << stage.name << " stage " << stage.stage
                             << " is a hw kernel.\n";
                    HWKernel cur_kernel(cur_func, stage.name);

                    // if cur_func is non-pure function, we may already create
                    // an HWKernel struct for it when we visit its update definition stages
                    if (dag.kernels.count(stage.name))
                        cur_kernel = dag.kernels[stage.name];

                    // figure out wether it is a line buffered kernel or an inlined kernel
                    if (func.schedule().accelerate_inputs().count(stage.name) ||
                        (compute_level == cur_func.schedule().compute_level() &&
                         store_level == cur_func.schedule().store_level())) {
                        // it is a linebuffered kernel
                        cur_kernel.is_inlined = false;
                        debug(3) << "[buffered]\n";
                    } else {
                        // It is a function "inlined" into a buffered kernel
                        cur_kernel.is_inlined = true;
                        debug(3) << "[inlined]\n";
                    }

                    // merge the bounds of consumers if they are inlined into the same buffered kernel
                    map<string, Box> consumer_boxes;
                    for (size_t j = 0; j < stage.consumers.size(); j++) {
                        const BoundsInference_Stage &consumer = inlined_stages[stage.consumers[j]];
                        const Box &b = stage.bounds.find(make_pair(consumer.name,
                                                                   consumer.stage))->second;

                        debug(3) << "box for " << consumer.name << " stage " << consumer.stage << ":\n";
                        for (size_t k = 0; k < b.size(); k++)
                            debug(3) << "  [" << simplify(b[k].min) << ", "
                                     << simplify(b[k].max) << "]\n";

                        string buffered_kernel_name;
                        if (consumer.name == cur_kernel.name) {
                            // the consumer is the kernel itself, meaning the kernel is non-pure
                            // TODO check if there is something to be done in this branch
                            buffered_kernel_name = cur_kernel.name;
                        } else {
                            internal_assert(dag.kernels.count(consumer.name) == 1);
                            const HWKernel &consumer_kernel = dag.kernels[consumer.name];
                            if (!consumer_kernel.is_inlined) {
                                buffered_kernel_name = consumer_kernel.name;
                            } else {
                                internal_assert(consumer_kernel.consumer_stencils.size() == 1)
                                    << "The inlined kernel " << consumer.name << "has more than one consumer.\n";
                                buffered_kernel_name = consumer_kernel.consumer_stencils.begin()->first;
                            }
                        }

                        // merge the bounds given the name of the buffered kernel
                        if (consumer_boxes.count(buffered_kernel_name)) {
                            merge_boxes(consumer_boxes[buffered_kernel_name], b);
                        } else {
                            consumer_boxes[buffered_kernel_name] = b;
                        }
                    }

                    // extract the stencil specs for each merged consumer bounds
                    for (const auto &p : consumer_boxes) {
                        // insert the consumer name if it is not the kernel function itself
                        // FIXME I am note sure whether this IF condition is correct
                        if (p.first != cur_kernel.name) {
                            vector<StencilDimSpecs> consumer_stencil;
                            consumer_stencil = extract_stencil_specs(p.second, scan_loops, stencil_bounds, store_bounds);
                            debug(3) << "extracted stencil: " << consumer_stencil << "\n";
                            cur_kernel.consumer_stencils[p.first] = consumer_stencil;

                            // If there is schedule of the fifo depth, use the value from
                            // schedule; otherwise, use zero as default.
                            if (cur_func.schedule().fifo_depths().count(p.first)) {
                                cur_kernel.consumer_fifo_depths[p.first]
                                    = cur_func.schedule().fifo_depths().find(p.first)->second;
                            } else {
                                cur_kernel.consumer_fifo_depths[p.first] = 0;
                            }
                        }
                    }

                    // calculate the stencil specs of the cur_kernel
                    cur_kernel.dims = merge_consumer_stencils(cur_kernel.consumer_stencils);

                    if (!cur_kernel.is_inlined) {
                        // check consistency between min_pos and store_bounds.min
                        // TODO bring this check to a more expressive level
                        for (size_t i = 0; i < cur_kernel.dims.size(); i++) {
                            const StencilDimSpecs &dim = cur_kernel.dims[i];
                            internal_assert(is_one(simplify(dim.store_bound.min == substitute(loop_mins, dim.min_pos))));
                        }
                    }

                    // save the bounds values in scope
                    for (int i = 0; i < cur_func.dimensions(); i++) {
                        string arg = cur_func.name() + ".s" + std::to_string(stage.stage) + "." + cur_func.args()[i];
                        // calculate the max position of the stencil windows
                        Expr stencil_max;
                        if (cur_kernel.is_inlined) {
                            stencil_max = simplify(cur_kernel.dims[i].min_pos + cur_kernel.dims[i].size - 1);
                        } else {
                            // NOTE we use 'step' here since r we will have line buffer
                            stencil_max = simplify(cur_kernel.dims[i].min_pos + cur_kernel.dims[i].step - 1);
                        }
                        stencil_bounds.push(arg + ".min", cur_kernel.dims[i].min_pos);
                        stencil_bounds.push(arg + ".max", stencil_max);
                        store_bounds.push(arg + ".min", cur_kernel.dims[i].store_bound.min);
                        store_bounds.push(arg + ".max", cur_kernel.dims[i].store_bound.max);
                    }

                    // push RDom value to stencil_bounds and store_bounds
                    if(stage.stage > 0) {
                        //const Definition &r = cur_func.updates()[stage.stage - 1];
                        Schedule update_schedule = cur_func.update_schedule(stage.stage - 1);
                        // TODO check the sliding dimensions are all pure, referring to
                        // BoundsInference::Stage::define_bounds()
                        //if (r.domain.defined()) {
                        for (ReductionVariable i : update_schedule.rvars()) {
                            string arg = cur_func.name() + ".s" + std::to_string(stage.stage) + "." + i.var;
                            internal_assert(is_const(i.min));
                            internal_assert(is_const(i.extent));
                            Expr min = i.min;
                            Expr max = simplify(i.extent + i.min - 1);
                            stencil_bounds.push(arg + ".min", min);
                            stencil_bounds.push(arg + ".max", max);
                            store_bounds.push(arg + ".min", min);
                            store_bounds.push(arg + ".max", max);
                        }
                    }

                    debug(3) << cur_kernel << "\n\n";
                    // update dag and hw_kernel_stages
                    dag.kernels[cur_kernel.name] = cur_kernel;
                }
                i--;
            }
        }
    }

public:
    BuildDAGForFunction(Function f, const map<string, Function> &e,
                        const vector<BoundsInference_Stage> &s)
        : func(f), env(e), inlined_stages(s),
          compute_level(f.schedule().accelerate_compute_level()),
          store_level(f.schedule().accelerate_store_level()),
          is_scan_loops(false) {}

    HWKernelDAG build(Stmt s) {
        s.accept(this);
        dag.name = func.name();
        dag.loop_vars = scan_loops;
        dag.input_kernels = func.schedule().accelerate_inputs(); // TODO we don't use it later
        dag.compute_level = compute_level;
        dag.store_level = store_level;
        calculate_input_streams(dag);
        /*
        debug(0) << "after building producer pointers:" << "\n";
        for (const auto &p : dag.kernels)
            debug(0) << p.second << "\n";
        for (const auto &p : dag.taps)
            debug(0) << p.second << "\n";
        */
        return dag;
    }
};

Stmt extract_hw_kernel_dag(Stmt s, const map<string, Function> &env,
                           const vector<BoundsInference_Stage> &inlined_stages,
                           vector<HWKernelDAG> &dags) {
    s = SubstituteInConstants().mutate(s);
    debug(4) << "IR after simplification:\n" << s << "\n";

    // for each accelerated function, build a dag of HW kernels for it
    for (const auto &p : env) {
        Function func = p.second;
        if(!func.schedule().is_accelerated())
            continue;
        debug(3) << "Found accelerate function " << func.name() << "\n";
        debug(3) << func.schedule().store_level().to_string() << "\n";
        BuildDAGForFunction builder(func, env, inlined_stages);
        dags.push_back(builder.build(s));
    }
    return s;
}

}
}
