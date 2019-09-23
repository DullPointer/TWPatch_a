font_draw:
    SUB r0, #8 * 3
	push	{r0 - r7}
	mov	r4, #28
	str	r1, [sp, #4]
.L13:
	ldr	r5, [sp, #4]
	mov	r3, #15
	lsr	r5, r4
	mov	r2, #1
	and	r3, r5
	lsr	r3, #3
	lsl	r3, #1
	add	r3, #5
	lsl	r2, r3
	mov	r3, r2
	mov	r2, #180
	lsl	r2, #5
	add	r2, r0
	str	r2, [sp, #8]
	mov	r2, #4
	mov	r7, #0
	tst	r5, r2
	beq	.L3
	mov	r7, r3
.L3:
	mov	r2, r3
	mov	r1, #2
	lsl	r7, #8
	orr	r2, r7
	tst	r5, r1
	bne	.L5
	mov	r2, r7
.L5:
	mov	r1, #1
	lsl	r2, #8
	orr	r3, r2
	tst	r5, r1
	bne	.L11
	mov	r3, r2
.L11:
	mov	r7, r0
	mov	r5, r0
	add	r7, #24
.L8:
	mov	r2, #0
.L9:
	lsl	r1, r2, #3
	mov	r8, r1
	mov	r1, r3
	mov	r6, r8
	lsr	r1, r6
	strb	r1, [r5, r2]
	add	r2, #1
	cmp	r2, #3
	bne	.L9
	add	r5, #3
	cmp	r7, r5
	bne	.L8
	mov	r2, #180
	lsl	r2, #2
	add	r0, r2
	ldr	r2, [sp, #8]
	cmp	r0, r2
	bne	.L11
	cmp	r4, #0
	beq	.L1
	sub	r4, #4
	b	.L13
.L1:
	pop	{r0 - r7}
	bx	lr
