#pragma once

#include "RegisterFile.h"
#include "Simulator.h"
#include "ROB.h"
#include <string>
#include <vector>

struct CDBMessage {
    bool valid;
    int producerTag;
    int destinationRegister;
    int value;
    std::string rawText;
};

bool broadcastCDB(
    const CDBMessage& cdb,
    std::vector<ActiveInstruction>& activeInstructions,
    std::vector<ROBEntry>& rob
);