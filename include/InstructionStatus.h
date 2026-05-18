#pragma once

#include <string>

struct InstructionStatus {
    int staticPc = -1;
    std::string rawText;

    int issueCycle = -1;
    int executeStartCycle = -1;
    int executeEndCycle = -1;
    int writebackCycle = -1;
    int commitCycle = -1;

    bool flushed = false;
    int flushCycle = -1;

    bool isBranch = false;
    bool predictedTaken = false;
    bool actualTaken = false;
    bool branchResolved = false;

    int targetPc = -1;
    int fallthroughPc = -1;

    int predictorStateBefore = -1;
    int predictorStateAfter = -1;
};
