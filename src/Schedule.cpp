#include "Func.h"
#include "Function.h"
#include "IR.h"
#include "IRMutator.h"
#include "Schedule.h"
#include "Var.h"

namespace Halide {

LoopLevel::LoopLevel(Internal::IntrusivePtr<Internal::FunctionContents> f, 
                     const std::string &var_name, 
                     bool is_rvar) 
    : function_contents(f), var_name(var_name), is_rvar(is_rvar) {}
LoopLevel::LoopLevel(Internal::Function f, VarOrRVar v) : LoopLevel(f.get_contents(), v.name(), v.is_rvar) {}
LoopLevel::LoopLevel(Func f, VarOrRVar v) : LoopLevel(f.function().get_contents(), v.name(), v.is_rvar) {}

std::string LoopLevel::func_name() const {
    if (function_contents.defined()) {
        return Internal::Function(function_contents).name();
    }
    return "";
}

Func LoopLevel::func() const {
    internal_assert(!is_inline() && !is_root());
    internal_assert(function_contents.defined());
    return Func(Internal::Function(function_contents));
}

VarOrRVar LoopLevel::var() const {
    internal_assert(!is_inline() && !is_root());
    return VarOrRVar(var_name, is_rvar);
}

bool LoopLevel::is_inline() const {
    return var_name.empty();
}

/*static*/
LoopLevel LoopLevel::root() {
    return LoopLevel(nullptr, "__root", false);
}

bool LoopLevel::is_root() const {
    return var_name == "__root";
}

std::string LoopLevel::to_string() const {
    return (function_contents.defined() ? Internal::Function(function_contents).name() : "") + "." + var_name;
}

bool LoopLevel::match(const std::string &loop) const {
    return Internal::starts_with(loop, func_name() + ".") && 
           Internal::ends_with(loop, "." + var_name);
}

bool LoopLevel::match(const LoopLevel &other) const {
    // Must compare by name, not by pointer, since in() can make copies
    // that we need to consider equivalent
    return (func_name() == other.func_name() &&
            (var_name == other.var_name ||
             Internal::ends_with(var_name, "." + other.var_name) ||
             Internal::ends_with(other.var_name, "." + var_name)));
}

bool LoopLevel::operator==(const LoopLevel &other) const {
    // Must compare by name, not by pointer, since in() can make copies
    // that we need to consider equivalent
    return func_name() == other.func_name() && var_name == other.var_name;
}

namespace Internal {

typedef std::map<IntrusivePtr<FunctionContents>, IntrusivePtr<FunctionContents>> DeepCopyMap;

IntrusivePtr<FunctionContents> deep_copy_function_contents_helper(
    const IntrusivePtr<FunctionContents> &src,
    DeepCopyMap &copied_map);

/** A schedule for a halide function, which defines where, when, and
 * how it should be evaluated. */
struct ScheduleContents {
    mutable RefCount ref_count;

    LoopLevel store_level, compute_level;
    std::vector<ReductionVariable> rvars;
    std::vector<Split> splits;
    std::vector<Dim> dims;
    std::vector<StorageDim> storage_dims;
    std::vector<Bound> bounds;
    std::vector<Prefetch> prefetches;
    std::map<std::string, IntrusivePtr<Internal::FunctionContents>> wrappers;
    bool memoized;
    bool touched;
    bool allow_race_conditions;
    bool is_hw_kernel;   // TODO equivalent to !accelerate_exit.empty()
    bool is_accelerated;  // TODO equivalent to !accelerate_input.empty()
    bool is_linebuffered;
    std::set<std::string> accelerate_inputs;
    std::string accelerate_exit;
    LoopLevel accelerate_compute_level, accelerate_store_level;
    std::map<std::string, int> fifo_depths;   // key is the name of the consumer
    bool is_kernel_buffer;
    bool is_kernel_buffer_slice;
    std::map<std::string, Function> tap_funcs;
    std::map<std::string, Parameter> tap_params;

    ScheduleContents()
        : memoized(false), touched(false), allow_race_conditions(false),
          is_hw_kernel(false), is_accelerated(false), is_linebuffered(false),
          is_kernel_buffer(false), is_kernel_buffer_slice(false) {};

