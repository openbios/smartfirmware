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

/* (c) Copyright 1997 by CodeGen, Inc.  All Rights Reserved */

/*#define DEBUG*/

#include "defs.h"
#include "pci.h"

extern void pci_encode_reg_prop(Byte *arr, Int *len, Cell physhi, Cell physmid,
		Cell physlo, Cell sizehi, Cell sizelo);
extern Initentry pci_methods[];

extern void pci_bar_size(Environ *e, Pci_device_info *devinfo, Int reg,
		Int *ptype, Int *psizehi, Int *psizelo);

static Int
config_read(Pci_device_info *devinfo, Int addr)
{
	return pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, addr);
}

static void
config_write(Pci_device_info *devinfo, Int addr, Int value)
{
	pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, addr, value);
}

#ifdef DEBUG
static void
dump_ppb_registers(Environ *e, Pci_device_info *devinfo)
{
	int i;
	int j = 0;
	Int val;

	dprintf("PPB register dump:\n");

	for (i = 0; i < 0xFF; i += 4)
	{
		val = config_read(devinfo, i);
		dprintf("  %#02X: %#08X", i, val);

		if (j++ > 2)
		{
			dprintf("\n");
			j = 0;
		}
	}

	if (j)
		dprintf("\n");
}

#endif /* DEBUG */

Retcode
ppb_load_bridge_properties(Environ *e, Package *pkg)
{
	prop_set_str(pkg->props, "device_type", CSTR, "pci", CSTR);
	prop_set_int(pkg->props, "#address-cells", CSTR, 3);
	prop_set_int(pkg->props, "#size-cells", CSTR, 2);
	prop_set_int(pkg->props, "clock-frequency", CSTR, 33333333);
	return init_entries(e, pkg->dict, pci_methods);
}


#define PPB_DECODER_BUS		0
#define PPB_DECODER_IO		1
#define PPB_DECODER_MEM		2
#define PPB_DECODER_PMEM	3
#define PPB_DECODER_WINDOWS	4

#define PPB_DECODER_MASK_BUS8	0x0001
#define PPB_DECODER_MASK_IO16	0x0002
#define PPB_DECODER_MASK_IO32	0x0004
#define PPB_DECODER_MASK_MEM32	0x0008
#define PPB_DECODER_MASK_PMEM32	0x0010
#define PPB_DECODER_MASK_PMEM64	0x0020

#define PPB_DECODER_MASK_ALL	(PPB_DECODER_MASK_BUS8|PPB_DECODER_MASK_IO16|\
				PPB_DECODER_MASK_IO32|PPB_DECODER_MASK_MEM32|\
				PPB_DECODER_MASK_PMEM32|PPB_DECODER_MASK_PMEM64)

