# ooo_writeback.asm
#
# Test: out-of-order writeback with in-order commit
#
# ADDs may finish before MUL.
# ADDs may write back before MUL.
# But commit must still happen in program order.
#
# Setup:
# R3 = 5
# R5 = 2
#
# Expected final state:
# EXPECT_REG R1 10
# EXPECT_REG R2 7
# EXPECT_REG R4 7

ADDI R3, R0, 5
ADDI R5, R0, 2

MUL R1, R3, R5
ADD R2, R3, R5
ADD R4, R3, R5