#include "Simulator.h"

#include <iostream>

#include <queue>

Simulator::Simulator() {

    rf.write(1, 10);
    rf.write(3, 5);
    rf.write(5, 2);

    mem.store(0, 99);
}

// Types of functional units
enum class FUType {
    INT,
    MUL,
    MEM,
    NONE
};

struct FunctionalUnit {
    FUType type;

    int totalUnits;
    int busyUnits;
};

enum class RSType {
    INT,
    MUL,
    LOAD,
    STORE,
    NONE
};

struct CDBMessage {
    bool valid;
    int producerTag;
    int destinationRegister;
    int value;

    std::string rawText;
};

// Forward declaration
static FUType getFUType(OpCode opcode);
static RSType getRSType(OpCode opcode);

// Clock cycles required to perform operation
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

/*************************************** 
* Reservation Station                  * 
***************************************/
static int countRSEntries(const std::vector<ActiveInstruction>& activeInstructions, RSType type){
    int count = 0;

    for (const auto& active : activeInstructions) {
        if (getRSType(active.instr.opcode) == type) {
            count++;
        }
    }

    return count;
}

static int getRSCapacity(RSType type, int intCapacity, int mulCapacity, int loadCapacity, int storeCapacity){
    switch (type) {
        case RSType::INT:
            return intCapacity;

        case RSType::MUL:
            return mulCapacity;

        case RSType::LOAD:
            return loadCapacity;

        case RSType::STORE:
            return storeCapacity;

        default:
            return 0;
    }
}

static RSType getRSType(OpCode opcode) {
    switch (opcode) {
        case OpCode::ADD:
        case OpCode::SUB:
            return RSType::INT;

        case OpCode::MUL:
            return RSType::MUL;

        case OpCode::LD:
            return RSType::LOAD;

        case OpCode::SD:
            return RSType::STORE;

        default:
            return RSType::NONE;
    }
}

static std::string rsTypeToString(RSType type) {
    switch (type) {
        case RSType::INT: return "INT";
        case RSType::MUL: return "MUL";
        case RSType::LOAD: return "LOAD";
        case RSType::STORE: return "STORE";
        default: return "NONE";
    }
}


// Check if the instruction writes to a register (i.e., has a destination register)
static bool writesRegister(const Instruction& instr) {
    return instr.rd != -1;
}

/*************************************** 
* Functional Unit Section              * 
***************************************/

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

/*************************************** 
* Debug Printing                       * 
***************************************/

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

        std::cout << " | vj: ";
        if (active.qj == -1) {
            std::cout << active.vj;
        } else {
            std::cout << "-";
        }

        std::cout << " | vk: ";
        if (active.qk == -1) {
            std::cout << active.vk;
        } else {
            std::cout << "-";
        }

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
            std::cout << "  R" << i << " <- I" << regProducer[i] << "\n";
            anyPending = true;
        }
    }

    if (!anyPending) {
        std::cout << "  none\n";
    }
}

static void printRSState(
    const std::vector<ActiveInstruction>& activeInstructions,
    int intCapacity,
    int mulCapacity,
    int loadCapacity,
    int storeCapacity
) {
    int intCount = countRSEntries(activeInstructions, RSType::INT);
    int mulCount = countRSEntries(activeInstructions, RSType::MUL);
    int loadCount = countRSEntries(activeInstructions, RSType::LOAD);
    int storeCount = countRSEntries(activeInstructions, RSType::STORE);

    std::cout << "RS State:\n";
    std::cout << "  INT RS: " << intCount << "/" << intCapacity << "\n";
    std::cout << "  MUL RS: " << mulCount << "/" << mulCapacity << "\n";
    std::cout << "  Load Buffer: " << loadCount << "/" << loadCapacity << "\n";
    std::cout << "  Store Buffer: " << storeCount << "/" << storeCapacity << "\n";
}

static void printCDBQueue(std::queue<CDBMessage> cdbQueue) {
    std::cout << "CDB Queue:\n";

    if (cdbQueue.empty()) {
        std::cout << "  none\n";
        return;
    }

    while (!cdbQueue.empty()) {
        CDBMessage msg = cdbQueue.front();
        cdbQueue.pop();

        std::cout << "  I" << msg.producerTag
                  << ": " << msg.rawText
                  << " | value: " << msg.value
                  << " -> R" << msg.destinationRegister
                  << "\n";
    }
}

