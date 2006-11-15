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

/* (c) Copyright 2003 by CodeGen, Inc.  All Rights Reserved. */

#include <stddef.h>
#include <stdarg.h>

#define DEBUG
#include "defs.h"
#include "pci.h"

#define tostr2(num)	#num
#define tostr(num)	tostr2(num)


/* 64-bit RAM startup code when we are loaded by someone else.
 */
void
start(void)
{
    uLong *l4, *l3, *l2;
    uLong a;
    int i;

    asm("mov	$" tostr(STACKADDR) "-512,%esp");

/* build 24KB worth of page tables, that map VA 0000.0000.000 to 0000.FFFF.FFFF
 * with a one to one mapping to PAs.  All other VA's fault.
 */ 
    #define PAGE_TABLE		STACKADDR

    #define PT_P		0x0001
    #define PT_RW		0x0002
    #define PT_US		0x0004
    #define PT_PWT		0x0008
    #define PT_PCD		0x0010
    #define PT_A		0x0020
    #define PT_D		0x0040
    #define PT_PS		0x0080
    #define PT_G		0x0100
    #define PT_PAT		0x1000

    l4 = (uLong *)(PAGE_TABLE & ~0xFFF); /* align to 4K */
    l3 = &l4[512];
    l2 = &l3[512];
    a = 0;

    /* allow caching DRAM area 0..0x2000.0000 / 0x200000 == 256 */
    for (i = 0; i < 256; i++, a += 2*1024*1024)
	l2[i] = a | (PT_PS|PT_US|PT_RW|PT_P);

    for (; i < 4*512; i++, a += 2*1024*1024)
	l2[i] = a | (PT_PS|PT_PCD|PT_US|PT_RW|PT_P);

    l3[0] = (uLong)&l2[0*512] | (PT_PCD|PT_US|PT_RW|PT_P);
    l3[1] = (uLong)&l2[1*512] | (PT_PCD|PT_US|PT_RW|PT_P);
    l3[2] = (uLong)&l2[2*512] | (PT_PCD|PT_US|PT_RW|PT_P);
    l3[3] = (uLong)&l2[3*512] | (PT_PCD|PT_US|PT_RW|PT_P);

    l4[0] = (uLong)&l3[0*512] | (PT_PCD|PT_US|PT_RW|PT_P);
    l4[1] = (PT_PCD|PT_US|PT_RW);
    l4[2] = (PT_PCD|PT_US|PT_RW);
    l4[3] = (PT_PCD|PT_US|PT_RW);

    for (i = 4; i < 512; i++)
    {
	l3[i] = (PT_PCD|PT_US|PT_RW);
	l4[i] = (PT_PCD|PT_US|PT_RW);
    }

    /* aliases for kernel VM addrs ffff.ffff.8000.0000 to 0x0 physical */
    l3[510] = (uLong)&l2[0*512] | (PT_PCD|PT_US|PT_RW|PT_P);
    l3[511] = (uLong)&l2[1*512] | (PT_PCD|PT_US|PT_RW|PT_P);
    l4[511] = (uLong)&l3[0*512] | (PT_PCD|PT_US|PT_RW|PT_P);

    // Set new page base
    asm("mov	$" tostr(STACKADDR) ", %rax");
    asm("mov	%rax, %cr3");

    // SYSCFG - lock bits, turn off fixed MTRRs to access lower 1Mb RAM
    asm("mov	$0xC0010010, %ecx");
    asm("mov	$0x120601, %eax");
    asm("mov	$0, %edx");
    asm("wrmsr");

    /* misc reg inits - XXX - for testing */

    /* memory-mapped I/O */
    pci_config_write(0, 0, 24, 1, 0xB8, 0x200003);
    pci_config_write(0, 0, 24, 1, 0xBC, 0xFE0B00);

    /* clock power/timing low&high */
    pci_config_write(0, 0, 24, 3, 0xD8, 0x02710002);
    pci_config_write(0, 0, 24, 3, 0xDC, 0x20002710);

    /* PCI intr/ctrl rev - ISAEN SERREN */
    pci_config_write(0, 0, 4, 0, 0x3C, 0x040600FF);

    /* turn on built-in USB on 8111 */
    pci_config_write(0, 0, 5, 0, 0x48, 0x0100FD2F);

#if 0
    // SYSCFG - unlock some bits
    asm("mov	$0xC0010010, %ecx");
    asm("mov	$0x1E0601, %eax");
    asm("mov	$0, %edx");
    asm("wrmsr");

    // MTRRfix4K_C0000 
    asm("mov	$0x268,%ecx");
    asm("mov	$0x06060606, %edx");
    asm("mov	$0x06060606, %eax");
    asm("wrmsr");

    // MTRRfix16K_A0000 
    asm("mov	$0x259,%ecx");
    asm("wrmsr");

    // MTRRfix4K_C8000 
    asm("mov	$0x269,%ecx");
    asm("wrmsr");
    
    //MTRRfix4K_D0000 
    asm("mov	$0x26A,%ecx");
    asm("wrmsr");
    
    //MTRRfix4K_D8000 
    asm("mov	$0x268,%ecx");
    asm("wrmsr");
    
    //MTRRfix4K_F0000 
    asm("mov	$0x26E,%ecx");
    asm("wrmsr");
    
    //MTRRfix4K_F8000 
    asm("mov	$0x26F,%ecx");
    asm("wrmsr");

    pci_config_write(0, 0, 24, 1, 0xB0, 0x0);
    pci_config_write(0, 0, 24, 1, 0xB4, 0x0);
    pci_config_write(0, 0, 24, 1, 0xC0, 0x01003);
    pci_config_write(0, 0, 24, 1, 0xC4, 0xFF000);
    pci_config_write(0, 0, 24, 1, 0xC8, 0x0);
    pci_config_write(0, 0, 24, 1, 0xCC, 0x0);
    pci_config_write(0, 0, 24, 1, 0xD0, 0x0);
    pci_config_write(0, 0, 24, 1, 0xD4, 0x0);
    pci_config_write(0, 0, 24, 1, 0xD8, 0x0);
    pci_config_write(0, 0, 24, 1, 0xDC, 0x0);
#endif

#if 0
    // MTRRdefType - turn off MTRRs
    //asm("mov	$0x2FF,%ecx");
    //asm("mov	$800, %eax");
    //asm("wrmsr");

    // SYSCFG - lock bits, normal run mode
    asm("mov	$0xC0010010, %ecx");
    asm("mov	$0x160601, %eax");
    asm("mov	$0, %edx");
    asm("wrmsr");
#endif

    asm("jmp	main");
}
