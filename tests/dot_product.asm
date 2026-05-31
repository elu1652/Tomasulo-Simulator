# dot_product.asm
#
# Long dot product demo:
#
#   sum = A[0]*B[0] + A[1]*B[1] + ... + A[7]*B[7]
#
# This is meant to be a full simulator showcase.
#
# Demonstrates:
# - .REG/.MEM setup directives
# - repeated loads from memory
# - FP_MUL pipeline usage
# - FP_ADD accumulator dependency
# - independent integer pointer updates
# - branch prediction over a loop
# - ROB pressure from long-latency FP operations
# - CDB writeback pressure
# - performance statistics / IPC / stall breakdowns
# - architecture config comparisons
#
# Register mapping:
#   R10 = pointer to A
#   R11 = pointer to B
#   R13 = loop counter
#   R1  = loaded A[i]
#   R2  = loaded B[i]
#   R3  = product
#   R4  = accumulator sum
#
# A = [1, 2, 3, 4, 5, 6, 7, 8]
# B = [10, 20, 30, 40, 50, 60, 70, 80]
#
# Expected dot product:
#   1*10 + 2*20 + 3*30 + 4*40 + 5*50 + 6*60 + 7*70 + 8*80
# = 10 + 40 + 90 + 160 + 250 + 360 + 490 + 640
# = 2040
#
# EXPECT_REG R4 2040
# EXPECT_REG R10 32
# EXPECT_REG R11 64
# EXPECT_REG R13 0
#
# EXPECT_COMMIT_COUNT LD R1, 0(R10) 8
# EXPECT_COMMIT_COUNT LD R2, 0(R11) 8
# EXPECT_COMMIT_COUNT FMUL R3, R1, R2 8
# EXPECT_COMMIT_COUNT FADD R4, R4, R3 8
# EXPECT_COMMIT_COUNT ADDI R10, R10, 4 8
# EXPECT_COMMIT_COUNT ADDI R11, R11, 4 8
# EXPECT_COMMIT_COUNT ADDI R13, R13, -1 8
# EXPECT_COMMIT_COUNT BNE R13, R0, loop 8

# Initial register state
.REG R10 0       # pointer to A
.REG R11 32      # pointer to B
.REG R13 8       # loop count
.REG R4 0        # accumulator sum

# A array at memory addresses 0..28
.MEM 0 1
.MEM 4 2
.MEM 8 3
.MEM 12 4
.MEM 16 5
.MEM 20 6
.MEM 24 7
.MEM 28 8

# B array at memory addresses 32..60
.MEM 32 10
.MEM 36 20
.MEM 40 30
.MEM 44 40
.MEM 48 50
.MEM 52 60
.MEM 56 70
.MEM 60 80

loop:
LD R1, 0(R10)        # load A[i]
LD R2, 0(R11)        # load B[i]

FMUL R3, R1, R2      # product = A[i] * B[i]
FADD R4, R4, R3      # sum += product

ADDI R10, R10, 4     # advance A pointer
ADDI R11, R11, 4     # advance B pointer
ADDI R13, R13, -1    # decrement loop counter

BNE R13, R0, loop