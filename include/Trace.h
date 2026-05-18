#pragma once

#include <string>
#include <vector>

struct TraceActiveInstruction {
    int instructionId = -1;
    int robTag = -1;
    std::string rawText;

    bool executing = false;
    int remainingCycles = 0;
    std::string waitingReason;

    int vj = 0;
    int vk = 0;
    int qj = -1;
    int qk = -1;
};

struct TraceROBEntry {
    int robTag = -1;
    int instructionId = -1;
    std::string rawText;

    bool busy = false;
    bool ready = false;

    bool writesRegister = false;
    int destinationRegister = -1;
    int value = 0;

    bool writesMemory = false;
    int memoryAddress = -1;
    int memoryValue = 0;
};

struct TraceLSQEntry {
    int instructionId = -1;
    int robTag = -1;
    std::string rawText;

    bool busy = false;
    bool isLoad = false;
    bool isStore = false;

    bool addressReady = false;
    int address = 0;

    bool valueReady = false;
    int value = 0;
};

struct TraceRegisterProducer {
    int registerNumber = -1;
    int robTag = -1;
};

struct TraceBranchPredictionEntry {
    int pc = -1;
    std::string instruction;
    std::string predictorType;
    bool predictedTaken = false;
    bool actualTaken = false;
    bool branchResolved = false;
    bool resolvedThisCycle = false;
    bool predictionCorrect = false;
    int targetPc = -1;
    int fallthroughPc = -1;
    int stateBefore = -1;
    int stateAfter = -1;
    std::string stateBeforeText;
    std::string stateAfterText;
};

struct TraceSnapshot {
    int cycle = 0;
    int pc = 0;
    std::string predictorType;

    std::string issuedInstruction;
    std::string cdbBroadcast;
    std::string commitEvent;

    std::vector<int> registers;
    std::vector<int> memory;

    int robHead = 0;
    int robTail = 0;
    int robCount = 0;

    std::vector<TraceActiveInstruction> activeInstructions;
    std::vector<TraceROBEntry> robEntries;
    std::vector<TraceLSQEntry> lsqEntries;
    std::vector<TraceRegisterProducer> registerProducers;
    std::vector<TraceBranchPredictionEntry> branchPredictions;

    std::vector<std::string> events;
};

struct TraceRecorder {
    std::vector<TraceSnapshot> snapshots;

    void addSnapshot(const TraceSnapshot& snapshot);
    void writeJson(const std::string& filename) const;
};
