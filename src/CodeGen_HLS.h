#ifndef HALIDE_CODEGEN_HLS_H
#define HALIDE_CODEGEN_HLS_H

/** \file
 *
 * Defines an IRPrinter that emits HLS C++ code.
 */

#include "CodeGen_C.h"
#include "Module.h"
#include "Scope.h"

namespace Halide {

namespace Internal {

/** This class emits Xilinx Vivado HLS compatible C++ code.
 */
class CodeGen_HLS : public CodeGen_C {
public:
    /** Initialize a C code generator pointing at a particular output
     * stream (e.g. a file, or std::cout) */
    CodeGen_HLS(std::ostream &dest, bool is_header = false, const std::string &include_guard = "");
    ~CodeGen_HLS();


protected:

    using CodeGen_C::visit;

    void visit(const Call *);
    void visit(const Provide *);
    void visit(const Realize *);
};

}
}

#endif
