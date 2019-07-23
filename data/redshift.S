.3ds
.thumb
.create outfile,0

MOV r1, r0
B payload

.include "../test.S"

.func payload
    PUSH {r0, r1}
    LDR r1, =0x1EF00480
    ADD r0, PC, dec_t - . - 4
    BL ct_dec
    POP {r0, r1}
    BX r0


.align	4
.pool

.align 4
dec_t:
    .word 0xC0FF1000
    .word 0xC0CFAFEE
.close
