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

/* (c) Copyright 1996-1999 by CodeGen, Inc.  All Rights Reserved */

/*#define DEBUG*/

#include <stddef.h>
#include "defs.h"
#include "pci.h"

void
pci_init_pool(Pci_addresses *pool)
{
	pool->busaddrs = (Pci_address_block *)0;
	pool->ioaddrs = (Pci_address_block *)0;
	pool->memaddrs = (Pci_address_block *)0;
	pool->prefetchaddrs = (Pci_address_block *)0;
}

void
pci_destroy_blocks(Pci_address_block *blocks)
{
	Pci_address_block *next;

	for (; blocks; blocks = next)
	{
		next = blocks->next;
		free(blocks);
	}
}

void
pci_destroy_pool(Pci_addresses *pool)
{
	pci_destroy_blocks(pool->busaddrs);
	pci_destroy_blocks(pool->ioaddrs);
	pci_destroy_blocks(pool->memaddrs);
	pci_destroy_blocks(pool->prefetchaddrs);
	pci_init_pool(pool);
}

/* Add a block of memory to the pool of free blocks */
int
pci_free_block(Pci_address_block **blocks, uInt basehi, uInt baselo,
		uInt sizehi, uInt sizelo)
{
	Pci_address_block *prev;
	Pci_address_block *b;
	uInt nbaselo = baselo + sizelo;
	uInt nbasehi = basehi + sizehi + (nbaselo < baselo);

	if (!sizehi && !sizelo)
		return 0;

	for (prev = NULL, b = *blocks; b != NULL; prev = b, b = b->next)
	{
		/* see if we've found the insertion point */
		if (basehi < b->basehi || (basehi == b->basehi && baselo < b->baselo))
			break;
	}

	if (b && b->basehi == nbasehi && b->baselo == nbaselo)
	{
		/* merge blocks */
		b->baselo = baselo;
		b->basehi = basehi;
		b->sizelo += sizelo;
		b->sizehi += sizehi + (b->sizelo < sizelo);
	}
	else
	{
		Pci_address_block *b2 =
				(Pci_address_block *)malloc(sizeof (Pci_address_block));

		if (b2 == NULL)
		{
			DPRINTF(("pci_free_block: malloc failed!\n"));
			return 0;
		}

		b2->baselo = baselo;
		b2->basehi = basehi;
		b2->sizelo = sizelo;
		b2->sizehi = sizehi;
		b2->next = b;

		if (prev)
			prev->next = b2;
		else
			*blocks = b2;

		b = b2;
	}

	if (prev)
	{
		uInt nbaselo = prev->baselo + prev->sizelo;
		uInt nbasehi = prev->basehi + prev->sizehi +
				(prev->baselo < prev->baselo);

		if (b->basehi == nbasehi && b->baselo == nbaselo)
		{
			/* merge blocks */
			prev->sizelo += b->sizelo;
			prev->sizehi += b->sizehi + (prev->sizelo < b->sizelo);
			prev->next = b->next;
			free(b);
		}
	}

	return 1;
}

