#include "BranchPredictor.h"

BranchPredictor::BranchPredictor(BranchPredictorType type): type(type) {

}

bool BranchPredictor::predict(int pc) const {
    switch (type) {
        case BranchPredictorType::AlwaysNotTaken:
            return false;

        case BranchPredictorType::AlwaysTaken:
            return true;

        case BranchPredictorType::OneBit: {
            auto it = oneBitTable.find(pc);

            if (it == oneBitTable.end()) {
                return false;
            }

            return it->second;
        }

        case BranchPredictorType::TwoBit: {
            auto it = twoBitTable.find(pc);

            if (it == twoBitTable.end()) {
                return false; // weakly not taken
            }

            return it->second >= 2;
        }

        default:
            return false;
    }
}

void BranchPredictor::update(int pc, bool taken) {
    switch (type) {
        case BranchPredictorType::AlwaysNotTaken:
        case BranchPredictorType::AlwaysTaken:
            // Static predictor does not learn.
            return;

        case BranchPredictorType::OneBit:
            oneBitTable[pc] = taken;
            return;

        case BranchPredictorType::TwoBit: {
            if (twoBitTable.find(pc) == twoBitTable.end()) {
                twoBitTable[pc] = 1; // weakly not taken
            }

            int& state = twoBitTable[pc];

            if (taken && state < 3) {
                state++;
            } else if (!taken && state > 0) {
                state--;
            }

            return;
        }
    }
}

int BranchPredictor::getState(int pc) const {
    switch (type) {
        case BranchPredictorType::AlwaysNotTaken:
        case BranchPredictorType::AlwaysTaken:
            return -1;

        case BranchPredictorType::OneBit: {
            auto it = oneBitTable.find(pc);
            if (it == oneBitTable.end()) {
                return 0;
            }
            return it->second ? 1 : 0;
        }

        case BranchPredictorType::TwoBit: {
            auto it = twoBitTable.find(pc);
            if (it == twoBitTable.end()) {
                return 1; // weakly not taken default
            }
            return it->second;
        }

        default:
            return -1;
    }
}
