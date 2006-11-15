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

/* (c) Copyright 2002-2003 by CodeGen, Inc.  All Rights Reserved. */
/* entry point for 64-bit long-mode x86-64 C code
   - assumes RAM is running, and entry is in 32-bit mode
   - some 32->64-bit code switchover borrowed from sample AMD code
 */

//#define DEBUG

//	These are the selectors set up in the gdt

#define CODE_SEL_32	0x08
#define DATA_SEL_32	0x10
#define CODE_SEL_LONG	0x18

//	Define the MSR that controls long mode

#define EFER_MSR		0x0c0000080

//	Define the fixed physical address where we will put the page tables

//#define PAGE_TABLE_BASE	0x00100000
#define PAGE_TABLE_BASE		STACKADDR

//	Define the bits in the controlling MSR

#define SYSCALL_ON	0x000000001		// Enable SYSCALL instruction
#define LONG_MODE_ON	0x000000100		// Enable long mode
#define REX32_MODE_ON	0x000000200		// Enable Rex32

//	Define the bits in CR0

#define CR0_PG			0x80000000

//code	segment	use32 byte public 'code'
//	assume cs:code, ds:code, es:code, ss:code

//	Enter here in 32 bit normal mode, CS L bit not set

	.text

/* entry point for both RAM and ROM images */
	.globl	start64
start64:
	.code32

#ifdef DEBUG
	mov	$0x3F8 + 0x0, %dx
	mov	$'1', %al
	out	%al, %dx
	out	%al,$0xEB
	mov	$0x3F8 + 0x5, %dx
1:
	in	%dx,%al
	and	$0x40,%al
	jz	1b
#endif

	// turn on PAE mode
	mov	%cr4,%eax
	bts	$5,%eax
	mov	%eax,%cr4

#ifdef DEBUG
	mov	$0x3F8 + 0x0, %dx
	mov	$'3', %al
	out	%al, %dx
	out	%al,$0xEB
	mov	$0x3F8 + 0x5, %dx
1:
	in	%dx,%al
	and	$0x40,%al
	jz	1b
#endif

//	Now, turn on LME

	// [already turned on in start1.as86]
	xor	%edx, %edx
	movl	$LONG_MODE_ON, %eax
	movl	$EFER_MSR, %ecx
	wrmsr				// Write to it

#ifdef DEBUG
	mov	$0x3F8 + 0x0, %dx
	mov	$'4', %al
	out	%al, %dx
	out	%al,$0xEB
	mov	$0x3F8 + 0x5, %dx
1:
	in	%dx,%al
	and	$0x40,%al
	jz	1b
#endif

//	Set up paging

	mov	$PAGE_TABLE_BASE, %eax
	mov	%eax, %cr3		// Set page base

#ifdef DEBUG
	mov	$0x3F8 + 0x0, %dx
	mov	$'5', %al
	out	%al, %dx
	out	%al,$0xEB
	mov	$0x3F8 + 0x5, %dx
1:
	in	%dx,%al
	and	$0x40,%al
	jz	1b
#endif

// wait for HDT
/*****
	clc
1:
	jnc	1b
*****/

//	Finally, turn paging on. This also turns on LMA

	mov	%cr0, %eax
	or	$CR0_PG, %eax		// Turn paging on,
	and	$~0x60000000, %eax	// make sure cache is write-back,
	mov	%eax, %cr0		// and turn on LMA.
//	jmp	2f
//2:

	mov	$GDT64ptr,%eax
	lgdt	(%eax)

	.byte	0xea			// JMPF
	.long	LMAOn
	.word	CODE_SEL_LONG

//	Entry here is with long mode enabled and active, paging on, 2Mb pages

LMAOn:
	.code64

	nop
	nop
	nop
	nop

#ifdef DEBUG
	mov	$0x3F8 + 0x0, %dx
	mov	$'6', %al
	out	%al, %dx
	out	%al,$0xEB
	mov	$0x3F8 + 0x5, %dx
1:
	in	%dx,%al
	and	$0x40,%al
	jz	1b
#endif

	mov	$GDT64ptr,%rax
	lgdt	(%rax)

#ifdef DEBUG
	mov	$0x3F8 + 0x0, %dx
	mov	$'7', %al
	out	%al, %dx
	out	%al,$0xEB
	mov	$0x3F8 + 0x5, %dx
1:
	in	%dx,%al
	and	$0x40,%al
	jz	1b
#endif

//****	Insert any test code below this point

	call	main			/* and jump to 64-bit startup code */

//****	Insert any test code above this point


	xor	%eax, %eax		// BOP(0) is a breakpoint
	.byte	0xf1			// ICEBP
	hlt


	.align 8
GDT64:
					// segment 0x00 : null
	.long	0, 0
					// segment 0x08 : flat code32
	.word	0xFFFF			// lolimit
	.word	0				// lobase
	.byte	0				// midbase
	.byte	0x1F | 0 | 0x80		// RWXAC, dpl = 0, present
	.byte	0x0f | 0 | 0x40 | 0x80	// hilimit, xx, 32bit
	.byte	0				// hibase

					// segment 0x10 : flat data
	.word	0xFFFF			// lolimit
	.word	0				// lobase
	.byte	0				// midbase
	.byte	0x13 | 0 | 0x80		// RWA, dpl = 0, present
	.byte	0x2f | 0 | 0x40 | 0x80	// hilimit, xx, 64bit
	.byte	0				// hibase

					// segment 0x18 : flat code64
	.word	0xFFFF			// lolimit
	.word	0				// lobase
	.byte	0				// midbase
	.byte	0x80 | 0x18 | 0x03	// RWXAC, dpl = 0, present
	.byte	0x00 | 0x20 | 0x0F		// hilimit, xx, 64bit
	.byte	0				// hibase


