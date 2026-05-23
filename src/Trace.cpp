#include "Trace.h"

#include <fstream>
#include <iostream>

#include "ArchitectureConfig.h"
#include "Instruction.h"

static std::string escapeJson(const std::string& input) {
    std::string output;

    for (char c : input) {
        switch (c) {
            case '"':
                output += "\\\"";
                break;
            case '\\':
                output += "\\\\";
                break;
            case '\n':
                output += "\\n";
                break;
            case '\r':
                output += "\\r";
                break;
            case '\t':
                output += "\\t";
                break;
            default:
                output += c;
                break;
        }
    }

    return output;
}

static void writeIntArray(std::ofstream& out,
                          const std::string& name,
                          const std::vector<int>& values,
                          bool trailingComma) {
    out << "      \"" << name << "\": [";

    for (size_t i = 0; i < values.size(); i++) {
        if (i > 0) {
            out << ", ";
        }

        out << values[i];
    }

    out << "]";

    if (trailingComma) {
        out << ",";
    }

    out << "\n";
}

static void writePipelineArray(
    std::ofstream& out,
    const std::vector<std::vector<TracePipelineStage>>& pipelines,
    int indentSpaces
) {
    std::string indent(indentSpaces, ' ');
    std::string stageIndent(indentSpaces + 2, ' ');
    std::string entryIndent(indentSpaces + 4, ' ');

    out << "[\n";

    for (size_t pipeIndex = 0; pipeIndex < pipelines.size(); pipeIndex++) {
        const auto& pipeline = pipelines[pipeIndex];

        out << stageIndent << "[\n";

        for (size_t stageIndex = 0; stageIndex < pipeline.size(); stageIndex++) {
            const TracePipelineStage& stage = pipeline[stageIndex];

            out << entryIndent << "{ "
                << "\"occupied\": " << (stage.occupied ? "true" : "false") << ", "
                << "\"instructionId\": ";

            if (stage.occupied) {
                out << stage.instructionId;
            } else {
                out << "null";
            }

            out << " }";

            if (stageIndex + 1 < pipeline.size()) {
                out << ",";
            }

            out << "\n";
        }

        out << stageIndent << "]";

        if (pipeIndex + 1 < pipelines.size()) {
            out << ",";
        }

        out << "\n";
    }

    out << indent << "]";
}

static void writeRSUsage(
    std::ofstream& out,
    const std::string& name,
    const TraceReservationStationUsage& usage,
    bool trailingComma
) {
    out << "        \"" << name << "\": {\n";
    out << "          \"used\": " << usage.used << ",\n";
    out << "          \"capacity\": " << usage.capacity << "\n";
    out << "        }";

    if (trailingComma) {
        out << ",";
    }

    out << "\n";
}

void TraceRecorder::addSnapshot(const TraceSnapshot& snapshot) {
    snapshots.push_back(snapshot);
}

void TraceRecorder::setArchitectureConfig(const ArchitectureConfig& config) {
    architectureConfig = config;
}

void TraceRecorder::setProgram(
    const std::vector<Instruction>& instructions,
    const std::vector<std::string>& setupDirectiveLines
) {
    program.clear();
    program.reserve(instructions.size());

    for (int i = 0; i < static_cast<int>(instructions.size()); i++) {
        TraceProgramEntry entry;
        entry.pc = i;
        entry.text = instructions[i].rawText;
        entry.sourceLine = instructions[i].sourceLine;
        program.push_back(entry);
    }

    setupDirectives = setupDirectiveLines;
}

void TraceRecorder::setInstructionStatus(
    const std::vector<InstructionStatus>& statusTable
) {
    instructionStatus.clear();
    instructionStatus.reserve(statusTable.size());

    for (int i = 0; i < static_cast<int>(statusTable.size()); i++) {
        const InstructionStatus& status = statusTable[i];
        TraceInstructionStatusEntry entry;

        entry.instructionId = i;
        entry.pc = status.staticPc;
        entry.rawText = status.rawText;
        entry.issueCycle = status.issueCycle;
        entry.execStartCycle = status.executeStartCycle;
        entry.execEndCycle = status.executeEndCycle;
        entry.writebackCycle = status.writebackCycle;
        entry.commitCycle = status.commitCycle;
        entry.flushed = status.flushed;
        entry.flushCycle = status.flushCycle;

        instructionStatus.push_back(entry);
    }
}

void TraceRecorder::setPerformanceStats(const PerformanceStats& stats) {
    performanceStats = stats;
}

