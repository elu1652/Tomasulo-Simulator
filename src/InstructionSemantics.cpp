#include "InstructionSemantics.h"

int getLatency(OpCode opcode, const ArchitectureConfig& config) {
    switch (opcode) {
        case OpCode::ADD:
        case OpCode::ADDI:
        case OpCode::SUB:
        case OpCode::BEQ:
        case OpCode::BNE:
            return config.intLatency;

        case OpCode::MUL:
            return config.mulLatency;

        // FP-style instructions currently use integer values, but are routed
        // through separate FP resources to model latency and structural hazards.
        case OpCode::FADD:
        case OpCode::FSUB:
            return config.fpAddLatency;

        case OpCode::FMUL:
            return config.fpMulLatency;

        case OpCode::FDIV:
            return config.fpDivLatency;

        case OpCode::LD:
            return config.loadLatency;

        case OpCode::SD:
            return config.storeLatency;

        default:
            return config.intLatency;
    }
}

bool writesRegister(const Instruction& instr) {
    return instr.rd != -1;
}
