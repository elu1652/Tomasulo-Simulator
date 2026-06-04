#pragma once

#include <string>

struct ArchitectureConfig {
    // Defaults are the project behavior used by CLI/tests when no per-run
    // frontend or command-line override is provided.
    int robCapacity = 4;

    int intRsCapacity = 2;
    int mulRsCapacity = 2;
    int fpAddRsCapacity = 2;
    int fpMulRsCapacity = 4;
    int loadBufferCapacity = 2;
    int storeBufferCapacity = 2;

    int intFuCount = 2;
    int mulFuCount = 1;
    int memFuCount = 2;

    int fpAddPipelineCount = 1;
    int fpAddPipelineDepth = 4;

    int fpMulPipelineCount = 1;
    int fpMulPipelineDepth = 7;

    int intLatency = 1;
    int mulLatency = 3;
    int loadLatency = 2;
    int storeLatency = 2;
    int fpAddLatency = 4;
    int fpMulLatency = 7;
    int fpDivLatency = 10;

    bool l1dEnabled = false;
    int l1dNumSets = 8;
    int l1dBlockSizeWords = 4   ;
    int l1dHitLatency = 1;
    int l1dMissPenalty = 10;
};

bool applyArchitectureConfigOverride(
    ArchitectureConfig& config,
    const std::string& key,
    int value,
    std::string& error
);

bool parseArchitectureConfigOverrides(
    const std::string& spec,
    ArchitectureConfig& config,
    std::string& error
);
