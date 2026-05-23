#pragma once

#include <string>
#include <utility>
#include <vector>

class Memory;
class RegisterFile;

// Assembly setup directives are test/program initialization metadata.
// They are applied before cycle 1 and intentionally do not allocate dynamic
// instructions, enter the ROB/RS/CDB, or appear in the instruction timeline.
struct ProgramSetup {
    std::vector<std::pair<int, int>> registerInitializers;
    std::vector<std::pair<int, int>> memoryInitializers;
    std::vector<std::string> directiveLines;
};

void applyProgramSetup(
    RegisterFile& rf,
    Memory& mem,
    const ProgramSetup& setup
);
