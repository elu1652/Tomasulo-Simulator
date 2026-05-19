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

        case BranchPredictorType::GShare: {
            int index = getGShareIndex(pc);
            auto it = gshareTable.find(index);

            if (it == gshareTable.end()) {
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

        case BranchPredictorType::GShare: {
            int index = getGShareIndex(pc);

            if (gshareTable.find(index) == gshareTable.end()) {
                gshareTable[index] = 1; // weakly not taken
            }

            int& state = gshareTable[index];

            if (taken && state < 3) {
                state++;
            } else if (!taken && state > 0) {
                state--;
            }

            int mask = (1 << historyBits) - 1;
            globalHistory = ((globalHistory << 1) | (taken ? 1 : 0)) & mask;

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

        case BranchPredictorType::GShare: {
            int index = getGShareIndex(pc);
            auto it = gshareTable.find(index);

            if (it == gshareTable.end()) {
                return 1; // weakly not taken default
            }

            return it->second;
        }

        default:
            return -1;
    }
}

int BranchPredictor::getGShareIndex(int pc) const {
    int mask = (1 << historyBits) - 1;
    return (pc ^ globalHistory) & mask;
}

int BranchPredictor::getGlobalHistory() const {
    return globalHistory;
}

int BranchPredictor::getHistoryBits() const {
    return historyBits;
}

int BranchPredictor::getGShareIndexForTrace(int pc) const {
    return getGShareIndex(pc);
}

int BranchPredictor::getGShareCounterByIndex(int index) const {
    auto it = gshareTable.find(index);

    if (it == gshareTable.end()) {
        return 1; // weakly not taken default
    }

    return it->second;
}

std::string branchPredictorTypeToString(BranchPredictorType type) {
    switch (type) {
        case BranchPredictorType::AlwaysNotTaken:
            return "always-not-taken";
        case BranchPredictorType::AlwaysTaken:
            return "always-taken";
        case BranchPredictorType::OneBit:
            return "one-bit";
        case BranchPredictorType::TwoBit:
            return "two-bit";
        case BranchPredictorType::GShare:
            return "gshare";
        default:
            return "unknown";
    }
}

bool parseBranchPredictorType(const std::string& mode, BranchPredictorType& type) {
    if (mode == "always-not-taken" || mode == "not-taken") {
        type = BranchPredictorType::AlwaysNotTaken;
        return true;
    }

    if (mode == "always-taken" || mode == "taken") {
        type = BranchPredictorType::AlwaysTaken;
        return true;
    }

    if (mode == "one-bit" || mode == "1bit" || mode == "1-bit") {
        type = BranchPredictorType::OneBit;
        return true;
    }

    if (mode == "two-bit" || mode == "2bit" || mode == "2-bit") {
        type = BranchPredictorType::TwoBit;
        return true;
    }

    if (mode == "gshare" || mode == "g-share") {
        type = BranchPredictorType::GShare;
        return true;
    }

    return false;
}

bool branchPredictorIsStatic(BranchPredictorType type) {
    return type == BranchPredictorType::AlwaysNotTaken ||
           type == BranchPredictorType::AlwaysTaken;
}

int branchPredictorTraceState(
    const BranchPredictor& predictor,
    BranchPredictorType type,
    int pc
) {
    if (branchPredictorIsStatic(type)) {
        return -1;
    }

    return predictor.getState(pc);
}

std::string branchPredictorStateText(BranchPredictorType type, int state) {
    if (branchPredictorIsStatic(type) || state < 0) {
        return "N/A";
    }

    if (type == BranchPredictorType::OneBit) {
        return state == 0 ? "Not Taken" : "Taken";
    }

    if (type == BranchPredictorType::TwoBit || type == BranchPredictorType::GShare) {
        switch (state) {
            case 0:
                return "Strongly Not Taken";
            case 1:
                return "Weakly Not Taken";
            case 2:
                return "Weakly Taken";
            case 3:
                return "Strongly Taken";
            default:
                return "N/A";
        }
    }

    return "N/A";
}
