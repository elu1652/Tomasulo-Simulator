# store_commit.asm
#
# Test: store commits through the ROB
#
# R1 = 7
# Store writes R1 to Mem[8]
# Load reads Mem[8] into R2 after the store commits
#
# Expected final state:
# EXPECT_REG R1 7
# EXPECT_REG R2 7
# EXPECT_MEM 8 7

ADDI R3, R0, 5
ADDI R5, R0, 2

ADD R1, R3, R5
SD R1, 8(R0)

# Non-taken branch used as a simple non-speculative barrier.
# This gives the older store time to commit before the load executes.
BNE R1, R1, after_store

after_store:
LD R2, 8(R0)