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

// copied from src/runtime/zynq.cpp
const string zynq_runtime =
    "#ifndef CMA_BUFFER_T_DEFINED\n"
    "#define CMA_BUFFER_T_DEFINED\n"
    "struct mMap;\n"
    "typedef struct cma_buffer_t {\n"
    "  unsigned int id; // ID flag for internal use\n"
    "  unsigned int width; // Width of the image\n"
    "  unsigned int stride; // Stride between rows, in pixels. This must be >= width\n"
    "  unsigned int height; // Height of the image\n"
    "  unsigned int depth; // Byte-depth of the image\n"
    "  unsigned int phys_addr; // Bus address for DMA\n"
    "  void* kern_addr; // Kernel virtual address\n"
    "  struct mMap* cvals;\n"
    "  unsigned int mmap_offset;\n"
    "} cma_buffer_t;\n"
    "#endif\n"
    "// Zynq runtime API\n"
    "int halide_zynq_init();\n"
    "void halide_zynq_free(void *user_context, void *ptr);\n"
    "int halide_zynq_cma_alloc(struct buffer_t *buf);\n"
    "int halide_zynq_cma_free(struct buffer_t *buf);\n"
    "int halide_zynq_subimage(const struct buffer_t* image, struct cma_buffer_t* subimage, void *address_of_subimage_origin, int width, int height);\n"
    "int halide_zynq_hwacc_launch(struct cma_buffer_t bufs[]);\n"
    "int halide_zynq_hwacc_sync(int task_id);\n";
}

CodeGen_Zynq_C::CodeGen_Zynq_C(ostream &dest,
                               Target target,
                               OutputKind output_kind)
    : CodeGen_C(dest, target, output_kind) {
    stream  << zynq_runtime;
}

void CodeGen_Zynq_C::visit(const Realize *op) {
    internal_assert(ends_with(op->name, ".stream"));
    open_scope();
    string slice_name = op->name;
    buffer_slices.push_back(slice_name);

    do_indent();
    stream << "cma_buffer_t " << print_name(slice_name) << ";\n";
    // Recurse
    print_stmt(op->body);
    close_scope(slice_name);
}

void CodeGen_Zynq_C::visit(const ProducerConsumer *op) {
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
        do_indent();
        stream << "cma_buffer_t _cma_bufs[" << buffer_slices.size() << "];\n";
        for (size_t i = 0; i < buffer_slices.size(); i++) {
            do_indent();
            stream << "_cma_bufs[" << i << "] = " << print_name(buffer_slices[i]) << ";\n";
        }
        do_indent();
        stream << "int _process_id = halide_zynq_hwacc_launch(_cma_bufs);\n";
        do_indent();
        stream << "halide_zynq_hwacc_sync(_process_id);\n";

        buffer_slices.clear();
    } else {
        CodeGen_C::visit(op);
    }
}

void CodeGen_Zynq_C::visit(const Call *op) {
    ostringstream rhs;
    if (op->is_intrinsic("halide_zynq_cma_alloc")) {
        internal_assert(op->args.size() == 1);
        string buffer = print_expr(op->args[0]);
        rhs << "halide_zynq_cma_alloc(" << buffer << ")";
        print_assignment(op->type, rhs.str());
    } else if (op->is_intrinsic("halide_zynq_cma_free")) {
        internal_assert(op->args.size() == 1);
        string buffer = print_expr(op->args[0]);
        do_indent();
        stream << "halide_zynq_cma_free(" << buffer << ");\n";
    } else if (op->is_intrinsic("stream_subimage")) {
        /* IR:
           stream_subimage(direction, buffer_var, stream_var,
                       address_of_subimage_origin,
                       dim_0_stride, dim_0_extent, ...)

           C code:
           halide_zynq_subimage(&buffer_var, &stream_var, address_of_subimage_origin, width, height);
    */
        internal_assert(op->args.size() >= 6);
        const Variable *buffer_var = op->args[1].as<Variable>();
        internal_assert(buffer_var && buffer_var->type == type_of<struct buffer_t *>());
        string buffer_name = print_expr(op->args[1]);
        string slice_name = print_expr(op->args[2]);
        string address_of_subimage_origin = print_expr(op->args[3]);

        string width, height;
        // TODO check the lower demesion matches the buffer depth
        // TODO static check that the slice is within the bounds of kernel buffer
        size_t arg_length = op->args.size();
        width = print_expr(op->args[arg_length-3]);
        height = print_expr(op->args[arg_length-1]);

        do_indent();
        stream << "halide_zynq_subimage("
               << print_name(buffer_name) << ", &" << print_name(slice_name) << ", "
               << address_of_subimage_origin << ", " << width << ", " << height << ");\n";
    } else if (op->name == "address_of") {
        std::ostringstream rhs;
        const Load *l = op->args[0].as<Load>();
        internal_assert(op->args.size() == 1 && l);
        rhs << "(("
            << print_type(l->type.element_of()) // index is in elements, not vectors.
            << " *)"
            << print_name(l->name)
            << " + "
            << print_expr(l->index)
            << ")";
        print_assignment(op->type, rhs.str());
    } else {
        CodeGen_C::visit(op);
    }
}
}
}