static bool broadcastCDB(
    const CDBMessage& cdb,
    std::vector<ActiveInstruction>& activeInstructions,
    std::vector<int>& regProducer,
    RegisterFile& rf
) {
    if (!cdb.valid) {
        return false;
    }

    bool wokeSomeone = false;

    std::cout << "  Broadcast: I" << cdb.producerTag << "\n";

    if (regProducer[cdb.destinationRegister] == cdb.producerTag) {
        rf.write(cdb.destinationRegister, cdb.value);
        regProducer[cdb.destinationRegister] = -1;

        std::cout << "  RF Write: R" << cdb.destinationRegister
                  << " = " << cdb.value << "\n";
    } else {
        std::cout << "  RF Write: skipped, newer producer owns R"
                  << cdb.destinationRegister << "\n";
    }

    for (auto& other : activeInstructions) {
        if (other.qj == cdb.producerTag) {
            other.qj = -1;
            other.vj = cdb.value;

            std::cout << "  Wakeup: I" << other.instructionIndex
                      << " qj resolved by I" << cdb.producerTag
                      << " with value " << cdb.value << "\n";

            wokeSomeone = true;
        }

        if (other.qk == cdb.producerTag) {
            other.qk = -1;
            other.vk = cdb.value;

            std::cout << "  Wakeup: I" << other.instructionIndex
                      << " qk resolved by I" << cdb.producerTag
                      << " with value " << cdb.value << "\n";

            wokeSomeone = true;
        }
    }

    if (!wokeSomeone) {
        std::cout << "  Wakeup: none\n";
    }

    return wokeSomeone;
}

/*************************************** 
* Execution                            * 
***************************************/

