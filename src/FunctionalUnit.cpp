#include "FunctionalUnit.h"

// Determine which functional unit is used based on OpCode
FUType getFUType(OpCode opcode) {
    switch (opcode) {
        case OpCode::ADD:
        case OpCode::SUB:
        case OpCode::BEQ:
        case OpCode::BNE:
            return FUType::INT;

        case OpCode::MUL:
            return FUType::MUL;

        case OpCode::LD:
        case OpCode::SD:
            return FUType::MEM;

        default:
            return FUType::NONE;
    }
}

// Return proper Functional Unit
FunctionalUnit* getFU(
    FUType type,
    FunctionalUnit& intFU,
    FunctionalUnit& mulFU,
    FunctionalUnit& memFU
) {
    switch (type) {
        case FUType::INT: return &intFU;
        case FUType::MUL: return &mulFU;
        case FUType::MEM: return &memFU;
        default: return nullptr;
    }
}

// Check if functional unit is available
bool fuAvailable(FunctionalUnit* fu) {
    return fu != nullptr && fu->busyUnits < fu->totalUnits;
}

std::string fuTypeToString(FUType type) {
    switch (type) {
        case FUType::INT: return "INT";
        case FUType::MUL: return "MUL";
        case FUType::MEM: return "MEM";
        default: return "NONE";
    }
}