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

/*#define DEBUG*/

#include "defs.h"

#ifdef MAIN
#include <stdio.h>
#define dprintf printf
#endif

/* General purpose allocator for use by /memory /mmu and /pci nodes codes */

/* The block structure keeps track of the base address and size of each */
/* free block. There is a special case that has to be handled.  Since the */
/* list of free block is always composed of block that 1 or more bytes, the */
/* size value of zero is used to represent 2^n sized block and not a zero */
/* sized block.  This case has to be handled specially in the code. */
/* Since the address fields may be large than the size field it is possible */
/* to have multiple "full sized" blocks in the list of free blocks. */

#define SIZE(a, b) (&(b)->addr[(a)->acells])

#if 0
#define PATH(x) printf("path: %s\n", (x))
#else
#define PATH(x)
#endif

Retcode
alloc_init(Allocator **aptr, int acells, int scells)
{
	Allocator *a;
	int maxcells = (acells > scells) ? acells : scells;
	int sz = sizeof (Allocator) + (sizeof (uInt) * (5 * maxcells - 1));
	PATH("init 1");

	if (scells > acells)
	{
		PATH("init 2");
		return E_BAD_ARGUMENT;
	}

	PATH("init 3");
	a = (Allocator *)malloc(sz);
	*aptr = a;

	if (a == NULL)
	{
		PATH("init 4");
		return E_OUT_OF_MEMORY;
	}

	PATH("init 5");
	a->acells = acells;
	a->scells = scells;
	a->bsize = sizeof (Allocator_block) + (sizeof (uInt) * (acells + scells - 1));
	a->list = NULL;
	a->t1 = a->x;			/* temps for intermediate calculations */
	a->t2 = &a->t1[maxcells];
	a->u1 = &a->t2[maxcells];	/* temps for users calculations */
	a->u2 = &a->u1[maxcells];
	a->u3 = &a->u2[maxcells];
	return NO_ERROR;
}

Retcode
alloc_fini(Allocator *a)
{
	Allocator_block *b = a->list;
	Allocator_block *n;

	PATH("fini 1");

	for (; b; b = n)
	{
		PATH("fini 2");
		n = b->next;
		free(b);
	}

	PATH("fini 3");
	free(a);
	return NO_ERROR;
}

static int
zero(uInt a[], int cells)
{
	int i;

	PATH("zero 1");

	for (i = 0; i < cells; i++)
	{
		PATH("zero 2");

		if (a[i])
		{
			PATH("zero 3");
			return 0;
		}

		PATH("zero 4");
	}

	PATH("zero 5");
	return 1;
}

static int
cmp(uInt a[], uInt b[], int cells)
{
	int i;
	PATH("cmp 1");

	for (i = cells - 1; i >= 0; i--)
	{
		PATH("cmp 2");

		if (a[i] < b[i])
		{
			PATH("cmp 3");
			return -1;
		}

		if (a[i] > b[i])
		{
			PATH("cmp 4");
			return 1;
		}

		PATH("cmp 5");
	}

	PATH("cmp 6");
	return 0;
}

static int
and(uInt a[], uInt b[], uInt c[], int cells)
{
	int i;
	uInt sum = 0;
	PATH("and 1");

	for (i = 0; i < cells; i++)
	{
		uInt t = a[i] & b[i];
		PATH("and 2");

		if (c)
		{
			PATH("and 3");
			c[i] = t;
		}

		PATH("and 4");
		sum |= t;
	}

	PATH("and 5");
	DPRINTF(("sum %#X\n", sum));
	return sum != 0;
}

static int
or(uInt a[], uInt b[], uInt c[], int cells)
{
	int i;
	uInt sum = 0;
	PATH("or 1");

	for (i = 0; i < cells; i++)
	{
		uInt t = a[i] | b[i];
		PATH("or 2");

		if (c)
		{
			PATH("or 3");
			c[i] = t;
		}

		PATH("or 4");
		sum |= t;
	}

	PATH("or 5");
	DPRINTF(("sum %#X\n", sum));
	return sum != 0;
}

static int
not(uInt a[], uInt b[], int cells)
{
	int i;
	uInt t;
	uInt sum = 0;
	PATH("not 1");

	for (i = 0; i < cells; i++)
	{
		PATH("not 2");
		t = ~a[i];
		b[i] = t;
		sum |= t;
	}

	PATH("and 3");
	return sum != 0;
}

static int
add(uInt a[], uInt b[], uInt s[], int ascells, int bcells)
{
	int i;
	int carry = 0;
	PATH("add 1");

	for (i = 0; i < ascells; i++)
	{
		uInt t1 = a[i];
		uInt t2 = (i < bcells) ? b[i] : 0;
		uInt t3 = t1 + t2;
		uInt t4 = t3 + carry;

		carry = (t3 < t1 || t4 < t3);
		PATH("add 2");
//		printf("carry %d %X %X %X %X\n", carry, t1, t2, t3, t4);
		s[i] = t4;
	}

	return carry;
}

