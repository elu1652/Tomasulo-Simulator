#include "CDB.h"   

bool broadcastCDB(
    const CDBMessage& cdb,
    std::vector<ActiveInstruction>& activeInstructions,
    std::vector<ROBEntry>& rob
) {
    if (!cdb.valid) {
        return false;
    }

    bool wokeSomeone = false;

    std::cout << "  Broadcast: I" << cdb.producerTag << "\n"; // Producer

    ROBEntry& entry = rob[cdb.robTag];
    entry.ready = true;
    entry.writesRegister = true;
    entry.destinationRegister = cdb.destinationRegister;
    entry.value = cdb.value;

    std::cout << "  ROB Write: ROB" << cdb.robTag
          << " / I" << cdb.producerTag
          << " value = " << cdb.value
          << " -> R" << cdb.destinationRegister << "\n";

    for (auto& other : activeInstructions) {
        if (other.qj == cdb.robTag) {
            other.qj = -1;
            other.vj = cdb.value;

            std::cout << "  Wakeup: I" << other.instructionIndex
                    << " qj resolved by ROB" << cdb.robTag
                    << " / I" << cdb.producerTag
                    << " with value " << cdb.value << "\n";

            wokeSomeone = true;
        }

        if (other.qk == cdb.robTag) {
            other.qk = -1;
            other.vk = cdb.value;

            std::cout << "  Wakeup: I" << other.instructionIndex
                      << " qk resolved by I" << cdb.producerTag
                      << " with value " << cdb.value << "\n";

            wokeSomeone = true;
        }
    }

    if (!wokeSomeone) {
        std::cout << "  Wakeup: none\n";
    }

    return wokeSomeone;
}

void flushCDBQueue(std::queue<CDBMessage>& cdbQueue, int branchIndex) {
    std::queue<CDBMessage> kept;

    while (!cdbQueue.empty()) {
        CDBMessage msg = cdbQueue.front();
        cdbQueue.pop();

        if (msg.producerTag > branchIndex) {
            std::cout << "  Flushed CDB: I"
                      << msg.producerTag
                      << " "
                      << msg.rawText
                      << "\n";
        } else {
            kept.push(msg);
        }
    }

    cdbQueue = kept;
}