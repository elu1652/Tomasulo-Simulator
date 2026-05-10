#pragma once

#include <unordered_map>

enum class BranchPredictorType {
    AlwaysNotTaken,
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
};