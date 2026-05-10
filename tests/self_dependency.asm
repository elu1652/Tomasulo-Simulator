# self_dependency.asm
#
# Test: same register used as both source and destination
#
# I1 should read the old R1 from I0 before renaming R1 to itself.
# I2 should depend on I1.
#
# Setup:
# R3 = 5
# R5 = 2
#
# Expected final state:
# EXPECT_REG R1 9
# EXPECT_REG R2 14

ADDI R3, R0, 5
ADDI R5, R0, 2

ADD R1, R3, R5
ADD R1, R1, R5
ADD R2, R1, R3