int
ppb_probe_address_decoders(Environ *e, Package *pkg, Pci_device_info *devinfo)
{
	int decoders;
	uInt reg;
	uInt reg2;
	uInt reg3;

	decoders = PPB_DECODER_MASK_BUS8;

	/* determine size of I/O decoder */
	reg = config_read(devinfo, PPB_CONFIG_IOBASE);
	config_write(devinfo, PPB_CONFIG_IOBASE, (reg & 0xFFFF0000) | 0xF0F0);
	reg2 = config_read(devinfo, PPB_CONFIG_IOBASE);
	config_write(devinfo, PPB_CONFIG_IOBASE, (reg & 0xFFFF0000));
	reg3 = config_read(devinfo, PPB_CONFIG_IOBASE);
	config_write(devinfo, PPB_CONFIG_IOBASE, reg);

	if ((reg2 & 0xF0F0) != 0xF0F0 || (reg3 & 0xF0F0) != 0)
		/* not present */
		DPRINTF(("ppb_probe_address_decoders: IO NP: reg2 = %#X, reg3 = %#X\n", reg2, reg3));
	else if ((reg2 & 0xF0F) == 0)
		decoders |= PPB_DECODER_MASK_IO16;	/* 16-bit */
	else if ((reg2& 0xF0F) == 0x101)
	{
		reg = config_read(devinfo, PPB_CONFIG_IOBASEH);
		config_write(devinfo, PPB_CONFIG_IOBASEH, 0xFFFFFFFF);
		reg2 = config_read(devinfo, PPB_CONFIG_IOBASEH);
		config_write(devinfo, PPB_CONFIG_IOBASEH, 0);
		reg3 = config_read(devinfo, PPB_CONFIG_IOBASEH);
		config_write(devinfo, PPB_CONFIG_IOBASEH, reg);

		if (reg2 == 0xFFFFFFFF && reg3 == 0)
			decoders |= PPB_DECODER_MASK_IO32;	/* 32-bit */
		else
			DPRINTF(("ppb_probe_address_decoders: IO32 NP: reg2 = %#X, reg3 = %#X\n", reg2, reg3));
	}
	else
		/* inconsistient or reserved decoder type */
		DPRINTF(("ppb_probe_address_decoders: IO: reg2 = %#X, reg3 = %#X\n", reg2, reg3));

	/* determine size of mem decoder */
	reg = config_read(devinfo, PPB_CONFIG_MEMBASE);
	config_write(devinfo, PPB_CONFIG_MEMBASE, 0xFFF0FFF0);
	reg2 = config_read(devinfo, PPB_CONFIG_MEMBASE);
	config_write(devinfo, PPB_CONFIG_MEMBASE, 0);
	reg3 = config_read(devinfo, PPB_CONFIG_MEMBASE);
	config_write(devinfo, PPB_CONFIG_MEMBASE, reg);

	if (reg2 == 0xFFF0FFF0 && reg3 == 0)
		decoders |= PPB_DECODER_MASK_MEM32;

	/* determine size of pmem decoder */
	reg = config_read(devinfo, PPB_CONFIG_PMEMBASE);
	config_write(devinfo, PPB_CONFIG_PMEMBASE, 0xFFF0FFF0);
	reg2 = config_read(devinfo, PPB_CONFIG_PMEMBASE);
	config_write(devinfo, PPB_CONFIG_PMEMBASE, 0);
	reg3 = config_read(devinfo, PPB_CONFIG_PMEMBASE);
	config_write(devinfo, PPB_CONFIG_PMEMBASE, reg);

	if (reg2 == 0xFFF0FFF0 && reg3 == 0)
		decoders |= PPB_DECODER_MASK_PMEM32;
	else if (reg2 == 0xFFF1FFF1 && reg3 == 0x10001)
	{
		reg = config_read(devinfo, PPB_CONFIG_PMEMBASEH);
		config_write(devinfo, PPB_CONFIG_PMEMBASEH, 0xFFFFFFFF);
		reg2 = config_read(devinfo, PPB_CONFIG_PMEMBASEH);
		config_write(devinfo, PPB_CONFIG_PMEMBASEH, 0);
		reg3 = config_read(devinfo, PPB_CONFIG_PMEMBASEH);
		config_write(devinfo, PPB_CONFIG_PMEMBASEH, reg);

		if (reg2 == 0xFFFFFFFF && reg3 == 0)
		{
			reg = config_read(devinfo, PPB_CONFIG_PMEMLIMITH);
			config_write(devinfo, PPB_CONFIG_PMEMLIMITH, 0xFFFFFFFF);
			reg2 = config_read(devinfo, PPB_CONFIG_PMEMLIMITH);
			config_write(devinfo, PPB_CONFIG_PMEMLIMITH, 0);
			reg3 = config_read(devinfo, PPB_CONFIG_PMEMLIMITH);
			config_write(devinfo, PPB_CONFIG_PMEMLIMITH, reg);

			if (reg2 == 0xFFFFFFFF && reg3 == 0)
				decoders |= PPB_DECODER_MASK_PMEM64;
		}
	}

	return decoders;
}

struct pci_address_window 
{
	uInt basehi;
	uInt baselo;
	uInt sizehi;
	uInt sizelo;
	uInt limithi;
	uInt limitlo;
	uInt pad[2];
};

typedef struct pci_address_window Pci_address_window;

void
ppb_set_window(Pci_address_window *window, uInt basehi, uInt baselo,
		uInt sizehi, uInt sizelo)
{
	window->baselo = baselo;
	window->basehi = basehi;
	window->sizelo = sizelo;
	window->sizehi = sizehi;
	window->limitlo = baselo + sizelo;
	window->limithi = basehi + sizehi + (window->limitlo < sizelo);
}

void
ppb_set_window_size(Pci_address_window *window, uInt align)
{
	uInt basehi = window->basehi;
	uInt baselo = window->baselo;
	uInt limithi = window->limithi;
	uInt limitlo = window->limitlo;
	uInt sizehi = limithi - basehi - (limitlo < baselo);
	uInt sizelo = limitlo - baselo;

	if (limithi < basehi || (limithi == basehi && limitlo < baselo))
	{
	    window->sizehi = sizehi;
	    window->sizelo = sizelo;
	    return;
	}

	align--;
	sizelo += align;
	sizehi += (sizelo < align);

	sizelo &= ~align;

	window->sizehi = sizehi;
	window->sizelo = sizelo;
}

