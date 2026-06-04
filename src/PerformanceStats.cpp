#include "PerformanceStats.h"

#include <iomanip>
#include <iostream>

double PerformanceStats::ipc() const {
    if (totalCycles <= 0) {
        return 0.0;
    }

    return static_cast<double>(committedInstructions) / totalCycles;
}

double PerformanceStats::branchAccuracy() const {
    if (branchCount <= 0) {
        return 0.0;
    }

    return 100.0 * static_cast<double>(branchCorrect) / branchCount;
}

void recordCommittedInstruction(PerformanceStats& stats, OpCode opcode) {
    switch (opcode) {
        case OpCode::ADD:
        case OpCode::ADDI:
        case OpCode::SUB:
            stats.intInstructionsCommitted++;
            break;

        case OpCode::MUL:
            stats.mulInstructionsCommitted++;
            break;

        case OpCode::FADD:
        case OpCode::FSUB:
            stats.fpAddInstructionsCommitted++;
            break;

        case OpCode::FMUL:
        case OpCode::FDIV:
            stats.fpMulInstructionsCommitted++;
            break;

        case OpCode::LD:
            stats.loadInstructionsCommitted++;
            break;

        case OpCode::SD:
            stats.storeInstructionsCommitted++;
            break;

        case OpCode::BEQ:
        case OpCode::BNE:
            stats.branchInstructionsCommitted++;
            break;

        default:
            break;
    }
}

void printPerformanceStats(const PerformanceStats& stats) {
    // High-level performance.
    std::cout << "\nPerformance Summary:\n";
    std::cout << "  Total cycles: " << stats.totalCycles << "\n";
    std::cout << "  Committed instructions: " << stats.committedInstructions << "\n";
    std::cout << "  IPC: " << std::fixed << std::setprecision(2) << stats.ipc() << "\n";

    // Stall accounting.
    std::cout << "\nStall Summary:\n";
    std::cout << "  Cycles with any stall: " << stats.cyclesWithAnyStall << "\n";
    std::cout << "  Issue stall cycles: " << stats.issueStallCycles << "\n";
    std::cout << "  Backend stall cycles: " << stats.backendStallCycles << "\n";
    std::cout << "  Total stall events: " << stats.totalStallEvents << "\n";

    std::cout << "\nIssue Stalls:\n";
    std::cout << "  ROB full stall cycles: " << stats.robFullStallCycles << "\n";
    std::cout << "  RS full stall cycles: " << stats.rsFullStallCycles << "\n";

    std::cout << "\nBackend Stall Events:\n";
    std::cout << "  RAW dependency stall events: " << stats.rawDependencyStallEvents << "\n";
    std::cout << "  FU busy stall events: " << stats.fuBusyStallEvents << "\n";
    std::cout << "  Memory ordering stall events: " << stats.memoryOrderingStallEvents << "\n";

    if (stats.l1dEnabled) {
        // Optional L1D cache statistics. memoryStallCycles is the accumulated
        // miss penalty beyond normal hit latency.
        double hitRate = stats.l1dAccesses == 0
            ? 0.0
            : static_cast<double>(stats.l1dHits) / stats.l1dAccesses * 100.0;

        double missRate = stats.l1dAccesses == 0
            ? 0.0
            : static_cast<double>(stats.l1dMisses) / stats.l1dAccesses * 100.0;

        double avgLatency = stats.l1dAccesses == 0
            ? 0.0
            : static_cast<double>(stats.l1dTotalAccessLatency) / stats.l1dAccesses;

        std::cout << "\nL1 Data Cache:\n";
        std::cout << "  Accesses: " << stats.l1dAccesses << "\n";
        std::cout << "  Hits: " << stats.l1dHits << "\n";
        std::cout << "  Misses: " << stats.l1dMisses << "\n";
        std::cout << "  Hit rate: " << hitRate << "%\n";
        std::cout << "  Miss rate: " << missRate << "%\n";
        std::cout << "  Writebacks: " << stats.l1dWritebacks << "\n";
        std::cout << "  Average access latency: " << avgLatency << "\n";
        std::cout << "  Memory stall cycles: " << stats.memoryStallCycles << "\n";
    }

    // CDB, branch, mix, and occupancy summaries.
    std::cout << "\nCDB:\n";
    std::cout << "  Broadcasts: " << stats.cdbBroadcasts << "\n";
    std::cout << "  Max queue size: " << stats.cdbQueueMaxSize << "\n";

    std::cout << "\nBranches:\n";
    std::cout << "  Total branches: " << stats.branchCount << "\n";
    std::cout << "  Correct predictions: " << stats.branchCorrect << "\n";
    std::cout << "  Mispredictions: " << stats.branchMispredictions << "\n";
    std::cout << "  Accuracy: " << std::fixed << std::setprecision(1)
              << stats.branchAccuracy() << "%\n";

    std::cout << "\nCommitted Instruction Mix:\n";
    std::cout << "  INT: " << stats.intInstructionsCommitted << "\n";
    std::cout << "  MUL: " << stats.mulInstructionsCommitted << "\n";
    std::cout << "  FP_ADD: " << stats.fpAddInstructionsCommitted << "\n";
    std::cout << "  FP_MUL: " << stats.fpMulInstructionsCommitted << "\n";
    std::cout << "  LOAD: " << stats.loadInstructionsCommitted << "\n";
    std::cout << "  STORE: " << stats.storeInstructionsCommitted << "\n";
    std::cout << "  BRANCH: " << stats.branchInstructionsCommitted << "\n";

    std::cout << "\nMax Occupancy:\n";
    std::cout << "  ROB: " << stats.robMaxOccupancy << "\n";
    std::cout << "  INT RS: " << stats.intRsMaxOccupancy << "\n";
    std::cout << "  MUL RS: " << stats.mulRsMaxOccupancy << "\n";
    std::cout << "  FP_ADD RS: " << stats.fpAddRsMaxOccupancy << "\n";
    std::cout << "  FP_MUL RS: " << stats.fpMulRsMaxOccupancy << "\n";
    std::cout << "  Load Buffer: " << stats.loadBufferMaxOccupancy << "\n";
    std::cout << "  Store Buffer: " << stats.storeBufferMaxOccupancy << "\n";
    std::cout << "  FP_ADD pipeline: " << stats.fpAddPipelineMaxOccupancy << "\n";
    std::cout << "  FP_MUL pipeline: " << stats.fpMulPipelineMaxOccupancy << "\n";
}
