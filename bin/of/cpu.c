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

/* (c) Copyright 1997 by CodeGen, Inc.  All Rights Reserved. */

/* Generic (non-functional) /cpu code skeleton with MMU support to be used
   as an aid to bring up a machine-dependent port.
 */

#include "defs.h"


/* static free list for demo purposes, much like /memory node */
#define MAX_FREE	64

static struct free_list
{
	Byte *mem;
	uInt size;
} g_free_list[MAX_FREE];


static Retcode
make_reg_prop(Environ *e, Package *mmu, Byte *propname)
{
	Package *savepkg = e->currpkg;
	Cell saveinst = e->currinst;
	Retcode ret;
	Byte *prop = e->fbrk;
	Int plen = 0;
	Int i, j;

	e->currpkg = mmu;
	e->currinst = (uPtr)NULL;
	
	for (i = 0; i < MAX_FREE; i++)
	{
		if (g_free_list[i].size == 0)
			continue;
		
		/* first encode the single-cell virtual address */
		PUSHP(e, g_free_list[i].mem);
		ret = execute_word(e, "encode-int");
		
		if (ret != NO_ERROR)
			return ret;
		
		POP(e, j);		/* plen */
		DROP(e);		/* paddr */
		plen += j;
		
		/* then encode its size */
		PUSH(e, g_free_list[i].size);
		execute_word(e, "encode-int");
		
		POP(e, j);		/* plen */
		DROP(e);		/* paddr */
		plen += j;
	}
	
	ret = add_property(mmu->props, propname, CSTR, prop, plen);
	e->currpkg = savepkg;
	e->currinst = saveinst;
	return ret;
}

static Retcode
make_available(Environ *e, Package *mmu)
{
	return make_reg_prop(e, mmu, "available");
}

static Retcode
make_existing(Environ *e, Package *mmu)
{
	return make_reg_prop(e, mmu, "existing");
}


C(f_mmu_claim)		/* ([virt] size align -- base) */
{
	Int align;
	uInt size;
	Byte *reqaddr = NULL;
	Int i, j;
	struct free_list *f;
	
	IFCKSP(e, 2, 1);
	POP(e, align);
	POPTYPE(e, size, uInt);
	
	if (align == 0)
		POPT(e, reqaddr, Byte*);
	
	for (i = 0; i < MAX_FREE; i++)
	{
		f = &g_free_list[i];
		
		if (f->size == 0)
			continue;
		
		if (reqaddr && f->mem <= reqaddr &&
				f->size - (reqaddr - f->mem) >= size)
		{
			/* need to split this block? */
			if (f->size - (reqaddr - f->mem) - size > 0)
			{
				for (j = 0; j < MAX_FREE; j++)
					if (g_free_list[j].size == 0)		/* find a free slot */
						break;
				
				if (j >= MAX_FREE)
					return E_OUT_OF_MEMORY;
				
				g_free_list[j].mem = reqaddr + size;	/* leftover at tail */
				g_free_list[j].size = f->size - size - (reqaddr - f->mem);
			}
			
			f->size = reqaddr - f->mem;		/* leftover at the head */
			break;
		}
		else if (f->size >= size)
		{
			reqaddr = f->mem;
			
			if ((uPtr)reqaddr % align)		/* aligned as requested? */
			{
				reqaddr += align - (uPtr)reqaddr % align;
				i = size + align - (uPtr)reqaddr % align;
				
				if (f->size < i)	/* make sure there's still enough room */
					continue;
				
				size = i;
			}
			
			f->size -= size;
			f->mem += size;
		}
	}
	
	PUSHP(e, reqaddr);
	return make_available(e, e->currpkg);
}

C(f_mmu_release)			/* (virt size --) */
{
	Int i;
	uInt size;
	Byte *mem;

	IFCKSP(e, 2, 0);
	POPTYPE(e, size, uInt);
	POPT(e, mem, Byte*);
	
	/* find a free slot - ought to coalesce adjacent blocks */
	for (i = 0; i < MAX_FREE; i++)
		if (g_free_list[i].size == 0)
			break;
	
	if (i >= MAX_FREE)
		return E_OUT_OF_MEMORY;
	
	g_free_list[i].mem = mem;
	g_free_list[i].size = size;
	return make_available(e, e->currpkg);
}

C(f_mmu_map)		/* map (phys.lo ... phys.hi virt size mode --) */
{
	Int cells = get_address_cells(e->currpkg->parent);
	Byte *virt;
	Int size, mode;
	Int lo, hi;

	IFCKSP(e, cells + 3, 0);
	POP(e, mode);
	POP(e, size);
	POPT(e, virt, Byte*);

	/* hack to get only the hi and lo cells - machine-dependent */
	POP(e, lo);
	DROPN(e, cells - 2);
	POP(e, hi);

	/* perform machine-dependent mapping here */

	return NO_ERROR;
}