static int
sub(uInt a[], uInt b[], uInt d[], int acells, int bcells)
{
	int i;
	int borrow = 0;
	PATH("sub 1");

	for (i = 0; i < acells; i++)
	{
		uInt t1 = a[i];
		uInt t2 = t1 - ((i < bcells) ? b[i] : 0);
		uInt t3 = t2 - borrow;
		PATH("sub 2");
		borrow = (t2 > t1 || t3 > t2);
		d[i] = t3;
	}

	PATH("sub 3");
	return borrow;
}

static int
neg(uInt a[], uInt b[], int cells)
{
	int i;
	int carry = 1;
	PATH("neg 1");

	for (i = 0; i < cells; i++)
	{
		uInt t = ~a[i];
		PATH("neg 2");
		b[i] = t + carry;
		carry = b[i] < t;
	}

	PATH("neg 3");
	return 0;
}

static int
addcmp(uInt a[], uInt b[], uInt c[], uInt d[], int accells, int bdcells)
{
	int i;
	int carryab = 0;
	int carrycd = 0;
	int x = 0;
	PATH("addcmp 1");

	for (i = 0; i < accells; i++)
	{
		uInt t1 = a ? a[i] : 0;
		uInt t2 = c ? c[i] : 0;
		uInt t3 = t1 + ((b && i < bdcells) ? b[i] : 0);
		uInt t4 = t2 + ((d && i < bdcells) ? d[i] : 0);
		uInt t5 = t3 + carryab;
		uInt t6 = t4 + carrycd;
		carryab = (t3 < t1 || t5 < t3);
		carrycd = (t4 < t2 || t6 < t4);
		PATH("addcmp 2");

		if (t5 < t6)
		{
			PATH("addcmp 3");
			x = -1;
		}
		else if (t5 > t6)
		{
			PATH("addcmp 4");
			x = 1;
		}

		PATH("addcmp 5");
	}

	PATH("addcmp 6");
	return x;
}

void
cellcpy(uInt d[], uInt s[], int cells)
{
	int i;
	PATH("cellcpy 1");

	for (i = 0; i < cells; i++)
	{
		PATH("cellcpy 2");
		d[i] = s[i];
	}
}

void
cellzero(uInt a[], int cells)
{
	int i;
	PATH("cellzero 1");

	for (i = 0; i < cells; i++)
	{
		PATH("cellzero 2");
		a[i] = 0;
	}
}

Retcode
alloc_fixed(Allocator *a, uInt addr[], uInt size[])
{
	Allocator_block *b = a->list;
	Allocator_block *p = NULL;
	int x = 1;
	int fsalloc;
	int fsblock;
	PATH("alloc_fixed 1");

	fsalloc = zero(size, a->scells);

	if (fsalloc && !zero(addr, a->scells))	/* Yes, only scells */
	{
		PATH("alloc_fixed 2");
		return E_BAD_ARGUMENT;
	}

	for (; b; p = b, b = b->next)
	{
		PATH("alloc_fixed 3");
		x = cmp(addr, b->addr, a->acells);

		if (x <= 0)
		{
			PATH("alloc_fixed 4");
			break;
		}

		PATH("alloc_fixed 5");
	}

	PATH("alloc_fixed 6");

	if (b && x == 0)
	{
		PATH("alloc_fixed 7");
		/* addr is at the start of block b */
		fsblock = zero(SIZE(a, b), a->scells);
		x = cmp(SIZE(a, b), size, a->scells);

		if (!fsblock && x < 0)
		{
			PATH("alloc_fixed 8");
			return E_OUT_OF_STORAGE;
		}

		PATH("alloc_fixed 9");

		if (x == 0)
		{
			PATH("alloc_fixed 10");

			if (p)
			{
				PATH("alloc_fixed 11");
				p->next = b->next;
			}
			else
			{
				PATH("alloc_fixed 12");
				a->list = b->next;
			}

			PATH("alloc_fixed 13");
			free(b);
			return NO_ERROR;
		}

		PATH("alloc_fixed 14");
		add(b->addr, size, b->addr, a->acells, a->scells);
		sub(SIZE(a, b), size, SIZE(a, b), a->scells, a->scells);
		return NO_ERROR;
	}

	if (p == NULL)
	{
		PATH("alloc_fixed 15");
		return E_OUT_OF_STORAGE;
	}

	PATH("alloc_fixed 16");
	/* block is in or after block p */
	fsblock = zero(SIZE(a, p), a->scells);
	x = addcmp(p->addr, SIZE(a, p), addr, size, a->acells, a->scells);

	if (!fsblock && x < 0)
	{
		PATH("alloc_fixed 17");
		return E_OUT_OF_STORAGE;		/* block ends after end of p */
	}

	PATH("alloc_fixed 18");

	if (x == 0)
	{
		PATH("alloc_fixed 19");
		/* block ends at end of p */
		sub(SIZE(a, p), size, SIZE(a, p), a->scells, a->scells);
		return NO_ERROR;
	}

	PATH("alloc_fixed 20");
	b = (Allocator_block *)malloc(a->bsize);

	if (b == NULL)
	{
		PATH("alloc_fixed 21");
		return E_OUT_OF_MEMORY;
	}

	PATH("alloc_fixed 22");
	add(addr, size, b->addr, a->acells, a->scells);
	add(p->addr, SIZE(a, p), a->t1, a->acells, a->scells);
	sub(a->t1, b->addr, a->t1, a->acells, a->acells);
	cellcpy(SIZE(a, b), a->t1, a->scells);
	sub(SIZE(a, p), size, SIZE(a, p), a->scells, a->scells);
	sub(SIZE(a, p), SIZE(a, b), SIZE(a, p), a->scells, a->scells);
	b->next = p->next;
	p->next = b;
	return NO_ERROR;
}

