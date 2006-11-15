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

/* (c) Copyright 2001,2003 by CodeGen, Inc.  All Rights Reserved. */

/* MMU dependancies here - needs to be customized for a particular system */

/*#define DEBUG*/

#include <stdlib.h>

#include "defs.h"

extern void flush_caches();
extern void flush_tlb();

#define MODE_MASK		0xFFF00FFF
#define MODE_NX_MASK	0x80000000
#define MODE_AVL2_MASK	0x7FF00000
#define MODE_AVL_MASK	0x00000E00
#define MODE_G_MASK		0x00000100
#define MODE_PAT_MASK	0x00000080
#define MODE_D_MASK		0x00000040
#define MODE_A_MASK		0x00000020
#define MODE_PCD_MASK	0x00000010
#define MODE_PWT_MASK	0x00000008
#define MODE_US_MASK	0x00000004
#define MODE_RW_MASK	0x00000002
#define MODE_P_MASK		0x00000001

#define MODE_CLAIM		0x00000007
#define MODE_MAP_IN		0x00000007

#define PAGE_SIZE	(4096)
#define PAGE_MASK	(PAGE_SIZE - 1)
#define LPAGE_SIZE	(512*4096)
#define LPAGE_MASK	(LPAGE_SIZE - 1)

#define PTE_NX_MASK			0x8000000000000000UL
#define PTE_AVL2_MASK		0x7FF0000000000000UL
#define PTE_PHYS_MASK		0x000FFFFFFFFFF000UL
#define PTE_AVL_MASK		0x0000000000000E00UL
#define PTE_G_MASK			0x0000000000000100UL
#define PTE_PAT_MASK		0x0000000000000080UL
#define PTE_D_MASK			0x0000000000000040UL
#define PTE_A_MASK			0x0000000000000020UL
#define PTE_PCD_MASK		0x0000000000000010UL
#define PTE_PWT_MASK		0x0000000000000008UL
#define PTE_US_MASK			0x0000000000000004UL
#define PTE_RW_MASK			0x0000000000000002UL
#define PTE_P_MASK			0x0000000000000001UL
#define PTE_TYPE_MASK		0x0000000000000003UL

#define PDE_PHYS_MASK		0x000FFFFFFFE00000UL
#define PDE_PAT_MASK		0x0000000000001000UL
#define PDE_PS_MASK			0x0000000000000080UL

#define MODE_TO_PTE(x)		(((uLong)(x & 0xFFF00000) << 32) | (x & 0xFFF))
#define PTE_TO_MODE(x)		(uInt)(((x >> 32) & 0xFFF00000) | (x & 0xFFF))
#define MODE_TO_PDE(x)		(((uLong)(x & 0xFFF00000) << 32) | (x & 0xF7F) | ((x & MODE_PAT_MASK) << 5))
#define PDE_TO_MODE(x)		(uInt)(((x >> 32) & 0xFFF00000) | (x & 0xF7F) | ((x >> 5) & MODE_PAT_MASK))

#define VA_OFFSET			0
#define OF_PA_TO_VA(pa)		((uPtr)((uPtr)(pa) + VA_OFFSET))
#define OF_VA_TO_PA(va)		((uPtr)((uPtr)(va) - VA_OFFSET))

static uPtr
page_table_base()
{
	uLong mapaddr;

	asm(" mov	%%cr3,%0" : "=a"(mapaddr));
	mapaddr &= PTE_PHYS_MASK;
	return OF_PA_TO_VA(mapaddr);
}

static void
set_page_table_base(uLong *table)
{
	uLong mapaddr = OF_VA_TO_PA(table);
	asm(" mov	%0,%%cr3" : : "a"(mapaddr));
}

struct mapping
{
	uPtr virt;
	uPtr phys;
	uLong size;
	uInt mode;
};

const struct mapping defaultmappings[] =
{
	{ 0x000000000, 0x000000000, 0x020000000, MODE_CLAIM },
	{ 0x020000000, 0x020000000, 0x0E0000000, MODE_MAP_IN },
	{ 0, 0, 0 },		/* end of list */
};

