#include <iostream>
#include <fstream>
#include <limits>
#include <algorithm>

#include "CodeGen_HLS_Target.h"
#include "CodeGen_Internal.h"
#include "Substitute.h"
#include "IRMutator.h"
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
using std::ofstream;

namespace {

class ContainForLoop : public IRVisitor {
    using IRVisitor::visit;
    void visit(const For *op) {
        found = true;
        return;
    }

public:
    bool found;

    ContainForLoop() : found(false) {}
};

bool contain_for_loop(Stmt s) {
    ContainForLoop cfl;
    s.accept(&cfl);
    return cfl.found;
}

}

CodeGen_HLS_Target::CodeGen_HLS_Target(const string &name, Target target)
    : target_name(name),
      hdrc(hdr_stream,
           target.with_feature(Target::CPlusPlusMangling),
           CodeGen_HLS_C::CPlusPlusHeader),
      srcc(src_stream,
           target.with_feature(Target::CPlusPlusMangling),
           CodeGen_HLS_C::CPlusPlusImplementation) { }


CodeGen_HLS_Target::~CodeGen_HLS_Target() {
    hdr_stream << "#endif\n";

    // write the header and the source streams into files
    string src_name = target_name + ".cpp";
    string hdr_name = target_name + ".h";
    ofstream src_file(src_name.c_str());
    ofstream hdr_file(hdr_name.c_str());
    src_file << src_stream.str() << endl;
    hdr_file << hdr_stream.str() << endl;
    src_file.close();
    hdr_file.close();
}

namespace {
const string hls_header_includes =
    "#include <assert.h>\n"
    "#include <stdio.h>\n"
    "#include <stdlib.h>\n"
    "#include <hls_stream.h>\n"
    "#include \"Stencil.h\"\n";
}

void CodeGen_HLS_Target::init_module() {
    debug(1) << "CodeGen_HLS_Target::init_module\n";

    // wipe the internal streams
    hdr_stream.str("");
    hdr_stream.clear();
    src_stream.str("");
    src_stream.clear();

    // initialize the header file
    string module_name = "HALIDE_CODEGEN_HLS_TARGET_" + target_name + "_H";
    std::transform(module_name.begin(), module_name.end(), module_name.begin(), toupper);
    hdr_stream << "#ifndef " << module_name << '\n';
    hdr_stream << "#define " << module_name << "\n\n";
    hdr_stream << hls_header_includes << '\n';

    // initialize the source file
    src_stream << "#include \"" << target_name << ".h\"\n\n";
    src_stream << "#include \"Linebuffer.h\"\n"
               << "#include \"halide_math.h\"\n";

}

void CodeGen_HLS_Target::add_kernel(Stmt s,
                                    const string &name,
                                    const vector<HLS_Argument> &args) {
    debug(1) << "CodeGen_HLS_Target::add_kernel " << name << "\n";

    hdrc.add_kernel(s, name, args);
    srcc.add_kernel(s, name, args);
}

void CodeGen_HLS_Target::dump() {
    std::cerr << src_stream.str() << std::endl;
}


string CodeGen_HLS_Target::CodeGen_HLS_C::print_stencil_pragma(const string &name) {
    ostringstream oss;
    internal_assert(stencils.contains(name));
    Stencil_Type stype = stencils.get(name);
    if (stype.type == Stencil_Type::StencilContainerType::Stream ||
        stype.type == Stencil_Type::StencilContainerType::AxiStream) {
        oss << "#pragma HLS STREAM variable=" << print_name(name) << " depth=" << stype.depth << "\n";
        if (stype.depth <= 100) {
            // use shift register implementation when the FIFO is shallow
            oss << "#pragma HLS RESOURCE variable=" << print_name(name) << " core=FIFO_SRL\n\n";
        }
    } else if (stype.type == Stencil_Type::StencilContainerType::Stencil) {
        oss << "#pragma HLS ARRAY_PARTITION variable=" << print_name(name) << ".value complete dim=0\n\n";
    } else {
        internal_error;
    }
    return oss.str();
}

