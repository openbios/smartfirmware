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

/* MMU dependancies here - needs to be customized for a particular system */

/*#define DEBUG*/

#include <stdlib.h>

#include "defs.h"

extern void flush_caches();
extern void flush_tlb();

#define MODE_MASK	0x00000DEC
#define MODE_AP_MASK	0x00000C00
#define MODE_DOM_MASK	0x000001E0
#define MODE_CB_MASK	0x0000000C
#define MODE_CLAIM	0x00000C0C
#define MODE_MAP_IN	0x00000C00

#define PAGE_SIZE	(4096)
#define PAGE_MASK	(PAGE_SIZE - 1)
#define SECTION_SIZE	(1048576)
#define SECTION_MASK	(SECTION_SIZE - 1)

#define STE_TABLE_PHYS_MASK	0xFFFFFC00
#define STE_SECTION_PHYS_MASK	0xFFF00000
#define STE_AP_MASK		0x00000C00
#define STE_DOMAIN_MASK		0x000001E0
#define STE_MBO			0x00000010
#define STE_C_MASK		0x00000008
#define STE_B_MASK		0x00000004
#define STE_TYPE_MASK		0x00000003

#define STE_TYPE_FAULT		0
#define STE_TYPE_PAGE		1
#define STE_TYPE_SECTION	2
#define STE_TYPE_RESERVED	3

#define PTE_LARGE_PHYS_MASK	0xFFFF0000
#define PTE_SMALL_PHYS_MASK	0xFFFFF000
#define PTE_AP_MASKS		0x00000FF0
#define PTE_C_MASK		0x00000008
#define PTE_B_MASK		0x00000004
#define PTE_TYPE_MASK		0x00000003

#define PTE_TYPE_FAULT		0
#define PTE_TYPE_LARGE		1
#define PTE_TYPE_SMALL		2
#define PTE_TYPE_RESERVED	3

struct mapping
{
	uPtr virt;
	uPtr phys;
	uInt size;
	uInt mode;
};

const struct mapping defaultmappings[] =
{
	{ EDB7312_VA_EP7312, EDB7312_PA_EP7312, EDB7312_SZ_EP7312, MODE_MAP_IN },
	{ EDB7312_VA_SRAM, EDB7312_PA_SRAM, EDB7312_SZ_SRAM, MODE_MAP_IN },
	{ EDB7312_VA_CS8900A, EDB7312_PA_CS8900A, EDB7312_SZ_CS8900A, MODE_MAP_IN },
	{ EDB7312_VA_LPT, EDB7312_PA_LPT, EDB7312_SZ_LPT, MODE_MAP_IN },
	{ EDB7312_VA_KBDEXTROW, EDB7312_PA_KBDEXTROW, EDB7312_SZ_KBDEXTROW,
			MODE_MAP_IN },
	{ EDB7312_VA_USB, EDB7312_PA_USB, EDB7312_SZ_USB, MODE_MAP_IN },
	{ EDB7312_VA_PS2, EDB7312_PA_PS2, EDB7312_SZ_PS2, MODE_MAP_IN },
	{ EDB7312_VA_IDE, EDB7312_PA_IDE, EDB7312_SZ_IDE, MODE_MAP_IN },
	{ EDB7312_VA_NAND, EDB7312_PA_NAND, EDB7312_SZ_NAND, MODE_MAP_IN },
	{ EDB7312_VA_NVRAM, EDB7312_PA_NVRAM, EDB7312_SZ_NVRAM, MODE_MAP_IN },
	{ 0, 0, 0 },		/* end of list */
};

static void
acopy(uInt *dst, uInt *src, int size)
{
	int i;

	for (i = 0; i < size; i += sizeof (uInt))
		*dst++ = *src++;
}

