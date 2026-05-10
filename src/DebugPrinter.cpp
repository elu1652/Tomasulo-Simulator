#include "DebugPrinter.h"
#include <iomanip>

void printFUState(
    const FunctionalUnit& intFU,
    const FunctionalUnit& mulFU,
    const FunctionalUnit& memFU
) {
    std::cout << "FU State:\n";

    std::cout << "  INT: "
              << intFU.busyUnits << "/" << intFU.totalUnits
              << " busy\n";

    std::cout << "  MUL: "
              << mulFU.busyUnits << "/" << mulFU.totalUnits
              << " busy\n";

    std::cout << "  MEM: "
              << memFU.busyUnits << "/" << memFU.totalUnits
              << " busy\n";
}

void printTag(int tag) {
    if (tag == -1) {
        std::cout << "-";
    } else {
        std::cout << "I" << tag;
    }
}

void printActiveInstructions(
    const std::vector<ActiveInstruction>& activeInstructions
) {
    std::cout << "Active Instructions:\n";

    if (activeInstructions.empty()) {
        std::cout << "  none\n";
        return;
    }

    for (const auto& active : activeInstructions) {
        std::cout << "  I" << active.instructionIndex
                  << ": " << active.instr.rawText << " | ";

        if (active.executing && active.remainingCycles == 0) {
            std::cout << "done this cycle";
        } else if (active.executing) {
            std::cout << "executing";
        } else {
            std::cout << "waiting";
        }

        std::cout << " | rem: " << active.remainingCycles;

        std::cout << " | vj: ";
        if (active.qj == -1) {
            std::cout << active.vj;
        } else {
            std::cout << "-";
        }

        std::cout << " | vk: ";
        if (active.qk == -1) {
            std::cout << active.vk;
        } else {
            std::cout << "-";
        }

        std::cout << " | qj: ";
        printTag(active.qj);

        std::cout << " | qk: ";
        printTag(active.qk);

        if (!active.executing) {
            std::cout << " | reason: " << active.waitingReason;
        }

        std::cout << "\n";
    }
}

void printRegisterProducer(const std::vector<int>& regProducer) {
    std::cout << "Register Producers:\n";

    bool anyPending = false;

    for (int i = 0; i < regProducer.size(); i++) {
        if (regProducer[i] != -1) {
            std::cout << "  R" << i << " <- I" << regProducer[i] << "\n";
            anyPending = true;
        }
    }

    if (!anyPending) {
        std::cout << "  none\n";
    }
}

void printRSState(
    const std::vector<ActiveInstruction>& activeInstructions,
    int intCapacity,
    int mulCapacity,
    int loadCapacity,
    int storeCapacity
) {
    int intCount = countRSEntries(activeInstructions, RSType::INT);
    int mulCount = countRSEntries(activeInstructions, RSType::MUL);
    int loadCount = countRSEntries(activeInstructions, RSType::LOAD);
    int storeCount = countRSEntries(activeInstructions, RSType::STORE);

    std::cout << "RS State:\n";
    std::cout << "  INT RS: " << intCount << "/" << intCapacity << "\n";
    std::cout << "  MUL RS: " << mulCount << "/" << mulCapacity << "\n";
    std::cout << "  Load Buffer: " << loadCount << "/" << loadCapacity << "\n";
    std::cout << "  Store Buffer: " << storeCount << "/" << storeCapacity << "\n";
}

void printCDBQueue(std::queue<CDBMessage> cdbQueue) {
    std::cout << "CDB Queue:\n";

    if (cdbQueue.empty()) {
        std::cout << "  none\n";
        return;
    }

    while (!cdbQueue.empty()) {
        CDBMessage msg = cdbQueue.front();
        cdbQueue.pop();

        std::cout << "  I" << msg.producerTag
                  << ": " << msg.rawText
                  << " | value: " << msg.value
                  << " -> R" << msg.destinationRegister
                  << "\n";
    }
}

void printROB(
    const std::queue<int>& robQueueOriginal,
    const std::vector<ROBEntry>& rob,
    int robCapacity
) {
    std::queue<int> robQueue = robQueueOriginal;

    std::cout << "ROB: "
          << robQueueOriginal.size()
          << "/"
          << robCapacity
          << "\n";

    if (robQueue.empty()) {
        std::cout << "  empty\n";
        return;
    }

    while (!robQueue.empty()) {
        int tag = robQueue.front();
        robQueue.pop();

        const ROBEntry& entry = rob[tag];

        std::cout << "  I" << tag
                  << " | " << entry.rawText
                  << " | ready: " << (entry.ready ? "yes" : "no");

        if (entry.writesRegister) {
            std::cout << " | R" << entry.destinationRegister
                      << " = " << entry.value;
        }

        if (entry.writesMemory) {
            std::cout << " | Mem[" << entry.memoryAddress
                      << "] = " << entry.memoryValue;
        }

        std::cout << "\n";
    }
}

