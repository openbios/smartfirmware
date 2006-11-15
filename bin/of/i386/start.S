/*  Copyright (c) 1992-2005 CodeGen, Inc.  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Redistributions in any form must be accompanied by information on
 *     how to obtain complete source code for the CodeGen software and any
 *     accompanying software that uses the CodeGen software.  The source code
 *     must either be included in the distribution or be available for no
 *     more than the cost of distribution plus a nominal fee, and must be
 *     freely redistributable under reasonable conditions.  For an
 *     executable file, complete source code means the source code for all
 *     modules it contains.  It does not include source code for modules or
 *     files that typically accompany the major components of the operating
 *     system on which the executable file runs.  It does not include
 *     source code generated as output by a CodeGen compiler.
 *
 *  THIS SOFTWARE IS PROVIDED BY CODEGEN AS IS AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  (Commercial source/binary licensing and support is also available.
 *   Please contact CodeGen for details. http://www.codegen.com/)
 */

/* (c) Copyright 1996,1998,2000 by CodeGen, Inc.  All Rights Reserved. */
/* Some code from OpenBSD's i386 biosboot.S to setup flat 32-bit mode.
   Some more code from LinuxBIOS's startup to try to init GX chip.
 */

#ifdef ROM
#	define addr32  .byte 0x67
#	define data32  .byte 0x66
#else
#	define addr32  /* */
#	define data32  /* */
#endif

#define PCI_CONFIG_ADDR		0xCF8
#define PCI_CONFIG_DATA		0xCFC
#define PCI_CFE				0x80000000	/* enable config space */

/* delay N us*/
#define DELAY(num)	\
        mov $num * 0x100, %ecx;	\
	0:  loop 0b;


	.text

/* entry point for both RAM and ROM images */
	.globl	_start
_start:
	.globl	start
start:


#ifdef ROM
	//#include "intel.h"
	//#include "intel_440ram.S"

//#if 0

#define CS_READ(which)  \
			 mov  $0x80000000,%eax ; \
			 mov  which,%ax ; \
			 and  $0xfc,%al            /* clear bits 1-0 */ ; \
			 mov  $0xcf8,%dx           /* port 0xcf8 ?*/ ; \
			 outl %eax,%dx             /* open up CS config */ ; \
			 add  $0x4,%dl             /* 0xcfc data port 0 */ ; \
			 mov  %which,%al ; \
			 and  $0x3,%al             /* only bits 1-0 */ ; \
			 add  %al,%dl ; \
			 inb  %dx,%al              /* read  */ ; \


#define CS_WRITE(which, data)  \
			 mov  $0x80000000,%eax       /* 32bit word with bit 31 set */ ; \
			 mov  which,%ax             /* put the reg# in the low part */ ; \
			 and  $0xfc,%al             /* dword align the reg# */ ; \
			 mov  $0xcf8,%dx            /* enable port  */ ; \
			 outl %eax,%dx ; \
			 add  $0x4,%dl             /* 1st data port */ ; \
			 mov  which,%ax             /* register# */ ; \
			 and  $0x3,%ax ; \
			 add  %al,%dl ; \
			 mov  data, %al ; \
			 outb %al,%dx                /* write to reg */ 

/* this sets up the intel 440 dram. */
	//CS_WRITE($0x7A, $0x00)	/* from errata - disable refresh? */

/*	now the fun begins.
	turn on the dramand wait a while (this from the intel book)
	turn power on and set the nop bit too
 */
	CS_WRITE($0x76, $0x30)
	//mov %eax, 0x400

	DELAY(0x200)

/* First, set registers we can set once to reasonable values  */
/*; note -- this code auto-generated ...*/
/*; note -- we need to grab the nice macro from the other chipset file.*/
/*; set it for sdram, 31 microsecond refresh (? ok? )*/
/*; should we be doing this just yet? who knows?*/
/*; turn on the power and set refresh clock to 31 us*/
	//CS_WRITE($0x50, $0x0)
	CS_WRITE($0x57, $0x12)		/* 0x12 */