    // Pass an IRMutator through to all Exprs referenced in the ScheduleContents
    void mutate(IRMutator *mutator) {
        for (ReductionVariable &r : rvars) {
            if (r.min.defined()) {
                r.min = mutator->mutate(r.min);
            }
            if (r.extent.defined()) {
                r.extent = mutator->mutate(r.extent);
            }
        }
        for (Split &s : splits) {
            if (s.factor.defined()) {
                s.factor = mutator->mutate(s.factor);
            }
        }
        for (Bound &b : bounds) {
            if (b.min.defined()) {
                b.min = mutator->mutate(b.min);
            }
            if (b.extent.defined()) {
                b.extent = mutator->mutate(b.extent);
            }
            if (b.modulus.defined()) {
                b.modulus = mutator->mutate(b.modulus);
            }
            if (b.remainder.defined()) {
                b.remainder = mutator->mutate(b.remainder);
            }
        }
        for (Prefetch &p : prefetches) {
            if (p.offset.defined()) {
                p.offset = mutator->mutate(p.offset);
            }
        }
    }
};


template<>
EXPORT RefCount &ref_count<ScheduleContents>(const ScheduleContents *p) {
    return p->ref_count;
}

template<>
EXPORT void destroy<ScheduleContents>(const ScheduleContents *p) {
    delete p;
}

Schedule::Schedule() : contents(new ScheduleContents) {}

Schedule Schedule::deep_copy(
        std::map<IntrusivePtr<FunctionContents>, IntrusivePtr<FunctionContents>> &copied_map) const {

    internal_assert(contents.defined()) << "Cannot deep-copy undefined Schedule\n";
    Schedule copy;
    copy.contents->store_level = contents->store_level;
    copy.contents->compute_level = contents->compute_level;
    copy.contents->rvars = contents->rvars;
    copy.contents->splits = contents->splits;
    copy.contents->dims = contents->dims;
    copy.contents->storage_dims = contents->storage_dims;
    copy.contents->bounds = contents->bounds;
    copy.contents->prefetches = contents->prefetches;
    copy.contents->memoized = contents->memoized;
    copy.contents->touched = contents->touched;
    copy.contents->allow_race_conditions = contents->allow_race_conditions;

    // HLS related fields
    copy.contents->is_hw_kernel = contents->is_hw_kernel;
    copy.contents->is_accelerated = contents->is_accelerated;
    copy.contents->is_linebuffered = contents->is_linebuffered;
    copy.contents->accelerate_inputs = contents->accelerate_inputs;
    copy.contents->accelerate_exit = contents->accelerate_exit;
    copy.contents->accelerate_compute_level = contents->accelerate_compute_level;
    copy.contents->accelerate_store_level = contents->accelerate_store_level;
    copy.contents->fifo_depths = contents->fifo_depths;
    copy.contents->is_kernel_buffer = contents->is_kernel_buffer;
    copy.contents->is_kernel_buffer_slice = contents->is_kernel_buffer_slice;
    copy.contents->tap_funcs = contents->tap_funcs;
    copy.contents->tap_params = contents->tap_params;

    // Deep-copy wrapper functions. If function has already been deep-copied before,
    // i.e. it's in the 'copied_map', use the deep-copied version from the map instead
    // of creating a new deep-copy
    for (const auto &iter : contents->wrappers) {
        IntrusivePtr<FunctionContents> &copied_func = copied_map[iter.second];
        if (copied_func.defined()) {
            copy.contents->wrappers[iter.first] = copied_func;
        } else {
            copy.contents->wrappers[iter.first] = deep_copy_function_contents_helper(iter.second, copied_map);
            copied_map[iter.second] = copy.contents->wrappers[iter.first];
        }
    }
    internal_assert(copy.contents->wrappers.size() == contents->wrappers.size());
    return copy;
}

bool &Schedule::memoized() {
    return contents->memoized;
}

bool Schedule::memoized() const {
    return contents->memoized;
}

bool &Schedule::touched() {
    return contents->touched;
}

bool Schedule::touched() const {
    return contents->touched;
}

const std::vector<Split> &Schedule::splits() const {
    return contents->splits;
}

std::vector<Split> &Schedule::splits() {
    return contents->splits;
}

std::vector<Dim> &Schedule::dims() {
    return contents->dims;
}

const std::vector<Dim> &Schedule::dims() const {
    return contents->dims;
}

std::vector<StorageDim> &Schedule::storage_dims() {
    return contents->storage_dims;
}

const std::vector<StorageDim> &Schedule::storage_dims() const {
    return contents->storage_dims;
}

std::vector<Bound> &Schedule::bounds() {
    return contents->bounds;
}

const std::vector<Bound> &Schedule::bounds() const {
    return contents->bounds;
}

std::vector<Prefetch> &Schedule::prefetches() {
    return contents->prefetches;
}

const std::vector<Prefetch> &Schedule::prefetches() const {
    return contents->prefetches;
}

std::vector<ReductionVariable> &Schedule::rvars() {
    return contents->rvars;
}

const std::vector<ReductionVariable> &Schedule::rvars() const {
    return contents->rvars;
}

std::map<std::string, IntrusivePtr<Internal::FunctionContents>> &Schedule::wrappers() {
    return contents->wrappers;
}

const std::map<std::string, IntrusivePtr<Internal::FunctionContents>> &Schedule::wrappers() const {
    return contents->wrappers;
}

void Schedule::add_wrapper(const std::string &f,
                           const IntrusivePtr<Internal::FunctionContents> &wrapper) {
    if (contents->wrappers.count(f)) {
        if (f.empty()) {
            user_warning << "Replacing previous definition of global wrapper in function \""
                         << f << "\"\n";
        } else {
            internal_error << "Wrapper redefinition in function \"" << f << "\" is not allowed\n";
        }
    }
    contents->wrappers[f] = wrapper;
}

LoopLevel &Schedule::store_level() {
    return contents->store_level;
}

LoopLevel &Schedule::compute_level() {
    return contents->compute_level;
}

const LoopLevel &Schedule::store_level() const {
    return contents->store_level;
}

const LoopLevel &Schedule::compute_level() const {
    return contents->compute_level;
}

bool &Schedule::is_hw_kernel() {
    return contents->is_hw_kernel;
}

bool Schedule::is_hw_kernel() const {
    return contents->is_hw_kernel;
}

bool Schedule::is_accelerated() const {
    return contents->is_accelerated;
}

bool &Schedule::is_accelerated() {
    return contents->is_accelerated;
}

bool Schedule::is_linebuffered() const {
    return contents->is_linebuffered;
}

bool &Schedule::is_linebuffered() {
    return contents->is_linebuffered;
}

bool Schedule::is_kernel_buffer() const {
    return contents->is_kernel_buffer;
}

bool &Schedule::is_kernel_buffer() {
    return contents->is_kernel_buffer;
}

bool Schedule::is_kernel_buffer_slice() const {
    return contents->is_kernel_buffer_slice;
}

bool &Schedule::is_kernel_buffer_slice() {
    return contents->is_kernel_buffer_slice;
}

const std::set<std::string> &Schedule::accelerate_inputs() const{
    return contents->accelerate_inputs;
}

std::set<std::string> &Schedule::accelerate_inputs() {
    return contents->accelerate_inputs;
}

const std::map<std::string, Function> &Schedule::tap_funcs() const {
    return contents->tap_funcs;
}

std::map<std::string, Function> &Schedule::tap_funcs() {
    return contents->tap_funcs;
}

const std::map<std::string, Parameter> &Schedule::tap_params() const {
    return contents->tap_params;
}

std::map<std::string, Parameter> &Schedule::tap_params() {
    return contents->tap_params;
}

const std::map<std::string, int> &Schedule::fifo_depths() const {
    return contents->fifo_depths;
}

std::map<std::string, int> &Schedule::fifo_depths() {
    return contents->fifo_depths;
}

const std::string &Schedule::accelerate_exit() const{
    return contents->accelerate_exit;
}

std::string &Schedule::accelerate_exit() {
    return contents->accelerate_exit;
}

LoopLevel &Schedule::accelerate_compute_level() {
    internal_assert(is_accelerated());
    return contents->accelerate_compute_level;
}

const LoopLevel &Schedule::accelerate_compute_level() const {
    internal_assert(is_accelerated());
    return contents->accelerate_compute_level;
}

LoopLevel &Schedule::accelerate_store_level() {
    internal_assert(is_accelerated());
    return contents->accelerate_store_level;
}

const LoopLevel &Schedule::accelerate_store_level() const {
    internal_assert(is_accelerated());
    return contents->accelerate_store_level;
}

bool &Schedule::allow_race_conditions() {
    return contents->allow_race_conditions;
}

bool Schedule::allow_race_conditions() const {
    return contents->allow_race_conditions;
}

void Schedule::accept(IRVisitor *visitor) const {
    for (const ReductionVariable &r : rvars()) {
        if (r.min.defined()) {
            r.min.accept(visitor);
        }
        if (r.extent.defined()) {
            r.extent.accept(visitor);
        }
    }
    for (const Split &s : splits()) {
        if (s.factor.defined()) {
            s.factor.accept(visitor);
        }
    }
    for (const Bound &b : bounds()) {
        if (b.min.defined()) {
            b.min.accept(visitor);
        }
        if (b.extent.defined()) {
            b.extent.accept(visitor);
        }
        if (b.modulus.defined()) {
            b.modulus.accept(visitor);
        }
        if (b.remainder.defined()) {
            b.remainder.accept(visitor);
        }
    }
    for (const Prefetch &p : prefetches()) {
        if (p.offset.defined()) {
            p.offset.accept(visitor);
        }
    }
}

void Schedule::mutate(IRMutator *mutator) {
    if (contents.defined()) {
        contents->mutate(mutator);
    }
}

}  // namespace Internal
}  // namespace Halide

