#include "Simulator.h"

#include <iostream>

Simulator::Simulator() {

    rf.write(1, 10);
    rf.write(3, 5);
    rf.write(5, 2);

    mem.store(0, 99);
}

void Simulator::execute(const std::vector<Instruction>& instructions) {

    for (const auto& instr : instructions) {

        switch (instr.opcode) {

            case OpCode::ADD: {
                int result = rf.read(instr.rs1) + rf.read(instr.rs2);
                rf.write(instr.rd, result);

                std::cout << "Executed ADD\n";
                break;
            }

            case OpCode::SUB: {
                int result = rf.read(instr.rs1) - rf.read(instr.rs2);
                rf.write(instr.rd, result);

                std::cout << "Executed SUB\n";
                break;
            }

            case OpCode::MUL: {
                int result = rf.read(instr.rs1) * rf.read(instr.rs2);
                rf.write(instr.rd, result);

                std::cout << "Executed MUL\n";
                break;
            }

            case OpCode::LD: {
                int address = rf.read(instr.rs1) + instr.immediate;
                int value = mem.load(address);

                rf.write(instr.rd, value);

                std::cout << "Executed LD\n";
                break;
            }

            case OpCode::SD: {
                int address = rf.read(instr.rs1) + instr.immediate;
                int value = rf.read(instr.rs2);

                mem.store(address, value);

                std::cout << "Executed SD\n";
                break;
            }

            default:
                break;
        }
    }

    std::cout << "\nFinal Register State:\n";
    rf.print();

    std::cout << "\nMemory State:\n";
    mem.print();
}