string CodeGen_HLS_Target::CodeGen_HLS_C::guess_name(const string &arg_name,
                                                     uint32_t index) {

    /**
     * Attempt to extract useful names from the arg name.
     * If it fails, fall back to arg_%d as before.
     */
    string name = arg_name;
    auto pos = name.find(":");
    if (pos != std::string::npos) {
        /* we don't need the pipe: part */
        name = name.substr(++pos);
    }
    pos = name.find(".");
    if (pos != std::string::npos) {
        /* $ does compile, but it's more confusing since it implies a naming
         * conflift in Halide */
        if (name.find("$") == std::string::npos)
            return name.substr(0, pos);
    }
    /* fall back */
    return "arg_" + std::to_string(index);
}

void CodeGen_HLS_Target::CodeGen_HLS_C::add_kernel(Stmt stmt,
                                                   const string &name,
                                                   const vector<HLS_Argument> &args) {
    // Emit the function prototype
    stream << "void " << name << "(\n";
    for (size_t i = 0; i < args.size(); i++) {
        string arg_name = guess_name(args[i].name, i);
        if (args[i].is_stencil) {
            CodeGen_HLS_Base::Stencil_Type stype = args[i].stencil_type;
            internal_assert(args[i].stencil_type.type == Stencil_Type::StencilContainerType::AxiStream ||
                            args[i].stencil_type.type == Stencil_Type::StencilContainerType::Stencil);
            stream << print_stencil_type(args[i].stencil_type) << " ";
            if (args[i].stencil_type.type == Stencil_Type::StencilContainerType::AxiStream) {
                stream << "&";  // hls_stream needs to be passed by reference
            }
            stream << arg_name;
            allocations.push(args[i].name, {args[i].stencil_type.elemType});
            stencils.push(args[i].name, args[i].stencil_type);
        } else {
            stream << print_type(args[i].scalar_type) << " " << arg_name;
        }

        if (i < args.size()-1) stream << ",\n";
    }

    if (is_header()) {
        stream << ");\n";
    } else {
        stream << ")\n";
        open_scope();

        // add HLS pragma at function scope
        stream << "#pragma HLS DATAFLOW\n"
               << "#pragma HLS INLINE region\n"
               << "#pragma HLS INTERFACE s_axilite port=return"
               << " bundle=config\n";
        for (size_t i = 0; i < args.size(); i++) {
            string arg_name = guess_name(args[i].name, i);
            if (args[i].is_stencil) {
                if (ends_with(args[i].name, ".stream")) {
                    // stream arguments use AXI-stream interface
                    stream << "#pragma HLS INTERFACE axis register "
                           << "port=" << arg_name << "\n";
                } else {
                    // stencil arguments use AXI-lite interface
                    stream << "#pragma HLS INTERFACE s_axilite "
                           << "port=" << arg_name
                           << " bundle=config\n";
                    stream << "#pragma HLS ARRAY_PARTITION "
                           << "variable=" << arg_name << ".value complete dim=0\n";
                }
            } else {
                // scalar arguments use AXI-lite interface
                stream << "#pragma HLS INTERFACE s_axilite "
                       << "port=" << arg_name << " bundle=config\n";
            }
        }
        stream << "\n";

        // create alias (references) of the arguments using the names in the IR
        do_indent();
        stream << "// alias the arguments\n";
        for (size_t i = 0; i < args.size(); i++) {
            string arg_name = guess_name(args[i].name, i);
            do_indent();
            if (args[i].is_stencil) {
                CodeGen_HLS_Base::Stencil_Type stype = args[i].stencil_type;
                stream << print_stencil_type(args[i].stencil_type) << " &"
                       << print_name(args[i].name) << " = " << arg_name << ";\n";
            } else {
                stream << print_type(args[i].scalar_type) << " &"
                       << print_name(args[i].name) << " = " << arg_name << ";\n";
            }
        }
        stream << "\n";

        // print body
        print(stmt);

        close_scope("kernel hls_target" + print_name(name));
    }
    stream << "\n";

    for (size_t i = 0; i < args.size(); i++) {
        // Remove buffer arguments from allocation scope
        if (args[i].stencil_type.type == Stencil_Type::StencilContainerType::Stream) {
            allocations.pop(args[i].name);
            stencils.pop(args[i].name);
        }
    }
}

