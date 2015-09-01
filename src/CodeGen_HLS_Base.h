#ifndef HALIDE_CODEGEN_HLS_BASE_H
#define HALIDE_CODEGEN_HLS_BASE_H

/** \file
 *
 * Defines an base class of the HLS C code-generator
 */
#include "CodeGen_C.h"
#include "Module.h"
#include "Scope.h"

namespace Halide {

namespace Internal {

/** This class emits C++ code from given Halide stmt that contains
 * stream and stencil types.
 */
class CodeGen_HLS_Base : public CodeGen_C {
public:
    /** Initialize a C code generator pointing at a particular output
     * stream (e.g. a file, or std::cout) */
    CodeGen_HLS_Base(std::ostream &dest, bool is_header = false,
                     const std::string &include_guard = "", const std::string &additional_headers = "")
        : CodeGen_C(dest, is_header, include_guard, additional_headers) {}

    struct Stencil_Type {
        bool is_stream;  // is is a stream of stencils?
        Type type;  // type of the element
        Region bounds;  // extent of each dimension
    };

protected:
    Scope<Stencil_Type> stencils;  // scope of stencils and streams of stencils

    std::string print_stencil_type(Stencil_Type);

    using CodeGen_C::visit;

    void visit(const Call *);
    void visit(const Provide *);
    void visit(const Realize *);
};

}
}

#endif
