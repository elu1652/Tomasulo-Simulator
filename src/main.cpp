#include "Parser.h"
#include "Simulator.h"
#include "BranchPredictor.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string filename = "../tests/nested_loop.asm";
    BranchPredictorType predictorType = BranchPredictorType::TwoBit;

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
        } else {
            filename = arg;
        }
    }

    std::cout << "Running program: " << filename << "\n";

    std::cout << "\nBranch predictor: "
          << branchPredictorTypeToString(predictorType)
          << "\n";

    Parser parser;
    std::vector<Instruction> instructions = parser.parseFile(filename);

    if (instructions.empty()) {
        std::cerr << "No instructions loaded from: " << filename << "\n";
        return 1;
    }

    Simulator sim(predictorType);
    sim.execute(instructions);

    return 0;
}
