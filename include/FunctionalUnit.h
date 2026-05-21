#pragma once

#include "Instruction.h"
#include <string>
#include <vector>

enum class FUType {
    INT,
    MUL,
    FP_ADD,
    FP_MUL,
    MEM,
    NONE
};

struct PipelineStage {
    bool occupied = false;
    int instructionId = -1;
};

struct FunctionalUnit {
    FUType type;
    int totalUnits;
    int busyUnits;

    bool pipelined = false;
    int pipelineDepth = 1;

    // Pipeline stages store dynamic instruction IDs for resource and trace
    // visualization; ActiveInstruction owns the execution state.
    std::vector<std::vector<PipelineStage>> pipelines;
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

void initializePipeline(FunctionalUnit& fu);

void advancePipeline(FunctionalUnit& fu);

bool insertIntoPipeline(FunctionalUnit& fu, int instructionId);

void removeFromPipeline(FunctionalUnit& fu, int instructionId);
