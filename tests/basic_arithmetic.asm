# basic_arithmetic.asm
#
# Test: independent arithmetic instructions
#
# Setup:
# R3 = 5
# R5 = 2
#
# Expected final state:
# EXPECT_REG R1 7
# EXPECT_REG R2 3
# EXPECT_REG R3 5
# EXPECT_REG R4 10
# EXPECT_REG R5 2

ADDI R3, R0, 5
ADDI R5, R0, 2

ADD R1, R3, R5
SUB R2, R3, R5
MUL R4, R3, R5