Retcode
alloc_constrained(Allocator *a, uInt min[], uInt max[], uInt align[], uInt mask[],
	uInt size[], uInt addr[])
{
	Allocator_block *b;
	int x;
	int y;
	int fsalloc = zero(size, a->scells);
	int fsblock;
	uInt *bsize = a->t1;
	uInt *off = a->t2;
	uInt one = 1;
	PATH("alloc_constrained 1");

	DPRINTF(("min %#x, max %#x, align %#x, mask %#x, size %#x, addr %#x\n",
			min ? min[0] : 0, max ? max[0] : -1, align ? align[0] : 0,
			mask ? mask[0] : -1, size ? size[0] : 0, addr[0]));


	/* convert power of two alignment into a mask */
	if (align)
	{
		PATH("alloc_constrained 2");
		sub(align, &one, align, a->acells, 1);
	}

	/* select a block */
	for (b = a->list; b; b = b->next)
	{
		PATH("alloc_constrained 3");
		DPRINTF(("alloc block: addr %#x size %#x\n", b->addr[0], SIZE(a, b)[0]));
		/* apply min constraint */
		cellcpy(addr, b->addr, a->acells);
		cellcpy(bsize, SIZE(a, b), a->scells);
		fsblock = zero(bsize, a->scells);

		if (min)
		{
			x = cmp(addr, min, a->acells);
			PATH("alloc_constrained 4");

			DPRINTF(("min check x %d\n", x));

			if (x < 0)
			{
				PATH("alloc_constrained 5");
				sub(min, addr, off, a->acells, a->acells);
				y = cmp(off, bsize, a->scells);
				DPRINTF(("size check y %d\n", y));

				if (!fsblock && y >= 0)
				{
					PATH("alloc_constrained 6");
					continue;
				}

				PATH("alloc_constrained 7");
				add(addr, off, addr, a->acells, a->scells);
				sub(bsize, off, bsize, a->scells, a->scells);
				fsblock = 0;
			}

			PATH("alloc_constrained 8");
		}

		PATH("alloc_constrained 9");
		DPRINTF(("passed min check\n"));

		DPRINTF(("addr %#x, align %#x, off %#x\n", addr[0], align[0], off[0]));

		/* apply alignment constraint */
		if (align && and(addr, align, off, a->acells))
		{
			PATH("alloc_constrained 10");
			DPRINTF(("after and: addr %#x, align %#x, off %#x\n", addr[0], align[0], off[0]));
			sub(align, off, off, a->acells, a->acells);
			DPRINTF(("after sub: align %#x, off %#x\n", align[0], off[0]));
			add(off, &one, off, a->acells, 1);
			DPRINTF(("after add: off %#x\n", off[0]));

			y = cmp(off, bsize, a->scells);
			DPRINTF(("size check y %d\n", y));

			if (!fsblock && y >= 0)
			{
				PATH("alloc_constrained 11");
				continue;
			}

			PATH("alloc_constrained 12");
			add(addr, off, addr, a->acells, a->scells);
			DPRINTF(("after add: addr %#x, off %#x\n", addr[0], off[0]));
			sub(bsize, off, bsize, a->scells, a->scells);
			DPRINTF(("after sub: bsize %#x, off %#x\n", bsize[0], off[0]));
			fsblock = 0;
		}

		PATH("alloc_constrained 13");
		DPRINTF(("passed align check\n"));

		/* check max constraint */
		if (max)
		{
			PATH("alloc_constrained 14");
			add(addr, size, off, a->acells, a->scells);
			y = cmp(off, max, a->acells);

			DPRINTF(("addr %#x, size %#x, off %#x max %#x passed max check\n", addr[0], size[0], off[0], max[0]));

			if (y > 0)
			{
				PATH("alloc_constrained 15");
				break;
			}

			PATH("alloc_constrained 16");
		}

		PATH("alloc_constrained 17");
		DPRINTF(("passed max check\n"));

		/* check mask constraint */
		if (mask && and(addr, mask, NULL, a->acells))
		{
			PATH("alloc_constrained 18");
			continue;
		}

		PATH("alloc_constrained 19");
		DPRINTF(("passed mask check\n"));

		/* check size constraint */
		if (fsalloc && !fsblock)
		{
			PATH("alloc_constrained 20");
			continue;
		}

		PATH("alloc_constrained 21");
		DPRINTF(("passed full size check\n"));

		y = cmp(bsize, size, a->scells);

		if (!fsblock && y < 0)
		{
			PATH("alloc_constrained 22");
			continue;
		}

		PATH("alloc_constrained 23");
		DPRINTF(("passed size check\n"));

		/* call alloc fixed to allocate it */
		return alloc_fixed(a, addr, size);
	}

	PATH("alloc_constrained 24");
	DPRINTF(("didn't find a block\n"));
	return E_OUT_OF_STORAGE;
}