/* The constraints upon allocated block are that the returned address */
/* must be greater than or equal to the min and all bits not set in the */
/* mask must be zero.  Obviously the size of the block must be greater than */
/* or equal to size.  The address of the block is returned in addr */
int
pci_allocate_block(Pci_address_block **blocks, 
		uInt sizehi, uInt sizelo, uInt alignhi, uInt alignlo,
		uInt minhi, uInt minlo, uInt maskhi, uInt masklo,
		uInt *addrhi, uInt *addrlo)
{
	Pci_address_block *prev = NULL;
	Pci_address_block *b = NULL;
	uInt amasklo = alignlo - 1;
	uInt amaskhi = alignlo ? 0 : alignhi - 1;
	uInt basehi = 0;
	uInt baselo = 0;
	uInt bsizehi = 0;
	uInt bsizelo = 0;

	DPRINTF(("pci_allocate_block: blocks %#X, size %#X,%#X, align %#X,%#X, min %#X,%#X, mask %#X,%#X, addr %#X,%#X\n", blocks, sizehi, sizelo, alignhi, alignlo, minhi, minlo, maskhi, masklo, addrhi, addrlo));

	if (!sizehi && !sizelo)
	{
		DPRINTF(("pci_allocate_block: fail, size is zero\n"));
		return 0;		/* nothing to allocate, so fail */
	}

	for (prev = NULL, b = *blocks; b != NULL; prev = b, b = b->next)
	{
		basehi = b->basehi;
		baselo = b->baselo;
		bsizehi = b->sizehi;
		bsizelo = b->sizelo;
		DPRINTF(("pci_allocate_block: b = %#X, base %#X,%#X, size %#X,%#X\n",
				b, basehi, baselo, bsizehi, bsizelo));

		/* round up */
		if ((basehi & amaskhi) || (baselo & amasklo))
		{
			uInt offhi = alignhi - (basehi & amaskhi) -
					(alignlo < (baselo & amasklo));
			uInt offlo = alignlo - (baselo & amasklo);
			DPRINTF(("pci_allocate_block: rounding block up\n"));

			if (bsizehi < offhi || ((bsizehi == offhi) && (bsizelo < offlo)))
				continue;

			bsizehi -= offhi + (bsizelo < offlo);
			bsizelo -= offlo;
			baselo += offlo;
			basehi += offhi - (baselo < offlo);
			DPRINTF(("pci_allocate_block: base %#X,%#X, size %#X,%#X\n", basehi, baselo, bsizehi, bsizelo));
		}

		/* check for min constraint */
		if (basehi < minhi || ((basehi == minhi) && (baselo < minlo)))
		{
			uInt offhi = minhi - basehi - (minlo < baselo);
			uInt offlo = minlo - baselo;
			DPRINTF(("pci_allocate_block: doing min adjustment\n"));

			if (bsizehi < offhi || ((bsizehi == offhi) && (bsizelo < offlo)))
				continue;

			bsizehi -= offhi + (bsizelo < offlo);
			bsizelo -= offlo;
			basehi = minhi;
			baselo = minlo;
			DPRINTF(("pci_allocate_block: base %#X,%#X, size %#X,%#X\n", basehi, baselo, bsizehi, bsizelo));
		}

		/* check for mask constraint */
		if ((basehi & ~maskhi) || (baselo & ~masklo))
			continue;

		/* check for big enough block */
		if (bsizehi < sizehi || (bsizehi == sizehi && bsizelo < sizelo))
			continue;

		/* all constraints met */
		break;
	}

	if (!b)
	{
		DPRINTF(("pci_allocate_block: fail, no block found\n"));
		return 0;
	}

	if (addrlo)
		*addrlo = baselo;

	if (addrhi)
		*addrhi = basehi;

	DPRINTF(("pci_allocate_block: found, base %#X,%#X, size %#X,%#X, b = %#X\n", basehi, baselo, bsizehi, bsizelo, b));

	/* remove the block */
	if (prev)
		prev->next = b->next;
	else
		*blocks = b->next;

	/* free the leading chunk */
	if (basehi != b->basehi || baselo != b->baselo)
		pci_free_block(blocks, b->basehi, b->baselo,
				basehi - b->basehi - (baselo < b->baselo), baselo - b->baselo);

	/* free the trailing chunk */
	if (sizehi < bsizehi || (sizehi == bsizehi && sizelo < bsizelo))
		pci_free_block(blocks, basehi + sizehi + (baselo + sizelo < sizelo),
				baselo + sizelo, bsizehi - sizehi - (bsizelo < sizelo),
				bsizelo - sizelo);

	free(b);
	DPRINTF(("pci_allocate_block: ok\n"));
	return 1;
}

