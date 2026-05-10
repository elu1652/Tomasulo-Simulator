# load_use_dependency.asm
#
# Test: load-use dependency chain
#
# Initializes Mem[0] = 99, then loads it into R1.
# I1 waits for the load result from I0.
# I2 waits for the ADD result from I1.
#
# Setup:
# R3 = 5
# R5 = 2
# Mem[0] = 99
#
# Expected final state:
# EXPECT_REG R1 99
# EXPECT_REG R2 104
# EXPECT_REG R4 106
# EXPECT_MEM 0 99

ADDI R3, R0, 5
ADDI R5, R0, 2
ADDI R10, R0, 99
SD R10, 0(R0)

BNE R10, R10, after_init

after_init:
LD R1, 0(R0)
ADD R2, R1, R3
ADD R4, R2, R5