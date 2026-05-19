# two_bit_optimal.asm
#
# Test: repeated loop pattern designed to favor 2-bit branch prediction
#
# Inner loop branch pattern per outer iteration:
#   T T T N
#
# Repeated 4 times:
#   T T T N | T T T N | T T T N | T T T N
#
# A 2-bit predictor should handle this better than a 1-bit predictor because
# it does not immediately switch direction after one loop-exit miss.
#
# Final architectural state should be correct for all predictors.
#
# Expected final state:
# R1 = 0
# R2 = 0
# R3 = 16
# R4 = 1
#
# EXPECT_REG R1 0
# EXPECT_REG R2 0
# EXPECT_REG R3 16
# EXPECT_REG R4 1
# EXPECT_COMMIT_COUNT ADD R3, R3, R4 16
# EXPECT_COMMIT_COUNT BNE R2, R0, inner_loop 16
# EXPECT_COMMIT_COUNT BNE R1, R0, outer_loop 4

ADDI R1, R0, 4
ADDI R3, R0, 0
ADDI R4, R0, 1

outer_loop:
ADDI R2, R0, 4

inner_loop:
ADD R3, R3, R4
SUB R2, R2, R4
BNE R2, R0, inner_loop

SUB R1, R1, R4
BNE R1, R0, outer_loop