Retcode
alloc_aligned(Allocator *a, uInt align[], uInt size[], uInt addr[])
{
	PATH("alloc_aligned 1");
	return alloc_constrained(a, NULL, NULL, align, NULL, size, addr);
}

Retcode
alloc_block(Allocator *a, uInt size[], uInt addr[])
{
	Allocator_block *b;
	int fsalloc = zero(size, a->scells);
	int fsblock;

	PATH("alloc_block 1");

	/* search for a block that meets the size constraint */
	for (b = a->list; b; b = b->next)
	{
		int x = cmp(SIZE(a, b), size, a->scells);
		PATH("alloc_block 2");

		fsblock = zero(SIZE(a, b), a->scells);

		if (fsalloc && !fsblock)
		{
			PATH("alloc_block 3");
			continue;
		}

		PATH("alloc_block 4");

		if (fsblock || x >= 0)
		{
			PATH("alloc_block 5");
			cellcpy(addr, b->addr, a->acells);
			return alloc_fixed(a, addr, size);
		}

		PATH("alloc_block 6");
	}

	PATH("alloc_block 7");
	return E_OUT_OF_STORAGE;
}

Retcode
free_block(Allocator *a, uInt addr[], uInt size[])
{
	Allocator_block *b = a->list;
	Allocator_block *p = NULL;
	int x = 1;
	int y = -1;
	PATH("free_block 1");

	for (; b; p = b, b = b->next)
	{
		PATH("free_block 2");
		x = cmp(addr, b->addr, a->acells);

		if (x <= 0)
		{
			PATH("free_block 2");
			break;
		}

		PATH("free_block 3");
	}

	PATH("free_block 4");

	if (p)
	{
		PATH("free_block 5");
		y = addcmp(p->addr, SIZE(a, p), addr, NULL, a->acells, a->scells);
	}
	else
	{
		PATH("free_block 6");
		y = -1;
	}

	if (b)
	{
		PATH("free_block 7");
		x = addcmp(addr, size, b->addr, NULL, a->acells, a->scells);
	}
	else
	{
		PATH("free_block 8");
		x = -1;
	}

	if (y > 0)
	{
		PATH("free_block 9");
		return E_BAD_ARGUMENT;
	}

	if (x > 0)
	{
		PATH("free_block 10");
		return E_BAD_ARGUMENT;
	}

	if (y == 0)
	{
		/* consolidate */
		PATH("free_block 11");
		add(SIZE(a, p), size, SIZE(a, p), a->scells, a->scells);

		if (x == 0)
		{
			PATH("free_block 12");
			add(SIZE(a, p), SIZE(a, b), SIZE(a, p), a->scells, a->scells);
			p->next = b->next;
			free(b);
		}

		PATH("free_block 13");
		return NO_ERROR;
	}

	if (x == 0)
	{
		PATH("free_block 14");
		cellcpy(b->addr, addr, a->acells);
		add(SIZE(a, b), size, SIZE(a, b), a->scells, a->scells);
		return NO_ERROR;
	}

	PATH("free_block 15");
	b = (Allocator_block *)malloc(a->bsize);

	if (b == NULL)
	{
		PATH("free_block 16");
		return E_OUT_OF_MEMORY;
	}

	if (p)
	{
		PATH("free_block 17");
		b->next = p->next;
		p->next = b;
	}
	else
	{
		PATH("free_block 18");
		b->next = a->list;
		a->list = b;
	}

	PATH("free_block 19");
	cellcpy(b->addr, addr, a->acells);
	cellcpy(SIZE(a, b), size, a->scells);
	return NO_ERROR;
}

