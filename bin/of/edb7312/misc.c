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

#include "defs.h"

Retcode
misc_mkdev(Environ *e, Package *ppkg, uInt addr, uInt size,
	    char *name, char *devtype)
{
	Package *pkg = new_pkg_name(ppkg, name);
	Byte *prop;
	Int plen = 0;

	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	prop_set_str(pkg->props, "device_type", CSTR, devtype, CSTR);

	/* encode "reg" property for unit address */
	prop = prop_alloc(e, 2 * sizeof (Int));

	if (prop == NULL)
		return E_OUT_OF_MEMORY;

	prop_encode_int(prop + plen, &plen, addr);
	prop_encode_int(prop + plen, &plen, size);
	return add_property(pkg->props, "reg", CSTR, prop, plen);
}

const struct devinfo
{
	uInt addr;
	uInt size;
	char *name;
	char *devtype;
} misc_devs[] = 
{
	{ 0x30000000, 0x00000002, "parallel",     "serial" },
/*	{ 0x30010000, 0x00000001, "keyboard-row", "misc" },	*/
	{ 0x40000000, 0x00000010, "usb",          "usb-device" },
	{ 0x80000500, 0x00000010, "spi",          "serial" },
	{ 0x80002000, 0x00000140, "dai",          "serial" },
	{ 0, 0, 0, 0 },
};

CC(misc_install)
{
	Retcode ret = NO_ERROR;
	Retcode ret2;
	const struct devinfo *d = misc_devs;

	while (d->name)
	{
		ret2 = misc_mkdev(e, e->currpkg, d->addr, d->size, d->name, d->devtype);

		if (ret == NO_ERROR)
			ret = ret2;

		d++;
	}

	return ret;
}
