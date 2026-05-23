# tests/final_dynamic_matvec_2iter.asm
#
# Based on final exam dynamic scheduling table.
#
# Setup directives initialize architectural state before cycle 1, so the
# first real instruction is the first load in the loop.
#
# To compare the first two completed loop iterations in the trace:
#   After iteration 1: R3 = 10 * 20 = 200, R4 = 200
#   After iteration 2: R3 = 20 * 30 = 600, R4 = 800
#
# The full program executes three loop iterations because R13 starts at 3.

.REG R10 0
.REG R11 4
.REG R13 3
.REG R4 0

.MEM 0 10
.MEM 4 20
.MEM 8 30
.MEM 12 40

loop:
LD R1, 0(R10)
LD R2, 0(R11)
FMUL R3, R1, R2
FADD R4, R4, R3
ADDI R10, R10, 4
ADDI R11, R11, 4
ADDI R13, R13, -1
BNE R13, R0, loop
