#include "FunctionalUnit.h"

// Map each opcode class onto the structural execution resource it uses.
FUType getFUType(OpCode opcode) {
    switch (opcode) {
        case OpCode::ADD:
        case OpCode::ADDI:
        case OpCode::SUB:
        case OpCode::BEQ:
        case OpCode::BNE:
            return FUType::INT;

        case OpCode::MUL:
            return FUType::MUL;

        case OpCode::FADD:
        case OpCode::FSUB:
            return FUType::FP_ADD;

        case OpCode::FMUL:
        case OpCode::FDIV:
            return FUType::FP_MUL;

        case OpCode::LD:
        case OpCode::SD:
            return FUType::MEM;

        default:
            return FUType::NONE;
    }
}

// Return the concrete functional unit instance for a resource type.
FunctionalUnit* getFU(
    FUType type,
    FunctionalUnit& intFU,
    FunctionalUnit& mulFU,
    FunctionalUnit& fpAddFU,
    FunctionalUnit& fpMulFU,
    FunctionalUnit& memFU
) {
    switch (type) {
        case FUType::INT: return &intFU;
        case FUType::MUL: return &mulFU;
        case FUType::FP_ADD: return &fpAddFU;
        case FUType::FP_MUL: return &fpMulFU;
        case FUType::MEM: return &memFU;
        default: return nullptr;
    }
}

bool fuAvailable(FunctionalUnit* fu) {
    return fu != nullptr && fu->busyUnits < fu->totalUnits;
}

std::string fuTypeToString(FUType type) {
    switch (type) {
        case FUType::INT: return "INT";
        case FUType::MUL: return "MUL";
        case FUType::FP_ADD: return "FP_ADD";
        case FUType::FP_MUL: return "FP_MUL";
        case FUType::MEM: return "MEM";
        default: return "NONE";
    }
}
