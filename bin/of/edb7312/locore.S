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

/* (c) Copyright 1999 by CodeGen, Inc.  All Rights Reserved. */
/* This code is copied and hacked from the OpenBSD ARM locore.S.
   Much has been eliminated for SmartFirmware. */

/* $NetBSD: locore.S,v 1.8 1996/03/20 18:57:58 mark Exp $ */

/*
 * Copyright (C) 1994 Mark Brinicombe
 * Copyright (C) 1994 Brini
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Brini.
 * 4. The name of Brini may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/* register equates */
fp	.req	r11
ip	.req	r12
sp	.req	r13
lr	.req	r14
pc	.req	r15

#define PSR_MODE_MASK	0x1F
#define PSR_MODE_USR	0x10

#define CLIENT_SPSR	0
#define CLIENT_R0	4
#define CLIENT_R1	8
#define CLIENT_R2	12
#define CLIENT_SP	56
#define CLIENT_LR	60
#define CLIENT_PC	64
#define CLIENT_SSP	68
#define CLIENT_CPSR	72

#if 0
savestate(state_t *statep)	/* R0 and PC have already been saved */
	/* save R1-R7 */
	/* save SPSR */
	/* save CPSR */
	/* switch to prior mode */
	/* save R8-R14 */
	/* switch back */
	/* return */

restorestate(state_t *statep)	/* restore all but R0 and PC */
	/* restore SPSR */
	/* restore CPSR */
	/* switch to prior mode */
	/* restore R2-R14 */
	/* switch back */
	/* restore R1 */
	/* return */

