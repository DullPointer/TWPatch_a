.thumb
.func ct_dec
    mov r3, r8
	push	{r2, r3, r4, r5, r6, r7}
	mov	r3, #0
	mov	r4, r1
	sub	sp, sp, #20
	mov	r5, #128
	str	r0, [sp, #4]
	mov	r7, #1
	mov	r0, r3
	add	r4, #252
	str	r3, [r1]
	lsl	r5, r5, #1
	str	r3, [r4, #4]
	str	r4, [sp, #12]
.L2:
	mov	r2, #0
.L7:
	mov	r4, r7
	lsl	r4, r2
	str	r4, [sp]
.L5:
	lsr	r6, r3, #5
	lsl	r4, r6, #2
	str	r4, [sp, #8]
	mov	r4, #31
	and	r4, r3
	mov	r8, r4
	mov	r4, r7
	mov	r6, r8
	lsl	r4, r6
	mov	r8, r4
	ldr	r6, [sp, #8]
	ldr	r4, [sp, #4]
	add	r3, r3, #1
	ldr	r6, [r6, r4]
	mov	r4, r8
	tst	r6, r4
	beq	.L9
	ldr	r4, [sp]
	add	r0, r0, r4
	b	.L5
.L9:
	add	r2, #8
	cmp	r2, #24
	bne	.L7
	ldr	r2, [sp, #12]
	sub	r5, r5, #1
	str	r0, [r1, #4]
	str	r0, [r2, #8]
	cmp	r5, #0
	bne	.L2
	add	sp, sp, #20
	;@ sp needed
    mov r0, r3
	pop	{r2, r3, r4, r5, r6, r7}
    mov r8, r3
    bx lr
