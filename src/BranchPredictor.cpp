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

    if (type == BranchPredictorType::TwoBit) {
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
