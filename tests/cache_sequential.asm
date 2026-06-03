# ARCH_CONFIG l1dEnabled=1,l1dNumSets=8,l1dLineSizeBytes=16,l1dHitLatency=1,l1dMissPenalty=10
# EXPECT_REG R7 100
# EXPECT_STAT l1dAccesses 4
# EXPECT_STAT l1dHits 3
# EXPECT_STAT l1dMisses 1
# EXPECT_STAT memoryStallCycles 10

.REG R10 0
.MEM 0 10
.MEM 4 20
.MEM 8 30
.MEM 12 40

LD R1, 0(R10)
LD R2, 4(R10)
LD R3, 8(R10)
LD R4, 12(R10)
ADD R5, R1, R2
ADD R6, R3, R4
ADD R7, R5, R6
