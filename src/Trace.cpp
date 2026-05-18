#include "Trace.h"

#include <fstream>

# include <iostream>

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

void TraceRecorder::addSnapshot(const TraceSnapshot& snapshot) {
    snapshots.push_back(snapshot);
}

void TraceRecorder::writeJson(const std::string& filename) const {
    std::ofstream out(filename);

    if (!out.is_open()) {
        std::cerr << "Error: could not write trace file: "
                  << filename
                  << "\n";
        return;
    }


    out << "{\n";
    out << "  \"cycles\": [\n";

    for (size_t i = 0; i < snapshots.size(); i++) {
        const TraceSnapshot& s = snapshots[i];

        out << "    {\n";
        out << "      \"cycle\": " << s.cycle << ",\n";
        out << "      \"pc\": " << s.pc << ",\n";
        out << "      \"issuedInstruction\": \"" << escapeJson(s.issuedInstruction) << "\",\n";
        out << "      \"cdbBroadcast\": \"" << escapeJson(s.cdbBroadcast) << "\",\n";
        out << "      \"commitEvent\": \"" << escapeJson(s.commitEvent) << "\",\n";

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