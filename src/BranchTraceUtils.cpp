#include "BranchTraceUtils.h"

std::string formatBinary(int value, int bits) {
    std::string result;

    for (int i = bits - 1; i >= 0; --i) {
        result += ((value >> i) & 1) ? '1' : '0';
    }

    return result;
}

static std::string predictorStateBits(BranchPredictorType type, int state) {
    if (branchPredictorIsStatic(type)) {
        return "static";
    }

    if (state < 0) {
        return "";
    }

    if (type == BranchPredictorType::OneBit) {
        return formatBinary(state, 1);
    }

    return formatBinary(state, 2);
}

static std::string predictorStatePrediction(
    BranchPredictorType type,
    int state
) {
    if (type == BranchPredictorType::AlwaysNotTaken) {
        return "NT";
    }

    if (type == BranchPredictorType::AlwaysTaken) {
        return "T";
    }

    if (state < 0) {
        return "-";
    }

    if (type == BranchPredictorType::OneBit) {
        return state == 1 ? "T" : "NT";
    }

    return state >= 2 ? "T" : "NT";
}

TracePredictorState makeTracePredictorState(
    const BranchPredictor& branchPredictor,
    BranchPredictorType predictorType
) {
    TracePredictorState state;

    state.predictorType = branchPredictorTypeToString(predictorType);

    if (predictorType == BranchPredictorType::GShare) {
        state.globalHistory = branchPredictor.getGlobalHistory();
        state.globalHistoryBits = branchPredictor.getHistoryBits();
        state.globalHistoryText = formatBinary(
            state.globalHistory,
            state.globalHistoryBits
        );
    }

    if (branchPredictorIsStatic(predictorType)) {
        TracePredictorStateEntry entry;

        entry.index = -1;
        entry.state = -1;
        entry.stateBits = "static";
        entry.stateText = predictorType == BranchPredictorType::AlwaysTaken
            ? "Always Taken"
            : "Always Not Taken";
        entry.prediction = predictorStatePrediction(predictorType, -1);

        state.entries.push_back(entry);
        return state;
    }

    for (const BranchPredictorTableEntry& tableEntry :
         branchPredictor.getTableEntries()) {
        TracePredictorStateEntry entry;

        entry.index = tableEntry.index;
        entry.state = tableEntry.state;
        entry.stateBits = predictorStateBits(predictorType, tableEntry.state);
        entry.stateText = branchPredictorStateText(
            predictorType,
            tableEntry.state
        );
        entry.prediction = predictorStatePrediction(
            predictorType,
            tableEntry.state
        );

        state.entries.push_back(entry);
    }

    return state;
}