void
init_virtual()
{
	int i;
	static void unmap_region(uPtr virt, uInt size);
	static void map_region(uPtr virt, uPtr phys, uInt size, uInt mode);
	uByte *pagezero;
	uInt *vectors;
	extern char exception_vectors[];
	extern char start[];
	extern uInt g_ep7312_base;

	unmap_region((uPtr)0xF7200000, (uInt)0x01E00000);

	for (i = 0; defaultmappings[i].size; i++)
		map_region(defaultmappings[i].virt, defaultmappings[i].phys,
				defaultmappings[i].size, defaultmappings[i].mode);

	flush_caches();
	flush_tlb();
	g_ep7312_base = EDB7312_VA_EP7312;

	/* setup page zero */
	pagezero = (uByte *)0xF70FB000;
	acopy((uInt *)pagezero, (uInt *)0, PAGE_SIZE);

	map_region((uPtr)0, (uPtr)0xC00FB000, PAGE_SIZE, MODE_CLAIM);

	flush_caches();
	flush_tlb();

	/* unmap all unused memory */
	/* leave the first meg mapped so that NVRAM can be read out of flash */
	unmap_region((uPtr)0x00001000, (uInt)0xEFFFF000);
	unmap_region((uPtr)0xF0F00000, (uInt)0x06100000);
	unmap_region((uPtr)0xF9000000, (uInt)0x07000000);

	flush_tlb();

	/* copy in our exception vectors */
	acopy((uInt *)0, (uInt *)exception_vectors, start - exception_vectors);

#define ROM_OFFSET	0x08FE0000

	/* relocate exception vectors */
	vectors = (uInt *)0x40;

	if ((vectors[0] >> 16) < 0xC000)
	{
		vectors[0] -= ROM_OFFSET;
		vectors[1] -= ROM_OFFSET;
		vectors[2] -= ROM_OFFSET;
		vectors[3] -= ROM_OFFSET;
		vectors[4] -= ROM_OFFSET;
		vectors[5] -= ROM_OFFSET;
		vectors[6] -= ROM_OFFSET;
		vectors[7] -= ROM_OFFSET;
	}


	DPRINTF(("vector[0] = %#x\n", vectors[0]));
	DPRINTF(("vector[1] = %#x\n", vectors[1]));
	DPRINTF(("vector[2] = %#x\n", vectors[2]));
	DPRINTF(("vector[3] = %#x\n", vectors[3]));
	DPRINTF(("vector[4] = %#x\n", vectors[4]));
	DPRINTF(("vector[5] = %#x\n", vectors[5]));
	DPRINTF(("vector[6] = %#x\n", vectors[6]));
	DPRINTF(("vector[7] = %#x\n", vectors[7]));
}

#define VA_OFFSET		(0xF7000000 - 0xC0000000)
#define OF_PA_TO_VA(pa)		((uPtr)((uPtr)(pa) + VA_OFFSET))
#define OF_VA_TO_PA(va)		((uPtr)((uPtr)(va) - VA_OFFSET))

static uPtr
section_table_base()
{
	uInt mapaddr;

	asm(" mrc     	p15, 0, %0, c2, c0" : "=r"(mapaddr));
	mapaddr &= 0xFFFFC000;
	return OF_PA_TO_VA(mapaddr);
}

static void
unmap_section(uPtr section)
{
	uInt *stebase = (uInt *)section_table_base();
	int secindex = (section >> 20) & 0xFFF;
	uInt ste = stebase[secindex];
#ifdef DEBUG2
	DPRINTF(("unmap_section: section table base: %p, ste %#x\n", stebase, ste));
#endif

	switch (ste & STE_TYPE_MASK)
	{
	case STE_TYPE_FAULT:
		/* invalid */
		return;
	case STE_TYPE_PAGE:
		/* page table */

		/* free page table */
		free((uInt *)OF_PA_TO_VA(ste & STE_TABLE_PHYS_MASK));

		/* mark section invalid */
		stebase[secindex] = STE_TYPE_FAULT;
		return;
	case STE_TYPE_SECTION:
		/* section */

		/* mark section invalid */
		stebase[secindex] = STE_TYPE_FAULT;
		return;
	case STE_TYPE_RESERVED:
		/* reserved */
		return;
	}
}

