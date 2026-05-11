#pragma once

#include "RegisterFile.h"
#include "Memory.h"
#include "InstructionStatus.h"

#include <vector>
#include <string>

struct ROBEntry {
    bool busy = false;
    bool ready = false;

    int robTag = -1;          // physical slot
    int instructionId = -1;   // dynamic ID / statusTable index

    std::string rawText;

    bool writesRegister = false;
    int destinationRegister = -1;
    int value = 0;

    bool writesMemory = false;
    int memoryAddress = -1;
    int memoryValue = 0;
};

struct ReorderBuffer {
    std::vector<ROBEntry> entries;
    int head = 0;
    int tail = 0;
    int count = 0;

    explicit ReorderBuffer(int capacity)
        : entries(capacity) {}

    bool full() const {
        return count == entries.size();
    }

    bool empty() const {
        return count == 0;
    }

    int capacity() const {
        return entries.size();
    }
};

void commitROB(
    ReorderBuffer& rob,
    std::vector<int>& regProducer,
    RegisterFile& rf,
    Memory& mem,
    std::vector<InstructionStatus>& statusTable,
    int cycle
);

void flushROB(
    ReorderBuffer& rob,
    int branchIndex
);

int allocateROB(
    ReorderBuffer& rob,
    int instructionId,
    const std::string& rawText
);