/* for those registers that are 0 and default to 0 we're not going to */
/*; do the register set. We're seeing problems that may be timing, so the */
/*; less we poke at this thing the better. */
/*; WRONG. seems we need this?*/
	CS_WRITE($0x59, $0)

/*; bank size registers -- 20, then 20, then 40, then 40, 60, 60, 80, 80*/
/*; on our mboards. */
	CS_WRITE($0x60, $0x20)
	CS_WRITE($0x61, $0x20)
	CS_WRITE($0x62, $0x20)
	CS_WRITE($0x63, $0x20)
	CS_WRITE($0x64, $0x20)
	CS_WRITE($0x65, $0x20)
	CS_WRITE($0x66, $0x20)
	CS_WRITE($0x67, $0x20)
	
	//CS_WRITE($0x69, $0xA0)
	//CS_WRITE($0x6a, $0x0A)
	CS_WRITE($0x69, $0x00)
	CS_WRITE($0x6a, $0x00)
	CS_WRITE($0x6b, $0x00)
	CS_WRITE($0x6c, $0x00)
	CS_WRITE($0x6d, $0x00)
	CS_WRITE($0x6e, $0x00)

	//CS_WRITE($0x74, $0x22)
	//CS_WRITE($0x75, $0x22)
	CS_WRITE($0x74, $0x02)
	CS_WRITE($0x75, $0x00)

	//CS_WRITE($0x78, $0x07)
	//CS_WRITE($0x79, $0x55)
	CS_WRITE($0x78, $0x23)
	CS_WRITE($0x79, $0x01)

#if 0
	CS_WRITE($0xF4, $0x00)	/* from errata */
	CS_WRITE($0xF5, $0xF8)	/* from errata */
	CS_WRITE($0xF6, $0x00)	/* from errata */
	CS_WRITE($0xF7, $0x60)	/* from errata */
#endif

	/* from errata - enable refresh? */
	/*CS_WRITE($0x7A, $0x14)*/

#if 0	
/*; These regs read back as zero, but setting to zero is reserved (!)*/
/*; So I won't do anything for now and hope it improves things. */
/*; Based on the book, it seems they should be set to 'all 1', i.e.*/
/*; set for 100 mhz part. */
/*;	mov eax, 0x800000c8*/
/*;	out dx, eax*/
/*;	mov dx, 0xcfe*/
/*;	mov al, 0x0*/
/*;	out dx, al*/
/*;	mov dx, 0xcf8*/
/*;	mov eax, 0x800000c8*/
/*;	out dx, eax*/
/*;	mov dx, 0xcff*/
/*;	mov al, 0x0*/
/*;	out dx, al*/
#endif
	CS_WRITE($0xcc, $0x1e /*0x61*/)
		
/*; standard init of sdram at this point:*/
/*; it is on and the bridge is configured for 256M dimms. */
/*; now we have to do a precharge all (see the pc100 sdram book*/
/* thank goodness we're in protected mode! */
	mov $0xcf8, %dx
	movl $0x80000074, %eax
	outl %eax, %dx
/*; now we stick to reg 76, so no more of that*/
	mov $0xcfe, %dx

/* shortcut to reuse reg 0x76 still in CONFIG_ADDR */
#define PUT76(data) mov data, %al ;  outb %al, %dx

	//CS_WRITE($0x76, $0x30)
/* do the nop cycle -- this is a read*/
	mov 0x400, %eax

/*; now we have to kick the dram to do this? so what, I guess we hit it, */
/*; but we're not in bigreal, wonder what will happen. Just to bank 0 for*/
/*; now. A10 has to be high. (pc100 sdram book)*/
/*; this should be a Precharge all*/
	PUT76($0x50)
/* store to dram for the precharge */
	mov 0x400, %eax
/*; now we do the eight CBRs*/
	PUT76($0x90)
/*;        PORT_80 0xdf*/
	mov 0x400, %eax
	mov 0x400, %eax
	mov 0x400, %eax
	mov 0x400, %eax
	mov 0x400, %eax
	mov 0x400, %eax
	mov 0x400, %eax
	mov 0x400, %eax

/*; now a mode register set*/
	PUT76($0x70)
