#ifndef HALIDE_CODEGEN_HLS_TARGET_H
#define HALIDE_CODEGEN_HLS_TARGET_H

/** \file
 *
 * Defines an IRPrinter that emits HLS C++ code.
 */

#include "CodeGen_HLS_Base.h"
#include "CodeGen_Internal.h"
#include "Module.h"
#include "Scope.h"
#include <tuple>

namespace Halide {

namespace Internal {

struct HLS_Argument {
    std::string name;

    bool is_stencil;

    Type scalar_type;

    CodeGen_HLS_Base::Stencil_Type stencil_type;
};


class HLS_Closure : public Closure {
public:
    HLS_Closure(Stmt s);

    std::vector<HLS_Argument> arguments(const Scope<CodeGen_HLS_Base::Stencil_Type> &scope);

protected:
    using Closure::visit;

};

/** This class emits Xilinx Vivado HLS compatible C++ code.
 */
class CodeGen_HLS_Target {
public:
    /** Initialize a C code generator pointing at a particular output
     * stream (e.g. a file, or std::cout) */
    CodeGen_HLS_Target(const std::string &name);
    virtual ~CodeGen_HLS_Target();

    void init_module();

    void add_kernel(Stmt stmt,
                    const std::string &name,
                    const std::vector<HLS_Argument> &args);

    void dump();

protected:
    class CodeGen_HLS_C : public CodeGen_HLS_Base {
    public:
        CodeGen_HLS_C(std::ostream &s, OutputKind output_kind) : CodeGen_HLS_Base(s, output_kind) {}

        void add_kernel(Stmt stmt,
                        const std::string &name,
                        const std::vector<HLS_Argument> &args);
        std::vector<std::tuple<Stmt, std::string, std::vector<HLS_Argument>>> subroutines;

    protected:
        std::string print_stencil_pragma(const std::string &name);

        using CodeGen_HLS_Base::visit;

        void visit(const ProducerConsumer *op);
        void visit(const For *op);
        void visit(const Allocate *op);
    };

    /** A name for the HLS target */
    std::string target_name;

    /** String streams for building header and source files. */
    // @{
    std::ostringstream hdr_stream;
    std::ostringstream src_stream;
    // @}

    /** Code generators for HLS target header and the source. */
    // @{
    CodeGen_HLS_C hdrc;
    CodeGen_HLS_C srcc;
    // @}
};

}
}

#endif
