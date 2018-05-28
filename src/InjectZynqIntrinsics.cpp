#include "InjectZynqIntrinsics.h"

#include "IRMutator.h"
#include "IROperator.h"
#include "Substitute.h"

namespace Halide {
namespace Internal {
namespace {

using std::string;
using std::vector;
using std::map;

// Set the host field of any buffer_init calls on the given buffer to null.
class NullifyHostField : public IRMutator {
    using IRMutator::visit;
    void visit(const Variable *op) {
        if (op->name == buf_name) {
            expr = make_zero(Handle());
        } else {
            expr = op;
        }
    }
    std::string buf_name;
public:
    NullifyHostField(const std::string &b) : buf_name(b) {}
};

Stmt call_extern_and_assert(const string& name, const vector<Expr>& args) {
    Expr call = Call::make(Int(32), name, args, Call::Extern);
    string call_result_name = unique_name(name + "_result");
    Expr call_result_var = Variable::make(Int(32), call_result_name);
    return LetStmt::make(call_result_name, call,
                         AssertStmt::make(EQ::make(call_result_var, 0), call_result_var));
}

class InjectCmaIntrinsics : public IRMutator {
    const map<string, Function> &env;

    using IRMutator::visit;

    Stmt make_zynq_malloc(const string &buf_name) const {
        Expr buf = Variable::make(type_of<struct halide_buffer_t *>(), buf_name + ".buffer");
        Stmt device_malloc = call_extern_and_assert("halide_zynq_cma_alloc", {buf});
        return device_malloc;
    }

    Stmt make_zynq_free(const string &buf_name) const {
        Expr buf = Variable::make(type_of<struct halide_buffer_t *>(), buf_name + ".buffer");
        Stmt device_free = Evaluate::make(Call::make(Int(32), "halide_zynq_cma_free",
                                                     {buf}, Call::Extern));
        return device_free;
    }

    void visit(const Allocate *op) {
        auto iter = env.find(op->name);

        // If it's not in the environment it's some anonymous
        // realization that we should skip (e.g. an inlined reduction)
        if (iter == env.end()) {
            IRMutator::visit(op);
            return;
        }

        if (iter->second.schedule().is_kernel_buffer()) {
            debug(3) << "find a kernel buffer " << op->name << "\n";
            // function (accessed by the accelerator pipeline) are scheduled to store in kernel buffer
            // we want to use cma (contiguous memory allocator)
            // IR before:
            //   allocate hw_output[uint8 * WIDTH * HEIGHT]
            //   let hw_output.buffer = _halide_buffer_init(...)
            //
            // IR after:
            //   let hw_output.buffer = _halide_buffer_init(...)
            //   let halide_zynq_cma_malloc_result = halide_zynq_cma_malloc(hw_output.buffer)
            //   assert((halide_zynq_cma_malloc_result == 0), halide_zynq_cma_malloc_result)
            //   allocate hw_output[uint8 * WIDTH * HEIGHT] in Heap
            //   ....
            //   halide_zynq_cma_free(hw_output.buffer)

            // Get the args from a previously inserted buffer_init call
            // and pop the call out from the body
            const string buffer_name = op->name;
            const LetStmt *buffer_init_let = op->body.as<LetStmt>();
            internal_assert(buffer_init_let && buffer_init_let->name == op->name + ".buffer");
            Expr let_value = substitute_in_all_lets(buffer_init_let->value);
            const Call *buffer_init_call = let_value.as<Call>();
            internal_assert(buffer_init_call && buffer_init_call->name == Call::buffer_init) << Expr(buffer_init_call) << "\n";
            Stmt inner_body = mutate(buffer_init_let->body);

            Stmt zynq_malloc = make_zynq_malloc(op->name);
            Stmt zynq_free = make_zynq_free(op->name);

            // Create a new Allocation scope inside the buffer
            // creation, use the host pointer as the allocation and
            // set the destructor to a nop.
            inner_body = Allocate::make(op->name, op->type, op->extents, op->condition, inner_body,
                                        Call::make(Handle(), Call::buffer_get_host,
                                                   { Variable::make(type_of<struct halide_buffer_t *>(), op->name + ".buffer") },
                                                   Call::Extern),
                                        "halide_zynq_free");
            // Wrap malloc and free around Allocate.
            inner_body = Block::make(zynq_malloc, inner_body);
            inner_body = Block::make(inner_body, zynq_free);

            // Rewrite original buffer_init call and wrap it around the combined malloc.
            // The original buffer_init call uses address_of on the
            // allocate node. We want it to be initially null and let
            // the zynq_malloc fill it in instead. The
            // Allocate node was just rewritten to just grab this
            // pointer out of the buffer after the combined
            // allocation, so no memory is dropped.
            Expr buf = NullifyHostField(op->name).mutate(buffer_init_let->value);
            stmt = LetStmt::make(op->name + ".buffer", buf, inner_body);
        } else {
            IRMutator::visit(op);
        }
    }

public:
    InjectCmaIntrinsics(const map<string, Function> &e)
        : env(e) {}
};

}  // namespace

Stmt inject_zynq_intrinsics(Stmt s,
                            const map<string, Function> &env) {
    // TODO(jingpu) check it we still need this after implementing device_interface
    return InjectCmaIntrinsics(env).mutate(s);
}

}  // namespace Internal
}  // namespace Halide
