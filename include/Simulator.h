#pragma once

#include <vector>

#include "Instruction.h"
#include "RegisterFile.h"
#include "Memory.h"

class Simulator {
private:
    RegisterFile rf;
    Memory mem;

public:
    Simulator();

    void execute(const std::vector<Instruction>& instructions);
};