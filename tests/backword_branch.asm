# backward_loop_finite.asm
# Expected behavior:
# R1 starts at 10
# Each loop subtracts R5 = 2
# Loop stops when R1 == 0
#
# Iterations:
# R1 = 8
# R1 = 6
# R1 = 4
# R1 = 2
# R1 = 0
#
# Final:
# R1 = 0
# R2 = 5

loop:
SUB R1, R1, R5
BNE R1, R0, loop
ADD R2, R1, R3