uLong *
copypagetables(uLong *table, int level)
{
    uLong *ntable = (uLong *)memalign(4096, 4096);
    int i;

    if (ntable == NULL)
    {
		dprintf("Fatal error allocating initial page tables\n");
spin:	goto spin; 
    }

    DPRINTF(("new level %d page table %p from %p\n", level, ntable, table));
//    memset(ntable, 0, 4096);

    for (i = 0; i < 512; i++)
	{
		uLong pte = table[i];

		if ((pte & PTE_P_MASK) &&
			(level > 2 || (level == 2 && ((pte & PDE_PS_MASK) == 0))))
		{
			uLong *base = (uLong *)OF_PA_TO_VA(pte & PTE_PHYS_MASK);
			base = copypagetables(base, level - 1);

			if (base == NULL)
			{
				DPRINTF(("Fatal error allocating initial page tables\n"));
spin2:			goto spin2; 
			}

			DPRINTF(("index %d, old PTE %p, ", i, pte));
			pte &= ~PTE_PHYS_MASK;
			pte |= (uLong)OF_VA_TO_PA(base);
			DPRINTF(("new PTE %p\n", pte));
		}

		ntable[i] = pte;
	}

    return ntable;
}

void
dumppagetables(uLong *pml4base)
{
	int i;
	int j;
	int k;
	int l;
	uLong *pdpbase;
	uLong *pdbase;
	uLong *ptbase;
	uLong pml4e;
	uLong pdpe;
	uLong pde;
	uLong pte;

	/* show the L4 table */
	for (i = 0; i < 512; i++)
	{
		pml4e = pml4base[i];

		if (pml4e & PTE_P_MASK)
			DPRINTF(("L4[%d] = %#lx\n", i, pml4e));
	}

	/* show the L3 tables */
	for (i = 0; i < 512; i++)
	{
		pml4e = pml4base[i];

		if ((pml4e & PTE_P_MASK) == 0)
			continue;

		pdpbase = (uLong *)OF_PA_TO_VA(pml4e & PTE_PHYS_MASK);

		for (j = 0; j < 512; j++)
		{
			pdpe = pdpbase[j];

			if (pdpe & PTE_P_MASK)
				DPRINTF(("L3[%d][%d] = %#lx\n", i, j, pdpe));
		}
	}

	/* show the L2 tables */
	for (i = 0; i < 512; i++)
	{
		pml4e = pml4base[i];

		if ((pml4e & PTE_P_MASK) == 0)
			continue;

		pdpbase = (uLong *)OF_PA_TO_VA(pml4e & PTE_PHYS_MASK);

		for (j = 0; j < 512; j++)
		{
			pdpe = pdpbase[j];

			if ((pdpe & PTE_P_MASK) == 0)
				continue;

			pdbase = (uLong *)OF_PA_TO_VA(pdpe & PTE_PHYS_MASK);

//			for (k = 0; k < 512; k++)
			for (k = 0; k < 2; k++)
			{
				pde = pdbase[k];

				if (pde & PTE_P_MASK)
				{
					if (pde & PDE_PS_MASK)
						DPRINTF(("L2[%d][%d][%d] = %#lx (%#lx -> %#lx)\n", i, j, k, pde, ((uLong)i << 39) | ((uLong)j << 30) | (k << 21), pde & PDE_PHYS_MASK));
					else
						DPRINTF(("L2[%d][%d][%d] = %#lx\n", i, j, k, pde));
				}
			}
		}
	}

#if 0
	/* show the L1 tables */
	for (i = 0; i < 512; i++)
	{
		pml4e = pml4base[i];

		if ((pml4e & PTE_P_MASK) == 0)
			continue;

		pdpbase = (uLong *)OF_PA_TO_VA(pml4e & PTE_PHYS_MASK);

		for (j = 0; j < 512; j++)
		{
			pdpe = pdpbase[j];

			if ((pdpe & PTE_P_MASK) == 0)
				continue;

			pdbase = (uLong *)OF_PA_TO_VA(pdpe & PTE_PHYS_MASK);

			for (k = 0; k < 512; k++)
			{
				pde = pdbase[k];

				if ((pde & PTE_P_MASK) == 0 || (pde & PDE_PS_MASK) != 0)
					continue;

				ptbase = (uLong *)OF_PA_TO_VA(pde & PTE_PHYS_MASK);

				for (l = 0; l < 512; l++)
				{
					pte = ptbase[l];

					if (pte & PTE_P_MASK)
						DPRINTF(("L1[%d][%d][%d][%d] = %#lx (%#lx -> %#lx)\n", i, j, k, l, pte, ((uLong)i << 39) | ((uLong)j << 30) | (k << 21) | (l << 12), pte & PTE_PHYS_MASK));
				}
			}
		}
	}
#endif
}

