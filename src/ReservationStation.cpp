#include "ReservationStation.h"

/*************************************** 
* Reservation Station                  * 
***************************************/
int countRSEntries(const std::vector<ActiveInstruction>& activeInstructions, RSType type){
    int count = 0;

    for (const auto& active : activeInstructions) {
        if (getRSType(active.instr.opcode) == type) {
            count++;
        }
    }

    return count;
}

int getRSCapacity(RSType type, int intCapacity, int mulCapacity, int loadCapacity, int storeCapacity){
    switch (type) {
        case RSType::INT:
            return intCapacity;

        case RSType::MUL:
            return mulCapacity;

        case RSType::LOAD:
            return loadCapacity;

        case RSType::STORE:
            return storeCapacity;

        default:
            return 0;
    }
}

RSType getRSType(OpCode opcode) {
    switch (opcode) {
        case OpCode::ADD:
        case OpCode::SUB:
            return RSType::INT;

        case OpCode::MUL:
            return RSType::MUL;

        case OpCode::LD:
            return RSType::LOAD;

        case OpCode::SD:
            return RSType::STORE;

        default:
            return RSType::NONE;
    }
}

std::string rsTypeToString(RSType type) {
    switch (type) {
        case RSType::INT: return "INT";
        case RSType::MUL: return "MUL";
        case RSType::LOAD: return "LOAD";
        case RSType::STORE: return "STORE";
        default: return "NONE";
    }
}
