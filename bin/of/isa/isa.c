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

/* basic ISA bus support - no EISA or PnP support yet */

#include "stddef.h"
#include "defs.h"
#include "isa.h"

Retcode (*isa_mem_read)(uInt addr, void *value, int size);
Retcode (*isa_mem_write)(uInt addr, Int value, int size);
Retcode (*isa_io_read)(uInt addr, void *value, int size);
Retcode (*isa_io_write)(uInt addr, Int value, int size);
Retcode (*isa_map_in)(Int physhi, Int physlo, Cell size, Cell *virt);
Retcode (*isa_map_out)(Cell virt, Cell size);
Retcode (*isa_dma_alloc)(Cell size, Cell *virt);
Retcode (*isa_dma_free)(Cell virt, Cell size);
Retcode (*isa_dma_map_in)(Cell virt, Cell size, Int cacheable, Cell *devaddr);
Retcode (*isa_dma_map_out)(Cell virt, Cell devaddr, Cell size);
Retcode (*isa_dma_sync)(Cell virt, Cell devaddr, Cell size);

C(f_isa_open)			/* open (-- okay?) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, FTRUE);
	return NO_ERROR;
}

C(f_isa_close)			/* close (--) */
{
	return NO_ERROR;
}

C(f_isa_map_in)			/* map-in (phys.lo phys.hi size -- virt) */
{
	Int phys_low;
	Int phys_hi;
	Cell size;
	Cell virt;
	Retcode ret;

	IFCKSP(e, 3, 1);
	POP(e, size);
	POP(e, phys_hi);
	POP(e, phys_low);
	ret = isa_map_in(phys_hi, phys_low, size, &virt);
	PUSH(e, virt);
	return ret;
}

C(f_isa_map_out)		/* map-out (virt size --) */
{
	Cell virt;
	Cell size;

	IFCKSP(e, 2, 0);
	POP(e, size);
	POP(e, virt);
	return isa_map_out(virt, size);
}

C(f_isa_dma_alloc)			/* dma-alloc (size -- virt) */
{
	Cell virt;
	Cell size;
	Retcode ret;

	IFCKSP(e, 1, 1);
	POP(e, size);
	ret = isa_dma_alloc(size, &virt);
	PUSH(e, virt);
	return ret;
}

C(f_isa_dma_free)			/* dma-free (virt size --) */
{
	Cell virt;
	Cell size;

	IFCKSP(e, 2, 0);
	POP(e, size);
	POP(e, virt);
	return isa_dma_free(virt, size);
}

C(f_isa_dma_map_in)			/* dma-map-in (virt size cacheable? -- devaddr) */
{
	Cell virt;
	Cell size;
	Int cacheable;
	Cell devaddr;
	Retcode ret;

	IFCKSP(e, 3, 1);
	POP(e, cacheable);
	POP(e, size);
	POP(e, virt);
	ret = isa_dma_map_in(virt, size, cacheable, &devaddr);
	PUSH(e, devaddr);
	return ret;
}

C(f_isa_dma_map_out)		/* dma-map-out (virt devaddr size --) */
{
	Cell virt;
	Cell devaddr;
	Cell size;

	IFCKSP(e, 3, 0);
	POP(e, size);
	POP(e, devaddr);
	POP(e, virt);
	return isa_dma_map_out(virt, devaddr, size);
}

C(f_isa_dma_sync)			/* dma-sync (virt devaddr size --) */
{
	Cell virt;
	Cell devaddr;
	Cell size;

	IFCKSP(e, 3, 0);
	POP(e, size);
	POP(e, devaddr);
	POP(e, virt);
	return isa_dma_sync(virt, devaddr, size);
}

/* probe-self (arg-str arg-len reg-str reg-len fcode-str fcode-len --) */
C(f_isa_probe_self)
{
	Byte *arg, *reg, *fcode;
	Int arglen, reglen, fcodelen;

	IFCKSP(e, 6, 0);
	POP(e, fcodelen);
	POPT(e, fcode, Byte*);
	POP(e, reglen);
	POPT(e, reg, Byte*);
	POP(e, arglen);
	POPT(e, arg, Byte*);

	/* TODO */

	return E_UNIMPLEMENTED;
}

