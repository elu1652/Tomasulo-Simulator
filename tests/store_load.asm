# store_load.asm
#
# Test: store then load from same address
#
# This test stores a computed value to memory, then loads from the same address.
# R1 is set to 7 using ADD.
# The store writes R1 to Mem[8].
# A non-taken branch is used as a simple barrier so the store can commit before the load.
# The load should read 7 from Mem[8].
#
# Expected final state:
# EXPECT_REG R1 7
# EXPECT_REG R2 7
# EXPECT_REG R3 5
# EXPECT_REG R5 2
# EXPECT_MEM 8 7

ADDI R3, R0, 5
ADDI R5, R0, 2
ADD R1, R3, R5
SD R1, 8(R0)
BNE R1, R1, after_store
after_store:
LD R2, 8(R0)
