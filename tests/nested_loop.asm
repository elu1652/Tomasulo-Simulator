# nested_loop.asm
#
# Test: nested loop with 3 outer iterations and 4 inner iterations
#
# Outer loop runs 3 times.
# Inner loop runs 4 times per outer iteration.
# Each inner iteration adds 10 to R3.
#
# Total additions:
# 3 * 4 = 12
#
# Expected:
# R3 = 120
#
# EXPECT_REG R1 0
# EXPECT_REG R2 0
# EXPECT_REG R3 120
# EXPECT_REG R4 10
# EXPECT_COMMIT_COUNT ADD R3, R3, R4 12

ADDI R1, R0, 3
ADDI R3, R0, 0
ADDI R4, R0, 10
ADDI R5, R0, 1

outer:
ADDI R2, R0, 4

inner:
ADD R3, R3, R4
SUB R2, R2, R5
BNE R2, R0, inner

SUB R1, R1, R5
BNE R1, R0, outer