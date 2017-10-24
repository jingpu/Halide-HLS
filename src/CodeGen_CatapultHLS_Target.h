#ifndef HALIDE_CODEGEN_CATAPULTHLS_TARGET_H
#define HALIDE_CODEGEN_CATAPULTHLS_TARGET_H

/** \file
 *
 * Defines an IRPrinter that emits CatapultHLS C++ code.
 */

#include "CodeGen_CatapultHLS_Base.h"
#include "Module.h"
#include "Scope.h"

namespace Halide {

namespace Internal {

struct CatapultHLS_Argument {
    std::string name;

    bool is_stencil;

    Expr size;

    Type scalar_type;

    CodeGen_CatapultHLS_Base::Stencil_Type stencil_type;
};

/** This class emits Catapult HLS compatible C++ code.
 */
class CodeGen_CatapultHLS_Target {
public:
    /** Initialize a C code generator pointing at a particular output
     * stream (e.g. a file, or std::cout) */
    CodeGen_CatapultHLS_Target(const std::string &name, Target target);
    virtual ~CodeGen_CatapultHLS_Target();

    void init_module();

    void add_kernel(Stmt stmt,
                    const std::string &name,
                    const std::vector<CatapultHLS_Argument> &args);

    void dump();

protected:
    class CodeGen_CatapultHLS_C : public CodeGen_CatapultHLS_Base {
    public:
        CodeGen_CatapultHLS_C(std::ostream &s, Target target, OutputKind output_kind)
            : CodeGen_CatapultHLS_Base(s, target, output_kind) {}

        void add_kernel(Stmt stmt,
                        const std::string &name,
                        const std::vector<CatapultHLS_Argument> &args);

    protected:
        std::string print_stencil_pragma(const std::string &name);

        using CodeGen_CatapultHLS_Base::visit;

        void visit(const For *op);
        void visit(const Allocate *op);
    };

    /** A name for the CatapultHLS target */
    std::string target_name;

    /** String streams for building header and source files. */
    // @{
    std::ostringstream hdr_stream;
    std::ostringstream src_stream;
    // @}

    /** Code generators for CatapultHLS target header and the source. */
    // @{
    CodeGen_CatapultHLS_C hdrc;
    CodeGen_CatapultHLS_C srcc;
    // @}
};

}
}

#endif
