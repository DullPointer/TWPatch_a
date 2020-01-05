.3ds
.thumb
.create outfile,0

.align 4

PUSH {r4, r5}

; 130000
MOV r0, #0x13
LSL r0, #16
SUB r0, #0xFF
SUB r0, #1

LDR r2, [PC, #((copylen - . - 2) & ~3)]
ADD r1, PC, #(payload_end - . - 2 + 4) & ~3

MOV r3, #0

copyloop:
LDR r4, [r1, r3]
STR r4, [r0, r3]

ADD r3, #4
CMP r3, r2
BLT copyloop

POP {r4, r5}

MOV r1, r2
SWI 0x7A


/*LDR r0, [PC, #((copylen - . - 2) & ~3)]
ADD r1, PC, #(payload_end - . - 2 + 4) & ~3
ADD r1, r0
MOV r0, #1
ORR r1, r0
BX r1
*/
B payload_end

.align	4
.pool

copylen:
    .word 0xC0FF1000

payload_end:
.close
