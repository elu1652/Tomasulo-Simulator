#pragma once

#include <string>
#include <vector>

enum class BranchPredictorType;

class BranchPredictor;
class LoadStoreQueue;
class Memory;
class RegisterFile;
class ReorderBuffer;

struct ActiveInstruction;
struct FunctionalUnit;
struct InstructionStatus;
struct TraceSnapshot;

TraceSnapshot makeTraceSnapshot(
    int cycle,
    int pc,
    BranchPredictorType predictorType,
    const BranchPredictor& branchPredictor,
    const std::vector<ActiveInstruction>& activeInstructions,
    const ReorderBuffer& rob,
    const LoadStoreQueue& lsq,
    const std::vector<int>& regProducer,
    const std::vector<InstructionStatus>& statusTable,
    const RegisterFile& rf,
    const Memory& mem,
    const FunctionalUnit& fpAddFU,
    const FunctionalUnit& fpMulFU,
    int intRSCapacity,
    int mulRSCapacity,
    int fpAddRSCapacity,
    int fpMulRSCapacity,
    int loadBufferCapacity,
    int storeBufferCapacity,
    const std::string& issuedInstruction,
    const std::string& cdbBroadcast,
    const std::string& commitEvent,
    const std::vector<std::string>& events
);
