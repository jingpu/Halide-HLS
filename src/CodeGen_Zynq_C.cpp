#include <iostream>
#include <limits>

#include "CodeGen_Zynq_C.h"
#include "CodeGen_Internal.h"
#include "IROperator.h"
#include "Simplify.h"

namespace Halide {
namespace Internal {

using std::ostream;
using std::endl;
using std::string;
using std::vector;
using std::ostringstream;
using std::to_string;

namespace {
const string zynq_headers =
    "#include <fcntl.h>\n"
    "#include <sys/ioctl.h>\n"
    "#include <sys/mman.h>\n"
    "#ifndef KBUF_T_DEFINED\n"
    "#define KBUF_T_DEFINED\n"
    "typedef struct kbuf_t{\n"
    " unsigned int id; // ID flag for internal use\n"
    " unsigned int width; // Width of the image\n"
    " unsigned int stride; // Stride between rows, in pixels. This must be >= width\n"
    " unsigned int height; // Height of the image\n"
    " unsigned int depth; // Byte-depth of the image\n"
    " unsigned int phys_addr; // Bus address for DMA\n"
    " void* kern_addr; // Kernel virtual address\n"
    " struct mMap* cvals;\n"
    " unsigned int mmap_offset;\n"
    "} kbuf_t;\n"
    "#endif\n";

// copied from src/runtime/zynq_driver.cpp
const string runtime_zynq_driver =
    "#ifndef _IOCTL_CMDS_H_\n"
    "#define _IOCTL_CMDS_H_\n"
    "#define GET_BUFFER 1000 // Get an unused buffer\n"
    "#define GRAB_IMAGE 1001 // Acquire image from camera\n"
    "#define FREE_IMAGE 1002 // Release buffer\n"
    "#define PROCESS_IMAGE 1003 // Push to stencil path\n"
    "#define PEND_PROCESSED 1004 // Retreive from stencil path\n"
    "#endif\n"
    "static int halide_slice_kbuf(kbuf_t* src, kbuf_t* des, int x, int y, int width, int height) {\n"
    " *des = *src; // copy depth, stride, data, etc.\n"
    " des->width = width;\n"
    " des->height = height;\n"
    " des->phys_addr += src->depth * (y * src->stride + x);\n"
    " des->mmap_offset += src->depth * (y * src->stride + x);\n"
    " return 0;\n"
    "}\n"
    "static int halide_alloc_kbuf(int fd, kbuf_t* ptr) {\n"
    " return ioctl(fd, GET_BUFFER, (long unsigned int)ptr);\n"
    "}\n"
    "static int halide_free_kbuf(int fd, kbuf_t* ptr) {\n"
    " return ioctl(fd, FREE_IMAGE, (long unsigned int)ptr);\n"
    "}\n"
    "static int halide_process_image(int fd, kbuf_t* ptr) {\n"
    " return ioctl(fd, PROCESS_IMAGE, (long unsigned int)ptr);\n"
    "}\n"
    "static int halide_pend_processed(int fd, int id) {\n"
    " return ioctl(fd, PEND_PROCESSED, (long unsigned int)id);\n"
    "}\n";
}

CodeGen_Zynq_C::CodeGen_Zynq_C(ostream &dest)
    : CodeGen_C(dest, false, "", zynq_headers) {
    stream  << runtime_zynq_driver;
}

void CodeGen_Zynq_C::visit(const Allocate *op) {
    if (op->free_function == "_kernel_buffer") {
        open_scope();
        /* IR:
           allocate kb_var[uint8 * 8 * 8 * 128 * 128] custom_new{"_kernel_buffer"}custom_delete{ _kernel_buffer(); }

           C code:
           kbuf_t kbuf_var;
           kbuf_var.width = 128;
           kbuf_var.height =128;
           kbuf_var.depth = 64;
           kbuf_var.stride = 128;
           int get_kbuf_status = halide_alloc_kbuf(__cma, &kbuf_var);
           if(get_kbuf_status < 0) {
            printf("Failed to allocate kernel buffer for var.\n");
            return 0;
           }
           uint8_t *_var = (uint8_t*) mmap(NULL,
                         kbuf_var.stride * kbuf_var.height * kbuf_var.depth,
                         PROT_WRITE, MAP_SHARED, __cma, kbuf_var.mmap_offset);
        */
        string kbuf_name = "kbuf_" + op->name;
        string get_kbuf_status_name = "status_get_" + kbuf_name;

        // Currently kernel buffer only supports 2-D data layout,
        // so we fold lower dimensions into the 'depth' dimension
        size_t nDims = op->extents.size();
        unsigned width, height, depth;
        internal_assert(nDims >= 2);
        depth = op->type.bytes();
        if (nDims > 2) {
            for (size_t i = 0; i < nDims - 2; i++)
                depth *= *as_const_int(op->extents[i]);
        }
        width = *as_const_int(op->extents[nDims-2]);
        height = *as_const_int(op->extents[nDims-1]);

        do_indent();
        stream << "kbuf_t " << print_name(kbuf_name) << ";\n";
        do_indent();
        stream << print_name(kbuf_name) << ".width = " << width << ";\n";
        do_indent();
        stream << print_name(kbuf_name) << ".height = " << height << ";\n";
        do_indent();
        stream << print_name(kbuf_name) << ".depth = " << depth << ";\n";
        do_indent();
        stream << print_name(kbuf_name) << ".stride = " << width << ";\n";
        do_indent();
        stream << "int " << print_name(get_kbuf_status_name) << " = "
               << "halide_alloc_kbuf(__cma, &"
               << print_name(kbuf_name) << ");\n";

        do_indent();
        stream << "if ("<< print_name(get_kbuf_status_name) << " < 0)";
        open_scope();
        do_indent();
        stream << "printf(\"Failed to allocate kernel buffer for " << op->name
               << ".\\n\");\n";
        close_scope("");

        Allocation alloc;
        alloc.type = op->type;
        alloc.free_function = op->free_function;
        allocations.push(op->name, alloc);
        heap_allocations.push(op->name, 0);
        do_indent();
        stream << print_type(op->type) << "*" << print_name(op->name)
               << " = (" << print_type(op->type) << "*) mmap(NULL, "
               << print_name(kbuf_name) << ".stride * "
               << print_name(kbuf_name) << ".height * "
               << print_name(kbuf_name) << ".depth, "
               << "PROT_WRITE, MAP_SHARED, __cma, "
               << print_name(kbuf_name) << ".mmap_offset);\n";

        op->body.accept(this);

        // Should have been freed internally
        internal_assert(!allocations.contains(op->name));

        close_scope("alloc " + print_name(op->name));
    } else {
        CodeGen_C::visit(op);
    }
}

void CodeGen_Zynq_C::visit(const Free *op) {
    string free_function;
    if (heap_allocations.contains(op->name)) {
        free_function = allocations.get(op->name).free_function;
    }
    if (free_function == "_kernel_buffer"){
        /* IR:
           free kb_var

           C code:
           munmap((void*)_var, kbuf_var.stride * kbuf_var.height * kbuf_var.depth);
           halide_free_kbuf(__cma, &kbuf_var);
        */
        internal_assert(heap_allocations.contains(op->name));
        string kbuf_name = "kbuf_" + op->name;

        do_indent();
        stream << "munmap((void*)" << print_name(op->name) << ", "
               << print_name(kbuf_name) << ".stride * "
               << print_name(kbuf_name) << ".height * "
               << print_name(kbuf_name) << ".depth);\n";
        do_indent();
        stream << "halide_free_kbuf(__cma, &" << print_name(kbuf_name) << ");\n";

        heap_allocations.pop(op->name);
        allocations.pop(op->name);
    } else {
        CodeGen_C::visit(op);
    }
}

void CodeGen_Zynq_C::visit(const Realize *op) {
    // TODO add assertions

    /* IR:
       realize var.stencil.stream([0, 8], [0, 8], [0, 1], [0, 1]) {
         create_kbuf(slice_var)
         slice_kbuf(kbuf_var, slice_var, 0, 8, 0, 8, (xo * 32), 32, (yo * 32), 32)
         ...
       }

       C code:
       kbuf_t slice_var;
       int status_slice_var = slice_kbuf(&kbuf_var, &slice_var, xo * 32, yo * 32, 32, 32);
       if (status_slice_var < 0) {
       printf("Failed slicing buffer for var.\n");
       return 0;
       }
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
    open_scope();

    internal_assert(slice_call->args.size() >= 6);
    const Variable *kbuf_var = slice_call->args[0].as<Variable>();
    const Variable *slice_var = slice_call->args[1].as<Variable>();
    internal_assert(kbuf_var && slice_var);

    string kbuf_name = kbuf_var->name;
    string slice_name = slice_var->name;
    buffer_slices.push_back(slice_name);
    string slice_status_name = "status_" + slice_name;
    string width, height, x_offset, y_offset;
    // TODO check the lower demesion matches the buffer depth
    // TODO static check that the slice is within the bounds of kernel buffer
    size_t arg_length = slice_call->args.size();
    x_offset = print_expr(slice_call->args[arg_length-4]);
    width = print_expr(slice_call->args[arg_length-3]);
    y_offset = print_expr(slice_call->args[arg_length-2]);
    height = print_expr(slice_call->args[arg_length-1]);

    do_indent();
    stream << "kbuf_t " << print_name(slice_name) << ";\n";
    do_indent();
    stream << "int " << print_name(slice_status_name) << " = halide_slice_kbuf(&"
           << print_name(kbuf_name) << ", &" << print_name(slice_name) << ", "
           << x_offset << ", " << y_offset << ", "
           << width << ", " << height << ");\n";
    do_indent();
    stream << "if ("<< print_name(slice_status_name) << " < 0)";
    open_scope();
    do_indent();
    stream << "printf(\"Failed to slice kernel buffer for " << slice_name
           << ".\\n\");\n";
    close_scope("");

    const ProducerConsumer *pc = block_2->rest.as<ProducerConsumer>();
    internal_assert(pc && !pc->update.defined());

    if (starts_with(pc->name, "_hls_target.")) {
        // reachs the HW boundary
        /* C code:
           kbuf_t kbufs[3];
           kbufs[0] = kbuf_in0;
           kbufs[1] = kbuf_in1;
           kbufs[2] = kbuf_out;
           halide_process_image(__hwacc, kbufs);
           halide_pend_processed(__hwacc);
        */
        // TODO check the order of buffer slices is consistent with
        // the order of DMA ports in the driver
        do_indent();
        stream << "kbuf_t _kbufs[" << buffer_slices.size() << "];\n";
        for (size_t i = 0; i < buffer_slices.size(); i++) {
            do_indent();
            stream << "_kbufs[" << i << "] = " << print_name(buffer_slices[i]) << ";\n";
        }
        do_indent();
        stream << "int _process_id = halide_process_image(__hwacc, _kbufs);\n";
        do_indent();
        stream << "halide_pend_processed(__hwacc, _process_id);\n";

        buffer_slices.clear();
    } else {
        // skip traverse produce node, since the node is generating
        // input streams
        pc->consume.accept(this);
    }
    close_scope(slice_name);
}


}
}
