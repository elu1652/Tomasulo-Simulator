#pragma once

#include "RegisterFile.h"
#include "Simulator.h"
#include "ROB.h"
#include <string>
#include <vector>

#include <queue>

struct CDBMessage {
    bool valid;
    int producerTag;
    int robTag;
    int destinationRegister;
    int value;
    std::string rawText;
};

bool broadcastCDB(
    const CDBMessage& cdb,
    std::vector<ActiveInstruction>& activeInstructions,
    std::vector<ROBEntry>& robEntries
);

void flushCDBQueue(std::queue<CDBMessage>& cdbQueue, int branchIndex);