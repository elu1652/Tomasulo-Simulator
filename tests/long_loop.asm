# long_loop.asm
#
# Test: long loop for testing predictors
#
# R1 is the loop counter, starts at 10.
# R2 is the accumulated value.
# Each iteration adds 10 to R2.
# Loop stops when R1 reaches 0.
#
# Expected final state:
# EXPECT_REG R1 0
# EXPECT_REG R2 100
# EXPECT_REG R3 10
# EXPECT_COMMIT_COUNT ADD R2, R2, R3 10

ADDI R1, R0, 10
ADDI R2, R0, 0
ADDI R3, R0, 10
ADDI R4, R0, 1
loop:
ADD R2, R2, R3
SUB R1, R1, R4
BNE R1, R0, loop
