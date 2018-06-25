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

    bool is_stencil;

    Type scalar_type;

    CodeGen_HLS_Base::Stencil_Type stencil_type;
};

/** This class emits Xilinx Vivado HLS compatible C++ code.
 */
class CodeGen_HLS_Target {
public:
    /** Initialize a C code generator pointing at a particular output
     * stream (e.g. a file, or std::cout) */
    CodeGen_HLS_Target(const std::string &name, Target target);
    virtual ~CodeGen_HLS_Target();

    void init_module();

    void add_kernel(Stmt stmt,
                    const std::string &name,
                    const std::vector<HLS_Argument> &args);

    void dump();

protected:
    class CodeGen_HLS_C : public CodeGen_HLS_Base {
    public:
        CodeGen_HLS_C(std::ostream &s, Target target, OutputKind output_kind)
            : CodeGen_HLS_Base(s, target, output_kind) {}

        void add_kernel(Stmt stmt,
                        const std::string &name,
                        const std::vector<HLS_Argument> &args);

    protected:
        std::string print_stencil_pragma(const std::string &name);

        using CodeGen_HLS_Base::visit;

        void visit(const For *op);
        void visit(const Allocate *op);
    private:
        /**
         * Attempt to extract useful names from the arg name.
         * If it fails, fall back to arg_%d as before.
         */
        static std::string get_arg_name(const std::string &name, uint32_t index) const;
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