/*;        PORT_80 0xdf*/
/*; we have to do a memory reference that sets things up. */
/*; ok -- we go with burst length 4 (?) wrap type 0, ltmode 3*/
/*; ltmod 3 matches the other mode in the bridge. This is slow, */
/*; we fix later if possible*/
/*; now we reference the dram to issue the mrs*/
	movl 0x0150, %eax
	//mov %eax, 0x400

/*; well, maybe it is working now.*/
/*; delay a bit and go to normal mode*/
/*;        PORT_80 0xdf*/
	PUT76($0x17)
/*	mov $0113, %ax */
/*	outb %ax, %dx */
	//PUT76($0x10)
	//CS_WRITE($0x7A, $0x14)	/* from errata - enable refresh? */
//#endif // 0

#else	/* !ROM */
	/* load the gdtr */
	lgdt	Gdtr

	/* set the PE bit of CR0 */
	mov		%cr0, %eax 
	orl		$0x01, %eax 	/* CR0 PE */
	mov		%eax, %cr0

	/* make intrasegment jump to flush the processor pipeline and */
	/* reload CS register */
	jmp		j
  j:

	/* set up ds, ss, es, etc */
	movl	$0x10, %eax
	mov		%ax, %ds
	mov		%ax, %ss
	mov		%ax, %es
	mov		%ax, %fs
	mov		%ax, %gs
#endif	/* !ROM */

	/* init our stack */
	movl	$STACKADDR, %esp

	/* make sure we can read/write to RAM now */
	mov		$0xDEAD, %ax
	xor		%bx, %bx
	push	%ax
	pop		%bx
	cmp		%bx,%ax
die:
	jnz		die


#if 0
// try to initialize PIIX4 so we can get something out the serial port
//

#define CSR_W(dev, func, reg, val)	\
	mov  $0x80000000 | (dev << 11) | (func << 8) | reg ,%eax;	\
	mov  $0xcf8,%dx;	\
	outl %eax,%dx;	\
	mov  $val, %eax;	\
	add  $0x4,%dl;	\
	outl %eax,%dx

	/* PIIX4 power-management */
	CSR_W(7, 3, 0x50, 0x1D5800)
	CSR_W(7, 3, 0x54, 0x2124849)
	CSR_W(7, 3, 0x58, 0x2000004)
	CSR_W(7, 3, 0x5C, 0x10000002)
	CSR_W(7, 3, 0x60, 0x40E70290)
	CSR_W(7, 3, 0x40, 0x401)
	CSR_W(7, 3, 0x80, 0x01)
	CSR_W(7, 3, 0x90, 0x441)

	/* /pci/isa */
	CSR_W(7, 0, 0xC8, 0x25000000)
	CSR_W(7, 0, 0x80, 0x70000)
	CSR_W(7, 0, 0xB0, 0x904005)
	CSR_W(7, 0, 0xF8, 0xF30)

	/* /pci/host */
	CSR_W(0, 0, 0x50, 0x40C)
	CSR_W(0, 0, 0x54, 0x11000000)
	CSR_W(0, 0, 0x58, 0x111003)
	CSR_W(0, 0, 0x68, 0x0)
	CSR_W(0, 0, 0x70, 0x380A1F20)
	CSR_W(0, 0, 0x78, 0x38140123)
	CSR_W(0, 0, 0xF4, 0x6000F800)

	/* /pci/isa */
	CSR_W(7, 0, 0x4C, 0x01300000)
	CSR_W(7, 0, 0x64, 0x10)
	CSR_W(7, 0, 0x68, 0x80FE00)
	CSR_W(7, 0, 0x80, 0x70000)
	CSR_W(7, 0, 0xB0, 0x904005)
	CSR_W(7, 0, 0xC8, 0x25000000)
	CSR_W(7, 0, 0x68, 0x80FE00)
	CSR_W(7, 0, 0x74, 0xC0C0000)
	CSR_W(7, 0, 0x90, 0x0)


// initialize serial debug port - uses DX and AL
//
initsio:
	// set the serial port to a sane state