C(f_isa_decode_unit)	/* decode-unit (str len -- phys.lo phys.hi) */
{
	Byte *str;
	Int slen;
	Cell err, lo, hi = 0;

	IFCKSP(e, 2, 2);
	POP(e, slen);
	POPT(e, str, Byte*);
	setstrlen(&str, &slen);

	if (slen && (str[0] == 'm' || str[0] == 'M'))	/* memory space */
	{
		str++;
		slen--;
	}
	else	/* assume I/O */
	{
		hi |= ISA_IO_ADDRESS;

		/* skip any 'i' in front */
		if (slen && (str[0] == 'i' || str[0] == 'I'))
		{
			str++;
			slen--;
		}

		/* see if this is a 10-bit ('t') or 11-bit ('v') address */
		if (slen && (str[0] == 't' || str[0] == 'T'))
		{
			hi |= ISA_10_BIT_ADDR;
			str++;
			slen--;
		}
		else if (slen && (str[0] == 'v' || str[0] == 'V'))
		{
			hi |= ISA_11_BIT_ADDR;
			str++;
			slen--;
		}
	}

	parse_number(16, &str, &slen, &lo, &err, FALSE);

	PUSH(e, lo);
	PUSH(e, hi);
	return NO_ERROR;
}

C(f_isa_encode_unit)	/* encode-unit (phys.lo phys.hi -- str len) */
{
	static Byte buf[32];
	Cell hi;
	Cell lo;
	Byte *s = buf;

	IFCKSP(e, 2, 2);
	POP(e, hi);
	POP(e, lo);
	*s = '\0';

	if (hi & ISA_IO_ADDRESS)
	{
		*s++ = 'i';

		if (hi & ISA_10_BIT_ADDR)
			*s++ = 't';
		else if (hi & ISA_11_BIT_ADDR)
			*s++ = 'v';
	}
	else
		*s++ = 'm';

	bprintf(s, "%x", (unsigned int)lo);

	PUSHP(e, buf);
	PUSH(e, strlen(buf));
	return NO_ERROR;
}

C(f_isa_probe_pnp)			/* probe-pnp (--) */
{
	/* TODO */
	return NO_ERROR;
}

/* make it easy to create ISA devices using Isa_device */
Retcode
new_isa_device(Environ *e, Isa_device *dev)
{
	Retcode ret;
	Package *pkg;
	Byte *prop;
	Int plen = 0;
	Int i;
	EC(f_reg);

	IFCKSP(e, 0, 3);
	pkg = new_pkg_name(e->currpkg, dev->name);

	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	/* handy way to get to our device info without decoding Forth params */
	pkg->self = (struct pself*)dev;
	dev->pkg = pkg;		/* our caller may want this */

	/* set the type of this device */
	prop_set_str(pkg->props, "device_type", CSTR, dev->type, CSTR);

	/* encode "reg" property for unit address */
	prop = prop_alloc(e, 3 * sizeof (Int));

	if (prop == NULL)
		return E_OUT_OF_MEMORY;

	prop_encode_int(prop + plen, &plen, dev->physhi);
	prop_encode_int(prop + plen, &plen, dev->physlo);
	prop_encode_int(prop + plen, &plen, dev->size);

	/* add in any additional reg props for reserved addresses, etc */
	if (dev->numreg)
	{
		prop_alloc(e, dev->numreg * 3 * sizeof (Int));

		for (i = 0; i < dev->numreg; i++)
		{
			prop_encode_int(prop + plen, &plen, dev->reg[i * 3]);
			prop_encode_int(prop + plen, &plen, dev->reg[i * 3 + 1]);
			prop_encode_int(prop + plen, &plen, dev->reg[i * 3 + 2]);
		}
	}

	ret = add_property(pkg->props, "reg", CSTR, prop, plen);

	/* create interrupts property if IRQ is not zero */
	if (ret == NO_ERROR && dev->irq[0])
	{
		prop = prop_alloc(e, 2 * sizeof (Int));

		if (prop == NULL)
			return E_OUT_OF_MEMORY;

		plen = 0;
		prop_encode_int(prop + plen, &plen, dev->irq[0]);
		prop_encode_int(prop + plen, &plen, dev->irq[1]);
		ret = add_property(pkg->props, "interrupts", CSTR, prop, plen);
	}

	/* create dma property if DMA is >= zero */
	if (ret == NO_ERROR && dev->dma[0] >= 0)
	{
		prop = prop_alloc(e, 5 * sizeof (Int));
		plen = 0;

		if (prop == NULL)
			return E_OUT_OF_MEMORY;

		for (i = 0; i < 5; i++)
			prop_encode_int(prop + plen, &plen, dev->dma[i]);

		add_property(pkg->props, "dma", CSTR, prop, plen);
	}

	if (ret == NO_ERROR)
		ret = init_entries(e, pkg->dict, dev->methods);

	return ret;
}

