#include "ArchitectureConfig.h"

#include <sstream>
#include <string>
#include <unordered_map>

namespace {

struct Range {
    int min;
    int max;
};

const std::unordered_map<std::string, Range>& configRanges() {
    static const std::unordered_map<std::string, Range> ranges = {
        {"robCapacity", {1, 128}},
        {"intRsCapacity", {1, 64}},
        {"mulRsCapacity", {1, 64}},
        {"fpAddRsCapacity", {1, 64}},
        {"fpMulRsCapacity", {1, 64}},
        {"loadBufferCapacity", {1, 64}},
        {"storeBufferCapacity", {1, 64}},
        {"intFuCount", {1, 16}},
        {"mulFuCount", {1, 16}},
        {"memFuCount", {1, 16}},
        {"fpAddPipelineCount", {1, 16}},
        {"fpAddPipelineDepth", {1, 32}},
        {"fpMulPipelineCount", {1, 16}},
        {"fpMulPipelineDepth", {1, 32}},
        {"intLatency", {1, 64}},
        {"mulLatency", {1, 64}},
        {"loadLatency", {1, 64}},
        {"storeLatency", {1, 64}},
        {"fpAddLatency", {1, 64}},
        {"fpMulLatency", {1, 64}},
        {"fpDivLatency", {1, 64}},
    };

    return ranges;
}

bool validateRange(
    const std::string& key,
    int value,
    std::string& error
) {
    const auto& ranges = configRanges();
    auto it = ranges.find(key);

    if (it == ranges.end()) {
        error = "unknown architecture config key: " + key;
        return false;
    }

    if (value < it->second.min || value > it->second.max) {
        error = key + " must be between " +
                std::to_string(it->second.min) +
                " and " +
                std::to_string(it->second.max);
        return false;
    }

    return true;
}

} // namespace

bool applyArchitectureConfigOverride(
    ArchitectureConfig& config,
    const std::string& key,
    int value,
    std::string& error
) {
    if (!validateRange(key, value, error)) {
        return false;
    }

    if (key == "robCapacity") config.robCapacity = value;
    else if (key == "intRsCapacity") config.intRsCapacity = value;
    else if (key == "mulRsCapacity") config.mulRsCapacity = value;
    else if (key == "fpAddRsCapacity") config.fpAddRsCapacity = value;
    else if (key == "fpMulRsCapacity") config.fpMulRsCapacity = value;
    else if (key == "loadBufferCapacity") config.loadBufferCapacity = value;
    else if (key == "storeBufferCapacity") config.storeBufferCapacity = value;
    else if (key == "intFuCount") config.intFuCount = value;
    else if (key == "mulFuCount") config.mulFuCount = value;
    else if (key == "memFuCount") config.memFuCount = value;
    else if (key == "fpAddPipelineCount") config.fpAddPipelineCount = value;
    else if (key == "fpAddPipelineDepth") config.fpAddPipelineDepth = value;
    else if (key == "fpMulPipelineCount") config.fpMulPipelineCount = value;
    else if (key == "fpMulPipelineDepth") config.fpMulPipelineDepth = value;
    else if (key == "intLatency") config.intLatency = value;
    else if (key == "mulLatency") config.mulLatency = value;
    else if (key == "loadLatency") config.loadLatency = value;
    else if (key == "storeLatency") config.storeLatency = value;
    else if (key == "fpAddLatency") config.fpAddLatency = value;
    else if (key == "fpMulLatency") config.fpMulLatency = value;
    else if (key == "fpDivLatency") config.fpDivLatency = value;

    return true;
}

bool parseArchitectureConfigOverrides(
    const std::string& spec,
    ArchitectureConfig& config,
    std::string& error
) {
    std::stringstream stream(spec);
    std::string item;

    while (std::getline(stream, item, ',')) {
        if (item.empty()) {
            continue;
        }

        size_t equals = item.find('=');

        if (equals == std::string::npos) {
            error = "expected key=value in architecture config: " + item;
            return false;
        }

        std::string key = item.substr(0, equals);
        std::string valueText = item.substr(equals + 1);

        try {
            size_t parsedChars = 0;
            int value = std::stoi(valueText, &parsedChars);

            if (parsedChars != valueText.size()) {
                error = key + " must be an integer";
                return false;
            }

            if (!applyArchitectureConfigOverride(config, key, value, error)) {
                return false;
            }
        } catch (...) {
            error = key + " must be an integer";
            return false;
        }
    }

    return true;
}