#define SAVECLIENT
	str	r0,r0_save		\
	ldr	r0,g_clientstate	\
	str	r14,[r0,#CLIENT_PC];	\
	ldr	r14,r0_save		\
	str	r14,[r0,#CLIENT_R0];	\
	bl	savestate

#define RESTORESTATE
	ldr	r0,g_clientstate	\
	bl	restorestate		\
	ldr	r14,[r0,#CLIENT_PC];	\
	ldr	r0,[r0,#CLIENT_R0];	
#endif

#define	SAVECLIENT			\
	str	r0,r0_save;		\
	ldr	r0,g_clientstate;	\
	str	r14,[r0,#CLIENT_PC];	\
	add	r0,r0,#CLIENT_R1;	\
	stmia	r0,{r1-r12};		\
	add	r0,r0,#(CLIENT_SP-CLIENT_R1);	\
        stmia   r0,{r13-r14}^;         \
        mov     r0,r0;                 \
	sub	r0,r0,#CLIENT_SP;	\
	mrs	r1,spsr_all;		\
	str	r1,[r0,#CLIENT_SPSR];	\
	ldr	r1,r0_save;		\
	str	r1,[r0,#CLIENT_R0];	\

#define RESTORECLIENT			\
	ldr	r1,[r0,#CLIENT_SPSR];	\
	msr	spsr_all,r1;		\
	add	r14,r0,#CLIENT_R0;	\
	ldmia	r14,{r0-r14}^;		\
	ldr	lr,[r0,#CLIENT_PC];	\
	mov	r0,r0			\


#define PAGETABLE_L1_BASE	0xC00FC000

	.text
	.global	exception_vectors
exception_vectors:
	/* exception vectors at the bottom of RAM/ROM */
	ldr	pc,start_addr		/* reset */
	ldr	pc,undefinsn_addr	/* undefined instruction */
	ldr	pc,swtrap_addr		/* software trap */
	ldr	pc,abortpre_addr	/* abort prefetch */
	ldr	pc,abortdata_addr	/* abort data */
	ldr	pc,reserved_addr	/* reserved */

	ldr	pc,irq_addr			/* IRQ */
	ldr	pc,fiq_addr			/* FIQ */
	/* END exception_vectors */

	.word		0			/* pad out to 64 bytes */
	.word		0
	.word		0
	.word		0

	.word		0
	.word		0
	.word		0
	.word		0

#ifdef ROM
#define ROM_OFFSET	0x08FE0000
#else
#define ROM_OFFSET	0
#endif
start_addr:
	.word	start + ROM_OFFSET
undefinsn_addr:
	.word	undefinsn_exception + ROM_OFFSET
swtrap_addr:
	.word	swtrap_exception + ROM_OFFSET
abortpre_addr:
	.word	abortpre_exception + ROM_OFFSET
abortdata_addr:
	.word	abortdata_exception + ROM_OFFSET
reserved_addr:
	.word	reserved_exception + ROM_OFFSET
irq_addr:
	.word	interrupt + ROM_OFFSET
fiq_addr:
	.word	interrupt + ROM_OFFSET

	.word		0			/* pad out to 64 bytes */
	.word		0
	.word		0
	.word		0

	.word		0
	.word		0
	.word		0
	.word		0

	.global		g_interrupt_vectors
g_interrupt_vectors:
	.word		0		/* 0 */
	.word		0
	.word		0
	.word		0
	.word		0		/* 4 */
	.word		0
	.word		0
	.word		0
	.word		0		/* 8 */
	.word		0
	.word		0
	.word		0
	.word		0		/* 12 */
	.word		0
	.word		0
	.word		0
	.word		0		/* 16 */
	.word		0
	.word		0
	.word		0
	.word		0		/* 20 */
	.word		0
	.word		0
	.word		0
	.word		0		/* 24 */
	.word		0
	.word		0
	.word		0
	.word		0		/* 28 */
	.word		0
	.word		0
	.word		0
	.word		0		/* 32 */
	.word		0
	.word		0
	.word		0
	.word		0		/* 36 */
	.word		0
	.word		0
	.word		0
	.word		0		/* 40 */
	.word		0
	.word		0
	.word		0
	.word		0		/* 44 */
	.word		0
	.word		0
	.word		0
	.word		0		/* 48 */
	.word		0
	.word		0
	.word		0
	.word		0		/* 52 */
	.word		0
	.word		0
	.word		0
	.word		0		/* 56 */
	.word		0
	.word		0
	.word		0
	.word		0		/* 60 */
	.word		0
	.word		0
	.word		0

	.global		g_image_size
g_image_size:
	.word		0			/* size of ROM image */

	.global		g_image_checksum
g_image_checksum:
	.word		0			/* Checksum of ROM image */


	.global	start
start:
	/* initialize stack pointers for various modes */
	/* Stack locations:
	 *	IRQ		F70FAC00-F70FAFFF	TOS
	 *	FIRQ		F70FA800-F70FABFF	TOS-1024
	 *	Abort		F70FA400-F70FA7FF	TOS-2048
	 *	Undef		F70FA000-F70FA3FF	TOS-3072
	 *	SVC		F70F8800-F70F9FFF	TOS-4096
	 *	Callback	F70F8800-F70F87FF	TOS-10240
	 */
	mov		r0, #0xD2			/* IRQ mode, no intrs */
	msr		cpsr_all, r0
    	ldr    		sp, tos

	mov		r0, #0xD1			/* FIRQ mode, no intrs */
	msr		cpsr_all, r0
    	ldr    		sp, tos
	sub		sp, sp, #1024

	mov		r0, #0xD7			/* Abort mode, no intrs */
	msr		cpsr_all, r0
    	ldr 		sp, tos
	sub		sp, sp, #2048

	mov		r0, #0xDB			/* Undef mode, no intrs */
	msr		cpsr_all, r0
	ldr 		sp, tos
	sub		sp, sp, #3072

	mov		r0, #0x13			/* SVC mode, FIRQ/IRQ on */
	msr		cpsr_all, r0
	mov		fp, #0x00000000		/* trace back starts here */
	ldr  		sp, tos
	sub		sp, sp, #4096
	b		init2
tos:
	.word		0xF70FB000

init2:

	/* copy ROM image into RAM - assumes hard reset */
#ifdef ROM
	/* Setup the MMU for 32 bit code and data. */
	mov		r0, #0x00000070
	mcr		p15, 0, r0, c1, c0

	/* disable all interrupts */
	/* (they should be disabled on reset, but just in case...) */
	mov		r12,#0x80000000	/* internal register base address*/
	mov		r0,#0x00000000
	str		r0,[r12,#0x0280]	/* INTMR1 = 0x8000.0280 */
	add		r11,r12,#0x1000
	str		r0,[r11,#0x280]		/* INTMR2 = 0x8000.1280 */
	add		r10,r12,#0x2000
	str		r0,[r10,#0x280]		/* INTMR3 = 0x8000.2280 */

	/*
	 * Enable SDRAM Bank 0 and Bank 1, each is 8 megabytes x32
	 * By default, Bank 0 is at address 0xC00000000
	 * and Bank 1 is at address 0xD0000000
	 */
	/* enable SDRAM */
	mov 		r0,#0x22
	add 		r0,r0,#0x500
	str		r0, [r10,#0x300]

	/* enable dram refresh at maximum rate (64kHz) */
	/* store result in DRFPR register 0x8000.2340 */
	mov		r0, #0x100
	str		r0, [r10, #0x340]
		
	/* Set SYSCON2 to zero (default value, step not necessary).  Bit 2 sets x32/16 DRAM */
		;
	mov		r1,#0x0			/* 32 bit wide SDRAM */
       	str     	r1,[r11,#0x100]         /* init syscon2 register */
     
	/* Set bits 1:2 in SYSCON3 for 74 MHz clock speed (default is 18MHz) */
       	mov     	r1, #0x06
       	str     	r1,[r10,#0x200]         /* init syscon3 register */
	mov		r0,r0			/* NOP */
	mov		r0,r0			/* NOP */

	/*
	 * Now configure the MemConfig register to get the following:
	 *
	 * nCS0 = NOR FLASH, 32-bit, 8 wait states @74Mhz (unreliable at 4)
	 * nCS1 = NAND FLASH, 32-bit, 2 wait states
	 * nCS2 = Ethernet, 16-bit, 8 wait states (was 32-bit =0x00)
	 * nCS3 = Parallel/Keyboard/GPIOs, 32-bit, 1 wait state
	 * nCS4 = USB, 8-bit, 1 wait state, 2 w/s random (was 32-bit, =0x3c)
	 * nCS5 = Unused/general purpose, 32-bit, 8 wait states
	 */
#define MemConfig1value 	0x1F101700
#define MemConfig2value 	0x00001f13	/* boot rom and internal SRAM are ignored  */
		
	/* configure nCS0-nCS3 */
	ldr		r1, memconfig1
	str		r1,[r12,#0x0180]		/*MEMCFG1 = 0x8000.0180*/

	/* configure nCS4 &nCS5 */
	ldr		r1, memconfig2
	str		r1,[r12,#0x01c0]		/*MEMCFG2 = 0x8000.01c*/
#endif

#ifdef MMU
	/* initialize the MMU L1 table */
    	ldr    	r0, pagetableva
	mov	r2, #0
	ldr	r1, .L35
.L5:
	orr	r3, r2, r1
	str	r3, [r0], #4
	add	r2, r2, #1048576
	cmn	r2, #1073741825
	bls	.L5
	mvn	r3, #1056964608
	mov	ip, r3
	cmp	r2, r3
	bhi	.L32
	ldr	r1, .L35+4
.L10:
	orr	r3, r2, r1
	str	r3, [r0], #4
	add	r2, r2, #1048576
	cmp	r2, ip
	bls	.L10
.L32:
	cmn	r2, #268435457
	bhi	.L33
	ldr	r1, .L35
.L15:
	orr	r3, r2, r1
	str	r3, [r0], #4
	add	r2, r2, #1048576
	cmn	r2, #268435457
	bls	.L15
.L33:
	mvn	r3, #251658240
	mov	ip, r3
	cmp	r2, r3
	bhi	.L34
	ldr	r1, .L35+4
.L20:
	bic	r3, r2, #805306368
	add	r3, r3, #1048576
	orr	r3, r3, r1
	str	r3, [r0], #4
	add	r2, r2, #1048576
	cmp	r2, ip
	bls	.L20
.L34:
	ldr	r3, .L35+8
	str	r3, [r0], #4
	add	r2, r2, #1048576
	mvn	r3, #150994944
	mov	ip, r3
	cmp	r2, r3
	bhi	.L23
	ldr	r1, .L35
.L25:
	orr	r3, r2, r1
	str	r3, [r0], #4
	add	r2, r2, #1048576
	cmp	r2, ip
	bls	.L25
.L23:
	ldr	r3, .L35+8
	str	r3, [r0], #4
	ldr	r3, .L35+12
	str	r3, [r0], #4
	add	r2, r2, #2097152
	cmn	r2, #2097152
	bhi	.L28
	ldr	r1, .L35
.L30:
	orr	r3, r2, r1
	str	r3, [r0], #4
	add	r2, r2, #1048576
	cmn	r2, #2097152
	bls	.L30
.L28:
	orr	r3, r2, #3088
	orr	r3, r3, #2
	str	r3, [r0, #0]

    	/* Set up the MMU.  Start by flushing the cache and TLB. */
    	mov     	r0, #0x00000000
    	mcr     	p15, 0, r0, c5, c0

    	/* Flush the cache */
    	mcr     	p15, 0, r0, c7, c7, 0

	/* Flush the TLB */
    	mcr     	p15, 0, r0, c8, c7, 0

    	/* Set user mode access for all 16 domains. */
    	ldr     	r0, accessmodes
    	mcr     	p15, 0, r0, c3, c0

    	/* Tell the MMU where to find the page table. */
    	ldr     	r0, pagetableptr
    	mcr     	p15, 0, r0, c2, c0

	/*
	 * Before we can enable the MMU, we must copy this program and the tables to
	 * physical DRAM starting at location 0xc0f0.0000.  
	 * After the MMU is enabled, the DRAM will be remapped to 0 and the Flash
	 * boot code will unmapped.
	 * We'll simply move first 832Kbytes since this code is <832K
	 * r13 and r14 should be used with care since it stores the stack pointer
	 * and link address
	 */
#endif 

#ifdef ROM
#if RUN_IN_ROM
	/* this doesn't work since the problem flash with botch program */
	/* execution, we could just copy a bit of code to RAM but, I'm */
	/* punting for now. */
	ldr		r0,etextptr		/* Ld r0 w/addr to start copy */
	mov		r1,#0x100000		/* mask to first 1Meg */
	sub		r1,r1,#1
	and		r0,r0,r1

    	mov     	r1,#0xC0000    		/* Ld r1 w/ addr to stop copy.*/
#else
    	mov     	r0,#0x00000    		/* Ld r1 w/ addr to start copy.*/
    	mov     	r1,#0xD0000    		/* Ld r1 w/ addr to stop copy.*/
#endif
    	mov     	r2,#0xC0000000 		/* Ld r2 w/ destination addr.*/
    	add     	r2,r2,#0x020000 	/* Ld r2 w/ destination addr.*/
copy_code:			                /* Copy code to SDRAM.*/
    	ldmia   	r0!,{r3-r12}    	/* Load words to copy.*/
    	stmia   	r2!,{r3-r12}    	/* Store words to new location.*/
    	subs		r3,r0,r1        	/* Check if done*/
    	blt     	copy_code       	/* If not done, keep copying.*/
#endif

#ifdef MMU
    	/* Enable the MMU. */
    	mov     r0, #0x0000007d
    	mcr     p15, 0, r0, c1, c0

	/*
    	 * At this point, program will be running from location 0x01000.0000 (16MB)
	 * since now the first 16MB is DRAM
    	 * There should always be two NOP instructions following the enable or
    	 * disable of the MMU.
	 */
    	mov     r0, r0
    	mov     r0, r0
#endif

#ifdef ROM
	ldr	r0, nextinsnptr
	mov	r15, r0
nextinsn:
	mov	r0, r0

	/*
	 * Now the system is ready for JTAG debugging.  As an alternative, one could 
	 * place the c_entry call at this point to jump to the user's embedded C routine.
	 *
	 * Wait forever.  We DO NOT want to place the processor in a sleep mode
	 * since the processor must be running for ICE to work!
	 * This part simply loops while blinking the LED.  Once ICE is connect, LED stops.
	 * Note:  Port D is output on reset
	 */
#if 0
	mov		r3,#0x0E000000		/* src addr - EPROM */
/*	mov		r3,#0x00500000		/ * src addr RAM image */
	mov		r4,#0x00000000		/* dst addr - RAM mapped by CMA10x */
	mov		r5,#0x00080000		/* max len (0.5Mb) */
1:
	ldr		r6,[r3],#4
	str		r6,[r4],#4
	subs	r5,r5,#4
	bne		1b
#endif
#endif

#ifndef ROM
	/* disable interupts now that the MMU is setup nicely */
	mov		r12,#0x80000000	/* internal register base address*/
	mov		r0,#0x00000000
	str		r0,[r12,#0x0280]	/* INTMR1 = 0x8000.0280 */
	add		r11,r12,#0x1000
	str		r0,[r11,#0x280]		/* INTMR2 = 0x8000.1280 */
	add		r10,r12,#0x2000
	str		r0,[r10,#0x280]		/* INTMR3 = 0x8000.2280 */
#endif

	.global	arm_init
	b		arm_init			/* eventually calls main() */
	/* END start */


	.global g_fault
g_fault:
	.word	0
	.global g_clientstate
g_clientstate:
	.word	0

r0_save:
	.word	0

	.global	undefinsn_exception
undefinsn_exception:
	SAVECLIENT
	mov	r1,#1
	bl	fault_handler
	RESTORECLIENT
	subs	pc,lr,#4			/* return */

	.global	swtrap_exception
swtrap_exception:
	mov	pc,lr				/* return */

	SAVECLIENT
	mov	r1,#2
	bl	fault_handler
	RESTORECLIENT
	subs	pc,lr,#4			/* return */

	.global	abortpre_exception
abortpre_exception:
	SAVECLIENT
	mov	r1,#3
	bl	fault_handler
	RESTORECLIENT
	subs	pc,lr,#4			/* return */

	.global	abortdata_exception
abortdata_exception:
	SAVECLIENT
	mov	r1,#4
	bl	fault_handler
	RESTORECLIENT
	subs	pc,lr,#4			/* return */

	.global	reserved_exception
reserved_exception:
	SAVECLIENT
	mov	r1,#5
	bl	fault_handler
	RESTORECLIENT
	subs	pc,lr,#4			/* return */


	.global get_psr
get_psr:
	mrs     r0, cpsr_all;           /* Put the SPSR on the stack */    \
	mov		pc, lr

	.global g_interruptstate
g_interruptstate:
	.word	0
int_r0_save:
	.word	0

	.global interrupt
interrupt:
	str		r0,int_r0_save
	ldr		r0,g_interruptstate
	sub		lr,lr,#4		/* adjust link register */
	str		lr,[r0,#CLIENT_PC]	/* store exception PC */
	str		sp,[r0,#CLIENT_SSP]
	add		r0,r0,#CLIENT_R1
	stmia		r0,{r1-r12}		/* store registers */
	sub		r0,r0,#CLIENT_R1
	/* move r0 over */
	ldr		r1,int_r0_save
	str		r1,[r0,#CLIENT_R0]
	/* store prior mode r13,r14 registers */
	mrs		r1,spsr_all
	str		r1,[r0,#CLIENT_SPSR]
	mrs		r2,cpsr_all
	str		r2,[r0,#CLIENT_CPSR]
#if 0
	bic		r3,r2,#PSR_MODE_MASK
	and		r4,r1,#PSR_MODE_MASK
	orr		r3,r3,r4
	cmp		r1,#PSR_MODE_USR
	beq		irqusermode

	msr		cpsr_all,r3		/* switch to prior mode */
	mov		r0,r0
	str		sp,[r0,#CLIENT_SP]
	str		lr,[r0,#CLIENT_LR]
	msr		cpsr_all,r2		/* switch back */
	mov		r0,r0
	b		irqcont

irqusermode:
#endif
	add		r0,r0,#CLIENT_SP
        stmia   	r0,{r13-r14}^
        mov     	r0,r0
	sub		r0,r0,#CLIENT_SP

irqcont:
	bl		intr_handler

	ldr		r1,[r0,#CLIENT_SPSR]
	msr		spsr_all,r1
	ldr		lr,[r0,#CLIENT_PC]
#if 0
	and		r1,r1,#PSR_MODE_MASK
	cmp		r1,#PSR_MODE_USR
	beq		irqusermode2

	mrs		r2,cpsr_all
	bic		r3,r2,#PSR_MODE_MASK
	orr		r1,r1,r3
	msr		cpsr_all,r1		/* switch to prior mode */
	mov		r0,r0
	mov		r1,r2

	ldr		sp,[r0,#CLIENT_SP]
	ldr		lr,[r0,#CLIENT_LR]
	add		r0,r0,#CLIENT_R2
	ldmia		r0,{r2-r12}
	mov		r0,r0
	msr		cpsr_all,r1		/* switch back */
	mov		r0,r0
	ldr		r1,[r0,#(CLIENT_R1-CLIENT_R2)]
	ldr		r0,[r0,#(CLIENT_R0-CLIENT_R2)]
	movs		pc,lr			/* and return */

irqusermode2:
#endif
	add		r0,r0,#CLIENT_R1
	ldmia		r0,{r1-r14}^
	mov		r0,r0
	ldr		r0,[r0,#(CLIENT_R0-CLIENT_R1)]
	movs		pc,lr			/* and return */

	
ci_r0_save:
	.word	0

	/* arm_client_interface changes stacks before calling client_interface */
	.extern client_interface
	.global	arm_client_interface
arm_client_interface:
	str	r0,ci_r0_save
	ldr	r0,tos
	sub	r0,r0,#10240
	sub	r0,r0,#8
	str	sp,[r0]
	str	lr,[r0,#4]
	mov	sp,r0
	ldr	r0,ci_r0_save
	bl	client_interface
	ldr	lr,[sp,#4]
	ldr	sp,[sp]
	mov	pc,lr


    	/* Flush the cache */
	.global	flush_caches
flush_caches:
    	mov     	r0, #0x00000000
    	mcr     	p15, 0, r0, c7, c7, 0
	mov		pc, lr

    	/* Flush the TLB */
	.global	flush_tlb
flush_tlb:
    	mov     	r0, #0x00000000
    	mcr     	p15, 0, r0, c8, c7, 0
	mov		pc, lr



#ifdef ROM
etextptr:
	.word		etext
nextinsnptr:
	.word		nextinsn
memconfig1:
	.word		MemConfig1value
memconfig2:
	.word		MemConfig2value
#endif
#ifdef MMU
pagetableva:
#ifdef ROM
	.word		PAGETABLE_L1_BASE
#else
	.word		0xF70FC000
#endif
pagetableptr:
	.word		PAGETABLE_L1_BASE
.L35:
	.word	3090
	.word	3102
	.word	-1073738722
	.word	3098
#endif
accessmodes:
	.word		0x55555555
