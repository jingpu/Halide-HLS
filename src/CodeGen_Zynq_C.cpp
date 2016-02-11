#include <iostream>
#include <limits>

#include "CodeGen_Zynq_C.h"
#include "CodeGen_Internal.h"
#include "Substitute.h"
#include "IROperator.h"
#include "Param.h"
#include "Var.h"
#include "Lerp.h"
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
    "#include <sys/time.h>\n"
    "#include <fcntl.h>\n"
    "#include <unistd.h>\n"
    "#include <sys/ioctl.h>\n"
    "#include <sys/mman.h>\n"
    "#include \"buffer.h\"\n"
    "#include \"ioctl_cmds.h\"\n";

const string slice_buffer_func =
    "static int _slice_buffer(Buffer* src, Buffer* des, unsigned int x, unsigned int y, unsigned int width, unsigned int height) {\n"
    " if (width == 0 || height == 0) {\n"
    "  printf(\"slice_buffer failed: width and height of slide should be non-zero.\\n\");\n"
    "  return -1;\n"
    " }\n"
    " if (x + width > src->width || y + height > src->height) {\n"
    "  printf(\"slice_buffer failed: slice is out of range.\\n\");\n"
    "  return -1;\n"
    " }\n"
    " *des = *src; // copy depth, stride, data, etc.\n"
    " des->width = width;\n"
    " des->height = height;\n"
    " des->phys_addr += src->depth * (y * src->stride + x);\n"
    " des->mmap_offset += src->depth * (y * src->stride + x);\n"
    " return 0;\n"
    "}\n";

const string open_device_stmt =
    " // Open the hardware device\n"
    " int hwacc = open(\"/dev/hwacc0\", O_RDWR);\n"
    " if(hwacc == -1){\n"
    "  printf(\"Failed to open hardware device!\\n\");\n"
    "  return(0);\n"
    " }\n";

const string extern_calls_prototypes =
    "int32_t halide_error_bad_elem_size(void *, const char *, const char *, int32_t, int32_t);\n"
    "int32_t halide_error_access_out_of_bounds(void *, const char *, int32_t, int32_t, int32_t, int32_t, int32_t);\n"
    "int32_t halide_error_constraint_violated(void *, const char *, int32_t, const char *, int32_t);\n"
    "int32_t halide_error_buffer_allocation_too_large(void *, const char *, int64_t, int64_t);\n"
    "int32_t halide_error_buffer_extents_too_large(void *, const char *, int64_t, int64_t);\n"
    "int32_t halide_error_explicit_bounds_too_small(void *, const char *, const char *, int32_t, int32_t, int32_t, int32_t);\n";
}

CodeGen_Zynq_C::CodeGen_Zynq_C(ostream &dest)
    : CodeGen_C(dest, false, "", zynq_headers) {
    stream  << extern_calls_prototypes
            << slice_buffer_func;
}


// added open and close device statements
void CodeGen_Zynq_C::compile(const LoweredFunc &f) {
    // Use CodeGen_C implementation for headers and external functions
    if (is_header || f.linkage == LoweredFunc::External) {
        CodeGen_C::compile(f);
        return;
    }

    internal_assert(emitted.count(f.name) == 0)
        << "Function '" << f.name << "'  has already been emitted.\n";
    emitted.insert(f.name);

    const std::vector<Argument> &args = f.args;

    have_user_context = false;
    for (size_t i = 0; i < args.size(); i++) {
        // TODO: check that its type is void *?
        have_user_context |= (args[i].name == "__user_context");
    }

    // Emit prototypes for any extern calls used.
    // TODO call ExternCallPrototypes e(stream, emitted) in CodeGen_C

    // Emit the function prototype
    // the function isn't public, mark it static.
    stream << "static ";
    stream << "int " << f.name << "(";
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i].is_buffer()) {
            stream << "buffer_t *"
                   << print_name(args[i].name)
                   << "_buffer";
        } else {
            stream << "const "
                   << print_type(args[i].type)
                   << " "
                   << print_name(args[i].name);
        }

        if (i < args.size()-1) stream << ", ";
    }

    stream << ") HALIDE_FUNCTION_ATTRS {\n";
    indent += 1;

    // Unpack the buffer_t's
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i].is_buffer()) {
            push_buffer(args[i].type, args[i].name);
        }
    }

    // open device
    do_indent();
    stream << open_device_stmt;

    // Emit the body
    print(f.body);

    // close device
    do_indent();
    stream << "close(hwacc);\n";

    // Return success.
    do_indent();
    stream << "return 0;\n";

    indent -= 1;
    stream << "}\n";

    // Done with the buffer_t's, pop the associated symbols.
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i].is_buffer()) {
            pop_buffer(args[i].name);
        }
    }

}

