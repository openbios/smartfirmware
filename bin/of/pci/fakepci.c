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

#include "defs.h"
#include "pci.h"
#include "fakepci.h"

#ifdef FAKE_PCI
extern Fake_pci_dev_methods dc21140_methods;

static struct fake_pci_dev init_fake_pci_devs[] =
{
    { 0, 7, 0, &dc21140_methods },
    { 0, 8, 0, &dc21140_methods },
    { 0, 9, 0, &dc21140_methods },
    { 0, 9, 1, &dc21140_methods },
    { 1, 2, 0, &dc21140_methods },
};

static struct fake_pci_dev ****fake_pci_devs = NULL;

static void
do_fake_init()
{
	int i, j;

	for (i = 0; i < sizeof init_fake_pci_devs / sizeof init_fake_pci_devs[0];
			i++)
	{
		struct fake_pci_dev *fake_dev = &init_fake_pci_devs[i];
		int bus = fake_dev->bus;
		int dev = fake_dev->dev;

		if (!fake_dev->methods->init(fake_dev))
			continue;

		if (fake_pci_devs == NULL)
		{
			fake_pci_devs = (struct fake_pci_dev ****)malloc(256 *
					sizeof (struct fake_pci_dev ***));

			if (fake_pci_devs == NULL)
				return;

			for (j = 0; j < 256; j++)
				fake_pci_devs[j] = NULL;
		}

		if (fake_pci_devs[bus] == NULL)
		{
			fake_pci_devs[bus] = (struct fake_pci_dev ***)malloc(32 *
					sizeof (struct fake_pci_dev **));

			if (fake_pci_devs[bus] == NULL)
				return;

			for (j = 0; j < 32; j++)
				fake_pci_devs[bus][j] = NULL;
		}

		if (fake_pci_devs[bus][dev] == NULL)
		{
			fake_pci_devs[bus][dev] = (struct fake_pci_dev **)malloc(8 *
					sizeof (struct fake_pci_dev *));

			if (fake_pci_devs[bus][dev] == NULL)
				return;

			for (j = 0; j < 8; j++)
				fake_pci_devs[bus][dev][j] = NULL;
		}

		fake_pci_devs[bus][dev][fake_dev->func] = fake_dev;
	}
}

Int
pci_num_host_bridges()
{
	return 1;
}

Int
pci_config_read(Int hbridge, Int bus, Int dev, Int func, Int addr)
{
	struct fake_pci_dev *fake_dev;

	if (hbridge)
		return 0;

	if (fake_pci_devs == NULL)
		do_fake_init();
	
	if (fake_pci_devs == NULL ||
			fake_pci_devs[bus] == NULL ||
			fake_pci_devs[bus][dev] == NULL ||
			fake_pci_devs[bus][dev][func] == NULL)
		return 0;

	fake_dev = fake_pci_devs[bus][dev][func];
	addr &= ~3;

	if (addr < 0 || addr >= fake_dev->configsize)
		return 0;

	return (*fake_dev->methods->configread)(fake_dev, addr);
}

void
pci_config_write(Int hbridge, Int bus, Int dev, Int func, Int addr, Int value)
{
	struct fake_pci_dev *fake_dev;

	if (hbridge)
		return;

	if (fake_pci_devs == NULL)
		do_fake_init();
	
	if (fake_pci_devs == NULL ||
			fake_pci_devs[bus] == NULL ||
			fake_pci_devs[bus][dev] == NULL ||
			fake_pci_devs[bus][dev][func] == NULL)
		return;

	fake_dev = fake_pci_devs[bus][dev][func];
	addr &= ~3;

	if (addr < 0 || addr >= fake_dev->configsize)
		return;

	(*fake_dev->methods->configwrite)(fake_dev, addr, value);
}

Retcode
pci_intr_ack(Int hbridge)
{
    return NO_ERROR;
}

Retcode
pci_special_cycle(Int hbridge, Int bus, Int data)
{
    return NO_ERROR;
}

struct devmap
{
	Int base;
	Int size;
	Fake_pci_dev *dev;
	Int offset;
	int bar;
};

