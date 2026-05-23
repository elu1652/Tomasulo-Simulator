#include "ProgramSetup.h"

#include "Memory.h"
#include "RegisterFile.h"

#include <iostream>

void applyProgramSetup(
    RegisterFile& rf,
    Memory& mem,
    const ProgramSetup& setup
) {
    for (const auto& [reg, value] : setup.registerInitializers) {
        if (reg == 0 && value != 0) {
            std::cerr << "Warning: .REG R0 " << value
                      << " ignored; R0 is always zero\n";
        }

        rf.write(reg, value);
    }

    for (const auto& [address, value] : setup.memoryInitializers) {
        mem.store(address, value);
    }
}
