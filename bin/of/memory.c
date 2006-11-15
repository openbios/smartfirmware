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

/* (c) Copyright 1996-1997,2001 by CodeGen, Inc.  All Rights Reserved. */

/* This initializes the /memory node, and marks the first
   g_machine_memory_used bytes as already allocated.  This should be
   called after the Forth environment is initialized but before other
   packages are initialized, so it's slipped into "install_list" in
   machdep.c before the other packages.  We use a variety of static
   vars here since this package should only be created once.
 */

#include "defs.h"


/* claim and release use the "available" property as a free list to
   allocate memory.  To make things easy, we use a static list of
   available memory, then encode that into "available" after performing
   the operation.
 */

static Allocator *g_memory_allocator = NULL;

static Retcode
make_available(Environ *e)
{
	Int cells = get_address_cells(e->root);
	Int scells = get_size_cells(e->root);
	Retcode ret;
	Byte *prop;
	Int plen = 0;
	Int i;
	Allocator *a = g_memory_allocator;
	Allocator_block *b = NULL;
	uInt *addr = a->u1;
	uInt *size = a->u2;

	DPRINTF(("make_available: e=%p memory=%p currinst=%p cells=%d=%p\n",
			e, e->memory, e->currinst, cells));

	alloc_info(a, &b, addr, size);
	
	for (i = 1; b; alloc_info(a, &b, addr, size), i++)
		;

	prop = prop_alloc(e, i * 2 * sizeof (Int));

	DPRINTF(("make_available: prop=%p\n", prop));

	if (prop == NULL)
		return E_OUT_OF_MEMORY;

	b = NULL;
	alloc_info(a, &b, addr, size);

	for (; b; alloc_info(a, &b, addr, size))
	{
		/* first encode the address */
		for (i = cells - 1; i >= 0; i--)
			prop_encode_int(prop + plen, &plen, addr[i]);
		
		for (i = scells - 1; i >= 0; i--)
			prop_encode_int(prop + plen, &plen, size[i]);
	}
	
	DPRINTF(("make_available: add-prop available: prop=%p plen=%d\n",
			prop, plen));
	ret = add_property(e->memory->props, "available", CSTR, prop, plen);
	DPRINTF(("make_available: return %s (%d)\n", err2str(ret), ret));
	return ret;
}

C(f_memory_claim)		/* ([phys.lo ... phys.hi] size align -- base-addr) */
{
	Int cells = get_address_cells(e->root);
	Int scells = get_size_cells(e->root);
	Cell sz;
	Cell algn;
	Cell t;
	Cell base;
	Int i;
	Allocator *a = g_memory_allocator;
	uInt *addr = a->u1;
	uInt *size = a->u2;
	uInt *align = a->u3;
	Retcode ret;
	Retcode ret2;
	
	IFCKSP(e, 2, 1);
	POP(e, algn);
	POP(e, sz);

	t = algn;

	for (i = 0; i < scells; i++)
	{
		size[i] = sz;
		align[i] = t;
#ifdef SF_64BIT
		sz >>= 32;
		t >>= 32;
#else
		sz = 0;
		t = 0;
#endif
	}
	
	if (algn == 0)
	{
		IFCKSP(e, cells, 1);

		for (i = cells - 1; i >= 0; i--)
			POPT(e, addr[i], uInt);

		ret = alloc_fixed(a, addr, size);
	}
	else
	{
		ret = alloc_aligned(a, align, size, addr);
	}

	base = 0;

	if (ret == NO_ERROR)
		for (i = cells - 1; i >= 0; i--)
		{
#ifdef SF_64BIT
			base <<= 32;
#else
			base = 0;
#endif
			base |= addr[i];
		}
	
	DPRINTF(("memory-claim: align=%#x size=%#x cells=%d base=%p\n",
			align, size, cells, base));

	PUSH(e, base);
	ret2 = make_available(e);
	return ret != NO_ERROR ? ret : ret2;
}

C(f_memory_release)			/* (phys.lo ... phys.hi size --) */
{
	Int cells = get_address_cells(e->root);
	Int scells = get_size_cells(e->root);
	Int i;
	Cell sz;
	Allocator *a = g_memory_allocator;
	uInt *addr = a->u1;
	uInt *size = a->u2;
	Retcode ret;
	Retcode ret2;

	IFCKSP(e, cells + 1, 0);
	POP(e, sz);

	for (i = 0; i < scells; i++)
	{
		size[i] = sz;
#ifdef SF_64BIT
		sz >>= 32;
#else
		sz = 0;
#endif
	}
	
	for (i = cells - 1; i >= 0; i--)
		POPT(e, addr[i], uInt);

	ret = free_block(a, addr, size);
	ret2 = make_available(e);
	return ret != NO_ERROR ? ret : ret2;
}

static Retcode
do_mem_test(Environ *e, Byte *addr, uInt len, char *mesg, Bool virt)
{
	Retcode ret;
	Int fail;
	Int cells = get_address_cells(e->root);
	int i;

	if (!virt)
	{
		PUSHP(e, addr);
	
		for (i = 1; i < cells; i++)
			PUSH(e, 0);

		PUSH(e, len);
		ret = execute_static_method_name(e, e->root, "map-in", CSTR);

		if (ret != NO_ERROR)
			return ret;

		POPT(e, addr, Byte *);
	}

	PUSHP(e, addr);
	PUSH(e, len);
	ret = execute_word(e, "memory-test-suite");

	if (ret != NO_ERROR)
		return ret;

	POP(e, fail);

	if (!virt)
	{
		PUSHP(e, addr);
		PUSH(e, len);
		ret = execute_static_method_name(e, e->root, "map-out", CSTR);

		if (ret != NO_ERROR)
			return ret;
	}

	if (fail)
	{
		cprintf(e, "failure during test of %s\n", mesg);
		return E_MEM_TEST_FAIL;
	}

	return NO_ERROR;
}