Retcode
find_constrained(Allocator *a, uInt min[], uInt max[], uInt align[],
		uInt mask[], uInt addr[], uInt size[])
{
	uInt *baddr = a->t1;
	uInt *bsize = a->t1;
	uInt *off = a->t2;
	uInt *amask;
	uInt *nmask;
	uInt *limit;
	uInt *bits;
	uInt *nbits;
	Allocator_block *b = NULL;
	int acells = a->acells;
	int scells = a->scells;

	/* compute 'amask = align - 1' */
	if (align)
	{
		uInt one = 1;
		cellcpy(amask, align, acells);
		sub(amask, &one, amask, acells, 1);
	}
	else
		amask = NULL;

	/* compute 'nmask = ~mask' */
	if (mask)
		not(mask, nmask, a->acells);
	else
		nmask = NULL;

	for (b = a->list; b != NULL; b = b->next)
	{
		cellcpy(baddr, b->addr, acells);
		cellcpy(bsize, SIZE(a, b), scells);
		DPRINTF(("find_constrained: b = %#X, base %#X, size %#X\n",
				b, baddr[0], bsize[0]));

		/* round up */
		if (align && and(baddr, amask, off, acells))
		{
			/* calculate the offset to an aligned address */
			sub(align, off, off, acells, acells);

			/* check to see if the block is large enough for the block */
			/* to be aligned. */
			if (cmp(bsize, off, scells) < 0)
				continue;

			/* calculate the aligned address and revised block size */
			sub(bsize, off, bsize, scells, acells);
			add(baddr, off, baddr, acells, acells);

			DPRINTF(("find_constrained: base %#X, size %#X\n", baddr[0], bsize[0]));
		}

		/* check for min constraint */
		if (min && cmp(baddr, min, acells) < 0)
		{
			sub(min, baddr, off, acells, acells);
			DPRINTF(("find_constrained: doing min adjustment\n"));

			if (cmp(bsize, off, scells) < 0)
				continue;

			sub(bsize, off, bsize, scells, acells);
			cellcpy(baddr, min, acells);
			DPRINTF(("find_constrained: base %%#X, size %#X\n", baddr[0], bsize[0]));
		}

		/* check for mask constraint */
		if (mask && and(baddr, nmask, NULL, acells))
			continue;

		/* round down size */
		and(bsize, amask, bsize, acells);

		if (zero(bsize, acells))
			continue;

		DPRINTF(("find_constrained: size align: base %#X, size %#X\n", baddr[0], bsize[0]));

		sub(baddr, bsize, limit, acells, scells);

		if (and(limit, nmask, NULL, a->acells))
		{
			uInt one = 1;
			/* clear lower bits in mask, bits = mask + 1 */
			add(mask, &one, bits, acells, 1);
			/* select lowest bit in result, bits &= -bits */
			neg(bits, nbits, acells);
			and(bits, nbits, bits, acells);
			/* convert into a lower bits only mask */
			sub(bits, &one, bits, acells, 1);

			/* find the limit that will not violate the mask constraint */
			/* limit = (limit & mask) | bits */
			and(limit, mask, limit, acells);
			or(limit, bits, limit, acells);

			/* calculate the usable portion of the block */
			sub(limit, baddr, bsize, acells, acells);
		}

		DPRINTF(("find_constrained: limit mask: base %#X, size %#X\n", baddr[0], bsize[0]));

		/* all constraints met */
		if (cmp(bsize, size, scells) > 0)
		{
			/* this is the new bigest block */
			cellcpy(addr, baddr, acells);
			cellcpy(size, bsize, scells);
		}
	}

	DPRINTF(("find_constrained: found, base %%#X, size %#X\n", addr[0], size[0]));

	if (zero(size, scells))
		return E_OUT_OF_STORAGE;

	DPRINTF(("find_constrained: ok\n"));
	return NO_ERROR;
}

Retcode
alloc_info(Allocator *a, Allocator_block **bptr, uInt addr[], uInt size[])
{
	Allocator_block *b = *bptr;
	PATH("alloc_info 1");

	if (b)
	{
		PATH("alloc_info 2");
		b = b->next;
	}
	else
	{
		PATH("alloc_info 3");
		b = a->list;
	}

	*bptr = b;

	if (b == NULL)
	{
		PATH("alloc_info 4");
		return NO_ERROR;
	}

	PATH("alloc_info 5");
	cellcpy(addr, b->addr, a->acells);
	cellcpy(size, SIZE(a, b), a->scells);
	return NO_ERROR;
}

#ifdef MAIN
void
print_cells(uInt x[], int cells)
{
	int i;
	int dot = 0;

	for (i = cells - 1; i >= 0; i--)
	{
		if (dot)
			printf(".");

		dot = 1;
		printf("%04X.%04X", (x[i] >> 16) & 0xFFFF, x[i] & 0xFFFF);
	}
}

alloc_print(Allocator *a)
{
	Allocator_block *b = a->list;
	int i = 1;

	printf("Allocator: acells=%d: scells=%d: bsize=%d\n", a->acells, a->scells,
			a->bsize);

	for (; b; b = b->next)
	{
		uInt *size;
		uInt one = 1;

		printf("\t#%d: sz=", i++);
		size = SIZE(a, b);
		print_cells(size, a->scells);
		printf(": ");
		print_cells(b->addr, a->acells);
		add(b->addr, size, a->t1, a->acells, a->scells);
		sub(a->t1, &one, a->t1, a->acells, 1);
		printf(" - ");
		print_cells(a->t1, a->acells);
		printf("\n");
	}
}

