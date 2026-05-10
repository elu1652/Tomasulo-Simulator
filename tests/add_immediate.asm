# add_immediate.asm
#
# Test: ADDI instruction
#
# Expected final state:
# EXPECT_REG R3 5
# EXPECT_REG R5 2
# EXPECT_REG R1 7

ADDI R3, R0, 5
ADDI R5, R0, 2
ADD R1, R3, R5