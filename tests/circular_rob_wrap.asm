# circular_rob_wrap.asm
#
# Test: circular ROB slot reuse
#
# ROB capacity is 4.
# This program has more than 4 committing instructions.
# If physical ROB slots are not reused correctly, the program may stall forever
# or produce incorrect register state.
#
# EXPECT_REG R1 1
# EXPECT_REG R2 2
# EXPECT_REG R3 3
# EXPECT_REG R4 4
# EXPECT_REG R5 5
# EXPECT_REG R6 6

ADDI R1, R0, 1
ADDI R2, R0, 2
ADDI R3, R0, 3
ADDI R4, R0, 4
ADDI R5, R0, 5
ADDI R6, R0, 6