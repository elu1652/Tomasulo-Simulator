# one_bit_optimal.asm
#
# Test: pattern designed to favor 1-bit branch prediction
#
# Main branch outcome pattern:
#   T T N N T T N N T T N N
#
# A 1-bit predictor can adapt quickly after each direction change.
# A 2-bit predictor is slower to switch because of hysteresis.
#
# This does not test architectural correctness of prediction accuracy directly.
# Final architectural state should be correct for all predictors.
#
# Expected final state:
# R1 = 0
# R2 = 0
# R3 = 6
# R4 = 6
# R5 = 1
# R6 = 12
#
# EXPECT_REG R1 0
# EXPECT_REG R2 0
# EXPECT_REG R3 6
# EXPECT_REG R4 6
# EXPECT_REG R5 1
# EXPECT_REG R6 12
# EXPECT_COMMIT_COUNT BNE R1, R0, taken_path 12
# EXPECT_COMMIT_COUNT ADDI R3, R3, 1 6
# EXPECT_COMMIT_COUNT ADDI R4, R4, 1 6

ADDI R1, R0, 1
ADDI R2, R0, 0
ADDI R3, R0, 0
ADDI R4, R0, 0
ADDI R5, R0, 1
ADDI R6, R0, 12

loop:
BNE R1, R0, taken_path

not_taken_path:
ADDI R4, R4, 1
SUB R2, R5, R2
SUB R1, R5, R2
BEQ R0, R0, after_branch

taken_path:
ADDI R3, R3, 1
SUB R2, R5, R2
ADD R1, R2, R0

after_branch:
SUB R6, R6, R5
BNE R6, R0, loop