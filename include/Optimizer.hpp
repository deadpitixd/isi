// Easily includes most things
#include <Compiler.hpp>

std::optional<Value> foldValues(OpCode op, const Value& a, const Value& b) {
    bool isFloat = std::holds_alternative<double>(a) || std::holds_alternative<double>(b);

    if (isFloat) {
        double valA = valueToFloat(a);
        double valB = valueToFloat(b);
        switch (op) {
            case OP_ADD: return valA + valB;
            case OP_SUB: return valA - valB;
            case OP_MUL: return valA * valB;
            case OP_DIV: 
                if (valB == 0.0) return std::nullopt;
                return valA / valB;
            default: return std::nullopt;
        }
    } else {
        int valA = valueToInt(a);
        int valB = valueToInt(b);
        switch (op) {
            case OP_ADD: return valA + valB;
            case OP_SUB: return valA - valB;
            case OP_MUL: return valA * valB;
            case OP_DIV: 
                if (valB == 0) return std::nullopt;
                return valA / valB;
            default: return std::nullopt;
        }
    }
}

std::vector<Instruction> Optimize(const std::vector<Instruction>& compiled) {
    std::vector<Instruction> out{};

    for (size_t i = 0; i < compiled.size(); i++) {
        OpCode op = compiled[i].op;

        // Check if the current instruction is a math operator
        if ((op == OP_ADD || op == OP_SUB || op == OP_MUL || op == OP_DIV) && out.size() >= 2) {
            const Instruction& first = out[out.size() - 2];
            const Instruction& second = out[out.size() - 1];

            if (first.op == OP_PUSH && second.op == OP_PUSH) {
                auto foldedResult = foldValues(op, first.value, second.value);
                
                if (foldedResult.has_value()) {
                    out.pop_back();
                    out.pop_back();

                    out.push_back({OP_PUSH, foldedResult.value()});
                    continue;
                }
            }
        }

        out.push_back(compiled[i]);
    }
    return out;
}