# forward_branch.asm
#
# Test: forward taken branch
#
# R1 = R3 + R5 = 7
# R2 = R1 + R5 = 9
# BNE R2, R3 is taken because 9 != 5
# ADD R4, R0, R0 should be skipped
# target instruction should execute
#
# Setup:
# R3 = 5
# R5 = 2
#
# Expected final state:
# EXPECT_REG R1 7
# EXPECT_REG R2 9
# EXPECT_REG R4 0
# EXPECT_REG R6 7

ADDI R3, R0, 5
ADDI R5, R0, 2

ADD R1, R3, R5
ADD R2, R1, R5
BNE R2, R3, target
ADD R4, R0, R0

target:
ADD R6, R3, R5