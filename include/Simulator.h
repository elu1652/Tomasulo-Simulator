#pragma once

#include <vector>

#include "ArchitectureConfig.h"
#include "Instruction.h"
#include "RegisterFile.h"
#include "Memory.h"
#include "InstructionStatus.h"
#include "BranchPredictor.h"
#include "Program.h"
#include "DataCache.h"
#include "PerformanceStats.h"


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

    int gshareGlobalHistoryBefore = -1;
    int gshareGlobalHistoryAfter = -1;
    int gshareIndex = -1;
    int gshareCounterBefore = -1;
    int gshareCounterAfter = -1;

    int robTag;

    bool hasForwardedLoadValue = false;
    int forwardedLoadValue = 0;
    bool memoryAddressComputed = false;
    bool hasCacheAccessResult = false;
    CacheAccessResult cacheAccessResult;
};



class Simulator {
private:
    RegisterFile rf;
    Memory mem;
    std::vector<InstructionStatus> statusTable;
    BranchPredictorType predictorType;
    ArchitectureConfig architectureConfig;
    DataCache dataCache;

    CacheAccessResult startLoadAccess(
        int address,
        PerformanceStats& stats,
        std::vector<std::string>& traceEvents
    );
    CacheAccessResult startStoreAccess(
        int address,
        PerformanceStats& stats,
        std::vector<std::string>& traceEvents
    );

public:
    explicit Simulator(
        BranchPredictorType predictorType = BranchPredictorType::TwoBit,
        const ArchitectureConfig& architectureConfig = ArchitectureConfig{}
    );

    void execute(
        const std::vector<Instruction>& instructions,
        const ProgramSetup& setup = ProgramSetup{}
    );

    ExecutionResult computeResult(const ActiveInstruction& active);
    
};
