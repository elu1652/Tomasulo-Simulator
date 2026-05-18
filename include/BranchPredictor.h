#pragma once

#include <string>
#include <unordered_map>

enum class BranchPredictorType {
    AlwaysNotTaken,
    AlwaysTaken,
    OneBit,
    TwoBit
};

class BranchPredictor {
private:
    BranchPredictorType type;

    // 1-bit predictor table:
    // false = predict not taken, true = predict taken.
    std::unordered_map<int, bool> oneBitTable;

    // 2-bit predictor:
    // 0 = strongly not taken
    // 1 = weakly not taken
    // 2 = weakly taken
    // 3 = strongly taken
    std::unordered_map<int, int> twoBitTable;

public:
    explicit BranchPredictor(
        BranchPredictorType type = BranchPredictorType::OneBit
    );

    bool predict(int pc) const;
    void update(int pc, bool taken);
    int getState(int pc) const;
};

std::string branchPredictorTypeToString(BranchPredictorType type);
bool parseBranchPredictorType(const std::string& mode, BranchPredictorType& type);
bool branchPredictorIsStatic(BranchPredictorType type);
int branchPredictorTraceState(
    const BranchPredictor& predictor,
    BranchPredictorType type,
    int pc
);
std::string branchPredictorStateText(BranchPredictorType type, int state);
