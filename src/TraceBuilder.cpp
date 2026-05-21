#include "TraceBuilder.h"

#include "BranchTraceUtils.h"
#include "BranchPredictor.h"
#include "FunctionalUnit.h"
#include "InstructionStatus.h"
#include "LSQ.h"
#include "Memory.h"
#include "ROB.h"
#include "RegisterFile.h"
#include "ReservationStation.h"
#include "Simulator.h"
#include "Trace.h"

static std::vector<std::vector<TracePipelineStage>> makeTracePipeline(
    const FunctionalUnit& fu
) {
    std::vector<std::vector<TracePipelineStage>> tracePipeline;

    if (!fu.pipelined) {
        return tracePipeline;
    }

    for (const auto& pipeline : fu.pipelines) {
        std::vector<TracePipelineStage> traceStages;

        for (const auto& stage : pipeline) {
            TracePipelineStage traceStage;
            traceStage.occupied = stage.occupied;
            traceStage.instructionId = stage.instructionId;

            traceStages.push_back(traceStage);
        }

        tracePipeline.push_back(traceStages);
    }

    return tracePipeline;
}

static TraceReservationStationUsage makeTraceRSUsage(
    const std::vector<ActiveInstruction>& activeInstructions,
    RSType type,
    int capacity
) {
    TraceReservationStationUsage usage;
    usage.used = countRSEntries(activeInstructions, type);
    usage.capacity = capacity;

    return usage;
}

TraceSnapshot makeTraceSnapshot(
    int cycle,
    int pc,
    BranchPredictorType predictorType,
    const BranchPredictor& branchPredictor,
    const std::vector<ActiveInstruction>& activeInstructions,
    const ReorderBuffer& rob,
    const LoadStoreQueue& lsq,
    const std::vector<int>& regProducer,
    const std::vector<InstructionStatus>& statusTable,
    const RegisterFile& rf,
    const Memory& mem,
    const FunctionalUnit& fpAddFU,
    const FunctionalUnit& fpMulFU,
    int intRSCapacity,
    int mulRSCapacity,
    int fpAddRSCapacity,
    int fpMulRSCapacity,
    int loadBufferCapacity,
    int storeBufferCapacity,
    const std::string& issuedInstruction,
    const std::string& cdbBroadcast,
    const std::string& commitEvent,
    const std::vector<std::string>& events
) {
    TraceSnapshot snapshot;

    snapshot.cycle = cycle;
    snapshot.pc = pc;
    snapshot.predictorType = branchPredictorTypeToString(predictorType);
    snapshot.predictorState = makeTracePredictorState(
        branchPredictor,
        predictorType
    );
    snapshot.issuedInstruction = issuedInstruction;
    snapshot.cdbBroadcast = cdbBroadcast;
    snapshot.commitEvent = commitEvent;
    snapshot.events = events;
    snapshot.registers = rf.snapshot(32);
    snapshot.memory = mem.snapshot(32);
    snapshot.fuPipelines.fpAdd = makeTracePipeline(fpAddFU);
    snapshot.fuPipelines.fpMul = makeTracePipeline(fpMulFU);
    snapshot.rsState.intRS = makeTraceRSUsage(
        activeInstructions,
        RSType::INT,
        intRSCapacity
    );
    snapshot.rsState.mulRS = makeTraceRSUsage(
        activeInstructions,
        RSType::MUL,
        mulRSCapacity
    );
    snapshot.rsState.fpAddRS = makeTraceRSUsage(
        activeInstructions,
        RSType::FP_ADD,
        fpAddRSCapacity
    );
    snapshot.rsState.fpMulRS = makeTraceRSUsage(
        activeInstructions,
        RSType::FP_MUL,
        fpMulRSCapacity
    );
    snapshot.rsState.loadBuffer = makeTraceRSUsage(
        activeInstructions,
        RSType::LOAD,
        loadBufferCapacity
    );
    snapshot.rsState.storeBuffer = makeTraceRSUsage(
        activeInstructions,
        RSType::STORE,
        storeBufferCapacity
    );

    snapshot.robHead = rob.head;
    snapshot.robTail = rob.tail;
    snapshot.robCount = rob.count;

    for (int reg = 0; reg < static_cast<int>(regProducer.size()); reg++) {
        if (regProducer[reg] == -1) {
            continue;
        }

        TraceRegisterProducer producer;

        producer.registerNumber = reg;
        producer.robTag = regProducer[reg];

        snapshot.registerProducers.push_back(producer);
    }

    for (int i = 0; i < static_cast<int>(statusTable.size()); i++) {
        const InstructionStatus& status = statusTable[i];

        if (!status.isBranch) {
            continue;
        }

        TraceBranchPredictionEntry branch;

        branch.instructionId = i;
        branch.pc = status.staticPc;
        branch.instruction = status.rawText;
        branch.predictorType = branchPredictorTypeToString(predictorType);
        branch.predictedTaken = status.predictedTaken;
        branch.actualTaken = status.actualTaken;
        branch.branchResolved = status.branchResolved;
        branch.resolvedThisCycle = status.branchResolved &&
                                   status.writebackCycle == cycle;
        branch.predictionCorrect = status.branchResolved &&
                                   status.predictedTaken == status.actualTaken;
        branch.targetPc = status.targetPc;
        branch.fallthroughPc = status.fallthroughPc;
        branch.stateBefore = status.predictorStateBefore;
        branch.stateAfter = status.predictorStateAfter;
        branch.stateBeforeText = branchPredictorStateText(
            predictorType,
            status.predictorStateBefore
        );
        branch.stateAfterText = status.branchResolved
            ? branchPredictorStateText(predictorType, status.predictorStateAfter)
            : "pending";
        branch.globalHistoryBefore = status.gshareGlobalHistoryBefore;
        branch.globalHistoryAfter = status.gshareGlobalHistoryAfter;
        branch.gshareIndex = status.gshareIndex;
        branch.counterBefore = status.gshareCounterBefore;
        branch.counterAfter = status.gshareCounterAfter;

        snapshot.branchPredictions.push_back(branch);
    }

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
