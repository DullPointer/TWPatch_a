.3ds
.thumb
.create outfile,0

.align 4

B already_inited
B already_inited
B already_inited
B already_inited

.align 4

.func draw_hex ; r0 fb, r1 val
    ;.include "../fontest.S"
    .include "../fontest_fontable.S"

.align 4
fonttable:
    
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

PUSH {r4, r5, r6, r7}
MOV r6, LR
MOV r7, r8

/*
ADD r5, PC, #(initflag - . - 2) & ~3

LDR r0, =0x1EC46000
MOV r1, #1 ; R button
LSL r1, #8
ADD r1, #2 ; B button
LDRH r2, [r0]
MVN r2, r2
TST r2, r1
BEQ skip_btn

LDR r3, [r5, #0]
CMP r3, #0
BNE cont_draw

LDR r4, [r5, #4]
ADD r4, #0xFF
ADD r4, #1

MOV r3, #1
LSL r3, #8
TST r2, r3
BNE str_addr
SUB r4, #0xC0
str_addr:
STR r4, [r5, #4]

MOV r1, #1

B cont_draw

skip_btn:
MOV r2, #0

cont_draw:
STR r2, [r5, #0]
*/


LDR r0, =0x1EF00400 + 0x40
LDR r2, [r0, #0x78 - 0x40]
MOV r3, #1
AND r2, r3
LSL r2, #2
ADD r0, #0x68 - 0x40
ADD r2, r0
LDR r0, [r2] ; TODO: FIX THIS CRASH

LSL r0, #8
LSR r0, #8

LDR r2, =0x1F000000 + 0x2D000 + 0xCA80 + (0xF0 * 3)
ADD r0, r2
MOV r3, r0

;LDR r4, =0x1EC41000 + 0x100
;LDR r4, =0x1FFFFFA0 - 0x1FFAC000 + 0x100000
;LDR r4, =0x1FFB6800 - 0x1FF84800 + 0x100000
LDR r4, [PC, #(initflag - . - 2 + 4) & ~3]
MOV r5, #0

/*
LDR r1, =0x76543210
BL draw_hex

LDR r1, =0xFEDCBA98
BL draw_hex
*/

;SUB r0, #0x20

print_again:
LDR r1, [r4, r5]
BL draw_hex
ADD r5, #4
CMP r5, #(4 * 16)
BNE print_again


/*LDR r2, =(9 * 8 * 240 * 3) - (22 * 8 * 3)
SUB r0, r2
MOV r2, #0

LDR r1, [PC, #(initflag - . - 2) & ~3]
BL draw_hex
*/

MOV r1, r3
LDR r0, =0xFFFF8001
;MOV r2, #1
;LSL r2, #16
MOV r2, #0x46
LSL r2, #12
SWI 0x54 ; FlushProcessDataCache(Handle, void*, u32)

MOV LR, r6
MOV r8, r7
POP {r4, r5, r6, r7}

B payload_end

.align	4
.pool

initflag:
    .word 0
    ;.word 0xEB00000 + 0x10147100;0x1EC41100 ;0x12F000 ;0x1FFB6800 - 0x1FF84800 + 0x100000
    ;.word 0x1273AC
    ;.word 0x129EE8 + (4 * 32)
    .word 0x1ED01000

.align 4
payload_end:
.close
