# circular_rob_dependency.asm
#
# Test: circular ROB slot reuse with dependency wakeup
#
# ROB capacity is 4.
# This test forces ROB wraparound, then creates dependencies on values
# produced around the wrap point.
#
# Expected:
# - ROB0/ROB1 get reused
# - R6 depends on R4 and R5
# - R7 depends on R6
#
# EXPECT_REG R1 1
# EXPECT_REG R2 2
# EXPECT_REG R3 3
# EXPECT_REG R4 4
# EXPECT_REG R5 5
# EXPECT_REG R6 9
# EXPECT_REG R7 14

ADDI R1, R0, 1
ADDI R2, R0, 2
ADDI R3, R0, 3
ADDI R4, R0, 4
ADDI R5, R0, 5
ADD R6, R4, R5
ADD R7, R6, R5