# backword_branch.asm
#
# Test: finite backward branch loop
#
# R1 starts at 10.
# Each loop subtracts R5 = 2.
# Loop stops when R1 == R0.
#
# Iterations:
# R1 = 8
# R1 = 6
# R1 = 4
# R1 = 2
# R1 = 0
#
# Expected final state:
# EXPECT_REG R1 0
# EXPECT_REG R2 5
#
# EXPECT_COMMIT_COUNT ADD R2, R1, R3 1

ADDI R1, R0, 10
ADDI R3, R0, 5
ADDI R5, R0, 2

loop:
SUB R1, R1, R5
BNE R1, R0, loop
ADD R2, R1, R3