Retcode
ppb_get_bridge_address_space(Environ *e, int decoders, Pci_addresses *avail,
		Pci_addresses *addrs, Pci_address_window *windows)
{
	uInt basehi;
	uInt baselo;
	uInt sizehi;
	uInt sizelo;

	/* clear out data */
	pci_init_pool(addrs);
	memset(windows, 0, sizeof *windows * PPB_DECODER_WINDOWS);
	ppb_set_window(&windows[PPB_DECODER_BUS], 0, 0, 0, 0);
	ppb_set_window(&windows[PPB_DECODER_IO], 0, 0, 0, 0);
	ppb_set_window(&windows[PPB_DECODER_MEM], 0, 0, 0, 0);
	ppb_set_window(&windows[PPB_DECODER_PMEM], 0, 0, 0, 0);

	/* find the biggest block available for bus numbers */
	if ((decoders & PPB_DECODER_MASK_BUS8) &&
			pci_find_largest_block(&avail->busaddrs, 0, 1, 0, 0, 0, 0xFF,
			0, &baselo, 0, &sizelo))
	{
		pci_free_bus_range(addrs, baselo, sizelo);
		ppb_set_window(&windows[PPB_DECODER_BUS], 0, baselo, 0, sizelo);
	}

	/* find the biggest block available for I/O 16 */
	if ((decoders & PPB_DECODER_MASK_IO16) &&
			pci_find_largest_block(&avail->ioaddrs, 0, 0x1000, 0, 0, 0, 0xFFFF,
			0, &baselo, 0, &sizelo))
	{
		pci_free_io_range(addrs, baselo, sizelo);
		ppb_set_window(&windows[PPB_DECODER_IO], 0, baselo, 0, sizelo);
	}

	/* find the biggest block available for I/O 32 */
	if ((decoders & PPB_DECODER_MASK_IO32) &&
			pci_find_largest_block(&avail->ioaddrs, 0, 0x1000, 0, 0x10000,
			0, 0xFFFFFFFF, 0, &baselo, 0, &sizelo))
	{
		pci_free_io_range(addrs, baselo, sizelo);
		ppb_set_window(&windows[PPB_DECODER_IO], 0, baselo, 0, sizelo);
	}

	/* find the biggest block available for memory 32 */
	if ((decoders & PPB_DECODER_MASK_MEM32) &&
			pci_find_largest_block(&avail->memaddrs, 0, 0x100000, 0, 0x100000,
			0, 0xFFFFFFFF, 0, &baselo, 0, &sizelo))
	{
		pci_free_mem_range(addrs, 0, baselo, 0, sizelo);
		ppb_set_window(&windows[PPB_DECODER_MEM], 0, baselo, 0, sizelo);
	}

	/* find the biggest block available for prefetch memory 32 */
	if ((decoders & PPB_DECODER_MASK_PMEM32) &&
			pci_find_largest_block(&avail->prefetchaddrs, 0, 0x100000,
			0, 0x100000, 0, 0xFFFFFFFF, 0, &baselo, 0, &sizelo))
	{
		pci_free_pmem_range(addrs, 0, baselo, 0, sizelo);
		ppb_set_window(&windows[PPB_DECODER_PMEM], 0, baselo, 0, sizelo);
	}

	/* find the biggest block available for prefetch memory 64 */
	if ((decoders & PPB_DECODER_MASK_PMEM64) &&
			pci_find_largest_block(&avail->prefetchaddrs, 0, 0x100000,
			0, 0x100000, 0xFFFFFFFF, 0xFFFFFFFF, &basehi, &baselo,
			&sizehi, &sizelo))
	{
		pci_free_pmem_range(addrs, basehi, baselo, sizehi, sizelo);
		ppb_set_window(&windows[PPB_DECODER_PMEM], basehi, baselo, sizehi,
				sizelo);
	}

	return NO_ERROR;
}

