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

/* Simple program to take over MMU management */

/*#define DEBUG*/

#include "sfclient.h"

extern void strcpy(char *, char *);
extern void strcat(char *, char *);
extern int strcmp(char *, char *);

/* dummies to avoid normal gcc/g++ runtime goo */
void __gccmain() { }
void __main() { }

#define SWAP4(x) ((((x) >> 24) & 0xFF) | (((x) >> 8) & 0xFF00) | \
		(((x) & 0xFF00) << 8) | (((x) & 0xFF) << 24))

/* debug printf - could use varargs and vsprintf if so desired
 */
void
dprintf(const char *str)
{
#ifdef DEBUG
	sf_puts(str);
#endif
}

#ifdef DEBUG
#define DPRINTF(x) sf_printf x
#else
#define DPRINTF(x)
#endif

/* convert virtual address (va) to a physical address (pa) */
int
getpa(Instance *mmu, uInt va, uInt *pa)
{
	Cell args[1];
	Cell ret[4];
	Retcode r;
	DPRINTF(("getpa: va %#x, pa %#x\n", va, pa));

	args[0] = (Cell)va;
	r = sf_call_method("translate", mmu, 1, 4, args, ret);
	DPRINTF(("getpa: ret %#x %#x %#x %#x\n", ret[0], ret[1], ret[2], ret[3]));

	if (r != R_OK || ret[0] != 0)
		return 0;

	*pa = ret[3];
	DPRINTF(("getpa: *pa %#x\n", *pa));
	return 1;
}

struct translation
{
	uInt virt;
	uInt size;
	uInt phys;
	uInt mode;
};

int
counttables(uByte *prop, Int len)
{
	struct translation *trans = (struct translation *)prop;
	int num;
	int i;
	uInt dirmap[4096/32];
	DPRINTF(("counttables: prop %#x, len %d\n", prop, len));

	num = len / sizeof (struct translation);

	for (i = 0; i < 4096/32; i++)
		dirmap[i] = 0;

	for (i = 0; i < num; i++)
	{
		uInt virt = SWAP4(trans[i].virt);
		uInt size = SWAP4(trans[i].size);
		uInt phys = SWAP4(trans[i].phys);
		uInt off;
		int dir;

		DPRINTF(("counttables: t %#x\n", &trans[i]));
		DPRINTF(("counttables: virt %#x, phys %#x, size %#x\n", virt, phys, size));

		if (virt & 0xFFFFF)
		{
			/* virtual address is mid section so we need a page table */
			dir = (virt >> 20) & 0xFFF;

			dirmap[dir / 32] |= 1 << (dir % 32);

			off = ((dir + 1) << 20) - virt;
			virt += off;

			if (size <= off)
				size = 0;
			else
			{
				size -= off;
				phys += off;
			}
		}

		while ((size & 0xFFF00000) && (phys & 0xFFFFF))
		{
			/* section map will not work so page table is required */
			dir = (virt >> 20) & 0xFFF;
			dirmap[dir / 32] |= 1 << (dir % 32);
			virt += 0x100000;
			phys += 0x100000;
			size -= 0x100000;
		}

		if (size & 0xFFFFF)
		{
			/* translation ends mid setion so page table is required */
			dir = ((virt + size) >> 20) & 0xFFF;
			dirmap[dir / 32] |= 1 << (dir % 32);
		}
	}

	num = 0;

	for (i = 0; i < 4096; i++)
		if (dirmap[i / 32] & (1 << (i % 32)))
			num++;

	return num;
}

#define FAULT		0
#define PAGE		1
#define SECT		2
#define RESERVED	3
#define LARGE_PAGE	1
#define SMALL_PAGE	2

int
addsectmapping(uInt *table, uInt virt, uInt phys, uInt mode)
{
	int dir = (virt >> 20) & 0xFFF;
	DPRINTF(("addsectmapping: table %#x, virt %#x, phys %#x, mode %#x, dir %#x\n", table, virt, phys, mode));

	if ((table[dir] & 3) == PAGE)
		return 0;

	table[dir] = phys | (mode & 0xDEC) | SECT;
	return 1;
}

