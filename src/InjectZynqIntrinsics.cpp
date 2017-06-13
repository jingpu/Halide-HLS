#include "InjectZynqIntrinsics.h"
#include "IRMutator.h"
#include "IROperator.h"

namespace Halide {
namespace Internal {

using std::string;
using std::vector;
using std::map;

/*
class InjectCmaIntrinsics : public IRMutator {
    const map<string, Function> &env;

    using IRMutator::visit;

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
            // The IR is like:
            //
            //  let buffer_name.zerocopy_buffer = create_buffer_t(null_handle(), ...)
            //  let zynq_cma_alloc_result = halide_zynq_cma_alloc(buffer_name.zerocopy_buffer)
            //  assert((zynq_cma_alloc_result == 0), zynq_cma_alloc_result)
            //  allocate buffer_name[...]custom_new{buffer_name.zerocopy_buffer.host}custom_delete{ halide_zynq_free(); }
            // get the args from a previously inserted create_buffer_t call
            // and pop the call out from the body
            const LetStmt *lets = op->body.as<LetStmt>();
            internal_assert(lets);
            const Call *buf = lets->value.as<Call>();
            internal_assert(buf && buf->name == Call::create_buffer_t);
            Stmt new_body = mutate(lets->body);

            // allocate node
            string zerocopy_buffer_name = op->name + ".buffer";
            Expr zerocopy_buffer = Variable::make(type_of<struct buffer_t *>(), zerocopy_buffer_name);
            Expr new_expr = Call::make(Handle(), Call::extract_buffer_host, {zerocopy_buffer}, Call::Intrinsic);
            string free_function = "halide_zynq_free";

            Stmt free = Evaluate::make(Call::make(Int(32), "halide_zynq_cma_free", {zerocopy_buffer}, Call::Intrinsic));
            stmt = Allocate::make(op->name, op->type, op->extents, op->condition, Block::make(new_body, free), new_expr, free_function);

            // call to halide_zynq_cma_alloc and assertion
            string call_result_name = unique_name("zynq_cma_alloc_result");
            Expr call_result_var = Variable::make(Int(32), call_result_name);
            Expr call = Call::make(Int(32), "halide_zynq_cma_alloc", {zerocopy_buffer}, Call::Intrinsic);
            stmt = Block::make(LetStmt::make(call_result_name, call, AssertStmt::make(call_result_var == 0, call_result_var)), stmt);

            // create the new buffer_t
            vector<Expr> args = buf->args;
            args[0] = Call::make(Handle(), Call::null_handle, {}, Call::PureIntrinsic);
            Expr zerocopy_buf = Call::make(type_of<struct buffer_t *>(),
                                           Call::create_buffer_t,
                                           args, Call::Intrinsic);
            stmt = LetStmt::make(zerocopy_buffer_name,  zerocopy_buf, stmt);


        } else {
            IRMutator::visit(op);
        }
    }

public:
    InjectCmaIntrinsics(const map<string, Function> &e)
        : env(e) {}
};
*/

Stmt inject_zynq_intrinsics(Stmt s,
                            const map<string, Function> &env) {
    // TODO(jingpu) check it we still need this after implementing device_interface
    //return InjectCmaIntrinsics(env).mutate(s);
    return s;
}

}
}
