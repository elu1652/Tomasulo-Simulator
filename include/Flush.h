#pragma once

#include "InstructionStatus.h"
#include "FunctionalUnit.h"
#include "ReservationStation.h"

#include <vector>

void flushActiveInstructions(
    std::vector<ActiveInstruction>& activeInstructions,
    int branchIndex,
    FunctionalUnit& intFU,
    FunctionalUnit& mulFU,
    FunctionalUnit& memFU,
    std::vector<InstructionStatus>& statusTable
);

void flushRegProducers(std::vector<int>& regProducer, int branchIndex);