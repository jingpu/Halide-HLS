#include <iostream>
#include <limits>

#include "CodeGen_HLS_Testbench.h"
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
using std::pair;
using std::map;

class HLS_Closure : public Closure {
public:
    HLS_Closure(Stmt s)  {
        s.accept(this);
    }

    vector<HLS_Argument> arguments(const Scope<CodeGen_HLS_Base::Stencil_Type> &scope);

protected:
    using Closure::visit;
    void visit(const Realize *op);

};


vector<HLS_Argument> HLS_Closure::arguments(const Scope<CodeGen_HLS_Base::Stencil_Type> &streams_scope) {
    vector<HLS_Argument> res;
    for (const pair<string, Closure::BufferRef> &i : buffers) {
        debug(3) << "buffer: " << i.first << " " << i.second.size;
        if (i.second.read) debug(3) << " (read)";
        if (i.second.write) debug(3) << " (write)";
        debug(3) << "\n";
    }
    internal_assert(buffers.empty()) << "we expect no references to buffers in a hw pipeline.\n";
    for (const pair<string, Type> &i : vars) {
        debug(3) << "var: " << i.first << "\n";
        if(ends_with(i.first, ".stream")) {
            res.push_back({i.first, streams_scope.get(i.first)});
        } else {
            internal_assert(false) << "we cannot handle non-stream arguments yet.\n";
        }
    }
    return res;
}

void HLS_Closure::visit(const Realize *op) {
    for (size_t i = 0; i < op->bounds.size(); i++) {
        op->bounds[i].min.accept(this);
        op->bounds[i].extent.accept(this);
    }
    op->condition.accept(this);
    ignore.push(op->name, 0);
    op->body.accept(this);
    ignore.pop(op->name);
}


namespace {
const string hls_headers =
    "#include <hls_stream.h>\n"
    "#include \"stencil.h\"\n"
    "#include \"hls_target.h\"\n";
}

CodeGen_HLS_Testbench::CodeGen_HLS_Testbench(ostream &tb_stream)
    : CodeGen_HLS_Base(tb_stream, false, "", hls_headers),
      cg_target("hls_target") {
    cg_target.init_module();
}

CodeGen_HLS_Testbench::~CodeGen_HLS_Testbench() {
}

void CodeGen_HLS_Testbench::visit(const ProducerConsumer *op) {
    if (ends_with(op->name, ".stencil.stream")) {
        HLS_Closure c(op->produce);
        // TODO support other closure
        if(c.buffers.empty()) {
            vector<HLS_Argument> args = c.arguments(stencils);

            // generate HLS target code using the child code generator
            cg_target.add_kernel(op->produce, op->name, args);

            do_indent();
            stream << "// produce " << op->name << '\n';
            do_indent();
            stream << "hls_target" << print_name(op->name) << "(";
            for(size_t i = 0; i < args.size(); i++) {
                stream << print_name(args[i].name);
                if(i != args.size() - 1)
                    stream << ", ";
            }
            stream <<");\n";
            do_indent();
            stream << "// consume " << op->name << '\n';
            print_stmt(op->consume);
        } else {
            CodeGen_HLS_Base::visit(op);
        }
    } else {
        CodeGen_HLS_Base::visit(op);
    }
}

}
}