static void writePerformanceStats(std::ofstream& out, const PerformanceStats& stats) {
    out << "  \"performanceStats\": {\n";
    out << "    \"totalCycles\": " << stats.totalCycles << ",\n";
    out << "    \"committedInstructions\": " << stats.committedInstructions << ",\n";
    out << "    \"ipc\": " << stats.ipc() << ",\n";
    out << "    \"cyclesWithAnyStall\": " << stats.cyclesWithAnyStall << ",\n";
    out << "    \"issueStallCycles\": " << stats.issueStallCycles << ",\n";
    out << "    \"backendStallCycles\": " << stats.backendStallCycles << ",\n";
    out << "    \"totalStallEvents\": " << stats.totalStallEvents << ",\n";
    out << "    \"robFullStallCycles\": " << stats.robFullStallCycles << ",\n";
    out << "    \"rsFullStallCycles\": " << stats.rsFullStallCycles << ",\n";
    out << "    \"rawDependencyStallEvents\": " << stats.rawDependencyStallEvents << ",\n";
    out << "    \"fuBusyStallEvents\": " << stats.fuBusyStallEvents << ",\n";
    out << "    \"memoryOrderingStallEvents\": " << stats.memoryOrderingStallEvents << ",\n";
    out << "    \"cdbBroadcasts\": " << stats.cdbBroadcasts << ",\n";
    out << "    \"cdbQueueMaxSize\": " << stats.cdbQueueMaxSize << ",\n";
    out << "    \"branchCount\": " << stats.branchCount << ",\n";
    out << "    \"branchCorrect\": " << stats.branchCorrect << ",\n";
    out << "    \"branchMispredictions\": " << stats.branchMispredictions << ",\n";
    out << "    \"branchAccuracy\": " << stats.branchAccuracy() << ",\n";
    out << "    \"instructionMix\": {\n";
    out << "      \"int\": " << stats.intInstructionsCommitted << ",\n";
    out << "      \"mul\": " << stats.mulInstructionsCommitted << ",\n";
    out << "      \"fpAdd\": " << stats.fpAddInstructionsCommitted << ",\n";
    out << "      \"fpMul\": " << stats.fpMulInstructionsCommitted << ",\n";
    out << "      \"load\": " << stats.loadInstructionsCommitted << ",\n";
    out << "      \"store\": " << stats.storeInstructionsCommitted << ",\n";
    out << "      \"branch\": " << stats.branchInstructionsCommitted << "\n";
    out << "    },\n";
    out << "    \"maxOccupancy\": {\n";
    out << "      \"rob\": " << stats.robMaxOccupancy << ",\n";
    out << "      \"intRs\": " << stats.intRsMaxOccupancy << ",\n";
    out << "      \"mulRs\": " << stats.mulRsMaxOccupancy << ",\n";
    out << "      \"fpAddRs\": " << stats.fpAddRsMaxOccupancy << ",\n";
    out << "      \"fpMulRs\": " << stats.fpMulRsMaxOccupancy << ",\n";
    out << "      \"loadBuffer\": " << stats.loadBufferMaxOccupancy << ",\n";
    out << "      \"storeBuffer\": " << stats.storeBufferMaxOccupancy << ",\n";
    out << "      \"fpAddPipeline\": " << stats.fpAddPipelineMaxOccupancy << ",\n";
    out << "      \"fpMulPipeline\": " << stats.fpMulPipelineMaxOccupancy << "\n";
    out << "    }\n";
    out << "  }";
}

static void writeArchitectureConfig(
    std::ofstream& out,
    const ArchitectureConfig& config
) {
    // Export the exact per-run config used by Simulator so the frontend never
    // has to infer capacities from project defaults or hardcoded fallbacks.
    out << "  \"architectureConfig\": {\n";
    out << "    \"robCapacity\": " << config.robCapacity << ",\n";
    out << "    \"intRsCapacity\": " << config.intRsCapacity << ",\n";
    out << "    \"mulRsCapacity\": " << config.mulRsCapacity << ",\n";
    out << "    \"fpAddRsCapacity\": " << config.fpAddRsCapacity << ",\n";
    out << "    \"fpMulRsCapacity\": " << config.fpMulRsCapacity << ",\n";
    out << "    \"loadBufferCapacity\": " << config.loadBufferCapacity << ",\n";
    out << "    \"storeBufferCapacity\": " << config.storeBufferCapacity << ",\n";
    out << "    \"intFuCount\": " << config.intFuCount << ",\n";
    out << "    \"mulFuCount\": " << config.mulFuCount << ",\n";
    out << "    \"memFuCount\": " << config.memFuCount << ",\n";
    out << "    \"fpAddPipelineCount\": " << config.fpAddPipelineCount << ",\n";
    out << "    \"fpAddPipelineDepth\": " << config.fpAddPipelineDepth << ",\n";
    out << "    \"fpMulPipelineCount\": " << config.fpMulPipelineCount << ",\n";
    out << "    \"fpMulPipelineDepth\": " << config.fpMulPipelineDepth << ",\n";
    out << "    \"intLatency\": " << config.intLatency << ",\n";
    out << "    \"mulLatency\": " << config.mulLatency << ",\n";
    out << "    \"loadLatency\": " << config.loadLatency << ",\n";
    out << "    \"storeLatency\": " << config.storeLatency << ",\n";
    out << "    \"fpAddLatency\": " << config.fpAddLatency << ",\n";
    out << "    \"fpMulLatency\": " << config.fpMulLatency << ",\n";
    out << "    \"fpDivLatency\": " << config.fpDivLatency << "\n";
    out << "  }";
}

