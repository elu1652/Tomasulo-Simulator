#include "CDB.h"   

bool broadcastCDB(
    const CDBMessage& cdb,
    std::vector<ActiveInstruction>& activeInstructions,
    std::vector<int>& regProducer,
    RegisterFile& rf
) {
    if (!cdb.valid) {
        return false;
    }

    bool wokeSomeone = false;

    std::cout << "  Broadcast: I" << cdb.producerTag << "\n";

    if (regProducer[cdb.destinationRegister] == cdb.producerTag) {
        rf.write(cdb.destinationRegister, cdb.value);
        regProducer[cdb.destinationRegister] = -1;

        std::cout << "  RF Write: R" << cdb.destinationRegister
                  << " = " << cdb.value << "\n";
    } else {
        std::cout << "  RF Write: skipped, newer producer owns R"
                  << cdb.destinationRegister << "\n";
    }

    for (auto& other : activeInstructions) {
        if (other.qj == cdb.producerTag) {
            other.qj = -1;
            other.vj = cdb.value;

            std::cout << "  Wakeup: I" << other.instructionIndex
                      << " qj resolved by I" << cdb.producerTag
                      << " with value " << cdb.value << "\n";

            wokeSomeone = true;
        }

        if (other.qk == cdb.producerTag) {
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