void
ppb_update_windows(Pci_addresses *addrs, Pci_address_window *windows)
{
	Pci_address_block *b;

	for (b = addrs->busaddrs; b; b = b->next)
	{
		uInt limit = b->baselo + b->sizelo;

		if (limit == windows[PPB_DECODER_BUS].limitlo)
			windows[PPB_DECODER_BUS].limitlo = b->baselo;
	}

	for (b = addrs->ioaddrs; b; b = b->next)
	{
		uInt limitlo = b->baselo + b->sizelo;
		uInt limithi = b->basehi + b->sizehi + (limitlo < b->sizelo);

		if (b->baselo == windows[PPB_DECODER_IO].baselo && 
				b->basehi == windows[PPB_DECODER_IO].basehi)
		{
			DPRINTF(("ppb: adjusting I/O limit\n"));
			windows[PPB_DECODER_IO].baselo = limitlo;
			windows[PPB_DECODER_IO].basehi = limithi;
		}

		if (limitlo == windows[PPB_DECODER_IO].limitlo && 
				limithi == windows[PPB_DECODER_IO].limithi)
		{
			DPRINTF(("ppb: adjusting I/O limit\n"));
			windows[PPB_DECODER_IO].limitlo = b->baselo;
			windows[PPB_DECODER_IO].limithi = b->basehi;
		}
	}

	for (b = addrs->memaddrs; b; b = b->next)
	{
		uInt limitlo = b->baselo + b->sizelo;
		uInt limithi = b->basehi + b->sizehi + (limitlo < b->sizelo);

		if (b->baselo == windows[PPB_DECODER_MEM].baselo && 
				b->basehi == windows[PPB_DECODER_MEM].basehi)
		{
			DPRINTF(("ppb: adjusting mem limit\n"));
			windows[PPB_DECODER_MEM].baselo = limitlo;
			windows[PPB_DECODER_MEM].basehi = limithi;
		}

		if (limitlo == windows[PPB_DECODER_MEM].limitlo && 
				limithi == windows[PPB_DECODER_MEM].limithi)
		{
			DPRINTF(("ppb: adjusting mem limit\n"));
			windows[PPB_DECODER_MEM].limitlo = b->baselo;
			windows[PPB_DECODER_MEM].limithi = b->basehi;
		}
	}

	for (b = addrs->prefetchaddrs; b; b = b->next)
	{
		uInt limitlo = b->baselo + b->sizelo;
		uInt limithi = b->basehi + b->sizehi + (limitlo < b->sizelo);

		if (b->baselo == windows[PPB_DECODER_PMEM].baselo && 
				b->basehi == windows[PPB_DECODER_PMEM].basehi)
		{
			DPRINTF(("ppb: adjusting pmem limit\n"));
			windows[PPB_DECODER_PMEM].baselo = limitlo;
			windows[PPB_DECODER_PMEM].basehi = limithi;
		}

		if (limitlo == windows[PPB_DECODER_PMEM].limitlo && 
				limithi == windows[PPB_DECODER_PMEM].limithi)
		{
			DPRINTF(("ppb: adjusting pmem limit\n"));
			windows[PPB_DECODER_PMEM].limitlo = b->baselo;
			windows[PPB_DECODER_PMEM].limithi = b->basehi;
		}
	}

	ppb_set_window_size(&windows[PPB_DECODER_BUS], 1);
	ppb_set_window_size(&windows[PPB_DECODER_IO], 0x1000);
	ppb_set_window_size(&windows[PPB_DECODER_MEM], 0x100000);
	ppb_set_window_size(&windows[PPB_DECODER_PMEM], 0x100000);
}

void
ppb_allocate_window(Pci_address_block **blocks, Pci_address_window *window)
{
	if (window->sizehi || window->sizelo)
		pci_allocate_fixed_block(blocks, window->basehi, window->baselo,
				window->sizehi, window->sizelo);
}

void
ppb_allocate_windows(Pci_addresses *addrs, Pci_address_window *windows)
{
	ppb_allocate_window(&addrs->busaddrs, &windows[PPB_DECODER_BUS]);
	ppb_allocate_window(&addrs->ioaddrs, &windows[PPB_DECODER_IO]);
	ppb_allocate_window(&addrs->memaddrs, &windows[PPB_DECODER_MEM]);
	ppb_allocate_window(&addrs->prefetchaddrs, &windows[PPB_DECODER_PMEM]);
}

