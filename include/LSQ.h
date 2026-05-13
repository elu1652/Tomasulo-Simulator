#pragma once

#include "Instruction.h"

#include <vector>
#include <string>

struct LSQEntry {
    bool busy = false;

    bool isLoad = false;
    bool isStore = false;

    int instructionId = -1; // dynamic I#
    int robTag = -1;        // physical ROB#

    bool addressReady = false;
    int address = 0;

    bool valueReady = false; // mostly for stores
    int value = 0;

    std::string rawText;
};

struct LoadCheckResult {
    bool canExecute = true;
    bool shouldForward = false;
    int forwardedValue = 0;
    std::string reason;
};

struct LoadStoreQueue {
    std::vector<LSQEntry> entries;

    void addLoad(int instructionId, int robTag, const std::string& rawText);
    void addStore(int instructionId, int robTag, const std::string& rawText);

    bool hasOlderStore(int instructionId) const;

    void markStoreReady(int instructionId, int address, int value);

    void removeCommitted(int instructionId);
    void flushYoungerThan(int branchInstructionId);

    LoadCheckResult checkLoad(int instructionId, int loadAddress) const;
};