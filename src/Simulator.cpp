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
    std::string waitingReason;

    int qj; // tag for source operand 1
    int qk; // tag for source operand 2
};

ExecutionResult Simulator::executeInstruction(const Instruction& instr) {
    ExecutionResult result;
    result.writesRegister = false;
    result.destinationRegister = -1;
    result.value = 0;

    result.writesMemory = false;
    result.memoryAddress = -1;
    result.memoryValue = 0;

    switch (instr.opcode) {
        case OpCode::ADD: {
            int value = rf.read(instr.rs1) + rf.read(instr.rs2);
            rf.write(instr.rd, value);

            result.writesRegister = true;
            result.destinationRegister = instr.rd;
            result.value = value;
            break;
        }

        case OpCode::SUB: {
            int value = rf.read(instr.rs1) - rf.read(instr.rs2);
            rf.write(instr.rd, value);

            result.writesRegister = true;
            result.destinationRegister = instr.rd;
            result.value = value;
            break;
        }

        case OpCode::MUL: {
            int value = rf.read(instr.rs1) * rf.read(instr.rs2);
            rf.write(instr.rd, value);

            result.writesRegister = true;
            result.destinationRegister = instr.rd;
            result.value = value;
            break;
        }

        case OpCode::LD: {
            int address = rf.read(instr.rs1) + instr.immediate;
            int value = mem.load(address);
            rf.write(instr.rd, value);

            result.writesRegister = true;
            result.destinationRegister = instr.rd;
            result.value = value;
            break;
        }

        case OpCode::SD: {
            int address = rf.read(instr.rs1) + instr.immediate;
            int value = rf.read(instr.rs2);
            mem.store(address, value);

            result.writesMemory = true;
            result.memoryAddress = address;
            result.memoryValue = value;
            break;
        }

        default:
            break;
    }

    return result;
}

// Check if the instruction writes to a register (i.e., has a destination register)
static bool writesRegister(const Instruction& instr) {
    return instr.rd != -1;
}

// Types of functional units
enum class FUType {
    INT,
    MUL,
    MEM,
    NONE
};

// Determine which functional unit is used based on OpCode
static FUType getFUType(OpCode opcode) {
    switch (opcode) {
        case OpCode::ADD:
        case OpCode::SUB:
            return FUType::INT;

        case OpCode::MUL:
            return FUType::MUL;

        case OpCode::LD:
        case OpCode::SD:
            return FUType::MEM;

        default:
            return FUType::NONE;
    }
}

struct FunctionalUnit {
    FUType type;

    int totalUnits;
    int busyUnits;
};

// Return proper Functional Unit
static FunctionalUnit* getFU(
    FUType type,
    FunctionalUnit& intFU,
    FunctionalUnit& mulFU,
    FunctionalUnit& memFU
) {
    switch (type) {
        case FUType::INT: return &intFU;
        case FUType::MUL: return &mulFU;
        case FUType::MEM: return &memFU;
        default: return nullptr;
    }
}

// Check if functional unit is available
static bool fuAvailable(FunctionalUnit* fu) {
    return fu != nullptr && fu->busyUnits < fu->totalUnits;
}

static std::string fuTypeToString(FUType type) {
    switch (type) {
        case FUType::INT: return "INT";
        case FUType::MUL: return "MUL";
        case FUType::MEM: return "MEM";
        default: return "NONE";
    }
}

static void printFUState(
    const FunctionalUnit& intFU,
    const FunctionalUnit& mulFU,
    const FunctionalUnit& memFU
) {
    std::cout << "FU State:\n";

    std::cout << "  INT: "
              << intFU.busyUnits << "/" << intFU.totalUnits
              << " busy\n";

    std::cout << "  MUL: "
              << mulFU.busyUnits << "/" << mulFU.totalUnits
              << " busy\n";

    std::cout << "  MEM: "
              << memFU.busyUnits << "/" << memFU.totalUnits
              << " busy\n";
}

static void printTag(int tag) {
    if (tag == -1) {
        std::cout << "-";
    } else {
        std::cout << "I" << tag;
    }
}

static void printActiveInstructions(
    const std::vector<ActiveInstruction>& activeInstructions
) {
    std::cout << "Active Instructions:\n";

    if (activeInstructions.empty()) {
        std::cout << "  none\n";
        return;
    }

    for (const auto& active : activeInstructions) {
        std::cout << "  I" << active.instructionIndex
                  << ": " << active.instr.rawText << " | ";

        if (active.executing && active.remainingCycles == 0) {
            std::cout << "done this cycle";
        } else if (active.executing) {
            std::cout << "executing";
        } else {
            std::cout << "waiting";
        }

        std::cout << " | rem: " << active.remainingCycles;

        std::cout << " | qj: ";
        printTag(active.qj);

        std::cout << " | qk: ";
        printTag(active.qk);

        if (!active.executing) {
            std::cout << " | reason: " << active.waitingReason;
        }

        std::cout << "\n";
    }
}

