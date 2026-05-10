# branch_not_taken.asm
#
# Test: forward branch not taken
#
# This test checks that a BEQ branch does not update the PC when the comparison is false.
# R1 is set to 7 and R3 is set to 5.
# BEQ R1, R3, target should not be taken.
# Therefore the instruction after the branch should still execute.
#
# Expected final state:
# EXPECT_REG R1 7
# EXPECT_REG R2 7
# EXPECT_REG R3 5
# EXPECT_REG R4 7
# EXPECT_REG R5 2

ADDI R3, R0, 5
ADDI R5, R0, 2
ADD R1, R3, R5
BEQ R1, R3, target
ADD R2, R3, R5
target:
ADD R4, R3, R5
