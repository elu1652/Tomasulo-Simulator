#include "InstructionSemantics.h"

int getLatency(OpCode opcode) {
    switch (opcode) {
        case OpCode::ADD:
        case OpCode::ADDI:
        case OpCode::SUB:
        case OpCode::BEQ:
        case OpCode::BNE:
            return 1;

        case OpCode::MUL:
            return 3;

        // FP-style instructions currently use integer values, but are routed
        // through separate FP resources to model latency and structural hazards.
        case OpCode::FADD:
        case OpCode::FSUB:
            return 4;

        case OpCode::FMUL:
            return 7;

        case OpCode::FDIV:
            return 10;

        case OpCode::LD:
        case OpCode::SD:
            return 2;

        default:
            return 1;
    }
}

bool writesRegister(const Instruction& instr) {
    return instr.rd != -1;
}
