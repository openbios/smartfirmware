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

/* (c) Copyright 1996-1997,2000 by CodeGen, Inc.  All Rights Reserved. */

/* ISA bus access layer - used by isa.c */

#include "defs.h"
#include "isa.h"

Retcode
isabase_mem_read(uInt addr, void *value, int size)
{
	switch (size)
	{
	case 1:
		*(Byte*)value = *(Byte*)(uPtr)addr;
		break;
	
	case 2:
		*(Short*)value = *(uShort*)(uPtr)addr;
		break;
	
	case 4:
		*(Int*)value = *(uInt*)(uPtr)addr;
		break;
	}

	return NO_ERROR;
}

Retcode
isabase_mem_write(uInt addr, Int value, int size)
{
	switch (size)
	{
	case 1:
		*(uByte*)(uPtr)addr = (uByte)value;
		break;
	
	case 2:
		*(uShort*)(uPtr)addr = (uShort)value;
		break;
	
	case 4:
		*(uInt*)(uPtr)addr = (uInt)value;
		break;
	}

	return NO_ERROR;
}

Retcode
isabase_io_read(uInt addr, void *value, int size)
{
	switch (size)
	{
	case 1:
		__asm __volatile("inb %%dx,%0" : "=a" (*(uByte*)value) : "d" (addr));
		break;

	case 2:
		__asm __volatile("inw %%dx,%0" : "=a" (*(uShort*)value) : "d" (addr));
		break;

	case 4:
		__asm __volatile("inl %%dx,%0" : "=a" (*(uInt*)value) : "d" (addr));
		break;

	default:
		return E_BAD_ARGUMENT;
	}

	return NO_ERROR;
}

Retcode
isabase_io_write(uInt addr, Int value, int size)
{
	switch (size)
	{
	case 1:
		__asm __volatile("outb %0,%%dx" : : "a" ((uByte)value), "d" (addr));
		break;

	case 2:
		__asm __volatile("outw %0,%%dx" : : "a" ((uShort)value), "d" (addr));
		break;

	case 4:
		__asm __volatile("outl %0,%%dx" : : "a" ((uInt)value), "d" (addr));
		break;

	default:
		return E_BAD_ARGUMENT;
	}

	return NO_ERROR;
}

Retcode
isabase_map_in(Int physhi, Int physlo, Cell size, Cell *virt)
{
	if (physhi)
		return E_INVALID_POINTER;

	*virt = physlo;
	return NO_ERROR;
}

Retcode
isabase_map_out(Cell virt, Cell size)
{
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
isabase_dma_alloc(Cell size, Cell *virt)
{
	int i;

	DPRINTF(("isabase_dma_alloc: size=%#Cx virt=%p\n", size, virt));

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
	return NO_ERROR;
}

Retcode
isabase_dma_free(Cell virt, Cell size)
{
	int i;

	DPRINTF(("isabase_dma_free: virt=%#Cx size=%#Cx\n", virt, size));

	/* try to find this entry in our list */
	for (i = 0; i < MAX_DMA_LIST; i++)
		if (g_dma_list[i].dma == virt)
			break;

	/* no luck */
	if (i >= MAX_DMA_LIST)
		return E_INVALID_POINTER;

	/* free the actual malloc memory and clear the entry */
	free(g_dma_list[i].mem);
	g_dma_list[i].mem = NULL;
	g_dma_list[i].dma = (uPtr)NULL;
	return NO_ERROR;
}

Retcode
isabase_dma_map_in(Cell virt, Cell size, Int cacheable, Cell *devaddr)
{
	*devaddr = virt;
	return NO_ERROR;
}

Retcode
isabase_dma_map_out(Cell virt, Cell devaddr, Cell size)
{
	return NO_ERROR;
}

Retcode
isabase_dma_sync(Cell virt, Cell devaddr, Cell size)
{
	__asm("wbinvd");
	return NO_ERROR;
}

#if 0
/* machine-specific function pointers to access ISA bus */
Retcode (*isa_mem_read)(uInt addr, void *value, int size)
	= isabase_mem_read;
Retcode (*isa_mem_write)(uInt addr, Int value, int size)
	= isabase_mem_write;
Retcode (*isa_io_read)(uInt addr, void *value, int size)
	= isabase_io_read;
Retcode (*isa_io_write)(uInt addr, Int value, int size)
	= isabase_io_write;
Retcode (*isa_map_in)(Int physhi, Int physlo, Cell size, Cell *virt)
	= isabase_map_in;
Retcode (*isa_map_out)(Cell virt, Cell size)
	= isabase_map_out;
Retcode (*isa_dma_alloc)(Cell size, Cell *virt)
	= isabase_dma_alloc;
Retcode (*isa_dma_free)(Cell virt, Cell size)
	= isabase_dma_free;
Retcode (*isa_dma_map_in)(Cell virt, Cell size, Int cacheable, Cell *devaddr)
	= isabase_dma_map_in;
Retcode (*isa_dma_map_out)(Cell virt, Cell devaddr, Cell size)
	= isabase_dma_map_out;
Retcode (*isa_dma_sync)(Cell virt, Cell devaddr, Cell size)
	= isabase_dma_sync;
#endif

Retcode
install_isabase(Environ *e)
{
	isa_mem_read = isabase_mem_read;
	isa_mem_write = isabase_mem_write;
	isa_io_read = isabase_io_read;
	isa_io_write = isabase_io_write;
	isa_map_in = isabase_map_in;
	isa_map_out = isabase_map_out;
	isa_dma_alloc = isabase_dma_alloc;
	isa_dma_free = isabase_dma_free;
	isa_dma_map_in = isabase_dma_map_in;
	isa_dma_map_out = isabase_dma_map_out;
	isa_dma_sync = isabase_dma_sync;
	return NO_ERROR;
}