C(f_mmu_unmap)		/* unmap (virt size --) */
{
	Byte *virt;
	Int size;

	IFCKSP(e, 2, 0);
	POP(e, size);
	POPT(e, virt, Byte*);

	/* perform machine-dependent unmapping here */

	return NO_ERROR;
}

C(f_mmu_modify)		/* modify (virt size mode --) */
{
	Byte *virt;
	Int size, mode;

	IFCKSP(e, 3, 0);
	POP(e, mode);
	POP(e, size);
	POPT(e, virt, Byte*);

	/* perform machine-dependent modification here */

	return NO_ERROR;
}

/* translate (virt -- false | physlo .. phys.hi mode true) */
C(f_mmu_translate)
{
	Byte *virt;
	Int cells = get_address_cells(e->currpkg->parent);
	Int i;
	Int lo = 0, hi = 0, mode = 0;
	Retcode ret = NO_ERROR;

	IFCKSP(e, 1, cells + 2);
	POPT(e, virt, Byte*);

	/* perform machine-dependent address translation here */

	if (ret == NO_ERROR)		/* no translation error */
	{
		PUSH(e, lo);

		for (i = 2; i < cells; i++)
			PUSH(e, 0);

		PUSH(e, hi);
		PUSH(e, mode);
		PUSH(e, FTRUE);
	}
	else
		PUSH(e, FFALSE);		/* translation error */

	return ret;
}


C(f_cpu_open)					/* open (-- okay?) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, FTRUE);
	return NO_ERROR;
}

C(f_cpu_close)					/* close (--) */
{
	return NO_ERROR;
}

C(f_cpu_selftest)				/* selftest (--) */
{
	Bool diag = diagnostic_mode(e);

	if (diag)
		cprintf(e, "cpu selftest...\n");

	PUSH(e, 0);			/* successful */
	return NO_ERROR;
}


/* methods for the /cpu node */
static const Initentry cpu_methods[] =
{
	{ "open",          f_cpu_open,         INVALID_FCODE },
	{ "close",         f_cpu_close,        INVALID_FCODE },
	{ "selftest",      f_cpu_selftest,     INVALID_FCODE },

	/* addition nodes for MMU support, if desired */
	{ "claim",         f_mmu_claim,        INVALID_FCODE },
	{ "release",       f_mmu_release,      INVALID_FCODE },
	{ "map",           f_mmu_map,          INVALID_FCODE },
	{ "unmap",         f_mmu_unmap,        INVALID_FCODE },
	{ "modify",        f_mmu_modify,       INVALID_FCODE },
	{ "translate",     f_mmu_translate,    INVALID_FCODE },
	{ NULL,            NULL },
};


CC(install_cpu)
{
	Package *pkg = new_pkg_name(e->root, "cpu");
	Package *savepkg = e->currpkg;
	Int cells = get_address_cells(e->root);
	Int i;
	Instance *inst;
	Retcode ret;

	/* create the /cpu node */

	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	ret = init_entries(e, pkg->dict, cpu_methods);

	if (ret == NO_ERROR)
		prop_set_str(pkg->props, "device_type", CSTR, "cpu/mmu", CSTR);

	/* create the "reg" property - no meaningful values here */
	for (i = 0; i < cells; i++)
		PUSH(e, 0);			/* phys.* */
	
	PUSH(e, 0);				/* size */
	e->currpkg = pkg;
	ret = execute_word(e, "reg");
	e->currpkg = savepkg;
	
	/* initialize these to all of memory - machine-dependent */
	memset(g_free_list, 0, sizeof g_free_list);
	g_free_list[0].mem = g_machine_memory;
	g_free_list[0].size = g_machine_memory_size;
	
	/* initialize the "available" and "existing" properties */
	if (ret == NO_ERROR)
		ret = make_available(e, pkg);

	if (ret == NO_ERROR)
		ret = make_existing(e, pkg);
	
	if (ret != NO_ERROR)
		return ret;
	
	/* open this device to create an instance for the /chosen property "mmu" */
	PUSH(e, "/cpu");
	PUSH(e, 4);
	ret = execute_word(e, "open-dev");
	
	if (ret != NO_ERROR)
		return ret;
	
	POPT(e, inst, Instance*);
	e->currpkg = savepkg;
	ret = prop_set_ptr(e->chosen->props, "mmu", CSTR, (uPtr)inst);
	
	return ret;
}