int
addpagemapping(uInt *table, uInt **free, uInt virt, uInt phys, uInt mode)
{
	int dir = (virt >> 20) & 0xFFF;
	uInt *l2;
	int i;
	DPRINTF(("addpagemapping: table %#x, free %#x, virt %#x, phys %#x, mode %#x, dir %#x\n", table, free, virt, phys, mode));

	if ((table[dir] & 3) == SECT)
		return 0;

	if ((table[dir] & 3) != PAGE)
	{
		/* allocate page table */
		table[dir] = (uInt)*free | (mode & 0x1E0) | PAGE;
		l2 = *free;
		*free = (uInt *)((uByte *)l2 + 1024);

		for (i = 0; i < 256; i++)
			l2[i] = 0;
	}

	l2 = (uInt *)(table[dir] & ~3);
	mode &= 0xC0C;
	mode |= (mode >> 2) & 0x300;
	mode |= (mode >> 4) & 0x0F0;
	l2[(virt >> 12) & 0xFF] = phys | mode | SMALL_PAGE;
	return 1;
}

int
addmapping(uInt *table, uInt **free, struct translation *t)
{
	uInt virt = SWAP4(t->virt);
	uInt size = SWAP4(t->size);
	uInt phys = SWAP4(t->phys);
	uInt mode = SWAP4(t->mode);
	DPRINTF(("addmapping: table %#x, free %#x, t %#x\n", table, free, t));
	DPRINTF(("addmapping: virt %#x, phys %#x, size %#x, mode %#x\n", virt, phys, size, mode));

	if ((virt & 0xFFF) || (phys & 0xFFF) || (size & 0xFFF))
	{
	    DPRINTF(("addmapping: unaligned input\n"));
	    return 0;
	}

	while (size >= 0x1000)
	{
		if (size >= 0x100000 && (virt & 0xFFFFF) == 0 &&
				(phys & 0xFFFFF) == 0)
		{
			if (!addsectmapping(table, virt, phys, mode))
				return 0;

			virt += 0x100000;
			phys += 0x100000;
			size -= 0x100000;
		}
		else
		{
			if (!addpagemapping(table, free, virt, phys, mode))
				return 0;

			virt += 0x1000;
			phys += 0x1000;
			size -= 0x1000;
		}
	}

	return 1;
}

int
buildtables(Instance *mmu, uByte *l1table, int l1tablelen, uByte *prop, Int len)
{
	uInt *table = (uInt *)l1table;
	uInt *free = (uInt *)(l1table + 16*1024);
	struct translation *trans = (struct translation *)prop;
	int num;
	int i;
	DPRINTF(("addmapping: mmu %#x, l1table %#x, l1tablelen %d, prop %#x, len %d\n", mmu, l1table, l1tablelen, prop, len));

	num = len / sizeof (struct translation);

	/* zero out the L1 table */
	for (i = 0; i < 4096; i++)
		table[i] = 0;

	/* add each translation to the L1 table, incorporating any L2 tables */
	/* as needed. */
	for (i = 0; i < num; i++)
		if (!addmapping(table, &free, &trans[i]))
			return 0;

	for (i = 0; i < 4096; i++)
	{
		if ((table[i] & 3) == PAGE)
		{
			/* convert VA to PA in PDE */
			uInt va = table[i] & 0xFFFFFC00;
			uInt pa;

			if (!getpa(mmu, va, &pa))
				return 0;

			table[i] &= 0x3FF;
			table[i] |= (pa & ~0x3FF);
		}
	}

	return 1;
}

