#include "Parser.h"
#include "Simulator.h"

int main() {

    Parser parser;

    auto instructions = parser.parseFile("../examples/simple.asm");

    Simulator sim;

    sim.execute(instructions);

    return 0;
}