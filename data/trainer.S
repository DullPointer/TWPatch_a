.3ds
.thumb
.create outfile,0

.align 4

latch_data equ 4
latch_stat equ 8
latch_all  equ 12

rtcom_timeo_shift equ 14


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
    
    rtc_latch_loop:
    STRH r0, [r1]
    LDRH r2, [r1]
    LSL r2, #(31 - 14)
    BMI rtc_latch_loop
    
    POP {r0 - r3}
    BX LR
    
    .align 4
    .pool
.endfunc

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
    MOV r3, r1
    
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
        
        CMP r1, #1
        BNE rtcom_kil ; status >= 0x83 (invalid for now)
        
        ; status == 0x81 DS ACK
        LDMIA r0!, {r1, r2}
        SUB r0, #8
    
    rtcom_cmdhandle:
        
        ; Ping returns an error, so let's save space by not including it
        
        CMP r1, #2
        BEQ .cm02
        
        ;CMP r1, #0x40
        ;BEQ .cm40
        
        CMP r1, #0x41
        BEQ .cm41
        
        CMP r1, #0x42
        BEQ .cm42
        
        CMP r1, #0x44
        BEQ .cm44
        
        ; fallthru'
        
    rtcom_einval:
        MOV r0, #0x82
        B rtcom_latstat
        
    rtcom_retval:
        STRB r1, [r5, #0]
        MOV r0, latch_data
        BL rtc_latch
        
        MOV r0, #0x80
      rtcom_latstat:
        STRB r0, [r5, #1]
        MOV r0, latch_stat
        BL rtc_latch
        
        B rtcom_reset
        
        /*
    .cm02: ; TestCPad
        CMP r2, #3
        BGE rtcom_einval
        
        LDR r3, =0x127384
        LDRB r1, [r3, r2]
        ADD r2, #1
        STR r2, [r0, #4]
        B rtcom_retval
        */
        
    .cm02:
        MOV r1, #0x77
        B rtcom_retval
        
    .cm40: ; GetProcAddress
        ;B rtcom_einval
        
    .cm41: ; UploadCode
        MOV r1, r3
        LDR r3, =0x200FFC
        STRB r1, [r3, r2]
        ADD r2, #1
        STR r2, [r0, #4]
        B rtcom_retval
        
    .cm42: ; UserCodeRWX
        LDR r0, =0x200FFC
        ;LDR r1, [r0]
        ADD r0, #4
        MOV r1, #8
        LSL r1, #12
        SWI 0x7A
        B rtcom_retval
    
    .cm44: ; UserCodeExecARM
        MOV r1, r2
        PUSH {r0, r2}
        LDR r2, =0x200FFC
        MOV r0, r3
        LSL r0, #24
        LSR r0, #24
        ADD r2, #4
        
        BLX r2
        MOV r1, r0
        
        POP {r0, r2}
        ADD r2, #1
        STR r2, [r0, #4]
        B rtcom_retval
        
        
rtcom_kil:
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

POP {r0 - r7}
MOV r8, r0
MOV LR, r1
POP {r0, r1}
.close
