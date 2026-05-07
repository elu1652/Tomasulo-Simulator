#include "Parser.h"
#include "Simulator.h"

int main() {

    Parser parser;

    auto instructions = parser.parseFile("../examples/cdb_test.asm");

    Simulator sim;

    sim.execute(instructions);

    return 0;
}