ExecutionResult Simulator::computeResult(const ActiveInstruction& active) {
    ExecutionResult result;
    result.writesRegister = false;
    result.destinationRegister = -1;
    result.value = 0;

    result.writesMemory = false;
    result.memoryAddress = -1;
    result.memoryValue = 0;

    const Instruction& instr = active.instr;

    switch (instr.opcode) {
        case OpCode::ADD: {
            int value = active.vj + active.vk;
            //rf.write(instr.rd, value);

            result.writesRegister = true;
            result.destinationRegister = instr.rd;
            result.value = value;
            break;
        }

        case OpCode::SUB: {
            int value = active.vj - active.vk;
            //rf.write(instr.rd, value);

            result.writesRegister = true;
            result.destinationRegister = instr.rd;
            result.value = value;
            break;
        }

        case OpCode::MUL: {
            int value = active.vj * active.vk;
            //rf.write(instr.rd, value);

            result.writesRegister = true;
            result.destinationRegister = instr.rd;
            result.value = value;
            break;
        }

        case OpCode::LD: {
            int address = active.vj + instr.immediate;
            int value = mem.load(address);
            //rf.write(instr.rd, value);

            result.writesRegister = true;
            result.destinationRegister = instr.rd;
            result.value = value;
            break;
        }

        case OpCode::SD: {
            int address = active.vj + instr.immediate;
            int value = active.vk;
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



void Simulator::execute(const std::vector<Instruction>& instructions) {

    const int INT_RS_CAPACITY = 2;
    const int MUL_RS_CAPACITY = 2;
    const int LOAD_BUFFER_CAPACITY = 1;
    const int STORE_BUFFER_CAPACITY = 2;

    int cycle = 1;
    int pc = 0;

    statusTable.clear();
    statusTable.resize(instructions.size());

    std::vector<ActiveInstruction> activeInstructions; // Act as our reservation station

    std::vector<int> regProducer(32, -1); // Initialize register producers. -1 means no registers currently writing to it

    std::queue<CDBMessage> cdbQueue;

    // Functional unit initialization
    FunctionalUnit intFU {FUType::INT, 2, 0};
    FunctionalUnit mulFU {FUType::MUL, 1, 0};
    FunctionalUnit memFU {FUType::MEM, 1, 0};

    while (pc < instructions.size() || !activeInstructions.empty() || !cdbQueue.empty()) {

        std::cout << "\nCycle " << cycle << "\n";


        // Issue one instruction per cycle, in order
        if (pc < instructions.size()) {

            const Instruction& instrToIssue = instructions[pc];

            RSType rsType = getRSType(instrToIssue.opcode);

            int currentEntries = countRSEntries(activeInstructions, rsType);

            int capacity = getRSCapacity(rsType, INT_RS_CAPACITY, MUL_RS_CAPACITY, LOAD_BUFFER_CAPACITY, STORE_BUFFER_CAPACITY);
            
            if(currentEntries >= capacity){
                std::cout << "Issue stalled: " << instrToIssue.rawText
                  << " | " << rsTypeToString(rsType)
                  << " RS full\n";
            }   
            else{
                ActiveInstruction newInstr;
                newInstr.instr = instructions[pc];
                newInstr.instructionIndex = pc;
                newInstr.remainingCycles = getLatency(newInstr.instr.opcode);
                newInstr.executing = false;
                newInstr.waitingReason = "Not started";

                newInstr.issueCycle = cycle;

                newInstr.qj = -1;
                newInstr.qk = -1;
                newInstr.vj = 0;
                newInstr.vk = 0;

                if (newInstr.instr.rs1 != -1) { // Source register 1
                    int producer = regProducer[newInstr.instr.rs1];
                    if (producer == -1){
                        newInstr.vj = rf.read(newInstr.instr.rs1); // Read value from register file if no producer/RAW dependency
                        newInstr.qj= -1;
                    }
                    else{
                        newInstr.qj = producer; // Instruction waits for producer to broadcast
                    }
                }

                if (newInstr.instr.rs2 != -1) { // Source register 2
                    int producer = regProducer[newInstr.instr.rs2];
                    if (producer == -1){
                        newInstr.vk = rf.read(newInstr.instr.rs2); // Read value from register file if no producer/RAW dependency
                        newInstr.qk= -1;
                    }
                    else{
                        newInstr.qk = producer; // Instruction waits for producer to broadcast
                    }
                }

                activeInstructions.push_back(newInstr); // Add new instruction to reservation station

                statusTable[pc].issueCycle = cycle; // Record issue cycle 

                if(writesRegister(newInstr.instr)){ // Check if instruction writes to a register, if so it is a producer
                    regProducer[newInstr.instr.rd] = pc;
                }

                std::cout << "Issued: " << newInstr.instr.rawText << "\n";

                pc++;
                }
        }

        // Execution stage
        for (auto& active : activeInstructions) {
            if(!active.executing){
                if (active.issueCycle == cycle) {
                    active.waitingReason = "issued this cycle";
                    continue;
                }
                // Check for RAW dependency. Don't execute if detected
                if(active.qj != -1 || active.qk != -1){
                    active.waitingReason = "RAW dependency";
                    continue;
                }

                FUType type = getFUType(active.instr.opcode);
                FunctionalUnit* fu = getFU(type, intFU, mulFU, memFU);

                // Check for structural hazard/no FU available
                if(!fuAvailable(fu)){
                    active.waitingReason = fuTypeToString(type) + " FU busy";
                    continue;
                }

                // Update FU and active instruction status
                fu->busyUnits++;
                active.executing = true;
                active.waitingReason = "";

                statusTable[active.instructionIndex].executeStartCycle = cycle;
            }
            active.remainingCycles--;
        }

        // Print state of system
        printFUState(intFU, mulFU, memFU);
        printRSState(activeInstructions, INT_RS_CAPACITY, MUL_RS_CAPACITY, LOAD_BUFFER_CAPACITY, STORE_BUFFER_CAPACITY);
        printRegisterProducer(regProducer);
        printActiveInstructions(activeInstructions);
        printCDBQueue(cdbQueue);

        if (!cdbQueue.empty()) {
            CDBMessage cdb = cdbQueue.front();
            cdbQueue.pop();

            std::cout << "CDB Broadcast: I" << cdb.producerTag
                    << " " << cdb.rawText << "\n";

            broadcastCDB(cdb, activeInstructions, regProducer, rf);
            statusTable[cdb.producerTag].writebackCycle = cycle;
            statusTable[cdb.producerTag].commitCycle = cycle;
        } else {
            std::cout << "CDB Broadcast: none\n";
        }

        // Complete finished instructions
        for (int i = 0; i < activeInstructions.size(); ) {
            if (activeInstructions[i].remainingCycles == 0) {
                int index = activeInstructions[i].instructionIndex;

                ExecutionResult result = computeResult(activeInstructions[i]);

                FUType type = getFUType(activeInstructions[i].instr.opcode);
                FunctionalUnit* fu = getFU(type, intFU, mulFU, memFU);

                // Free FU
                if (fu != nullptr) {
                    fu->busyUnits--;
                }

                statusTable[index].executeEndCycle = cycle;

                // Print completion message
                std::cout << "Execution complete: I" << index << " " << activeInstructions[i].instr.rawText << "\n";

                if (result.writesRegister) {
                    std::cout << "  Result queued for CDB: R" << result.destinationRegister
                            << " = " << result.value << "\n";

                    CDBMessage cdb;
                    cdb.valid = result.writesRegister;
                    cdb.producerTag = index;
                    cdb.destinationRegister = result.destinationRegister;
                    cdb.value = result.value;
                    cdb.rawText = activeInstructions[i].instr.rawText;

                    cdbQueue.push(cdb);
                }

                if (result.writesMemory) {
                    std::cout << "  Memory write: Mem[" << result.memoryAddress
                            << "] = " << result.memoryValue << "\n";

                    std::cout << "  No register broadcast\n";
                    statusTable[index].writebackCycle = cycle;
                    statusTable[index].commitCycle = cycle;
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