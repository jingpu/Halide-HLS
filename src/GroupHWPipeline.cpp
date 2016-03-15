#include "GroupHWPipeline.h"

#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Debug.h"

namespace Halide {
namespace Internal {

using std::string;
using std::map;
using std::vector;
using std::pair;
using std::make_pair;

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

            // insert a "start_hwacc" call to include the device
            // handlers "__hwacc" and "__cma" in the closure
            Expr fd_hwacc = Variable::make(Int(32), "__hwacc");
            Expr fd_cma = Variable::make(Int(32), "__cma");
            Stmt hw_call = Evaluate::make(Call::make(Handle(), "start_hwacc", {fd_hwacc, fd_cma}, Call::Intrinsic));
            stmt = ProducerConsumer::make( "_hls_target." + op->name, Block::make(hw_call, pipeline_body), Stmt(), op->consume);
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

}
}