void
test_allocator(int acells, int scells)
{
	Retcode ret;
	Allocator *a;
	int tsz = acells + scells;
	uInt *t1;
	uInt *t2;
	uInt **blocks;
	char *stat;
	int i;
	int mbits = scells * 32;

	printf("\nAllocator Test acells=%d scells=%d\n", acells, scells);
	printf("==================================\n");

	ret = alloc_init(&a, acells, scells);

	blocks = (uInt **)malloc(mbits * sizeof (uInt *));
	stat = (char *)malloc(mbits * sizeof (char));
	t1 = (uInt *)malloc(sizeof (uInt) * tsz * (mbits + 2));
	t2 = &t1[tsz];
	blocks[0] = &t2[tsz];

	for (i = 1; i < mbits; i++)
		blocks[i] = &blocks[i - 1][tsz];

	for (i = 0; i < acells; i++)
		t1[i] = 0;

	for (i = 0; i < scells; i++)
		t2[i] = 0;

	t2[scells - 1] = 0x80000000;	
/*	t2[scells - 1] = 0;		/ * make a "full size" block available */
	free_block(a, t1, t2);
	printf("\nInitial allocator state:\n");
	printf("------------------------\n");
	alloc_print(a);
	printf("\n");
	t2[scells - 1] = 0;

	for (i = 0; i < mbits; i++)
	{
		uInt *size = &blocks[i][acells];
		cellcpy(size, t2, scells);
		size[i / 32] = 1 << (i % 32);
		stat[i] = alloc_block(a, size, blocks[i]) == NO_ERROR;
		printf("alloc: 2^%d: %d: ", i, stat[i]);
		print_cells(blocks[i], acells);
		printf(": ");
		print_cells(size, scells);
		printf("\n");
	}

	printf("\nAllocator state after power of two allocations:\n");
	printf("------------------------\n");
	alloc_print(a);
	printf("\n");

	for (i = 1; i < mbits; i += 2)
	{

#ifdef DEBUG
		printf("about to free %d, stat %d\n", i, stat[i]);
#endif

		if (stat[i])
		{
			uInt *size = &blocks[i][acells];
			Retcode ret;

#ifdef DEBUG
			printf("before free: ");
			print_cells(blocks[i], acells);
			printf(": ");
			print_cells(size, scells);
			printf("\n");
#endif

			ret = free_block(a, blocks[i], size);

			printf("free: %d: ", ret == NO_ERROR);
			print_cells(blocks[i], acells);
			printf(": ");
			print_cells(size, scells);
			printf("\n");

			stat[i] = 0;
		}
	}

	printf("\nAllocator state after odd block frees:\n");
	printf("------------------------\n");
	alloc_print(a);
	printf("\n");

	for (i = 1; i < mbits; i += 2)
	{
		int bit = (mbits - 1) - i;
		uInt *size = &blocks[i][acells];
		cellcpy(size, t2, scells);
		size[bit / 32] = 1 << (bit % 32);
		stat[i] = alloc_block(a, size, blocks[i]) == NO_ERROR;
		printf("alloc: 2^%d: %d: ", bit, stat[i]);
		print_cells(blocks[i], acells);
		printf(": ");
		print_cells(size, scells);
		printf("\n");
	}

	printf("\nAllocator state after odd block allocations:\n");
	printf("------------------------\n");
	alloc_print(a);
	printf("\n");

	for (i = 0; i < mbits; i++)
	{
		if (stat[i])
		{
			uInt *size = &blocks[i][acells];
			Retcode ret = free_block(a, blocks[i], size);
			printf("free: %d: ", ret == NO_ERROR);
			print_cells(blocks[i], acells);
			printf(": ");
			print_cells(size, scells);
			printf("\n");
			stat[i] = 0;
		}
	}

	printf("\nAllocator state after frees:\n");
	printf("------------------------\n");
	alloc_print(a);
	printf("\n");

	alloc_fini(a);
	free(blocks);
	free(stat);
	free(t1);
}

struct test_allocator
{
	Allocator *a;
	int acells;
	int scells;
	uInt *t1;
	uInt *t2;
	uInt *t3;
	uInt *t4;
	uInt *t5;
};

typedef struct test_allocator Test_allocator;

struct test_block
{
	int allocator;
	int stat;
	uInt *a;
	uInt *s;
};

typedef struct test_block Test_block;

int
resize_allocators(Test_allocator **pallocators, int *num, int n)
{
	int size = n + 1;
	Test_allocator *a;

	if (n < *num)
		return 1;

	a = *pallocators;

	if (a)
	    a = (Test_allocator *)realloc(a, size * sizeof (Test_allocator));
	else
	    a = (Test_allocator *)malloc(size * sizeof (Test_allocator));

	if (a == NULL)
		return 0;

	memset(&a[*num], '\0', (size - *num) * sizeof (Test_allocator));
	*pallocators = a;
	*num = size;
	return 1;
}

int
resize_test_blocks(Test_block **ptestblocks, int *num, int n)
{
	int size = n + 1;
	Test_block *b;

	if (n < *num)
		return 1;

	printf("resize_test_blocks: %d\n", n);

	b = *ptestblocks;

	if (b)
	    b = (Test_block *)realloc(b, size * sizeof (Test_block));
	else
	    b = (Test_block *)malloc(size * sizeof (Test_block));

	if (b == NULL)
		return 0;

	memset(&b[*num], '\0', (size - *num) * sizeof (Test_block));
	*ptestblocks = b;
	*num = size;
	return 1;
}

int
parse_int(char *str, char **end, int *n)
{
	unsigned long ul;

	while (*str == ' ' || *str == '\t')
		str++;

	ul = strtoul(str, end, 10);
//	printf("start %s: end %s: n %d\n", str, *end, ul);
	*n = ul;
	return *end != str;
}

