.3ds
.thumb
.create outfile,0

.align 4

; LCD window

LDR r0, =#0x1ED02200
LDR r2, =#8
LDR r3, =#384 + 8 - 1
STR r2, [r0, #0x18]
STR r3, [r0, #0x1C]

; MTX setup

LDR r0, =#0x1EC11000

LDR r2, =#384 - 1 ; width
LDR r3, =#240 - 1 ; height
LSL r3, #16
ORR r2, r3
STR r2, [r0, #0x04]

; MTX matrix X

MOV r3, #0x03
LSL r3, #8
ORR r0, r3 ; 0x1EC1x300 ; X matrix

MOV r2, #6 - 1 ; width
MOV r3, #0b011011 ; bits
STR r2, [r0, #0x00] ; 5
STR r3, [r0, #0x04] ; /3

MOV r2, r0
ADD r2, #0x40 ; MTX_ARRAY

MOV r1, #0
MOV r3, #24
mtx_loop1:
STMIA r2!, {r1}
SUB r3, #1
BNE mtx_loop1

MOV r1, #0x40
LSL r1, #8
MOV r3, #8
mtx_loop2:
STMIA r2!, {r1}
SUB r3, #1
BNE mtx_loop2

MOV r1, #0
MOV r3, #16
mtx_loop3:
STMIA r2!, {r1}
SUB r3, #1
BNE mtx_loop3

B payload_end

.align	4
.pool

payload_end:
.close
