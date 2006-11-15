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

/* (c) Copyright 1997,1999,2001,2003 by CodeGen, Inc.  All Rights Reserved. */

/* /cpu code for AMD64
   Needs customizing, especially for clock frequency, time-base, etc.
   See comments marked "CUSTOMIZE" and amd64_props array below.
 */

/*#define DEBUG*/

#include "defs.h"

#define PAGE_SIZE		4096
#define PAGE_MASK		(PAGE_SIZE - 1)

static Allocator *g_virt_allocator;
static Allocator *g_of_virt_allocator;

uInt
amd64_get_version()
{
	return 0x00030000;		/* steping C0 */
}

static Retcode
make_reg_prop(Environ *e, Allocator *a, Byte *propname)
{
	Package *savepkg = e->currpkg;
	Cell saveinst = e->currinst;
	Retcode ret;
	Byte *prop;
	Int plen = 0;
	Int i;
	Allocator_block *b = NULL;
	uInt addr[2];
	uInt size[2];

	e->currpkg = e->mmu;
	e->currinst = (uPtr)NULL;

	alloc_info(a, &b, addr, size);
	
	for (i = 1; b; alloc_info(a, &b, addr, size), i++)
		;

	prop = prop_alloc(e, i * 4 * sizeof (Int));

	b = NULL;
	alloc_info(a, &b, addr, size);
	
	for (i = 1; b; alloc_info(a, &b, addr, size), i++)
	{
		DPRINTF(("make_reg_prop: addr=%#X,%#X size=%#X,%#X\n", addr[1], addr[0], size[1], size[0]));
		/* first encode the double-cell virtual address */
		prop_encode_int(prop + plen, &plen, addr[1]);
		prop_encode_int(prop + plen, &plen, addr[0]);
		/* next encode the double-cell block size */
		prop_encode_int(prop + plen, &plen, size[1]);
		prop_encode_int(prop + plen, &plen, size[0]);
	}
	
	ret = add_property(e->mmu->props, propname, CSTR, prop, plen);
	e->currpkg = savepkg;
	e->currinst = saveinst;
	return ret;
}

static Retcode
make_available(Environ *e)
{
	return make_reg_prop(e, g_virt_allocator, "available");
}

static Retcode
make_of_available(Environ *e)
{
	return make_reg_prop(e, g_of_virt_allocator, "of-available");
}

static Retcode
make_existing(Environ *e)
{
	return make_reg_prop(e, g_virt_allocator, "existing");
}

static Bool
check_callback(Environ *e)
{
	Cell array[10];
	int cret;

	DPRINTF(("In check_callback:\n"));

	if (e->callback == NULL)
		return FALSE;

	array[0] = (Cell)"translate";
	array[1] = 1;		/* one argument */
	array[2] = 4;		/* one return */
	array[3] = (Cell)0xFFFFFFEF00000000;
	cret = machine_callback(g_e, g_e->callback, array);
	DPRINTF(("check_callback: cret=%d ret=%d\n", cret, array[4]));
	return cret == 4 && array[4] == 0;
}

