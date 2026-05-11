#include "Flush.h"

#include <iostream>

void flushActiveInstructions(
    std::vector<ActiveInstruction>& activeInstructions,
    int branchIndex,
    FunctionalUnit& intFU,
    FunctionalUnit& mulFU,
    FunctionalUnit& memFU,
    std::vector<InstructionStatus>& statusTable
) {
    for (int i = 0; i < activeInstructions.size(); ) {
        if (activeInstructions[i].instructionIndex > branchIndex) {
            int flushedId = activeInstructions[i].instructionIndex;
            statusTable[flushedId].flushed = true;

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

void flushRegProducers(
    std::vector<int>& regProducer,
    const ReorderBuffer& rob,
    int branchIndex
) {
    for (int reg = 0; reg < static_cast<int>(regProducer.size()); reg++) {
        int tag = regProducer[reg];

        if (tag == -1) {
            continue;
        }

        if (tag < 0 || tag >= rob.capacity()) {
            regProducer[reg] = -1;
            continue;
        }

        const ROBEntry& entry = rob.entries[tag];

        if (!entry.busy || entry.instructionId > branchIndex) {
            std::cout << "  Cleared producer: R"
                      << reg
                      << " <- ROB"
                      << tag
                      << "\n";

            regProducer[reg] = -1;
        }
    }
}