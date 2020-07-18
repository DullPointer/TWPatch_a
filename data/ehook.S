.3ds
.thumb
.create outfile,0

.align 4

PUSH {r0, LR}

LDR r0, =0x1FF85000
LDR r2, =0x1FF94000

LDR r1, [r0]
LSL r1, #12
LSR r1, #12
ADD r1, r2
ADD r1, #0x98

LDR r0, [r1]
CMP r0, #0
BNE nonzero_tls

MOV r2, #0x0B
MVN r2, r2
LSL r2, #12
STR r2, [r1]

nonzero_tls:

ADD r0, PC, #(getTLS - . - 2) & ~3
BLX r0

MOV r2, #1

ADD r1, PC, #(ehandler - . - 2) & ~3

STR r2, [r0, #0x44]
STR r2, [r0, #0x48]
STR r1, [r0, #0x40]

LDR r2, =0x1FFA7010

LDR r0, =0x46C0

MOV r1, #0
MVN r1, r1
STRB r1, [r2, #2]
MOV r1, #0
STRB r1, [r2, #0]
STRB r1, [r2, #1]

LDR r2, =0x1FFAB136
STRH r0, [r2]
LDR r2, =0x1FFAB350
STRH r0, [r2]
;LDR r2, =0x1FFAB36E
ADD r2, #(0x1FFAB36E - 0x1FFAB350)
STRH r0, [r2]
;LDR r2, =0x1FFAB37A
ADD r2, #(0x1FFAB37A - 0x1FFAB36E)
STRH r0, [r2]
;LDR r2, =0x1FFAB386
ADD r2, #(0x1FFAB386 - 0x1FFAB37A)
STRH r0, [r2]

/*
LDR r1, =0x60002000
LDR r2, =0x1002A4
LDR r0, =0x100000
STRH r1, [r2]

MOV r1, r0
LSR r1, #8
SWI 0x7A
LDR r0, =0x100000
MOV r1, r0
LSR r1, #8
ADD r0, r1
SWI 0x7A
*/

MOV r0, #4
MOV r1, #0
MOV r2, #0
SWI 0x7C


LDR r2, =0x1EC46000
MOV r0, #2
LDRH r1, [r2]
TST r1, r0
BNE testend

STR r0, [r0]

testend:


POP {r0, r1}
MOV LR, r1

B endpool
.pool

.align 4
.arm

getTLS:

MRC p15, 0, r0, c13, c0, 3
BX LR

ehandler:
MOV r0, #0
MOV r1, #0
MOV r2, #0
MOV r3, #0

LDR r0, =0x1ED02204
LDR r1, =0xEDB88320

MOV r2, #1
LSL r2, #24

strloop:
LSL r3, r1, #8
LSR r3, #8
ORR r3, r2
STR r3, [r0]

ROR r1, #1

B strloop

B ehandler

.pool

.align 4
endpool:
.close
