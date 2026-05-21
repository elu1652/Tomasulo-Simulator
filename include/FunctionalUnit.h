#pragma once

#include "Instruction.h"
#include <string>

enum class FUType {
    INT,
    MUL,
    FP_ADD,
    FP_MUL,
    MEM,
    NONE
};

struct FunctionalUnit {
    FUType type;
    int totalUnits;
    int busyUnits;
};

FUType getFUType(OpCode opcode);

FunctionalUnit* getFU(
    FUType type,
    FunctionalUnit& intFU,
    FunctionalUnit& mulFU,
    FunctionalUnit& fpAddFU,
    FunctionalUnit& fpMulFU,
    FunctionalUnit& memFU
);

bool fuAvailable(FunctionalUnit* fu);

std::string fuTypeToString(FUType type);