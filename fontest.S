font_draw:
    SUB r0, #0x20
	push	{r0 - r7}
	push	{r0, r1}
	mov	r4, #180
	lsl	r4, #5
	mul	r2, r4
	mov	r4, #28
	lsl	r3, #5
	add	r2, r3
	add	r0, r2
.L7:
	mov	r3, r1
	mov	r2, #15
	lsr	r3, r4
	and	r3, r2
	mov	r2, #0
	lsl	r3, #3
	str	r3, [sp, #0]
.L5:
	add	r5, PC, (.L11 - . - 2) & ~3
	ldr	r3, [sp, #0]
	add	r3, r5
	ldrb	r3, [r3, r2]
	mov	r5, #0
	mov	r8, r3
	mov	r3, #180
	lsl	r3, #2
	mul	r3, r2
	add	r3, r0
.L4:
	mov	r6, r8
	mov	r7, #1
	asr	r6, r5
	and	r6, r7
	beq	.L2
	mov	r6, #1
	neg	r6, r6
.L2:
	add	r5, #1
	str	r6, [r3]
	add	r3, #4
	cmp	r5, #8
	bne	.L4
	add	r2, #1
	cmp	r2, #8
	bne	.L5
	mov	r3, #180
	lsl	r3, #5
	add	r0, r3
	cmp	r4, #0
	beq	.L1
	sub	r4, #4
	b	.L7
.L1:
	pop	{r0, r1}
	pop	{r0 - r7}
    bx lr
.L11:
	.align	4
