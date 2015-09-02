#include <iostream>
#include <fstream>
#include <limits>
#include <algorithm>

#include "CodeGen_HLS_Target.h"
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
using std::ofstream;

CodeGen_HLS_Target::CodeGen_HLS_Target(const string &name)
    : target_name(name),
      hdrc(hdr_stream, true),
      srcc(src_stream, false) { }


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
    "#include \"stencil.h\"\n";
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


void CodeGen_HLS_Target::CodeGen_HLS_C::add_kernel(Stmt stmt,
                                                   const string &name,
                                                   const vector<HLS_Argument> &args) {
    // Emit the function prototype
    stream << "void " << print_name(name) << "(\n";
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i].is_stencil) {
            CodeGen_HLS_Base::Stencil_Type stype = args[i].stencil_type;
            stream << print_stencil_type(args[i].stencil_type) << " ";
            if (stype.is_stream) {
                stream << "&";  // hls_stream needs to be passed by reference
            }
            stream << print_name(args[i].name);
            allocations.push(args[i].name, {args[i].stencil_type.type, "null"});
            stencils.push(args[i].name, args[i].stencil_type);
        } else {
            stream << print_type(args[i].scalar_type) << " " << print_name(args[i].name);
        }

        if (i < args.size()-1) stream << ",\n";
    }

    if (is_header) {
        stream << ");\n";
    } else {
        stream << ")\n";
        open_scope();
        print(stmt);
        close_scope("kernel hls_target" + print_name(name));
    }
    stream << "\n";

    for (size_t i = 0; i < args.size(); i++) {
        // Remove buffer arguments from allocation scope
        if (args[i].stencil_type.is_stream) {
            allocations.pop(args[i].name);
            stencils.pop(args[i].name);
        }
    }

}

}
}
