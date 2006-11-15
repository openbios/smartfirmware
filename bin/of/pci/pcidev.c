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

/* (c) Copyright 1996-1997 by CodeGen, Inc.  All Rights Reserved */

/*#define DEBUG*/

#include "defs.h"
#include "pci.h"

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

static void
pcidev_get_extended_device_info(Environ *e, Pci_device_info *devinfo)
{
	Int reg = config_read(devinfo, PCI_CONFIG_SUBSYSTEM);
	devinfo->subsysvendid = reg & 0xFFFF;
	devinfo->subsysdevid = (reg >> 16) & 0xFFFF;
}

static Retcode
pcidev_probe(Environ *e, Package *cpkg, Pci_device_info *devinfo,
		Pci_addresses *addrs)
{
	Retcode err;
	Int reg, intr;

	/* load header type 0 specific properties */
	reg = config_read(devinfo, PCI_CONFIG_INTERRUPT);
	DPRINTF(("pcidev_probe: CONFIG_INTERRUPT=%#x\n", reg));
	prop_set_int(cpkg->props, "min-grant", CSTR, (reg >> 16) & 0xFF);
	prop_set_int(cpkg->props, "max-latency", CSTR, (reg >> 24) & 0xFF);

	/* get intr line info (machine-dependent) */
	err = pci_config_map_intr(devinfo->hbridge, devinfo->bus, devinfo->dev,
			(reg >> 8) & 0xFF, &intr);

	/* PCI book recommends setting unmapped devs intr-line to 0xFF */
	if (err != NO_ERROR)
		intr = 0xFF;

	/* set intr line info */
	config_write(devinfo, PCI_CONFIG_INTERRUPT,
		(reg & ~0xFF) | (intr & 0xFF));

	/* attempt to load fcode from the expansion ROM */
	err = pci_load_expansion_rom(e, cpkg, devinfo, addrs);

	if (err == NO_ERROR)
		return NO_ERROR;

	/* attempt to load a builtin device driver */
	err = pci_load_builtin_driver(e, cpkg, devinfo, addrs);

	if (err == NO_ERROR)
		return NO_ERROR;

	/* no luck with the device.  Give it a generic configuration. */
	pci_load_reg_and_name_props(e, cpkg, devinfo);
	return err;
}

static Retcode
pcidev_rom_size(Environ *e, Package *pkg, Pci_device_info *devinfo, 
		uInt *size)
{
	Int save;
	Int set;
	Int sz;

	save = config_read(devinfo, PCI_CONFIG_ROMADDR);
	config_write(devinfo, PCI_CONFIG_ROMADDR, 0xFFFFFFFE);
	set = config_read(devinfo, PCI_CONFIG_ROMADDR);
	config_write(devinfo, PCI_CONFIG_ROMADDR, save);

	/* calculate size of expansion ROM BAR */
	sz = (~set | 0x7FF) + 1;
	sz &= -sz;		/* clear any un-programable upper bits */

	if (size)
		*size = sz;

	return NO_ERROR;
}

static Retcode
pcidev_write_rom_address(Environ *e, Package *pkg, Pci_device_info *devinfo,
		uInt addr)
{
	if (addr)
		config_write(devinfo, PCI_CONFIG_ROMADDR, addr | 1);
	else
		config_write(devinfo, PCI_CONFIG_ROMADDR, 0);

	return NO_ERROR;
}

static Retcode
pcidev_address_offset_list(Environ *e, Package *pkg, Pci_device_info *devinfo,
		uChar **offsets, int *numoffsets)
{
	static uChar offsetlist[] =
	{ 
		PCI_CONFIG_BAR0,
		PCI_CONFIG_BAR1,
		PCI_CONFIG_BAR2,
		PCI_CONFIG_BAR3,
		PCI_CONFIG_BAR4,
		PCI_CONFIG_BAR5,
		PCI_CONFIG_ROMADDR
	};

	if (offsets)
		*offsets = offsetlist;

	if (numoffsets)
		*numoffsets = sizeof offsetlist / sizeof offsetlist[0];

	return NO_ERROR;
}

static Retcode
pcidev_get_address_info(Environ *e, Package *pkg, Pci_device_info *devinfo,
		int reg, uInt *physhi, uInt *sizehi, uInt *sizelo)
{
	if (reg >= PCI_CONFIG_BAR0 && reg <= PCI_CONFIG_BAR5)
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

	if (reg == PCI_CONFIG_ROMADDR)
	{
		/* check the for an expansion rom */
		pcidev_rom_size(e, pkg, devinfo, sizelo);

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
pcidev_write_address(Environ *e, Package *pkg, Pci_device_info *devinfo,
		int reg, uInt physhi, uInt physmid, uInt physlo)
{
	if (reg == PCI_CONFIG_ROMADDR)
		return pcidev_write_rom_address(e, pkg, devinfo, physlo);

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

Pci_header_methods pci_header0_methods =
{
	pcidev_get_extended_device_info,
	pcidev_probe,
	pcidev_rom_size,
	pcidev_write_rom_address,
	pcidev_address_offset_list,
	pcidev_get_address_info,
	pcidev_write_address,
};