void
ppb_program_windows(Pci_device_info *devinfo, int decoders,
		Pci_address_window *windows)
{
	uInt pri_bus;
	uInt sec_bus;
	uInt sub_bus;
	uInt io_base;
	uInt io_limit;
	uInt io32_base;
	uInt io32_limit;
	uInt mem_base;
	uInt mem_limit;
	uInt pmem_base;
	uInt pmem_limit;
	uInt xmem_base;
	uInt xmem_limit;

	/* init bus nums */
	pri_bus = devinfo->bus;
	sec_bus = windows[PPB_DECODER_BUS].baselo;
	sub_bus = windows[PPB_DECODER_BUS].limitlo - 1;

	if (windows[PPB_DECODER_IO].sizelo || windows[PPB_DECODER_IO].sizehi)
	{
		io_base = (windows[PPB_DECODER_IO].baselo >> 8) & 0xF0;
		io_limit = ((windows[PPB_DECODER_IO].limitlo - 1) >> 8) & 0xF0;
		io32_base = (windows[PPB_DECODER_IO].baselo >> 16) & 0xFFFF;
		io32_limit = ((windows[PPB_DECODER_IO].limitlo - 1) >> 16) & 0xFFFF;
	}
	else
	{
		io_base = 0xF0;
		io_limit = 0xE0;
		io32_base = 0xFFFF;
		io32_limit = 0xFFFF;
	}


	if (windows[PPB_DECODER_MEM].sizelo || windows[PPB_DECODER_MEM].sizehi)
	{
		mem_base = (windows[PPB_DECODER_MEM].baselo >> 16) & 0xFFF0;
		mem_limit = ((windows[PPB_DECODER_MEM].limitlo - 1) >> 16) & 0xFFF0;
	}
	else
	{
		mem_base = 0xFFF0;
		mem_limit = 0xFFE0;
	}

	if (windows[PPB_DECODER_PMEM].sizelo || windows[PPB_DECODER_PMEM].sizehi)
	{
		pmem_base = (windows[PPB_DECODER_PMEM].baselo >> 16) & 0xFFF0;
		pmem_limit = ((windows[PPB_DECODER_PMEM].limitlo - 1) >> 16) & 0xFFF0;
		xmem_base = windows[PPB_DECODER_PMEM].basehi;
		xmem_limit = windows[PPB_DECODER_PMEM].limithi -
				(windows[PPB_DECODER_PMEM].limitlo == 0);
	}
	else
	{
		pmem_base = 0xFFF0;
		pmem_limit = 0xFFE0;
		xmem_base = 0xFFFFFFFF;
		xmem_limit = 0xFFFFFFFF;
	}

	config_write(devinfo, PPB_CONFIG_BUSNUM, (sub_bus << 16) | (sec_bus << 8) |
			pri_bus);
	config_write(devinfo, PPB_CONFIG_IOBASE, (io_limit << 8) | io_base);
	config_write(devinfo, PPB_CONFIG_MEMBASE, (mem_limit << 16) | mem_base);
	config_write(devinfo, PPB_CONFIG_PMEMBASE, (pmem_limit << 16) | pmem_base);
	config_write(devinfo, PPB_CONFIG_PMEMBASEH, xmem_base);
	config_write(devinfo, PPB_CONFIG_PMEMLIMITH, xmem_limit);
	config_write(devinfo, PPB_CONFIG_IOBASEH, (io32_limit << 16) | io32_base);
}

void
pci_encode_range_prop(Byte *arr, Int *len,
		Cell physhi, Cell physmid, Cell physlo,
		Cell sizehi, Cell sizelo)
{
	Int l = *len;
	
	/* child addr */
	prop_encode_int(&arr[l], &l, physhi);
	prop_encode_int(&arr[l], &l, physmid);
	prop_encode_int(&arr[l], &l, physlo);

	/* parent addr is the same since it is a 1-1 map */
	prop_encode_int(&arr[l], &l, physhi);
	prop_encode_int(&arr[l], &l, physmid);
	prop_encode_int(&arr[l], &l, physlo);

	/* size of map */
	prop_encode_int(&arr[l], &l, sizehi);
	prop_encode_int(&arr[l], &l, sizelo);
	*len = l;
}

