#include <iostream>
#include <limits>

#include "CodeGen_CatapultHLS_Base.h"
#include "CodeGen_Internal.h"
#include "Substitute.h"
#include "IROperator.h"
#include "Param.h"
#include "Var.h"
#include "Lerp.h"
#include "Simplify.h"

namespace Halide {
namespace Internal {

using std::ostream;
using std::endl;
using std::string;
using std::vector;
using std::ostringstream;
using std::to_string;

string CodeGen_CatapultHLS_Base::print_stencil_type(Stencil_Type stencil_type) {
    ostringstream oss;
    // C: Stencil<uint16_t, 1, 1, 1> stencil_var;
    // C: hls::stream<Stencil<uint16_t, 1, 1, 1> > stencil_stream_var;

    switch(stencil_type.type) {
    case Stencil_Type::StencilContainerType::Stencil :
        oss << "Stencil<" << print_type(stencil_type.elemType);

        for(const auto &range : stencil_type.bounds) {
            internal_assert(is_one(simplify(range.min == 0)));
            oss << ", " << range.extent;
        }
        oss << ">";
        break;
    case Stencil_Type::StencilContainerType::Stream :
        oss << "hls::stream<PackedStencil<";
        oss << print_type(stencil_type.elemType);

        for(const auto &range : stencil_type.bounds) {
            internal_assert(is_one(simplify(range.min == 0)));
            oss << ", " << range.extent;
        }
        oss << "> >";
        break;
    case Stencil_Type::StencilContainerType::AxiStream :
        oss << "hls::stream<AxiPackedStencil<";
        oss << print_type(stencil_type.elemType);

        for(const auto &range : stencil_type.bounds) {
            internal_assert(is_one(simplify(range.min == 0)));
            oss << ", " << range.extent;
        }
        oss << "> >";
        break;
    default: internal_error;
    }
    return oss.str();
}

string CodeGen_CatapultHLS_Base::print_name(const string &name) {
    ostringstream oss;

    // Prefix an underscore to avoid reserved words (e.g. a variable named "while")
    if (isalpha(name[0])) {
        oss << '_';
    }

    for (size_t i = 0; i < name.size(); i++) {
	// Catapult HLS compiler doesn't like '__'
        if (!isalnum(name[i])) {
            oss << "_";
        }
        else oss << name[i];
    }
    return oss.str();
}

string CodeGen_CatapultHLS_Base::print_stencil_pragma(const string &name) {
    // nothing is printed by default
    return string();
}

void CodeGen_CatapultHLS_Base::visit(const Call *op) {
    if (op->name == "linebuffer") {
        //IR: linebuffer(buffered.stencil_update.stream, buffered.stencil.stream, extent_0[, extent_1, ...])
        //C: linebuffer<extent_0[, extent_1, ...]>(buffered.stencil_update.stream, buffered.stencil.stream)
        internal_assert(op->args.size() >= 3);
        string a0 = print_expr(op->args[0]);
        string a1 = print_expr(op->args[1]);
        do_indent();
        stream << "linebuffer<";
        for(size_t i = 2; i < op->args.size(); i++) {
            stream << print_expr(op->args[i]);
            if (i != op->args.size() -1)
                stream << ", ";
        }
        stream << ">(" << a0 << ", " << a1 << ");\n";
        id = "0"; // skip evaluation
    } else if (op->name == "write_stream") {
        if (op->args.size() == 2) {
            // normal case
            // IR: write_stream(buffered.stencil_update.stream, buffered.stencil_update)
            // C: buffered_stencil_update_stream.write(buffered_stencil_update);
            string a0 = print_expr(op->args[0]);
            string a1 = print_expr(op->args[1]);
            do_indent();
            stream << a0 << ".write(" << a1 << ");\n";
            id = "0"; // skip evaluation
        } else {
            // write stream call for the dag output kernel
            // IR: write_stream(output.stencil.stream, output.stencil, loop_var_1, loop_max_1, ...)
            // C:  AxiPackedStencil<uint8_t, 1, 1, 1> _output_stencil_packed = _output_stencil;
            //     if (_loop_var_1 == loop_max_1 && ...)
            //       _output_stencil_packed.last = 1;
            //     else
            //       _output_stencil_packed.last = 0;
            //     _output_stencil_stream.write(_output_stencil_packed);
            internal_assert(op->args.size() > 2 && op->args.size() % 2 == 0);
            const Variable *stream_var = op->args[0].as<Variable>();
            const Variable *stencil_var = op->args[1].as<Variable>();
            internal_assert(stream_var && stencil_var);
            string stream_name = stream_var->name;
            string stencil_name = stencil_var->name;
            string packed_stencil_name = stencil_name + "_packed";

            internal_assert(stencils.contains(stencil_name));
            Stencil_Type stencil_type = stencils.get(stencil_name);
            internal_assert(stencil_type.type == Stencil_Type::StencilContainerType::Stencil);

            // emit code declaring the packed stencil
            do_indent();
            stream << "AxiPacked" << print_stencil_type(stencil_type) << " "
                   << print_name(packed_stencil_name) << " = "
                   << print_name(stencil_name) << ";\n";

            // emit code asserting TLAST
            vector<string> loop_vars, loop_maxes;
            for (size_t i = 2; i < op->args.size(); i += 2) {
                loop_vars.push_back(print_expr(op->args[i]));
                loop_maxes.push_back(print_expr(op->args[i+1]));
            }
            do_indent();
            stream << "if (";
            for (size_t i = 0; i < loop_vars.size(); i++) {
                stream << loop_vars[i] << " == " << loop_maxes[i];
                if (i < loop_vars.size() - 1)
                    stream << " && ";
            }
            stream << ") {\n";
            do_indent();
            stream << ' ' << print_name(packed_stencil_name) << ".last = 1;\n";
            do_indent();
            stream << "} else {\n";
            do_indent();
            stream << ' ' << print_name(packed_stencil_name) << ".last = 0;\n";
            do_indent();
            stream << "}\n";

            // emit code writing stream
            do_indent();
            stream << print_name(stream_name) << ".write("
                   << print_name(packed_stencil_name) << ");\n";
            id = "0"; // skip evaluation
        }
    } else if (op->name == "read_stream") {
        internal_assert(op->args.size() == 2 || op->args.size() == 3);
        string a1 = print_expr(op->args[1]);

        const Variable *stream_name_var = op->args[0].as<Variable>();
        internal_assert(stream_name_var);
        string stream_name = stream_name_var->name;
        if (op->args.size() == 3) {
            // stream name is maggled with the consumer name
            const StringImm *consumer_imm = op->args[2].as<StringImm>();
            internal_assert(consumer_imm);
            stream_name += ".to." + consumer_imm->value;
        }
        do_indent();
        stream << a1 << " = " << print_name(stream_name) << ".read();\n";
        id = "0"; // skip evaluation
    } else if (ends_with(op->name, ".stencil") ||
               ends_with(op->name, ".stencil_update") ||
               ends_with(op->name, ".acc")) {
        ostringstream rhs;
        // IR: out.stencil_update(0, 0, 0)
        // C: out_stencil_update(0, 0, 0)
        vector<string> args_indices(op->args.size());
        for(size_t i = 0; i < op->args.size(); i++)
            args_indices[i] = print_expr(op->args[i]);

        if (ends_with(op->name, ".acc")) {
            rhs << print_name(op->name);
        } else {
            rhs << print_name(op->name) << "(";
            for(size_t i = 0; i < op->args.size(); i++) {
                rhs << args_indices[i];
                if (i != op->args.size() - 1)
                    rhs << ", ";
            }
            rhs << ")";
        }
        print_assignment(op->type, rhs.str());
    } else if (op->name == "dispatch_stream") {
        // emits the calling arguments in comment
        vector<string> args(op->args.size());
        for(size_t i = 0; i < op->args.size(); i++)
            args[i] = print_expr(op->args[i]);

        do_indent();
        stream << "// dispatch_stream(";
        for(size_t i = 0; i < args.size(); i++) {
            stream << args[i];

            if (i != args.size() - 1)
                stream << ", ";
        }
        stream << ");\n";
        // syntax:
        //   dispatch_stream(stream_name, num_of_dimensions,
        //                   stencil_size_dim_0, stencil_step_dim_0, store_extent_dim_0,
        //                   [stencil_size_dim_1, stencil_step_dim_1, store_extent_dim_1, ...]
        //                   num_of_consumers,
        //                   consumer_0_name, fifo_0_depth,
        //                   consumer_0_offset_dim_0, consumer_0_extent_dim_0,
        //                   [consumer_0_offset_dim_1, consumer_0_extent_dim_1, ...]
        //                   [consumer_1_name, ...])

        // recover the structed data from op->args
        internal_assert(op->args.size() >= 2);
        const Variable *stream_name_var = op->args[0].as<Variable>();
        internal_assert(stream_name_var);
        string stream_name = stream_name_var->name;
        size_t num_of_demensions = *as_const_int(op->args[1]);
        vector<int> stencil_sizes(num_of_demensions);
        vector<int> stencil_steps(num_of_demensions);
        vector<int> store_extents(num_of_demensions);

        internal_assert(op->args.size() >= num_of_demensions*3 + 2);
        for (size_t i = 0; i < num_of_demensions; i++) {
            stencil_sizes[i] = *as_const_int(op->args[i*3 + 2]);
            stencil_steps[i] = *as_const_int(op->args[i*3 + 3]);
            store_extents[i] = *as_const_int(op->args[i*3 + 4]);
        }

        internal_assert(op->args.size() >= num_of_demensions*3 + 3);
        size_t num_of_consumers = *as_const_int(op->args[num_of_demensions*3 + 2]);
        vector<string> consumer_names(num_of_consumers);
        vector<int> consumer_fifo_depth(num_of_consumers);
        vector<vector<int> > consumer_offsets(num_of_consumers);
        vector<vector<int> > consumer_extents(num_of_consumers);

        internal_assert(op->args.size() >= num_of_demensions*3 + 3 + num_of_consumers*(2 + 2*num_of_demensions));
        for (size_t i = 0; i < num_of_consumers; i++) {
            const StringImm *string_imm = op->args[num_of_demensions*3 + 3 + (2 + 2*num_of_demensions)*i].as<StringImm>();
            internal_assert(string_imm);
            consumer_names[i] = string_imm->value;
            const IntImm *int_imm = op->args[num_of_demensions*3 + 4 + (2 + 2*num_of_demensions)*i].as<IntImm>();
            internal_assert(int_imm);
            consumer_fifo_depth[i] = int_imm->value;
            vector<int> offsets(num_of_demensions);
            vector<int > extents(num_of_demensions);
            for (size_t j = 0; j < num_of_demensions; j++) {
                offsets[j] = *as_const_int(op->args[num_of_demensions*3 + 5 + (2 + 2*num_of_demensions)*i + 2*j]);
                extents[j] = *as_const_int(op->args[num_of_demensions*3 + 6 + (2 + 2*num_of_demensions)*i + 2*j]);
            }
            consumer_offsets[i] = offsets;
            consumer_extents[i] = extents;
        }

        // emits declarations of streams for each consumer
        internal_assert(stencils.contains(stream_name));
        Stencil_Type stream_type = stencils.get(stream_name);

        // Optimization. if there is only one consumer and its fifo depth is zero
        // , use C++ reference for the consumer stream
        if (num_of_consumers == 1 && consumer_fifo_depth[0] == 0) {
            string consumer_stream_name = stream_name + ".to." + consumer_names[0];
            do_indent();
            stream << print_stencil_type(stream_type) << " &"
                   << print_name(consumer_stream_name) << " = "
                   << print_name(stream_name) << ";\n";
            id = "0"; // skip evaluation
            return;
        }

        for (size_t i = 0; i < num_of_consumers; i++) {
            string consumer_stream_name = stream_name + ".to." + consumer_names[i];
            Stencil_Type consumer_stream_type = stream_type;
            consumer_stream_type.depth = std::max(consumer_fifo_depth[i], 1); // HLS tool doesn't support zero-depth FIFO yet
            do_indent();
            stream << print_stencil_type(consumer_stream_type) << ' '
                   << print_name(consumer_stream_name) << ";\n";
            // pragma
            stencils.push(consumer_stream_name, consumer_stream_type);
            stream << print_stencil_pragma(consumer_stream_name);
            stencils.pop(consumer_stream_name);
        }

        // emits for a loop for each dimensions (larger dimension number, outer the loop)
        for (int i = num_of_demensions - 1; i >= 0; i--) {
            string dim_name = "_dim_" + to_string(i);
            do_indent();
            // HLS C: for(int dim = 0; dim <= store_extent - stencil.size; dim += stencil.step)
            stream << "for (int " << dim_name <<" = 0; "
                   << dim_name << " <= " << store_extents[i] - stencil_sizes[i] << "; "
                   << dim_name << " += " << stencil_steps[i] << ")\n";
        }
        open_scope();
        // pragma
        stream << "#pragma HLS PIPELINE\n";
        // read stencil from stream
        Stencil_Type stencil_type = stream_type;
        stencil_type.type = Stencil_Type::StencilContainerType::Stencil;
        string stencil_name = "tmp_stencil";
        do_indent();
        stream << "Packed" << print_stencil_type(stencil_type) << ' '
               << print_name(stencil_name) << " = "
               << print_name(stream_name) << ".read();\n";

        // dispatch the stencil to each consumer stream
        for (size_t i = 0; i < num_of_consumers; i++) {
            string consumer_stream_name = stream_name + ".to." + consumer_names[i];
            // emits the predicate for dispatching stencils
            // HLS C: if(dim_0 >= consumer_offset_0 && dim_0 <= consumer_offset_0 + consumer_extent_0 - stencil_size_0
            //           [&& dim_1 >= consumer_offset_1 && dim_1 <= consumer_offset_1 + consumer_extent_1 - stencil_size_1...])
            do_indent();
            stream << "if (";
            for (size_t j = 0; j < num_of_demensions; j++) {
                string dim_name = "_dim_" + to_string(j);
                stream << dim_name << " >= " << consumer_offsets[i][j] << " && "
                       << dim_name << " <= " << consumer_offsets[i][j] + consumer_extents[i][j] - stencil_sizes[j];
                if (j != num_of_demensions - 1)
                    stream << " && ";
            }
            stream << ")\n";

            // emits the write call in the if body
            open_scope();
            do_indent();
            stream << print_name(consumer_stream_name) << ".write("
                   << print_name(stencil_name) << ");\n";
            close_scope("");
        }

        close_scope("");

        id = "0"; // skip evaluation
    } else {
        CodeGen_C::visit(op);
    }
}

void CodeGen_CatapultHLS_Base::visit(const Realize *op) {
    if (ends_with(op->name, ".stream")) {
        // create a stream type
        internal_assert(op->types.size() == 1);
        allocations.push(op->name, {op->types[0]});
        Stencil_Type stream_type({Stencil_Type::StencilContainerType::Stream,
                    op->types[0], op->bounds, 1});
        stencils.push(op->name, stream_type);

        // emits the declaration for the stream
        do_indent();
        stream << print_stencil_type(stream_type) << ' ' << print_name(op->name) << ";\n";
        stream << print_stencil_pragma(op->name);

        // traverse down
        op->body.accept(this);

        // We didn't generate free stmt inside for stream type
        allocations.pop(op->name);
        stencils.pop(op->name);

    } else if (ends_with(op->name, ".stencil") ||
               ends_with(op->name, ".stencil_update")) {
        // create a stencil type
        internal_assert(op->types.size() == 1);
        allocations.push(op->name, {op->types[0]});
        Stencil_Type stype({Stencil_Type::StencilContainerType::Stencil, op->types[0], op->bounds, 1});
        stencils.push(op->name, stype);

        do_indent();
        // Stencil<uint16_t, 1, 1, 1> conv1_stencil_update;
        stream << print_stencil_type(stype) << ' ' << print_name(op->name) << ";\n";
        stream << print_stencil_pragma(op->name);

        op->body.accept(this);

        // We didn't generate free stmt inside for stream type
        allocations.pop(op->name);
        stencils.pop(op->name);
    } else if (ends_with(op->name, ".acc")) {
        allocations.push(op->name, {op->types[0]});

        do_indent();
        stream << print_type(op->types[0]) << ' ' << print_name(op->name) << ";\n";

        op->body.accept(this);

        allocations.pop(op->name);
    } else {
        CodeGen_C::visit(op);
    }
}

void CodeGen_CatapultHLS_Base::visit(const Provide *op) {
    if (ends_with(op->name, ".stencil") ||
        ends_with(op->name, ".stencil_update") ||
        ends_with(op->name, ".acc")) {
        // IR: buffered.stencil_update(1, 2, 3) =
        // C: buffered_stencil_update(1, 2, 3) =
        vector<string> args_indices(op->args.size());
        for(size_t i = 0; i < op->args.size(); i++)
            args_indices[i] = print_expr(op->args[i]);

        internal_assert(op->values.size() == 1);
        string id_value = print_expr(op->values[0]);

        do_indent();

        if(ends_with(op->name, ".acc")) {
            stream << print_name(op->name) << " = " << id_value << ";\n";
        } else {
            stream << print_name(op->name) << "(";

            for(size_t i = 0; i < op->args.size(); i++) {
                stream << args_indices[i];
                if (i != op->args.size() - 1)
                    stream << ", ";
            }
            stream << ") = " << id_value << ";\n";
        }
        cache.clear();
    } else {
        CodeGen_C::visit(op);
    }
}

}
}