int
pci_allocate_fixed_block(Pci_address_block **blocks,
		uInt addrhi, uInt addrlo, uInt sizehi, uInt sizelo)
{
	Pci_address_block *prev = NULL;
	Pci_address_block *b = NULL;
	uInt limhi;
	uInt limlo;
	uInt basehi = 0;
	uInt baselo = 0;
	uInt bsizehi = 0;
	uInt bsizelo = 0;
	uInt limithi = 0;
	uInt limitlo = 0;

	DPRINTF(("pci_allocate_block: blocks %#X, base %#X,%#X, size %#X,%#X\n", blocks, addrhi, addrlo, sizehi, sizelo));

	if (!sizehi && !sizelo)
	{
		DPRINTF(("pci_allocate_fixed_block: fail, size is zero\n"));
		return 0;		/* nothing to allocate, so fail */
	}

	limlo = addrlo + sizelo;
	limhi = addrhi + sizehi + (limlo < sizelo);

	for (prev = NULL, b = *blocks; b != NULL; prev = b, b = b->next)
	{
		basehi = b->basehi;
		baselo = b->baselo;
		bsizehi = b->sizehi;
		bsizelo = b->sizelo;
		DPRINTF(("pci_allocate_fixed_block: b = %#X, base %#X,%#X, size %#X,%#X\n",
				b, basehi, baselo, bsizehi, bsizelo));

		limitlo = baselo + bsizelo;
		limithi = basehi + bsizehi + (limitlo < bsizelo);

		if (limhi < limithi || (limhi == limithi && limlo <= limitlo))
			break;
	}

	if (!b)
	{
		DPRINTF(("pci_allocate_fixed_block: fail, no block found\n"));
		return 0;
	}

	if (!(basehi < addrhi || (basehi == addrhi && baselo <= addrlo)))
	{
		DPRINTF(("pci_allocate_fixed_block: fail, already allocated\n"));
		return 0;
	}

	DPRINTF(("pci_allocate_fixed_block: found, base %#X,%#X, size %#X,%#X, b = %#X\n", basehi, baselo, bsizehi, bsizelo, b));

	/* remove the block */
	if (prev)
		prev->next = b->next;
	else
		*blocks = b->next;

	/* free the leading chunk */
	if (addrhi != basehi || addrlo != baselo)
		pci_free_block(blocks, basehi, baselo,
				addrhi - basehi - (addrlo < baselo), addrlo - baselo);

	/* free the trailing chunk */
	if (limhi != limithi || limlo != limitlo)
		pci_free_block(blocks, limhi, limlo, 
				limithi - limhi - (limitlo < limlo), limitlo - limlo);

	free(b);
	DPRINTF(("pci_allocate_fixed_block: ok\n"));
	return 1;
}