// almost that same as CodeGen_C::visit(const For *)
// we just add a 'HLS PIPELINE' pragma after the 'for' statement
void CodeGen_HLS_Target::CodeGen_HLS_C::visit(const For *op) {
    internal_assert(op->for_type == ForType::Serial)
        << "Can only emit serial for loops to HLS C\n";

    string id_min = print_expr(op->min);
    string id_extent = print_expr(op->extent);

    do_indent();
    stream << "for (int "
           << print_name(op->name)
           << " = " << id_min
           << "; "
           << print_name(op->name)
           << " < " << id_min
           << " + " << id_extent
           << "; "
           << print_name(op->name)
           << "++)\n";

    open_scope();
    // add a 'PIPELINE' pragma if it is an innermost loop
    if (!contain_for_loop(op->body)) {
        //stream << "#pragma HLS DEPENDENCE array inter false\n"
        //       << "#pragma HLS LOOP_FLATTEN off\n";
        stream << "#pragma HLS PIPELINE II=1\n";
    }
    op->body.accept(this);
    close_scope("for " + print_name(op->name));
}

class RenameAllocation : public IRMutator {
    const string &orig_name;
    const string &new_name;

    using IRMutator::visit;

    void visit(const Load *op) {
        if (op->name == orig_name ) {
            Expr index = mutate(op->index);
            expr = Load::make(op->type, new_name, index, op->image, op->param, op->predicate);
        } else {
            IRMutator::visit(op);
        }
    }

    void visit(const Store *op) {
        if (op->name == orig_name ) {
            Expr value = mutate(op->value);
            Expr index = mutate(op->index);
            stmt = Store::make(new_name, value, index, op->param, op->predicate);
        } else {
            IRMutator::visit(op);
        }
    }

    void visit(const Free *op) {
        if (op->name == orig_name) {
            stmt = Free::make(new_name);
        } else {
            IRMutator::visit(op);
        }
    }

public:
    RenameAllocation(const string &o, const string &n)
        : orig_name(o), new_name(n) {}
};

// most code is copied from CodeGen_C::visit(const Allocate *)
// we want to check that the allocation size is constant, and
// add a 'HLS ARRAY_PARTITION' pragma to the allocation
void CodeGen_HLS_Target::CodeGen_HLS_C::visit(const Allocate *op) {
    // We don't add scopes, as it messes up the dataflow directives in HLS compiler.
    // Instead, we rename the allocation to a unique name
    //open_scope();

    internal_assert(!op->new_expr.defined());
    internal_assert(!is_zero(op->condition));
    int32_t constant_size;
    constant_size = op->constant_allocation_size();
    if (constant_size > 0) {

    } else {
        internal_error << "Size for allocation " << op->name
                       << " is not a constant.\n";
    }

    // rename allocation to avoid name conflict due to unrolling
    string alloc_name = op->name + unique_name('a');
    Stmt new_body = RenameAllocation(op->name, alloc_name).mutate(op->body);

    Allocation alloc;
    alloc.type = op->type;
    allocations.push(alloc_name, alloc);

    do_indent();
    stream << print_type(op->type) << ' '
           << print_name(alloc_name)
           << "[" << constant_size << "];\n";
    // add a 'ARRAY_PARTITION" pragma
    //stream << "#pragma HLS ARRAY_PARTITION variable=" << print_name(op->name) << " complete dim=0\n\n";

    new_body.accept(this);

    // Should have been freed internally
    internal_assert(!allocations.contains(alloc_name))
        << "allocation " << alloc_name << " is not freed.\n";

    //close_scope("alloc " + print_name(op->name));

}

}
}