C(f_mmu_claim)		/* ([virt] size align -- base) */
{
	uLong align;
	uLong size;
	uLong addr;
	uInt offset;
	Allocator *a = g_virt_allocator;
	Retcode ret = NO_ERROR;
	Retcode ret2;
	
	IFCKSP(e, 2, 1);
	POPTYPE(e, align, uLong);
	POPTYPE(e, size, uLong);
	
	if (align == 0)
	{
		POPTYPE(e, addr, uLong);

		/* ajdust to include begining and ends of first and last pages */
		offset = addr & PAGE_MASK;
		addr -= offset;
		size += offset;
		size = (size + PAGE_SIZE - 1) & ~PAGE_MASK;

		if (check_callback(e))
		{
			Cell array[12];
			int cret;

			DPRINTF(("f_mmu_claim: calling back to client\n"));

			/* use client callback to map in the space */
			array[0] = (Cell)"claim-virt";
			array[1] = 4;
			array[2] = 3;
			array[3] = (Cell)addr;
			array[4] = (Cell)addr;
			array[5] = (Cell)size;
			array[6] = (Cell)PAGE_SIZE;
			cret = machine_callback(e, e->callback, array);
			DPRINTF(("f_mmu_claim: cret = %d, ret = %d\n", cret, array[7]));

			if (array[7] != NO_ERROR)
				ret = array[7];
			else if (array[8] != 0)
				addr = 0;
			else
				addr = array[9];
		}
		else
			ret = alloc_fixed(a, (uInt *)&addr, (uInt *)&size);
	}
	else if (size == 0)
	{
		addr = 0;
		ret = NO_ERROR;
	}
	else
	{
		size = (size + PAGE_SIZE - 1) & ~PAGE_MASK;

		if (check_callback(e))
		{
			Cell array[12];
			int cret;

			DPRINTF(("f_mmu_claim: calling back to client\n"));

			/* use client callback to map in the space */
			array[0] = (Cell)"claim-virt";
			array[1] = 4;
			array[2] = 3;
			array[3] = (Cell)0;
			array[4] = (Cell)0 - size;
			array[5] = (Cell)size;
			array[6] = (Cell)align;
			cret = machine_callback(e, e->callback, array);
			DPRINTF(("f_mmu_claim: cret = %d, ret = %d\n", cret, array[7]));

			if (array[7] != NO_ERROR)
				ret = array[7];
			else if (array[8] != 0)
				addr = 0;
			else
				addr = array[9];
		}
		else
		{
			uLong bigalign = 0x200000;

			ret = E_OUT_OF_STORAGE;

			/* attempt to large page align the block if it is big and the */
			/* alignment works out. */
			if (size > bigalign && (bigalign % align) == 0)
			{
				ret = alloc_aligned(a, (uInt *)&bigalign, (uInt *)&size, (uInt *)&addr);
			}
	
			if (ret != NO_ERROR)
				ret = alloc_aligned(a, (uInt *)&align, (uInt *)&size, (uInt *)&addr);
		}
	}

	PUSH(e, addr);
	ret2 = make_available(e);
	return ret != NO_ERROR ? ret : ret2;
}

C(f_mmu_release)			/* (virt size --) */
{
	Allocator *a = g_virt_allocator;
	uLong size;
	uLong addr;
	uInt offset;
	Retcode ret;
	Retcode ret2;

	IFCKSP(e, 2, 0);
	POPTYPE(e, size, uLong);
	POPTYPE(e, addr, uLong);

	offset = addr & PAGE_MASK;
	addr -= offset;
	size += offset;
	size = (size + PAGE_SIZE - 1) & ~PAGE_MASK;

	if (check_callback(e))
	{
		Cell array[12];
		int cret;

		DPRINTF(("f_mmu_release: calling back to client\n"));

		/* use client callback to map in the space */
		array[0] = (Cell)"release-virt";
		array[1] = 2;
		array[2] = 1;
		array[3] = (Cell)addr;
		array[4] = (Cell)size;
		cret = machine_callback(e, e->callback, array);
		DPRINTF(("f_mmu_release: cret = %d, ret = %d\n", cret, array[5]));
		return array[5];
	}

	ret = free_block(a, (uInt *)&addr, (uInt *)&size);
	ret2 =  make_available(e);
	return ret != NO_ERROR ? ret : ret2;
}

C(f_mmu_of_claim)		/* (size -- base) */
{
	uLong size;
	uLong addr;
	Retcode ret;
	Retcode ret2 = NO_ERROR;
	uLong align = PAGE_SIZE;
	
	IFCKSP(e, 1, 1);
	POPTYPE(e, size, uLong);

	size = (size + PAGE_SIZE - 1) & ~PAGE_MASK;
	
	if (size == 0)
	{
		addr = 0;
		ret = NO_ERROR;
	}
	else
	{
		DPRINTF(("f_mmu_of_claim: g_of_virt_allocator %p\n", g_of_virt_allocator));
		ret = alloc_aligned(g_of_virt_allocator, (uInt *)&align, (uInt *)&size, (uInt *)&addr);
		ret2 = make_of_available(e);
		DPRINTF(("f_mmu_of_claim: addr %#x,%#x size %#x,%#x\n", 
			((uInt *)&addr)[1], ((uInt *)&addr)[0],
			((uInt *)&size)[1], ((uInt *)&size)[0]));
	}

	PUSH(e, addr);
	return ret != NO_ERROR ? ret : ret2;
}

