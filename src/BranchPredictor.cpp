#include "BranchPredictor.h"

bool BranchPredictor::predict(int pc) const {
    auto it = table.find(pc);

    if (it == table.end()) {
        return false; // Default prediction: not taken
    }

    return it->second;
}

void BranchPredictor::update(int pc, bool taken) {
    table[pc] = taken;
}