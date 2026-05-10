# rob_full.asm 
#
# Test: ROB full stall
#
# The ROB should eventually show 4/4.
# Issue should stall because the ROB is full.
# The simulator should continue running, not terminate.
#
# Setup:
# R3 = 5
# R5 = 2
#
# Expected final state:
# EXPECT_REG R2 10
# EXPECT_REG R6 7

ADDI R3, R0, 5
ADDI R5, R0, 2

MUL R1, R3, R5
MUL R2, R3, R5
ADD R6, R3, R5
LD R1, 0(R0)
LD R5, 4(R1)