void
init_virtual()
{
	uLong *l4 = (uLong *)OF_PA_TO_VA(page_table_base());
//	dprintf("Initial page tables\n");
//	dumppagetables(l4);
	l4 = copypagetables(l4, 4);
//	dprintf("Copied page tables\n");
//	dumppagetables(l4);

	flush_caches();
	flush_tlb();
	set_page_table_base(l4);
	flush_caches();
	flush_tlb();
}

static uLong
cvt_lpage_to_pages(uLong lpte)
{
	uLong *ptebase;
	uLong pte;
	int i;

	/* allocate page table */
	ptebase = (uLong *)memalign(4096, 4096);		/* allocate page table */

	if (ptebase == (uLong *)0)
		return 0UL;

	pte = lpte & 0xFFFFFFFFFFE00F7F;
	pte |= (lpte & PDE_PAT_MASK) >> 5;

	/* creating initial mappings */
	for (i = 0; i < 512; i++)
		ptebase[i] = pte | (i << 12);

	return OF_VA_TO_PA(ptebase) | (lpte & (PTE_US_MASK|PTE_RW_MASK)) |
			PTE_P_MASK;
}

static void
unmap_page(uPtr page, int size)
{
	uLong *pml4base = (uLong *)page_table_base();
	uLong *pdpebase;
	uLong *pdebase;
	uLong *ptebase;
	int pml4index = (page >> 39) & 0x1FF;
	int pdpeindex = (page >> 30) & 0x1FF;
	int pdeindex = (page >> 21) & 0x1FF;
	int pteindex = (page >> 12) & 0x1FF;
	uLong pte = pml4base[pml4index];
	int i;

	if ((pte & PTE_P_MASK) == 0)
		return;

	pdpebase = (uLong *)OF_PA_TO_VA(pte & PTE_PHYS_MASK);
	pte = pdpebase[pdpeindex];

	if ((pte & PTE_P_MASK) == 0)
		return;

	pdebase = (uLong *)OF_PA_TO_VA(pte & PTE_PHYS_MASK);
	pte = pdebase[pdeindex];

	if ((pte & PTE_P_MASK) == 0)
		return;

	if ((pte & PDE_PS_MASK) == 0)
	{
		ptebase = (uLong *)OF_PA_TO_VA(pte & PTE_PHYS_MASK);

		if (size == PAGE_SIZE)
		{
			ptebase[pteindex] = 0UL;

			for (i = 0; i < 512; i++)
				if (ptebase[i] & PTE_P_MASK)
					break;

			if (i < 512)
				return;
		}

		free(ptebase);
	}
	else if (size == PAGE_SIZE)
	{
		/* split the large page into small pages */
		pte = cvt_lpage_to_pages(pte);
		pdebase[pdeindex] = pte;
		ptebase = (uLong *)OF_PA_TO_VA(pte & PTE_PHYS_MASK);
		ptebase[pteindex] = 0UL;
		return;
	}

	/* release the large page */
	pdebase[pdeindex] = 0UL;

	for (i = 0; i < 512; i++)
		if (pdebase[i] & PTE_P_MASK)
			break;

	if (i < 512)
		return;

	free(pdebase);
	pdpebase[pdpeindex] = 0UL;

	for (i = 0; i < 512; i++)
		if (pdpebase[i] & PTE_P_MASK)
			break;

	if (i < 512)
		return;

	free(pdpebase);
	pml4base[pml4index] = 0UL;
}

static void
unmap_region(uPtr virt, uInt size)
{
	uInt psize;
	virt &= ~(uLong)PAGE_MASK;
	DPRINTF(("unmap_region: virt %#lx, size %#x\n", virt, size));

	while (size >= PAGE_SIZE)
	{
		if ((virt & LPAGE_MASK) || size < LPAGE_SIZE)
			psize = PAGE_SIZE;
		else
			psize = LPAGE_SIZE;

		unmap_page(virt, psize);
		virt += psize;
		size -= psize;
	}
}

