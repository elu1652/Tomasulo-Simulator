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

    bool isBranch = false;
    bool branchTaken = false;
    int branchTarget = -1;
};

struct ActiveInstruction {
    Instruction instr;
    int instructionIndex;
    int remainingCycles;
    bool executing;
    std::string waitingReason;

    int issueCycle;

    int qj; // tag for source operand 1
    int qk; // tag for source operand 2

    int vj; // value for source operand 1
    int vk; // value for source operand 2
};



class Simulator {
private:
    RegisterFile rf;
    Memory mem;
    std::vector<InstructionStatus> statusTable;

public:
    Simulator();

    void execute(const std::vector<Instruction>& instructions);

    ExecutionResult computeResult(const ActiveInstruction& active);
};