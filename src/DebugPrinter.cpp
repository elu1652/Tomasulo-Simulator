#include "DebugPrinter.h"

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