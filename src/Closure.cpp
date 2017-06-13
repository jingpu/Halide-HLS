#include "Closure.h"
#include "Debug.h"

namespace Halide {
namespace Internal {

using std::string;
using std::map;
using std::vector;
using std::pair;

Closure::Closure(Stmt s, const string &loop_variable) {
    if (!loop_variable.empty()) {
        ignore.push(loop_variable, 0);
    }
    s.accept(this);
}

void Closure::visit(const Let *op) {
    op->value.accept(this);
    ignore.push(op->name, 0);
    op->body.accept(this);
    ignore.pop(op->name);
}

void Closure::visit(const LetStmt *op) {
    op->value.accept(this);
    ignore.push(op->name, 0);
    op->body.accept(this);
    ignore.pop(op->name);
}

void Closure::visit(const For *op) {
    ignore.push(op->name, 0);
    op->min.accept(this);
    op->extent.accept(this);
    op->body.accept(this);
    ignore.pop(op->name);
}

void Closure::found_buffer_ref(const string &name, Type type,
                               bool read, bool written, Halide::Buffer<> image) {
    if (!ignore.contains(name)) {
        debug(3) << "Adding buffer " << name << " to closure\n";
        Buffer &ref = buffers[name];
        ref.type = type.element_of(); // TODO: Validate type is the same as existing refs?
        ref.read = read;
        ref.write = written;

        // If reading an image/buffer, compute the size.
        if (image.defined()) {
            ref.size = image.size_in_bytes();
            ref.dimensions = image.dimensions();
        }
    } else {
        debug(3) << "Not adding " << name << " to closure\n";
    }
}

void Closure::visit(const Load *op) {
    op->predicate.accept(this);
    op->index.accept(this);
    found_buffer_ref(op->name, op->type, true, false, op->image);
}

void Closure::visit(const Store *op) {
    op->predicate.accept(this);
    op->index.accept(this);
    op->value.accept(this);
    found_buffer_ref(op->name, op->value.type(), false, true, Halide::Buffer<>());
}

void Closure::visit(const Allocate *op) {
    if (op->new_expr.defined()) {
        op->new_expr.accept(this);
    }
    ignore.push(op->name, 0);
    for (size_t i = 0; i < op->extents.size(); i++) {
        op->extents[i].accept(this);
    }
    op->body.accept(this);
    ignore.pop(op->name);
}

void Closure::visit(const Variable *op) {
    if (ignore.contains(op->name)) {
        debug(3) << "Not adding " << op->name << " to closure\n";
    } else {
        debug(3) << "Adding " << op->name << " to closure\n";
        vars[op->name] = op->type;
    }
}

void Closure::visit(const Realize *op) {
    for (size_t i = 0; i < op->bounds.size(); i++) {
        op->bounds[i].min.accept(this);
        op->bounds[i].extent.accept(this);
    }
    op->condition.accept(this);
    ignore.push(op->name, 0);
    op->body.accept(this);
    ignore.pop(op->name);
}

void Closure::visit(const Call *op) {
    if (op->name == "create_kbuf") {
        // adds the variable to ignore list
        debug(3) << "visit call " << op->name << ": ";
        internal_assert(op->args.size() == 1);
        const Variable *var = op->args[0].as<Variable>();
        internal_assert(var);
        internal_assert(!ignore.contains(var->name))
            << "kbuf_t " << var->name << " is already created.\n";
        debug(3) << "adding " << var->name << " to closure.\n";
        ignore.push(var->name, 0);
    } else if (op->call_type == Call::Intrinsic &&
        (ends_with(op->name, ".stencil") || ends_with(op->name, ".stencil_update"))) {
        // consider call to stencil and stencil_update
        debug(3) << "visit call " << op->name << ": ";
        if(!ignore.contains(op->name)) {
            debug(3) << "adding to closure.\n";
            vars[op->name] = Type();
        } else {
            debug(3) << "not adding to closure.\n";
        }
    }
    IRVisitor::visit(op);
}

}
}