C(f_memory_selftest)			/* selftest (-- 0 | error-code) */
{
	Bool diag;
	Allocator *a = g_memory_allocator;
	Allocator_block *b = NULL;
	Retcode ret = NO_ERROR;
	uInt *addr = a->u1;
	uInt *size = a->u2;

	IFCKSP(e, 0, 2);
	e->mask = ~0;		/* test all bits */

	diag = diagnostic_mode(e);

	if (diag)
		cprintf(e, "testing unclaimed /memory...\n");

	alloc_info(a, &b, addr, size);

	/* test all memory in free list */
	for (; b; alloc_info(a, &b, addr, size))
		ret = do_mem_test(e, (Byte *)(uPtr)addr[0], size[0],
				"unclaimed /memory", FALSE);

	/* test all free memory in Forth environment */
	if (diag)
		cprintf(e, "testing unused Forth memory...\n");

	if (ret == NO_ERROR)
		ret = do_mem_test(e, e->fbrk, MEM_SIZE - (e->fbrk - e->fmem),
				"Forth memory", TRUE);

	PUSH(e, ret);
	return NO_ERROR;
}

C(f_memory_open)			/* open (-- okay?) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, FTRUE);
	return NO_ERROR;
}

C(f_memory_close)			/* close (--) */
{
	return NO_ERROR;
}


static const Initentry memory_methods[] =
{
	{ "open",          f_memory_open,           INVALID_FCODE },
	{ "close",         f_memory_close,          INVALID_FCODE },
	{ "claim",         f_memory_claim,          INVALID_FCODE },
	{ "release",       f_memory_release,        INVALID_FCODE },
	{ "selftest",      f_memory_selftest,       INVALID_FCODE },
	{ NULL,            NULL },
};

/* initialize and setup the "/memory" package, for lack of a
   better place to put it
 */
CC(install_memory)
{
	Package *pkg = new_pkg_name(e->root, "memory");
	Package *savepkg = e->currpkg;
	Retcode ret;
	Instance *inst;
	Int cells = get_address_cells(e->root);
	Int scells = get_size_cells(e->root);
	Allocator *a;
	uInt *addr;
	uInt *size;
	Cell tmp;
	Int i;
	
	DPRINTF(("install_memory: pkg=%p cells=%d...\n", pkg, cells));
	e->currpkg = pkg;
	e->memory = pkg;
	prop_set_str(pkg->props, "device_type", CSTR, "memory", CSTR);
	DPRINTF(("install_memory: init_entries...\n"));
	ret = init_entries(e, pkg->dict, memory_methods);
	
	if (ret != NO_ERROR)
		return ret;

	/* create the "reg" property using the two g_machine_memory_* vars */
	PUSHP(e, g_machine_memory);

	for (i = 1; i < cells; i++)
		PUSH(e, 0);

	PUSH(e, g_machine_memory_size);

	for (i = 1; i < scells; i++)
		PUSH(e, 0);

	DPRINTF(("install_memory: create reg\n"));
	ret = execute_word(e, "reg");
	e->currpkg = savepkg;

	if (ret != NO_ERROR)
		return ret;
	
	ret = alloc_init(&a, cells, scells);
	
	if (ret != NO_ERROR)
		return ret;

	addr = a->u1;
	size = a->u2;
	tmp = (Cell)(uPtr)g_machine_memory;

	for (i = 0; i < cells; i++)
	{
		addr[i] = tmp;
#ifdef SF_64BIT
		tmp >>= 32;
#else
		tmp = 0;
#endif
	}

	tmp = g_machine_memory_size;

	for (i = 0; i < cells; i++)
	{
		size[i] = tmp;
#ifdef SF_64BIT
		tmp >>= 32;
#else
		tmp = 0;
#endif
	}

	/* initialilze our global free list */
	ret = free_block(a, addr, size);
	
	if (ret != NO_ERROR)
		return ret;

	g_memory_allocator = a;
	
	/* create the "available" property */
	DPRINTF(("install_memory: make_available: mach_mem=%p size=%#x\n",
			g_machine_memory, g_machine_memory_size));
	ret = make_available(e);
	
	if (ret != NO_ERROR)
		return ret;
	
	/* open this device to create an instance for /chosen property "memory" */
	PUSH(e, "/memory");
	PUSH(e, 7);
	DPRINTF(("install_memory: open-dev\n"));
	ret = execute_word(e, "open-dev");
	
	if (ret != NO_ERROR)
		return ret;
	
	POPT(e, inst, Instance*);
	e->currpkg = savepkg;
	ret = prop_set_ptr(e->chosen->props, "memory", CSTR, (uPtr)inst);
	
	if (ret != NO_ERROR)
		return ret;

	/* allocate the amount reserved for the malloc pool */
	PUSHP(e, g_machine_memory + g_machine_memory_offset);
	
	for (i = 1; i < cells; i++)
		PUSH(e, 0);
	
	PUSH(e, g_machine_memory_used);
	PUSH(e, 0);			/* align of 0 means use the requested memory address */
	DPRINTF(("install_memory: memory-claim\n"));
	ret = f_memory_claim(e);

	if (ret != NO_ERROR)
		return ret;
	
	/* sanity check */
	if ((Byte*)(uPtr)TOP(e) != g_machine_memory + g_machine_memory_offset)
		return E_INIT_ERROR;
	
	DROP(e);
	DPRINTF(("install_memory: done\n"));
	return NO_ERROR;
}
