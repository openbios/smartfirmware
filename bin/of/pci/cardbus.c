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
extern void pci_bar_size(Environ *e, Pci_device_info *devinfo, Int reg,
		Int *ptype, Int *psizehi, Int *psizelo);
extern Initentry pci_methods[];

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

Retcode
cardbus_load_bridge_properties(Environ *e, Package *pkg)
{
	prop_set_str(pkg->props, "device_type", CSTR, "pci", CSTR);
	prop_set_int(pkg->props, "#address-cells", CSTR, 3);
	prop_set_int(pkg->props, "#size-cells", CSTR, 2);
	prop_set_int(pkg->props, "clock-frequency", CSTR, 33333333);
	return init_entries(e, pkg->dict, pci_methods);
}

#define CARDBUS_DECODER_BUS			0
#define CARDBUS_DECODER_IO0			1
#define CARDBUS_DECODER_IO1			2
#define CARDBUS_DECODER_MEM0		3
#define CARDBUS_DECODER_MEM1		4
#define CARDBUS_DECODER_WINDOWS		5

#define CARDBUS_DECODER_MASK_BUS8	0x0001
#define CARDBUS_DECODER_MASK_IO016	0x0002
#define CARDBUS_DECODER_MASK_IO032	0x0004
#define CARDBUS_DECODER_MASK_IO116	0x0008
#define CARDBUS_DECODER_MASK_IO132	0x0010
#define CARDBUS_DECODER_MASK_MEM032	0x0020
#define CARDBUS_DECODER_MASK_MEM132	0x0040

#define CARDBUS_DECODER_MASK_ALL	(CARDBUS_DECODER_MASK_BUS8|\
				CARDBUS_DECODER_MASK_IO016|CARDBUS_DECODER_MASK_IO032|\
				CARDBUS_DECODER_MASK_IO116|CARDBUS_DECODER_MASK_IO132|\
				CARDBUS_DECODER_MASK_MEM032|CARDBUS_DECODER_MASK_MEM132)