#define IS_ENTRY(p)			((Ptr)(p) & 1)
#define TAG_ENTRY(p)		((void *)((Ptr)(p) | 1))
#define ENTRY_POINTER(p)	((struct devmap *)((Ptr)(p) & ~1))

typedef struct devmap ****Addrmap;

static Addrmap mem_map = NULL;
static Addrmap io_map = NULL;

static Retcode
fakepci_lookupentry(Addrmap *map, Int addr, struct devmap **entry)
{
	Addrmap m = *map;
	int index1 = (addr >> 22) & 0x3FF;
	int index2 = (addr >> 12) & 0x3FF;
	int index3 = (addr >> 2) & 0x3FF;

	if (m == NULL)
		return E_NO_DEVICE;

	if (m[index1] == NULL)
		return E_NO_DEVICE;

	if (IS_ENTRY(m[index1]))
	{
		*entry = ENTRY_POINTER(m[index1]);
		return NO_ERROR;
	}

	if (m[index1][index2] == NULL)
		return E_NO_DEVICE;

	if (IS_ENTRY(m[index1][index2]))
	{
		*entry = ENTRY_POINTER(m[index1][index2]);
		return NO_ERROR;
	}

	if (m[index1][index2][index3] == NULL)
		return E_NO_DEVICE;

	*entry = m[index1][index2][index3];
	return NO_ERROR;
}

static struct devmap **
fakepci_alloctable()
{
	int i;
	struct devmap **t =
			(struct devmap **)malloc(1024 * sizeof (struct devmap *));

	if (t == NULL)
		return NULL;

	for (i = 0; i < 1024; i++)
		t[i] = NULL;

	return t;
}

static Retcode
fakepci_insertentry(Addrmap *map, Int addr, int level, struct devmap *entry)
{
	Addrmap m;
	int index1 = (addr >> 22) & 0x3FF;
	int index2 = (addr >> 12) & 0x3FF;
	int index3 = (addr >> 2) & 0x3FF;

	/* place the entry in the mapping tree */
	if (*map == NULL)
	{
		*map = (struct devmap ****)fakepci_alloctable();

		if (*map == NULL)
			return E_OUT_OF_MEMORY;
	}

	m = *map;

	/* set the 1st level index */
	if (level == 1)
	{
		if (m[index1] != NULL)
			return E_NO_DEVICE;

		m[index1] = TAG_ENTRY(entry);
		return NO_ERROR;
	}

	if (IS_ENTRY(m[index1]))
		return E_NO_DEVICE;

	if (m[index1] == NULL)
	{
		m[index1] = (struct devmap ***)fakepci_alloctable();

		if (m[index1] == NULL)
			return E_OUT_OF_MEMORY;
	}

	if (level == 2)
	{
		if (m[index1][index2] != NULL)
			return E_NO_DEVICE;

		m[index1][index2] = TAG_ENTRY(entry);
		return NO_ERROR;
	}

	if (IS_ENTRY(m[index1][index2]))
		return E_NO_DEVICE;

	if (m[index1][index2] == NULL)
	{
		m[index1][index2] = fakepci_alloctable();

		if (m[index1][index2] == NULL)
			return E_OUT_OF_MEMORY;
	}

	if (m[index1][index2][index3] != NULL)
		return E_NO_DEVICE;

	m[index1][index2][index3] = entry;
	return NO_ERROR;
}

static Retcode
fakepci_removeentry(Addrmap *map, Int addr, int level)
{
	Addrmap m = *map;
	int index1 = (addr >> 22) & 0x3FF;
	int index2 = (addr >> 12) & 0x3FF;
	int index3 = (addr >> 2) & 0x3FF;

	/* place the entry in the mapping tree */
	if (m == NULL)
		return E_NO_DEVICE;

	if (level == 1)
	{
		if (!IS_ENTRY(m[index1]))
			return E_NO_DEVICE;

		m[index1] = NULL;
		return NO_ERROR;
	}

	if (IS_ENTRY(m[index1]) || m[index1] == NULL)
		return E_NO_DEVICE;

	if (level == 2)
	{
		if (!IS_ENTRY(m[index1][index2]))
			return E_NO_DEVICE;

		m[index1][index2] = NULL;
		return NO_ERROR;
	}

	if (IS_ENTRY(m[index1][index2]) || m[index1][index2] == NULL)
		return E_NO_DEVICE;

	if (m[index1][index2][index3] != NULL)
		return E_NO_DEVICE;

	m[index1][index2][index3] = NULL;
	return NO_ERROR;
}

