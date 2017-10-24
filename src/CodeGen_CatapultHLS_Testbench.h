#ifndef HALIDE_CODEGEN_CATAPULTHLS_TESTBENCH_H
#define HALIDE_CODEGEN_CATAPULTHLS_TESTBENCH_H

/** \file
 *
 * Defines the code-generator for producing CatapultHLS testbench code
 */
#include <sstream>

#include "CodeGen_CatapultHLS_Base.h"
#include "CodeGen_CatapultHLS_Target.h"
#include "Module.h"
#include "Scope.h"

namespace Halide {

namespace Internal {

/** A code generator that emits Catapult HLS compatible C++ testbench code.
 */
class CodeGen_CatapultHLS_Testbench : public CodeGen_CatapultHLS_Base {
public:
    CodeGen_CatapultHLS_Testbench(std::ostream &tb_stream,
                          Target target,
                          OutputKind output_kind);
    ~CodeGen_CatapultHLS_Testbench();

protected:
    using CodeGen_CatapultHLS_Base::visit;
    void visit(const ProducerConsumer *);
    void visit(const Call *);
    void visit(const Realize *);
    void visit(const Block *);
    void visit(const Allocate *);

private:
    CodeGen_CatapultHLS_Target cg_target;
    Scope<Expr> init_scope;
};

}
}

#endif
