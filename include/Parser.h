#pragma once

#include <string>
#include <vector>
#include "Instruction.h"

class Parser {
public:
    std::vector<Instruction> parseFile(const std::string& filename);
};