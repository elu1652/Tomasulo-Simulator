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

LoadCheckResult LoadStoreQueue::checkLoad(
    int instructionId,
    int loadAddress
) const {
    LoadCheckResult result;

    bool foundForwardingStore = false;
    int forwardedValue = 0;

    for (const LSQEntry& entry : entries) {
        if (!entry.busy) {
            continue;
        }

        // Only inspect older memory operations.
        if (entry.instructionId >= instructionId) {
            break;
        }

        if (!entry.isStore) {
            continue;
        }

        // Older store address unknown:
        // load must wait because it might be the same address.
        if (!entry.addressReady) {
            result.canExecute = false;
            result.reason = "older store address pending";
            return result;
        }

        // Older store has a different address:
        // no memory dependency with this store.
        if (entry.address != loadAddress) {
            continue;
        }

        // Older store has same address but no value yet:
        // load must wait.
        if (!entry.valueReady) {
            result.canExecute = false;
            result.reason = "older store value pending";
            return result;
        }

        // Older store has same address and value is ready:
        // forward from it. Keep scanning in case there is a younger
        // older store to the same address.
        foundForwardingStore = true;
        forwardedValue = entry.value;
    }

    result.canExecute = true;

    if (foundForwardingStore) {
        result.shouldForward = true;
        result.forwardedValue = forwardedValue;
    }

    return result;
}