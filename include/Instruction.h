#pragma once

#include <string>

enum class OpCode {
    ADD,
    SUB,
    MUL,
    LD,
    SD,
    BNE,
    INVALID
};

struct Instruction {
    OpCode opcode = OpCode::INVALID;

    int rd = -1;
    int rs1 = -1;
    int rs2 = -1;

    int immediate = 0;

    std::string label;
    std::string rawText;
};