static Retcode
fakepci_lookup(Addrmap *map, Int addr, Fake_pci_dev **dev, int *bar,
		Int *offset)
{
	struct devmap *entry;
	Retcode ret;

	if ((ret = fakepci_lookupentry(map, addr, &entry)) != 0)
		return ret;

	if (addr < entry->base || addr >= entry->base + entry->size)
		return E_NO_DEVICE;

	*dev = entry->dev;
	*bar = entry->bar;
	*offset = (addr - entry->base) + entry->offset;
	return NO_ERROR;
}

static Retcode
fakepci_map(Addrmap *map, Int addr, Int size, Fake_pci_dev *dev, int bar,
	Int offset) 
{
	struct devmap *entry = (struct devmap *)malloc(sizeof (struct devmap));
	Int inc;
	Retcode ret = NO_ERROR;
	int level;

	if (entry == NULL)
		return E_OUT_OF_MEMORY;

	entry->base = addr;
	entry->size = size;
	entry->dev = dev;
	entry->bar = bar;
	entry->offset = offset;

	if (size < 4096)
		level = 3;
	else if (size < 4194304)
		level = 2;
	else
		level = 1;

	inc = 4 << (10 * (3 - level));

	for (offset = 0; offset < size; offset += inc)
		if ((ret = fakepci_insertentry(map, addr + offset, level, entry)) != 0)
			break;

	if (offset < size)
	{
		size = offset;

		for (offset = 0; offset < size; offset += inc)
			fakepci_removeentry(map, addr + offset, level);

		free(entry);
		return ret;
	}

	return NO_ERROR;
}

static Retcode
fakepci_unmap(Addrmap *map, Int addr, Int size, Fake_pci_dev *dev, int bar,
		Int offset)
{
	struct devmap *entry;
	Retcode ret;
	int level;
	Int inc;

	if ((ret = fakepci_lookupentry(map, addr, &entry)) != 0)
		return ret;

	/* make sure the region to be unmapped is within the current region */
	if (entry->base != addr || entry->size != size)
		return E_NO_DEVICE;

	if (entry->dev != dev || entry->bar != bar || entry->offset != offset)
		return E_NO_DEVICE;

	/* remove the existing pointers */
	if (size < 4096)
		level = 3;
	else if (size < 4194304)
		level = 2;
	else
		level = 1;

	inc = 4 << (10 * (3 - level));

	for (offset = 0; offset < entry->size; offset += inc)
		fakepci_removeentry(map, addr + offset, level);

	free(entry);
	return NO_ERROR;
}

Retcode
fakepci_mem_map(Int addr, Int size, Fake_pci_dev *dev, int bar, Int offset)
{
	return fakepci_map(&mem_map, addr, size, dev, bar, offset);
}

Retcode
fakepci_mem_unmap(Int addr, Int size, Fake_pci_dev *dev, int bar, Int offset)
{
	return fakepci_unmap(&mem_map, addr, size, dev, bar, offset);
}

Retcode
fakepci_io_map(Int addr, Int size, Fake_pci_dev *dev, int bar, Int offset)
{
	return fakepci_map(&io_map, addr, size, dev, bar, offset);
}

Retcode
fakepci_io_unmap(Int addr, Int size, Fake_pci_dev *dev, int bar, Int offset)
{
	return fakepci_unmap(&io_map, addr, size, dev, bar, offset);
}

Retcode
pci_mem_read(Int hbridge, uInt addr, void *value, int size)
{
	Fake_pci_dev *dev;
	int bar;
	Int offset;
	Retcode ret;

	if (hbridge)
		return E_INVALID_POINTER;

	ret = fakepci_lookup(&mem_map, addr, &dev, &bar, &offset);

	if (ret != NO_ERROR)
		return ret;

	return dev->methods->memread(dev, bar, offset, value, size);
}

Retcode
pci_mem_read64(Int hbridge, uInt addrhi, uInt addrlo, void *value, int size)
{
	if (addrhi)
		return E_INVALID_POINTER;

	return pci_mem_read(hbridge, addrlo, value, size);
}

