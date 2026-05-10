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