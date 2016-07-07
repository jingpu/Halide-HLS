#ifndef HALIDE_CODEGEN_ZYNQ_LLVM_H
#define HALIDE_CODEGEN_ZYNQ_LLVM_H

/** \file
 *
 * Defines an class of Zynq LLVM code-generator
 */
#include "CodeGen_X86.h"
#include "Module.h"

namespace Halide {

namespace Internal {

/** This class emits LLVM module for Zynq from given Halide stmt that contains
 * hardware accelerators. The interfacing to hardware will be replaced
 * with Zynq Linux driver calls.
 */
class CodeGen_Zynq_LLVM : public CodeGen_X86 {
public:
    CodeGen_Zynq_LLVM(Target t);

protected:
    std::vector<llvm::Value *> buffer_slices;

    using CodeGen_X86::visit;

    void visit(const Realize *);
    void visit(const ProducerConsumer *op);
    void visit(const Call *);
};

}
}

#endif