Retcode
ppb_probe_pci_bridge(Environ *e, Package *pkg, Pci_device_info *devinfo,
		Pci_addresses *addrs)
{
	Pci_addresses addrwindows;
	Pci_address_window windows[PPB_DECODER_WINDOWS];
	Retcode ret;
	Package *savepkg;
	uInt pri_bus;
	uInt sec_bus;
	uInt sub_bus;
	int decoders;
	uInt cmd;
	Byte *busprops;
	Int busplen;
	Byte *props;
	Int plen;
#ifdef ISA_BEHIND_PPB
	int isablock = 0;
#endif

	/* configure standard bridge registers and allocate base addresses */
	DPRINTF(("ppb: ppb_probe_pci_bridge(0x%X, 0x%X, 0x%X, 0x%X)\n", e, pkg, devinfo, addrs));
	cmd = config_read(devinfo, PPB_CONFIG_COMMAND);
	cmd &= ~(PCI_COMMAND_IOENABLE|PCI_COMMAND_MEMENABLE|
			PCI_COMMAND_MASTERENABLE);
	config_write(devinfo, PPB_CONFIG_COMMAND, cmd);

	ppb_load_bridge_properties(e, pkg);
	pci_load_reg_and_name_props(e, pkg, devinfo);

	savepkg = e->currpkg;
	e->currpkg = pkg;
	ret = execute_word(e, "assign-addresses");
	e->currpkg = savepkg;

	if (ret != NO_ERROR)
		return ret;

	decoders = ppb_probe_address_decoders(e, pkg, devinfo);

	DPRINTF(("ppb: ppb_probe_pci_bridge: decoders = %#X\n", decoders));

	ppb_get_bridge_address_space(e, decoders, addrs, &addrwindows,
			windows);

	/* load up the PCI-PCI bridge */
	ppb_program_windows(devinfo, decoders, windows);

	pri_bus = devinfo->bus;
	sec_bus = pci_allocate_buses(&addrwindows, 1);
	sub_bus = windows[PPB_DECODER_BUS].limitlo - 1;

	if (!sec_bus)
		return E_OUT_OF_MEMORY;

	config_write(devinfo, PPB_CONFIG_COMMAND, cmd | PCI_COMMAND_IOENABLE|
			PCI_COMMAND_MEMENABLE|PCI_COMMAND_MASTERENABLE);

	/* patches for various broken chipsets */
	/* TI bridge needs secondary-to-primary write-posting (1) turned off,
	   and errata says posted-write reconnect (5) needs to be turned off too */
	if (devinfo->vendid == 0x104C && devinfo->devid == 0xAC21 &&
			devinfo->revisionid == 0x01)
		config_write(devinfo, 0x6C, 0x00400D00);

	/* setup bus-range property */
	DPRINTF(("ppb: ppb_probe_pci_bridge: sec_bus = %d\n", sec_bus));
	busplen = 0;
	busprops = prop_alloc(e, 2 * sizeof (Int));

	if (busprops == NULL)
		return E_OUT_OF_MEMORY;

	prop_encode_int(busprops + busplen, &busplen, sec_bus);
	prop_encode_int(busprops + busplen, &busplen, sub_bus);
	ret = add_property(pkg->props, "bus-range", CSTR, busprops, busplen);

	if (ret != NO_ERROR)
		return ret;

	/* create "ranges" property for all mapping registers - per range:
	   physhi..lo of child bus, physhi..lo of parent, sizehi..lo of child
	 */
	plen = 0;
	props = prop_alloc(e, 3 * 8 * sizeof (Int));

	if (props == NULL)
		return E_OUT_OF_MEMORY;

#ifdef DEBUG
	dump_ppb_registers(e, devinfo);
#endif

#ifdef ISA_BEHIND_PPB
	/* attempt to allocate the 16MB region starting at FD00.0000 for the */
	/* PCI-ISA bus bridge that may be behind us. */
	if (pci_allocate_fixed_block(&addrwindows.memaddrs, 0, 0xFD000000,
			0, 0x1000000))
		isablock = 1;
#endif

	DPRINTF(("ppb: ppb_probe_pci_bridge: probing secondary bus\n"));
	ret = pci_probe_bus(e, pkg, devinfo->hbridge, sec_bus, &addrwindows, 32);
	DPRINTF(("ppb: ppb_probe_pci_bridge: ret = %d\n", ret));

	if (ret != NO_ERROR)
		return ret;

#ifdef ISA_BEHIND_PPB
	if (isablock)
	{
		int found = 0;
		/* search our children for an ISA-bridge or a PCI-PCI bus bridge that */
		/* claimed the ISA-bridge address space */
		found = 1;		/* assume there is one for now */

		if (!found)
			pci_free_mem_range(&addrwindows, 0, 0xFD000000, 0, 0x1000000);
	}
#endif

	ppb_update_windows(&addrwindows, windows);
	config_write(devinfo, PPB_CONFIG_COMMAND, cmd);

	sub_bus = windows[PPB_DECODER_BUS].limitlo - 1;

	/* load up the PCI-PCI bridge with the updated windows */
	ppb_program_windows(devinfo, decoders, windows);

	config_write(devinfo, PPB_CONFIG_COMMAND, cmd | PCI_COMMAND_IOENABLE|
			PCI_COMMAND_MEMENABLE|PCI_COMMAND_MASTERENABLE);

	if (windows[PPB_DECODER_IO].sizelo || windows[PPB_DECODER_IO].sizehi)
		pci_encode_range_prop(props, &plen,
			PCI_PHYSHI_MK(0, 0, 0, PCI_SPACE_IO, 0, 0, 0, 0),
			windows[PPB_DECODER_IO].basehi, windows[PPB_DECODER_IO].baselo,
			windows[PPB_DECODER_IO].sizehi, windows[PPB_DECODER_IO].sizelo);

	if (windows[PPB_DECODER_MEM].sizelo || windows[PPB_DECODER_MEM].sizehi)
		pci_encode_range_prop(props, &plen,
			PCI_PHYSHI_MK(0, 0, 0, PCI_SPACE_MEM, 0, 0, 0, 0),
			windows[PPB_DECODER_MEM].basehi, windows[PPB_DECODER_MEM].baselo,
			windows[PPB_DECODER_MEM].sizehi, windows[PPB_DECODER_MEM].sizelo);

	if (windows[PPB_DECODER_PMEM].sizelo || windows[PPB_DECODER_PMEM].sizehi)
		pci_encode_range_prop(props, &plen,
			PCI_PHYSHI_MK(0, 0, 0, PCI_SPACE_XMEM, 0, 0, 0, 0),
			windows[PPB_DECODER_PMEM].basehi, windows[PPB_DECODER_PMEM].baselo,
			windows[PPB_DECODER_PMEM].sizehi, windows[PPB_DECODER_PMEM].sizelo);

#ifdef DEBUG
	dump_ppb_registers(e, devinfo);
#endif

	/* add ranges property */
	ret = add_property(pkg->props, "ranges", CSTR, props, plen);

	if (ret != NO_ERROR)
		return ret;

	/* update 2nd number in bus-range property to correct sub-bus num */
	busplen = sizeof (Int);
	prop_encode_int(busprops + sizeof (Int), &busplen, sub_bus);

	ppb_allocate_windows(addrs, windows);
	pci_destroy_pool(&addrwindows);

	/* Build assigned address property */
	/* It may partially exist from previous call to assign-address */

	return NO_ERROR;
}

