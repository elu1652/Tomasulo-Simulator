# I1 should read old R1 from I0 before renaming R1 to itself
# I2 should depend on I1

# Final:
# R1 = 9
# R2 = 14

ADD R1, R3, R5
ADD R1, R1, R5
ADD R2, R1, R3