C(f_mmu_of_release)			/* (virt size --) */
{
	uInt offset;
	uLong size;
	uLong addr;
	Retcode ret;
	Retcode ret2;

	IFCKSP(e, 2, 0);
	POPTYPE(e, size, uLong);
	POPTYPE(e, addr, uLong);

	offset = addr & PAGE_MASK;
	size += offset;
	addr -= offset;
	size = (size + PAGE_SIZE - 1) & ~PAGE_MASK;

	ret = free_block(g_of_virt_allocator, (uInt *)&addr, (uInt *)&size);
	ret2 =  make_of_available(e);

	return ret != NO_ERROR ? ret : ret2;
}

struct translation
{
	struct translation *next;
	uLong phys;
	uLong virt;
	uLong size;
	uInt mode;
};

typedef struct translation Translation;

static Translation *g_translations;

static Retcode
remove_translation(Environ *e, Cell virt, Cell size)
{
	Translation *t;
	Translation *p = NULL;
	Translation *n;
	uInt over;

	if (size == 0)
		return NO_ERROR;

	for (t = g_translations; t; p = t, t = t->next)
	{
		if (virt <= t->virt && t->virt < virt + size)
		{
			/* t intersects virt:size */
			/* reduce the size of t so that it no longer intersects */
			over = (virt + size) - t->virt;
			t->size -= over;
			t->virt += over;
			t->phys += over;
		}
		else if (t->virt < virt && virt < t->virt + t->size)
		{
			/* virt intersects t */
			if (virt + size < t->virt + t->size)
			{
				/* split the block in two */
				n = (Translation *)malloc(sizeof (Translation));

				if (n == NULL)
					return E_OUT_OF_MEMORY;

				n->virt = virt + size;
				n->size = (t->virt + t->size) - (virt + size);
				n->phys = t->phys + t->size - n->size;
				n->mode = t->mode;
				n->next = t->next;
				t->next = n;
				t->size -= n->size + size;
				return NO_ERROR;
			}

			if (virt + size == t->virt + t->size)
			{
				t->size -= size;
				return NO_ERROR;
			}

			t->size = virt - t->virt;
		}
	}

	/* remove zero size blocks that may have been created above */
	for (p = NULL, t = g_translations; t; p = t, t = n)
	{
		n = t->next;

		if (t->size == 0)
		{
			if (p)
				p->next = n;
			else
				g_translations = n;

			free(t);
			t = p;
		}
	}

	return NO_ERROR;
}

Retcode
add_translation(Environ *e, uLong phys, Cell virt, Cell size, Int mode)
{
	Translation *t;
	Translation *p = NULL;
	Translation *n;
	Retcode ret;

	if (size == 0)
		return NO_ERROR;

	ret = remove_translation(e, virt, size);

	if (ret != NO_ERROR)
		return ret;

	for (t = g_translations; t; p = t, t = t->next)
		if (virt < t->virt)
			break;

	t = (Translation *)malloc(sizeof (Translation));

	if (t == NULL)
		return E_OUT_OF_MEMORY;

	t->virt = virt;
	t->phys = phys;
	t->size = size;
	t->mode = mode;

	if (p)
	{
		if (p->virt + p->size == virt && p->mode == mode &&
				p->phys + p->size == phys)
		{
			/* merge p and t */
			p->size += size;
			free(t);
			t = p;
		}
		else
		{
			t->next = p->next;
			p->next = t;
		}

	}
	else
	{
		t->next = g_translations;
		g_translations = t;
	}

	n = t->next;

	if (n && t->virt + t->size == n->virt && t->mode == n->mode &&
		    t->phys + t->size == n->phys)
	{
		/* merge t and n */
		t->size += n->size;
		t->next = n->next;
		free(n);
	}

	return NO_ERROR;
}

static Retcode
modify_translations(Environ *e, Cell virt, Cell size, Int mode)
{
	Translation *t;
	int changed = 1;

	/* this is a really sucky algorithm but it works, we modify one */
	/* block per pass until no more blocks need to be modified */
	while (changed)
	{
		changed = 0;
	
		for (t = g_translations; t; t = t->next)
		{
			if (virt <= t->virt && t->virt < virt + size)
			{
				Cell s = (virt + size) - t->virt;
				Cell p;
				Cell v;

				if (s >= t->size)
				{
					t->mode = mode;
					continue;
				}

				v = t->virt;
				p = t->phys;
				remove_translation(e, v, s);
				add_translation(e, v, p, s, mode);
				changed = 1;
				break;
			}
			else if (t->virt < virt && virt < t->virt + t->size)
			{
				Cell s = (t->virt + t->size) - virt;
				Cell p;

				if (s > size)
					s = size;

				p = t->phys + (virt - t->virt);
				remove_translation(e, virt, s);
				add_translation(e, virt, p, s, mode);
				changed = 1;
				break;
			}
		}
	}

	return NO_ERROR;
}

