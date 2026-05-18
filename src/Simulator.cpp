#include "Simulator.h"
#include "FunctionalUnit.h"
#include "ReservationStation.h"
#include "CDB.h"
#include "DebugPrinter.h"
#include "ROB.h"
#include "Flush.h"
#include "BranchPredictor.h"
#include "LSQ.h"
#include "Trace.h"

#include <iostream>
#include <queue>

Simulator::Simulator(BranchPredictorType predictorType): predictorType(predictorType) {

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

static TraceSnapshot makeTraceSnapshot(
    int cycle,
    int pc,
    const std::vector<ActiveInstruction>& activeInstructions,
    const ReorderBuffer& rob,
    const LoadStoreQueue& lsq,
    const std::string& issuedInstruction,
    const std::string& cdbBroadcast,
    const std::string& commitEvent,
    const std::vector<std::string>& events
) {
    TraceSnapshot snapshot;

    snapshot.cycle = cycle;
    snapshot.pc = pc;
    snapshot.issuedInstruction = issuedInstruction;
    snapshot.cdbBroadcast = cdbBroadcast;
    snapshot.commitEvent = commitEvent;
    snapshot.events = events;

    snapshot.robHead = rob.head;
    snapshot.robTail = rob.tail;
    snapshot.robCount = rob.count;

    for (const ActiveInstruction& active : activeInstructions) {
        TraceActiveInstruction traceActive;

        traceActive.instructionId = active.instructionIndex;
        traceActive.robTag = active.robTag;
        traceActive.rawText = active.instr.rawText;
        traceActive.executing = active.executing;
        traceActive.remainingCycles = active.remainingCycles;
        traceActive.waitingReason = active.waitingReason;
        traceActive.vj = active.vj;
        traceActive.vk = active.vk;
        traceActive.qj = active.qj;
        traceActive.qk = active.qk;

        snapshot.activeInstructions.push_back(traceActive);
    }

    for (const ROBEntry& entry : rob.entries) {
        if (!entry.busy) {
            continue;
        }

        TraceROBEntry traceEntry;

        traceEntry.robTag = entry.robTag;
        traceEntry.instructionId = entry.instructionId;
        traceEntry.rawText = entry.rawText;
        traceEntry.busy = entry.busy;
        traceEntry.ready = entry.ready;
        traceEntry.writesRegister = entry.writesRegister;
        traceEntry.destinationRegister = entry.destinationRegister;
        traceEntry.value = entry.value;
        traceEntry.writesMemory = entry.writesMemory;
        traceEntry.memoryAddress = entry.memoryAddress;
        traceEntry.memoryValue = entry.memoryValue;

        snapshot.robEntries.push_back(traceEntry);
    }

    for (const LSQEntry& entry : lsq.entries) {
        if (!entry.busy) {
            continue;
        }

        TraceLSQEntry traceEntry;

        traceEntry.instructionId = entry.instructionId;
        traceEntry.robTag = entry.robTag;
        traceEntry.rawText = entry.rawText;
        traceEntry.busy = entry.busy;
        traceEntry.isLoad = entry.isLoad;
        traceEntry.isStore = entry.isStore;
        traceEntry.addressReady = entry.addressReady;
        traceEntry.address = entry.address;
        traceEntry.valueReady = entry.valueReady;
        traceEntry.value = entry.value;

        snapshot.lsqEntries.push_back(traceEntry);
    }

    return snapshot;
}

static void addTraceEvent(std::vector<std::string>& traceEvents,
                          const std::string& event) {
    traceEvents.push_back(event);
}

// Compute the result of an instruction after it finishes execution.
// Register results are queued for CDB broadcast.
// Loads either read committed memory or use a value forwarded by the LSQ.
// Stores produce an address/value pair that becomes ready in the ROB;
// memory itself is updated only when the store commits.
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
            int value;
            if (active.hasForwardedLoadValue) {
                value = active.forwardedLoadValue;
            } else {
                value = mem.load(address);
            }

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
    const int LOAD_BUFFER_CAPACITY = 2;
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
    // regProducer[R] stores the physical ROB slot that will produce R.
    // -1 means the architectural register file currently has the latest value.
    std::vector<int> regProducer(32, -1);

    std::queue<CDBMessage> cdbQueue;
    
    ReorderBuffer circularROB(ROB_CAPACITY);
    std::vector<ROBEntry>& robEntries = circularROB.entries;

    LoadStoreQueue lsq;

    TraceRecorder traceRecorder;

    // Functional unit initialization
    FunctionalUnit intFU {FUType::INT, 2, 0};
    FunctionalUnit mulFU {FUType::MUL, 2, 0};
    FunctionalUnit memFU {FUType::MEM, 2, 0};

    // Branch predictor used for speculative issue.
    // Change the predictor type here to compare prediction strategies.
    BranchPredictor branchPredictor(predictorType);

    // Main simulation loop.
    // The simulator continues until there are no more instructions to fetch,
    // no active reservation station entries, no pending CDB broadcasts,
    // and no uncommitted ROB entries.
    while (pc < instructions.size() || !activeInstructions.empty() || !cdbQueue.empty() || !circularROB.empty()) {

        std::cout << "\n\nCycle " << cycle << "\n";

        std::string issuedInstructionThisCycle;
        std::string cdbBroadcastThisCycle = "none";
        std::string commitEventThisCycle;
        std::vector<std::string> traceEvents;

        // ISSUE STAGE
        // Issue at most one instruction per cycle in order.
        // Branches use the configured branch predictor to choose the next PC.
        // If predicted taken, issue continues from the branch target;
        // otherwise, issue continues down the fall-through path.
        if (pc < instructions.size()) {

            const Instruction& instrToIssue = instructions[pc];

            RSType rsType = getRSType(instrToIssue.opcode);

            int currentEntries = countRSEntries(activeInstructions, rsType);

            int capacity = getRSCapacity(rsType, INT_RS_CAPACITY, MUL_RS_CAPACITY, LOAD_BUFFER_CAPACITY, STORE_BUFFER_CAPACITY);

            if (circularROB.full()) {
                std::string event =
                    "Issue stalled: " + instructions[pc].rawText + " | ROB full";

                std::cout << event << "\n";
                traceEvents.push_back(event);
            }
            else if (currentEntries >= capacity) {
                std::string event =
                    "Issue stalled: " + instrToIssue.rawText +
                    " | " + rsTypeToString(rsType) + " RS full";

                std::cout << event << "\n";
                traceEvents.push_back(event);
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

                newInstr.robTag = -1;

                // Allocate a physical ROB slot for every issued instruction.
                // Stores and branches also enter the ROB so commit stays in program order.
                int robTag = allocateROB(circularROB, dynamicId, newInstr.instr.rawText);

                if (robTag == -1) {
                    std::cout << "Internal error: ROB allocation failed\n";
                    return;
                }

                newInstr.robTag = robTag;

                // Loads and stores also enter the LSQ so memory ordering can be checked
                // independently from register RAW dependencies.
                if (newInstr.instr.opcode == OpCode::LD) {
                    lsq.addLoad(dynamicId, newInstr.robTag, newInstr.instr.rawText);
                }

                if (newInstr.instr.opcode == OpCode::SD) {
                    lsq.addStore(dynamicId, newInstr.robTag, newInstr.instr.rawText);
                }

                newInstr.isBranch =
                    newInstr.instr.opcode == OpCode::BEQ ||
                    newInstr.instr.opcode == OpCode::BNE;

                if (newInstr.isBranch) {
                    newInstr.predictorStateBefore = branchPredictor.getState(pc);
                    newInstr.predictedTaken = branchPredictor.predict(pc);
                    newInstr.predictedTarget =
                        newInstr.predictedTaken ? newInstr.instr.branchTarget : pc + 1;

                    std::string event =
                        "Branch prediction for I" + std::to_string(dynamicId) +
                        ": " + (newInstr.predictedTaken ? "taken" : "not taken");

                    std::cout << "  " << event << "\n";
                    traceEvents.push_back(event);
                }

                // Source operands are read either from the architectural register file,
                // from a ready ROB entry, or as producer tags if the value is still pending.
                if (newInstr.instr.rs1 != -1) { // Source register 1
                    int producerTag = regProducer[newInstr.instr.rs1];
                    if (producerTag == -1){
                        newInstr.vj = rf.read(newInstr.instr.rs1); // Read value from register file if no producer/RAW dependency
                        newInstr.qj= -1;
                    }
                    else if (robEntries[producerTag].ready && robEntries[producerTag].writesRegister) {
                        newInstr.vj = robEntries[producerTag].value; // Read value from ROB if it's in ROB waiting to be committed
                        newInstr.qj = -1;
                    }
                    else{
                        newInstr.qj = producerTag; // Instruction waits for producer to broadcast
                    }
                }

                if (newInstr.instr.rs2 != -1) { // Source register 2
                    int producerTag = regProducer[newInstr.instr.rs2];
                    if (producerTag == -1){
                        newInstr.vk = rf.read(newInstr.instr.rs2); // Read value from register file if no producer/RAW dependency
                        newInstr.qk= -1;
                    }
                    else if (robEntries[producerTag].ready && robEntries[producerTag].writesRegister) {
                        newInstr.vk = robEntries[producerTag].value; // Read value from ROB if it's in ROB waiting to be committed
                        newInstr.qk = -1;
                    }
                    else{
                        newInstr.qk = producerTag; // Instruction waits for producer to broadcast
                    }
                }

                activeInstructions.push_back(newInstr); // Add new instruction to reservation station

                // Record instruction status
                InstructionStatus status;
                status.staticPc = pc;
                status.rawText = newInstr.instr.rawText;
                status.issueCycle = cycle;
                status.isBranch = newInstr.isBranch;
                status.predictedTaken = newInstr.predictedTaken;
                status.predictorStateBefore = newInstr.predictorStateBefore;
                statusTable.push_back(status);

                // Register-writing instructions become the newest producer for their destination register.
                if(writesRegister(newInstr.instr)){
                    regProducer[newInstr.instr.rd] = newInstr.robTag;
                }

                std::cout << "Issued: " << newInstr.instr.rawText << "\n";     
                issuedInstructionThisCycle = newInstr.instr.rawText;
                traceEvents.push_back("Issued: I" +
                                    std::to_string(dynamicId) +
                                    " " +
                                    newInstr.instr.rawText);
                
                issuedInstructionThisCycle = newInstr.instr.rawText;

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
                // Check for dependencies
                // qj = base register used to compute the memory address
                // qk = store data value
                
                // The store address can be generated as soon as qj is ready,
                // even if the store data qk is still waiting. This lets younger loads
                // compare addresses early and bypass stores to different addresses.
                if (active.instr.opcode == OpCode::SD) {
                    if (active.qj != -1) {
                        active.waitingReason = "store address RAW dependency";
                        continue;
                    }

                    if (!active.memoryAddressComputed) {
                        int address = active.vj + active.instr.immediate;
                        lsq.markAddressReady(active.instructionIndex, address);
                        active.memoryAddressComputed = true;

                        std::cout << "  LSQ Address: I"
                                << active.instructionIndex
                                << " address = "
                                << address
                                << "\n";

                        traceEvents.push_back(
                            "LSQ Address: I" +
                            std::to_string(active.instructionIndex) +
                            " address = " +
                            std::to_string(address)
                        );
                    }

                    if (active.qk != -1) {
                        active.waitingReason = "store value RAW dependency";
                        continue;
                    }
                } else {
                    if(active.qj != -1 || active.qk != -1){
                        active.waitingReason = "RAW dependency";
                        continue;
                    }
                }

                // Loads and stores both record their computed effective address in the LSQ.
                // For stores, this may already have happened above as soon as qj became ready.
                // For loads, the address is recorded before checking older stores.
                if ((active.instr.opcode == OpCode::LD ||
                    active.instr.opcode == OpCode::SD) &&
                    !active.memoryAddressComputed) {

                    int address = active.vj + active.instr.immediate;
                    lsq.markAddressReady(active.instructionIndex, address);
                    active.memoryAddressComputed = true;

                    std::cout << "  LSQ Address: I"
                            << active.instructionIndex
                            << " address = "
                            << address
                            << "\n";
                }

                // LSQ check for loads.
                // A load may execute if all older stores have known different addresses.
                // If an older store has the same address and its value is ready,
                // the load forwards the value directly from the LSQ instead of waiting
                // for the store to commit to memory.
                if (active.instr.opcode == OpCode::LD) {
                    int loadAddress = active.vj + active.instr.immediate;

                    LoadCheckResult loadCheck =
                        lsq.checkLoad(active.instructionIndex, loadAddress);

                    if (!loadCheck.canExecute) {
                        active.waitingReason = loadCheck.reason;
                        continue;
                    }

                    if (loadCheck.shouldForward) {
                        active.hasForwardedLoadValue = true;
                        active.forwardedLoadValue = loadCheck.forwardedValue;

                        std::string event =
                            "LSQ Forward: I" +
                            std::to_string(active.instructionIndex) +
                            " gets value " +
                            std::to_string(loadCheck.forwardedValue) +
                            " from older store";

                        std::cout << "  " << event << "\n";
                        traceEvents.push_back(event);
                    } else {
                        active.hasForwardedLoadValue = false;
                        active.forwardedLoadValue = 0;
                    }
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
                traceEvents.push_back(
                    "Execution started: I" +
                    std::to_string(active.instructionIndex) +
                    " " +
                    active.instr.rawText
                );

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
        printROB(circularROB);

        // COMMIT STAGE
        // Commit happens before this cycle's CDB broadcast.
        // A result that broadcasts this cycle cannot commit until a later cycle.
        int committedInstructionId = commitROB(
            circularROB,
            regProducer,
            rf,
            mem,
            statusTable,
            cycle
        );

        if (committedInstructionId != -1) {
            commitEventThisCycle =
                "Committed I" + std::to_string(committedInstructionId);

            traceEvents.push_back(commitEventThisCycle);
            lsq.removeCommitted(committedInstructionId);
        }

        // WRITEBACK / CDB BROADCAST STAGE
        // Broadcast at most one queued result per cycle.
        if (!cdbQueue.empty()) {
            CDBMessage cdb = cdbQueue.front();
            cdbQueue.pop();

            cdbBroadcastThisCycle =
                "I" + std::to_string(cdb.producerTag) + " " + cdb.rawText;

            std::cout << "CDB Broadcast: " << cdbBroadcastThisCycle << "\n";

            traceEvents.push_back("CDB Broadcast: " + cdbBroadcastThisCycle);

            broadcastCDB(cdb, activeInstructions, robEntries);
            statusTable[cdb.producerTag].writebackCycle = cycle;
            
        } else {
            std::cout << "CDB Broadcast: none\n";
            cdbBroadcastThisCycle = "none";
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
                std::string completeEvent =
                    "Execution complete: I" +
                    std::to_string(index) +
                    " " +
                    activeInstructions[i].instr.rawText;

                std::cout << completeEvent << "\n";
                traceEvents.push_back(completeEvent);

                if (result.writesRegister) {
                    std::cout << "  Result queued for CDB: R" << result.destinationRegister
                            << " = " << result.value << "\n";

                    traceEvents.push_back(
                        "Result queued for CDB: I" +
                        std::to_string(index) +
                        " R" +
                        std::to_string(result.destinationRegister) +
                        " = " +
                        std::to_string(result.value)
                    );

                    CDBMessage cdb;
                    cdb.valid = result.writesRegister;
                    cdb.producerTag = index;
                    cdb.destinationRegister = result.destinationRegister;
                    cdb.value = result.value;
                    cdb.rawText = activeInstructions[i].instr.rawText;
                    cdb.robTag = activeInstructions[i].robTag;     // physical ROB slot

                    cdbQueue.push(cdb);
                }

                if (result.writesMemory) {
                    ROBEntry& entry = robEntries[activeInstructions[i].robTag];

                    entry.ready = true;
                    entry.writesMemory = true;
                    entry.memoryAddress = result.memoryAddress;
                    entry.memoryValue = result.memoryValue;

                    lsq.markStoreReady(index, result.memoryAddress, result.memoryValue);

                    statusTable[index].writebackCycle = cycle;

                    std::cout << "  Store result ready in ROB: Mem["
                            << result.memoryAddress << "] = "
                            << result.memoryValue << "\n";

                    traceEvents.push_back(
                        "Store result ready in ROB: I" +
                        std::to_string(index) +
                        " Mem[" +
                        std::to_string(result.memoryAddress) +
                        "] = " +
                        std::to_string(result.memoryValue)
                    );

                    traceEvents.push_back("No CDB broadcast for store I" +
                                        std::to_string(index));

                    std::cout << "  No CDB broadcast\n";
                }

                if (result.isBranch) {
                    statusTable[index].isBranch = true;
                    statusTable[index].actualTaken = result.branchTaken;
                    statusTable[index].branchResolved = true;

                    ROBEntry& entry = robEntries[activeInstructions[i].robTag];
                    entry.ready = true;

                    statusTable[index].writebackCycle = cycle;

                    std::cout << "  Branch resolved: "
                            << (result.branchTaken ? "taken" : "not taken")
                            << "\n";

                    traceEvents.push_back(
                        "Branch resolved: I" +
                        std::to_string(index) +
                        " " +
                        (result.branchTaken ? "taken" : "not taken")
                    );

                    // Compare the stored prediction made at issue time against the actual branch result.
                    // On a misprediction, redirect the PC and flush all younger wrong-path instructions.
                    bool predictedTaken = activeInstructions[i].predictedTaken;
                    
                    int branchPc = statusTable[index].staticPc;
                    branchPredictor.update(branchPc, result.branchTaken);

                    int predictorStateAfter = branchPredictor.getState(branchPc);
                    activeInstructions[i].predictorStateAfter = predictorStateAfter;
                    statusTable[index].predictorStateAfter = predictorStateAfter;

                    if (result.branchTaken != predictedTaken) {

                        traceEvents.push_back("Branch misprediction detected: I" +
                                            std::to_string(index));

                        traceEvents.push_back(
                            "Predicted: " +
                            std::string(predictedTaken ? "taken" : "not taken")
                        );

                        traceEvents.push_back(
                            "Actual: " +
                            std::string(result.branchTaken ? "taken" : "not taken")
                        );

                        std::cout << "  Branch misprediction detected\n";
                        
                        std::cout << "  Predicted: "
                                << (predictedTaken ? "taken" : "not taken")
                                << "\n";

                        std::cout << "  Actual: "
                                << (result.branchTaken ? "taken" : "not taken")
                                << "\n";

                        if (result.branchTaken) {
                            pc = result.branchTarget;
                            traceEvents.push_back(
                                "PC redirected to instruction " + std::to_string(pc)
                            );
                        } else {
                            pc = statusTable[index].staticPc + 1;
                            traceEvents.push_back(
                                "PC redirected to instruction " + std::to_string(pc)
                            );
                        }

                        std::cout << "  PC redirected to instruction "
                                << pc
                                << "\n";

                        flushActiveInstructions(activeInstructions, index, intFU, mulFU, memFU, statusTable);
                        flushCDBQueue(cdbQueue, index);
                        flushROB(circularROB, index);
                        flushRegProducers(regProducer, circularROB, index);
                        lsq.flushYoungerThan(index);

                        traceEvents.push_back(
                            "Flushed younger instructions after I" +
                            std::to_string(index)
                        );

                    } else {
                        std::cout << "  Branch prediction correct\n";
                        traceEvents.push_back("Branch prediction correct: I" +
                                                std::to_string(index));     
                    }
                    std::cout << "  ROB entry ready\n";
                    traceEvents.push_back("ROB entry ready: I" +
                                            std::to_string(index));
                }

                if (!result.writesRegister && !result.writesMemory && !result.isBranch) {
                    ROBEntry& entry = robEntries[activeInstructions[i].robTag];
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
        traceRecorder.addSnapshot(
            makeTraceSnapshot(
                cycle,
                pc,
                activeInstructions,
                circularROB,
                lsq,
                issuedInstructionThisCycle,
                cdbBroadcastThisCycle,
                commitEventThisCycle,
                traceEvents
            )
        );
        cycle++;
    }

    std::cout << "\nFinal Register State:\n";
    rf.print();

    std::cout << "\nMemory State:\n";
    mem.print();

    printInstructionStatusTable(statusTable);
    printBranchPredictionSummary(statusTable);

    traceRecorder.writeJson("../trace.json");
    std::cout << "\nTrace written to ../trace.json\n";
}   