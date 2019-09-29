font_draw:
	nop
	sub r0, #(8 * 3)
	push	{r0 - r7, lr}
	mov	r2, #28
	mov	r5, #1
	sub	sp, #20
	str	r1, [sp, #12]
.L7:
	ldr	r4, [sp, #12]
	mov	r3, #15
	lsr	r4, r2
	and	r3, r4
	lsr	r6, r3, #3
	lsl	r7, r6, #8
	orr	r6, r7
	mov	r1, r6
	lsr	r7, r3, #1
	and	r7, r5
	lsl	r7, #9
	and	r4, r5
	lsr	r3, #2
	orr	r7, r1
	lsl	r4, #17
	and	r3, r5
	lsl	r3, #1
	orr	r4, r7
	orr	r4, r3
	lsl	r6, #16
	orr	r6, r4
	lsl	r3, r6, #2
	orr	r6, r3
	lsl	r3, r6, #4
	orr	r6, r3
	mov	r3, #180
	lsl	r3, #5
	add	r3, r0
	str	r6, [sp, #4]
	str	r3, [sp, #8]
.L5:
	mov	r7, r0
	mov	r4, r0
	add	r7, #24
.L2:
	mov	r3, #0
.L3:
	lsl	r1, r3, #3
	mov	r8, r1
	mov	r6, r8
	ldr	r1, [sp, #4]
	lsr	r1, r6
	strb	r1, [r4, r3]
	add	r3, #1
	cmp	r3, #3
	bne	.L3
	add	r4, #3
	cmp	r7, r4
	bne	.L2
	mov	r3, #180
	lsl	r3, #2
	add	r0, r3
	ldr	r3, [sp, #8]
	cmp	r0, r3
	bne	.L5
	cmp	r2, #0
	beq	.L1
	sub	r2, #4
	b	.L7
.L1:
	add	sp, #20
	pop	{r0 - r7, pc}