#ifdef DEBUG
void
ppb_dump_regs(Environ *e, Package *cpkg, Pci_device_info *devinfo)
{
    uInt busnum = config_read(devinfo, PPB_CONFIG_BUSNUM);
    uInt iobase = config_read(devinfo, PPB_CONFIG_IOBASE);
    uInt membase = config_read(devinfo, PPB_CONFIG_MEMBASE);
    uInt pmembase = config_read(devinfo, PPB_CONFIG_PMEMBASE);
    uInt pmembaseh = config_read(devinfo, PPB_CONFIG_PMEMBASEH);
    uInt pmemlimith = config_read(devinfo, PPB_CONFIG_PMEMLIMITH);
    uInt iobaseh = config_read(devinfo, PPB_CONFIG_IOBASEH);
    uInt cmd = config_read(devinfo, PPB_CONFIG_COMMAND);

    DPRINTF(("ppb: pri bus %d, sec bus %d, sub bus %d, cmd 0x%08X\n",
			busnum & 0xFF, (busnum >> 8) & 0xFF, (busnum >> 16) & 0xFF, cmd));
    DPRINTF(("\tI/O range 0x%04X.%02X00 .. 0x%04X.%02XFF\n", iobaseh & 0xFFFF,
			iobase & 0xF0, (iobaseh >> 16) & 0xFFFF,
			((iobase >> 8) | 0xF) & 0xFF));
    DPRINTF(("\tmem range 0x%04X.0000 .. 0x%04X.FFFF\n", membase & 0xFFF0,
			((membase >> 16) | 0xF) & 0xFFFF));
    DPRINTF(("\tpmem range 0x%04X.%04X.%04X.0000 .. 0x%04X.%04X.%04X.FFFF\n",
			(pmembaseh >> 16) & 0xFFFF, pmembaseh & 0xFFFF, pmembase & 0xFFF0,
			(pmemlimith >> 16) & 0xFFFF, pmemlimith & 0xFFFF,
			((pmembase >> 16) | 0xF) & 0xFFFF));
    DPRINTF(("PMEMBASE = 0x%X\n", pmembase));
}
#endif

/* PCI-PCI bridge */
Retcode
ppb_probe(Environ *e, Package *cpkg, Pci_device_info *devinfo,
		Pci_addresses *addrs)
{
	Retcode err;
	DPRINTF(("ppb: pci_probe_header1(0x%X, 0x%X, 0x%X, 0x%X)\n", e, cpkg, devinfo, addrs));

	/* attempt to load fcode from the expansion ROM */
	err = pci_load_expansion_rom(e, cpkg, devinfo, addrs);

	if (err == NO_ERROR)
		return NO_ERROR;

	DPRINTF(("ppb: pci_probe_header1: no expansion ROM\n"));

	/* attempt to load a builtin device driver */
	err = pci_load_builtin_driver(e, cpkg, devinfo, addrs);

	if (err == NO_ERROR)
		return NO_ERROR;

	DPRINTF(("ppb: pci_probe_header1: no built-in driver\n"));

	/* no luck with the device.  Give it a generic configuration. */
	err = ppb_probe_pci_bridge(e, cpkg, devinfo, addrs);
#ifdef DEBUG
	ppb_dump_regs(e, cpkg, devinfo);
#endif
	return err;
}

static void
ppb_get_extended_device_info(Environ *e, Pci_device_info *devinfo)
{
	devinfo->subsysvendid = 0;
	devinfo->subsysdevid = 0;
}

static Retcode
ppb_rom_size(Environ *e, Package *pkg, Pci_device_info *devinfo, 
		uInt *size)
{
	Int save;
	Int set;
	Int sz;

	save = config_read(devinfo, PPB_CONFIG_ROMADDR);
	config_write(devinfo, PPB_CONFIG_ROMADDR, 0xFFFFFFFE);
	set = config_read(devinfo, PPB_CONFIG_ROMADDR);
	config_write(devinfo, PPB_CONFIG_ROMADDR, save);

	/* calculate size of expansion ROM BAR */
	sz = (~set | 0x7FF) + 1;
	sz &= -sz;		/* clear any un-programable upper bits */

	if (size)
		*size = sz;

	return NO_ERROR;
}

