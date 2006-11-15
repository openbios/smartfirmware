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

/* (c) Copyright 2003 by CodeGen, Inc.  All Rights Reserved. */

/* Driver for 8111 functions to enable various capabilities.
 */

//#define DEBUG

#include "defs.h"
#include "pci.h"

int g_amd8111_device_id;

void
pci_config_update(int hbridge, int bus, int dev, int func, int reg, uInt set,
		uInt clr)
{
	uInt val = pci_config_read(hbridge, bus, dev, func, reg);
	val |= set;
	val &= ~clr;
	pci_config_write(hbridge, bus, dev, func, reg, val);
}

/* Turn on legacy IDE ports before ISA bus is probed.  This has to
 * be done twice - once before ISA is probed (ie when LPC is inited)
 * and again when the IDE func is probed.  Otherwise the second probe
 * turns off the I/O space access!
 *
 * Also turn on the magic 0xCF9 hard-reset feature, which requires
 * poking the system-management function of the 8111.
 */
PCI(install_amd_8111_driver)
{
    EPCI(install_pciisa_driver);
    /* hard-wired in 8111 */
    #define LPC_FUNC		0
    #define IDE_FUNC		1
    #define SMB_FUNC		2
    #define SYSMGMT_FUNC	3

    DPRINTF(("amd_8111_legacy_ide: pkg=%p devinfo=%p"
			" vendid=%#x devid=%#x\n",
			pkg, devinfo, devinfo->vendid, devinfo->devid));

    /* enable I/O space */
    pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			IDE_FUNC, 0x04, 0x02000005);

    /* and both IDE interfaces */
	pci_config_update(devinfo->hbridge, devinfo->bus, devinfo->dev,
			IDE_FUNC, 0x40, 0x03, 0x00);

#if 0
    /* init various other sysmgmt regs */
	pci_config_update(devinfo->hbridge, devinfo->bus, devinfo->dev,
			SYSMGMT_FUNC, 0x40, 0x2000, 0x0);
#endif

	if (devinfo->func == LPC_FUNC)
	{
		/* XXX: setup the reg property so that the odd ball BARs get space */
		/* NVRAM @74, HPET @, Watchdog Timer @A8 */
		g_amd8111_device_id = devinfo->dev;
		return install_pciisa_driver(e, pkg, devinfo, addrs);
	}

	if (devinfo->func == IDE_FUNC)
	{
		pci_load_reg_and_name_props(e, pkg, devinfo);
		prop_set_str(pkg->props, "name", CSTR, "pciide", CSTR);
	}

	if (devinfo->func == SMB_FUNC)
	{
		pci_load_reg_and_name_props(e, pkg, devinfo);
		prop_set_str(pkg->props, "name", CSTR, "smbus", CSTR);
	}

    if (devinfo->func == SYSMGMT_FUNC)
	{
		/* enable access to 0xCF9 for reset-all to work properly */
		pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			SYSMGMT_FUNC, 0x40, 0x2000 | 0x1409B100);

		/* init other sysmgmt registers */
		pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			SYSMGMT_FUNC, 0x48, 0x500420);

		pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			SYSMGMT_FUNC, 0x4C, 0x1000000);

		/* PCI IRQ routing register (msw: PIRQ:DCBA) */
		/* values must also be updated in pcibase.c:pci_config_map_intr */
		pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			SYSMGMT_FUNC, 0x54, 0x55BA000F);

		/* init various ISA ports for legacy interrupts */
		/* 0C20 : 0001.1100.0010.0000 */
		pci_io_write(devinfo->hbridge, 0x4D0, 0x1C20, 2);
		pci_io_write(devinfo->hbridge, 0x61, 0x03C, 1);

		pci_load_reg_and_name_props(e, pkg, devinfo);
		/* XXX: setup the reg property so that the odd ball BAR gets space */
		/* PM @58 */
//		physhi physmid physlo sizehi sizelo
//		self | 58 0 0 0 100
		prop_set_str(pkg->props, "name", CSTR, "sysmgmt", CSTR);
	}

    return NO_ERROR;
}

/* loaded for LPC, IDE, SMBUS, & System Mgmt funcs */
Pci_driver amd_8111_driver =
{
	{ 0, 0, 0, 0, 0, 0, 0x1022, 0x7468, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0xFFFF, 0xFFF8, 0, 0, 0 },
	install_amd_8111_driver
};
