#include "FunctionalUnit.h"

// Map each opcode class onto the structural execution resource it uses.
FUType getFUType(OpCode opcode) {
    switch (opcode) {
        case OpCode::ADD:
        case OpCode::ADDI:
        case OpCode::SUB:
        case OpCode::BEQ:
        case OpCode::BNE:
            return FUType::INT;

        case OpCode::MUL:
            return FUType::MUL;

        case OpCode::FADD:
        case OpCode::FSUB:
            return FUType::FP_ADD;

        case OpCode::FMUL:
        case OpCode::FDIV:
            return FUType::FP_MUL;

        case OpCode::LD:
        case OpCode::SD:
            return FUType::MEM;

        default:
            return FUType::NONE;
    }
}

// Return the concrete functional unit instance for a resource type.
FunctionalUnit* getFU(
    FUType type,
    FunctionalUnit& intFU,
    FunctionalUnit& mulFU,
    FunctionalUnit& fpAddFU,
    FunctionalUnit& fpMulFU,
    FunctionalUnit& memFU
) {
    switch (type) {
        case FUType::INT: return &intFU;
        case FUType::MUL: return &mulFU;
        case FUType::FP_ADD: return &fpAddFU;
        case FUType::FP_MUL: return &fpMulFU;
        case FUType::MEM: return &memFU;
        default: return nullptr;
    }
}

bool fuAvailable(FunctionalUnit* fu) {
    if (fu == nullptr) {
        return false;
    }

    if (!fu->pipelined) {
        return fu->busyUnits < fu->totalUnits;
    }

    for (const auto& pipeline : fu->pipelines) {
        if (!pipeline.empty() && !pipeline[0].occupied) {
            return true;
        }
    }

    return false;
}

std::string fuTypeToString(FUType type) {
    switch (type) {
        case FUType::INT: return "INT";
        case FUType::MUL: return "MUL";
        case FUType::FP_ADD: return "FP_ADD";
        case FUType::FP_MUL: return "FP_MUL";
        case FUType::MEM: return "MEM";
        default: return "NONE";
    }
}

void initializePipeline(FunctionalUnit& fu) {
    if (!fu.pipelined) {
        return;
    }

    fu.pipelines.clear();

    for (int unit = 0; unit < fu.totalUnits; unit++) {
        std::vector<PipelineStage> pipeline;

        for (int stage = 0; stage < fu.pipelineDepth; stage++) {
            pipeline.push_back(PipelineStage{});
        }

        fu.pipelines.push_back(pipeline);
    }
}

void advancePipeline(FunctionalUnit& fu) {
    if (!fu.pipelined) {
        return;
    }

    for (auto& pipeline : fu.pipelines) {
        if (pipeline.empty()) {
            continue;
        }

        // Drop whatever is in the final stage visually.
        // Actual execution completion is still controlled by remainingCycles.
        pipeline.back() = PipelineStage{};

        for (int stage = static_cast<int>(pipeline.size()) - 1; stage > 0; stage--) {
            pipeline[stage] = pipeline[stage - 1];
        }

        pipeline[0] = PipelineStage{};
    }
}

bool insertIntoPipeline(FunctionalUnit& fu, int instructionId) {
    if (!fu.pipelined) {
        return false;
    }

    for (auto& pipeline : fu.pipelines) {
        if (!pipeline.empty() && !pipeline[0].occupied) {
            pipeline[0].occupied = true;
            pipeline[0].instructionId = instructionId;
            return true;
        }
    }

    return false;
}

void removeFromPipeline(FunctionalUnit& fu, int instructionId) {
    if (!fu.pipelined) {
        return;
    }

    for (auto& pipeline : fu.pipelines) {
        for (auto& stage : pipeline) {
            if (stage.occupied && stage.instructionId == instructionId) {
                stage = PipelineStage{};
                return;
            }
        }
    }
}