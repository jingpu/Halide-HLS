#ifndef HALIDE_CODEGEN_HLS_TESTBENCH_H
#define HALIDE_CODEGEN_HLS_TESTBENCH_H

/** \file
 *
 * Defines the code-generator for producing HLS testbench code
 */
#include <sstream>

#include "CodeGen_HLS_Base.h"
#include "CodeGen_HLS_Target.h"
#include "Module.h"
#include "Scope.h"

namespace Halide {

namespace Internal {

/** A code generator that emits Xilinx Vivado HLS compatible C++ testbench code.
 */
class CodeGen_HLS_Testbench : public CodeGen_HLS_Base {
public:
    CodeGen_HLS_Testbench(std::ostream &tb_stream,
                          Target target,
                          OutputKind output_kind);
    ~CodeGen_HLS_Testbench();

protected:
    using CodeGen_HLS_Base::visit;
    void visit(const ProducerConsumer *);
    void visit(const Call *);
    void visit(const Realize *);
    void visit(const Block *);
    void visit(const Allocate *);

private:
    CodeGen_HLS_Target cg_target;
    Scope<Expr> init_scope;
};

}
}

#endif
