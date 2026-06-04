#include "Parser.h"
#include "Simulator.h"
#include "BranchPredictor.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

bool parseDirectiveValue(
    const std::string& text,
    int& value,
    std::string& error
) {
    if (text == "true") {
        value = 1;
        return true;
    }

    if (text == "false") {
        value = 0;
        return true;
    }

    try {
        size_t parsedChars = 0;
        value = std::stoi(text, &parsedChars);

        if (parsedChars != text.size()) {
            error = "expected integer or boolean value: " + text;
            return false;
        }
    } catch (...) {
        error = "expected integer or boolean value: " + text;
        return false;
    }

    return true;
}

bool applyArchitectureConfigDirectivesFromFile(
    const std::string& filename,
    ArchitectureConfig& architectureConfig,
    std::string& error
) {
    // Test/demo files can enable and tune L1D without requiring a long
    // command-line override. Explicit --arch-config values are applied later
    // so CLI overrides still win.
    static const std::unordered_map<std::string, std::string> directiveKeys = {
        {"ARCH_L1D_ENABLED", "l1dEnabled"},
        {"ARCH_L1D_NUM_SETS", "l1dNumSets"},
        {"ARCH_L1D_BLOCK_SIZE_WORDS", "l1dBlockSizeWords"},
        {"ARCH_L1D_HIT_LATENCY", "l1dHitLatency"},
        {"ARCH_L1D_MISS_PENALTY", "l1dMissPenalty"},
    };

    std::ifstream input(filename);

    if (!input.is_open()) {
        return true;
    }

    std::string line;
    int lineNumber = 0;

    while (std::getline(input, line)) {
        lineNumber++;

        std::istringstream stream(line);
        std::string commentMarker;
        std::string directive;
        std::string valueText;

        if (!(stream >> commentMarker >> directive >> valueText)) {
            continue;
        }

        if (commentMarker != "#") {
            continue;
        }

        auto it = directiveKeys.find(directive);

        if (it == directiveKeys.end()) {
            continue;
        }

        int value = 0;

        if (!parseDirectiveValue(valueText, value, error)) {
            error = filename + ":" + std::to_string(lineNumber) + ": " + error;
            return false;
        }

        if (!applyArchitectureConfigOverride(
                architectureConfig,
                it->second,
                value,
                error
            )) {
            error = filename + ":" + std::to_string(lineNumber) + ": " + error;
            return false;
        }
    }

    return true;
}

} // namespace

int main(int argc, char* argv[]) {
    std::string filename = "../tests/nested_loop.asm";
    BranchPredictorType predictorType = BranchPredictorType::TwoBit;
    ArchitectureConfig architectureConfig;
    std::vector<std::string> architectureConfigSpecs;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--predictor") {
            if (i + 1 >= argc) {
                std::cerr << "Missing predictor type after --predictor\n";
                return 1;
            }

            std::string mode = argv[++i];

            if (!parseBranchPredictorType(mode, predictorType)) {
                std::cerr << "Unknown predictor type: " << mode << "\n";
                std::cerr << "Valid options: always-not-taken, always-taken, one-bit, two-bit\n";
                std::cerr << "Aliases: not-taken, taken, 1bit, 1-bit, 2bit, 2-bit\n";
                return 1;
            }
        } else if (arg == "--arch-config") {
            if (i + 1 >= argc) {
                std::cerr << "Missing config after --arch-config\n";
                return 1;
            }

            architectureConfigSpecs.push_back(argv[++i]);
        } else {
            filename = arg;
        }
    }

    std::string error;

    if (!applyArchitectureConfigDirectivesFromFile(
            filename,
            architectureConfig,
            error
        )) {
        std::cerr << "Invalid architecture config directive: "
                  << error
                  << "\n";
        return 1;
    }

    for (const std::string& spec : architectureConfigSpecs) {
        if (!parseArchitectureConfigOverrides(
                spec,
                architectureConfig,
                error
            )) {
            std::cerr << "Invalid architecture config: "
                      << error
                      << "\n";
            return 1;
        }
    }

    std::cout << "Running program: " << filename << "\n";

    std::cout << "\nBranch predictor: "
          << branchPredictorTypeToString(predictorType)
          << "\n";

    Parser parser;
    ParsedProgram program = parser.parseProgram(filename);

    if (program.instructions.empty()) {
        std::cerr << "No instructions loaded from: " << filename << "\n";
        return 1;
    }

    Simulator sim(predictorType, architectureConfig);
    sim.execute(program.instructions, program.setup);

    return 0;
}
