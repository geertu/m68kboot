
export test_register

	text

test_register:
	move.l	a2,-(sp)
	move.l	sp,a2
	move.l	$8,a1
	move.w	sr,d1
	or.w	#$700,sr
	move.l	#berr,$8

	moveq	#0,d0
	tst.b	(a0)
	nop
	moveq	#1,d0
berr:
	move.l	a2,sp
	move.l	a1,$8
	move.w	d1,sr
	move.l	(sp)+,a2
	rts