int
pci_find_largest_block(Pci_address_block **blocks, 
		uInt alignhi, uInt alignlo, uInt minhi, uInt minlo,
		uInt maskhi, uInt masklo,
		uInt *addrhi, uInt *addrlo, uInt *sizehi, uInt *sizelo)
{
	Pci_address_block *b = NULL;
	uInt amasklo = alignlo - 1;
	uInt amaskhi = alignlo ? 0 : alignhi - 1;
	uInt basehi;
	uInt baselo;
	uInt bsizehi;
	uInt bsizelo;
	uInt limithi;
	uInt limitlo;
	uInt mbasehi = 0;
	uInt mbaselo = 0;
	uInt msizehi = 0;
	uInt msizelo = 0;

	DPRINTF(("pci_find_largest_block: blocks %#X, size %#X,%#X, align %#X,%#X, min %#X,%#X, mask %#X,%#X, addr %#X,%#X\n", blocks, sizehi, sizelo, alignhi, alignlo, minhi, minlo, maskhi, masklo, addrhi, addrlo));

	for (b = *blocks; b != NULL; b = b->next)
	{
		basehi = b->basehi;
		baselo = b->baselo;
		bsizehi = b->sizehi;
		bsizelo = b->sizelo;
		DPRINTF(("pci_find_largest_block: b = %#X, base %#X,%#X, size %#X,%#X\n",
				b, basehi, baselo, bsizehi, bsizelo));

		/* round up */
		if ((basehi & amaskhi) || (baselo & amasklo))
		{
			uInt offhi = alignhi - (basehi & amaskhi) -
					(alignlo < (baselo & amasklo));
			uInt offlo = alignlo - (baselo & amasklo);
			DPRINTF(("pci_find_largest_block: rounding block up\n"));

			if (bsizehi < offhi || ((bsizehi == offhi) && (bsizelo < offlo)))
				continue;

			bsizehi -= offhi + (bsizelo < offlo);
			bsizelo -= offlo;
			baselo += offlo;
			basehi += offhi - (baselo < offlo);
			DPRINTF(("pci_find_largest_block: base %#X,%#X, size %#X,%#X\n", basehi, baselo, bsizehi, bsizelo));
		}

		/* check for min constraint */
		if (basehi < minhi || ((basehi == minhi) && (baselo < minlo)))
		{
			uInt offhi = minhi - basehi - (minlo < baselo);
			uInt offlo = minlo - baselo;
			DPRINTF(("pci_find_largest_block: doing min adjustment\n"));

			if (bsizehi < offhi || ((bsizehi == offhi) && (bsizelo < offlo)))
				continue;

			bsizehi -= offhi + (bsizelo < offlo);
			bsizelo -= offlo;
			basehi = minhi;
			baselo = minlo;
			DPRINTF(("pci_find_largest_block: base %#X,%#X, size %#X,%#X\n", basehi, baselo, bsizehi, bsizelo));
		}

		/* check for mask constraint */
		if ((basehi & ~maskhi) || (baselo & ~masklo))
			continue;

		/* round down size */
		bsizehi &= ~amaskhi;
		bsizelo &= ~amasklo;

		if (!bsizehi && !bsizelo)
			continue;

		DPRINTF(("pci_find_largest_block: size algin: base %#X,%#X, size %#X,%#X\n", basehi, baselo, bsizehi, bsizelo));

		limitlo = baselo + bsizelo;
		limithi = basehi + bsizehi + (limitlo < bsizelo);

		if ((limithi & ~maskhi) || (limitlo & ~masklo))
		{
			uInt bitslo = masklo + 1;
			uInt bitshi = maskhi + (bitslo == 0);
			int carry = bitslo == 0;

			bitslo &= -bitslo;
			bitshi &= ~bitshi + carry;

			bitshi -= bitslo == 0;
			bitslo--;

			limithi &= maskhi;
			limitlo &= masklo;
			limithi |= bitshi; 
			limitlo |= bitslo;
			bsizehi = limithi - basehi - (limitlo < baselo);
			bsizelo = limitlo - baselo;
		}

		DPRINTF(("pci_find_largest_block: limit mask: base %#X,%#X, size %#X,%#X\n", basehi, baselo, bsizehi, bsizelo));

		/* all constraints met */
		if (bsizehi > msizehi || (bsizehi == msizehi && bsizelo > msizelo))
		{
			mbasehi = basehi;
			mbaselo = baselo;
			msizehi = bsizehi;
			msizelo = bsizelo;
		}
	}

	DPRINTF(("pci_find_largest_block: found, base %#X,%#X, size %#X,%#X\n", mbasehi, mbaselo, msizehi, msizelo));

	if (!msizehi && !msizelo)
		return 0;

	if (addrlo)
		*addrlo = mbaselo;

	if (addrhi)
		*addrhi = mbasehi;

	if (sizelo)
		*sizelo = msizelo;

	if (sizehi)
		*sizehi = msizehi;

	DPRINTF(("pci_find_largest_block: ok\n"));
	return 1;
}

int
pci_allocate_buses(Pci_addresses *addrs, int num)
{
	uInt busbase;

	if (pci_allocate_block(&addrs->busaddrs, 0, num, 0, 1, 0, 0, 0, 0xFF, NULL,
			&busbase))
		return busbase;

	return 0;
}

uInt
pci_allocate_io10(Pci_addresses *addrs, uInt size)
{
	uInt iobase;

	if (pci_allocate_block(&addrs->ioaddrs, 0, size, 0, size, 0, 0, 0, 0x3FF,
			NULL, &iobase))
		return iobase;

	return 0;
}

