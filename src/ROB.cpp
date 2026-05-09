#include "ROB.h"
#include "RegisterFile.h"
#include "Memory.h"
#include "InstructionStatus.h"
#include <queue>


void commitROB(
    std::queue<int>& robQueue,
    std::vector<ROBEntry>& rob,
    std::vector<int>& regProducer,
    RegisterFile& rf,
    Memory& mem,
    std::vector<InstructionStatus>& statusTable,
    int cycle
) {
    if (robQueue.empty()) {
        std::cout << "ROB Commit: none\n";
        return;
    }

    int tag = robQueue.front();
    ROBEntry& entry = rob[tag];

    if (!entry.ready) {
        std::cout << "ROB Commit: stalled at I" << tag
                  << " not ready\n";
        return;
    }

    std::cout << "ROB Commit: I" << tag
              << " " << entry.rawText << "\n";

    if (entry.writesRegister) {
        rf.write(entry.destinationRegister, entry.value);

        std::cout << "  RF Commit: R" << entry.destinationRegister
                  << " = " << entry.value << "\n";

        if (regProducer[entry.destinationRegister] == tag) {
            regProducer[entry.destinationRegister] = -1;
        }
    }

    if (entry.writesMemory) {
        mem.store(entry.memoryAddress, entry.memoryValue);

        std::cout << "  Memory Commit: Mem[" << entry.memoryAddress
                  << "] = " << entry.memoryValue << "\n";
    }

    statusTable[tag].commitCycle = cycle;

    entry.busy = false;
    robQueue.pop();
}