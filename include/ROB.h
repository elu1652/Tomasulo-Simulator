#pragma once

#include "RegisterFile.h"
#include "Memory.h"
#include "InstructionStatus.h"

#include <queue>
#include <vector>
#include <string>

struct ROBEntry {
    bool busy = false;
    bool ready = false;

    int tag = -1;
    std::string rawText;

    bool writesRegister = false;
    int destinationRegister = -1;
    int value = 0;

    bool writesMemory = false;
    int memoryAddress = -1;
    int memoryValue = 0;
};

void commitROB(
    std::queue<int>& robQueue,
    std::vector<ROBEntry>& rob,
    std::vector<int>& regProducer,
    RegisterFile& rf,
    Memory& mem,
    std::vector<InstructionStatus>& statusTable,
    int cycle
);

void flushROBQueue(
    std::queue<int>& robQueue,
    std::vector<ROBEntry>& rob,
    int branchIndex
);