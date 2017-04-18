#include "PerfectNestedLoops.h"

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
        stmt = mutate(op->body);
    }
};

class CountLoopNests : public IRVisitor {
    using IRVisitor::visit;

    void visit(const For *op) {
        // only count one loop level, no recursion
        ++count;
    }

public:
    int count = 0;
};


class PullOutPrologueEpilogue : public IRMutator {
    bool at_prologue = true;

    void pull_out_stmt(Stmt s) {
        if (at_prologue) {
            if (!prologue.defined()) {
                prologue = s;
            } else {
                prologue = Block::make(prologue, s);
            }
        } else {
            if (!epilogue.defined()) {
                epilogue = s;
            } else {
                epilogue = Block::make(epilogue, s);
            }
        }
    }

    using IRMutator::visit;
    void visit(const For *op) {
        // stop resursion at the inner loop
        stmt = op;
        at_prologue = false;
    }


    void visit(const Block *op) {
        Stmt first = mutate(op->first);
        Stmt rest = mutate(op->rest);
        if (!first.defined()) {
            stmt = rest;
        } else if (!rest.defined()) {
            stmt = first;
        } else if (first.same_as(op->first) &&
                   rest.same_as(op->rest)) {
            stmt = op;
        } else {
            stmt = Block::make(first, rest);
        }
    }

    // Pull out Evaluate, Store, Provide, IfThenElse
    void visit(const Evaluate *op) {
        pull_out_stmt(op);
        stmt = Stmt();
    }

    void visit(const Store *op) {
        pull_out_stmt(op);
        stmt = Stmt();
    }

    void visit(const Provide *op) {
        pull_out_stmt(op);
        stmt = Stmt();
    }

    void visit(const IfThenElse *op) {
        pull_out_stmt(op);
        stmt = Stmt();
    }

    void visit(const LetStmt *op) {
        internal_error << "Cannot handle LetStmt " << Stmt(op) << "\n\n";
    }

public:
    Stmt prologue, epilogue;
};

class PushPrologueEpilogueIntoInnerLoop : public IRMutator {
    Stmt prologue, epilogue;
    using IRMutator::visit;

    void visit(const For *op) {
        Expr var = Variable::make(Int(32), op->name);
        Stmt body = op->body;
        if (prologue.defined()) {
            Expr prologue_cond = var == op->min;
            Stmt prologue_if = IfThenElse::make(prologue_cond, prologue, Stmt());
            body = Block::make(prologue_if, body);
        }
        if (epilogue.defined()) {
            Expr epilogue_cond = var == op->min + op->extent - 1;
            Stmt epilogue_if = IfThenElse::make(epilogue_cond, epilogue, Stmt());
            body = Block::make(body, epilogue_if);
        }
        if (body.same_as(op->body)) {
            stmt = op;
        } else {
            stmt = For::make(op->name, op->min, op->extent, op->for_type, op->device_api, body);
        }
    }

public:
    PushPrologueEpilogueIntoInnerLoop(Stmt p, Stmt e)
        : prologue(p), epilogue(e) {}
};

class PerfectNestedLoopsSingleKernel : public IRMutator {
    using IRMutator::visit;

    // traverse each loop level of a hw kernel
    void visit(const For *op) {
        CountLoopNests counter;
        op->body.accept(&counter);
        debug(3) << "for " << op->name << " has " << counter.count
                 << " inner loop nests.\n";

        if (counter.count == 1) {
            debug(3) << "Perfecting loop body: \n" << op->body << "\n\n";

            PullOutPrologueEpilogue p;
            Stmt body = p.mutate(op->body);

            debug(3) << "Pulled out prologue and epilogue:\n" << body << "\n\n";

            debug(3) << "prologue:\n" << p.prologue << "\n\n";
            debug(3) << "epilogue:\n" << p.epilogue << "\n\n";

            body = PushPrologueEpilogueIntoInnerLoop(p.prologue,
                                                     p.epilogue).mutate(body);

            debug(3) << "Push prologue and epilogue into inner loop:\n" << body << "\n\n";

            // recurse
            body = mutate(body);

            stmt = For::make(op->name, op->min, op->extent, op->for_type, op->device_api, body);
        } else {
            if (counter.count > 1) {
                user_warning << "A sequence of " << counter.count
                             << " loop nests found in the body of loop " << op->name
                             << ". Cannot perfect this loop body.\n";
            }
            IRMutator::visit(op);
        }
    }
};


class PerfectNestedLoopsForPipeline : public IRMutator {
    using IRMutator::visit;

    // traverse each hw kernel
    void visit(const ProducerConsumer *op) {
        if (op->is_producer && ends_with(op->name, ".stream")) {
            debug(3) << "find a HW kernel " << op->name <<"\n";
            Stmt body = FlattenProducerConsumerNodes().mutate(op->body);
            body = PerfectNestedLoopsSingleKernel().mutate(body);
            if (body.same_as(op->body)) {
                stmt = op;
            } else {
                stmt = ProducerConsumer::make(op->name, op->is_producer, body);
            }
        } else {
            IRMutator::visit(op);
        }
    }
};

class PerfectNestedLoops : public IRMutator {
    using IRMutator::visit;

    void visit(const ProducerConsumer *op) {
        if (op->is_producer && starts_with(op->name, "_hls_target.")) {
            debug(3) << "find a HW pipeline " << op->name <<"\n";
            Stmt body = PerfectNestedLoopsForPipeline().mutate(op->body);
            if (body.same_as(op->body)) {
                stmt = op;
            } else {
                stmt = ProducerConsumer::make(op->name, op->is_producer, body);
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
