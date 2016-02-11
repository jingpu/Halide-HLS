#include "UpdateKBufferSlices.h"

#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Debug.h"
#include "Bounds.h"

namespace Halide {
namespace Internal {

using std::string;
using std::vector;
using std::map;


class UpdateSliceForFunc :public IRMutator {
    const string &func_name;
    const Region &bounds;
    using IRMutator::visit;

    void visit(const Realize *op) {
        // removes possible "__auto_insert__" prefix
        string prefix = "__auto_insert__";
        string var_name = op->name;
        if (starts_with(var_name, prefix)) {
            var_name = var_name.substr(prefix.size(), var_name.size() - prefix.size());
        }
        // removes possoble suffix ".stencil.stream" or ".stencil_update.stream"
        size_t pos_suffix = var_name.rfind(".stencil");
        var_name =  var_name.substr(0, pos_suffix);
        if (ends_with(func_name, var_name)) {
            debug(3) << "find buffer slice " << op->name << "\n";
            Box b = box_provided(op->body, func_name);
            merge_boxes(b, box_required(op->body, func_name));

            vector<Expr> args({func_name, var_name});
            internal_assert(b.size() == bounds.size());
            for(size_t i = 0; i < b.size(); i++) {
                args.push_back(b[i].min - bounds[i].min); // offset the min possible according to the realize bounds
                args.push_back(b[i].max - b[i].min + 1);
            }

            Stmt slice_call = Evaluate::make(Call::make(Handle(), "slice_buffer", args, Call::Intrinsic));
            stmt = Realize::make(op->name, op->types, op->bounds, op->condition,
                                 Block::make(slice_call, op->body));
        } else {
            IRMutator::visit(op);
        }
    }
public:
    UpdateSliceForFunc(const string &s, const Region &b)
        : func_name(s), bounds(b) {}
};

class UpdateKBufferSlices : public IRMutator {
    const map<string, Function> &env;

    using IRMutator::visit;

    void visit(const Realize *op) {
        // Find the args for this function
        map<string, Function>::const_iterator iter = env.find(op->name);
        if (iter != env.end() &&
            iter->second.schedule().is_kernel_buffer()) {
            // Recurses
            Stmt new_body = mutate(op->body);
            // then modifies the slice node in the body
            new_body = UpdateSliceForFunc(op->name, op->bounds).mutate(new_body);

            stmt = Realize::make(op->name, op->types, op->bounds,
                                 op->condition, new_body);
        } else {
            IRMutator::visit(op);
        }
    }
public:
    UpdateKBufferSlices(const map<string, Function> &e) : env(e) {}
};

Stmt update_kbuffer_slices(Stmt s, const map<string, Function> &env) {
    s = UpdateKBufferSlices(env).mutate(s);
    return s;
}

}
}
