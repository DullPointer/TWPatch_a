.3ds
.thumb
.create outfile,0

.align 4

;ADD r0, PC, #((initflag - . - 2) & ~3)
;LDR r1, [r0]
;CMP r1, #0
;BNE sk_already_inited
B sk_already_inited

; runonce
    SUB r1, #1
    STR r1, [r0]
    
    /*
    LDR r0, =0x1209EC
    LDR r2, =0x1F000000
    LDR r3, =0x1F300000
    
    STR r2, [r0, #0]
    STR r3, [r0, #4]
    STRH r1, [r0, #8]
    
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
    
    */
    /*
    MOV r1, #0xC0
    LSL r1, #16
    ADD r1, #0xFF
    
    STR r1, [r0, #0x18]
    */
    
sk_already_inited:
    B already_inited

.align 4
.pool

.align 4
.func draw_hex ; r0 fb, r1 val, r2 y, r3 x
    .include "../fontest.S"

.align 4
fonttable:
    ;.db 0b00011110,0b00110011,0b00110111,0b00111111,0b00111011,0b00110011,0b00011110,0b00000000
    ;.db 0b00001100,0b00011100,0b00111100,0b00001100,0b00001100,0b00001100,0b00001100,0b00000000
    ;.db 0b00011110,0b00110011,0b00000011,0b00000110,0b00001100,0b00011000,0b00111111,0b00000000
    ;.db 0b00011110,0b00110011,0b00000011,0b00001110,0b00000011,0b00110011,0b00011110,0b00000000
    ;.db 0b00001110,0b00011110,0b00110110,0b01100110,0b01111111,0b00000110,0b00000110,0b00000000
    ;.db 0b00111111,0b00110000,0b00111110,0b00000011,0b00000011,0b00110011,0b00011110,0b00000000
    ;.db 0b00001110,0b00011000,0b00110000,0b00111110,0b00110011,0b00110011,0b00011110,0b00000000
    ;.db 0b00111111,0b00000011,0b00000011,0b00000110,0b00001100,0b00001100,0b00001100,0b00000000
    ;.db 0b00011110,0b00110011,0b00110011,0b00011110,0b00110011,0b00110011,0b00011110,0b00000000
    ;.db 0b00011110,0b00110011,0b00110011,0b00011111,0b00000011,0b00000110,0b00011100,0b00000000
    ;.db 0b00011110,0b00110011,0b00110011,0b00111111,0b00110011,0b00110011,0b00110011,0b00000000
    ;.db 0b00111110,0b00110011,0b00110011,0b00111110,0b00110011,0b00110011,0b00111110,0b00000000
    ;.db 0b00001111,0b00011000,0b00110000,0b00110000,0b00110000,0b00011000,0b00001111,0b00000000
    ;.db 0b00111100,0b00110110,0b00110011,0b00110011,0b00110011,0b00110110,0b00111100,0b00000000
    ;.db 0b00111111,0b00110000,0b00110000,0b00111100,0b00110000,0b00110000,0b00111111,0b00000000
    ;.db 0b00111111,0b00110000,0b00110000,0b00111100,0b00110000,0b00110000,0b00110000,0b00000000
    
    .db 0b00000000,0b00111110,0b01111111,0b01001101,0b01011001,0b01111111,0b00111110,0b00000000
    .db 0b00000000,0b00010000,0b00110000,0b01111111,0b01111111,0b00000000,0b00000000,0b00000000
    .db 0b00000000,0b00100001,0b01100011,0b01000111,0b01001101,0b01111001,0b00110001,0b00000000
    .db 0b00000000,0b00100010,0b01100011,0b01001001,0b01001001,0b01111111,0b00110110,0b00000000
    .db 0b00001100,0b00011100,0b00110100,0b01100100,0b01111111,0b01111111,0b00000100,0b00000000
    .db 0b00000000,0b01110010,0b01110011,0b01010001,0b01010001,0b01011111,0b01001110,0b00000000
    .db 0b00000000,0b00011110,0b00111111,0b01101001,0b01001001,0b01001111,0b00000110,0b00000000
    .db 0b00000000,0b01000000,0b01000000,0b01000111,0b01001111,0b01111000,0b01110000,0b00000000
    .db 0b00000000,0b00110110,0b01111111,0b01001001,0b01001001,0b01111111,0b00110110,0b00000000
    .db 0b00000000,0b00110000,0b01111001,0b01001001,0b01001011,0b01111110,0b00111100,0b00000000
    .db 0b00000000,0b00111111,0b01111111,0b01001000,0b01001000,0b01111111,0b00111111,0b00000000
    .db 0b00000000,0b01111111,0b01111111,0b01001001,0b01001001,0b01111111,0b00110110,0b00000000
    .db 0b00000000,0b00011100,0b00111110,0b01100011,0b01000001,0b01000001,0b01000001,0b00000000
    .db 0b00000000,0b01111111,0b01111111,0b01000001,0b01100011,0b00111110,0b00011100,0b00000000
    .db 0b00000000,0b01111111,0b01111111,0b01001001,0b01001001,0b01000001,0b01000001,0b00000000
    .db 0b00000000,0b01111111,0b01111111,0b01001000,0b01001000,0b01000000,0b01000000,0b00000000
    
    .pool

