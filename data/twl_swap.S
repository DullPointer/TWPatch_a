 .3ds
.thumb
.create outfile,0

.align 4

; ???
/*
LDR r0, =0x1209EC
LDR r2, =0x1F000000
LDR r3, =0x1F300000

STR r2, [r0, #0]
STR r3, [r0, #4]
STRH r1, [r0, #8]
*/

; ???
/*
LDR r0, =0x10008E ; TODO unhardcode
LDRB r1, [r0]     ; load offset byte

CMP r1, #0x80     ; prevent nullptr deref
BLT already_inited

; do screen swap

LSL r1, #2        ; (this << 2)
ADD r1, r0        ; + PC
MOV r3, #3
ADD r1, r3        ; + 3
BIC r1, r3        ; & ~3
LDR r2, [r1]      ; upscale disable
LDR r0, [r2, #12] ; TwlState*

; ???
MOV r1, #0xC0
LSL r1, #16
ADD r1, #0xFF
STR r1, [r0, #0x18]
*/

LDR r0, =0x10008E ; TODO unhardcode
LDRB r1, [r0]     ; load offset byte

CMP r1, #0x80     ; prevent nullptr deref
BLT payload_end

; do screen swap

LSL r1, #2        ; (this << 2)
ADD r1, r0        ; + PC
MOV r3, #3
ADD r1, r3        ; + 3
BIC r1, r3        ; & ~3
LDR r2, [r1]      ; upscale disable
LDR r0, [r2, #12] ; TwlState*

LDR r2, [r0, #(0x30 + (0 * 0x14) + 4)]
LDR r3, [r0, #(0x30 + (1 * 0x14) + 4)]
STR r3, [r0, #(0x30 + (0 * 0x14) + 4)]
STR r2, [r0, #(0x30 + (1 * 0x14) + 4)]

B payload_end

.align 4
.pool

payload_end:
.close
