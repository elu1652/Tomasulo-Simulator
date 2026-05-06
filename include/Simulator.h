#pragma once

#include <vector>

#include "Instruction.h"
#include "RegisterFile.h"
#include "Memory.h"
#include "InstructionStatus.h"

class Simulator {
private:
    RegisterFile rf;
    Memory mem;
    std::vector<InstructionStatus> statusTable;

public:
    Simulator();

    void execute(const std::vector<Instruction>& instructions);

    void executeInstruction(const Instruction& instr);
};