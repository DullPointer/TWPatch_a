.3ds
.thumb
.create outfile,0

MOV r0, LR
B payload

.include "../test.S"

.func payload
    PUSH {r0, r2}
    LDR r1, =0x1EF00480
    ADD r0, PC, (dec_t - . - 2) & ~3
    BL ct_dec
    MOV r1, r0
    POP {r0, r2}
    MOV LR, r0
    ADD r2, PC, (dec_t - . - 2) & ~3
    ADD r1, #31
    LSR r1, #5
    LSL r1, #2
    ADD r2, r1
    ADD r2, #1
    BX r2


.align	4
.pool

.align 4
dec_t:
.close
