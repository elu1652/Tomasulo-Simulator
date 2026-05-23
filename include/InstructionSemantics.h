#pragma once

#include "ArchitectureConfig.h"
#include "Instruction.h"

int getLatency(OpCode opcode, const ArchitectureConfig& config);
bool writesRegister(const Instruction& instr);
