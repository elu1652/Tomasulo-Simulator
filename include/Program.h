#pragma once

#include <utility>
#include <vector>
#include <string>

#include "Instruction.h"

struct ProgramSetup {
    std::vector<std::pair<int, int>> registerInitializers;
    std::vector<std::pair<int, int>> memoryInitializers;
    std::vector<std::string> directiveLines;
};

struct ParsedProgram {
    std::vector<Instruction> instructions;
    ProgramSetup setup;
};
