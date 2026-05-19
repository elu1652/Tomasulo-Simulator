#pragma once

#include <string>
#include <unordered_map>

enum class BranchPredictorType {
    AlwaysNotTaken,
    AlwaysTaken,
    OneBit,
    TwoBit,
    GShare
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

    // GShare predictor:
    // Uses global branch history XORed with the branch PC to index
    // a table of 2-bit saturating counters.
    int globalHistory = 0;
    int historyBits = 4;
    std::unordered_map<int, int> gshareTable;

    int getGShareIndex(int pc) const;

public:
    explicit BranchPredictor(
        BranchPredictorType type = BranchPredictorType::OneBit
    );

    bool predict(int pc) const;
    void update(int pc, bool taken);
    int getState(int pc) const;

    int getGlobalHistory() const;
    int getHistoryBits() const;
    int getGShareIndexForTrace(int pc) const;
    int getGShareCounterByIndex(int index) const;
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