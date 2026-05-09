# ADDs may finish before MUL.
# ADDs may write back before MUL.
# But commit must still happen in order.

# Final:
# R1 = 10
# R2 = 7
# R4 = 7

MUL R1, R3, R5
ADD R2, R3, R5
ADD R4, R3, R5