int
parse_cells(char *str, char **end, uInt *cells, int ncells, uInt **null)
{
	char *start;
	int c;
	int s = 0;
	int ch;

	for (c = 0; c < ncells; c++)
		cells[c] = 0;

	c = 0;

	while (*str == ' ' || *str == '\t')
		str++;

	if (str[0] == '-')
	{
	    str++;

	    if (end)
		*end = str;

	    if (null)
		*null = (uInt *)0;

	    return 1;
	}

	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
		str += 2;

	start = str;

	while (((ch = *str) >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') ||
			(ch >= 'A' && ch <= 'F') || ch == '.')
		str++;

	if (end)
		*end = str;

	if (start == str)
		return 0;

	for (str--; str >= start; str--)
	{
		int dig;

		ch = *str;

		if (ch == '.')
			continue;

		if (ch >= '0' && ch <= '9')
			dig = ch - '0';
		else if (ch >= 'a' && ch <= 'f')
			dig = ch - 'a' + 10;
		else if (ch >= 'A' && ch <= 'F')
			dig = ch - 'A' + 10;

		cells[c] |= dig << s;
		s += 4;

		if (s >= 32)
		{
			s = 0;
			c++;

			if (c >= ncells)
				break;
		}
	}

	return 1;
}

int
new_allocator(Test_allocator *a, int acells, int scells)
{
	int c = acells + scells;

	if (alloc_init(&a->a, acells, scells) != NO_ERROR)
		return 0;

	a->acells = acells;
	a->scells = scells;
	a->t1 = (uInt *)malloc(c * 5 * sizeof (uInt));

	if (a->t1 == NULL)
	{
		alloc_fini(a->a);
		return 0;
	}

	a->t2 = &a->t1[c];
	a->t3 = &a->t2[c];
	a->t4 = &a->t3[c];
	a->t5 = &a->t4[c];
	return 1;
}

int
prep_block(Test_block *b, int n, Test_allocator *a)
{
	if (b->allocator == n)
		return 1;

	if (b->a != NULL)
		free(b->a);

	b->allocator = n;
	b->stat = 0;
	b->a = (uInt *)malloc((a->acells + a->scells) * sizeof (uInt));
	b->s = &b->a[a->acells];
	return b->a != NULL;
}

void
print_block(Test_block *b, Test_allocator *a, char *label)
{
	printf("%s: %d: ", label, b->stat);
	print_cells(b->a, a->acells);
	printf(": ");
	print_cells(b->s, a->scells);
	printf("\n");
}

void
run_test(char *filename)
{
	FILE *fp = fopen(filename, "r");
	int numallocators = 0;
	int numblocks = 0;
	Test_allocator *allocators = NULL;
	Test_block *blocks = NULL;
	Test_allocator *cur = NULL;
	char buf[1024];
	char *bp;
	char *cmd;
	int line = 0;

	if (fp == NULL)
		printf("%s: unable to open\n", filename ? filename : "<null>");
		

	while (fgets(buf, sizeof buf, fp) != NULL)
	{
		int ok;

		line++;
		bp = &buf[strlen(buf)];

		while (bp > buf && (bp[-1] == '\n' || bp[-1] == '\r' || bp[-1] == ' '))
			bp--;

		*bp = '\0';
		bp = buf;

		while (*bp == ' ')
			bp++;

		switch (bp[0])
		{
		case '#':		/* comment */
		case ';':		/* comment */
		case '\0':		/* blank line */
			break;
		case ':':		/* echo */
			for (bp++; *bp == ' ' || *bp == '\t'; bp++)
				;

			printf("%s\n", bp);
			break;
		case 'n':		/* new allocatore n N A S */
			{
				int n;
				int acells;
				int scells;

				ok = parse_int(bp + 1, &bp, &n);
				ok = ok && parse_int(bp, &bp, &acells);
				ok = ok && parse_int(bp, &bp, &scells);

				if (!ok)
				{
					printf("alloc_test: %d: parse error: %s\n", line, buf);
					break;
				}

				if (!resize_allocators(&allocators, &numallocators, n))
				{
					printf("alloc_test: %d: out of memory: %s\n", line, buf);
					break;
				}

				if (!new_allocator(&allocators[n], acells, scells))
				{
					printf("alloc_test: %d: out of memory: %s\n", line, buf);
					break;
				}

				cur = &allocators[n];
			}

			break;
		case 'd':		/* delete allocator d N */
			{
				int n;

				ok = parse_int(bp + 1, &bp, &n);

				if (!ok)
				{
					printf("alloc_test: %d: parse error: %s\n", line, buf);
					break;
				}

				if (n < 0 || n >= numallocators)
				{
					printf("alloc_test: %d: allocator out of range: %s\n", line, buf);
					break;
				}

				alloc_fini(allocators[n].a);
				free(allocators[n].t1);
				memset(&allocators[n], '\0', sizeof (Test_allocator));

				if (cur == &allocators[n])
					cur = NULL;
			}

			break;
		case 'c':		/* make current allocator n */
			{
				int n;

				ok = parse_int(bp + 1, &bp, &n);

				if (!ok)
				{
					printf("alloc_test: %d: parse error: %s\n", line, buf);
					break;
				}

				if (n < 0 || n >= numallocators)
				{
					printf("alloc_test: %d: allocator out of range: %s\n", line, buf);
					break;
				}

				cur = &allocators[n];
			}

			break;
		case 'S':		/* free fixed S A SZ */
			if (cur == NULL)
			{
				printf("alloc_test: %d: no current allocator\n", line);
				break;
			}

			{
				uInt *addr = cur->t1;
				uInt *size = cur->t2;

				ok = parse_cells(bp + 1, &bp, addr, cur->acells, NULL);
				ok = ok && parse_cells(bp + 1, &bp, size, cur->scells, NULL);

				if (!ok)
				{
					printf("alloc_test: %d: parse error: %s\n", line, buf);
					break;
				}
				
				ok = free_block(cur->a, addr, size);
				printf("free_block: %d: ", ok);
				print_cells(addr, cur->acells);
				printf(": ");
				print_cells(size, cur->scells);
				printf("\n");
			}

			break;
		case 'a':		/* allocate a B SZ */
		case 'f':		/* free f B */
		case 'F':		/* allocate fixed F B A SZ */
		case 'C':		/* allocate contrained C B MIN MAX ALIGN MASK SZ */
		case 'A':		/* allocate aligned A B ALIGN SZ */
			{
				int b;

				cmd = bp;

				if (cur == NULL)
				{
					printf("alloc_test: %d: no current allocator\n", line);
					break;
				}

				ok = parse_int(bp + 1, &bp, &b);

				if (ok)
				{
					if (!resize_test_blocks(&blocks, &numblocks, b) ||
							!prep_block(&blocks[b], cur - allocators, cur))
					{
						printf("alloc_test: %d: out of memory: %s\n", line, buf);
						break;
					}
				}

				switch (cmd[0])
				{
				case 'a':		/* allocate a B SZ */
					{
						uInt *size = blocks[b].s;

						ok = ok && parse_cells(bp, &bp, size, cur->scells, NULL);

						if (!ok)
						{
							printf("alloc_test: %d: parse error: %s\n", line, buf);
							break;
						}

						blocks[b].stat = (alloc_block(cur->a, size, blocks[b].a) == NO_ERROR);
						print_block(&blocks[b], cur, "alloc_block");
					}

					break;
				case 'f':		/* free f B */
					if (!ok)
					{
						printf("alloc_test: %d: parse error: %s\n", line, buf);
						break;
					}

					blocks[b].stat = free_block(cur->a, blocks[b].a, blocks[b].s);
					print_block(&blocks[b], cur, "free_block");
					blocks[b].stat = 0;
					break;
				case 'F':		/* allocate fixed F B A SZ */
					{
						uInt *addr = blocks[b].a;
						uInt *size = blocks[b].s;

						ok = ok && parse_cells(bp, &bp, addr, cur->acells, NULL);
						ok = ok && parse_cells(bp, &bp, size, cur->scells, NULL);

						if (!ok)
						{
							printf("alloc_test: %d: parse error: %s\n", line, buf);
							break;
						}

						blocks[b].stat = (alloc_fixed(cur->a, addr, size) == NO_ERROR);
						print_block(&blocks[b], cur, "alloc_fixed");
					}

					break;
				case 'C':		/* allocate contrained C B MIN MAX ALIGN MASK SZ */
					{
						uInt *min = cur->t1;
						uInt *max = cur->t2;
						uInt *align = cur->t3;
						uInt *mask = cur->t4;
						uInt *size = blocks[b].s;
						uInt *addr = blocks[b].a;

						ok = ok && parse_cells(bp, &bp, min, cur->acells, &min);
						ok = ok && parse_cells(bp, &bp, max, cur->acells, &max);
						ok = ok && parse_cells(bp, &bp, align, cur->acells, &align);
						ok = ok && parse_cells(bp, &bp, mask, cur->acells, &mask);
						ok = ok && parse_cells(bp, &bp, size, cur->scells, NULL);

						if (!ok)
						{
							printf("alloc_test: %d: parse error: %s\n", line, buf);
							break;
						}

						blocks[b].stat = (alloc_constrained(cur->a, min, max, align, mask, size, addr) == NO_ERROR);
						print_block(&blocks[b], cur, "alloc_constrained");
					}

					break;
				case 'A':		/* allocate aligned A B ALIGN SZ */
					{
						uInt *align = cur->t1;
						uInt *size = blocks[b].s;
						uInt *addr = blocks[b].a;

						ok = ok && parse_cells(bp, &bp, align, cur->acells, NULL);
						ok = ok && parse_cells(bp, &bp, size, cur->scells, NULL);

						if (!ok)
						{
							printf("alloc_test: %d: parse error: %s\n", line, buf);
							break;
						}

						blocks[b].stat = (alloc_aligned(cur->a, align, size, addr) == NO_ERROR);
						print_block(&blocks[b], cur, "alloc_aligned");
					}

					break;
				}
			}

			break;
		case 's':		/* display allocator state */
			if (cur == NULL)
			{
				printf("alloc_test: %d: no current allocator\n", line);
				break;
			}

			alloc_print(cur->a);

			{
				int i;

				for (i = 0; i < numblocks; i++)
					if (blocks[i].allocator == cur - allocators)
					{
						printf("Block #%d: ", i);
						print_block(&blocks[i], cur, "show");
					}
			}

			break;
		default:
			printf("alloc_test: unrecognized command: %s\n", bp);
			break;
		}
	}

	close(fp);
}

int
main(int argc, char **argv)
{
	if (argc == 1)
	{
		/* test the allocator */
		test_allocator(1, 1);
		test_allocator(2, 1);
		test_allocator(2, 2);
		test_allocator(3, 1);
		test_allocator(3, 2);
		test_allocator(3, 3);
		test_allocator(4, 1);
		test_allocator(4, 2);
		test_allocator(4, 3);
		test_allocator(4, 4);
	}
	else
	    run_test(argv[1]);

	exit(0);
}
#endif
