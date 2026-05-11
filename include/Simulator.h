#pragma once

#include <vector>

#include "Instruction.h"
#include "RegisterFile.h"
#include "Memory.h"
#include "InstructionStatus.h"
#include "BranchPredictor.h"


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

    bool isBranch = false;
    bool predictedTaken = false;
    int predictedTarget = -1;

    int predictorStateBefore = -1;
    int predictorStateAfter = -1;
};



class Simulator {
private:
    RegisterFile rf;
    Memory mem;
    std::vector<InstructionStatus> statusTable;
    BranchPredictorType predictorType;

public:
    explicit Simulator(BranchPredictorType predictorType = BranchPredictorType::TwoBit);

    void execute(const std::vector<Instruction>& instructions);

    ExecutionResult computeResult(const ActiveInstruction& active);
};