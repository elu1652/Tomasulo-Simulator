#include "ROB.h"
#include "RegisterFile.h"
#include "Memory.h"
#include "InstructionStatus.h"
#include <iostream>


int commitROB(
    ReorderBuffer& rob,
    std::vector<int>& regProducer,
    RegisterFile& rf,
    Memory& mem,
    std::vector<InstructionStatus>& statusTable,
    int cycle
) {
    if (rob.empty()) {
        std::cout << "ROB Commit: none\n";
        return -1;
    }

    int tag = rob.head;
    ROBEntry& entry = rob.entries[tag];

    if (!entry.ready) {
        std::cout << "ROB Commit: stalled at I"
                  << entry.instructionId
                  << " not ready\n";
        return -1;
    }

    std::cout << "ROB Commit: I" << entry.instructionId
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

    statusTable[entry.instructionId].commitCycle = cycle;

    int committedInstructionId = entry.instructionId;

    entry = ROBEntry{};

    rob.head = (rob.head + 1) % rob.capacity();
    rob.count--;

    return committedInstructionId;
}

void flushROB(
    ReorderBuffer& rob,
    int branchIndex
) {
    if (rob.empty()) {
        return;
    }

    int keptCount = 0;

    for (int i = 0; i < rob.count; i++) {
        int slot = (rob.head + i) % rob.capacity();
        ROBEntry& entry = rob.entries[slot];

        if (entry.busy && entry.instructionId > branchIndex) {
            std::cout << "  Flushed ROB: I"
                      << entry.instructionId
                      << " "
                      << entry.rawText
                      << "\n";

            entry = ROBEntry{};
        } else {
            keptCount++;
        }
    }

    rob.count = keptCount;
    rob.tail = (rob.head + rob.count) % rob.capacity();
}

int allocateROB(ReorderBuffer& rob, int instructionId, const std::string& rawText) {
    if (rob.full()) return -1;

    int tag = rob.tail;
    ROBEntry& entry = rob.entries[tag];

    entry = ROBEntry{};
    entry.busy = true;
    entry.ready = false;
    entry.robTag = tag;
    entry.instructionId = instructionId;
    entry.rawText = rawText;

    rob.tail = (rob.tail + 1) % rob.capacity();
    rob.count++;

    return tag;
}