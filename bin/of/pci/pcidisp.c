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

/* (c) Copyright 1996-1998 by CodeGen, Inc.  All Rights Reserved. */

/* builtin PCI display driver
   - hopefully generic enough to work with anything
 */

#include "defs.h"
#include "pci.h"


#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480
#define LINES			25
#define COLUMNS			80


C(f_screen_open)			/* screen-open (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Retcode ret;
	Entry *ent;
	Int plen;
	Byte *paddr;
	Int physhi, physmid, physlo, sizehi, sizelo, cfg;
	Pci_device_info *devinfo;
	EC(f_default_font);
	EC(f_set_font);
	EC(f_fb8_install);

	DPRINTF(("screen_open: pkg=%p\n", pkg));

	if (!pkg)
		return E_NULL_PACKAGE;

	devinfo = (Pci_device_info*)pkg->self;

	if (!devinfo)
		return E_BAD_PACKAGE;

	/* decode the assigned-addresses property to find the PCI address and size
	   of video memory */
	ent = find_table(pkg->props, "assigned-addresses", CSTR);

	if (!ent)
		return E_NO_PROPERTY;
	
	paddr = ent->v.array;
	plen = ent->len;
	DPRINTF(("screen_open: paddr=%p plen=%d\n", paddr, plen));
	
	/* find a map into memory space, which we assume is video-memory */
	do
		ret = pci_decode_reg_prop(&paddr, &plen, &physhi, &physmid,
				&physlo, &sizehi, &sizelo);
	while (PCI_PHYSHI_SPACE(physhi) != PCI_SPACE_MEM);
	
	/* only our frame-buffer var needs to be initialized here - all others
	   are handled by fb8-install */
	DPRINTF(("screen_open: physhi=%#x physmid=%#x physlo=%#x\n",
			physhi, physmid, physlo));

    ret = pci_map_in(devinfo->hbridge, physhi, physmid, physlo,
			SCREEN_HEIGHT * SCREEN_WIDTH, &e->framebuf);
	DPRINTF(("screen_open: sizehi=%#x sizelo=%#x framebuf=%#Cx\n",
			sizehi, sizelo, e->framebuf));

	/* enable I/O and memory */
	cfg = pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND);
	DPRINTF(("screen_open: before cfg=%#x\n", cfg));

	pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND,
			(cfg & 0xFFFF) | PCI_COMMAND_IOENABLE | PCI_COMMAND_MEMENABLE);

	cfg = pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND);
	DPRINTF(("screen_open: after cfg=%#x\n", cfg));

	if (ret != NO_ERROR)
		return ret;

	ret = execute_word(e, "default-font");
	
	if (ret != NO_ERROR)
		return ret;
	
	ret = execute_word(e, "set-font");
	
	if (ret != NO_ERROR)
		return ret;

	/* init fb & bg colors as per the OF spec for 8-bit color displays */
	e->pixsize = 8;
	e->fg = 15;
	e->bg = 0;

	/* see if we should invert fb/bg colors */
	if (get_config_bool(e, "inverse-video?", CSTR))
	{
		Int t = e->fg;
		e->fg = e->bg;
		e->bg = t;
	}
	
	IFCKSP(e, 0, 4);
	PUSH(e, SCREEN_WIDTH);
	PUSH(e, SCREEN_HEIGHT);
	PUSH(e, COLUMNS);
	PUSH(e, LINES);
	ret = f_fb8_install(e);	/* fb8-install */
	DPRINTF(("screen_open: ret=%d:%s\n", ret, err2str(ret)));

	return ret;
}

C(f_screen_close)			/* screen-close (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Pci_device_info *devinfo;
	Int cfg;

	if (!pkg)
		return E_NULL_PACKAGE;

	devinfo = (Pci_device_info*)pkg->self;

	if (!devinfo)
		return E_BAD_PACKAGE;

	/* disable I/O and memory access */
	cfg = pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND);
	pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND, cfg & 0xFFF8);
	
	return pci_map_out(devinfo->hbridge, e->framebuf,
			SCREEN_WIDTH * SCREEN_HEIGHT);
}

C(f_screen_selftest)		/* selftest (-- 0|error-code) */
{
	Bool diag = diagnostic_mode(e);

	if (diag)
	{
		cprintf(e, "screen selftest\n");
		/* do something someday */
	}

	PUSH(e, 0);			/* successful */
	return NO_ERROR;
}

/* use these methods for both input and output instead of making separate tables
 */
static const Initentry screen_methods[] =
{
	{ "screen-open",      f_screen_open,          INVALID_FCODE },
	{ "screen-close",     f_screen_close,         INVALID_FCODE },
	{ "selftest",         f_screen_selftest,      INVALID_FCODE },
	{ NULL,            NULL },
};

PCI(install_pci_display_driver)
{
	Retcode ret;
	Entry *ent;
	Package *savepkg = e->currpkg;
	Pci_device_info *newinfo;
	EC(f_is_install);
	EC(f_is_remove);

	DPRINTF(("install_pci_display: pkg=%p devinfo=%p\n", pkg, devinfo));

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	/* setup the basic properties for this device */
	pci_load_reg_and_name_props(e, pkg, devinfo);

	(void)prop_set_str(pkg->props, "device_type", CSTR, "display", CSTR);
	(void)prop_set_str(pkg->props, "iso6429-1983-colors", CSTR, "", CSTR);
	ret = init_entries(e, pkg->dict, screen_methods);
	
	/* need to pass in the xtok for our open routine to is-install */
	ent = find_table(pkg->dict, "screen-open", CSTR);
	
	if (ent == NULL)
		return E_NO_METHOD;
	
	e->currpkg = pkg;
	PUSH(e, ent->xtok);
	ret = execute_word(e, "is-install");	/* adds open, write, draw-logo, and restore */
	e->currpkg = savepkg;
	
	if (ret != NO_ERROR)
		return ret;
	
	/* need to pass in the xtok for our close routine to is-remove */
	ent = find_table(pkg->dict, "screen-close", CSTR);
	
	if (ent == NULL)
		return E_NO_METHOD;
	
	e->currpkg = pkg;
	PUSH(e, ent->xtok);
	ret = execute_word(e, "is-remove");		/* adds a close method */
	e->currpkg = savepkg;

	if (ret != NO_ERROR)
		return ret;

	/* allocate space and stash a copy of our devinfo for our methods */
	newinfo = (Pci_device_info*)malloc(sizeof *newinfo);

	if (newinfo == NULL)
		return E_OUT_OF_MEMORY;

	*newinfo = *devinfo;					/* copy it */
	pkg->self = (struct pself*)newinfo;		/* and save it */

	return ret;
}

Pci_driver pci_display_driver =
{
	{ 0, 0, 0, 0, 0, 256, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFFFFFFFF, 0, 0, 0, 0, 0 },
	install_pci_display_driver
};

Pci_driver pci_display2_driver =
{
	{ 0, 0, 0, 0, 0, 0x030000, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFFFF0000, 0, 0, 0, 0, 0 },
	install_pci_display_driver
};
