.3ds
.thumb
.create "testtest.bin",0

LDR r1, [PC, #0]
BLX r1
.word 0xC0FF1000
B .

.pool
.close