Retcode
make_translations(Environ *e)
{
	Package *savepkg = e->currpkg;
	Cell saveinst = e->currinst;
	Retcode ret;
	Byte *prop;
	Int plen = 0;
	Int i;
	Translation *t = NULL;

	e->currpkg = e->mmu;
	e->currinst = (uPtr)NULL;

	for (i = 0, t = g_translations; t; t = t->next, i++)
		;

	prop = prop_alloc(e, i * 7 * sizeof (Int));

	for (t = g_translations; t; t = t->next)
	{
		/* first encode the double-cell virtual address */
		prop_encode_int(prop + plen, &plen, t->virt >> 32);
		prop_encode_int(prop + plen, &plen, t->virt);

		/* second encode the size */
		prop_encode_int(prop + plen, &plen, t->size >> 32);
		prop_encode_int(prop + plen, &plen, t->size);

		/* third encode the double-cell physical address */
		prop_encode_int(prop + plen, &plen, t->phys >> 32);
		prop_encode_int(prop + plen, &plen, t->phys);

		/* forth encode the mode */
		prop_encode_int(prop + plen, &plen, t->mode);
	}
	
	ret = add_property(e->mmu->props, "translations", CSTR, prop, plen);
	e->currpkg = savepkg;
	e->currinst = saveinst;
	return ret;
}

C(f_mmu_map)		/* map (phys.lo ... phys.hi virt size mode --) */
{
	Cell virt, size;
	Cell offset;
	Int mode;
	uInt addr[ROOT_ADDRESS_CELLS];
	int i;
	Int cells = get_address_cells(e->root);

	IFCKSP(e, cells + 3, 0);
	POP(e, mode);
	POP(e, size);
	POP(e, virt);

	DPRINTF(("f_mmu_map: addr=%#x\n", addr));
	DPRINTF(("f_mmu_map: virt=%#x size=%#x mode=%#x\n", virt, size, mode));

	for (i = ROOT_ADDRESS_CELLS - 1; i >= 0; i--)
		POPTYPE(e, addr[i], uInt);

	offset = addr[0] & PAGE_MASK;
	DPRINTF(("f_mmu_map: addr[0]=%#x offset=%#x\n", addr[0], offset));

	if (offset != (virt & PAGE_MASK))
		return E_BAD_ARGUMENT;

	addr[0] -= offset;
	virt -= offset;
	size += offset;
	size = (size + PAGE_SIZE - 1) & ~PAGE_MASK;

	DPRINTF(("f_mmu_map: new addr[0]=%#x virt=%#x size=%#x\n", addr[0], virt, size));

	if (check_callback(e))
	{
		Cell array[12];
		int cret;

		DPRINTF(("f_mmu_map: calling back to client\n"));

		/* use client callback to map in the space */
		array[0] = (Cell)"map";
		array[1] = 4;
		array[2] = 2;
		array[3] = ((Cell)addr[1] << 32) | addr[0];
		array[4] = (Cell)virt;
		array[5] = (Cell)size;
		array[6] = (Cell)mode;
		cret = machine_callback(e, e->callback, array);
		DPRINTF(("f_mmu_map: cret = %d, ret = %d\n", cret, array[7]));
		return array[7];
	}

	DPRINTF(("f_mmu_map: about to add translation\n"));
	add_translation(e, ((uLong)addr[1] << 32) | addr[0], virt, size, mode);
	DPRINTF(("f_mmu_map: about to build translations property\n"));
	make_translations(e);
	DPRINTF(("f_mmu_map: add the mappings\n"));

	/* perform machine-dependent mapping here */
	{
		Retcode ret;
		ret = mmu_map(e, addr, virt, size, mode);
		DPRINTF(("f_mmu_map: added mappings, ret %d\n", ret));
		return ret;
	}
/*	return mmu_map(e, addr, virt, size, mode);	*/
}

