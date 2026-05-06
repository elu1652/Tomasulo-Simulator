#include <iostream>
#include "Parser.h"

int main() {
    Parser parser;

    auto instructions = parser.parseFile("../examples/simple.asm");

    std::cout << "Parsed instructions:\n";

    for (const auto& instr : instructions) {
        std::cout << instr.rawText << "\n";
        std::cout << "  rd: " << instr.rd << "\n";
        std::cout << "  rs1: " << instr.rs1 << "\n";
        std::cout << "  rs2: " << instr.rs2 << "\n";
        std::cout << "  imm: " << instr.immediate << "\n";
    }

    return 0;
}