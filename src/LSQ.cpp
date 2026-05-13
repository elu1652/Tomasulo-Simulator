#include "LSQ.h"

void LoadStoreQueue::addLoad(
    int instructionId,
    int robTag,
    const std::string& rawText
) {
    LSQEntry entry;
    entry.busy = true;
    entry.isLoad = true;
    entry.isStore = false;
    entry.instructionId = instructionId;
    entry.robTag = robTag;
    entry.rawText = rawText;

    entries.push_back(entry);
}

void LoadStoreQueue::addStore(
    int instructionId,
    int robTag,
    const std::string& rawText
) {
    LSQEntry entry;
    entry.busy = true;
    entry.isLoad = false;
    entry.isStore = true;
    entry.instructionId = instructionId;
    entry.robTag = robTag;
    entry.rawText = rawText;

    entries.push_back(entry);
}

bool LoadStoreQueue::hasOlderStore(int instructionId) const {
    for (const LSQEntry& entry : entries) {
        if (!entry.busy) {
            continue;
        }

        if (entry.instructionId >= instructionId) {
            break;
        }

        if (entry.isStore) {
            return true;
        }
    }

    return false;
}

void LoadStoreQueue::markStoreReady(
    int instructionId,
    int address,
    int value
) {
    for (LSQEntry& entry : entries) {
        if (entry.busy && entry.instructionId == instructionId) {
            entry.addressReady = true;
            entry.address = address;
            entry.valueReady = true;
            entry.value = value;
            return;
        }
    }
}

void LoadStoreQueue::removeCommitted(int instructionId) {
    for (LSQEntry& entry : entries) {
        if (entry.busy && entry.instructionId == instructionId) {
            entry.busy = false;
            return;
        }
    }
}

void LoadStoreQueue::flushYoungerThan(int branchInstructionId) {
    for (LSQEntry& entry : entries) {
        if (entry.busy && entry.instructionId > branchInstructionId) {
            entry.busy = false;
        }
    }
}