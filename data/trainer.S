.3ds
.thumb
.create outfile,0

.align 4

;flag_colorfill equ 1

latch_data equ 4
latch_stat equ 8
latch_all  equ 12

gpio_rtc   equ (0xEB00000 + 0x10147100)
colorfill_top   equ 0x1ED02204

payload_addr equ 0x201000

rtcom_timeo_shift equ 15


; Entry

PUSH {r0 - r7}
MOV r6, r8
MOV r7, LR
B rtcom_entry

.func rtc_latch ; r0 = bits
    PUSH {r0, r1, r2, LR}
    
    MOV r1, #3
    LSL r1, #13
    ORR r0, r1 ; |= 0x6000 (???)
    
    LDR r1, =gpio_rtc
    
    rtc_latch_loop:
    STRH r0, [r1]
    LDRH r2, [r1]
    LSL r2, #17 ; if( & 0x4000)
    BMI rtc_latch_loop
    
    POP {r0, r1, r2, PC}

.func rtc_irq
    MOV r1, #0
    STRH r1, [r5, #0x00] ; r5 is guaranteed to point to RTC +0
    LSL r2, r0, #12 ; r0 is guaranteed to be 8 (latch_stat equ 8)
    STRH r2, [r5, #0x00]
    rtc_irq_subloop:
    LSR r2, #1 ; assert ARM7 IRQ spinwait
    BNE rtc_irq_subloop
    BX LR
    
    ;.align 4
    ;.pool
.endfunc

rtcom_entry:



rtcom_reset:

LDR r5, =gpio_rtc
MOV r4, #1
LSL r4, rtcom_timeo_shift

rtcom_restart:
    SUB r4, #1
    BEQ rtcom_kil
    
    LDRH r1, [r5, #0x12]
    MOV r3, r1
    
    LSR r1, #8
    BEQ rtcom_done ; status == 00h
    
  .ifdef flag_colorfill
    LDR r2, =colorfill_top
    MOV r1, #1
    LSL r1, #24
    ORR r1, r3
    STR r1, [r2]
  .endif
    
    LSR r1, r3, #8
    
    ADD r0, PC, #(rtcom_state - . - 2) & ~3
    
    CMP r1, #0x80
    BGE rtcom_specialhandle ; bit7 set
    
    MOV r2, #0
    STR r1, [r0, #0]
    STR r2, [r0, #4]
    B rtcom_cmdhandle ; bit7 clear
    
    rtcom_specialhandle:
        SUB r1, #0x80
        BEQ rtcom_restart ; status == 0x80 self ACK
        CMP r1, #2
        BEQ rtcom_restart ; status == 0x82 self NAK
        CMP r1, #3
        BEQ rtcom_einval  ; status == 0x83  req NAK
        
        CMP r1, #1
        BNE rtcom_kil ; status > 0x83 (invalid for now)
        
        ; status == 0x81 DS ACK
        LDR r1, [r0, #0]
        LDR r2, [r0, #4]
    
    rtcom_cmdhandle:
        
        ; Ping returns an error, so let's save space by not including it
        
        CMP r1, #0x41
        BEQ .cm41
        
        CMP r1, #0x42
        BEQ .cm42
        
        CMP r1, #0x44
        BEQ .cm44
        
        ; fallthru'
        
    rtcom_einval:
        MOV r1, #0x82
        B rtcom_latstat
        
    rtcom_retval:
        STRB r1, [r5, #0x12]
        MOV r0, latch_data
        BL rtc_latch
        
        MOV r1, #0x80
      rtcom_latstat:
        STRB r1, [r5, #0x13]
        MOV r0, latch_stat
        BL rtc_latch
        BL rtc_irq
        
        B rtcom_reset
        
    .cm41: ; UploadCode
        LDR r1, =payload_addr
        SUB r1, #4
        STRB r3, [r1, r2]
        ADD r2, #1
        STR r2, [r0, #4]
        B rtcom_retval
        
    .cm42: ; UserCodeRWX
        LDR r0, =payload_addr
        LDR r1, [r0, #-4]
        SWI 0x7A
        B rtcom_retval
    
    .cm44: ; UserCodeExecARM
        
        MOV r1, r2
        PUSH {r0, r2}
        LDR r2, =payload_addr
        
        MOV r0, r3
        LSL r0, #24
        LSR r0, #24
        
        BLX r2 ; r0 = data, r1 = iterator
        MOV r1, r0
        
        POP {r0, r3}
        ADD r3, #1
        STR r3, [r0, #4]
        CMP r4, #0 ; set r4 = 0 on return to return error
        BNE rtcom_retval
        B rtcom_einval

.align 4
.pool        
        
rtcom_kil:
    MOV r1, #0x00
    STRB r1, [r5, #0x13]
    MOV r0, latch_stat
    BL rtc_latch
    B rtcom_done

.align 4
rtcom_state:
    .word 0 ; cmd
    .word 0 ; offs

.align 4
rtcom_done:

.ifdef flag_colorfill
  LDR r2, =colorfill_top
  MOV r1, #0
  STR r1, [r2]
  MOV r1, r1
.endif

MOV r8, r6
MOV LR, r7
POP {r0 - r7}

B endpool
.pool

.align 4
endpool:
.close