int
cardbus_probe_address_decoders(Environ *e, Package *pkg,
		Pci_device_info *devinfo)
{
	int decoders;
	uInt reg;
	uInt reg2;
	uInt reg3;

	decoders = CARDBUS_DECODER_MASK_BUS8;

	/* determine size of the I/O 0 decoder */
	reg = config_read(devinfo, CARDBUS_CONFIG_IOBASE0);
	config_write(devinfo, CARDBUS_CONFIG_IOBASE0, 0xFFFFFFFF);
	reg2 = config_read(devinfo, CARDBUS_CONFIG_IOBASE0);
	config_write(devinfo, CARDBUS_CONFIG_IOBASE0, 0x00000000);
	reg3 = config_read(devinfo, CARDBUS_CONFIG_IOBASE0);
	config_write(devinfo, CARDBUS_CONFIG_IOBASE0, reg);

	if ((reg2 & 0xFFFC) != 0xFFFC || (reg3 & 0xFFFC) != 0)
		/* not present */
		DPRINTF(("cardbus_probe_address_decoders: I/O 0 NP: reg2 = %#X, reg3 = %#X\n", reg2, reg3));
	else if ((reg2 & 0x3) == 0)
		decoders |= CARDBUS_DECODER_MASK_IO016;	/* 16-bit */
	else if ((reg2 & 0x3) == 1)
	{
		if ((reg2 & 0xFFFF0000) == 0xFFFF0000)
			decoders |= CARDBUS_DECODER_MASK_IO032;	/* 32-bit */
		else if ((reg2 & 0xFFFF0000) == 0x00000000)
			decoders |= CARDBUS_DECODER_MASK_IO016;	/* 16-bit */
		else
			DPRINTF(("cardbus_probe_address_decoders: I/O 0 32 NP: reg2 = %#X, reg3 = %#X\n", reg2, reg3));
	}
	else
		/* inconsistient or reserved decoder type */
		DPRINTF(("cardbus_probe_address_decoders: I/O 0: reg2 = %#X, reg3 = %#X\n", reg2, reg3));

	if (decoders & (CARDBUS_DECODER_MASK_IO016|CARDBUS_DECODER_MASK_IO032))
	{
		/* determine size of I/O decoder */
		reg = config_read(devinfo, CARDBUS_CONFIG_IOLIMIT0);
		config_write(devinfo, CARDBUS_CONFIG_IOLIMIT0, 0xFFFFFFFF);
		reg2 = config_read(devinfo, CARDBUS_CONFIG_IOLIMIT0);
		config_write(devinfo, CARDBUS_CONFIG_IOLIMIT0, 0x00000000);
		reg3 = config_read(devinfo, CARDBUS_CONFIG_IOLIMIT0);
		config_write(devinfo, CARDBUS_CONFIG_IOLIMIT0, reg);

		if (decoders & CARDBUS_DECODER_MASK_IO016)
		{
			if ((reg2 & 0xFFFC) != 0xFFFC || (reg3 & 0xFFFC) != 0)
			{
				DPRINTF(("cardbus_probe_address_decoders: I/O 0 16 limit: reg2 = %#X, reg3 = %#X\n", reg2, reg3));
				decoders &= ~CARDBUS_DECODER_MASK_IO016;
			}
		}
		else
		{
			if ((reg2 & 0xFFFFFFFC) != 0xFFFFFFFC || (reg3 & 0xFFFFFFFF) != 1)
			{
				DPRINTF(("cardbus_probe_address_decoders: I/O 0 32 limit: reg2 = %#X, reg3 = %#X\n", reg2, reg3));
				decoders &= ~CARDBUS_DECODER_MASK_IO032;
			}
		}
	}

	/* determine size of the I/O 1 decoder */
	reg = config_read(devinfo, CARDBUS_CONFIG_IOBASE1);
	config_write(devinfo, CARDBUS_CONFIG_IOBASE1, 0xFFFFFFFF);
	reg2 = config_read(devinfo, CARDBUS_CONFIG_IOBASE1);
	config_write(devinfo, CARDBUS_CONFIG_IOBASE1, 0x00000000);
	reg3 = config_read(devinfo, CARDBUS_CONFIG_IOBASE1);
	config_write(devinfo, CARDBUS_CONFIG_IOBASE1, reg);

	if ((reg2 & 0xFFFC) != 0xFFFC || (reg3 & 0xFFFC) != 0)
		/* not present */
		DPRINTF(("cardbus_probe_address_decoders: I/O 1 NP: reg2 = %#X, reg3 = %#X\n", reg2, reg3));
	else if ((reg2 & 0x3) == 0)
		decoders |= CARDBUS_DECODER_MASK_IO116;	/* 16-bit */
	else if ((reg2 & 0x3) == 1)
	{
		if ((reg2 & 0xFFFF0000) == 0xFFFF0000)
			decoders |= CARDBUS_DECODER_MASK_IO132;	/* 32-bit */
		else if ((reg2 & 0xFFFF0000) == 0x00000000)
			decoders |= CARDBUS_DECODER_MASK_IO116;	/* 16-bit */
		else
			DPRINTF(("cardbus_probe_address_decoders: I/O 1 32 NP: reg2 = %#X, reg3 = %#X\n", reg2, reg3));
	}
	else
		/* inconsistient or reserved decoder type */
		DPRINTF(("cardbus_probe_address_decoders: I/O 1: reg2 = %#X, reg3 = %#X\n", reg2, reg3));

	if (decoders & (CARDBUS_DECODER_MASK_IO116|CARDBUS_DECODER_MASK_IO132))
	{
		/* determine size of I/O decoder */
		reg = config_read(devinfo, CARDBUS_CONFIG_IOLIMIT1);
		config_write(devinfo, CARDBUS_CONFIG_IOLIMIT1, 0xFFFFFFFF);
		reg2 = config_read(devinfo, CARDBUS_CONFIG_IOLIMIT1);
		config_write(devinfo, CARDBUS_CONFIG_IOLIMIT1, 0x00000000);
		reg3 = config_read(devinfo, CARDBUS_CONFIG_IOLIMIT1);
		config_write(devinfo, CARDBUS_CONFIG_IOLIMIT1, reg);

		if (decoders & CARDBUS_DECODER_MASK_IO116)
		{
			if ((reg2 & 0xFFFC) != 0xFFFC || (reg3 & 0xFFFC) != 0)
			{
				DPRINTF(("cardbus_probe_address_decoders: I/O 1 16 limit: reg2 = %#X, reg3 = %#X\n", reg2, reg3));
				decoders &= ~CARDBUS_DECODER_MASK_IO116;
			}
		}
		else
		{
			if ((reg2 & 0xFFFFFFFC) != 0xFFFFFFFC || (reg3 & 0xFFFFFFFF) != 1)
			{
				DPRINTF(("cardbus_probe_address_decoders: I/O 1 32 limit: reg2 = %#X, reg3 = %#X\n", reg2, reg3));
				decoders &= ~CARDBUS_DECODER_MASK_IO132;
			}
		}
	}

	/* determine size of mem 0 decoder */
	reg = config_read(devinfo, CARDBUS_CONFIG_MEMBASE0);
	config_write(devinfo, CARDBUS_CONFIG_MEMBASE0, 0xFFFFFFFF);
	reg2 = config_read(devinfo, CARDBUS_CONFIG_MEMBASE0);
	config_write(devinfo, CARDBUS_CONFIG_MEMBASE0, 0x00000000);
	reg3 = config_read(devinfo, CARDBUS_CONFIG_MEMBASE0);
	config_write(devinfo, CARDBUS_CONFIG_MEMBASE0, reg);

	if (reg2 == 0xFFFFF000 && reg3 == 0)
		decoders |= CARDBUS_DECODER_MASK_MEM032;

	/* determine size of mem 1 decoder */
	reg = config_read(devinfo, CARDBUS_CONFIG_MEMBASE1);
	config_write(devinfo, CARDBUS_CONFIG_MEMBASE1, 0xFFFFFFFF);
	reg2 = config_read(devinfo, CARDBUS_CONFIG_MEMBASE1);
	config_write(devinfo, CARDBUS_CONFIG_MEMBASE1, 0x00000000);
	reg3 = config_read(devinfo, CARDBUS_CONFIG_MEMBASE1);
	config_write(devinfo, CARDBUS_CONFIG_MEMBASE1, reg);

	if (reg2 == 0xFFFFF000 && reg3 == 0)
		decoders |= CARDBUS_DECODER_MASK_MEM132;

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
cardbus_set_window(Pci_address_window *window, uInt basehi, uInt baselo,
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
cardbus_set_window_size(Pci_address_window *window, uInt align)
{
	uInt basehi = window->basehi;
	uInt baselo = window->baselo;
	uInt limithi = window->limithi;
	uInt limitlo = window->limitlo;
	uInt sizehi = limithi - basehi - (limitlo < baselo);
	uInt sizelo = limitlo - baselo;

	if (limithi < basehi || (limithi == basehi && limitlo < baselo))
	{
		window->sizehi = 0;
		window->sizelo = 0;
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
cardbus_get_bridge_address_space(Environ *e, int decoders, Pci_addresses *avail,
		Pci_addresses *addrs, Pci_address_window *windows)
{
	uInt baselo;
	uInt sizelo;

	/* clear out data */
	pci_init_pool(addrs);
	memset(windows, 0, sizeof *windows * CARDBUS_DECODER_WINDOWS);
	cardbus_set_window(&windows[CARDBUS_DECODER_BUS], 0, 0, 0, 0);
	cardbus_set_window(&windows[CARDBUS_DECODER_IO0], 0, 0, 0, 0);
	cardbus_set_window(&windows[CARDBUS_DECODER_IO1], 0, 0, 0, 0);
	cardbus_set_window(&windows[CARDBUS_DECODER_MEM0], 0, 0, 0, 0);
	cardbus_set_window(&windows[CARDBUS_DECODER_MEM1], 0, 0, 0, 0);

	/* find the biggest block available for bus numbers */
	if ((decoders & CARDBUS_DECODER_MASK_BUS8) &&
			pci_find_largest_block(&avail->busaddrs, 0, 1, 0, 0, 0, 0xFF,
			0, &baselo, 0, &sizelo))
	{
		pci_free_bus_range(addrs, baselo, sizelo);
		cardbus_set_window(&windows[CARDBUS_DECODER_BUS], 0, baselo, 0, sizelo);
	}

	/* find the biggest block available for I/O 16 */
	if ((decoders & CARDBUS_DECODER_MASK_IO016) &&
			pci_find_largest_block(&avail->ioaddrs, 0, 0x4, 0, 0, 0, 0xFFFF,
			0, &baselo, 0, &sizelo))
	{
		pci_free_io_range(addrs, baselo, sizelo);
		cardbus_set_window(&windows[CARDBUS_DECODER_IO0], 0, baselo, 0, sizelo);
	}
	else
		DPRINTF(("cardbus: No space for I/O 16\n"));

	/* find the biggest block available for I/O 32 */
	if ((decoders & CARDBUS_DECODER_MASK_IO032) &&
			pci_find_largest_block(&avail->ioaddrs, 0, 0x4, 0, 0x10000,
			0, 0xFFFFFFFF, 0, &baselo, 0, &sizelo))
	{
		pci_free_io_range(addrs, baselo, sizelo);
		cardbus_set_window(&windows[CARDBUS_DECODER_IO0], 0, baselo, 0, sizelo);
	}
	else
		DPRINTF(("cardbus: No space for I/O 32\n"));

	/* find the biggest block available for memory 32 */
	if ((decoders & CARDBUS_DECODER_MASK_MEM032) &&
			pci_find_largest_block(&avail->memaddrs, 0, 0x1000, 0, 0x100000,
			0, 0xFFFFFFFF, 0, &baselo, 0, &sizelo))
	{
		pci_free_mem_range(addrs, 0, baselo, 0, sizelo);
		cardbus_set_window(&windows[CARDBUS_DECODER_MEM0], 0, baselo, 0, sizelo);
	}
	else
		DPRINTF(("cardbus: No space for mem high\n"));

	/* find the biggest block available for memory 32 */
	if ((decoders & CARDBUS_DECODER_MASK_MEM132) &&
			pci_find_largest_block(&avail->memaddrs, 0, 0x1000, 0, 0x0,
			0, 0x000FFFFF, 0, &baselo, 0, &sizelo))
	{
		pci_free_mem_range(addrs, 0, baselo, 0, sizelo);
		cardbus_set_window(&windows[CARDBUS_DECODER_MEM1], 0, baselo, 0, sizelo);
	}
	else
		DPRINTF(("cardbus: No space for mem low\n"));

	return NO_ERROR;
}

void
cardbus_update_windows(Pci_addresses *addrs, Pci_address_window *windows)
{
	Pci_address_block *b;

	for (b = addrs->busaddrs; b; b = b->next)
	{
		uInt limit = b->baselo + b->sizelo;

		if (limit == windows[CARDBUS_DECODER_BUS].limitlo)
			windows[CARDBUS_DECODER_BUS].limitlo = b->baselo;
	}

	for (b = addrs->ioaddrs; b; b = b->next)
	{
		uInt limitlo = b->baselo + b->sizelo;
		uInt limithi = b->basehi + b->sizehi + (limitlo < b->sizelo);

		if (b->baselo == windows[CARDBUS_DECODER_IO0].baselo &&
				b->basehi == windows[CARDBUS_DECODER_IO0].basehi)
		{
			windows[CARDBUS_DECODER_IO0].baselo = limitlo & ~3;
			windows[CARDBUS_DECODER_IO0].basehi = limithi;
		}

		if (b->baselo == windows[CARDBUS_DECODER_IO1].baselo &&
				b->basehi == windows[CARDBUS_DECODER_IO1].basehi)
		{
			windows[CARDBUS_DECODER_IO1].baselo = limitlo & ~3;
			windows[CARDBUS_DECODER_IO1].basehi = limithi;
		}

		if (limitlo == windows[CARDBUS_DECODER_IO0].limitlo && 
				limithi == windows[CARDBUS_DECODER_IO0].limithi)
		{
			DPRINTF(("cardbus: adjusting I/O limit\n"));
			windows[CARDBUS_DECODER_IO0].limitlo = b->baselo;
			windows[CARDBUS_DECODER_IO0].limithi = b->basehi;
		}

		if (limitlo == windows[CARDBUS_DECODER_IO1].limitlo && 
				limithi == windows[CARDBUS_DECODER_IO1].limithi)
		{
			DPRINTF(("cardbus: adjusting I/O limit\n"));
			windows[CARDBUS_DECODER_IO1].limitlo = b->baselo;
			windows[CARDBUS_DECODER_IO1].limithi = b->basehi;
		}
	}

	for (b = addrs->memaddrs; b; b = b->next)
	{
		uInt limitlo = b->baselo + b->sizelo;
		uInt limithi = b->basehi + b->sizehi + (limitlo < b->sizelo);

		if (b->baselo == windows[CARDBUS_DECODER_MEM0].baselo &&
				b->basehi == windows[CARDBUS_DECODER_MEM0].basehi)
		{
			windows[CARDBUS_DECODER_MEM0].baselo = limitlo & ~0xFFF;
			windows[CARDBUS_DECODER_MEM0].basehi = limithi;
		}

		if (b->baselo == windows[CARDBUS_DECODER_MEM1].baselo &&
				b->basehi == windows[CARDBUS_DECODER_MEM1].basehi)
		{
			windows[CARDBUS_DECODER_MEM1].baselo = limitlo & ~0xFFF;
			windows[CARDBUS_DECODER_MEM1].basehi = limithi;
		}

		if (limitlo == windows[CARDBUS_DECODER_MEM0].limitlo && 
				limithi == windows[CARDBUS_DECODER_MEM0].limithi)
		{
			DPRINTF(("cardbus: adjusting mem 0 limit\n"));
			windows[CARDBUS_DECODER_MEM0].limitlo = b->baselo;
			windows[CARDBUS_DECODER_MEM0].limithi = b->basehi;
		}

		if (limitlo == windows[CARDBUS_DECODER_MEM1].limitlo && 
				limithi == windows[CARDBUS_DECODER_MEM1].limithi)
		{
			DPRINTF(("cardbus: adjusting mem 1 limit\n"));
			windows[CARDBUS_DECODER_MEM1].limitlo = b->baselo;
			windows[CARDBUS_DECODER_MEM1].limithi = b->basehi;
		}
	}

	cardbus_set_window_size(&windows[CARDBUS_DECODER_BUS], 1);
	cardbus_set_window_size(&windows[CARDBUS_DECODER_IO0], 0x4);
	cardbus_set_window_size(&windows[CARDBUS_DECODER_IO1], 0x4);
	cardbus_set_window_size(&windows[CARDBUS_DECODER_MEM0], 0x1000);
	cardbus_set_window_size(&windows[CARDBUS_DECODER_MEM1], 0x1000);
}

void
cardbus_allocate_window(Pci_address_block **blocks, Pci_address_window *window)
{
	if (window->sizehi || window->sizelo)
		pci_allocate_fixed_block(blocks, window->basehi, window->baselo,
				window->sizehi, window->sizelo);
}

void
cardbus_allocate_windows(Pci_addresses *addrs, Pci_address_window *windows)
{
	cardbus_allocate_window(&addrs->busaddrs, &windows[CARDBUS_DECODER_BUS]);
	cardbus_allocate_window(&addrs->ioaddrs, &windows[CARDBUS_DECODER_IO0]);
	cardbus_allocate_window(&addrs->ioaddrs, &windows[CARDBUS_DECODER_IO1]);
	cardbus_allocate_window(&addrs->memaddrs, &windows[CARDBUS_DECODER_MEM0]);
	cardbus_allocate_window(&addrs->memaddrs, &windows[CARDBUS_DECODER_MEM1]);
}

void
cardbus_program_windows(Pci_device_info *devinfo, int decoders,
		Pci_address_window *windows)
{
	uInt pri_bus;
	uInt sec_bus;
	uInt sub_bus;
	uInt io0_base;
	uInt io0_limit;
	uInt io1_base;
	uInt io1_limit;
	uInt mem0_base;
	uInt mem0_limit;
	uInt mem1_base;
	uInt mem1_limit;

	/* init bus nums */
	pri_bus = devinfo->bus;
	sec_bus = windows[CARDBUS_DECODER_BUS].baselo;
	sub_bus = windows[CARDBUS_DECODER_BUS].limitlo - 1;

	if (windows[CARDBUS_DECODER_IO0].sizelo ||
			windows[CARDBUS_DECODER_IO0].sizehi)
	{
		io0_base = windows[CARDBUS_DECODER_IO0].baselo;
		io0_limit = windows[CARDBUS_DECODER_IO0].limitlo - 1;
	}
	else
	{
		io0_base = 0xFFFFFFFC;
		io0_limit = 0xFFFFFFF8;
	}

	if (windows[CARDBUS_DECODER_IO1].sizelo ||
			windows[CARDBUS_DECODER_IO1].sizehi)
	{
		io1_base = windows[CARDBUS_DECODER_IO1].baselo;
		io1_limit = windows[CARDBUS_DECODER_IO1].limitlo - 1;
	}
	else
	{
		io1_base = 0xFFFFFFFC;
		io1_limit = 0xFFFFFFF8;
	}

	if (windows[CARDBUS_DECODER_MEM0].sizelo ||
			windows[CARDBUS_DECODER_MEM0].sizehi)
	{
		mem0_base = windows[CARDBUS_DECODER_MEM0].baselo;
		mem0_limit = windows[CARDBUS_DECODER_MEM0].limitlo - 1;
	}
	else
	{
		mem0_base = 0xFFFFF000;
		mem0_limit = 0xFFFFE000;
	}

	if (windows[CARDBUS_DECODER_MEM1].sizelo ||
			windows[CARDBUS_DECODER_MEM1].sizehi)
	{
		mem1_base = windows[CARDBUS_DECODER_MEM1].baselo;
		mem1_limit = windows[CARDBUS_DECODER_MEM1].limitlo - 1;
	}
	else
	{
		mem1_base = 0xFFFFF000;
		mem1_limit = 0xFFFFE000;
	}

	config_write(devinfo, CARDBUS_CONFIG_BUSNUM, (sub_bus << 16) |
			(sec_bus << 8) | pri_bus);
	config_write(devinfo, CARDBUS_CONFIG_MEMBASE0, mem0_base);
	config_write(devinfo, CARDBUS_CONFIG_MEMLIMIT0, mem0_limit);
	config_write(devinfo, CARDBUS_CONFIG_MEMBASE1, mem1_base);
	config_write(devinfo, CARDBUS_CONFIG_MEMLIMIT1, mem1_limit);
	config_write(devinfo, CARDBUS_CONFIG_IOBASE0, io0_base);
	config_write(devinfo, CARDBUS_CONFIG_IOLIMIT0, io0_limit);
	config_write(devinfo, CARDBUS_CONFIG_IOBASE1, io1_base);
	config_write(devinfo, CARDBUS_CONFIG_IOLIMIT1, io1_limit);
}

#ifdef DEBUG
void
cardbus_dump_regs(Environ *e, Package *cpkg, Pci_device_info *devinfo)
{
    uInt busnum = config_read(devinfo, CARDBUS_CONFIG_BUSNUM);
    uInt membase0 = config_read(devinfo, CARDBUS_CONFIG_MEMBASE0);
    uInt membase1 = config_read(devinfo, CARDBUS_CONFIG_MEMBASE1);
    uInt memlimit0 = config_read(devinfo, CARDBUS_CONFIG_MEMLIMIT0);
    uInt memlimit1 = config_read(devinfo, CARDBUS_CONFIG_MEMLIMIT1);
    uInt iobase0 = config_read(devinfo, CARDBUS_CONFIG_IOBASE0);
    uInt iobase1 = config_read(devinfo, CARDBUS_CONFIG_IOBASE1);
    uInt iolimit0 = config_read(devinfo, CARDBUS_CONFIG_IOLIMIT0);
    uInt iolimit1 = config_read(devinfo, CARDBUS_CONFIG_IOLIMIT1);
    uInt legbase = config_read(devinfo, CARDBUS_CONFIG_LEGACYBASE);
    uInt cmd = config_read(devinfo, CARDBUS_CONFIG_COMMAND);

    DPRINTF(("cardbus: pri bus %d, sec bus %d, sub bus %d, cmd 0x%08X\n",
	    busnum & 0xFF, (busnum >> 8) & 0xFF, (busnum >> 16) & 0xFF, cmd));
    DPRINTF(("\tmem range0 0x%04X.%04X .. 0x%04X.%04X\n", membase0 >> 16,
	    membase0 & 0xFFFF, memlimit0 >> 16, (memlimit0 | 0xFFF) & 0xFFFF));
    DPRINTF(("\tmem range1 0x%04X.%04X .. 0x%04X.%04X\n", membase1 >> 16,
	    membase1 & 0xFFFF, memlimit1 >> 16, (memlimit1 | 0xFFF) & 0xFFFF));
    DPRINTF(("\tI/O range0 0x%04X.%04X .. 0x%04X.%04X\n", iobase0 >> 16,
	    iobase0 & 0xFFFF, iolimit0 >> 16, (iolimit0 | 3) & 0xFFFF));
    DPRINTF(("\tI/O range1 0x%04X.%04X .. 0x%04X.%04X\n", iobase1 >> 16,
	    iobase1 & 0xFFFF, iolimit1 >> 16, (iolimit1 | 3) & 0xFFFF));
    DPRINTF(("\tI/O legacy base 0x%04X.%04X\n", legbase >> 16,
	    legbase & 0xFFFF));
}
#endif

Retcode
cardbus_probe_pci_bridge(Environ *e, Package *pkg, Pci_device_info *devinfo,
		Pci_addresses *addrs, int devs)
{
	Pci_addresses addrwindows;
	Pci_address_window windows[CARDBUS_DECODER_WINDOWS];
	uInt pri_bus;
	uInt sec_bus;
	uInt sub_bus;
	int decoders;
	Retcode ret;
	Package *savepkg;
	uInt cmd;
	Byte *props;
	Int plen;

	cmd = config_read(devinfo, CARDBUS_CONFIG_COMMAND);
	cmd &= ~(PCI_COMMAND_IOENABLE|PCI_COMMAND_MEMENABLE|
			PCI_COMMAND_MASTERENABLE);
	config_write(devinfo, CARDBUS_CONFIG_COMMAND, cmd);

	cardbus_load_bridge_properties(e, pkg);
	pci_load_reg_and_name_props(e, pkg, devinfo);

	savepkg = e->currpkg;
	e->currpkg = pkg;
	ret = execute_word(e, "assign-addresses");
	e->currpkg = savepkg;

	if (ret != NO_ERROR)
		return ret;

	decoders = cardbus_probe_address_decoders(e, pkg, devinfo);

	DPRINTF(("cardbus: cardbus_probe_pci_bridge: decoders = %#X\n", decoders));

	cardbus_get_bridge_address_space(e, decoders, addrs, &addrwindows,
			windows);

	cardbus_program_windows(devinfo, decoders, windows);

	/* load up the PCI-CardBus bridge */
	pri_bus = devinfo->bus;
	sec_bus = pci_allocate_buses(&addrwindows, 1);
	sub_bus = windows[CARDBUS_DECODER_BUS].limitlo - 1;

	if (!sec_bus)
		return E_OUT_OF_MEMORY;

	/* setup bus-range property */
	DPRINTF(("cardbus: cardbus_probe_pci_bridge: pri_bus %d, sec_bus %d, sub_bus %d\n", pri_bus, sec_bus, sub_bus));
	plen = 0;
	props = prop_alloc(e, 2 * sizeof (Int));

	if (props == NULL)
		return E_OUT_OF_MEMORY;

	prop_encode_int(props + plen, &plen, sec_bus);
	prop_encode_int(props + plen, &plen, sub_bus);
	ret = add_property(pkg->props, "bus-range", CSTR, props, plen);

	if (ret != NO_ERROR)
		return ret;

	config_write(devinfo, CARDBUS_CONFIG_COMMAND, cmd |
			(PCI_COMMAND_IOENABLE|PCI_COMMAND_MEMENABLE|
			PCI_COMMAND_MASTERENABLE));

#ifdef DEBUG
	cardbus_dump_regs(e, pkg, devinfo);
#endif

	DPRINTF(("cardbus: cardbus_probe_pci_bridge: probing secondary bus\n"));
	ret = pci_probe_bus(e, pkg, devinfo->hbridge, sec_bus, &addrwindows, devs);
	DPRINTF(("cardbus: cardbus_probe_pci_bridge: ret = %d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	DPRINTF(("cardbus: updating windows\n"));
	cardbus_update_windows(&addrwindows, windows);
	config_write(devinfo, CARDBUS_CONFIG_COMMAND, cmd);
	DPRINTF(("cardbus: reprogramming windows\n"));
	cardbus_program_windows(devinfo, decoders, windows);
	config_write(devinfo, CARDBUS_CONFIG_COMMAND, cmd |
			(PCI_COMMAND_IOENABLE|PCI_COMMAND_MEMENABLE|
			PCI_COMMAND_MASTERENABLE));

	DPRINTF(("cardbus: allocating window address space from parent\n"));
	cardbus_allocate_windows(addrs, windows);
/*	pci_print_addresses(e, &addrwindows);	*/
	DPRINTF(("cardbus: destroying pool\n"));
	pci_destroy_pool(&addrwindows);

#ifdef DEBUG
	cardbus_dump_regs(e, pkg, devinfo);
#endif

	DPRINTF(("cardbus: updating ranges property\n"));
	/* update 2nd number in bus-range property to correct sub-bus num */
	plen = sizeof (Int);
	prop_encode_int(props + sizeof (Int), &plen,
			windows[CARDBUS_DECODER_BUS].limitlo - 1);

	/* create "ranges" property for all mapping registers - per range:
	   physhi..lo of child bus, physhi..lo of parent, sizehi..lo of child
	 */
	plen = 0;
	props = prop_alloc(e, 4 * 8 * sizeof (Int));

	if (props == NULL)
		return E_OUT_OF_MEMORY;

	if (windows[CARDBUS_DECODER_MEM0].sizelo)
		pci_encode_range_prop(props, &plen,
				PCI_PHYSHI_MK(0, 0, 0, PCI_SPACE_MEM, 0, 0, 0, 0),
				0, windows[CARDBUS_DECODER_MEM0].baselo,
				0, windows[CARDBUS_DECODER_MEM0].limitlo - 
					windows[CARDBUS_DECODER_MEM0].baselo);

	if (windows[CARDBUS_DECODER_MEM1].sizelo)
		pci_encode_range_prop(props, &plen,
				PCI_PHYSHI_MK(0, 0, 0, PCI_SPACE_MEM, 0, 0, 0, 0),
				0, windows[CARDBUS_DECODER_MEM1].baselo,
				0, windows[CARDBUS_DECODER_MEM1].limitlo - 
					windows[CARDBUS_DECODER_MEM1].baselo);

	if (windows[CARDBUS_DECODER_IO0].sizelo)
		pci_encode_range_prop(props, &plen,
				PCI_PHYSHI_MK(0, 0, 0, PCI_SPACE_IO, 0, 0, 0, 0),
				0, windows[CARDBUS_DECODER_IO0].baselo,
				0, windows[CARDBUS_DECODER_IO0].limitlo - 
					windows[CARDBUS_DECODER_IO0].baselo);

	if (windows[CARDBUS_DECODER_IO1].sizelo)
		pci_encode_range_prop(props, &plen,
				PCI_PHYSHI_MK(0, 0, 0, PCI_SPACE_IO, 0, 0, 0, 0),
				0, windows[CARDBUS_DECODER_IO1].baselo,
				0, windows[CARDBUS_DECODER_IO1].limitlo - 
					windows[CARDBUS_DECODER_IO1].baselo);

	ret = add_property(pkg->props, "ranges", CSTR, props, plen);

	if (ret != NO_ERROR)
		return ret;

	/* Build assigned address property */
	/* It may partially exist from previous call to assign-address */

	return NO_ERROR;
}

/* PCI-CardBus bridge */
Retcode
cardbus_probe(Environ *e, Package *cpkg, Pci_device_info *devinfo,
		Pci_addresses *addrs)
{
	Retcode err;
	DPRINTF(("cardbus: cardbus_probe(0x%X, 0x%X, 0x%X, 0x%X)\n", e, cpkg, devinfo, addrs));

	/* attempt to load fcode from the expansion ROM */
	err = pci_load_expansion_rom(e, cpkg, devinfo, addrs);

	if (err == NO_ERROR)
		return NO_ERROR;

	DPRINTF(("cardbus: cardbus_probe: no expansion ROM\n"));

	/* attempt to load a builtin device driver */
	err = pci_load_builtin_driver(e, cpkg, devinfo, addrs);

	if (err == NO_ERROR)
		return NO_ERROR;

	DPRINTF(("cardbus: cardbus_probe: no built-in driver\n"));

	/* no luck with the device.  Give it a generic configuration. */
	err = cardbus_probe_pci_bridge(e, cpkg, devinfo, addrs, 32);
#ifdef DEBUG
	cardbus_dump_regs(e, cpkg, devinfo);
#endif
	return err;
}

static void
cardbus_get_extended_device_info(Environ *e, Pci_device_info *devinfo)
{
	Int reg = config_read(devinfo, CARDBUS_CONFIG_SUBSYSTEM);
	devinfo->subsysvendid = reg & 0xFFFF;
	devinfo->subsysdevid = (reg >> 16) & 0xFFFF;
}

static Retcode
cardbus_rom_size(Environ *e, Package *pkg, Pci_device_info *devinfo, 
		uInt *size)
{
	if (size)
		*size = 0U;

	return NO_ERROR;
}

static Retcode
cardbus_write_rom_address(Environ *e, Package *pkg, Pci_device_info *devinfo,
		uInt addr)
{
	return NO_ERROR;
}

static Retcode
cardbus_address_offset_list(Environ *e, Package *pkg, Pci_device_info *devinfo,
		uChar **offsets, int *numoffsets)
{
	static uChar offsetlist[] =
	{ 
		CARDBUS_CONFIG_BAR0,
		CARDBUS_CONFIG_LEGACYBASE,
	};

	if (offsets)
		*offsets = offsetlist;

	if (numoffsets)
		*numoffsets = sizeof offsetlist / sizeof offsetlist[0];

	return NO_ERROR;
}

static Retcode
cardbus_get_address_info(Environ *e, Package *pkg, Pci_device_info *devinfo,
		int reg, uInt *physhi, uInt *sizehi, uInt *sizelo)
{
	DPRINTF(("cardbus_get_address_info: e %#X, pkg %#X, devinfo %#X, reg %#X, physhi %#X, sizehi %#X, sizelo %#X\n", e, pkg, devinfo, reg, physhi, sizehi, sizelo));

	if (reg == CARDBUS_CONFIG_BAR0 || reg == CARDBUS_CONFIG_LEGACYBASE)
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

#if 0
	if (reg == CARDBUS_CONFIG_LEGACYBASE)
	{
		uInt regsave;
		uInt reg2;
		uInt reg3;
		uInt size;

		regsave = config_read(devinfo, reg);
		config_write(devinfo, reg, 0xFFFFFFFF);
		reg2 = config_read(devinfo, reg);
		config_write(devinfo, reg, 0x00000000);
		reg3 = config_read(devinfo, reg);
		config_write(devinfo, reg, regsave);

		DPRINTF(("cardbus: legacy base: reg2 %#x, reg3 %#x\n", reg2, reg3));

		if ((reg2 & 0xFFFF0000) == 0xFFFF0000 &&
				(reg3 & 0xFFFF0000) == 0x00000000)
		{
			/* 32-bit base register */
			size = (~reg2 | 3) + 1;
		}
		else
		{
			/* 16-bit base register */
			size = ((~reg2 | 3) + 1) & 0xFFFF;
		}

		if (physhi)
			*physhi = PCI_PHYSHI_MK(0, 0, 0, PCI_SPACE_IO, devinfo->bus,
					devinfo->dev, devinfo->func, reg);

		if (sizelo)
			*sizelo = size;

		if (sizehi)
			*sizehi = 0;

		return NO_ERROR;
	}
#endif

	if (physhi)
		*physhi = 0;

	if (sizehi)
		*sizehi = 0;

	if (sizelo)
		*sizelo = 0;

	return NO_ERROR;
}

static Retcode
cardbus_write_address(Environ *e, Package *pkg, Pci_device_info *devinfo,
		int reg, uInt physhi, uInt physmid, uInt physlo)
{
	switch (PCI_PHYSHI_SPACE(physhi))
	{
	case PCI_SPACE_IO:
	case PCI_SPACE_MEM:
		config_write(devinfo, reg, physlo);
		break;
	}

	return NO_ERROR;
}

Pci_header_methods pci_header2_methods =
{
	cardbus_get_extended_device_info,
	cardbus_probe,
	cardbus_rom_size,
	cardbus_write_rom_address,
	cardbus_address_offset_list,
	cardbus_get_address_info,
	cardbus_write_address,
};
