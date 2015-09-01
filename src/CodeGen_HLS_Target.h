#ifndef HALIDE_CODEGEN_HLS_TARGET_H
#define HALIDE_CODEGEN_HLS_TARGET_H

/** \file
 *
 * Defines an IRPrinter that emits HLS C++ code.
 */

#include "CodeGen_HLS_Base.h"
#include "Module.h"
#include "Scope.h"

namespace Halide {

namespace Internal {

struct HLS_Argument {
    std::string name;

    CodeGen_HLS_Base::Stencil_Type stencil_type;
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
        CodeGen_HLS_C(std::ostream &s, bool is_header) : CodeGen_HLS_Base(s, is_header) {}

        void add_kernel(Stmt stmt,
                        const std::string &name,
                        const std::vector<HLS_Argument> &args);

    protected:
        using CodeGen_HLS_Base::visit;
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