C(f_mmu_unmap)		/* unmap (virt size --) */
{
	Cell virt, size;
	Cell offset;

	IFCKSP(e, 2, 0);
	POP(e, size);
	POP(e, virt);
	DPRINTF(("f_mmu_unmap: virt %#x, size %#x\n", virt, size));

	offset = virt & PAGE_MASK;
	virt -= offset;
	size += offset;
	size = (size + PAGE_SIZE - 1) & ~PAGE_MASK;
	DPRINTF(("f_mmu_unmap: new virt %#x, size %#x\n", virt, size));

	if (check_callback(e))
	{
		Cell array[12];
		int cret;

		DPRINTF(("f_mmu_unmap: calling back to client\n"));

		/* use client callback to map in the space */
		array[0] = (Cell)"unmap";
		array[1] = 2;
		array[2] = 1;
		array[3] = (Cell)virt;
		array[4] = (Cell)size;
		cret = machine_callback(e, e->callback, array);
		DPRINTF(("f_mmu_unmap: cret = %d, ret = %d\n", cret, array[5]));
		return array[5];
	}

	remove_translation(e, virt, size);
	DPRINTF(("f_mmu_unmap: translation removed\n"));
	make_translations(e);
	DPRINTF(("f_mmu_unmap: translations property updated\n"));

	/* perform machine-dependent unmapping here */
	return mmu_unmap(e, virt, size);
}

C(f_mmu_modify)		/* modify (virt size mode --) */
{
	Cell virt, size;
	Cell offset;
	Int mode;

	IFCKSP(e, 3, 0);
	POP(e, mode);
	POP(e, size);
	POP(e, virt);

	offset = virt & ~PAGE_MASK;
	virt -= offset;
	size += offset;
	size = (size + PAGE_SIZE - 1) & ~PAGE_MASK;
	modify_translations(e, virt, size, mode);

	/* perform machine-dependent modification here */
	return mmu_modify(e, virt, size, mode);
}