uInt
pci_allocate_io16(Pci_addresses *addrs, uInt size)
{
	uInt iobase;

	if (pci_allocate_block(&addrs->ioaddrs, 0, size, 0, size, 0, 0x1000,
			0, 0xFFFF, NULL, &iobase))
		return iobase;

	return pci_allocate_io10(addrs, size);
}

uInt
pci_allocate_io32(Pci_addresses *addrs, uInt size)
{
	uInt iobase;

	if (pci_allocate_block(&addrs->ioaddrs, 0, size, 0, size, 0, 0x10000,
			0, 0xFFFFFFFF, NULL, &iobase))
		return iobase;

	return pci_allocate_io16(addrs, size);
}

uInt
pci_allocate_mem20(Pci_addresses *addrs, uInt size)
{
	uInt membase;

	if (pci_allocate_block(&addrs->memaddrs, 0, size, 0, size, 0, 0,
			0, 0xFFFFF, NULL, &membase))
		return membase;

	return 0;
}

uInt
pci_allocate_mem32(Pci_addresses *addrs, uInt size)
{
	uInt membase;

	if (pci_allocate_block(&addrs->memaddrs, 0, size, 0, size, 0, 0x100000,
			0, 0xFFFFFFFF, NULL, &membase))
		return membase;

	return pci_allocate_mem20(addrs, size);
}

int
pci_allocate_mem64(Pci_addresses *addrs, uInt sizehi, uInt sizelo,
		uInt *addrhi, uInt *addrlo)
{
	uInt addr;

	if (pci_allocate_block(&addrs->memaddrs, sizehi, sizelo, sizehi, sizelo,
			1, 0, 0xFFFFFFFF, 0xFFFFFFFF, addrhi, addrlo))
		return 1;

	if (sizehi)
		return 0;
	
	addr = pci_allocate_mem32(addrs, sizelo);

	if (!addr)
		return 0;

	if (addrlo)
		*addrlo = addr;

	if (addrhi)
		*addrhi = 0;

	return 1;
}

uInt
pci_allocate_pmem32(Pci_addresses *addrs, uInt size)
{
	uInt membase;

	if (pci_allocate_block(&addrs->prefetchaddrs, 0, size, 0, size, 0, 0x100000,
			0, 0xFFFFFFFF, NULL, &membase))
		return membase;

	return pci_allocate_mem32(addrs, size);
}

int
pci_allocate_pmem64(Pci_addresses *addrs, uInt sizehi, uInt sizelo,
		uInt *addrhi, uInt *addrlo)
{
	uInt addr;

	if (pci_allocate_block(&addrs->prefetchaddrs, sizehi, sizelo,
			sizehi, sizelo, 1, 0, 0xFFFFFFFF, 0xFFFFFFFF, addrhi, addrlo))
		return 1;

	if (sizehi)
		return 0;
	
	addr = pci_allocate_pmem32(addrs, sizelo);

	if (!addr)
		return 0;

	if (addrlo)
		*addrlo = addr;

	if (addrhi)
		*addrhi = 0;

	return 1;
}

int
pci_free_bus_range(Pci_addresses *addrs, int base, int size)
{
	return pci_free_block(&addrs->busaddrs, 0, base, 0, size);
}

int
pci_free_io_range(Pci_addresses *addrs, uInt base, uInt size)
{
	return pci_free_block(&addrs->ioaddrs, 0, base, 0, size);
}

int
pci_free_mem_range(Pci_addresses *addrs, uInt basehi, uInt baselo,
	uInt sizehi, uInt sizelo)
{
	return pci_free_block(&addrs->memaddrs, basehi, baselo, sizehi, sizelo);
}

int
pci_free_pmem_range(Pci_addresses *addrs, uInt basehi, uInt baselo,
	uInt sizehi, uInt sizelo)
{
	return pci_free_block(&addrs->prefetchaddrs, basehi, baselo, sizehi, sizelo);
}
