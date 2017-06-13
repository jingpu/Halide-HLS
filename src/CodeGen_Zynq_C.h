#ifndef HALIDE_CODEGEN_ZYNQ_C_H
#define HALIDE_CODEGEN_ZYNQ_C_H

/** \file
 *
 * Defines an class of Zynq C code-generator
 */
#include "CodeGen_C.h"
#include "Module.h"
#include "Scope.h"

namespace Halide {

namespace Internal {

/** This class emits Zynq C code from given Halide stmt that contains
 * hardware accelerators. The interfacing to hardware will be replaced
 * with Zynq Linux driver calls.
 */
class CodeGen_Zynq_C : public CodeGen_C {
public:
    /** Initialize a C code generator pointing at a particular output
     * stream (e.g. a file, or std::cout) */
    CodeGen_Zynq_C(std::ostream &dest,
                   Target target,
                   OutputKind output_kind);

protected:
    std::vector<std::string> buffer_slices;

    using CodeGen_C::visit;

    void visit(const Realize *);
    void visit(const ProducerConsumer *op);
    void visit(const Call *);
};

}
}

#endif
