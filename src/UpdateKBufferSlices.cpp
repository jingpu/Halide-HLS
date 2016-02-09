#include "UpdateKBufferSlices.h"

#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Debug.h"

namespace Halide {
namespace Internal {

using std::string;


class UpdateSliceForVar :public IRMutator {
    const string &var_name;
    const Region &bounds;
    using IRMutator::visit;

    void visit(const Realize *op) {
        if (starts_with(op->name, "slice_" + var_name)) {
            debug(2) << "find slice " << op->name << "\n";
            internal_assert(op->bounds.size() == 2*bounds.size());
            Region new_bounds;
            // TODO check the slice is within the range of kernel buffer,
            // and check that lower dimensions are not sliced
            for (size_t i = 0; i < bounds.size(); i++) {
                new_bounds.push_back(op->bounds[2*i]);
                Range new_range;
                new_range.min = op->bounds[2*i + 1].min - bounds[i].min;
                new_range.extent = op->bounds[2*i + 1].extent;
                new_bounds.push_back(new_range);
            }
            stmt = Realize::make(op->name, op->types, new_bounds,
                                 op->condition, op->body);
        } else {
            IRMutator::visit(op);
        }
    }
public:
    UpdateSliceForVar(const string &s, const Region &b)
        : var_name(s), bounds(b) {}
};

class UpdateKBufferSlices : public IRMutator {
    using IRMutator::visit;

    void visit(const Realize *op) {
        if (starts_with(op->name, "kb_")) {
            string var_name = op->name.substr(3, op->name.size() - 3);
            debug(2) << "find kernel buffer " << var_name << "\n";
            for (size_t i = 0; i < op->bounds.size(); i++)
                debug(3) << "  [" << op->bounds[i].min << ", "
                         << op->bounds[i].extent << "]\n";

            // Recurses
            Stmt new_body = mutate(op->body);
            // then modifies the slice node in the body
            new_body = UpdateSliceForVar(var_name, op->bounds).mutate(new_body);

            stmt = Realize::make(op->name, op->types, op->bounds,
                                 op->condition, new_body);
        } else {
            IRMutator::visit(op);
        }
    }
public:
    UpdateKBufferSlices() {}
};

Stmt update_kbuffer_slices(Stmt s) {
    s = UpdateKBufferSlices().mutate(s);
    return s;
}

}
}