// Keep trace serialization in one place so new visualizer fields can be added
// without changing simulator timing or state mutation order.
void TraceRecorder::writeJson(const std::string& filename) const {
    std::ofstream out(filename);

    if (!out.is_open()) {
        std::cerr << "Error: could not write trace file: "
                  << filename
                  << "\n";
        return;
    }


    out << "{\n";
    writePerformanceStats(out, performanceStats);
    out << ",\n";
    writeArchitectureConfig(out, architectureConfig);
    out << ",\n";
    out << "  \"program\": [\n";

    for (size_t i = 0; i < program.size(); i++) {
        const TraceProgramEntry& entry = program[i];

        out << "    {\n";
        out << "      \"pc\": " << entry.pc << ",\n";
        out << "      \"text\": \"" << escapeJson(entry.text) << "\",\n";
        out << "      \"sourceLine\": " << entry.sourceLine << "\n";
        out << "    }";

        if (i + 1 < program.size()) {
            out << ",";
        }

        out << "\n";
    }

    out << "  ],\n";
    out << "  \"setupDirectives\": [\n";

    for (size_t i = 0; i < setupDirectives.size(); i++) {
        out << "    \"" << escapeJson(setupDirectives[i]) << "\"";

        if (i + 1 < setupDirectives.size()) {
            out << ",";
        }

        out << "\n";
    }

    out << "  ],\n";
    out << "  \"instructionStatus\": [\n";

    for (size_t i = 0; i < instructionStatus.size(); i++) {
        const TraceInstructionStatusEntry& status = instructionStatus[i];

        out << "    {\n";
        out << "      \"instructionId\": " << status.instructionId << ",\n";
        out << "      \"pc\": " << status.pc << ",\n";
        out << "      \"rawText\": \"" << escapeJson(status.rawText) << "\",\n";
        out << "      \"issueCycle\": " << status.issueCycle << ",\n";
        out << "      \"execStartCycle\": " << status.execStartCycle << ",\n";
        out << "      \"execEndCycle\": " << status.execEndCycle << ",\n";
        out << "      \"writebackCycle\": " << status.writebackCycle << ",\n";
        out << "      \"commitCycle\": " << status.commitCycle << ",\n";
        out << "      \"flushed\": " << (status.flushed ? "true" : "false") << ",\n";
        out << "      \"flushCycle\": " << status.flushCycle << "\n";
        out << "    }";

        if (i + 1 < instructionStatus.size()) {
            out << ",";
        }

        out << "\n";
    }

    out << "  ],\n";
    out << "  \"cycles\": [\n";

    for (size_t i = 0; i < snapshots.size(); i++) {
        const TraceSnapshot& s = snapshots[i];

        out << "    {\n";
        out << "      \"cycle\": " << s.cycle << ",\n";
        out << "      \"pc\": " << s.pc << ",\n";
        out << "      \"predictorType\": \"" << escapeJson(s.predictorType) << "\",\n";
        out << "      \"issuedInstruction\": \"" << escapeJson(s.issuedInstruction) << "\",\n";
        out << "      \"cdbBroadcast\": \"" << escapeJson(s.cdbBroadcast) << "\",\n";
        out << "      \"commitEvent\": \"" << escapeJson(s.commitEvent) << "\",\n";
        writeIntArray(out, "registers", s.registers, true);
        writeIntArray(out, "memory", s.memory, true);

        out << "      \"rob\": {\n";
        out << "        \"head\": " << s.robHead << ",\n";
        out << "        \"tail\": " << s.robTail << ",\n";
        out << "        \"count\": " << s.robCount << ",\n";
        out << "        \"capacity\": " << s.robCapacity << ",\n";
        out << "        \"entries\": [\n";

        for (size_t j = 0; j < s.robEntries.size(); j++) {
            const TraceROBEntry& e = s.robEntries[j];

            out << "          {\n";
            out << "            \"robTag\": " << e.robTag << ",\n";
            out << "            \"instructionId\": " << e.instructionId << ",\n";
            out << "            \"rawText\": \"" << escapeJson(e.rawText) << "\",\n";
            out << "            \"busy\": " << (e.busy ? "true" : "false") << ",\n";
            out << "            \"ready\": " << (e.ready ? "true" : "false") << ",\n";
            out << "            \"writesRegister\": " << (e.writesRegister ? "true" : "false") << ",\n";
            out << "            \"destinationRegister\": " << e.destinationRegister << ",\n";
            out << "            \"value\": " << e.value << ",\n";
            out << "            \"writesMemory\": " << (e.writesMemory ? "true" : "false") << ",\n";
            out << "            \"memoryAddress\": " << e.memoryAddress << ",\n";
            out << "            \"memoryValue\": " << e.memoryValue << "\n";
            out << "          }";

            if (j + 1 < s.robEntries.size()) {
                out << ",";
            }

            out << "\n";
        }

        out << "        ]\n";
        out << "      },\n";

        out << "      \"activeInstructions\": [\n";

        for (size_t j = 0; j < s.activeInstructions.size(); j++) {
            const TraceActiveInstruction& a = s.activeInstructions[j];

            out << "        {\n";
            out << "          \"instructionId\": " << a.instructionId << ",\n";
            out << "          \"robTag\": " << a.robTag << ",\n";
            out << "          \"rawText\": \"" << escapeJson(a.rawText) << "\",\n";
            out << "          \"executing\": " << (a.executing ? "true" : "false") << ",\n";
            out << "          \"remainingCycles\": " << a.remainingCycles << ",\n";
            out << "          \"waitingReason\": \"" << escapeJson(a.waitingReason) << "\",\n";
            out << "          \"vj\": " << a.vj << ",\n";
            out << "          \"vk\": " << a.vk << ",\n";
            out << "          \"qj\": " << a.qj << ",\n";
            out << "          \"qk\": " << a.qk << "\n";
            out << "        }";

            if (j + 1 < s.activeInstructions.size()) {
                out << ",";
            }

            out << "\n";
        }

        out << "      ],\n";

        out << "      \"lsq\": [\n";

        for (size_t j = 0; j < s.lsqEntries.size(); j++) {
            const TraceLSQEntry& l = s.lsqEntries[j];

            out << "        {\n";
            out << "          \"instructionId\": " << l.instructionId << ",\n";
            out << "          \"robTag\": " << l.robTag << ",\n";
            out << "          \"rawText\": \"" << escapeJson(l.rawText) << "\",\n";
            out << "          \"busy\": " << (l.busy ? "true" : "false") << ",\n";
            out << "          \"isLoad\": " << (l.isLoad ? "true" : "false") << ",\n";
            out << "          \"isStore\": " << (l.isStore ? "true" : "false") << ",\n";
            out << "          \"addressReady\": " << (l.addressReady ? "true" : "false") << ",\n";
            out << "          \"address\": " << l.address << ",\n";
            out << "          \"valueReady\": " << (l.valueReady ? "true" : "false") << ",\n";
            out << "          \"value\": " << l.value << "\n";
            out << "        }";

            if (j + 1 < s.lsqEntries.size()) {
                out << ",";
            }

            out << "\n";
        }

        out << "      ],\n";

        out << "      \"registerProducers\": [\n";

        for (size_t j = 0; j < s.registerProducers.size(); j++) {
            const TraceRegisterProducer& p = s.registerProducers[j];

            out << "        {\n";
            out << "          \"register\": " << p.registerNumber << ",\n";
            out << "          \"robTag\": " << p.robTag << "\n";
            out << "        }";

            if (j + 1 < s.registerProducers.size()) {
                out << ",";
            }

            out << "\n";
        }

        out << "      ],\n";

        out << "      \"predictorState\": {\n";
        out << "        \"predictorType\": \""
            << escapeJson(s.predictorState.predictorType)
            << "\",\n";
        out << "        \"globalHistory\": "
            << s.predictorState.globalHistory
            << ",\n";
        out << "        \"globalHistoryBits\": "
            << s.predictorState.globalHistoryBits
            << ",\n";
        out << "        \"globalHistoryText\": \""
            << escapeJson(s.predictorState.globalHistoryText)
            << "\",\n";
        out << "        \"entries\": [\n";

        for (size_t j = 0; j < s.predictorState.entries.size(); j++) {
            const TracePredictorStateEntry& e = s.predictorState.entries[j];

            out << "          {\n";
            out << "            \"index\": " << e.index << ",\n";
            out << "            \"state\": " << e.state << ",\n";
            out << "            \"stateBits\": \""
                << escapeJson(e.stateBits)
                << "\",\n";
            out << "            \"stateText\": \""
                << escapeJson(e.stateText)
                << "\",\n";
            out << "            \"prediction\": \""
                << escapeJson(e.prediction)
                << "\"\n";
            out << "          }";

            if (j + 1 < s.predictorState.entries.size()) {
                out << ",";
            }

            out << "\n";
        }

        out << "        ]\n";
        out << "      },\n";

        out << "      \"fuPipelines\": {\n";
        out << "        \"FP_ADD\": ";
        writePipelineArray(out, s.fuPipelines.fpAdd, 8);
        out << ",\n";
        out << "        \"FP_MUL\": ";
        writePipelineArray(out, s.fuPipelines.fpMul, 8);
        out << "\n";
        out << "      },\n";

        out << "      \"rsState\": {\n";
        writeRSUsage(out, "INT", s.rsState.intRS, true);
        writeRSUsage(out, "MUL", s.rsState.mulRS, true);
        writeRSUsage(out, "FP_ADD", s.rsState.fpAddRS, true);
        writeRSUsage(out, "FP_MUL", s.rsState.fpMulRS, true);
        writeRSUsage(out, "LOAD", s.rsState.loadBuffer, true);
        writeRSUsage(out, "STORE", s.rsState.storeBuffer, false);
        out << "      },\n";

        out << "      \"branchPredictions\": [\n";

        for (size_t j = 0; j < s.branchPredictions.size(); j++) {
            const TraceBranchPredictionEntry& b = s.branchPredictions[j];

            out << "        {\n";
            out << "          \"instructionId\": " << b.instructionId << ",\n";
            out << "          \"pc\": " << b.pc << ",\n";
            out << "          \"instruction\": \"" << escapeJson(b.instruction) << "\",\n";
            out << "          \"predictorType\": \"" << escapeJson(b.predictorType) << "\",\n";
            out << "          \"predictedTaken\": " << (b.predictedTaken ? "true" : "false") << ",\n";
            out << "          \"actualTaken\": " << (b.actualTaken ? "true" : "false") << ",\n";
            out << "          \"branchResolved\": " << (b.branchResolved ? "true" : "false") << ",\n";
            out << "          \"resolvedThisCycle\": " << (b.resolvedThisCycle ? "true" : "false") << ",\n";
            out << "          \"predictionCorrect\": " << (b.predictionCorrect ? "true" : "false") << ",\n";
            out << "          \"targetPc\": " << b.targetPc << ",\n";
            out << "          \"fallthroughPc\": " << b.fallthroughPc << ",\n";
            out << "          \"stateBefore\": " << b.stateBefore << ",\n";
            out << "          \"stateAfter\": " << b.stateAfter << ",\n";
            out << "          \"stateBeforeText\": \"" << escapeJson(b.stateBeforeText) << "\",\n";
            out << "          \"stateAfterText\": \"" << escapeJson(b.stateAfterText) << "\",\n";
            out << "          \"globalHistoryBefore\": " << b.globalHistoryBefore << ",\n";
            out << "          \"globalHistoryAfter\": " << b.globalHistoryAfter << ",\n";
            out << "          \"gshareIndex\": " << b.gshareIndex << ",\n";
            out << "          \"counterBefore\": " << b.counterBefore << ",\n";
            out << "          \"counterAfter\": " << b.counterAfter << "\n";
            out << "        }";

            if (j + 1 < s.branchPredictions.size()) {
                out << ",";
            }

            out << "\n";
        }

        out << "      ],\n";

        out << "      \"events\": [\n";

        for (size_t j = 0; j < s.events.size(); j++) {
            out << "        \"" << escapeJson(s.events[j]) << "\"";

            if (j + 1 < s.events.size()) {
                out << ",";
            }

            out << "\n";
        }

        out << "      ]\n";
        out << "    }";

        if (i + 1 < snapshots.size()) {
            out << ",";
        }

        out << "\n";
    }

    out << "  ]\n";
    out << "}\n";
}
