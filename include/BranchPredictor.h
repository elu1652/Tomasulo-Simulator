#pragma once

#include <unordered_map>

class BranchPredictor {
private:
    // Maps static PC -> last observed outcome.
    // false = predict not taken, true = predict taken.
    std::unordered_map<int, bool> table;

public:
    bool predict(int pc) const;
    void update(int pc, bool taken);
};