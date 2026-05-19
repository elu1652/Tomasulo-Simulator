#include "Trace.h"

#include <fstream>
#include <iostream>

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

void TraceRecorder::addSnapshot(const TraceSnapshot& snapshot) {
    snapshots.push_back(snapshot);
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
