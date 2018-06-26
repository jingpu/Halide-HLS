#include <iostream>
#include <limits>

#include "CodeGen_Zynq_LLVM.h"
#include "CodeGen_Internal.h"
#include "IROperator.h"
#include <sys/mman.h>

namespace Halide {
namespace Internal {

using std::vector;
using llvm::Value;

CodeGen_Zynq_LLVM::CodeGen_Zynq_LLVM(Target t)
    : CodeGen_ARM(t) { }

void CodeGen_Zynq_LLVM::visit(const Realize *op) {
    internal_assert(ends_with(op->name, ".stream"));
    llvm::StructType *kbuf_type = module->getTypeByName("struct.cma_buffer_t");
    internal_assert(kbuf_type);
    llvm::Constant *one = llvm::ConstantInt::get(i32_t, 1);
    Value *slice_ptr = builder->CreateAlloca(kbuf_type, one, op->name);
    buffer_slices.push_back(slice_ptr);

    sym_push(op->name, slice_ptr);
    // Recurse
    codegen(op->body);
    sym_pop(op->name);
}

void CodeGen_Zynq_LLVM::visit(const ProducerConsumer *op) {
    if (op->is_producer && starts_with(op->name, "_hls_target.")) {
        // reachs the HW boundary
        /* C code:
           cma_buffer_t kbufs[3];
           kbufs[0] = kbuf_in0;
           kbufs[1] = kbuf_in1;
           kbufs[2] = kbuf_out;
           int process_id halide_zynq_hwacc_launch(kbufs);
           halide_pend_processed(process_id);
        */
        // TODO check the order of buffer slices is consistent with
        // the order of DMA ports in the driver
        llvm::StructType *kbuf_type = module->getTypeByName("struct.cma_buffer_t");
        internal_assert(kbuf_type);
        Value *set_size = llvm::ConstantInt::get(i32_t, buffer_slices.size());
        Value *slice_set = builder->CreateAlloca(kbuf_type, set_size);
        llvm::DataLayout d(module.get());
        size_t size_of_kbuf = d.getTypeAllocSize(kbuf_type);
        for (size_t i = 0; i < buffer_slices.size(); i++) {
            Value *slice_ptr = buffer_slices[i];
            Value *elem_ptr = builder->CreateConstInBoundsGEP1_32(
#if LLVM_VERSION >= 37
								  kbuf_type,
#endif
								  slice_set,
                                                                        i);
            builder->CreateMemCpy(elem_ptr, slice_ptr, size_of_kbuf, 0);
        }

        vector<Value *> process_args({slice_set});
        llvm::Function *process_fn = module->getFunction("halide_zynq_hwacc_launch");
        internal_assert(process_fn);
        Value *process_id = builder->CreateCall(process_fn, process_args);

        vector<Value *> pend_args({process_id});
        llvm::Function *pend_fn = module->getFunction("halide_zynq_hwacc_sync");
        internal_assert(pend_fn);
        builder->CreateCall(pend_fn, pend_args);

        buffer_slices.clear();
    } else {
        CodeGen_ARM::visit(op);
    }
}

void CodeGen_Zynq_LLVM::visit(const Call *op) {
    if (op->is_intrinsic("halide_zynq_cma_alloc")) {
        internal_assert(op->args.size() == 1);
        Value *buffer = codegen(op->args[0]);
        llvm::Function *fn = module->getFunction("halide_zynq_cma_alloc");
        internal_assert(fn);
        value = builder->CreateCall(fn, {buffer});
    } else if (op->is_intrinsic("halide_zynq_cma_free")) {
        internal_assert(op->args.size() == 1);
        Value *buffer = codegen(op->args[0]);
        llvm::Function *fn = module->getFunction("halide_zynq_cma_free");
        internal_assert(fn);
        value = builder->CreateCall(fn, {buffer});
    } else if (op->is_intrinsic("stream_subimage")) {
        /* IR:
           stream_subimage(direction, buffer_var, stream_var,
                       address_of_subimage_origin,
                       dim_0_stride, dim_0_extent, ...)

           C code:
           halide_zynq_subimage(&buffer_var, &stream_var, address_of_subimage_origin, width, height);
        */
        internal_assert(op->args.size() >= 6);
        Value *buffer_ptr = codegen(op->args[1]);
        Value *slice_ptr = codegen(op->args[2]);
        Value *address_of_subimage_origin = codegen(op->args[3]);
        // cast to the i8* type
        address_of_subimage_origin =
          builder->CreatePointerCast(address_of_subimage_origin,
                                     llvm::PointerType::get(i8_t, 0));

        // TODO check the lower demesion matches the buffer depth
        // TODO static check that the slice is within the bounds of kernel buffer
        size_t arg_length = op->args.size();
        Value *width = codegen(op->args[arg_length-3]);
        Value *height = codegen(op->args[arg_length-1]);

        llvm::Function *fn = module->getFunction("halide_zynq_subimage");
        vector<Value *> args({buffer_ptr, slice_ptr, address_of_subimage_origin, width, height});
        internal_assert(fn);
        value = builder->CreateCall(fn, args);
    } else if (op->name == "address_of") {
        internal_assert(op->args.size() == 1) << "address_of takes one argument\n";
        internal_assert(op->type.is_handle()) << "address_of must return a Handle type\n";
        const Load *load = op->args[0].as<Load>();
        internal_assert(load) << "The sole argument to address_of must be a Load node\n";
        internal_assert(load->index.type().is_scalar()) << "Can't take the address of a vector load\n";

        value = codegen_buffer_pointer(load->name, load->type, load->index);
    } else {
        CodeGen_ARM::visit(op);
    }
}

}
}