int
gettranslations(Package *mmu, uByte **prop, Int *len)
{
	uByte *p = *prop;
	Int o = *len;
	Int l;

	DPRINTF(("gettranslations: mmu %#x, prop %#x, len %#x, *prop %#x, *len %d\n", mmu, prop, len, p, o));

	if (mmu == NULL)
		return 0;

	l = sf_getproplen(mmu, "translations");

	if (l <= 0)
		return 0;

	DPRINTF(("gettranslations: l %d\n", l));

	while (l > o)
	{
		/* allocate space for translations property and get new length */
		if (p != NULL)
			sf_release(p, o);

		o = l + 32;
		p = sf_claim((void *)0, o, 4);
		DPRINTF(("gettranslations: p %#x, o %d\n", p, o));

		if (p == NULL || (Int)p == -1)
			return 0;

		*(uInt volatile *)p = 0xDEADBEEF;

		if (*(uInt volatile *)p != 0xDEADBEEF)
			DPRINTF(("gettranslations: p flakey! word[0] %#x\n",
					*(uInt volatile *)p));

		l = sf_getproplen(mmu, "translations");

		if (l <= 0)
			return 0;

		DPRINTF(("gettranslations: l %d\n", l));
	}

	DPRINTF(("gettranslations: get mmu %#x, p %#x, o %d\n", mmu, p, o));

	/* get translations property */
	l = sf_getprop(mmu, "translations", p, o);

	if (l <= 0)
		return 0;

	DPRINTF(("gettranslations: get l %d\n", l));

	*prop = p;
	*len = o;

	DPRINTF(("gettrans: t[0] %#x %#x %#x %#x\n", ((uInt *)p)[0], ((uInt *)p)[1],
			((uInt *)p)[2], ((uInt *)p)[3]));
	return l;
}

void
hexdumptable(uInt *table)
{
	int i;

	for (i = 0; i < 4096; i++)
		if (table[i])
			sf_printf("table[%X] = %#x\n", i, table[i]);
}

void
dumptable(uInt *table, uInt tablepa, uInt len)
{
	uInt addr;
	uInt va = 0;
	uInt pa = 0;
	uInt size = 0;
	uInt mode = 0;
	uInt ste;
	uInt ppa;
	uInt *ptable;
	uInt pte;
	int first = 1;
	int sec;
	int pg;
	hexdumptable(table);
	sf_printf("dumptable: table %p, tablepa %#x, len %d\n", table, tablepa, len);

	addr = 0;

	while (first || addr != 0)
	{
		sec = addr >> 20;
		pg = (addr >> 12) & 0xFF;
		ste = table[sec];
		first = 0;

		/* check for the start of a first mapping */
		switch (ste & 3)
		{
		case FAULT:
		case RESERVED:
			addr += 0x100000;
			continue;
		case SECT:
			va = addr;
			pa = ste & 0xFFF00000;
			mode = ste & 0xDEC;
			size = 0x100000;
			break;
		case PAGE:
			ppa = ste & 0xFFFFFC00;
			mode = ste & 0x1E0;

			if (ppa < tablepa && ppa >= tablepa + len)
			{
				addr += 0x100000;
				continue;
			}

			ptable = (uInt *)((uInt)table + (ppa - tablepa));
			pte = ptable[pg];
			va = addr;
			mode |= pte & 0xC0C;

			switch (pte & 3)
			{
			case FAULT:
			case RESERVED:
				addr += 0x1000;
				continue;
			case LARGE_PAGE:
				pa = pte & 0xFFFF0000;
				size = 0x10000;
				break;
			case SMALL_PAGE:
				pa = pte & 0xFFFFF000;
				size = 0x1000;
				break;
			}

			break;
		}

		/* if we get here we have a valid  mapping */
		addr += size;

		while (addr != 0)
		{
			sec = addr >> 20;
			pg = (addr >> 12) & 0xFF;
			ste = table[sec];

			switch (ste & 3)
			{
			case FAULT:
			case RESERVED:
				break;
			case SECT:
				if (pa + size != (ste & 0xFFF00000))
					break;

				if ((ste & 0xDEC) != mode)
					break;

				addr += 0x100000;
				size += 0x100000;
				continue;
			case PAGE:
				if ((mode & 0x1E0) != (ste & 0x1E0))
					break;

				ppa = ste & 0xFFFFFC00;

				if (ppa < tablepa && ppa >= tablepa + len)
					break;

				ptable = (uInt *)((uInt)table + (ppa - tablepa));
				pte = ptable[pg];

				if ((mode & 0xC0C) != (pte & 0xC0C))
					break;

				switch (pte & 3)
				{
				case FAULT:
				case RESERVED:
					break;
				case LARGE_PAGE:
					if (pa + size != (pte & 0xFFFF0000))
						break;

					addr += 0x10000;
					size += 0x10000;
					continue;
				case SMALL_PAGE:
					if (pa + size != (pte & 0xFFFFF000))
						break;

					addr += 0x1000;
					size += 0x1000;
					continue;
				}

				break;
			}

			break;
		}

		/* print out the mapping */
		sf_printf("%#x -> %#x: %#x (%#x)\n", va, pa, size, mode);
	}
}

