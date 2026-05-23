#pragma once

#include "Instruction.h"

struct PerformanceStats {
    int totalCycles = 0;
    int committedInstructions = 0;

    int cyclesWithAnyStall = 0;
    int issueStallCycles = 0;
    int backendStallCycles = 0;
    int robFullStallCycles = 0;
    int rsFullStallCycles = 0;

    int totalStallEvents = 0;
    int rawDependencyStallEvents = 0;
    int fuBusyStallEvents = 0;
    int memoryOrderingStallEvents = 0;

    int cdbBroadcasts = 0;
    int cdbQueueMaxSize = 0;

    int branchCount = 0;
    int branchCorrect = 0;
    int branchMispredictions = 0;

    int intInstructionsCommitted = 0;
    int mulInstructionsCommitted = 0;
    int fpAddInstructionsCommitted = 0;
    int fpMulInstructionsCommitted = 0;
    int loadInstructionsCommitted = 0;
    int storeInstructionsCommitted = 0;
    int branchInstructionsCommitted = 0;

    int robMaxOccupancy = 0;
    int intRsMaxOccupancy = 0;
    int mulRsMaxOccupancy = 0;
    int fpAddRsMaxOccupancy = 0;
    int fpMulRsMaxOccupancy = 0;
    int loadBufferMaxOccupancy = 0;
    int storeBufferMaxOccupancy = 0;
    int fpAddPipelineMaxOccupancy = 0;
    int fpMulPipelineMaxOccupancy = 0;

    double ipc() const;
    double branchAccuracy() const;
};

void recordCommittedInstruction(PerformanceStats& stats, OpCode opcode);
void printPerformanceStats(const PerformanceStats& stats);
