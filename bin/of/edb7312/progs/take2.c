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

/* (c) Copyright 2001 by CodeGen, Inc.  All Rights Reserved. */

/* Simple program to switch MMU tables */

#include "sfclient.h"

/* dummies to avoid normal gcc/g++ runtime goo */
void __gccmain() { }
void __main() { }

extern void memcpy(void *dst, void *src, unsigned int len);

static uByte g_mem[64*1024];

unsigned int
get_current_ttb()
{
	unsigned int x;
	/* get L1 table pointer */
	asm("mrc     	p15, 0, r0, c2, c0 ; "
	    "mov        %0,r0" : "=r"(x) : : "r0");
	return x & ~0x3FFF;
}

void
showtable(uInt *table, uInt vaoff)
{
	int i, j;

	for (i = 0; i < 4096; i++)
		if (table[i])
		{
			sf_printf("table[%X] = %#x\n", i, table[i]);

			if ((table[i] & 3) == 1)
			{
				uInt *p2 = (uInt *)((table[i] & 0xFFFFFC00) + vaoff);
				sf_printf("P2 VA %p\n", p2);

				for (j = 0; j < 256; j++)
					if ((p2[j] & 3) != 0)
						sf_printf("table[%X].p2[%X] = %#x\n", i, j, p2[j]);
			}
		}
}

void
copyl2(uInt *table, uInt newvaoff, uInt oldvaoff, uByte *p2buf)
{
	int i;

	for (i = 0; i < 4096; i++)
		if ((table[i] & 0x3) == 1)
		{
			uInt *op2 = (uInt *)((table[i] & 0xFFFFFC00) + oldvaoff);
			uInt *np2 = (uInt *)p2buf;
			p2buf += 0x400;

			table[i] = (table[i] & 0x3FF) | ((uInt)np2 - newvaoff);
			memcpy(np2, op2, 0x400);
		}
}

#define CURRENT_TABLE_BASE 0xF70FC000
#define NEW_VA_OFF (0xF0000000 - 0xC0100000)
#define OLD_VA_OFF (0xF7000000 - 0xC0000000)

int
takeovermmu()
{
	uPtr addr;
	uPtr oldaddr;
	uPtr newaddr;
	uByte *newtable;

	addr = (uPtr)g_mem;
	addr = (addr + 0x4000 - 1) & ~(0x4000 - 1);
	newtable = (uByte *)addr;

	/* copy the current table into our new table, this is EDB7312 specific */
	memcpy(newtable, (void *)CURRENT_TABLE_BASE, 0x4000);
	copyl2((uInt *)newtable, NEW_VA_OFF, OLD_VA_OFF, newtable + 0x4000);

	sf_printf("Current table:\n");
	showtable((uInt *)CURRENT_TABLE_BASE, OLD_VA_OFF);
	sf_printf("New table:\n");
	showtable((uInt *)newtable, NEW_VA_OFF);

	/* Convert to PA */
	addr -= (0xF0000000 - 0xC0100000);

	sf_printf("New L1 page table built\n");
	sf_printf("New Table VA %p PA %#x\n", newtable, addr);
	oldaddr = get_current_ttb();
	sf_printf("Current Table VA %#x PA %#x\n", CURRENT_TABLE_BASE, oldaddr);

	sf_printf("About to switch page tables\n");

	newaddr = addr;
	sf_printf("Switching to %s page table\n", newaddr == oldaddr ? "current" : "new");

	/* flush caches */
    	asm("mov	r0,%0 ; "
	    "mov     	r1, #0x00000000 ; "
    	    "mcr     	p15, 0, r1, c7, c7, 0 ; "	/* flush caches */
	/* reload L1 table pointer */
    	    "mcr     	p15, 0, r0, c2, c0 ; "	/* load L1 table pointer */

	/* flush tlb */
    	    "mcr     	p15, 0, r1, c8, c7, 0 ; "	/* flush TLB */
	    : : "r"(newaddr) : "r0","r1");
	asm("mov	r0,r0");			/* NOP */
	asm("mov	r0,r0");			/* NOP */

	sf_printf("Running with new tables\n");
	return 1;
}

int
main(int argc, const char *argv[], const char *envv[])
{
	/* addr of C client-interface is passed in argc after terminating NULL
	   - it is also passed as a hex string in envv as "client_interface"
	   - the addr of the asm interface is in a machine-dependent register
	 */
	sf_puts("In takemmu\r\n");
	sf_puts("Taking over MMU\r\n");

	takeovermmu();

	sf_puts("Done!\r\n");
	return R_OK;
}
