#include "Simulator.h"
#include "FunctionalUnit.h"
#include "ReservationStation.h"
#include "CDB.h"
#include "DebugPrinter.h"
#include "ROB.h"
#include "Flush.h"
#include "BranchPredictor.h"

#include <iostream>
#include <queue>
#include <iomanip>

Simulator::Simulator() {

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

// Compute the result of an instruction after it finishes execution.
// Register results are queued for CDB broadcast.
// Store and branch results update the ROB directly because they do not broadcast a register value.
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

    statusTable.clear();

    // Active reservation station entries.
    // Each entry tracks operand values/tags, execution state, and remaining latency.
    std::vector<ActiveInstruction> activeInstructions; 

    // Register renaming table.
    // regProducer[R] stores the ROB/dynamic instruction tag that will produce R.
    // -1 means the architectural register file currently has the latest value.
    std::vector<int> regProducer(32, -1); 

    std::queue<CDBMessage> cdbQueue;

    std::vector<ROBEntry> rob;
    std::queue<int> robQueue;

    // Functional unit initialization
    FunctionalUnit intFU {FUType::INT, 2, 0};
    FunctionalUnit mulFU {FUType::MUL, 1, 0};
    FunctionalUnit memFU {FUType::MEM, 1, 0};

    
    BranchPredictor branchPredictor;

    // Main simulation loop.
    // The simulator continues until there are no more instructions to fetch,
    // no active reservation station entries, no pending CDB broadcasts,
    // and no uncommitted ROB entries.
    while (pc < instructions.size() || !activeInstructions.empty() || !cdbQueue.empty() || !robQueue.empty()) {

        std::cout << "\nCycle " << cycle << "\n";


        // ISSUE STAGE
        // Issue at most one instruction per cycle, in program order.
        // With always-not-taken branch prediction, branches do not stall issue;
        // the PC naturally advances to the fall-through path.
        if (pc < instructions.size()) {

            const Instruction& instrToIssue = instructions[pc];

            RSType rsType = getRSType(instrToIssue.opcode);

            int currentEntries = countRSEntries(activeInstructions, rsType);

            int capacity = getRSCapacity(rsType, INT_RS_CAPACITY, MUL_RS_CAPACITY, LOAD_BUFFER_CAPACITY, STORE_BUFFER_CAPACITY);

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

                newInstr.isBranch =
                    newInstr.instr.opcode == OpCode::BEQ ||
                    newInstr.instr.opcode == OpCode::BNE;

                if (newInstr.isBranch) {
                    newInstr.predictedTaken = branchPredictor.predict(pc);
                    newInstr.predictedTarget =
                        newInstr.predictedTaken ? newInstr.instr.branchTarget : pc + 1;

                    std::cout << "  Branch prediction: "
                            << (newInstr.predictedTaken ? "taken" : "not taken")
                            << "\n";
                }

                // Source operands are read either from the architectural register file,
                // from a ready ROB entry, or as producer tags if the value is still pending.
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

                // Register-writing instructions become the newest producer for their destination register.
                if(writesRegister(newInstr.instr)){
                    regProducer[newInstr.instr.rd] = dynamicId;
                }

                std::cout << "Issued: " << newInstr.instr.rawText << "\n";

                // Allocate a ROB entry for every issued instruction.
                // Even stores and branches enter the ROB so commit remains in program order.
                int tag = dynamicId;

                ROBEntry entry;
                entry.busy = true;
                entry.ready = false;
                entry.tag = tag;
                entry.rawText = newInstr.instr.rawText;

                rob.push_back(entry);
                robQueue.push(tag);

                if (newInstr.isBranch && newInstr.predictedTaken) {
                    pc = newInstr.instr.branchTarget;
                } else {
                    pc++;
                }
            }
        }

        // EXECUTE STAGE
        // Ready instructions start execution when operands are available and a matching FU is free.
        // Already-executing instructions decrement their remaining latency.
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

        // COMMIT STAGE
        // Commit happens before this cycle's CDB broadcast.
        // A result that broadcasts this cycle cannot commit until a later cycle.
        commitROB(
            robQueue,
            rob,
            regProducer,
            rf,
            mem,
            statusTable,
            cycle
        );

        // WRITEBACK / CDB BROADCAST STAGE
        // Broadcast at most one queued result per cycle.
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

        // COMPLETION STAGE
        // Instructions that reach zero remaining cycles finish here.
        // Register-writing instructions enqueue a CDB message;
        // stores and branches mark their ROB entries ready directly.
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

                    // Static always-not-taken branch prediction.
                    // Since issue already followed pc + 1, a taken branch is a misprediction.
                    // Recovery redirects the PC and flushes all younger wrong-path instructions.
                    bool predictedTaken = activeInstructions[i].predictedTaken;
                    
                    int branchPc = statusTable[index].staticPc;
                    branchPredictor.update(branchPc, result.branchTaken);

                    if (result.branchTaken != predictedTaken) {
                        std::cout << "  Branch misprediction detected\n";
                        
                        std::cout << "  Predicted: "
                                << (predictedTaken ? "taken" : "not taken")
                                << "\n";

                        std::cout << "  Actual: "
                                << (result.branchTaken ? "taken" : "not taken")
                                << "\n";

                        if (result.branchTaken) {
                            pc = result.branchTarget;
                        } else {
                            pc = statusTable[index].staticPc + 1;
                        }

                        std::cout << "  PC redirected to instruction "
                                << pc
                                << "\n";

                        flushActiveInstructions(activeInstructions, index, intFU, mulFU, memFU, statusTable);
                        flushCDBQueue(cdbQueue, index);
                        flushROBQueue(robQueue, rob, index);
                        flushRegProducers(regProducer, index);
                    } else {
                        std::cout << "  Branch prediction correct\n";
                    }
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

    printInstructionStatusTable(statusTable);
}   