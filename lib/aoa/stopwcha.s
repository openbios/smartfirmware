/*
**	Called as start(argc, argv, envp)
*/


	.globl	 _stopwatchinit
_stopwatchinit:
	movw	$16,%ax
	movw	%ax,%gs

	movl	$960,%eax		;/* vector 0x78 * 8 bpv */
	movw	%gs:(%eax),%cx
	movw	%cx,sw_chain
	movw	%gs:6(%eax),%cx
	movw	%cx,sw_chain_hi
	movw	%gs:2(%eax),%cx
	movw	%cx,sw_chain_sel

	movl	$stopwatch_isr,%ecx
	movw	%cx,%gs:(%eax)
	movw	$64,%gs:2(%eax)		;/* selector 8 == 32-bit code */
	movw	$0x8f00,%gs:4(%eax)
	rorl	$16,%ecx
	movw	%cx,%gs:6(%eax)
	movw	%ds,%ax
	movw	%ax,%gs
	xorl    %eax,%eax
	movl    %eax,_stopwatchctr
	ret

stopwatch_isr:
	incl	_stopwatchctr
	ljmp	sw_chain

	.data

sw_chain:
	.short	0
sw_chain_hi:
	.short	0
sw_chain_sel:
	.short	0

	.globl	 _stopwatchctr
_stopwatchctr:
	.long	0
