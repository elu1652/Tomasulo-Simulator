#pragma once

#include <vector>

#include "Instruction.h"
#include "ProgramSetup.h"

struct ParsedProgram {
    std::vector<Instruction> instructions;
    ProgramSetup setup;
};
