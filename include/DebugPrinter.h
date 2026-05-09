#pragma once

#include "FunctionalUnit.h"
#include "ReservationStation.h"
#include "CDB.h"
#include "Simulator.h"

#include <queue>
#include <vector>

void printFUState(
    const FunctionalUnit& intFU,
    const FunctionalUnit& mulFU,
    const FunctionalUnit& memFU
);

void printActiveInstructions(
    const std::vector<ActiveInstruction>& activeInstructions
);

void printRegisterProducer(
    const std::vector<int>& regProducer
);

void printRSState(
    const std::vector<ActiveInstruction>& activeInstructions,
    int intCapacity,
    int mulCapacity,
    int loadCapacity,
    int storeCapacity
);

void printCDBQueue(std::queue<CDBMessage> cdbQueue);

void printROB(
    const std::queue<int>& robQueueOriginal,
    const std::vector<ROBEntry>& rob
);