static void
cvt_section_to_pages(uPtr section)
{
	uInt *stebase = (uInt *)section_table_base();
	uInt *ptebase;
	int secindex = (section >> 20) & 0xFFF;
	uInt ste = stebase[secindex];
	uInt pte;
	int i;

	DPRINTF(("cvt_sec_to_pages: section %#x, section index %#x\n", section, secindex));
	DPRINTF(("cvt_sec_to_pages: stebase %p, ste %#x\n", stebase, ste));

	if ((ste & STE_TYPE_MASK) == STE_TYPE_SECTION)	/* check if section */
	{
		/* allocate page table */
		ptebase = (uInt *)memalign(1024, 1024);		/* allocate page table */

		pte = ((ste & STE_AP_MASK) >> 10) << 4;		/* copy AP in PTE's */
		pte |= pte << 2;
		pte |= pte << 4;
		pte |= ste & (STE_SECTION_PHYS_MASK|STE_C_MASK|STE_B_MASK);
		pte |= PTE_TYPE_SMALL;

		/* creating initial mappings */
		for (i = 0; i < 256; i++)
			ptebase[i] = pte | (i << 12);

		/* point section at the page table */
		stebase[secindex] = (ste & STE_DOMAIN_MASK) | OF_VA_TO_PA(ptebase) | STE_TYPE_PAGE;
	}
}

static void
unmap_page(uPtr page)
{
	uInt *stebase = (uInt *)section_table_base();
	uInt *ptebase;
	int secindex = (page >> 20) & 0xFFF;
	int pageindex = (page >> 12) & 0xFF;
	uInt ste = stebase[secindex];
	int i;

	DPRINTF(("unmap_page: page %#x, section index %#x, page index %#x\n", page, secindex, pageindex));
#if 0
	DPRINTF(("unmap_page: stebase %p, ste %#x\n", stebase, ste));
#endif

	switch (ste & STE_TYPE_MASK)
	{
	case STE_TYPE_FAULT:
		/* invalid */
		return;
	case STE_TYPE_PAGE:
		/* page tables */

		/* find page table base */
		ptebase = (uInt *)OF_PA_TO_VA(ste & STE_TABLE_PHYS_MASK);

		/* mark this pages entry invalid */
		ptebase[pageindex] = PTE_TYPE_FAULT;

		/* check to see if all the pages are invalid */
		for (i = 0; i < 256; i++)
			if ((ptebase[i] & PTE_TYPE_MASK) != PTE_TYPE_FAULT)
				break;

		if (i == 256)
		{
			DPRINTF(("unmap_page: page table complete invalid, releasing\n"));
			/* if so free page table and */
			free(ptebase);
			/* mark section invalid */
			stebase[secindex] = STE_TYPE_FAULT;
		}
				
		return;
	case STE_TYPE_SECTION:
		/* section */

		/* convert to a page table */
		cvt_section_to_pages(page);
		ste = stebase[secindex];

		/* find page table base */
		ptebase = (uInt *)OF_PA_TO_VA(ste & STE_TABLE_PHYS_MASK);

		/* mark this pages entry invalid */
		ptebase[pageindex] = PTE_TYPE_FAULT;

		return;
	case STE_TYPE_RESERVED:
		/* reserved */
		return;
	}
}

