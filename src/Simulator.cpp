#include "Simulator.h"
#include "FunctionalUnit.h"
#include "ReservationStation.h"
#include "CDB.h"
#include "DebugPrinter.h"
#include "ROB.h"

#include <iostream>
#include <queue>

Simulator::Simulator() {

    //rf.write(1, 10);
    //rf.write(3, 5);
    //rf.write(5, 2);

    //mem.store(0, 99);
}

// Clock cycles required to perform operation
static int getLatency(OpCode opcode){
    switch (opcode) {
        case OpCode::ADD:
        case OpCode::ADDI:
        case OpCode::SUB:
        case OpCode::BEQ:
        case OpCode::BNE:
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

// Check if the instruction writes to a register (i.e., has a destination register)
static bool writesRegister(const Instruction& instr) {
    return instr.rd != -1;
}

static void flushActiveInstructions(
    std::vector<ActiveInstruction>& activeInstructions,
    int branchIndex,
    FunctionalUnit& intFU,
    FunctionalUnit& mulFU,
    FunctionalUnit& memFU
) {
    for (int i = 0; i < activeInstructions.size(); ) {
        if (activeInstructions[i].instructionIndex > branchIndex) {
            if (activeInstructions[i].executing) {
                FUType type = getFUType(activeInstructions[i].instr.opcode);
                FunctionalUnit* fu = getFU(type, intFU, mulFU, memFU);

                if (fu != nullptr && fu->busyUnits > 0) {
                    fu->busyUnits--;
                }
            }

            std::cout << "  Flushed RS/active: I"
                      << activeInstructions[i].instructionIndex
                      << " "
                      << activeInstructions[i].instr.rawText
                      << "\n";

            activeInstructions.erase(activeInstructions.begin() + i);
        } else {
            i++;
        }
    }
}

void flushRegProducers(std::vector<int>& regProducer, int branchIndex) {
    for (int reg = 0; reg < regProducer.size(); reg++) {
        if (regProducer[reg] > branchIndex) {
            std::cout << "  Cleared producer: R"
                      << reg
                      << " <- I"
                      << regProducer[reg]
                      << "\n";

            regProducer[reg] = -1;
        }
    }
}

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

            result.writesRegister = true;
            result.destinationRegister = instr.rd;
            result.value = value;
            break;
        }

        case OpCode::ADDI: {
            int value = active.vj + instr.immediate;

            result.writesRegister = true;
            result.destinationRegister = instr.rd;
            result.value = value;
            break;
        }

        case OpCode::SUB: {
            int value = active.vj - active.vk;

            result.writesRegister = true;
            result.destinationRegister = instr.rd;
            result.value = value;
            break;
        }

        case OpCode::MUL: {
            int value = active.vj * active.vk;

            result.writesRegister = true;
            result.destinationRegister = instr.rd;
            result.value = value;
            break;
        }

        case OpCode::LD: {
            int address = active.vj + instr.immediate;
            int value = mem.load(address);

            result.writesRegister = true;
            result.destinationRegister = instr.rd;
            result.value = value;
            break;
        }

        case OpCode::SD: {
            int address = active.vj + instr.immediate;
            int value = active.vk;

            result.writesMemory = true;
            result.memoryAddress = address;
            result.memoryValue = value;
            break;
        }
        case OpCode::BNE: {
            result.isBranch = true;
            result.branchTaken = active.vj != active.vk;
            result.branchTarget = instr.branchTarget;
            break;
        }

        case OpCode::BEQ: {
            result.isBranch = true;
            result.branchTaken = active.vj == active.vk;
            result.branchTarget = instr.branchTarget;
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
    const int ROB_CAPACITY = 4;

    int cycle = 1;
    int pc = 0;

    int nextDynamicId = 0;

    //bool unresolvedBranchInFlight = false;

    statusTable.clear();

    std::vector<ActiveInstruction> activeInstructions; // Act as our reservation station

    std::vector<int> regProducer(32, -1); // Initialize register producers. -1 means no registers currently writing to it

    std::queue<CDBMessage> cdbQueue;

    std::vector<ROBEntry> rob;
    std::queue<int> robQueue;

    // Functional unit initialization
    FunctionalUnit intFU {FUType::INT, 2, 0};
    FunctionalUnit mulFU {FUType::MUL, 1, 0};
    FunctionalUnit memFU {FUType::MEM, 1, 0};

    while (pc < instructions.size() || !activeInstructions.empty() || !cdbQueue.empty() || !robQueue.empty()) {

        std::cout << "\nCycle " << cycle << "\n";


        // Issue one instruction per cycle, in order
        if (pc < instructions.size()) {

            const Instruction& instrToIssue = instructions[pc];

            RSType rsType = getRSType(instrToIssue.opcode);

            int currentEntries = countRSEntries(activeInstructions, rsType);

            int capacity = getRSCapacity(rsType, INT_RS_CAPACITY, MUL_RS_CAPACITY, LOAD_BUFFER_CAPACITY, STORE_BUFFER_CAPACITY);
            /*
            if (unresolvedBranchInFlight) {
                std::cout << "Issue stalled: unresolved branch\n";
            }
            */
            if (robQueue.size() >= ROB_CAPACITY) {
                std::cout << "Issue stalled: " 
                        << instructions[pc].rawText 
                        << " | ROB full\n";
            }
            else if(currentEntries >= capacity){
                std::cout << "Issue stalled: " << instrToIssue.rawText
                  << " | " << rsTypeToString(rsType)
                  << " RS full\n";
            }   
            else{
                int dynamicId = nextDynamicId++;

                ActiveInstruction newInstr;
                newInstr.instr = instructions[pc];
                newInstr.instructionIndex = dynamicId;
                newInstr.remainingCycles = getLatency(newInstr.instr.opcode);
                newInstr.executing = false;
                newInstr.waitingReason = "Not started";

                newInstr.issueCycle = cycle;

                newInstr.qj = -1;
                newInstr.qk = -1;
                newInstr.vj = 0;
                newInstr.vk = 0;
                bool isBranch = newInstr.instr.opcode == OpCode::BEQ || newInstr.instr.opcode == OpCode::BNE;

                if (newInstr.instr.rs1 != -1) { // Source register 1
                    int producer = regProducer[newInstr.instr.rs1];
                    if (producer == -1){
                        newInstr.vj = rf.read(newInstr.instr.rs1); // Read value from register file if no producer/RAW dependency
                        newInstr.qj= -1;
                    }
                    else if (rob[producer].ready && rob[producer].writesRegister) {
                        newInstr.vj = rob[producer].value; // Read value from ROB if it's in ROB waiting to be committed
                        newInstr.qj = -1;
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
                    else if (rob[producer].ready && rob[producer].writesRegister) {
                        newInstr.vk = rob[producer].value; // Read value from ROB if it's in ROB waiting to be committed
                        newInstr.qk = -1;
                    }
                    else{
                        newInstr.qk = producer; // Instruction waits for producer to broadcast
                    }
                }

                activeInstructions.push_back(newInstr); // Add new instruction to reservation station

                // Record instruction status
                InstructionStatus status;
                status.staticPc = pc;
                status.rawText = newInstr.instr.rawText;
                status.issueCycle = cycle;
                statusTable.push_back(status);

                if(writesRegister(newInstr.instr)){ // Check if instruction writes to a register, if so it is a producer
                    regProducer[newInstr.instr.rd] = dynamicId;
                }

                std::cout << "Issued: " << newInstr.instr.rawText << "\n";

                /*
                if (isBranch) {
                    unresolvedBranchInFlight = true;
                }
                */

                // Create ROB entry
                int tag = dynamicId;

                ROBEntry entry;
                entry.busy = true;
                entry.ready = false;
                entry.tag = tag;
                entry.rawText = newInstr.instr.rawText;

                rob.push_back(entry);
                robQueue.push(tag);

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
        printROB(robQueue, rob, ROB_CAPACITY);

        // Commit ROB entries
        commitROB(
            robQueue,
            rob,
            regProducer,
            rf,
            mem,
            statusTable,
            cycle
        );

        // Broadcast on CDB
        if (!cdbQueue.empty()) {
            CDBMessage cdb = cdbQueue.front();
            cdbQueue.pop();

            std::cout << "CDB Broadcast: I" << cdb.producerTag
                    << " " << cdb.rawText << "\n";

            broadcastCDB(cdb, activeInstructions, rob);
            statusTable[cdb.producerTag].writebackCycle = cycle;
            
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
                    ROBEntry& entry = rob[index];

                    entry.ready = true;
                    entry.writesMemory = true;
                    entry.memoryAddress = result.memoryAddress;
                    entry.memoryValue = result.memoryValue;

                    statusTable[index].writebackCycle = cycle;

                    std::cout << "  Store result ready in ROB: Mem["
                            << result.memoryAddress << "] = "
                            << result.memoryValue << "\n";

                    std::cout << "  No CDB broadcast\n";
                }

                if (result.isBranch) {
                    ROBEntry& entry = rob[index];
                    entry.ready = true;

                    statusTable[index].writebackCycle = cycle;

                    std::cout << "  Branch resolved: "
                            << (result.branchTaken ? "taken" : "not taken")
                            << "\n";

                    bool predictedTaken = false; // static always-not-taken predictor

                    if (result.branchTaken != predictedTaken) {
                        std::cout << "  Branch misprediction detected\n";
                        std::cout << "  Predicted: not taken\n";
                        std::cout << "  Actual: taken\n";

                        pc = result.branchTarget;

                        std::cout << "  PC redirected to instruction "
                                << result.branchTarget
                                << "\n";

                        flushActiveInstructions(activeInstructions, index, intFU, mulFU, memFU);
                        flushCDBQueue(cdbQueue, index);
                        flushROBQueue(robQueue, rob, index);
                        flushRegProducers(regProducer, index);
                    } else {
                        std::cout << "  Branch prediction correct\n";
                    }
                    //unresolvedBranchInFlight = false;
                    std::cout << "  ROB entry ready\n";
                }

                if (!result.writesRegister && !result.writesMemory && !result.isBranch) {
                    ROBEntry& entry = rob[index];
                    entry.ready = true;

                    statusTable[index].writebackCycle = cycle;

                    std::cout << "  No register/memory result\n";
                    std::cout << "  ROB entry ready\n";
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

    for (int i = 0; i < statusTable.size(); i++) {
    std::cout
        << "I" << i
        << " | pc: " << statusTable[i].staticPc
        << " | " << statusTable[i].rawText
        << " | Issue: " << statusTable[i].issueCycle
        << " | Exec Start: " << statusTable[i].executeStartCycle
        << " | Exec End: " << statusTable[i].executeEndCycle
        << " | WB: " << statusTable[i].writebackCycle
        << " | Commit: " << statusTable[i].commitCycle
        << "\n";
    }
}   