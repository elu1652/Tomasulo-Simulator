#include "Simulator.h"

#include <iostream>

Simulator::Simulator() {

    rf.write(1, 10);
    rf.write(3, 5);
    rf.write(5, 2);

    mem.store(0, 99);
}

static int getLatency(OpCode opcode){
    switch (opcode) {
        case OpCode::ADD:
        case OpCode::SUB:
            return 1;

        case OpCode::MUL:
            return 3;

        case OpCode::LD:
        case OpCode::SD:
            return 2;

        default:
            return 1;
    }
}

struct ActiveInstruction {
    Instruction instr;
    int instructionIndex;
    int remainingCycles;
    bool busy;
};

void Simulator::executeInstruction(const Instruction& instr) {
    switch (instr.opcode) {

        case OpCode::ADD: {
            int result = rf.read(instr.rs1) + rf.read(instr.rs2);
            rf.write(instr.rd, result);
            break;
        }

        case OpCode::SUB: {
            int result = rf.read(instr.rs1) - rf.read(instr.rs2);
            rf.write(instr.rd, result);
            break;
        }

        case OpCode::MUL: {
            int result = rf.read(instr.rs1) * rf.read(instr.rs2);
            rf.write(instr.rd, result);
            break;
        }

        case OpCode::LD: {
            int address = rf.read(instr.rs1) + instr.immediate;
            int value = mem.load(address);
            rf.write(instr.rd, value);
            break;
        }

        case OpCode::SD: {
            int address = rf.read(instr.rs1) + instr.immediate;
            int value = rf.read(instr.rs2);
            mem.store(address, value);
            break;
        }

        default:
            break;
    }
}

void Simulator::execute(const std::vector<Instruction>& instructions) {

    int cycle = 1;
    int pc = 0;

    ActiveInstruction active;
    active.remainingCycles = 0;
    active.busy = false;

    statusTable.clear();
    statusTable.resize(instructions.size());

    while(pc < instructions.size() || active.busy){
        std::cout << "\nCycle " << cycle << "\n";

        if(!active.busy && pc < instructions.size()){
            active.instr = instructions[pc];
            active.instructionIndex = pc;
            active.remainingCycles = getLatency(active.instr.opcode);
            active.busy = true;
            statusTable[active.instructionIndex].startCycle = cycle;
            std::cout << "Started: " << active.instr.rawText << "\n";
            pc++;
        }
        if(active.busy){
            std::cout << "Executing: " << active.instr.rawText << " | remaining: " << active.remainingCycles << "\n";

            active.remainingCycles--;

            if (active.remainingCycles == 0) {
                executeInstruction(active.instr);

                statusTable[active.instructionIndex].completeCycle = cycle;

                std::cout << "Completed: " << active.instr.rawText << "\n";

                active.busy = false;
            }
        }
        cycle++;
    }
    

    std::cout << "\nFinal Register State:\n";
    rf.print();

    std::cout << "\nMemory State:\n";
    mem.print();

    std::cout << "\nInstruction Status Table:\n";

    for (int i = 0; i < instructions.size(); i++) {
        std::cout
            << instructions[i].rawText
            << " | Start: "
            << statusTable[i].startCycle
            << " | Complete: "
            << statusTable[i].completeCycle
            << "\n";
    }
}