static void
unmap_region(uPtr virt, uInt size)
{
	virt &= PTE_SMALL_PHYS_MASK;
	DPRINTF(("unmap_region: virt %#x, size %#x\n", virt, size));

	while (size)
	{
		if ((virt & ~STE_SECTION_PHYS_MASK) || size < SECTION_SIZE)
		{
			unmap_page(virt);
			virt += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
		else
		{
			unmap_section(virt);
			virt += SECTION_SIZE;
			size -= SECTION_SIZE;
		}
	}
}

static void
map_section(uPtr section, uInt phys, uInt mode)
{
	uInt *stebase = (uInt *)section_table_base();
	int secindex = (section >> 20) & 0xFFF;
#ifdef DEBUG
	uInt ste = stebase[secindex];
	DPRINTF(("map_section: virt %#x, phys %#x, mode %#x: section table base: %p, ste %#x\n", section, phys, mode, stebase, ste));
#endif
	stebase[secindex] = phys | mode | STE_TYPE_SECTION;
}

static void
map_page(uPtr page, uInt phys, uInt mode)
{
	uInt *stebase = (uInt *)section_table_base();
	int secindex = (page >> 20) & 0xFFF;
	int pageindex = (page >> 12) & 0xFF;
	uInt ste = stebase[secindex];
	uInt *ptebase;
	uInt pte;
	int i;

	DPRINTF(("map_page: page %#x, phys %#x, mode %#x\n", page, phys, mode));
#if 0
	DPRINTF(("map_page: page %#x, section index %#x, page index %#x\n", page, secindex, pageindex));
	DPRINTF(("map_page: stebase %p, ste %#x\n", stebase, ste));
#endif

	switch (ste & STE_TYPE_MASK)
	{
	case STE_TYPE_FAULT: /* invalid */
	case STE_TYPE_RESERVED: /* reserved */
		/* create page table with all entries invalid */

		/* allocate page table */
		ptebase = (uInt *)memalign(1024, 1024);		/* allocate page table */

		pte = 0xFF0 | PTE_TYPE_FAULT;	/* AP=3, C=0, B=0, Type=Fault */

		/* creating initial mappings */
		for (i = 0; i < 256; i++)
			ptebase[i] = pte;

		/* point section at the page table */
		stebase[secindex] = OF_VA_TO_PA(ptebase) | STE_TYPE_PAGE |
				(mode & MODE_DOM_MASK);

		pte = mode & MODE_AP_MASK;
		pte |= pte >> 2;
		pte |= pte >> 4;
		pte |= mode & MODE_CB_MASK;
		pte |= PTE_TYPE_SMALL;	/* AP=mode.ap, C=mode.c, B=mode.b, Type=Small */
		pte |= phys & PTE_SMALL_PHYS_MASK;

		/* load this page table entry */
		ptebase[pageindex] = pte;
		return;
	case STE_TYPE_SECTION:
		/* section */

		/* convert to a page table */
		cvt_section_to_pages(page);
		ste = stebase[secindex];

		/* FALLTHROUGH */
	case STE_TYPE_PAGE:
		/* page table */
		/* find page table base */
		ptebase = (uInt *)OF_PA_TO_VA(ste & STE_TABLE_PHYS_MASK);

		pte = mode & MODE_AP_MASK;
		pte |= pte >> 2;
		pte |= pte >> 4;
		pte |= mode & MODE_CB_MASK;
		pte |= phys & PTE_SMALL_PHYS_MASK;
		pte |= PTE_TYPE_SMALL;

		/* load this page table entry */
		ptebase[pageindex] = pte;
		return;
	}
}

static void
map_region(uPtr virt, uPtr phys, uInt size, uInt mode)
{
	virt &= PTE_SMALL_PHYS_MASK;
	phys &= PTE_SMALL_PHYS_MASK;
	DPRINTF(("map_region: virt %#x, phys %#x, size %#x, mode %#x\n", virt, phys, size, mode));

	while (size)
	{
		if ((virt & ~STE_SECTION_PHYS_MASK) || (phys & ~STE_SECTION_PHYS_MASK) ||
				size < SECTION_SIZE)
		{
			map_page(virt, phys, mode);
			virt += PAGE_SIZE;
			phys += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
		else
		{
			map_section(virt, phys, mode);
			virt += SECTION_SIZE;
			phys += SECTION_SIZE;
			size -= SECTION_SIZE;
		}
	}
}

/* machine specific function for mapping address space */
Retcode
mmu_map(Environ *e, uInt phys[], Cell virt, Cell size, uInt mode)
{
	uPtr p = (uPtr)phys[0];
	uPtr v = (uPtr)virt;
	uInt s = (uInt)size;
	uInt o;

	DPRINTF(("mmu_map: e %p, phys %#x, virt %#x, size %#x, mode %#x\n", e, phys[0], virt, size, mode));

	o = p & PAGE_MASK;

	if ((v & PAGE_MASK) != o)
		return E_ADDR_ALIGN_ERR;

	s += o;
	p &= ~PAGE_MASK;
	v &= ~PAGE_MASK;
	s = (s + PAGE_SIZE - 1) & ~PAGE_MASK;

	flush_caches();
	map_region(v, p, s, mode & MODE_MASK);
	flush_caches();
	flush_tlb();
	return NO_ERROR;
}

Retcode
mmu_unmap(Environ *e, Cell virt, Cell size)
{
	uPtr v = (uPtr)virt;
	uInt s = (uInt)size;

	DPRINTF(("mmu: unmap: virt %#x, size %#x\n", virt, size));

	s += v & PAGE_MASK;
	v &= ~PAGE_MASK;
	s = (s + PAGE_SIZE - 1) & ~PAGE_MASK;
	DPRINTF(("mmu: unmap: new virt %#x, size %#x\n", virt, size));

	flush_caches();
	unmap_region(v, s);
	flush_caches();
	flush_tlb();
	DPRINTF(("mmu: unmap: unmap_region done\n"));
	return NO_ERROR;
}

Retcode
mmu_modify(Environ *e, Cell virt, Cell size, uInt mode)
{
	return NO_ERROR;
}

Retcode
mmu_translate(Environ *e, Cell virt, uInt phys[], uInt *mode)
{
	uInt *stebase = (uInt *)section_table_base();
	uInt *ptebase;
	int secindex = (virt >> 20) & 0xFFF;
	int pageindex = (virt >> 12) & 0xFF;
	uInt ste = stebase[secindex];
	uInt pte = 0;

	DPRINTF(("mmu_translate: section %#x, section index %#x\n", virt, secindex));
	DPRINTF(("mmu_translate: stebase %p, ste %#x\n", stebase, ste));

	switch (ste & STE_TYPE_MASK)
	{
	case STE_TYPE_FAULT:
	case STE_TYPE_RESERVED:
		phys[0] = 0;
		*mode = 0;
		return E_INVALID_MEMORY_ADDR;
	case STE_TYPE_SECTION:
		phys[0] = (ste & STE_SECTION_PHYS_MASK) | (virt & ~STE_SECTION_PHYS_MASK);
		*mode = ste & MODE_MASK;
		break;
	case STE_TYPE_PAGE:
		ptebase = (uInt *)OF_PA_TO_VA(ste & STE_TABLE_PHYS_MASK);
		pte = ptebase[pageindex];
		*mode = ste & MODE_DOM_MASK;

		switch (pte & PTE_TYPE_MASK)
		{
		case PTE_TYPE_FAULT:
		case PTE_TYPE_RESERVED:
			phys[0] = 0;
			*mode = 0;
			return E_INVALID_MEMORY_ADDR;
		case PTE_TYPE_LARGE:
			phys[0] = (pte & PTE_LARGE_PHYS_MASK) | (virt & ~PTE_LARGE_PHYS_MASK);
			*mode |= pte & (MODE_AP_MASK|MODE_CB_MASK);
			break;
		case PTE_TYPE_SMALL:
			phys[0] = (pte & PTE_SMALL_PHYS_MASK) | (virt & ~PTE_SMALL_PHYS_MASK);
			*mode |= pte & (MODE_AP_MASK|MODE_CB_MASK);
			break;
		}
	}

	DPRINTF(("mmu_translate: result phys %#x, mode %#x\n", phys[0], *mode));
	return NO_ERROR;
}

Retcode
mmu_dma_alloc(Environ *e, Cell size, Cell *virt)
{
	void *mem = malloc((size_t)size);

	if (mem == NULL)
	{
		*virt = (Cell)0;
		return E_OUT_OF_MEMORY;
	}

	*virt = (Cell)mem;
	return NO_ERROR;
}

Retcode
mmu_dma_free(Environ *e, Cell virt, Cell size)
{
	void *mem = (void *)virt;
	free(mem);
	return NO_ERROR;
}

Retcode
mmu_dma_map_in(Environ *e, Cell virt, Cell size, Int type,
		uInt phys[])
{
	phys[0] = (uInt)OF_VA_TO_PA(virt);
	return NO_ERROR;
}

Retcode
mmu_dma_map_out(Environ *e, Cell virt, uInt phys[], Cell size)
{
	return NO_ERROR;
}

Retcode
mmu_dma_sync(Environ *e, Cell virt, uInt phys[], Cell size)
{
	flush_caches();
	return NO_ERROR;
}

const struct mapping physicalmappings[] =
{
	{ EDB7312_PA_EP7312, EDB7312_PA_EP7312, EDB7312_SZ_EP7312, MODE_MAP_IN },
	{ EDB7312_PA_SRAM, EDB7312_PA_SRAM, EDB7312_SZ_SRAM, MODE_MAP_IN },
	{ EDB7312_PA_CS8900A, EDB7312_PA_CS8900A, EDB7312_SZ_CS8900A, MODE_MAP_IN },
	{ EDB7312_PA_LPT, EDB7312_PA_LPT, EDB7312_SZ_LPT, MODE_MAP_IN },
	{ EDB7312_PA_KBDEXTROW, EDB7312_PA_KBDEXTROW, EDB7312_SZ_KBDEXTROW,
			MODE_MAP_IN },
	{ EDB7312_PA_USB, EDB7312_PA_USB, EDB7312_SZ_USB, MODE_MAP_IN },
	{ EDB7312_PA_PS2, EDB7312_PA_PS2, EDB7312_SZ_PS2, MODE_MAP_IN },
	{ EDB7312_PA_IDE, EDB7312_PA_IDE, EDB7312_SZ_IDE, MODE_MAP_IN },
	{ EDB7312_PA_NAND, EDB7312_PA_NAND, EDB7312_SZ_NAND, MODE_MAP_IN },
	{ 0xC0000000, 0xC0000000, 0x01000000, MODE_CLAIM },	/* 16MB system memory */
	{ 0, 0, 0 },		/* end of list */
};

Retcode
mmu_add_physical_mappings(Environ *e)
{
	int i;

	if (e->mmu)
	{
		Retcode ret;
		Cell base;
		IFCKSP(e, 0, 4);

		for (i = 0; physicalmappings[i].size; i++)
		{
			/* allocate the virtual address space */
			PUSH(e, physicalmappings[i].virt);
			PUSH(e, physicalmappings[i].size);
			PUSH(e, 0);
			ret = execute_static_method_name(e, e->mmu, "claim", CSTR);

			if (ret != NO_ERROR)
				return ret;

			POP(e, base);

			/* map the region */
			PUSH(e, physicalmappings[i].phys);
			PUSH(e, physicalmappings[i].virt);
			PUSH(e, physicalmappings[i].size);
			PUSH(e, physicalmappings[i].mode);
			ret = execute_static_method_name(e, e->mmu, "map", CSTR);

			if (ret != NO_ERROR)
				return ret;
		}

		return NO_ERROR;
	}


	for (i = 0; physicalmappings[i].size; i++)
		map_region(physicalmappings[i].virt, physicalmappings[i].phys,
				physicalmappings[i].size, physicalmappings[i].mode);

	return NO_ERROR;
}

static int
read_mapping(uInt *virt, uInt *phys, uInt *size, uInt *mode)
{
	uInt v = *virt;
	uInt p;
	uInt p2;
	uInt sz;
	uInt m;
	uInt m2;
	uInt *stebase = (uInt *)section_table_base();
	uInt *ptebase;
	int secindex = (v >> 20) & 0xFFF;
	int pageindex = (v >> 12) & 0xFF;
	uInt ste = stebase[secindex];
	uInt pte = 0;
	int valid = 0;

	DPRINTF(("read_mapping: section %#x, section index %#x\n", *virt, secindex));
	DPRINTF(("read_mapping: stebase %p, ste %#x\n", stebase, ste));

	if ((ste & STE_TYPE_MASK) != STE_TYPE_PAGE)
		valid = ((ste & STE_TYPE_MASK) == STE_TYPE_SECTION);
	else
	{
		ptebase = (uInt *)OF_PA_TO_VA(ste & STE_TABLE_PHYS_MASK);
		pte = ptebase[pageindex];
		valid = (((pte & PTE_TYPE_MASK) == PTE_TYPE_LARGE) ||
			((pte & PTE_TYPE_MASK) == PTE_TYPE_SMALL));
	}

	/* find the first non fault mapping starting at virt */
	while (!valid)
	{
		pageindex++;

		if ((ste & STE_TYPE_MASK) != STE_TYPE_PAGE || pageindex == 0x100)
		{
			secindex++;
			pageindex = 0;

			if (secindex == 0x1000)
				return 0;
		}

		ste = stebase[secindex];

		if ((ste & STE_TYPE_MASK) != STE_TYPE_PAGE)
			valid = ((ste & STE_TYPE_MASK) == STE_TYPE_SECTION);
		else
		{
			ptebase = (uInt *)OF_PA_TO_VA(ste & STE_TABLE_PHYS_MASK);
			pte = ptebase[pageindex];
			valid = (((pte & PTE_TYPE_MASK) == PTE_TYPE_LARGE) ||
				((pte & PTE_TYPE_MASK) == PTE_TYPE_SMALL));
		}
	}

	v = (secindex << 20) | (pageindex << 12);
	m = ste & MODE_DOM_MASK;

	if ((ste & STE_TYPE_MASK) != STE_TYPE_PAGE)
	{
		p = ste & STE_SECTION_PHYS_MASK;
		sz = 0x100000;
		m |= ste & (MODE_AP_MASK|MODE_CB_MASK);
	}
	else if ((pte & PTE_TYPE_MASK) != PTE_TYPE_SMALL)
	{
		p = pte & PTE_LARGE_PHYS_MASK;
		sz = 0x10000;
		m |= pte & (MODE_AP_MASK|MODE_CB_MASK);
	}
	else
	{
		p = pte & PTE_SMALL_PHYS_MASK;
		sz = 0x1000;
		m |= pte & (MODE_AP_MASK|MODE_CB_MASK);
	}

	/* find the next non-contiguous mapping starting at virt */
	while (valid)
	{
		pageindex++;

		if ((ste & STE_TYPE_MASK) != STE_TYPE_PAGE || pageindex == 0x100)
		{
			secindex++;
			pageindex = 0;

			if (secindex == 0x1000)
				break;
		}

		ste = stebase[secindex];

		if ((ste & STE_TYPE_MASK) != STE_TYPE_PAGE)
			valid = ((ste & STE_TYPE_MASK) == STE_TYPE_SECTION);
		else
		{
			ptebase = (uInt *)OF_PA_TO_VA(ste & STE_TABLE_PHYS_MASK);
			pte = ptebase[pageindex];
			valid = (((pte & PTE_TYPE_MASK) == PTE_TYPE_LARGE) ||
				((pte & PTE_TYPE_MASK) == PTE_TYPE_SMALL));
		}

		if (!valid)
			break;

		m2 = ste & MODE_DOM_MASK;

		if ((ste & STE_TYPE_MASK) != STE_TYPE_PAGE)
		{
			p2 = ste & STE_SECTION_PHYS_MASK;
			m2 |= ste & (MODE_AP_MASK|MODE_CB_MASK);

			if (p + sz != p2 || m != m2)
				break;

			sz += 0x100000;
		}
		else if ((pte & PTE_TYPE_MASK) != PTE_TYPE_SMALL)
		{
			p2 = pte & PTE_LARGE_PHYS_MASK;
			m2 |= pte & (MODE_AP_MASK|MODE_CB_MASK);

			if (p + sz != p2)
				break;

			sz += 0x10000;
		}
		else
		{
			p2 = pte & PTE_SMALL_PHYS_MASK;
			m2 |= pte & (MODE_AP_MASK|MODE_CB_MASK);

			if (p + sz != p2)
				break;

			sz += 0x1000;
		}
	}

	*virt = v;
	*phys = p;
	*size = sz;
	*mode = m;
	return 1;
}

Retcode
mmu_init_translations(Environ *e)
{
	uInt virt;
	uInt phys;
	uInt size;
	uInt mode;
	Int inst;
	Retcode ret;

	for (virt = 0; read_mapping(&virt, &phys, &size, &mode); virt += size)
	{
		ret = add_translation(e, phys, virt, size, mode);

		if (ret != NO_ERROR)
			return ret;
	}

	ret = prop_get_int(e->chosen->props, "mmu", CSTR, &inst);

	if (ret != NO_ERROR)
		return ret;

	return make_translations(e, ((Instance *)inst)->package);
}

#define MMU_MAP_IN_DATA			0x0001
#define MMU_MAP_IN_EXEC			0x0002
#define MMU_MAP_IN_IO			0x0004

#define MMU_DMA_MAP_IN_NOCACHE		0x0001
#define MMU_DMA_MAP_IN_CACHEABLE	0x0002

C(f_mapq)
{
	uInt *mapbase;
	uInt *pagebase;
	uInt addr;
	uInt pte;

	IFCKSP(e, 1, 1);
	POP(e, addr);

	mapbase = (uInt *)section_table_base();
	pte = mapbase[addr >> 20];

	switch (pte & STE_TYPE_MASK)
	{
	case STE_TYPE_FAULT:
		cprintf(e, "%#x: fault %#x\n", addr, pte);
		break;
	case STE_TYPE_PAGE:
		pagebase = (uInt *)OF_PA_TO_VA(pte & STE_TABLE_PHYS_MASK);
		cprintf(e, "%#x: %#x: page table %#x: domain:%d\n", addr, pte, pte & STE_TABLE_PHYS_MASK,
				(pte >> 5) & 0xF);
		pte = pagebase[(addr >> 12) & 0xFF];

		switch (pte & PTE_TYPE_MASK)
		{
		case PTE_TYPE_FAULT:
			cprintf(e, "%#x: fault %#x\n", addr, pte);
			break;
		case PTE_TYPE_LARGE:
			cprintf(e, "%#x: %#x: large page %#x: AP:%d C:%d B:%d\n", addr,
					pte, pte & PTE_LARGE_PHYS_MASK, (pte >> (4 + ((addr >> 14) & 3))) & 3,
					(pte >> 3) & 1, (pte >> 2) & 1);
			break;
		case PTE_TYPE_SMALL:
			cprintf(e, "%#x: %#x: small page %#x: AP:%d C:%d B:%d\n", addr,
					pte, pte & PTE_SMALL_PHYS_MASK, (pte >> (4 + ((addr >> 10) & 3))) & 3,
					(pte >> 3) & 1, (pte >> 2) & 1);
			break;
		case PTE_TYPE_RESERVED:
			cprintf(e, "%#x: reserved %#x\n", addr, pte);
			break;
		}

		break;
	case STE_TYPE_SECTION:
		/* section mapping */
		cprintf(e, "%#x: %#x: section %#x: AP:%d domain:%d C:%d B:%d offset:%#x\n", addr,
				pte, pte & STE_SECTION_PHYS_MASK, (pte >> 10) & 3, (pte >> 5) & 0xF,
				(pte >> 3) & 1, (pte >> 2) & 1, addr & 0xFFFFF);
		break;
	case STE_TYPE_RESERVED:
		cprintf(e, "%#x: reserved %#x\n", addr, pte);
		break;
	}

	return NO_ERROR;
}

const Initentry init_mmu[] =
{
	{ "map?", f_mapq, INVALID_FCODE, F_NONE, T_FUNC },
	{ NULL, NULL }
};
