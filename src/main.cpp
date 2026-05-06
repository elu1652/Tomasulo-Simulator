#include <iostream>
#include "Parser.h"
#include "RegisterFile.h"
#include "Memory.h"

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



    Memory memory;

    memory.store(0, 99);
    memory.store(4, 123);

    std::cout << memory.load(0) << "\n";
    std::cout << memory.load(4) << "\n";

    return 0;
}