static Retcode
ppb_write_rom_address(Environ *e, Package *pkg, Pci_device_info *devinfo,
		uInt addr)
{
	if (addr)
		config_write(devinfo, PCI_CONFIG_ROMADDR, addr | 1);
	else
		config_write(devinfo, PCI_CONFIG_ROMADDR, 0);

	return NO_ERROR;
}

static Retcode
ppb_address_offset_list(Environ *e, Package *pkg, Pci_device_info *devinfo,
		uChar **offsets, int *numoffsets)
{
	static uChar offsetlist[] =
	{ 
		PPB_CONFIG_BAR0,
		PPB_CONFIG_BAR1,
		PPB_CONFIG_ROMADDR,
	};

	if (offsets)
		*offsets = offsetlist;

	if (numoffsets)
		*numoffsets = sizeof offsetlist / sizeof offsetlist[0];

	return NO_ERROR;
}

static Retcode
ppb_get_address_info(Environ *e, Package *pkg, Pci_device_info *devinfo,
		int reg, uInt *physhi, uInt *sizehi, uInt *sizelo)
{
#ifdef DEBUG_FULL
	DPRINTF(("ppb_get_address_info: e %#X, pkg %#X, devinfo %#X, reg %#X, physhi %#X, sizehi %#X, sizelo %#X\n", e, pkg, devinfo, reg, physhi, sizehi, sizelo));
#endif

	if (reg >= PPB_CONFIG_BAR0 && reg <= PPB_CONFIG_BAR1)
	{
		int p = 0;
		int t = 0;
		int space = 0;
		Int type = 0;

		pci_bar_size(e, devinfo, reg, &type, sizehi, sizelo);

		if (type & 1)
		{
			space = PCI_SPACE_IO;		/* i/o space */
			t = (type >> 1) & 1;
		}
		else
		{
			/* memory space */
			p = (type >> 3) & 1;
			space = PCI_SPACE_MEM;

			switch (type & 6)
			{
			case 0:		/* 32 bit BAR */
				break;
			case 2:		/* 32 bit BAR in lower 1-meg */
				t = 1;
				break;
			case 4:		/* 64 bit BAR */
				space = PCI_SPACE_XMEM;
				break;
			case 6:		/* reserved */
				if (physhi)
					*physhi = 0;

				if (sizehi)
					*sizehi = 0;

				if (sizelo)
					*sizelo = 0;

				return NO_ERROR;
			}
		}

		if (physhi)
			*physhi = PCI_PHYSHI_MK(0, p, t, space, devinfo->bus, devinfo->dev,
					devinfo->func, reg);

		return NO_ERROR;
	}

	if (reg == PPB_CONFIG_ROMADDR)
	{
		/* check the for an expansion rom */
		ppb_rom_size(e, pkg, devinfo, sizelo);

		if (physhi)
			*physhi = PCI_PHYSHI_MK(0, 0, 0, PCI_SPACE_MEM, devinfo->bus,
					devinfo->dev, devinfo->func, PCI_CONFIG_ROMADDR);

		if (sizehi)
			*sizehi = 0;

		return NO_ERROR;
	}

	if (physhi)
		*physhi = 0;

	if (sizehi)
		*sizehi = 0;

	if (sizelo)
		*sizelo = 0;

	return NO_ERROR;
}

static Retcode
ppb_write_address(Environ *e, Package *pkg, Pci_device_info *devinfo,
		int reg, uInt physhi, uInt physmid, uInt physlo)
{
	if (reg == PPB_CONFIG_ROMADDR)
		return ppb_write_rom_address(e, pkg, devinfo, physlo);

	switch (PCI_PHYSHI_SPACE(physhi))
	{
	case PCI_SPACE_IO:
	case PCI_SPACE_MEM:
		config_write(devinfo, reg, physlo);
		break;
	case PCI_SPACE_XMEM:
		config_write(devinfo, reg, physlo);
		config_write(devinfo, reg + 4, physmid);
	}

	return NO_ERROR;
}

Pci_header_methods pci_header1_methods =
{
	ppb_get_extended_device_info,
	ppb_probe,
	ppb_rom_size,
	ppb_write_rom_address,
	ppb_address_offset_list,
	ppb_get_address_info,
	ppb_write_address,
};


#if 0
BRIDGE PROBING PROCESS
=======================
1)		Traverse device tree
		a)	on way down build the device node tree and reg properties
		b)	on way up 

1)		Probe all devices on the bus
2)		Assign address space for all devices on the bus
		When assigning addresses to a bridge 
3)	

#endif