/* translate (virt -- false | physlo .. phys.hi mode true) */
C(f_mmu_translate)
{
	Cell virt;
	Int cells = get_address_cells(e->root);
	Int i;
	uInt phys[MAX_ADDR_CELLS];
	uInt mode;
	Retcode ret = NO_ERROR;

	IFCKSP(e, 1, cells + 2);
	POP(e, virt);
	DPRINTF(("f_mmu_translate: virt %#x\n", virt));

	if (check_callback(e))
	{
		Cell array[12];
		int cret;

		DPRINTF(("f_mmu_translate: calling back to client\n"));

		/* use client callback to map in the space */
		array[0] = (Cell)"translate";
		array[1] = 1;
		array[2] = 4;
		array[3] = (Cell)virt;
		cret = machine_callback(e, e->callback, array);
		DPRINTF(("f_mmu_translate: cret = %d, ret = %d\n", cret, array[4]));

		if (array[5] == NO_ERROR)
		{
			PUSH(e, array[6] & 0xFFFFFFFFUL);
			PUSH(e, array[6] >> 32);
			PUSH(e, array[7]);
			PUSH(e, FTRUE);
			return NO_ERROR;
		}

		PUSH(e, FFALSE);
		return array[4];
	}

	/* perform machine-dependent address translation here */
	ret = mmu_translate(e, virt, phys, &mode);

	if (ret == NO_ERROR)		/* no translation error */
	{
		for (i = 0; i < cells; i++)
		    PUSH(e, phys[i]);

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


/* methods for each cpu node */
static const Initentry cpu_methods[] =
{
	{ "open",          f_cpu_open,         INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- ok?) open device") },
	{ "close",         f_cpu_close,        INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) close device") },
	{ "selftest",      f_cpu_selftest,     INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) test device") },

	/* addition nodes for MMU support */
	{ "claim",         f_mmu_claim,        INVALID_FCODE, F_NONE, T_FUNC
			HELP("([virt] size align -- base) claim virtual address space") },
	{ "release",       f_mmu_release,      INVALID_FCODE, F_NONE, T_FUNC
			HELP("(virt size --) release virtual address space") },
	{ "of-claim",      f_mmu_of_claim,     INVALID_FCODE, F_NONE, T_FUNC
			HELP("(size -- base) used by SmartFirmware to claim virtual address space") },
	{ "of-release",    f_mmu_of_release,   INVALID_FCODE, F_NONE, T_FUNC
			HELP("(virt size --) used by SmartFirmware to release virtual address space") },
	{ "map",           f_mmu_map,          INVALID_FCODE, F_NONE, T_FUNC
			HELP("(phys.o ... phys.hi virt size mode --) map virtual address\n"
			" space to physical addresses") },
	{ "unmap",         f_mmu_unmap,        INVALID_FCODE, F_NONE, T_FUNC
			HELP("(virt size --) unmap virtual address space") },
	{ "modify",        f_mmu_modify,       INVALID_FCODE, F_NONE, T_FUNC
			HELP("(virt size mode --) modify mapping mode for virtual address range") },
	{ "translate",     f_mmu_translate,    INVALID_FCODE, F_NONE, T_FUNC
			HELP("(virt -- false | phys.lo ... phys.hi mode true) translate virtual address") },
	{ NULL,            NULL },
};

C(f_cpus_open)					/* open (-- okay?) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, FTRUE);
	return NO_ERROR;
}

C(f_cpus_close)					/* close (--) */
{
	return NO_ERROR;
}

C(f_cpus_selftest)				/* selftest (--) */
{
	Bool diag = diagnostic_mode(e);

	if (diag)
		cprintf(e, "cpus selftest...\n");

	PUSH(e, 0);			/* successful */
	return NO_ERROR;
}

C(f_cpus_encode_unit)				/* encode-unit (phys -- str len) */
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

C(f_cpus_decode_unit)		/* decode-unit (str len -- phys) */
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


/* methods for /cpus node */
static const Initentry cpus_methods[] =
{
	{ "open",          f_cpus_open,         INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- okay?) open device") },
	{ "close",         f_cpus_close,        INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) close device") },
	{ "selftest",      f_cpus_selftest,     INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) test device") },
	{ "encode-unit",   f_cpus_encode_unit,  INVALID_FCODE, F_NONE, T_FUNC
			HELP("(phys -- str len) encode device address") },
	{ "decode-unit",   f_cpus_decode_unit,  INVALID_FCODE, F_NONE, T_FUNC
			HELP("(str len -- phys) decode device address string") },

	{ NULL,            NULL },
};


/*****
CUSTOMIZE!  This example is for a PPC 750 processor.
This list of properties must be customized for the specific PPC CPU to
be compliant with the IEEE-1275 PowerPC Processor Binding.
strval entries must be either present or absent (commented-out) depending
on the specific processor, and are commented out by default.
See the PowerPC Processor Binding for detailed descriptions of these values.
*****/

typedef struct amd64_prop
{
	Byte *name;
	Int val;
	Byte *strval;
} AMD64_prop;

static AMD64_prop amd64_prop_list[] =
{
	{ "model", 0, "Athlon64" },
	{ "clock-frequency", 800000000 },
	{ "timebase-frequency", 3686400 },
	{ "bus-frequency", 800000000 },

	{ "page-size", PAGE_SIZE },

	{ "tlb-size", 64 },
	{ "tlb-sets", 1 },

	{ "write-buffer-size", 32 },

	{ "cache-unified", 0, (char *)-1 },	/* unified I/D cache */
	{ "d-cache-size", 64*1024 },
	{ "d-cache-block-size", 64 },
	{ "d-cache-sets", 1024 },
	{ "i-cache-size", 64*1024 },
	{ "i-cache-block-size", 64 },
	{ "i-cache-sets", 1024 },

	{ "map-in-mode", 0x17 },
	{ "claim-mode", 0x07 },

	{ NULL, 0, NULL }
};

CC(install_cpu_amd64)
{
	Package *pkg = new_pkg_name(e->root, "cpus");
	Package *cpupkg = NULL;
	Package *savepkg = e->currpkg;
	Instance *inst;
	Retcode ret;
	AMD64_prop *pp;
	Allocator *a;
	uInt addr[2];
	uInt size[2];

	DPRINTF(("install_cpu_amd64\n"));

	/* create the /cpus node */
	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	DPRINTF(("install_cpu_amd64: cpus pkg %#x\n", pkg));

	ret = init_entries(e, pkg->dict, cpus_methods);

	DPRINTF(("install_cpu_amd64: cpus methods added\n"));

	if (ret == NO_ERROR)
		ret = prop_set_str(pkg->props, "device_type", CSTR, "cpus", CSTR);

	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "#address-cells", CSTR, 1);

	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "#size-cells", CSTR, 0);

	DPRINTF(("install_cpu_amd64: cpus properties set\n"));

	if (ret != NO_ERROR)
		return ret;

	cpupkg = new_pkg_name(pkg, "cpu");
	e->mmu = cpupkg;
	DPRINTF(("install_cpu_amd64: amd64 pkg %#x\n", cpupkg));

	ret = init_entries(e, cpupkg->dict, cpu_methods);

	DPRINTF(("install_cpu_amd64: cpu@0 methods added\n"));

	if (ret == NO_ERROR)
		ret = prop_set_str(pkg->props, "device_type", CSTR, "cpu", CSTR);

	/* create the "reg" property - uni-processor system has val 0 */
	PUSH(e, 0);				/* phys */
	e->currpkg = cpupkg;
	ret = execute_word(e, "reg");
	e->currpkg = savepkg;

	DPRINTF(("install_cpu_amd64: cpus reg property set\n"));

	if (ret != NO_ERROR)
		return ret;

	if (ret == NO_ERROR)
		ret = prop_set_int(cpupkg->props, "cpu-version", CSTR, amd64_get_version());

	DPRINTF(("install_cpu_amd64: cpu-version set\n"));

	/* this creates all the custom amd64-specific props listed above */
	for (pp = amd64_prop_list; ret == NO_ERROR && pp->name; pp++)
		if (pp->strval == (char *)-1)
			ret = add_property(cpupkg->props, pp->name, CSTR, NULL, 0);
		else if (pp->strval)
			ret = prop_set_str(cpupkg->props, pp->name, CSTR, pp->strval, CSTR);
		else
			ret = prop_set_int(cpupkg->props, pp->name, CSTR, pp->val);

	DPRINTF(("install_cpu_amd64: amd64 properties set\n"));

	if (ret != NO_ERROR)
		return ret;

	alloc_init(&a, 2, 2);
	addr[0] = 0x00000000;
	addr[1] = 0xFFFFFFE0;
	size[0] = 0x00000000;
	size[1] = 0x0000000F;
	free_block(a, addr, size);
	g_of_virt_allocator = a;
	ret = make_of_available(e);

	/* CUSTOMIZE - initialize these to all of memory */
	alloc_init(&a, 2, 2);
	g_virt_allocator = a;
	addr[0] = 0x00000000;
	addr[1] = 0x00000000;
	size[0] = 0x00000000;
	size[1] = 0x00008000;
	free_block(a, addr, size);
	addr[0] = 0x00000000;
	addr[1] = 0xFFFF8000;
	size[0] = 0x00000000;
	size[1] = 0x00008000;
	free_block(a, addr, size);

	/* initialize the "available" and "existing" properties */
	if (ret == NO_ERROR)
		ret = make_existing(e);

	DPRINTF(("install_cpu_amd64: amd64 existing set\n"));
	addr[0] = 0x00000000;
	addr[1] = 0xFFFFFFE0;
	size[0] = 0x00000000;
	size[1] = 0x00000010;
	alloc_fixed(a, addr, size);

	if (ret == NO_ERROR)
		ret = make_available(e);

	DPRINTF(("install_cpu_amd64: amd64 available set\n"));

	if (ret != NO_ERROR)
		return ret;

	ret = add_property(cpupkg->props, "translations", CSTR, NULL, 0);

	DPRINTF(("install_cpu_amd64: amd64 translations set\n"));

	/* open this device to create an instance for the /chosen property "mmu" */
	PUSH(e, "/cpus/cpu");
	PUSH(e, 9);
	ret = execute_word(e, "open-dev");
	DPRINTF(("install_cpu_amd64: /cpus/cpu opened\n"));

	if (ret != NO_ERROR)
		return ret;

	POPT(e, inst, Instance*);
	DPRINTF(("mmu inst %p\n", inst));
	e->currpkg = savepkg;
	ret = prop_set_ptr(e->chosen->props, "mmu", CSTR, (uPtr)inst);
	DPRINTF(("install_cpu_amd64: /chosen mmu prop set\n"));

	if (ret != NO_ERROR)
		return ret;

	ret = prop_set_ptr(e->chosen->props, "cpu", CSTR, (uPtr)inst);
	DPRINTF(("install_cpu_amd64: /chosen cpu prop set\n"));
	return ret;
}
