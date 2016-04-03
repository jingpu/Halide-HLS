#include <iostream>
#include <limits>

#include "CodeGen_Zynq_LLVM.h"
#include "CodeGen_Internal.h"
#include "IROperator.h"
#include <sys/mman.h>

namespace Halide {
namespace Internal {

using std::ostream;
using std::endl;
using std::string;
using std::vector;
using std::ostringstream;
using std::to_string;

namespace{
const int FLAG_PROT_WRITE = PROT_WRITE;
const int FLAG_MAP_SHARED = MAP_SHARED;
}

CodeGen_Zynq_LLVM::CodeGen_Zynq_LLVM(Target t)
    : CodeGen_ARM(t) { }

void CodeGen_Zynq_LLVM::visit(const Allocate *op) {
    if (op->free_function == "_kernel_buffer") {
        if (sym_exists(op->name + ".host")) {
            user_error << "Can't have two different buffers with the same name: "
                       << op->name << "\n";
        }

        /* IR:
           allocate kb_var[uint8 * 8 * 8 * 128 * 128] custom_new{"_kernel_buffer"}custom_delete{ _kernel_buffer(); }

           C code:
           Buffer kbuf_var;
           kbuf_var.width = 128;
           kbuf_var.height =128;
           kbuf_var.depth = 64;
           kbuf_var.stride = 128;
           halide_alloc_kbuf(__cma, &kbuf_var);
           uint8_t *_var = (uint8_t*) mmap(NULL,
                         kbuf_var.stride * kbuf_var.height * kbuf_var.depth,
                         PROT_WRITE, MAP_SHARED, __cma, kbuf_var.mmap_offset);
        */
        string kbuf_name = "kbuf_" + op->name;

        // Currently kernel buffer only supports 2-D data layout,
        // so we fold lower dimensions into the 'depth' dimension
        size_t nDims = op->extents.size();
        int width, height, depth;
        internal_assert(nDims >= 2);
        depth = op->type.bytes();
        if (nDims > 2) {
            for (size_t i = 0; i < nDims - 2; i++)
                depth *= *as_const_int(op->extents[i]);
        }
        width = *as_const_int(op->extents[nDims-2]);
        height = *as_const_int(op->extents[nDims-1]);


        Allocation allocation;
        allocation.constant_bytes = width * height * depth;
        allocation.stack_bytes = 0;
        allocation.ptr = NULL;
        allocation.destructor = NULL;
        allocation.destructor_function = NULL;

        // alloca struct.kbuf_t
        llvm::Value *val_width, *val_height, *val_depth;
        val_width = codegen(width);
        val_height = codegen(height);
        val_depth = codegen(depth);

        llvm::StructType *kbuf_type = module->getTypeByName("struct.kbuf_t");
        internal_assert(kbuf_type) << "Did not find kbuf_t in initial module";
        llvm::Constant *one = llvm::ConstantInt::get(i32, 1);
        llvm::Value *kbuf_ptr = builder->CreateAlloca(kbuf_type, one, kbuf_name);
        sym_push(kbuf_name, kbuf_ptr);

        llvm::Value *field_width_ptr =
            builder->CreateConstInBoundsGEP2_32(
#if LLVM_VERSION >= 37
                                                kbuf_type,
#endif
                                                kbuf_ptr,
                                                0,
                                                1);
        builder->CreateStore(val_width, field_width_ptr);
        llvm::Value *field_height_ptr =
            builder->CreateConstInBoundsGEP2_32(
#if LLVM_VERSION >= 37
                                                kbuf_type,
#endif
                                                kbuf_ptr,
                                                0,
                                                3);
        builder->CreateStore(val_height, field_height_ptr);
        llvm::Value *field_depth_ptr =
            builder->CreateConstInBoundsGEP2_32(
#if LLVM_VERSION >= 37
                                                kbuf_type,
#endif
                                                kbuf_ptr,
                                                0,
                                                4);
        builder->CreateStore(val_depth, field_depth_ptr);
        llvm::Value *field_stride_ptr =
            builder->CreateConstInBoundsGEP2_32(
#if LLVM_VERSION >= 37
                                                kbuf_type,
#endif
                                                kbuf_ptr,
                                                0,
                                                2);
        builder->CreateStore(val_width, field_stride_ptr);

        /*
          create call to allocate a kernel buffer

          C code:
          halide_alloc_kbuf(__cma, &kbuf_var);
        */
        vector<llvm::Value *> alloc_args(2);
        alloc_args[0] = sym_get("__cma", true);
        alloc_args[1] = kbuf_ptr;

        llvm::Function *alloc_fn = module->getFunction("halide_alloc_kbuf");
        internal_assert(alloc_fn) << "Did not find halide_alloc_kbuf in initial module";
        llvm::CallInst *alloc_call = builder->CreateCall(alloc_fn, alloc_args);

        // check return value is greater than or equal to zero
        llvm::Value *check = builder->CreateICmpSGE(alloc_call, llvm::ConstantInt::get(i32, 0));
        create_assertion(check, Call::make(Int(32), "halide_error_out_of_memory",
                                           std::vector<Expr>(), Call::Extern));


        /*
          create mmap() call to get a user space pointer

          C code:
          uint8_t *_var = (uint8_t*) mmap(NULL, allocation_bytes,
                          PROT_WRITE, MAP_SHARED, __cma, kbuf_var.mmap_offset);
        */
        vector<llvm::Value *> mmap_args(6);
        mmap_args[0] = llvm::ConstantPointerNull::get(llvm::PointerType::get(i8, 0));
        mmap_args[1] = codegen(allocation.constant_bytes);
        mmap_args[2] = codegen(FLAG_PROT_WRITE);
        mmap_args[3] = codegen(FLAG_MAP_SHARED);
        mmap_args[4] = sym_get("__cma", true);

        llvm::Value *field_mmap_offset_ptr =
            builder->CreateConstInBoundsGEP2_32(
#if LLVM_VERSION >= 37
                                                kbuf_type,
#endif
                                                kbuf_ptr,
                                                0,
                                                8);
        mmap_args[5] = builder->CreateLoad(field_mmap_offset_ptr);

        llvm::Function *mmap_fn = module->getFunction("halide_mmap");
        internal_assert(mmap_fn) << "Did not find mmap() in initial module";
        llvm::CallInst *mmap_call = builder->CreateCall(mmap_fn, mmap_args);

        // store the pointer in allocations
        allocation.ptr = mmap_call;

        // check that it is not a null ptr
        check = builder->CreateIsNotNull(allocation.ptr);
        create_assertion(check, Call::make(Int(32), "halide_error_out_of_memory",
                                           std::vector<Expr>(), Call::Extern));

        // store "munmap" as free_function in allocations
        string free_function = "halide_munmap";
        llvm::Function *free_fn = module->getFunction(free_function);
        internal_assert(free_fn) << "Could not find " << free_function << " in module.\n";
        allocation.destructor = NULL;
        allocation.destructor_function = free_fn;

        allocations.push(op->name, allocation);

        sym_push(op->name + ".host", allocation.ptr);

        codegen(op->body);

        // Should have been freed in the body
        internal_assert(!sym_exists(op->name + ".host"));
        internal_assert(!allocations.contains(op->name));
    } else {
        CodeGen_ARM::visit(op);
    }
}

void CodeGen_Zynq_LLVM::visit(const Free *op) {
    Allocation alloc = allocations.get(op->name);
    if (alloc.destructor_function->getName() == "halide_munmap"){
        /* IR:
           free kb_var

           C code:
           munmap((void*)_var, allocation_bytes);
           halide_free_kbuf(__cma, &kbuf_var);
        */
        string kbuf_name = "kbuf_" + op->name;
        llvm::Value *kbuf_ptr = sym_get(kbuf_name, true);

        vector<llvm::Value *> munmap_args(2);
        munmap_args[0] = alloc.ptr;
        munmap_args[1] = codegen(alloc.constant_bytes);
        llvm::Function *munmap_fn = module->getFunction("halide_munmap");
        internal_assert(munmap_fn) << "Could not find munmap in module.\n";
        builder->CreateCall(munmap_fn, munmap_args);

        vector<llvm::Value *> free_args(2);
        free_args[0] = sym_get("__cma", true);
        free_args[1] = kbuf_ptr;
        llvm::Function *free_fn = module->getFunction("halide_free_kbuf");
        internal_assert(free_fn) << "Did not find halide_free_kbuf in initial module";
        builder->CreateCall(free_fn, free_args);

        sym_pop(kbuf_name);
        allocations.pop(op->name);
        sym_pop(op->name + ".host");
    } else {
        CodeGen_ARM::visit(op);
    }
}

void CodeGen_Zynq_LLVM::visit(const Realize *op) {
    // TODO add assertions

    /* IR:
       realize var.stencil.stream([0, 8], [0, 8], [0, 1], [0, 1]) {
         create_kbuf(slice_var)
         slice_kbuf(kbuf_var, slice_var, 0, 8, 0, 8, (xo * 32), 32, (yo * 32), 32)
         ...
       }

       C code:
       Buffer slice_var;
       slice_kbuf(&kbuf_var, &slice_var, xo * 32, yo * 32, 32, 32);
    */
    // check that there is a call of slice_buffer in the body
    const Block *block_1 = op->body.as<Block>();
    internal_assert(block_1);
    const Evaluate *eva_1 = block_1->first.as<Evaluate>();
    internal_assert(eva_1);
    const Call *create_kbuf_call = eva_1->value.as<Call>();
    internal_assert(create_kbuf_call && create_kbuf_call->name == "create_kbuf");

    const Block *block_2 = block_1->rest.as<Block>();
    internal_assert(block_2);
    const Evaluate *eva_2 = block_2->first.as<Evaluate>();
    internal_assert(eva_2);
    const Call *slice_call = eva_2->value.as<Call>();
    internal_assert(slice_call && slice_call->name == "slice_kbuf");

    internal_assert(slice_call->args.size() >= 6);

    const Variable *kbuf_var = slice_call->args[0].as<Variable>();
    const Variable *slice_var = slice_call->args[1].as<Variable>();
    internal_assert(kbuf_var && slice_var);

    llvm::Value *kbuf_ptr = sym_get(kbuf_var->name, true);
    llvm::StructType *kbuf_type = module->getTypeByName("struct.kbuf_t");
    internal_assert(kbuf_type) << "Did not find Buffer in initial module";
    llvm::Constant *one = llvm::ConstantInt::get(i32, 1);
    llvm::Value *slice_ptr = builder->CreateAlloca(kbuf_type, one, slice_var->name);
    buffer_slices.push_back(slice_ptr);

    llvm::Value *width, *height, *x_offset, *y_offset;
    // TODO check the lower demesion matches the buffer depth
    // TODO static check that the slice is within the bounds of kernel buffer
    size_t arg_length = slice_call->args.size();
    x_offset = codegen(slice_call->args[arg_length-4]);
    width = codegen(slice_call->args[arg_length-3]);
    y_offset = codegen(slice_call->args[arg_length-2]);
    height = codegen(slice_call->args[arg_length-1]);

    vector<llvm::Value *> slice_args({kbuf_ptr, slice_ptr, x_offset, y_offset, width, height});
    llvm::Function *slice_fn = module->getFunction("halide_slice_kbuf");
    internal_assert(slice_fn) << "Could not find halide_slice_kbuf in module.\n";
    builder->CreateCall(slice_fn, slice_args);

    const ProducerConsumer *pc = block_2->rest.as<ProducerConsumer>();
    internal_assert(pc && !pc->update.defined());

    if (starts_with(pc->name, "_hls_target.")) {
        // reachs the HW boundary
        /* C code:
           Buffer kbufs[3];
           kbufs[0] = kbuf_in0;
           kbufs[1] = kbuf_in1;
           kbufs[2] = kbuf_out;
           halide_process_image(__hwacc, kbufs);
           halide_pend_processed(__hwacc);
        */
        // TODO check the order of buffer slices is consistent with
        // the order of DMA ports in the driver
        llvm::Value *set_size = llvm::ConstantInt::get(i32, buffer_slices.size());
        llvm::Value *slice_set = builder->CreateAlloca(kbuf_type, set_size);
        llvm::DataLayout d(module.get());
        size_t size_of_kbuf = d.getTypeAllocSize(kbuf_type);
        for (size_t i = 0; i < buffer_slices.size(); i++) {
            llvm::Value *slice_ptr = buffer_slices[i];
            llvm::Value *elem_ptr = builder->CreateConstInBoundsGEP1_32(slice_set,
                                                                        i);
            builder->CreateMemCpy(elem_ptr, slice_ptr, size_of_kbuf, 0);
        }

        llvm::Value *fd = sym_get("__hwacc", true);
        vector<llvm::Value *> process_args({fd, slice_set});
        llvm::Function *process_fn = module->getFunction("halide_process_image");
        internal_assert(process_fn) << "Did not find halide_process_image in initial module";
        llvm::Value *process_id = builder->CreateCall(process_fn, process_args);

        vector<llvm::Value *> pend_args({fd, process_id});
        llvm::Function *pend_fn = module->getFunction("halide_pend_processed");
        internal_assert(pend_fn) << "Did not find halide_pend_processed in initial module";
        builder->CreateCall(pend_fn, pend_args);

        buffer_slices.clear();
    } else {
        // skip traverse produce node, since the node is generating
        // input streams
        codegen(pc->consume);
    }
}


}
}