/* macros to access a 16550-compatible serial port for failsafe I/O */
#define COM1_PORT	0x3F8
#define COM2_PORT	0x2F8
#define SIO_THR	0x0		/* transmit holding register - write-only */
#define SIO_RBR	0x0		/* receiver buffer register - read-only */
#define SIO_DLL	0x0		/* divisor latch LSB - r/w - DLAB == 1 */
#define SIO_DLM	0x1		/* divisor latch MSB - r/w 0 DLAB == 1 */
#define SIO_IER	0x1		/* interrupt enable register - w/o */
#define SIO_IIR	0x2		/* interrupt identification reg - r/o */
#define SIO_FCR	0x2		/* FIFO control register - w/o */
#define SIO_LCR	0x3		/* line control register - r/w */
#define SIO_MCR	0x4		/* modem control register - r/w */
#define SIO_LSR	0x5		/* line status register - r/w */
#define SIO_MSR	0x6		/* modem status register - r/w */
#define SIO_SCR	0x7		/* scratch pad register - r/w */

#ifndef DEBUG_PORT
#define DEBUG_PORT	COM1_PORT
#endif /* DEBUG_PORT */

#define SIO_OUT(reg, val)	\
	mov		$val, %al;	\
	mov		$DEBUG_PORT + reg, %dx;	\
	out		%al, %dx;	\
	out		%al, $0xEB	/* IODELAY */

	// DLAB on
	SIO_OUT(SIO_LCR, 0x80);

	// clock value for 9600 baud
	SIO_OUT(SIO_DLL, 0x12);
	SIO_OUT(SIO_DLM, 0x00);

	// DLAB off, 8/1/N
	SIO_OUT(SIO_LCR, 0x03);

	// drop all lines, raise DTR
	SIO_OUT(SIO_MCR, 0x01);

	// disable all interrupts
	SIO_OUT(SIO_IER, 0x00);

	// turn on and clear I/O FIFOs
	SIO_OUT(SIO_FCR, 0x07);

again:
	// output character '?'
	SIO_OUT(SIO_THR, 0x3F);
	SIO_OUT(SIO_THR, 0x3F);
	SIO_OUT(SIO_THR, 0x3F);
	SIO_OUT(SIO_THR, 0x3F);

#endif /*0*/

	/* Set up a real frame, some day we will be doing returns */
	xorl	%eax,%eax			/* mark end of frames */
	movl	%eax,%ebp
	pushl	%ebp
	movl	%esp, %ebp

	/* Don't trust eflags. */
	pushl	$0x00000002		/* reserved default PSL flags */
	popfl

	mov		%esp,%eax

	call	main			/* and jump to C code */

	.globl _exit
_exit:
exit:
	jmp		exit


#ifndef ROM
	.align	4
1:		/* 0x00 : null */
	.long	0, 0
		/* 0x08 : flat code */
	.word	0xFFFF			# lolimit
	.word	0			# lobase
	.byte	0			# midbase
	.byte	31 | 0 | 0x80	# RWXAC, dpl = 0, present
	.byte	0xf | 0 | 0x40 | 0x80	# hilimit, xx, 32bit, 4k granularity
	.byte	0			# hibase
		/* 0x10 : flat data */
	.word	0xFFFF			# lolimit
	.word	0			# lobase
	.byte	0			# midbase
	.byte	19 | 0 | 0x80	# RWA, dpl = 0, present
	.byte	0xf | 0 | 0x40 | 0x80	# hilimit, xx, 32bit, 4k granularity
	.byte	0			# hibase

	.globl Gdtr
Gdtr:
	.word	. - 1b
	.long	1b
#endif /* !ROM */


/* dummy to make gcc happy */
	.globl	___main
___main:
	ret


	.globl	flush_caches
flush_caches:
	wbinvd
	ret


/* output AL to keyboard controller port in DX */
	.globl	kbd_out
kbd_out:
	out		%al, %dx
	out		%al,$0xEB	/* IODELAY */

	/* wait for I/O to be done */
.kbdout1:
	in		$0x64, %al
	out		%al, $0xEB	/* IODELAY */
	test	$0x02, %al
	jnz		.kbdout1

	ret
