# waw_renaming.asm
#
# Test: repeated writes to the same architectural register
#
# I2 should depend on I1, not I0.
#
# Setup:
# R3 = 5
# R5 = 2
#
# Expected final state:
# EXPECT_REG R1 10
# EXPECT_REG R2 15

ADDI R3, R0, 5
ADDI R5, R0, 2

ADD R1, R3, R5
MUL R1, R3, R5
ADD R2, R1, R3