GDT64ptr:
	.word	4 * 8			// Limit of GDT
	.quad	GDT64			// Address of GDT



	.align 8
GDT32:
					// segment 0x00 : null
	.long	0, 0
					// segment 0x08 : flat code
	.word	0xFFFF			// lolimit
	.word	0				// lobase
	.byte	0				// midbase
	.byte	31 | 0 | 0x80		// RWXAC, dpl = 0, present
	.byte	0xf | 0 | 0x40 | 0x80	// hilimit, xx, 32bit, 4k granularity
	.byte	0				// hibase

					// segment 0x10 : flat data
	.word	0xFFFF			// lolimit
	.word	0				// lobase
	.byte	0				// midbase
	.byte	19 | 0 | 0x80		// RWA, dpl = 0, present
	.byte	0xf | 0 | 0x40 | 0x80	// hilimit, xx, 32bit, 4k granularity
	.byte	0				// hibase

					// segment 0x18 : 64-bit code
	.word	0xFFFF			// lolimit
	.word	0				// lobase
	.byte	0				// midbase
	.byte	0x80 | 0x18 | 0x03	// RWXAC, dpl = 0, present
	.byte	0x80 | 0x20 | 0x0F		// hilimit, xx, 64bit
	.byte	0				// hibase

GDT32ptr:
	.word	4 * 8			// Limit of GDT
	.long	GDT32			// Address of GDT
	.long	0



// launch a protected-mode 32-bit app
// usage: void launch32(void (*func)(), ...);
// "func" is dropped from the stack before launching.
//
	.globl launch32
launch32:
	.code64

#ifdef DEBUG
	mov	$0x3F8 + 0x0, %dx
	mov	$'1', %al
	out	%al, %dx
	out	%al,$0xEB
	mov	$0x3F8 + 0x5, %dx
1:
	in	%dx,%al
	and	$0x40,%al
	jz	1b
#endif

    // 1. Switch to compatibility mode and place the processor at the
    //    highest privilege level (CPL=0).

	// load gdt since we do not want paging
	mov	$GDT32ptr, %rax
	lgdt	(%rax)

	// jump to segment 8 - have to use indirect far jump
	ljmp	*lj1
lj1:
	.long	lj2	// addr
	.word	8	// segment
lj2:


#ifdef DEBUG
	mov	$0x3F8 + 0x0, %dx
	mov	$'a', %al
	out	%al, %dx
	out	%al,$0xEB
	mov	$0x3F8 + 0x5, %dx
1:
	in	%dx,%al
	and	$0x40,%al
	jz	1b
#endif

    // 2. Deactivate long mode by clearing CR0.PG to 0. This causes
    //    the processor to clear the LMA bit to 0. The MOV CR0
    //    instruction used to disable paging must be located in an
    //    identity-mapped page. Once paging is disabled, the
    //    processor behaves as a standard 32-bit x86 processor.

	mov	%cr0, %rax
	and	$~CR0_PG, %eax
	mov	%rax, %cr0
	.code32

	nop
	nop
	nop
	nop

#ifdef DEBUG
	mov	$0x3F8 + 0x0, %dx
	mov	$'2', %al
	out	%al, %dx
	out	%al,$0xEB
	mov	$0x3F8 + 0x5, %dx
1:
	in	%dx,%al
	and	$0x40,%al
	jz	1b
#endif

    // 3. Load CR3 with the physical base-address of the legacy page
    //    tables.
    // [do not want paging]

    // 4. Disable long mode by clearing EFER.LME to 0.

	xor	%edx, %edx
	xor	%eax, %eax
	movl	$EFER_MSR, %ecx
	wrmsr

#ifdef DEBUG
	mov	$0x3F8 + 0x0, %dx
	mov	$'3', %al
	out	%al, %dx
	out	%al,$0xEB
	mov	$0x3F8 + 0x5, %dx
1:
	in	%dx,%al
	and	$0x40,%al
	jz	1b
#endif

    // 5. Enable legacy page-translation by setting CR0.PG to 1. The
    //    instruction following the MOV CR0 that enables paging
    //    must be a branch, and both the MOV CR0 and the following
    //    branch instruction must be located in an identity-mapped page.
    // [do not want paging]

#ifdef DEBUG
	mov	$0x3F8 + 0x0, %dx
	mov	$'4', %al
	out	%al, %dx
	out	%al,$0xEB
	mov	$0x3F8 + 0x5, %dx
1:
	in	%dx,%al
	and	$0x40,%al
	jz	1b
#endif

	// get the launch point off of the stack and go
	movl 0(%esp), %eax
	jmp  *%eax


/* Any additional x86-64 routines to be called from C go here.
 */
	.code64

// ...
