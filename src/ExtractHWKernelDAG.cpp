#include "ExtractHWKernelDAG.h"

#include <iostream>
#include "IRVisitor.h"
#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Debug.h"
#include "Substitute.h"
#include "IRPrinter.h"
#include "Simplify.h"
#include "Derivative.h"
#include "Bounds.h"

#include <algorithm>

namespace Halide {
namespace Internal {

using std::string;
using std::map;
using std::set;
using std::vector;
using std::ostream;


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

}

bool operator==(const StencilDimSpecs &left, const StencilDimSpecs &right) {
    return left.loop_var == right.loop_var &&
        is_one(simplify(left.min_pos == right.min_pos)) &&
        left.size == right.size &&
        left.step == right.step;
}

ostream &operator<<(ostream &out, const StencilDimSpecs &dim) {
    out << "[" << dim.min_pos << ", "
        << dim.size << ", looping over "<< dim.loop_var << " step " << dim.step << "]";
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
    out << "HWKernel " << k.name << " consumes " << k.producers << "\n"
        << "\tconsumed by " << k.consumers << "\n";
    if(k.is_buffered) {
        out << "[buffered] consumes " << k.buffered_producers << "\n"
        << "\tconsumed by " << k.buffered_consumers << "\n";
    }
    for (size_t i = 0; i < k.dims.size(); i++)
        out << "  dim " << k.func.args()[i] << ": " << k.dims[i] << '\n';

    //internal_assert(k.consumer_dims.size() == k.buffered_consumers.size());
    if (k.consumer_dims.size() > 1) {
        for (const auto &p : k.consumer_dims) {
            out << "consumer " << p.first << '\n';
            for (size_t i = 0; i < p.second.size(); i++)
                out << "  dim " << k.func.args()[i] << ": " << p.second[i] << '\n';
        }
    }
    return out;
}

vector<StencilDimSpecs>
extract_stencil_specs(Box box, const vector<string> scan_loops,
                      const Scope<Expr> &scope = Scope<Expr>()) {
    vector<StencilDimSpecs> res;
    for(size_t i = 0; i < box.size(); i++) {
        StencilDimSpecs dim_specs;
        Expr min = simplify(expand_expr(box[i].min, scope));
        Expr max = simplify(expand_expr(box[i].max, scope));
        Expr extent = simplify(expand_expr(max - min + 1, scope));

        dim_specs.min_pos = min;
        const IntImm *extent_int = extent.as<IntImm>();
        internal_assert(extent_int) << "stencil window extent ("
                                    << extent << ") is not a const.\n";
        dim_specs.size = extent_int->value;

        dim_specs.step = dim_specs.size;
        dim_specs.loop_var = "undef";
        // look for loop var that slides along this dimensions
        for (size_t j = 0; j < scan_loops.size(); j++) {
            if (expr_depends_on_var(min, scan_loops[j])) {
                dim_specs.loop_var = scan_loops[j];
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

// Build the producer pointers for each HWKernel in dag
void build_producer_pointers(HWKernelDAG &dag) {
    for (auto &p : dag.kernels) {
        const HWKernel &kernel = p.second;
        for (const string &s : kernel.consumers) {
            debug(3) << "Func " << kernel.name << "'s consumer " << s << "\n";

            internal_assert(dag.kernels.count(s));
            HWKernel &consumer = dag.kernels[s];
            consumer.producers.push_back(kernel.name);
        }
    }
}

// Returned a set of producer (input) kernels, which
// are set buffered (i.e. optimized as stream in this pass).
// We recursively search the producer kernels until we find
// an buffered kernels
vector<string> get_buffered_producers(const HWKernel &k, const HWKernelDAG &dag) {
    vector<string> res;
    for (const string &s : k.producers) {
        const auto it = dag.kernels.find(s);
        internal_assert(it != dag.kernels.end());
        const HWKernel &producer = it->second;
        if (producer.is_buffered) {
            res.push_back(s);
        } else {
            // recurse
            vector<string> buffered_producers = get_buffered_producers(producer, dag);
            res.insert(res.end(), buffered_producers.begin(), buffered_producers.end());
        }
    }
    return res;
}

// Build the buffered_producers and buffered_consumers pointers for each HWKernel in dag
void build_buffered_producer_and_consumer_pointers(HWKernelDAG &dag) {
    for (auto &p : dag.kernels) {
        HWKernel &kernel = p.second;
        if (kernel.is_buffered) {
            kernel.buffered_producers = get_buffered_producers(kernel, dag);
            for (const string &s : kernel.buffered_producers) {
                internal_assert(dag.kernels.count(s));
                HWKernel &consumer = dag.kernels[s];
                internal_assert(consumer.is_buffered);
                consumer.buffered_consumers.push_back(kernel.name);
            }
        }
    }
}

void erase_duplicates(vector<string> &vec) {
    sort(vec.begin(), vec.end());
    vec.erase(unique(vec.begin(), vec.end()), vec.end());
}

// erase the duplicated points in producers, consumers, buffered_producers and buffered_consumers
void erase_duplicated_pointers(HWKernelDAG &dag) {
    for (auto &p : dag.kernels) {
        HWKernel &kernel = p.second;
        erase_duplicates(kernel.consumers);
        erase_duplicates(kernel.producers);
        erase_duplicates(kernel.buffered_consumers);
        erase_duplicates(kernel.buffered_producers);
    }
}

class BuildDAGForFunction : public IRVisitor {
    Function func;
    const map<string, Function> &env;
    const vector<BoundsInference_Stage> &inlined_stages;

    HWKernelDAG dag;
    bool is_scan_loops;
    vector<string> scan_loops; // collection of loops vars that func windows scan along


    using IRVisitor::visit;

    void visit(const For *op) {
        const LoopLevel &compute_level = func.schedule().compute_level();
        const LoopLevel &store_level = func.schedule().store_level();

        // scan loops are loops between store level (exclusive) and
        // the compute level (inclusive) of the accelerated function
        if (is_scan_loops) {
            scan_loops.push_back(op->name);
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
            debug(3) << "Found compute level\n";
            debug(3) << "scan_loops = {";
            for(const string &s : scan_loops)
                debug(3) << s << ", ";
            debug(3) << "}\n";
            // Figure out how much of the accelerated func we're producing for each iteration
            Box box = box_provided(op->body, func.name());
            debug(3) << "Box for " << func.name() << ":\n";
            for (size_t k = 0; k < box.size(); k++) {
                debug(3) << "  [" << simplify(box[k].min) << ", " << simplify(box[k].max) << "] when " << box.used << "\n";
            }

            vector<StencilDimSpecs> dims = extract_stencil_specs(box, scan_loops);
            HWKernel k({func, func.name(), scan_loops, true, dims, {}, {}, {}});
            dag.kernels[k.name] = k;

            debug(3) << k << "\n";
            // save the bounds values in scope
            Scope<Expr> scope;
            for (int i = 0; i < func.dimensions(); i++) {
                string stage_name = func.name() + ".s0." + func.args()[i];
                scope.push(stage_name + ".min", box[i].min);
                scope.push(stage_name + ".max", box[i].max);
            }

            const set<string> input_func(func.schedule().accelerate_inputs().begin(),
                                         func.schedule().accelerate_inputs().end());
            // Figure out how much of each func in the pipeline we're producing
            // do this from output of the pipeline to inputs
            // 'inlined_stages' is sorted, so we do it straight backward
            set<int> visited_stages;
            int i = inlined_stages.size() - 1;
            while (i >= 0) {
                const BoundsInference_Stage &stage = inlined_stages[i];
                if (stage.name == func.name()) {
                    visited_stages.insert(i);
                    break;
                }
                i--;
            }
            i--;
            while (i >= 0) {
                const BoundsInference_Stage &stage = inlined_stages[i];
                // it is a hw kernel if all its consumers are hw kernels and
                // are not input functions
                bool is_hw_kernel = false;
                for(size_t j = 0; j < stage.consumers.size(); j++) {
                    int consumer_idx = stage.consumers[j];
                    if (visited_stages.count(consumer_idx) &&
                        !input_func.count(inlined_stages[consumer_idx].name)) {
                        is_hw_kernel = true;
                    }
                }
                if (is_hw_kernel) {
                    debug(3) << "func " << stage.name << " stage " << stage.stage
                             << " is a hw kernel.\n";
                    Function cur_func = env.find(stage.name)->second;
                    HWKernel cur_kernel({cur_func, stage.name, scan_loops, false, {}, {}, {}, {}});
                    // if cur_func is non-pure function, we may already create an HWKernel for it
                    if (dag.kernels.count(stage.name))
                        cur_kernel = dag.kernels[stage.name];

                    // check that all its consumers are visited, thus we know the bounds
                    // and merge the bounds of cur_func
                    Box cur_box;
                    for (size_t j = 0; j < stage.consumers.size(); j++) {
                        internal_assert(visited_stages.count(stage.consumers[j]));
                        const BoundsInference_Stage &consumer = inlined_stages[stage.consumers[j]];
                        const Box &b = stage.bounds.find(make_pair(consumer.name,
                                                                   consumer.stage))->second;
                        merge_boxes(cur_box, b);
                        // insert the consumer name if it is not the kernel function itself
                        if (consumer.name != cur_kernel.name) {
                            cur_kernel.consumers.push_back(consumer.name);
                            vector<StencilDimSpecs> consumer_dims = extract_stencil_specs(b, scan_loops, scope);
                            cur_kernel.consumer_dims[consumer.name] = consumer_dims;
                        }
                    }

                    for (size_t k = 0; k < cur_box.size(); k++)
                        debug(3) << "  [" << simplify(cur_box[k].min) << ", "
                                 << simplify(cur_box[k].max) << "]\n";

                    vector<StencilDimSpecs> dims = extract_stencil_specs(cur_box, scan_loops, scope);
                    if (!cur_kernel.dims.empty()) {
                        internal_assert(cur_kernel.dims.size() == dims.size());
                        for (size_t i = 0; i < dims.size(); i++) {
                            if (!(dims[i] == cur_kernel.dims[i]))
                                internal_error << "required regions of different stages for func "
                                               << cur_kernel.name << " are not the same.\n";
                        }
                    }
                    cur_kernel.dims = dims;

                    if (compute_level == cur_func.schedule().compute_level() &&
                        store_level == cur_func.schedule().store_level()) {
                        // it is a linebuffered kernel
                        cur_kernel.is_buffered = true;

                        for (int i = 0; i < cur_func.dimensions(); i++) {
                            string arg = cur_func.name() + ".s" + std::to_string(stage.stage) + "." + cur_func.args()[i];
                            scope.push(arg + ".min", cur_kernel.dims[i].min_pos);
                            scope.push(arg + ".max", simplify(cur_kernel.dims[i].min_pos + cur_kernel.dims[i].step - 1)); // NOTE we use 'step' here for we will have line buffer
                        }
                    } else {
                        // This must be a inlined non-pure function
                        internal_assert(!cur_func.is_pure() &&
                                        cur_func.schedule().compute_level().is_inline() &&
                                        cur_func.schedule().store_level().is_inline());

                        // inlined function is not linebuffered
                        cur_kernel.is_buffered = false;

                        for (int i = 0; i < cur_func.dimensions(); i++) {
                            string arg = cur_func.name() + ".s" + std::to_string(stage.stage) + "." + cur_func.args()[i];
                            scope.push(arg + ".min", simplify(expand_expr(cur_box[i].min, scope)));
                            scope.push(arg + ".max", simplify(expand_expr(cur_box[i].max, scope)));
                        }

                    }

                    // push RDom to scope
                    if(stage.stage > 0) {
                        const UpdateDefinition &r = cur_func.updates()[stage.stage - 1];
                        if (r.domain.defined()) {
                            for (ReductionVariable i : r.domain.domain()) {
                                string arg = cur_func.name() + ".s" + std::to_string(stage.stage) + "." + i.var;
                                scope.push(arg + ".min", simplify(expand_expr(i.min, scope)));
                                scope.push(arg + ".max", simplify(expand_expr(i.extent + i.min - 1, scope)));
                            }
                        }
                    }

                    debug(3) << cur_kernel << "\n";
                    // update dag and visited_stages
                    dag.kernels[cur_kernel.name] = cur_kernel;
                    visited_stages.insert(i);
                    if(input_func.count(stage.name))
                        dag.input_kernels.insert(stage.name);
                    debug(3) << "\n";
                }
                i--;
            }
        }

        if (store_level.match(op->name)) {
            debug(3) << "Found store level\n";
            // TODO collect the line buffer size (i.e. realization size)
        }
    }

public:
    BuildDAGForFunction(Function f, const map<string, Function> &e,
                        const vector<BoundsInference_Stage> &s)
        : func(f), env(e), inlined_stages(s) ,
          is_scan_loops(false) {}

    HWKernelDAG build(Stmt s) {
        s.accept(this);
        dag.name = func.name();
        dag.loop_vars = scan_loops;
        build_producer_pointers(dag);
        build_buffered_producer_and_consumer_pointers(dag);
        erase_duplicated_pointers(dag);

        debug(0) << "after building producer pointers:" << "\n";
        for (const auto &p : dag.kernels)
            debug(0) << p.second << "\n";

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
        BuildDAGForFunction builder(func, env, inlined_stages);
        dags.push_back(builder.build(s));
    }
    return s;
}

}
}