static void printRegisterProducer(const std::vector<int>& regProducer) {
    std::cout << "Register Producers:\n";

    bool anyPending = false;

    for (int i = 0; i < regProducer.size(); i++) {
        if (regProducer[i] != -1) {
            std::cout << "  R" << i
                      << " <- I" << regProducer[i]
                      << "\n";
            anyPending = true;
        }
    }

    if (!anyPending) {
        std::cout << "  none\n";
    }
}

void Simulator::execute(const std::vector<Instruction>& instructions) {
    int cycle = 1;
    int pc = 0;

    statusTable.clear();
    statusTable.resize(instructions.size());

    std::vector<ActiveInstruction> activeInstructions;

    std::vector<int> regProducer(32, -1);

    FunctionalUnit intFU {FUType::INT, 1, 0};
    FunctionalUnit mulFU {FUType::MUL, 1, 0};
    FunctionalUnit memFU {FUType::MEM, 1, 0};

    while (pc < instructions.size() || !activeInstructions.empty()) {

        std::cout << "\nCycle " << cycle << "\n";


        // Issue one instruction per cycle, in order
        if (pc < instructions.size()) {
            ActiveInstruction newInstr;
            newInstr.instr = instructions[pc];
            newInstr.instructionIndex = pc;
            newInstr.remainingCycles = getLatency(newInstr.instr.opcode);
            newInstr.executing = false;
            newInstr.waitingReason = "Not started";

            newInstr.qj = -1;
            newInstr.qk = -1;

            if (newInstr.instr.rs1 != -1) {
                newInstr.qj = regProducer[newInstr.instr.rs1];
            }

            if (newInstr.instr.rs2 != -1) {
                newInstr.qk = regProducer[newInstr.instr.rs2];
            }

            activeInstructions.push_back(newInstr);

            statusTable[pc].issueCycle = cycle;

            if(writesRegister(newInstr.instr)){
                regProducer[newInstr.instr.rd] = pc;
            }

            std::cout << "Issued: " << newInstr.instr.rawText << "\n";

            pc++;
        }

        // Execution stage
        for (auto& active : activeInstructions) {
            if(!active.executing){
                // Check for RAW dependency. Don't execute if detected
                if(active.qj != -1 || active.qk != -1){
                    //std::cout << "Waiting " << active.instr.rawText << " | RAW dependency\n";
                    active.waitingReason = "RAW dependency";
                    continue;
                }

                FUType type = getFUType(active.instr.opcode);
                FunctionalUnit* fu = getFU(type, intFU, mulFU, memFU);
                if(!fuAvailable(fu)){
                    //std::cout << "Waiting " << active.instr.rawText << " | structural hazard: FU busy\n";
                    active.waitingReason = fuTypeToString(type) + " FU busy";
                    continue;
                }
                fu->busyUnits++;
                active.executing = true;
                active.waitingReason = "";

                statusTable[active.instructionIndex].executeStartCycle = cycle;
            }
            
            //std::cout << "Executing: " << active.instr.rawText << " | remaining: " << active.remainingCycles << "\n";

            active.remainingCycles--;
        }

        printFUState(intFU, mulFU, memFU);
        printRegisterProducer(regProducer);
        printActiveInstructions(activeInstructions);

        // Complete finished instructions
        for (int i = 0; i < activeInstructions.size(); ) {
            if (activeInstructions[i].remainingCycles == 0) {
                int index = activeInstructions[i].instructionIndex;

                ExecutionResult result = executeInstruction(activeInstructions[i].instr);

                if (result.writesRegister) {
                    int rd = result.destinationRegister;

                    if (regProducer[rd] == index) {
                        regProducer[rd] = -1;
                    }
                }

                FUType type = getFUType(activeInstructions[i].instr.opcode);
                FunctionalUnit* fu = getFU(type, intFU, mulFU, memFU);

                if (fu != nullptr) {
                    fu->busyUnits--;
                }

                statusTable[index].executeEndCycle = cycle;
                statusTable[index].writebackCycle = cycle;
                statusTable[index].commitCycle = cycle;

                std::cout << "Completed: I" << index << " " << activeInstructions[i].instr.rawText << "\n";

                if (result.writesRegister) {
                    std::cout << "  Result: R" << result.destinationRegister
                            << " = " << result.value << "\n";

                    std::cout << "  Broadcast: I" << index << "\n";
                }

                if (result.writesMemory) {
                    std::cout << "  Memory write: Mem[" << result.memoryAddress
                            << "] = " << result.memoryValue << "\n";

                    std::cout << "  No register broadcast\n";
                }

                // Wake up instructions waiting for this completed instruction
                bool wokeSomeone = false;

                for (auto& other : activeInstructions) {
                    if (other.qj == index) {
                        std::cout << "  Wakeup: I" << other.instructionIndex
                                << " qj resolved by I" << index << "\n";
                        other.qj = -1;
                        wokeSomeone = true;
                    }

                    if (other.qk == index) {
                        std::cout << "  Wakeup: I" << other.instructionIndex
                                << " qk resolved by I" << index << "\n";
                        other.qk = -1;
                        wokeSomeone = true;
                    }
                }

                if (result.writesRegister && !wokeSomeone) {
                    std::cout << "  Wakeup: none\n";
                }

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