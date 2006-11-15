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

/* (c) Copyright 1997-1998 by CodeGen, Inc.  All Rights Reserved. */

/* Generic / bus-level code skeleton to be used as an aid to bring up a
   machine-dependent port.  This may be fine for most implementations
   and also serves as a template for other bus nodes.
 */

/*#define DEBUG*/

#include "defs.h"


C(f_root_open)			/* open (-- okay?) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, FTRUE);
	return NO_ERROR;
}

C(f_root_close)			/* close (--) */
{
	return NO_ERROR;
}

static Instance *
mmu_in_use(Environ *e)
{
	Retcode ret;
	uPtr inst;
	Instance *mmu;
	Package *pkg;
	Table *props;

	ret = prop_get_ptr(e->chosen->props, "mmu", CSTR, &inst);
	DPRINTF(("MMU inst %p\n", inst));

	if (ret != NO_ERROR)
		return (Instance *)0;

	DPRINTF(("Got MMU prop\n"));

	mmu = (Instance *)inst;

	if (mmu == NULL)
		return (Instance *)0;

	DPRINTF(("MMU inst not null, inst=%p\n", mmu));

	pkg = mmu->package;

	if (pkg == NULL)
		return (Instance *)0;

	DPRINTF(("MMU package not null\n"));

	props = pkg->props;

	DPRINTF(("MMU package props is %#x\n", props));

	if (props == NULL || find_table(props, "translations", CSTR) == NULL)
		return (Instance *)0;

	DPRINTF(("MMU package has translations prop\n"));
	return mmu;
}

C(f_root_map_in)		/* map-in (phys.lo ... phys.hi size -- virt) */
{
	Cell physlo, virt, size;
	Instance *mmu;
	Int mode = 0;
	Retcode ret;
	Int cells = get_address_cells(e->root);
	Int pagesize;

	IFCKSP(e, 1 + cells, 3 + cells);
	DPRINTF(("root_map_in: cells %d\n", cells));

	if ((mmu = mmu_in_use(e)) != NULL)
	{
		POP(e, size);
		physlo = TOP(e);
		DPRINTF(("root_map_in: mmu %p, physlo %#Cx, size %Cd\n", mmu, physlo, size));
		ret = prop_get_int(mmu->package->props, "page-size", CSTR, &pagesize);

		if (ret != NO_ERROR)
			return ret;

		DPRINTF(("root_map_in: page size %d\n", pagesize));

		size += physlo % pagesize;
		PUSH(e, size);

		ret = execute_method_name(e, mmu, "of-claim", CSTR);

		POP(e, virt);
		virt += physlo % pagesize;
		DPRINTF(("root_map_in: virt %#Cx\n", virt));
		PUSH(e, virt);
		PUSH(e, size);

		if (ret == NO_ERROR)
			ret = prop_get_int(mmu->package->props, "map-in-mode", CSTR, &mode);

		PUSH(e, mode);
		DPRINTF(("root_map_in: mode %#x\n", mode));

		if (ret == NO_ERROR)
			ret = execute_method_name(e, mmu, "map", CSTR);

		PUSH(e, virt);
		DPRINTF(("root_map_in: mapped at %#x\n", virt));
	}
	else
	{
		/* just use phys.lo as virt after dropping all the other values */
		POP(e, size);
		DROPN(e, cells - 1);
		ret = NO_ERROR;
	}

	return ret;
}

