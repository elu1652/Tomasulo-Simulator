#include "Parser.h"
#include "Simulator.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string filename;

    if (argc >= 2) {
        filename = argv[1];          // use file passed from terminal
    } else {
        filename = "../tests/nested_loop.asm";    // default file if no argument given
    }

    std::cout << "Running program: " << filename << "\n";

    Parser parser;
    std::vector<Instruction> instructions = parser.parseFile(filename);

    if (instructions.empty()) {
        std::cerr << "No instructions loaded from: " << filename << "\n";
        return 1;
    }

    Simulator sim;
    sim.execute(instructions);

    return 0;
}