Retcode
pci_mem_write(Int hbridge, uInt addr, Int value, int size)
{
	Fake_pci_dev *dev;
	int bar;
	Int offset;
	Retcode ret;

	if (hbridge)
		return E_INVALID_POINTER;

	ret = fakepci_lookup(&mem_map, addr, &dev, &bar, &offset);

	if (ret != NO_ERROR)
		return ret;

	return dev->methods->memwrite(dev, bar, offset, value, size);
}

Retcode
pci_mem_write64(Int hbridge, uInt addrhi, uInt addrlo, Int value, int size)
{
	if (addrhi)
		return E_INVALID_POINTER;

	return pci_mem_write(hbridge, addrlo, value, size);
}

Retcode
pci_io_read(Int hbridge, uInt addr, void *value, int size)
{
	Fake_pci_dev *dev;
	int bar;
	Int offset;
	Retcode ret;

	if (hbridge)
		return E_INVALID_POINTER;

	ret = fakepci_lookup(&io_map, addr, &dev, &bar, &offset);

	if (ret != NO_ERROR)
		return ret;

	return dev->methods->ioread(dev, bar, offset, value, size);
}

Retcode
pci_io_write(Int hbridge, uInt addr, Int value, int size)
{
	Fake_pci_dev *dev;
	int bar;
	Int offset;
	Retcode ret;

	if (hbridge)
		return E_INVALID_POINTER;

	ret = fakepci_lookup(&io_map, addr, &dev, &bar, &offset);

	if (ret != NO_ERROR)
		return ret;

	return dev->methods->iowrite(dev, bar, offset, value, size);
}

Retcode
pci_map_in(Int hbridge, Int phys_hi, Int phys_mid, Int phys_low,
	Cell size_hi, Cell *virt)
{
	if (hbridge)
		return E_INVALID_POINTER;

	*virt = 0;
	return NO_ERROR;
}

Retcode
pci_map_out(Int hbridge, Cell virt, Cell size)
{
	if (hbridge)
		return E_INVALID_POINTER;

	return NO_ERROR;
}

Retcode
pci_dma_alloc(Int hbridge, Cell size, Cell *virt)
{
	if (hbridge)
		return E_INVALID_POINTER;

	*virt = 0;
	return NO_ERROR;
}

Retcode
pci_dma_free(Int hbridge, Cell virt, Cell size)
{
	if (hbridge)
		return E_INVALID_POINTER;

	return NO_ERROR;
}

Retcode
pci_dma_map_in(Int hbridge, Cell virt, Cell size, Int cacheable,
		Cell *devaddr)
{
	if (hbridge)
		return E_INVALID_POINTER;

	*devaddr = virt;
	return NO_ERROR;
}

Retcode
pci_dma_map_out(Int hbridge, Cell virt, Cell devaddr, Cell size)
{
	if (hbridge)
		return E_INVALID_POINTER;

	return NO_ERROR;
}

Retcode
pci_dma_sync(Int hbridge, Cell virt, Cell devaddr, Cell size)
{
	if (hbridge)
		return E_INVALID_POINTER;

	return NO_ERROR;
}

Retcode
pci_init_addresses(Int hbridge, Pci_addresses *addrs)
{
	int i;

	if (hbridge)
		return E_INVALID_POINTER;

	memset(addrs, 0, sizeof (Pci_addresses));
	pci_free_bus_range(addrs, 1, 255);

	for (i = 0x1000; i < 0x10000; i += 0x1000)
	    pci_free_io_range(addrs, i, 256);

	pci_free_mem_range(addrs, 0, 0x80000, 0, 0x80000);
	pci_free_mem_range(addrs, 0, 0x10000000, 0, 0xF0000000);
	return NO_ERROR;
}

Retcode
pci_bus_package(Environ *e, Int hbridge, Package **pkg)
{
	if (hbridge)
		return E_INVALID_POINTER;

	*pkg = e->currpkg;
	return NO_ERROR;
}

Retcode
pci_config_map_intr(Int hbridge, Int bus, Int dev, Int pin, Int *intr)
{
	return E_NO_INTR_MAP;
}

#endif /* FAKE_PCI */