int
takeovermmu()
{
	Instance *choseninst;
	Package *chosenpkg;
	Instance *mmuinst;
	Package *mmupkg;
	Int len;
	uByte *translationsprop = NULL;
	uInt translationsproplen = 0;
	int numtables;
	int priornumtables;
	int l1tablelen = 0;
	uByte *l1table = NULL;
	uInt l1pa;
	DPRINTF(("takeovermmu:\n"));

	choseninst = sf_open("/chosen");

	if (choseninst == NULL)
		return 0;

	DPRINTF(("takeovermmu: choseninst %#x\n", choseninst));
	chosenpkg = sf_instance_to_package(choseninst);

	if (chosenpkg == NULL)
	{
		sf_close(choseninst);
		return 0;
	}

	DPRINTF(("takeovermmu: chosenpkg %#x\n", chosenpkg));
	len = sf_getprop(chosenpkg, "mmu", (uByte *)&mmuinst, sizeof mmuinst);
	mmuinst = (Instance *)SWAP4((uInt)mmuinst);

	if (len != sizeof mmuinst || mmuinst == NULL)
	{
		sf_close(choseninst);
		return 0;
	}

	DPRINTF(("takeovermmu: mmuinst %#x\n", mmuinst));
	sf_close(choseninst);
	mmupkg = sf_instance_to_package(mmuinst);

	if (mmupkg == NULL)
		return 0;

	DPRINTF(("takeovermmu: mmupkg %#x\n", mmupkg));

	/* get translations property */
	len = gettranslations(mmupkg, &translationsprop, &translationsproplen);

	if (len <= 0)
		return 0;

	DPRINTF(("takeovermmu: translationsprop %#x, propbuflen %d, len %d\n",
			translationsprop, translationsproplen, len));

	/* determine number of directories needed */
	priornumtables = -1;
	numtables = counttables(translationsprop, len);

	DPRINTF(("takeovermmu: numtables %d\n", numtables));

	while (numtables > priornumtables)
	{
		/* allocate L1 and L2 tables aligned on a 16K boundry */
		l1tablelen = 16*1024 + 1024 * numtables;
		l1table = sf_claim((void *)0, l1tablelen, 16*1024);

		if (l1table == (void *)0)
			return 0;

		DPRINTF(("takeovermmu: l1table %#x, l1tablelen %d\n", l1table, l1tablelen));

		*(uInt *)l1table = 0x12345678;

		if (*(uInt volatile *)l1table != 0x12345678)
			DPRINTF(("takeovermmu: l1table flakey! word[0] %#x\n",
					*(uInt volatile *)l1table));

		/* get translations property again since it may have changed */
		/* with the above claim */
		len = gettranslations(mmupkg, &translationsprop,
				&translationsproplen);

		if (len <= 0)
			return 0;

		priornumtables = numtables;
		numtables = counttables(translationsprop, len);
	}

	/* build L1 and L2 tables */
	if (!buildtables(mmuinst, l1table, l1tablelen, translationsprop, len))
		return 0;

	sf_printf("New page tables built\n");

	if (!getpa(mmuinst, (uInt)l1table, &l1pa))
		return 0;

	dumptable((uInt *)l1table, l1pa, l1tablelen);

	sf_printf("About to switch page tables\n");

	/* flush tlb and caches */
	asm("mov	r0,%0 ; "
    	    "mov     	r1, #0x00000000 ; "
    	    "mcr     	p15, 0, r1, c7, c7, 0 ; "	/* flush caches */
	    /* reload L1 table pointer */
    	    "mcr     	p15, 0, r0, c2, c0 ; "	/* load L1 table pointer */
	    /* flush tlb */
    	    "mcr     	p15, 0, r1, c8, c7, 0 ; "	/* flush TLB */
	    : : "r"(l1pa) : "r0","r1"); 	/* NOP */

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