static void
map_page(uPtr page, uLong phys, uInt size, uInt mode)
{
	uLong *pml4base = (uLong *)page_table_base();
	uLong *pdpebase;
	uLong *pdebase;
	uLong *ptebase;
	int pml4index = (page >> 39) & 0x1FF;
	int pdpeindex = (page >> 30) & 0x1FF;
	int pdeindex = (page >> 21) & 0x1FF;
	int pteindex = (page >> 12) & 0x1FF;
	uLong pte;
	int i;

	pte = pml4base[pml4index];

	if ((pte & PTE_P_MASK) == 0)
	{
		/* build the L4 page table */
		pdpebase = (uLong *)memalign(4096, 4096);		/* allocate page table */

		for (i = 0; i < 512; i++)
			pdpebase[i] = 0UL;

		pte = (uLong)OF_VA_TO_PA(pdpebase) | 
				(PTE_US_MASK|PTE_RW_MASK|PTE_P_MASK);
		pml4base[pml4index] = pte;
	}

	pdpebase = (uLong *)OF_PA_TO_VA(pte & PTE_PHYS_MASK);
	pte = pdpebase[pdpeindex];

	if ((pte & PTE_P_MASK) == 0)
	{
		/* build the L3 page table */
		pdebase = (uLong *)memalign(4096, 4096);		/* allocate page table */

		for (i = 0; i < 512; i++)
			pdebase[i] = 0UL;

		pte = (uLong)OF_VA_TO_PA(pdebase) | 
				(PTE_US_MASK|PTE_RW_MASK|PTE_P_MASK);
		pdpebase[pdpeindex] = pte;
	}

	pdebase = (uLong *)OF_PA_TO_VA(pte & PTE_PHYS_MASK);
	pte = pdebase[pdeindex];

	if (size == LPAGE_SIZE)
	{
		if ((pte & (PDE_PS_MASK|PTE_P_MASK)) == PTE_P_MASK)
		{
			ptebase = (uLong *)OF_PA_TO_VA(pte & PTE_PHYS_MASK);
			free(ptebase);
		}

		pdebase[pdeindex] = phys | MODE_TO_PDE(mode) | PDE_PS_MASK|PTE_P_MASK;
		return;
	}

	if ((pte & (PDE_PS_MASK|PTE_P_MASK)) != PTE_P_MASK)
	{
		/* build the L2 page table */
		ptebase = (uLong *)memalign(4096, 4096);		/* allocate page table */

		for (i = 0; i < 512; i++)
			ptebase[i] = 0UL;

		pte = (uLong)OF_VA_TO_PA(ptebase) | 
				(PTE_US_MASK|PTE_RW_MASK|PTE_P_MASK);
		pdebase[pdeindex] = pte;
	}

	ptebase = (uLong *)OF_PA_TO_VA(pte & PTE_PHYS_MASK);
	ptebase[pteindex] = phys | MODE_TO_PTE(mode) | PTE_P_MASK;
}

