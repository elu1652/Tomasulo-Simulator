#pragma once

#include "BranchPredictor.h"
#include "Trace.h"

#include <string>

std::string formatBinary(int value, int bits);

TracePredictorState makeTracePredictorState(
    const BranchPredictor& branchPredictor,
    BranchPredictorType predictorType
);
