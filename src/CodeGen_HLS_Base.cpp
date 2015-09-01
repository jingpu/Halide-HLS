#include <iostream>
#include <limits>

#include "CodeGen_HLS_Base.h"
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

string CodeGen_HLS_Base::print_stencil_type(Stencil_Type stencil_type) {
    ostringstream oss;
    // C: Stencil<uint16_t, 1, 1, 1> stencil_var;
    // C: hls::stream<Stencil<uint16_t, 1, 1, 1> > stencil_stream_var;
    if(stencil_type.is_stream)
        oss << "hls::stream<";

    oss << "Stencil<" << print_type(stencil_type.type);
    for(const auto &range : stencil_type.bounds)
        oss << ", " << range.extent;
    oss << ">";

    if(stencil_type.is_stream)
        oss << " >";

    return oss.str();
}

void CodeGen_HLS_Base::visit(const Call *op) {
    if (op->name == "linebuffer") {
        //IR: linebuffer(buffered.stencil_update.stream, buffered.stencil.stream, extent_0[, extent_1, ...])
        //C: linebuffer<extent_0[, extent_1, ...]>(buffered.stencil_update.stream, buffered.stencil.stream)
        internal_assert(op->args.size() >= 3);
        string a0 = print_expr(op->args[0]);
        string a1 = print_expr(op->args[1]);
        do_indent();
        stream << "linebuffer<";
        for(size_t i = 2; i < op->args.size(); i++) {
            stream << print_expr(op->args[i]);
            if (i != op->args.size() -1)
                stream << ", ";
        }
        stream << ">(" << a0 << ", " << a1 << ");\n";
        id = "0"; // skip evaluation
    } else if (op->name == "write_stream") {
        // IR: write_stream(buffered.stencil_update.stream, buffered.stencil_update)
        // C: buffered_stencil_update_stream.write(buffered_stencil_update);
        internal_assert(op->args.size() == 2);
        string a0 = print_expr(op->args[0]);
        string a1 = print_expr(op->args[1]);
        do_indent();
        stream << a0 << ".write(" << a1 << ");\n";
        id = "0"; // skip evaluation
    } else if (op->name == "read_stream") {
        internal_assert(op->args.size() == 2);
        string a0 = print_expr(op->args[0]);
        string a1 = print_expr(op->args[1]);
        do_indent();
        stream << a1 << " = " << a0 << ".read();\n";
        id = "0"; // skip evaluation
    } else if (ends_with(op->name, ".stencil") ||
               ends_with(op->name, ".stencil_update")) {
        ostringstream rhs;
        // IR: out.stencil_update(0, 0, 0)
        // C: out_stencil_update(0, 0, 0)
        vector<string> args_indices(op->args.size());
        for(size_t i = 0; i < op->args.size(); i++)
            args_indices[i] = print_expr(op->args[i]);

        rhs << print_name(op->name) << "(";
        for(size_t i = 0; i < op->args.size(); i++) {
            rhs << args_indices[i];
            if (i != op->args.size() - 1)
                rhs << ", ";
        }
        rhs << ")";

        print_assignment(op->type, rhs.str());
    } else {
        CodeGen_C::visit(op);
    }
}

void CodeGen_HLS_Base::visit(const Realize *op) {
    if (ends_with(op->name, ".stream")) {
        // create a stream type
        open_scope();
        internal_assert(op->types.size() == 1);
        allocations.push(op->name, {op->types[0], "null"});
        Stencil_Type stype({true, op->types[0], op->bounds});
        stencils.push(op->name, stype);

        do_indent();
        // C: hls::stream<Stencil<uint16_t, 1, 1, 1> > conv1_stencil_update_stream;
        stream << print_stencil_type(stype) << ' ' << print_name(op->name) << ";\n";

        op->body.accept(this);

        // We didn't generate free stmt inside for stream type
        allocations.pop(op->name);
        stencils.pop(op->name);

        close_scope("realize " + print_name(op->name));
    } else if (ends_with(op->name, ".stencil") ||
               ends_with(op->name, ".stencil_update")) {
        // create a stencil type
        open_scope();
        internal_assert(op->types.size() == 1);
        allocations.push(op->name, {op->types[0], "null"});
        Stencil_Type stype({false, op->types[0], op->bounds});
        stencils.push(op->name, stype);

        do_indent();
        // Stencil<uint16_t, 1, 1, 1> conv1_stencil_update;
        stream << print_stencil_type(stype) << ' ' << print_name(op->name) << ";\n";

        op->body.accept(this);

        // We didn't generate free stmt inside for stream type
        allocations.pop(op->name);
        stencils.pop(op->name);

        close_scope("realize " + print_name(op->name));
    } else {
        CodeGen_C::visit(op);
    }
}

void CodeGen_HLS_Base::visit(const Provide *op) {
    if (ends_with(op->name, ".stencil") ||
        ends_with(op->name, ".stencil_update")) {
        // IR: buffered.stencil_update(1, 2, 3) =
        // C: buffered_stencil_update(1, 2, 3) =
        vector<string> args_indices(op->args.size());
        for(size_t i = 0; i < op->args.size(); i++)
            args_indices[i] = print_expr(op->args[i]);

        internal_assert(op->values.size() == 1);
        string id_value = print_expr(op->values[0]);

        do_indent();
        stream << print_name(op->name) << "(";

        for(size_t i = 0; i < op->args.size(); i++) {
            stream << args_indices[i];
            if (i != op->args.size() - 1)
                stream << ", ";
        }
        stream << ") = " << id_value << ";\n";

        cache.clear();
    } else {
        CodeGen_C::visit(op);
    }
}

}
}
