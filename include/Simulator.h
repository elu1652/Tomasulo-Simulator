#pragma once

#include <vector>

#include "Instruction.h"
#include "RegisterFile.h"
#include "Memory.h"
#include "InstructionStatus.h"


struct ExecutionResult {
    bool writesRegister;
    int destinationRegister;
    int value;

    bool writesMemory;
    int memoryAddress;
    int memoryValue;
};


class Simulator {
private:
    RegisterFile rf;
    Memory mem;
    std::vector<InstructionStatus> statusTable;

public:
    Simulator();

    void execute(const std::vector<Instruction>& instructions);

    ExecutionResult executeInstruction(const Instruction& instr);
};