#pragma once

#include "Instruction.h"
#include "Simulator.h"
#include <string>
#include <vector>

enum class RSType {
    INT,
    MUL,
    FP_ADD,
    FP_MUL,
    LOAD,
    STORE,
    NONE
};

RSType getRSType(OpCode opcode);

std::string rsTypeToString(RSType type);

int countRSEntries(
    const std::vector<ActiveInstruction>& activeInstructions,
    RSType type
);

int getRSCapacity(
    RSType type,
    int intCapacity,
    int mulCapacity,
    int fpAddCapacity,
    int fpMulCapacity,
    int loadCapacity,
    int storeCapacity
);