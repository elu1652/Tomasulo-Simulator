# raw_dependency.asm
#
# Test: RAW dependency chain
#
# I1 waits for I0.
# I2 waits for I1.
#
# Setup:
# R3 = 5
# R5 = 2
#
# Expected final state:
# EXPECT_REG R1 7
# EXPECT_REG R2 9
# EXPECT_REG R4 14

ADDI R3, R0, 5
ADDI R5, R0, 2

ADD R1, R3, R5
ADD R2, R1, R5
ADD R4, R2, R3