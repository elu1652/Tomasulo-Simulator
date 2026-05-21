#pragma once

#include "Instruction.h"

int getLatency(OpCode opcode);
bool writesRegister(const Instruction& instr);
