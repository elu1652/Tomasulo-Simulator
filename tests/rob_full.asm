# ROB should eventually show 4/4.
# Issue should stall because ROB is full.
# The simulator should continue running, not terminate.

MUL R1, R3, R5
MUL R2, R3, R5
ADD R6, R3, R5
LD R1, 0(R0)
LD R5, 4(R1)