void CodeGen_Zynq_C::visit(const Allocate *op) {
    if (op->free_function == "_kernel_buffer") {
        open_scope();
        /* IR:
           allocate kb_var[uint8 * 8 * 8 * 128 * 128]

           C code:
           Buffer kbuf_var;
           kbuf_var.width = 128;
           kbuf_var.height =128;
           kbuf_var.depth = 64;
           kbuf_var.stride = 128;
           int get_kbuf_status = ioctl(hwacc, GET_BUFFER, (long unsigned int)&kbuf_var);
           if(get_kbuf_status < 0) {
            printf("Failed to allocate kernel buffer for var.\n");
            return 0;
           }
           uint8_t *_var = (uint8_t*) mmap(NULL,
                         kbuf_var.stride * kbuf_var.height * kbuf_var.depth,
                         PROT_WRITE, MAP_SHARED, hwacc, kbuf_var.mmap_offset);
        */
        string kbuf_name = "kbuf_" + op->name;
        string get_kbuf_status_name = "status_get_" + kbuf_name;

        // Currently kernel buffer only supports 2-D data layout,
        // so we fold lower dimensions into the 'depth' dimension
        size_t nDims = op->extents.size();
        unsigned width, height, depth;
        internal_assert(nDims >= 2);
        depth = 1;
        if (nDims > 2) {
            for (size_t i = 0; i < nDims - 2; i++)
                depth *= *as_const_int(op->extents[i]);
        }
        width = *as_const_int(op->extents[nDims-2]);
        height = *as_const_int(op->extents[nDims-1]);

        do_indent();
        stream << "Buffer " << print_name(kbuf_name) << ";\n";
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
               << "ioctl(hwacc, GET_BUFFER, (long unsigned int) &"
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
               << "PROT_WRITE, MAP_SHARED, hwacc, "
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
           ioctl(hwacc, FREE_IMAGE, (long unsigned int)&kbuf_var);
        */
        internal_assert(heap_allocations.contains(op->name));
        string kbuf_name = "kbuf_" + op->name;

        do_indent();
        stream << "munmap((void*)" << print_name(op->name) << ", "
               << print_name(kbuf_name) << ".stride * "
               << print_name(kbuf_name) << ".height * "
               << print_name(kbuf_name) << ".depth);\n";
        do_indent();
        stream << "ioctl(hwacc, FREE_IMAGE, (long unsigned int)&"
               << print_name(kbuf_name) << ");\n";

        heap_allocations.pop(op->name);
        allocations.pop(op->name);
    } else {
        CodeGen_C::visit(op);
    }
}
void CodeGen_Zynq_C::visit(const Call *op) {
    if (op->name == "linebuffer") {
    } else {
        CodeGen_C::visit(op);
    }
}

void CodeGen_Zynq_C::visit(const Realize *op) {
    // TODO add assertions

    /* IR:
       realize slice_var.stencil.stream([0, 8], [0, 8], [0, 1], [0, 1]) {
         slice_buffer("var", "slice_var", 0, 8, 0, 8, (xo * 32), 32, (yo * 32), 32)
         ...
       }

       C code:
       Buffer slice_var;
       status_slice_var = slice_buffer(&kbuf, &slice_var, xo * 32, yo * 32, 32, 32);
       if (status_slice_var < 0) {
       printf("Failed slicing buffer for var.\n");
       return 0;
       }
    */
    // check that there is a call of slice_buffer in the body
    const Block *block = op->body.as<Block>();
    internal_assert(block);
    const Evaluate *eva = block->first.as<Evaluate>();
    internal_assert(eva);
    const Call *slice_call = eva->value.as<Call>();
    internal_assert(slice_call && slice_call->name == "slice_buffer");
    open_scope();

    internal_assert(slice_call->args.size() >= 6);
    const StringImm *var_name = slice_call->args[0].as<StringImm>();
    const StringImm *slice_var_name = slice_call->args[1].as<StringImm>();
    internal_assert(var_name && slice_var_name);

    string kbuf_name = "kbuf_" + var_name->value;
    string slice_name = "slice_" + slice_var_name->value;
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
    stream << "Buffer " << print_name(slice_name) << ";\n";
    do_indent();
    stream << "int " << print_name(slice_status_name) << " = _slice_buffer(&"
           << print_name(kbuf_name) << ", &" << print_name(slice_name) << ", "
           << x_offset << ", " << y_offset << ", "
           << width << ", " << height << ");\n";
    do_indent();
    stream << "if ("<< print_name(slice_status_name) << " < 0)";
    open_scope();
    do_indent();
    stream << "printf(\"Failed to slice kernel buffer for " << var_name
           << ".\\n\");\n";
    close_scope("");

    const ProducerConsumer *pc = block->rest.as<ProducerConsumer>();
    internal_assert(pc && !pc->update.defined());

    if (starts_with(pc->name, "_hls_target.")) {
        // reachs the HW boundary
        /* C code:
           Buffer kbufs[3];
           kbufs[0] = kbuf_in0;
           kbufs[1] = kbuf_in1;
           kbufs[2] = kbuf_out;
           ioctl(hwacc, PROCESS_IMAGE, (long unsigned int)kbufs);
           ioctl(hwacc, PEND_PROCESSED, NULL);
        */
        // TODO check the order of buffer slices is consistent with
        // the order of DMA ports in the driver
        do_indent();
        stream << "Buffer _kbufs[" << buffer_slices.size() << "];\n";
        for (size_t i = 0; i < buffer_slices.size(); i++) {
            do_indent();
            stream << "_kbufs[" << i << "] = " << print_name(buffer_slices[i]) << ";\n";
        }
        do_indent();
        stream << "ioctl(hwacc, PROCESS_IMAGE, (long unsigned int)_kbufs);\n";
        do_indent();
        stream << "ioctl(hwacc, PEND_PROCESSED, NULL);\n";

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
