.3ds
.thumb
.create outfile,0

.align 4

latch_data equ 4
latch_stat equ 8
latch_all  equ 12

rtcom_timeo_shift equ 20

NOP
NOP
NOP
PUSH {r0, r1}
MOV r0, r8
MOV r1, LR
PUSH {r0 - r7}
B rtcom_reset

.func rtc_latch ; r0 = bits
    PUSH {r0 - r3}
    
    MOV r1, #3
    LSL r1, #14
    ORR r0, r1
    
    LDR r1, =0xEB00000 + 0x10147100
    NOP
    
    rtc_latch_loop:
    STRH r0, [r1]
    LDRH r2, [r1]
    LSL r2, #(31 - 14)
    BMI rtc_latch_loop
    
    POP {r0 - r3}
    BX LR
    
    .align 4
    .pool

.align 4

rtcom_reset:

LDR r5, =0xEB00000 + 0x10147112
MOV r4, #1
LSL r4, rtcom_timeo_shift
MOV r1, #0

rtcom_restart:
    SUB r4, #1
    BEQ rtcom_kil
    
    LDRH r1, [r5]
    ;MOV r0, r1
    
    LSR r1, #8
    BEQ rtcom_done ; status == 00h
    
    ADD r0, PC, #(rtcom_state - . - 2) & ~3
    
    CMP r1, #0x80
    BGE rtcom_specialhandle ; bit7 set
    
    MOV r2, #0
    STMIA r0!, {r1, r2}
    SUB r0, #8
    B rtcom_cmdhandle ; bit7 clear
    
    rtcom_specialhandle:
        SUB r1, #0x80
        BEQ rtcom_restart ; status == 0x80 self ACK
        CMP r1, #2
        BEQ rtcom_restart ; status == 0x82 self NAK
        
        SUB r1, #3
        BGE rtcom_kil ; status >= 0x83 (invalid for now)
        
        ; status == 0x81 DS ACK
        LDMIA r0!, {r1, r2}
        SUB r0, #8
    
    rtcom_cmdhandle:
        
        CMP r1, #1
        BEQ .cm01
        
        CMP r1, #2
        BEQ .cm02
        
        CMP r1, #0x40
        BEQ .cm40
        
        ; fallthru'
        
    rtcom_einval:
        MOV r1, #0x82
        STRB r1, [r5, #1]
        MOV r0, latch_stat
        BL rtc_latch
        B rtcom_reset
        
    rtcom_retval:
        STRB r1, [r5, #0]
        MOV r0, latch_data
        BL rtc_latch
        
        MOV r0, #0x80
        STRB r0, [r5, #1]
        MOV r0, latch_stat
        BL rtc_latch
        
        B rtcom_reset
        
    .cm01:
        B rtcom_einval
        
    .cm02:
        CMP r2, #3
        BGE rtcom_einval
        
        LDR r3, =0x127384
        LDRB r1, [r3, r2]
        ADD r2, #1
        STR r2, [r0, #4]
        B rtcom_retval
        
    .cm40:
        
        B rtcom_einval
        
    .cm41:
        
        B rtcom_einval
        
rtcom_kil:
    ADD r0, PC, #(rtcom_state - . - 2) & ~3
    MOV r1, #0x00
    STRB r1, [r5, #1]
    MOV r0, latch_stat
    BL rtc_latch
    B rtcom_done

.align 4
rtcom_state:
    .word 0 ; cmd
    .word 0 ; offs

.pool

.align 4
rtcom_done:
NOP
NOP
NOP
NOP

POP {r0 - r7}
MOV r8, r0
MOV LR, r1
POP {r0, r1}

NOP
NOP
NOP
NOP
.close