static void
map_region(uPtr virt, uPtr phys, uInt size, uInt mode)
{
	uInt psize;
	virt &= ~(uLong)PAGE_MASK;
	phys &= ~(uLong)PAGE_MASK;
	DPRINTF(("map_region: virt %#lx, phys %#lx, size %#x, mode %#x\n", virt, phys, size, mode));

	while (size >= PAGE_SIZE)
	{
		if ((virt & LPAGE_MASK) || (phys & LPAGE_MASK) || size < LPAGE_SIZE)
			psize = PAGE_SIZE;
		else
			psize = LPAGE_SIZE;

		map_page(virt, phys, psize, mode);
		virt += psize;
		phys += psize;
		size -= psize;
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

	DPRINTF(("mmu_map: e %p, phys %#x, virt %#Cx, size %#Cx, mode %#x\n", e, phys[0], virt, size, mode));

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

	DPRINTF(("mmu: unmap: virt %#Cx, size %#Cx\n", virt, size));

	s += v & PAGE_MASK;
	v &= ~PAGE_MASK;
	s = (s + PAGE_SIZE - 1) & ~PAGE_MASK;
	DPRINTF(("mmu: unmap: new virt %#lx, size %#x\n", v, s));

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
	uLong *pml4base = (uLong *)page_table_base();
	uLong *pdpebase;
	uLong *pdebase;
	uLong *ptebase;

	int pml4index = (virt >> 39) & 0x1FF;
	int pdpeindex = (virt >> 30) & 0x1FF;
	int pdeindex = (virt >> 21) & 0x1FF;
	int pteindex = (virt >> 12) & 0x1FF;
	uLong pte = pml4base[pml4index];

	DPRINTF(("mmu_translate: virt %#Cx, L4 index %#x\n", virt, pml4index));
	DPRINTF(("mmu_translate: pml4base %p, pml4e %#lx\n", pml4base, pte));

	phys[0] = 0;
	phys[1] = 0;
	*mode = 0;

	if ((pte & PTE_P_MASK) == 0)
		return E_INVALID_MEMORY_ADDR;

	pdpebase = (uLong *)OF_PA_TO_VA(pte & PTE_PHYS_MASK);
	pte = pdpebase[pdpeindex];

	DPRINTF(("mmu_translate: virt %#Cx, L3 index %#x\n", virt, pdpeindex));
	DPRINTF(("mmu_translate: pdpebase %p, pdpe %#lx\n", pdpebase, pte));

	if ((pte & PTE_P_MASK) == 0)
		return E_INVALID_MEMORY_ADDR;

	pdebase = (uLong *)OF_PA_TO_VA(pte & PTE_PHYS_MASK);
	pte = pdebase[pdeindex];

	DPRINTF(("mmu_translate: virt %#Cx, L2 index %#x\n", virt, pdeindex));
	DPRINTF(("mmu_translate: pdpebase %p, pdpe %#lx\n", pdebase, pte));

	if ((pte & PTE_P_MASK) == 0)
		return E_INVALID_MEMORY_ADDR;

	if ((pte & PDE_PS_MASK) != 0)
	{
		/* large page */
		*mode = PDE_TO_MODE(pte);
		phys[0] = (pte & PDE_PHYS_MASK) | (virt & LPAGE_MASK);
		phys[1] = (pte & PDE_PHYS_MASK) >> 32;
		DPRINTF(("mmu_translate: result phys %#x,%#x, mode %#x\n", phys[1], phys[0], *mode));
		return NO_ERROR;
	}

	ptebase = (uLong *)OF_PA_TO_VA(pte & PTE_PHYS_MASK);
	pte = ptebase[pteindex];

	if ((pte & PTE_P_MASK) == 0)
		return E_INVALID_MEMORY_ADDR;

	/* small page */
	phys[0] = (pte & PTE_PHYS_MASK) | (virt & PAGE_MASK);
	phys[1] = (pte & PTE_PHYS_MASK) >> 32;
	*mode = PTE_TO_MODE(pte);
	DPRINTF(("mmu_translate: result phys %#x,%#x, mode %#x\n", phys[1], phys[0], *mode));
	return NO_ERROR;
}

Retcode
mmu_dma_alloc(Environ *e, Cell size, Cell *virt)
{
	void *mem = malloc((size_t)size);
	/* XXX: need to page align and enusre that it is in the lower 4GB */

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
	phys[0] = (uLong)OF_VA_TO_PA(virt);
	phys[1] = (uLong)OF_VA_TO_PA(virt) >> 32;
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
read_mapping(uLong *virt, uLong *phys, uLong *size, uInt *mode)
{
	uLong v = *virt;
	uLong p;
	uLong sz;
	uInt m;
	uLong *pml4base = (uLong *)page_table_base();
	uLong *pdpebase;
	uLong *pdebase;
	uLong *ptebase;
	int pml4index = (v >> 39) & 0x1FF;
	int pdpeindex = (v >> 30) & 0x1FF;
	int pdeindex = (v >> 21) & 0x1FF;
	int pteindex = (v >> 12) & 0x1FF;
	uLong pml4e = pml4base[pml4index];
	uLong pdpe;
	uLong pde;
	uLong pte;
	int valid = 0;
	static int done = 0;

	if (done)
	{
		done = 0;
		DPRINTF(("read_mapping: RET: no mapping\n"));
		return 0;
	}

	DPRINTF(("read_mapping: virt %#lx, L4 index %#x\n", *virt, pml4index));
	DPRINTF(("read_mapping: L4 base %p, L4 PTE %#lx\n", pml4base, pml4e));

	if (pml4e & PTE_P_MASK)
	{
		pdpebase = (uLong *)OF_PA_TO_VA(pml4e & PTE_PHYS_MASK);
		pdpe = pdpebase[pdpeindex];
		DPRINTF(("read_mapping: L3 base %p, L3 PTE %#lx\n", pdpebase, pdpe));

		if (pdpe & PTE_P_MASK)
		{
			pdebase = (uLong *)OF_PA_TO_VA(pdpe & PTE_PHYS_MASK);
			pde = pdebase[pdeindex];
			DPRINTF(("read_mapping: L2 base %p, L2 PTE %#lx\n", pdebase, pde));

			if (pde & PTE_P_MASK)
			{
				if (pde & PDE_PS_MASK)
					valid = 1;
				else
				{
					ptebase = (uLong *)OF_PA_TO_VA(pde & PTE_PHYS_MASK);
					pte = ptebase[pteindex];
					DPRINTF(("read_mapping: PTE base %p, PTE %#lx\n", ptebase, pte));

					if (pde & PTE_P_MASK)
						valid = 1;
				}
			}
		}
	}

	DPRINTF(("read_mapping: valid %d\n", valid));

	/* find the first non fault mapping starting at virt */
	while (!valid)
	{
		DPRINTF(("read_mapping: L4 base %p, L4 PTE %#lx, L4 index %d\n", pml4base, pml4e, pml4index));

		while ((pml4e & PTE_P_MASK) == 0)
		{
			pml4index++;
			pdpeindex = 0;
			pdeindex = 0;
			pteindex = 0;

			if (pml4index >= 512)
				break;

			pml4e = pml4base[pml4index];
//			DPRINTF(("read_mapping: L4 not valid, new L4 PTE %#lx\n", pml4e));
		}

		if (pml4e & PTE_P_MASK)
		{
			pdpebase = (uLong *)OF_PA_TO_VA(pml4e & PTE_PHYS_MASK);
			pdpe = pdpebase[pdpeindex];
			DPRINTF(("read_mapping: L3 base %p, L3 PTE %#lx, L3 index %d\n", pdpebase, pdpe, pdpeindex));

			while ((pdpe & PTE_P_MASK) == 0)
			{
				pdpeindex++;
				pdeindex = 0;
				pteindex = 0;

				if (pdpeindex >= 512)
					break;

				pdpe = pdpebase[pdpeindex];
//				DPRINTF(("read_mapping: L3 not valid, new L3 PTE %#lx\n", pdpe));
			}

			DPRINTF(("final pdpe %#lx, pdpe index %d\n", pdpe, pdpeindex));

			if (pdpe & PTE_P_MASK)
			{
				pdebase = (uLong *)OF_PA_TO_VA(pdpe & PTE_PHYS_MASK);
				pde = pdebase[pdeindex];
				DPRINTF(("read_mapping: L2 base %p, L2 PTE %#lx, L2 index %d\n", pdebase, pde, pdeindex));

				while ((pde & PTE_P_MASK) == 0)
				{
					pdeindex++;
					pteindex = 0;

					if (pdeindex >= 512)
						break;

					pde = pdebase[pdeindex];
					DPRINTF(("read_mapping: L2 not valid, new L2 PTE %#lx, index %d\n", pde, pdeindex));
				}

				DPRINTF(("final pde %#lx, pde index %d\n", pde, pdeindex));

				if (pde & PTE_P_MASK)
				{
					if (pde & PDE_PS_MASK)
						valid = 1;
					else
					{
						ptebase = (uLong *)OF_PA_TO_VA(pde & PTE_PHYS_MASK);
						pte = ptebase[pteindex];
						DPRINTF(("read_mapping: PTE base %p, PTE %#lx\n", ptebase, pte));

						while ((pte & PTE_P_MASK) == 0)
						{
							pteindex++;
							pte = ptebase[pteindex];

							if (pteindex >= 512)
								break;

//							DPRINTF(("read_mapping: L1 not valid, new L1 PTE %#lx\n", pte));
						}

						if (pte & PTE_P_MASK)
							valid = 1;
					}
				}
			}
		}

		DPRINTF(("about to propogate carries (%d,%d,%d,%d)\n", pml4index, pdpeindex, pdeindex, pteindex));

		if (pteindex >= 512)
		{
			pteindex = 0;
			pdeindex++;
		}

		if (pdeindex >= 512)
		{
			pdeindex = 0;
			pdpeindex++;
		}

		if (pdpeindex >= 512)
		{
			pdpeindex = 0;
			pml4index++;
			pml4e = pml4base[pml4index];
		}

		if (pml4index >= 512)
		{
			/* we're done searching */
			DPRINTF(("read_mapping: RET: no mapping (search)\n"));
			return 0;
		}

		DPRINTF(("after carry propogate (%d,%d,%d,%d)\n", pml4index, pdpeindex, pdeindex, pteindex));
	}

	DPRINTF(("read_mapping: (%d,%d,%d,%d)\n", pml4index, pdpeindex, pdeindex, pteindex));

	v = (uLong)(((Long)pml4index << 55) >> 16) | ((uLong)pdpeindex << 30) |
			((uLong)pdeindex << 21) | ((uLong)pteindex << 12);

	DPRINTF(("read_mapping: va %#lx\n", v));

	if (pde & PDE_PS_MASK)
	{
		m = PDE_TO_MODE(pde) & ~(MODE_D_MASK|MODE_A_MASK);
		p = pde & PTE_PHYS_MASK;
		sz = LPAGE_SIZE;
	}
	else
	{
		m = PTE_TO_MODE(pte) & ~(MODE_D_MASK|MODE_A_MASK);
		p = pte & PTE_PHYS_MASK;
		sz = PAGE_SIZE;
	}

	/* find the next non-contiguous mapping starting at virt */
	while (valid)
	{
		pteindex++;

		if (pde & PDE_PS_MASK || pteindex >= 512)
		{
			pdeindex++;
			pteindex = 0;

			if (pdeindex >= 512)
			{
				pdpeindex++;
				pdeindex = 0;

				if (pdpeindex >= 512)
				{
					pml4index++;
					pdpeindex = 0;

					if (pml4index >= 512)
					{
						/* we're done searching and we found a region */
						break;
					}
				}
			}
		}

		pml4e = pml4base[pml4index];

		if (pml4e & PTE_P_MASK)
		{
			pdpebase = (uLong *)OF_PA_TO_VA(pml4e & PTE_PHYS_MASK);
			pdpe = pdpebase[pdpeindex];

			if (pdpe & PTE_P_MASK)
			{
				pdebase = (uLong *)OF_PA_TO_VA(pdpe & PTE_PHYS_MASK);
				pde = pdebase[pdeindex];

				if (pde & PTE_P_MASK)
				{
					if (pde & PDE_PS_MASK)
					{
						if (p + sz != (pde & PDE_PHYS_MASK) || m != (PDE_TO_MODE(pde) & ~(MODE_D_MASK|MODE_A_MASK)))
							break;

						sz += LPAGE_SIZE;
					}
					else
					{
						ptebase = (uLong *)OF_PA_TO_VA(pde & PTE_PHYS_MASK);
						pte = ptebase[pteindex];

						if (pte & PTE_P_MASK)
						{
							if (p + sz != (pte & PTE_PHYS_MASK) || m != (PTE_TO_MODE(pde) & ~(MODE_D_MASK|MODE_A_MASK)))
								break;

							sz += PAGE_SIZE;
						}
						else
							valid = 0;
					}
				}
				else
					valid = 0;
			}
			else
				valid = 0;
		}
		else
			valid = 0;
	}

	*virt = v;
	*phys = p;
	*size = sz;
	*mode = m;
	DPRINTF(("read_mapping: va %#lx pa %#lx sz %#lx mode %#x \n", v, p, sz, m));

	if (v + sz == 0)
		done = 1;

	DPRINTF(("read_mapping: RET: %smapping found\n", done ? "last " : ""));
	return 1;
}

extern Retcode add_translation(Environ *e, uLong phys, Cell virt, Cell size, Int mode);
extern Retcode make_translations(Environ *e);

Retcode
mmu_init_translations(Environ *e)
{
	uLong virt;
	uLong phys;
	uLong size;
	uInt mode;
//	Int inst;
	Retcode ret;

//	dprintf("Page tables before mmu_init_translations\n");
//	dumppagetables((uLong *)page_table_base());

	for (virt = 0; read_mapping(&virt, &phys, &size, &mode); virt += size)
	{
		ret = add_translation(e, phys, virt, size, mode);

		if (ret != NO_ERROR)
			return ret;
	}

	return make_translations(e);
}

static void
addmode(char *buf, char *name, uInt val)
{
    if (buf[0] != '\0')
    {
	buf += strlen(buf);
	*buf++ = ' ';
    }

    bprintf(buf, "%s:%x", name, val);
}

static char *
modestr(uInt mode)
{
    static char buf[128];
    mode &= MODE_MASK;
    buf[0] = '\0';

    addmode(buf, "NX", mode >> 31);
    addmode(buf, "AVL2", (mode >> 20) & 0x7FF);
    addmode(buf, "AVL", (mode >> 9) & 0x7);
    addmode(buf, "G", (mode >> 8) & 0x1);
    addmode(buf, "PAT", (mode >> 7) & 0x1);
    addmode(buf, "D", (mode >> 6) & 0x1);
    addmode(buf, "A", (mode >> 5) & 0x1);
    addmode(buf, "PCD", (mode >> 4) & 0x1);
    addmode(buf, "PWT", (mode >> 3) & 0x1);
    addmode(buf, "U/S", (mode >> 2) & 0x1);
    addmode(buf, "R/W", (mode >> 1) & 0x1);
    addmode(buf, "P", mode & 0x1);
    return buf;
}

#define MMU_MAP_IN_DATA			0x0001
#define MMU_MAP_IN_EXEC			0x0002
#define MMU_MAP_IN_IO			0x0004

#define MMU_DMA_MAP_IN_NOCACHE		0x0001
#define MMU_DMA_MAP_IN_CACHEABLE	0x0002

C(f_mapq)
{
	uLong *pml4base;
	uLong *pdpebase;
	uLong *pdebase;
	uLong *ptebase;
	uLong addr;
	uLong pte;

	IFCKSP(e, 1, 1);
	POP(e, addr);

	pml4base = (uLong *)page_table_base();
	pte = pml4base[(addr >> 39) & 0x1FF];

	if ((pte & PTE_P_MASK) == 0)
	{
		cprintf(e, "%#lx: fault %#lx pml4\n", addr, pte);
		return NO_ERROR;
	}

	pdpebase = (uLong *)OF_PA_TO_VA(pte & PTE_PHYS_MASK);
	pte = pdpebase[(addr >> 30) & 0x1FF];

	if ((pte & PTE_P_MASK) == 0)
	{
		cprintf(e, "%#lx: fault %#lx pdp\n", addr, pte);
		return NO_ERROR;
	}

	pdebase = (uLong *)OF_PA_TO_VA(pte & PTE_PHYS_MASK);
	pte = pdebase[(addr >> 21) & 0x1FF];

	if ((pte & PTE_P_MASK) == 0)
	{
		cprintf(e, "%#lx: fault %#lx pd\n", addr, pte);
		return NO_ERROR;
	}

	if (pte & PDE_PS_MASK)
	{
		cprintf(e, "%#lx: %#lx: 2MB page %#lx: mode:%#x %s\n", addr, pte,
				pte & PDE_PHYS_MASK, PDE_TO_MODE(pte),
				modestr(PDE_TO_MODE(pte)));
		return NO_ERROR;
	}

	ptebase = (uLong *)OF_PA_TO_VA(pte & PTE_PHYS_MASK);
	pte = ptebase[(addr >> 12) & 0x1FF];

	if ((pte & PTE_P_MASK) == 0)
	{
		cprintf(e, "%#lx: fault %#lx pt\n", addr, pte);
		return NO_ERROR;
	}

	cprintf(e, "%#lx: %#lx: page %#lx: mode:%#x %s\n", addr, pte,
			pte & PTE_PHYS_MASK, PTE_TO_MODE(pte),
			modestr(PTE_TO_MODE(pte)));
	return NO_ERROR;
}

const Initentry init_mmu[] =
{
	{ "map?", f_mapq, INVALID_FCODE, F_NONE, T_FUNC },
	{ NULL, NULL }
};
