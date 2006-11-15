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

#define PCI_CONFIG_ADDR		0xCF8ul
#define PCI_CONFIG_DATA		0xCFCul
#define PCI_MEMORY_BASE		0x00000000ul
#define PCI_MEMORY_END		0xFFFFFFFFul
#define PCI_HOST_MEM_BASE	0x00000000ul
#define PCI_HOST_MEM_END	0xFFFFFFFFul


Int
pci_num_host_bridges()
{
	return 1;
}

Int
pci_config_read(Int hbridge, Int bus, Int dev, Int func, Int addr)
{
	uInt a;
	
	if (hbridge)
		return 0;
		
	addr &= ~3;
	a = PCI_PHYSHI_MK(1, 0, 0, 0, bus, dev, func, addr);
	pci_io_write(hbridge, PCI_CONFIG_ADDR, LE4(a), sizeof a);
	pci_io_read(hbridge, PCI_CONFIG_DATA, (void *)&a, sizeof a);
	pci_io_write(hbridge, PCI_CONFIG_ADDR, 0, 4);
	DPRINTF((
		"pci_config_read: hbridge=%d bus=%d dev=%d func=%d addr=%#x val=%#x\n",
			hbridge, bus, dev, func, addr, LE4(a)));
	return LE4(a);
}

void
pci_config_write(Int hbridge, Int bus, Int dev, Int func, Int addr, Int value)
{
	uInt a;
	
	if (hbridge)
		return;

	addr &= ~3;
	a = PCI_PHYSHI_MK(1, 0, 0, 0, bus, dev, func, addr);
	pci_io_write(hbridge, PCI_CONFIG_ADDR, LE4(a), sizeof a);
	pci_io_write(hbridge, PCI_CONFIG_DATA, LE4(value), sizeof value);
	pci_io_write(hbridge, PCI_CONFIG_ADDR, 0, 4);
	DPRINTF((
		"pci_config_write: hbridge=%d bus=%d dev=%d func=%d addr=%#x val=%#x\n",
			hbridge, bus, dev, func, addr, LE4(value)));
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

void
do_read(uInt addr, void *value, int size)
{
	switch (size)
	{
	case 1:
		*(Byte*)value = *(Byte*)addr;
		break;
	
	case 2:
		*(Short*)value = LE2(*(uShort*)addr);
		break;
	
	case 4:
		*(Int*)value = LE4(*(uInt*)addr);
		break;
	}
}

void
do_write(uInt addr, Int value, int size)
{
	switch (size)
	{
	case 1:
		*(uByte*)addr = (uByte)value;
		break;
	
	case 2:
		*(uShort*)addr = LE2((uShort)value);
		break;
	
	case 4:
		*(uInt*)addr = LE4((uShort)value);
		break;
	}
}

Retcode
pci_mem_read(Int hbridge, uInt addr, void *value, int size)
{
	if (hbridge)
		return E_INVALID_POINTER;

	do_read(PCI_MEMORY_BASE + addr, value, size);
	return NO_ERROR;
}

Retcode
pci_mem_read64(Int hbridge, uInt addrhi, uInt addrlo, void *value, int size)
{
	if (addrhi)
		return E_INVALID_POINTER;

	pci_mem_read(hbridge, addrlo, value, size);
	return NO_ERROR;
}

Retcode
pci_mem_write(Int hbridge, uInt addr, Int value, int size)
{
	if (hbridge)
		return E_INVALID_POINTER;

	do_write(PCI_MEMORY_BASE + addr, value, size);
	return NO_ERROR;
}

Retcode
pci_mem_write64(Int hbridge, uInt addrhi, uInt addrlo, Int value, int size)
{
	if (addrhi)
		return E_INVALID_POINTER;

	pci_mem_write(hbridge, addrlo, value, size);
	return NO_ERROR;
}

Retcode
pci_io_read(Int hbridge, uInt addr, void *value, int size)
{
	uByte b;
	uShort s;
	uInt i;

	if (hbridge)
		return E_INVALID_POINTER;

	switch (size)
	{
	case 1:
		__asm __volatile("inb %%dx,%0" : "=a" (b) : "d" (addr));
		*(uByte*)value = b;
		break;

	case 2:
		__asm __volatile("inw %%dx,%0" : "=a" (s) : "d" (addr));
		*(uShort*)value = s;
		break;

	case 4:
		__asm __volatile("inl %%dx,%0" : "=a" (i) : "d" (addr));
		*(uInt*)value = i;
		break;

	default:
		return E_BAD_ARGUMENT;
	}

	return NO_ERROR;
}

Retcode
pci_io_write(Int hbridge, uInt addr, Int value, int size)
{
	uByte b;
	uShort s;
	uInt i;

	if (hbridge)
		return E_INVALID_POINTER;

	switch (size)
	{
	case 1:
		b = (uByte)value;
		__asm __volatile("outb %0,%%dx" : : "a" (b), "d" (addr));
		break;

	case 2:
		s = (uShort)value;
		__asm __volatile("outw %0,%%dx" : : "a" (s), "d" (addr));
		break;

	case 4:
		i = (uInt)value;
		__asm __volatile("outl %0,%%dx" : : "a" (i), "d" (addr));
		break;

	default:
		return E_BAD_ARGUMENT;
	}

	return NO_ERROR;
}

Retcode
pci_map_in(Int hbridge, Int phys_hi, Int phys_mid, Int phys_low, Cell size_hi,
		Cell *virt)
{
	DPRINTF(("hbridge=%#x phys.hi=%#x.%#x.%#x size.hi=%#x.%#x\n",
			hbridge, phys_hi, phys_mid, phys_low, size_hi));

	if (hbridge || phys_mid)
		return E_INVALID_POINTER;

	*virt = phys_low + PCI_MEMORY_BASE;
	return NO_ERROR;
}

Retcode
pci_map_out(Int hbridge, Cell virt, Cell size)
{
	if (hbridge)
		return E_INVALID_POINTER;

	return NO_ERROR;
}

#define MAX_DMA_LIST	48
#define DMA_PAGE_SIZE	4096

static struct dma_list
{
	Byte *mem;
	Cell dma;
	Int size;
} g_dma_list[MAX_DMA_LIST];

Retcode
pci_dma_alloc(Int hbridge, Cell size, Cell *virt)
{
	int i;

	if (hbridge)
		return E_INVALID_POINTER;

	/* try to find an empty slot in our list */
	for (i = 0; i < MAX_DMA_LIST; i++)
		if (g_dma_list[i].mem == NULL)
			break;

	/* no luck */
	if (i >= MAX_DMA_LIST)
		return E_OUT_OF_MEMORY;

	/* malloc the requested size + enough space to page-align the memory */
	g_dma_list[i].mem = (Byte*)malloc(size + DMA_PAGE_SIZE - 1);

	if (g_dma_list[i].mem == NULL)
		return E_OUT_OF_MEMORY;

	/* page-align the memory */
	g_dma_list[i].dma = (Cell)(uPtr)g_dma_list[i].mem;
	g_dma_list[i].dma += DMA_PAGE_SIZE;
	g_dma_list[i].dma &= ~(DMA_PAGE_SIZE - 1);
	g_dma_list[i].size = size;

	/* and return the aligned DMA address */
	*virt = g_dma_list[i].dma;

	DPRINTF(("pci_dma_alloc: hbridge=%d size=%#Cx virt=%#Cx\n",
			hbridge, size, *virt));
	return NO_ERROR;
}

Retcode
pci_dma_free(Int hbridge, Cell virt, Cell size)
{
	int i;

	DPRINTF(("pci_dma_free: hbridge=%d virt=%#Cx size=%#Cx\n",
			hbridge, virt, size));

	if (hbridge)
		return E_INVALID_POINTER;

	/* try to find this entry in our list */
	for (i = 0; i < MAX_DMA_LIST; i++)
		if (g_dma_list[i].dma == virt)
			break;

	/* no luck */
	if (i >= MAX_DMA_LIST)
		return E_INVALID_POINTER;

	/* free the actual malloc memory and clear the entry */
	DPRINTF(("pci_dma_free: hbridge=%d i=%d mem=%p size=%#Cx\n",
			hbridge, i, g_dma_list[i].mem, size));
	free(g_dma_list[i].mem);
	g_dma_list[i].mem = NULL;
	g_dma_list[i].dma = (uPtr)NULL;
	return NO_ERROR;
}

Retcode
pci_dma_map_in(Int hbridge, Cell virt, Cell size, Int cacheable,
		Cell *devaddr)
{
	if (hbridge)
		return E_INVALID_POINTER;

	*devaddr = virt + PCI_HOST_MEM_BASE;
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

	__asm("wbinvd");
	return NO_ERROR;
}

Retcode
pci_init_addresses(Int hbridge, Pci_addresses *addrs)
{
	if (hbridge)
		return E_INVALID_POINTER;

	memset(addrs, 0, sizeof (Pci_addresses));
	pci_free_bus_range(addrs, 1, 255);

	//for (i = 0x1000; i < 0x10000; i += 0x1000)
	    //pci_free_io_range(addrs, i, 256);

	/* can only get to 16-bits of I/O space */
	pci_free_io_range(addrs, 0x1000, 0x10000 - 0x1000);
	pci_free_mem_range(addrs, 0, 0xA0000, 0, 0x60000);
	pci_free_mem_range(addrs, 0, 0x80000000, 0, 0x80000000);
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
	/* TODO */
	return E_NO_INTR_MAP;
}