.align 4

already_inited:

/*
LDR r0, =0x1EC46000
MOV r1, #(1 << 1) ; B button
LDRH r2, [r0]
TST r2, r1
BNE skip_fbswap ; pressing button grounds it
*/

PUSH {r4, r5, r6, r7}
MOV r6, LR
MOV r7, r8

LDR r0, =0x1EF00400 + 0x40
LDR r2, [r0, #0x78 - 0x40]
MOV r3, #1
AND r2, r3
LSL r2, #2
ADD r0, #0x68 - 0x40
ADD r2, r0
LDR r0, [r2]

LSL r0, #8
LSR r0, #8
MOV r2, #0x1F
LSL r2, #24
ADD r0, r2

MOV r2, #0x2D
LSL r2, #12
ADD r0, r2

;this block might cause crash
;no crash, but no video
MOV r2, #0xCA
LSL r2, #1
ADD r2, #1
LSL r2, #7
ADD r0, r2
SUB r0, #0xF0
SUB r0, #0xF0
SUB r0, #0xF0

MOV r2, #0
MOV r3, #0

LDR r4, =0x1EC41100
MOV r5, #0

print_again:
LDRH r1, [r4]
BL draw_hex
ADD r4, #2
ADD r5, #2
CMP r5, #0x14
BNE print_again


;SUB r0, #0x20

;0x1EC47100 2
;0x1EC47110 1
;0x1EC47111 1
;0x1EC47120 4
;0x1EC47124 4
;0x1EC47130 4
;0x1EC47134 4
;0x1EC47140 4
;0x1EC47150 1
;0x1EC47151 1
;0x1EC47160 4
;0x1EC47164 4

LDR r4, =0x1EC47100
LDRH r1, [r4]
BL draw_hex

ADD r4, #0x10 ; 10h
LDRB r1, [r4]
BL draw_hex

ADD r4, #0x01 ; 11h
LDRB r1, [r4]
BL draw_hex

ADD r4, #0x0F ; 20h
LDR r1, [r4]
BL draw_hex

ADD r4, #0x04 ; 24h
LDR r1, [r4]
BL draw_hex

ADD r4, #0x0C ; 30h
LDR r1, [r4]
BL draw_hex

ADD r4, #0x04 ; 34h
LDR r1, [r4]
BL draw_hex

ADD r4, #0x0C ; 40h
LDR r1, [r4]
BL draw_hex

ADD r4, #0x10 ; 50h
LDRB r1, [r4]
BL draw_hex

ADD r4, #0x01 ; 51h
LDRB r1, [r4]
BL draw_hex

ADD r4, #0x0F ; 60h
LDR r1, [r4]
BL draw_hex

ADD r4, #0x04 ; 64h
LDR r1, [r4]
BL draw_hex


MOV r1, r4
LDR r0, =0xFFFF8001
MOV r2, #1
LSL r2, #16
SWI 0x54 ; FlushProcessDataCache(Handle, void*, u32)

MOV LR, r6
MOV r8, r7
POP {r4, r5, r6, r7}

/*
LDR r0, =0x10008E ; TODO unhardcode
LDRB r1, [r0]     ; load offset byte

CMP r1, #0x80     ; prevent nullptr deref
BLT hole_end

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
*/

skip_fbswap:

hole_end:

; fix LR

MOV r0, LR

;ADD r1, PC, #0
;LSR r1, #20
;CMP r1, #3
;BNE payload_end ; do not fuck up LR if already relocated

; only fix LR when running in 300000h region
ADD r0, #4
MOV LR, r0

B payload_end

.align	4
.pool

initflag:
    .word 0

payload_end:
.close
