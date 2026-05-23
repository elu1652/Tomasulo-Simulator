#pragma once

#include <string>
#include <vector>
#include "Instruction.h"
#include "Program.h"

class Parser {
public:
    ParsedProgram parseProgram(const std::string& filename);
    std::vector<Instruction> parseFile(const std::string& filename);
};