C(f_root_map_out)		/* map-out (virt size --) */
{
	Cell virt;
	Cell size;
	Instance *mmu;
	Retcode ret;

	IFCKSP(e, 2, 0);
	POP(e, size);
	POP(e, virt);
	DPRINTF(("root_map_out: virt %#Cx, size %#Cx\n", virt, size));

	if ((mmu = mmu_in_use(e)) == NULL)
	{
		/* real-mode so we don't need to do anything */
		DPRINTF(("root_map_out: real mode\n"));
		return NO_ERROR;
	}

	PUSH(e, virt);
	PUSH(e, size);
	ret = execute_method_name(e, mmu, "unmap", CSTR);
	DPRINTF(("root_map_out: unmap ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	PUSH(e, virt);
	PUSH(e, size);

	ret = execute_method_name(e, mmu, "of-release", CSTR);
	DPRINTF(("root_map_out: of-release ret=%d\n", ret));
	return ret;
}

/* The implementations of dma-alloc/dma-free simply call malloc/free to
   do the real work.  Other more useful work could be done here.
 */

C(f_root_dma_alloc)		/* dma-alloc (dma-len -- dma-buf) */
{
	Cell len;
#ifndef ROOT_ADDRESS_CELLS
	Byte *buf;
#else
	Retcode ret;
#endif
	Cell virt;

	IFCKSP(e, 1, 1);
	POP(e, len);

#ifdef ROOT_ADDRESS_CELLS
	ret = mmu_dma_alloc(e, len, &virt);
#else
	buf = (Byte*)malloc((size_t)len);

	if (buf == NULL)
		return E_OUT_OF_MEMORY;

	virt = (uPtr)buf;
#endif

	PUSH(e, virt);
	return NO_ERROR;
}

C(f_root_dma_free)			/* dma-free (dma-buf dma-len --) */
{
	Cell len;
	Cell buf;
	Retcode ret = NO_ERROR;

	IFCKSP(e, 2, 0);
	POP(e, len);
	POP(e, buf);

#ifdef ROOT_ADDRESS_CELLS
	ret = mmu_dma_free(e, buf, len);
#else
	if (len && buf)
		free((char*)(uPtr)buf);
	else
		ret = E_INVALID_MEMORY;
#endif

	return ret;
}

C(f_root_dma_map_in)	/* dma-map-in (... virt size cacheable? -- devaddr) */
{
	Int cacheable;
	Cell size, virt;
	Retcode ret = NO_ERROR;
#ifdef ROOT_ADDRESS_CELLS
	uInt phys[ROOT_ADDRESS_CELLS];
#endif

	IFCKSP(e, 3, 1);
	POP(e, cacheable);
	POP(e, size);
	POP(e, virt);

#ifdef ROOT_ADDRESS_CELLS
	ret = mmu_dma_map_in(e, virt, size, cacheable, phys);
	PUSH(e, phys[0]);
#else
	PUSH(e, virt);
#endif

	return ret;
}

C(f_root_dma_map_out)	/* dma-map-out (virt devaddr size --) */
{
	Cell virt, devaddr, size;
	Retcode ret = NO_ERROR;
#ifdef ROOT_ADDRESS_CELLS
	uInt phys[ROOT_ADDRESS_CELLS];
	int i;
#endif

	IFCKSP(e, 3, 0);
	POP(e, size);
	POP(e, devaddr);
	POP(e, virt);

#ifdef ROOT_ADDRESS_CELLS
	for (i = 1; i < ROOT_ADDRESS_CELLS; i++)
		phys[i] = 0;

	phys[0] = devaddr;
	ret = mmu_dma_map_out(e, virt, phys, size);
#endif

	return ret;
}

C(f_root_dma_sync)		/* dma-sync (virt devaddr size --) */
{
	Cell virt, devaddr, size;
	Retcode ret = NO_ERROR;
#ifdef ROOT_ADDRESS_CELLS
	uInt phys[ROOT_ADDRESS_CELLS];
	int i;
#endif

	IFCKSP(e, 3, 0);
	POP(e, size);
	POP(e, devaddr);
	POP(e, virt);

#ifdef ROOT_ADDRESS_CELLS
	for (i = 1; i < ROOT_ADDRESS_CELLS; i++)
		phys[i] = 0;

	phys[0] = devaddr;
	ret = mmu_dma_sync(e, virt, phys, size);
#endif

	return ret;
}

#ifndef ROOTDEV_NOCOMMASEP
C(f_root_decode_unit)		/* decode-unit (str len -- phys.lo ... phys.hi) */
{
	Byte *str;
	Int slen, i;
	Cell val, err;
	Int cells = get_address_cells(e->currpkg);

	IFCKSP(e, 2, cells);
	POP(e, slen);
	POPT(e, str, Byte*);
	setstrlen(&str, &slen);

	/* format is: HI,...,LO
	   we must extract vals onto stack in reverse order */

	for (i = 0; i < cells; i++)
	{
		parse_number(16, &str, &slen, &val, &err, FALSE);
		e->sp[cells - i] = val;

		if (*str == ',')
		{
			str++;
			slen--;
		}
	}

	e->sp += cells;
	return NO_ERROR;
}

C(f_root_encode_unit)		/* encode-unit (phys.lo ... phys.hi -- str len) */
{
	static Byte buf[STR_SIZE];
	Cell val;
	Byte *s = buf;
	Int cells = get_address_cells(e->currpkg);
	Int i;

	IFCKSP(e, cells, 2);
	*s = '\0';

	for (i = 0; i < cells; i++)
	{
		POP(e, val);
		bprintf(s, (i == 0) ? "%x" : ",%x", (unsigned int)val);
		s += strlen(s);
	}

	PUSHP(e, buf);
	PUSH(e, strlen(buf));
	return NO_ERROR;
}
#else

C(f_root_decode_unit)		/* decode-unit (str len -- phys.lo ... phys.hi) */
{
	Byte *str;
	Byte *str2;
	Int slen;
	Int slen2;
	Int i;
	Cell val;
	Cell err;
	Int cells = get_address_cells(e->currpkg);

	IFCKSP(e, 2, cells);

	POP(e, slen);
	POPT(e, str, Byte*);
	setstrlen(&str, &slen);

	/* format is: HI...LO
	   we must extract vals onto stack in reverse order */

	str2 = str + slen;
	slen2 = 0;

	for (i = 0; i < cells; i++)
	{
		Byte *t;
		Int tl;

		/* parse from the end of the string */
		if (str2 - str > 8)
		{
			str2 -= 8;
			slen2 = 8;
		}
		else
		{
			slen2 = str2 - str;
			str2 = str;
		}

		t = str2;
		tl = slen2;

		if (slen2)
		    parse_number(16, &t, &tl, &val, &err, FALSE);
		else
		    val = 0;

		e->sp[i + 1] = val;
	}

	e->sp += cells;
	return NO_ERROR;
}

C(f_root_encode_unit)		/* encode-unit (phys.lo ... phys.hi -- str len) */
{
	static Byte buf[STR_SIZE];
	Cell val;
	Byte *s = buf;
	Int cells = get_address_cells(e->currpkg);
	Int i;
	int nz = 0;

	IFCKSP(e, cells, 2);
	*s = '\0';

	for (i = 0; i < cells; i++)
	{
		POP(e, val);

		if (val || nz || i + 1 == cells)
			bprintf(s, nz ? "%08x" : "%x", (unsigned int)val);

		if (val)
			nz = 1;

		s += strlen(s);
	}

	PUSHP(e, buf);
	PUSH(e, strlen(buf));
	return NO_ERROR;
}
#endif


/* probe-self (arg-str arg-len reg-str reg-len fcode-str fcode-len --) */
C(f_root_probe_self)
{
	Retcode ret;
	Byte *args;
	Int alen;
	Cell fcode;
	Cell device[MAX_ADDR_CELLS];
	Int cells = get_address_cells(e->currpkg);
	Int i;
	Byte str[STR_SIZE];

	IFCKSP(e, 6, 0);

	/* convert fcode unit addr to phys.lo...phys.hi values on the stack */
	ret = f_root_decode_unit(e);

	if (ret != NO_ERROR)
		return ret;

	/* and map it in */
	PUSH(e, 0x10000);
	ret = f_root_map_in(e);

	if (ret != NO_ERROR)
		return ret;

	POP(e, fcode);

	/* now convert device string to phys.lo...phys.hi values on stack */
	ret = f_root_decode_unit(e);

	if (ret != NO_ERROR)
		return ret;

	for (i = 0; i < cells; i++)
		POP(e, device[i]);

	/* pop off rest of args passed to us */
	POP(e, alen);
	POPT(e, args, Byte*);

	/* copy args str into buffer to convert it to a PSTR */
	str[0] = (uByte)alen;
	memcpy(str + 1, args, alen);

	if (ret == NO_ERROR)
		ret = execute_word(e, "new-device");

	/* set new instance arguments */
	if (ret == NO_ERROR)
		((Instance*)(uPtr)e->currinst)->args = str;

	/* run the fcode */
	if (ret == NO_ERROR)
		ret = interp_fcode(e, (Byte*)(uPtr)fcode, MAGIC_FNEXT);

	/* clean up */
	PUSH(e, fcode);
	PUSH(e, 0x10000);
	ret = f_root_map_out(e);

	if (ret == NO_ERROR)
		ret = execute_word(e, "finish-device");

	return NO_ERROR;
}


static const Initentry root_methods[] =
{
	{ "open", f_root_open, INVALID_FCODE },
	{ "close", f_root_close, INVALID_FCODE },
	{ "map-in", f_root_map_in, INVALID_FCODE },
	{ "map-out", f_root_map_out, INVALID_FCODE },
	{ "dma-alloc", f_root_dma_alloc, INVALID_FCODE },
	{ "dma-free", f_root_dma_free, INVALID_FCODE },
	{ "dma-map-in", f_root_dma_map_in, INVALID_FCODE },
	{ "dma-map-out", f_root_dma_map_out, INVALID_FCODE },
	{ "dma-sync", f_root_dma_sync, INVALID_FCODE },
	{ "encode-unit", f_root_encode_unit, INVALID_FCODE },
	{ "decode-unit", f_root_decode_unit, INVALID_FCODE },
	{ "probe-self", f_root_probe_self, INVALID_FCODE },
	{ NULL,             NULL },
};


CC(install_root)
{
	Package *pkg = e->root;		/* already initialized by init_environ() */

	DPRINTF(("install_root: pkg=%p\n", pkg));

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	/* set #address-cells to something besides the default of 2 if desired */
#ifdef ROOT_ADDRESS_CELLS
	prop_set_int(pkg->props, "#address-cells", CSTR, ROOT_ADDRESS_CELLS);
	prop_set_int(pkg->props, "#size-cells", CSTR, 1);
#else
#ifdef DEFAULT_ADDRESS_CELLS
	prop_set_int(pkg->props, "#address-cells", CSTR, DEFAULT_ADDRESS_CELLS);
#endif
#ifdef DEFAULT_SIZE_CELLS
	prop_set_int(pkg->props, "#size-cells", CSTR, DEFAULT_SIZE_CELLS);
#endif
#endif

	return init_entries(e, pkg->dict, root_methods);
}
