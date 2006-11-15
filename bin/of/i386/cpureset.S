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

; (c) Copyright 1996,1998,2000 by CodeGen, Inc.  All Rights Reserved.
; Some code cribbed from OpenBSD i386 biosboot.S to setup flat 32-bit mode.
;
; to assemble:
; 	as86 -0 -l c.l -b c.o c.S
;	dd if=c.o of=c.oo bs=0x5 skip=1
; then pad+append to end of C image, and adjust entry point below
;


; where start of image should be in ROM
STARTADDR	set	0xFFFF7000


; start of our 16-bit bootstrap code that launchs to 32-bit world
;
	.org 0xFFFFFF00
	use16


; global segment table
;
gtab:
	.align	4
							; segment 0x00 : null
	.long	0, 0
							; segment 0x08 : flat code
	.word	0xFFFF					; lolimit
	.word	0						; lobase
	.byte	0						; midbase
	.byte	31 | 0 | 0x80			; RWXAC, dpl = 0, present
	.byte	0xf | 0 | 0x40 | 0x80	; hilimit, xx, 32bit, 4k granularity
	.byte	0						; hibase

							; segment 0x10 : flat data
	.word	0xFFFF					; lolimit
	.word	0						; lobase
	.byte	0						; midbase
	.byte	19 | 0 | 0x80			; RWA, dpl = 0, present
	.byte	0xf | 0 | 0x40 | 0x80	; hilimit, xx, 32bit, 4k granularity
	.byte	0						; hibase

							; segment 0x18 : null
	.long	0, 0

Gdtr:
	.word	4 * 8 - 1
	.long	gtab


; hard reset - cpureset vector (below) jumps here to start things off
;
reset:
	use16

	; needed for some processors?
	nop
	nop
	nop

	; disable all interrupts
	cli
	cld

	; invalidate the TLB
	xor  eax, eax
	mov  cr3, eax

	; invalidate cache and make sure it is turned off
	invd
	mov		eax, cr0 
	or		eax, #0x60000000
	mov		cr0, eax


	; init segment regs to point to CS for now (no RAM yet)
	; (leave ss pointing to low-RAM)
	mov		bx, cs
	mov		ds, bx
	xor		bx, bx
	mov		es, bx
	mov		fs, ax
	mov		gs, dx


	; load the gdtr
	.byte 0x66		; data32
	lgdt	[Gdtr]


	; set the PE bit of CR0
	mov		eax, cr0 
	or		eax, #0x01	; CR0 PE
	mov		cr0, eax

	use32

	; make intrasegment jump to flush the processor pipeline and
	; reload CS register

	;.byte 0x67		; addr32
	.byte 0x66		; data32
	jmp far 0x8:mode32


; running in 32-bit flat memory-space - init stuff before jumping to C
;
mode32:
	use32

	; 32bit mode
	; set up ds, ss, es, etc
	mov		eax, #0x10
	mov		ds, ax
	mov		ss, ax
	mov		es, ax
	mov		fs, ax
	mov		gs, ax

	; set up our stack at the top of DOS mem (640k)
	mov		esp, #0x0A0000

	jmp		0x8:#STARTADDR	; entry point for 32-bit start.S code (_start)




; reset vector must end up at 0xFFFFFFF0 (0xF000:FFF0) phys addr
;
	.org 0xFFFFFFF0
cpureset:
	use16

	jmp 0xF000:(reset & 0xFFFF)
	nop
	nop
	nop

	nop
	nop
	nop
