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

int getRSCapacity(
    RSType type,
    int intCapacity,
    int mulCapacity,
    int fpAddCapacity,
    int fpMulCapacity,
    int loadCapacity,
    int storeCapacity
) {
    switch (type) {
        case RSType::INT:
            return intCapacity;

        case RSType::MUL:
            return mulCapacity;

        case RSType::FP_ADD:
            return fpAddCapacity;

        case RSType::FP_MUL:
            return fpMulCapacity;

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
        case OpCode::ADDI:
        case OpCode::SUB:
        case OpCode::BEQ:
        case OpCode::BNE:
            return RSType::INT;

        case OpCode::MUL:
            return RSType::MUL;

        case OpCode::FADD:
        case OpCode::FSUB:
            return RSType::FP_ADD;

        case OpCode::FMUL:
        case OpCode::FDIV:
            return RSType::FP_MUL;

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
        case RSType::FP_ADD: return "FP_ADD";
        case RSType::FP_MUL: return "FP_MUL";
        case RSType::LOAD: return "LOAD";
        case RSType::STORE: return "STORE";
        default: return "NONE";
    }
}