C(f_probe_isa)				/* probe-isa (--) */
{
	Isa_device **devlist;
	Isa_device *dev;
	Retcode ret;

	/* install builtin ISA drivers - no support for ISA devices with Fcode */
	for (devlist = isa_devices; *devlist; devlist++)
	{
		dev = *devlist;

		/* if no probe routine, just assume the device exists */
		if (dev->probe && (*dev->probe)(e, dev) != NO_ERROR)
			continue;

		/* build node */
		if (dev->install)
			ret = (*dev->install)(e, dev);
		else
			ret = new_isa_device(e, dev);

		if (ret != NO_ERROR)
			return ret;
	}

	return NO_ERROR;
}

static Initentry isa_methods[] =
{
	{ "open", f_isa_open, INVALID_FCODE },
	{ "close", f_isa_close, INVALID_FCODE },
	{ "map-in", f_isa_map_in, INVALID_FCODE },
	{ "map-out", f_isa_map_out, INVALID_FCODE },
	{ "dma-alloc", f_isa_dma_alloc, INVALID_FCODE },
	{ "dma-free", f_isa_dma_free, INVALID_FCODE },
	{ "dma-map-in", f_isa_dma_map_in, INVALID_FCODE },
	{ "dma-map-out", f_isa_dma_map_out, INVALID_FCODE },
	{ "dma-sync", f_isa_dma_sync, INVALID_FCODE },
	{ "probe-self", f_isa_probe_self, INVALID_FCODE },
	{ "decode-unit", f_isa_decode_unit, INVALID_FCODE },
	{ "encode-unit", f_isa_encode_unit, INVALID_FCODE },
	{ "probe-pnp", f_isa_probe_pnp, INVALID_FCODE },
	{ NULL, NULL },
};

static Initentry isa_words[] =
{
	{ "probe-isa", f_probe_isa, INVALID_FCODE },
	{ NULL, NULL },
};

Retcode
install_isa(Environ *e, Package *pkg)
{
	Retcode ret;
	Byte *buf;
	Int blen;
	Package *savepkg;

	/* save the current name, if any, as "compatible" */
	if (prop_get_str(pkg->props, "name", CSTR, &buf, &blen) == NO_ERROR
			&& buf && blen)
		prop_set_str(pkg->props, "compatible", CSTR, buf, PSTR);

	/* change the name to "isa" from whatever it was before */
	ret = prop_set_str(pkg->props, "name", CSTR, "isa", CSTR);

	if (ret == NO_ERROR)
		ret = prop_set_str(pkg->props, "device_type", CSTR, "isa", CSTR);

	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "#address-cells", CSTR, 2);

	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "#size-cells", CSTR, 1);

	/* zero-length means use parent's address space */
	if (ret == NO_ERROR)
		ret = add_property(pkg->props, "ranges", CSTR, NULL, 0);

	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "eisa-slots", CSTR, 0);

	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "clock-frequency", CSTR, 8333333);

	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "slot-names", CSTR, 0);

	/* finally initialize the words and methods for this device */
	if (ret == NO_ERROR)
		ret = init_entries(e, e->names, isa_words);

	if (ret == NO_ERROR)
		ret = init_entries(e, pkg->dict, isa_methods);

	/* finally probe for and install any ISA device drivers */
	savepkg = e->currpkg;
	e->currpkg = pkg;
	ret = f_probe_isa(e);
	e->currpkg = savepkg;

	return ret;
}
