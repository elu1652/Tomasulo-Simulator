# I1 waits for load result from I0.
# I2 waits for ADD result from I1.

# Final:
# R1 = 99
# R2 = 104
# R4 = 106

LD R1, 0(R0)
ADD R2, R1, R3
ADD R4, R2, R5