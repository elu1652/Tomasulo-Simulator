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
    bool executing;
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

// Check if the instruction writes to a register (i.e., has a destination register)
static bool writesRegister(const Instruction& instr) {
    return instr.rd != -1;
}

// Check for RAW dependencies
static bool hasRawDependency(const Instruction& instr, const std::vector<bool>& regPending) {
    if (instr.rs1 != -1 && regPending[instr.rs1]) {
        return true;
    }

    if (instr.rs2 != -1 && regPending[instr.rs2]) {
        return true;
    }

    return false;
}

void Simulator::execute(const std::vector<Instruction>& instructions) {
    int cycle = 1;
    int pc = 0;

    statusTable.clear();
    statusTable.resize(instructions.size());

    std::vector<ActiveInstruction> activeInstructions;

    std::vector<bool> regPending(32, false);

    while (pc < instructions.size() || !activeInstructions.empty()) {

        std::cout << "\nCycle " << cycle << "\n";

        // Issue one instruction per cycle, in order
        if (pc < instructions.size()) {
            ActiveInstruction newInstr;
            newInstr.instr = instructions[pc];
            newInstr.instructionIndex = pc;
            newInstr.remainingCycles = getLatency(newInstr.instr.opcode);
            newInstr.executing = false;

            activeInstructions.push_back(newInstr);

            statusTable[pc].issueCycle = cycle;
            statusTable[pc].executeStartCycle = cycle;

            if(writesRegister(newInstr.instr)){
                regPending[newInstr.instr.rd] = true;
            }

            std::cout << "Issued: " << newInstr.instr.rawText << "\n";

            pc++;
        }

        // Execute all active instructions
        for (auto& active : activeInstructions) {
            if(!active.executing){
                if(hasRawDependency(active.instr, regPending)){
                    std::cout << "Waiting " << active.instr.rawText << " | RAW dependency\n";
                    continue;
                }
                active.executing = true;
                statusTable[active.instructionIndex].executeStartCycle = cycle;
            }
            std::cout << "Executing: " << active.instr.rawText << " | remaining: " << active.remainingCycles << "\n";

            active.remainingCycles--;
        }

        // Complete finished instructions
        for (int i = 0; i < activeInstructions.size(); ) {
            if (activeInstructions[i].remainingCycles == 0) {
                int index = activeInstructions[i].instructionIndex;

                executeInstruction(activeInstructions[i].instr);

                if (writesRegister(activeInstructions[i].instr)) {
                    regPending[activeInstructions[i].instr.rd] = false;
                }

                statusTable[index].executeEndCycle = cycle;
                statusTable[index].writebackCycle = cycle;
                statusTable[index].commitCycle = cycle;

                std::cout << "Completed: "
                          << activeInstructions[i].instr.rawText << "\n";

                activeInstructions.erase(activeInstructions.begin() + i);
            } else {
                i++;
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
            << " | Issue: " << statusTable[i].issueCycle
            << " | Exec Start: " << statusTable[i].executeStartCycle
            << " | Exec End: " << statusTable[i].executeEndCycle
            << " | WB: " << statusTable[i].writebackCycle
            << " | Commit: " << statusTable[i].commitCycle
            << "\n";
    }
}