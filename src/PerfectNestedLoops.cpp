#include "ReplaceImageParam.h"

#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Debug.h"

namespace Halide {
namespace Internal {

using std::string;
using std::map;
using std::vector;

// Flatten producer consumer nodes into sequential blocks
// of statements
class FlattenProducerConsumerNodes : public IRMutator {
    using IRMutator::visit;

    void visit(const ProducerConsumer *op) {
        Stmt produce = mutate(op->produce);
        Stmt blocks = produce;
        if (op->update.defined()) {
            Stmt update = mutate(op->update);
            blocks = Block::make(blocks, update);
        }
        Stmt consume = mutate(op->consume);
        blocks = Block::make(blocks, consume);
        stmt = blocks;
    }
};

class CollectAllocateRealize : public IRMutator {
    using IRMutator::visit;

    void visit(const Realize *op) {
        // TODO check the args doesn't depend on inner loops
        realizes.push_back(op);
        stmt = mutate(op->body);
    }

    void visit(const Allocate *op) {
        // TODO check the args doesn't depend on inner loops
        allocates.push_back(op);
        stmt = mutate(op->body);
    }

    void visit(const Free *op) {
        frees.push_back(op->name);
        stmt = Evaluate::make(make_zero(Int(32)));
    }

public:
    vector<const Realize *> realizes;
    vector<const Allocate *> allocates;
    vector<string> frees;
};

Stmt pull_up_allocate_realize(Stmt s) {
    CollectAllocateRealize collector;
    Stmt stmt = collector.mutate(s);
    for (const string &name : collector.frees) {
        stmt = Block::make(stmt, Free::make(name));
    }
    for (const auto &op : collector.allocates) {
        stmt = Allocate::make(op->name, op->type, op->extents, op->condition, stmt, op->new_expr, op->free_function);
    }
    for (const auto &op : collector.realizes) {
        stmt = Realize::make(op->name, op->types, op->bounds, op->condition, stmt);
    }
    return stmt;
}

class PerfectNestedLoopsForKernel : public IRMutator {
    using IRMutator::visit;

};

Stmt perfect_nested_loops_for_kernel(Stmt s) {
    s = FlattenProducerConsumerNodes().mutate(s);
    //s = pull_up_allocate_realize(s);
    //PerfectNestedLoopsForKernel().mutate(op->produce);
    return s;
}

class PerfectNestedLoopsForPipeline : public IRMutator {
    using IRMutator::visit;

    void visit(const ProducerConsumer *op) {
        if (ends_with(op->name, ".stream")) {
            debug(3) << "find a HW kernel " << op->name <<"\n";
            Stmt produce = perfect_nested_loops_for_kernel(op->produce);
            internal_assert(!op->update.defined());
            Stmt consume = mutate(op->consume);
            if (produce.same_as(op->produce) &&
                consume.same_as(op->consume)) {
                stmt = op;
            } else {
                stmt = ProducerConsumer::make(op->name, produce, Stmt(), consume);
            }
        } else {
            IRMutator::visit(op);
        }
    }
};

class PerfectNestedLoops : public IRMutator {
    using IRMutator::visit;

    void visit(const ProducerConsumer *op) {
        if (starts_with(op->name, "_hls_target.")) {
            debug(3) << "find a HW pipeline " << op->name <<"\n";
            Stmt produce = PerfectNestedLoopsForPipeline().mutate(op->produce);
            internal_assert(!op->update.defined());
            Stmt consume = mutate(op->consume);
            if (produce.same_as(op->produce) &&
                consume.same_as(op->consume)) {
                stmt = op;
            } else {
                stmt = ProducerConsumer::make(op->name, produce, Stmt(), consume);
            }
        } else {
            IRMutator::visit(op);
        }
    }
};

Stmt perfect_nested_loops(Stmt s) {
    s = PerfectNestedLoops().mutate(s);
    return s;
}

}
}