void printInstructionStatusTable(const std::vector<InstructionStatus>& statusTable) {
    // Final instruction status table.
    // Dynamic instruction IDs are shown, so loop iterations appear as separate rows.
    std::cout << "\nInstruction Status Table:\n";

    auto cycleString = [](int cycle) {
        return cycle == -1 ? std::string("-") : std::to_string(cycle);
    };

    std::cout
        << std::left
        << std::setw(6)  << "ID"
        << std::setw(6)  << "PC"
        << std::setw(28) << "Instruction"
        << std::setw(8)  << "Issue"
        << std::setw(12) << "ExecStart"
        << std::setw(10) << "ExecEnd"
        << std::setw(8)  << "WB"
        << std::setw(10) << "Commit"
        << std::setw(8)  << "Flush"
        << "\n";

    std::cout << std::string(104, '-') << "\n";

    for (int i = 0; i < statusTable.size(); i++) {
        std::cout
            << std::left
            << std::setw(6)  << ("I" + std::to_string(i))
            << std::setw(6)  << statusTable[i].staticPc
            << std::setw(28) << statusTable[i].rawText
            << std::setw(8)  << cycleString(statusTable[i].issueCycle)
            << std::setw(12) << cycleString(statusTable[i].executeStartCycle)
            << std::setw(10) << cycleString(statusTable[i].executeEndCycle)
            << std::setw(8)  << cycleString(statusTable[i].writebackCycle)
            << std::setw(10) << cycleString(statusTable[i].commitCycle)
            << std::setw(8)  << (statusTable[i].flushed ? "yes" : "no")
            << "\n";
    }
}

void printBranchPredictionSummary(const std::vector<InstructionStatus>& statusTable) {
    int resolvedBranches = 0;
    int correctPredictions = 0;
    
    bool hasBranch = false;

    for (const auto& status : statusTable) {
        if (status.isBranch) {
            hasBranch = true;
            break;
        }
    }

    if (!hasBranch) {
        return;
    }

    auto takenString = [](bool taken) {
        return taken ? "T" : "NT";
    };

    auto stateString = [](int state) {
        return state == -1 ? std::string("-") : std::to_string(state);
    };

    std::cout << "\nBranch Prediction Summary:\n";

    std::cout
        << std::left
        << std::setw(6)  << "ID"
        << std::setw(6)  << "PC"
        << std::setw(28) << "Instruction"
        << std::setw(13) << "StateBefore"
        << std::setw(12) << "Predicted"
        << std::setw(10) << "Actual"
        << std::setw(12) << "StateAfter"
        << std::setw(8)  << "Result"
        
        << "\n";

    std::cout << std::string(94, '-') << "\n";

    for (int i = 0; i < statusTable.size(); i++) {
        const auto& status = statusTable[i];

        if (!status.isBranch) {
            continue;
        }

        std::string result = "-";

        if (status.branchResolved) {
            result = status.predictedTaken == status.actualTaken ? "Hit" : "Miss";
        }
        if (status.branchResolved) {
            resolvedBranches++;

            if (status.predictedTaken == status.actualTaken) {
                correctPredictions++;
            }
        }

        std::cout
            << std::left
            << std::setw(6)  << ("I" + std::to_string(i))
            << std::setw(6)  << status.staticPc
            << std::setw(28) << status.rawText
            << std::setw(13) << stateString(status.predictorStateBefore)
            << std::setw(12) << takenString(status.predictedTaken)
            << std::setw(10) << (status.branchResolved ? takenString(status.actualTaken) : "-")
            << std::setw(12) << stateString(status.predictorStateAfter)
            << std::setw(8)  << result
            << "\n";
    }
    if (resolvedBranches > 0) {
        double accuracy =
            100.0 * correctPredictions / resolvedBranches;

        std::cout << "\nBranch prediction accuracy: "
                << correctPredictions << "/"
                << resolvedBranches << " correct = "
                << std::fixed << std::setprecision(2)
                << accuracy << "%\n\n";
    }
}