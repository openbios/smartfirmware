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

/* (c) Copyright 1996-2003 by CodeGen, Inc.  All Rights Reserved. */

#include "defs.h"


/* Extensible Client Services Package - proposal #215 */

/* push-package (phandle --) 0x129 */
C(f_push_package)
{
	Package *pkg;

	IFCKSP(e, 1, 0);
	POPT(e, pkg, Package*);

	if (e->pkgtop >= MAX_PKG_STACK_SIZE)
		return E_PKG_STACK_OVERFLOW;

	e->pkgstk[e->pkgtop++] = e->currpkg;
	e->currpkg = pkg;
	return NO_ERROR;
}

/* pop-package (--) 0x12A */
C(f_pop_package)
{
	if (e->pkgtop <= 0)
		return E_PKG_STACK_UNDERFLOW;

	e->currpkg = e->pkgstk[--e->pkgtop];
	return NO_ERROR;
}


/* 5.3.4.1  Open/close package */

/* find-package (name-str name-len -- false | phandle true) 0x204 */
C(f_find_package)
{
	Int len;
	Byte *str;
	Package *pkg;
	
	IFCKSP(e, 2, 2);
	POP(e, len);
	POPT(e, str, Byte*);

	if (str[0] == '/')		/* handle client-services package */
	{
		Byte buf[STR_SIZE];
		Byte *s;
		Byte *st = str;
		Int l = len;

		pkg = e->root;		/* start searching for package at / */

		do
		{
			s = buf;
			st++;		/* increment past '/' */
			l--;

			while (l && *st != '/')
			{
				*s++ = *st++;
				l--;
			}

			*s = '\0';
			pkg = find_package(pkg, buf, CSTR);
		} while (l && pkg);
	}
	else
		pkg = find_package(e->packages, str, len);

#ifdef SUN_COMPATIBILITY

	/* try looking it up as a device in the device tree instead */
	if (pkg == NULL)
		pkg = find_device(e, str, len);

#endif /* SUN_COMPATIBILITY */

	if (pkg == NULL)
		PUSH(e, FFALSE);
	else
	{
		PUSHP(e, pkg);
		PUSH(e, FTRUE);
	}
	
	return NO_ERROR;
}

/* open-package (arg-str arg-len phandle -- ihandle | 0) 0x205 */
C(f_open_package)
{
	Package *pkg;
	Byte *str;
	Int len, r;
	Instance *inst;
	Retcode ret;
	
	IFCKSP(e, 3, 1);
	POPT(e, pkg, Package*);
	POP(e, len);
	POPT(e, str, Byte*);
	
	if (pkg == NULL)
	{
		PUSH(e, 0);
		return E_NULL_PACKAGE;
	}
	
	/* create a new instance or dup the initinst if it exists */
	inst = pkg->initinst ? dup_instance(pkg->initinst) : new_instance(pkg);

	if (inst == NULL)
		return E_OUT_OF_MEMORY;

	inst->parent = (Instance*)(uPtr)e->currinst;
	inst->args = lstrdup(str, len);
	
	/* if there are instance-specific vars, the xtoks[] table needs
	   to point to the newly created vars and not the ones in initinst */
	if (inst->dict)
	{
		Entry *ent;

		for (ent = inst->dict->list; ent; ent = ent->link)
			e->xtoks[ent->xtok] = ent;
	}
	
	ret = execute_method_name(e, inst, "open", CSTR);
	POP(e, r);
	DPRINTF(("open-package: ret=%d:%s r=%d\n", ret, err2str(ret), r));
	
	if (ret == NO_ERROR && r == FTRUE)
		PUSHP(e, inst);
	else
		PUSH(e, 0);
	
	e->currinst = (uPtr)inst->parent;
	return NO_ERROR;
}

/* $open-package (arg-str arg-len name-str name-len -- ihandle | 0) 0x20F */
CC(f_dopen_package)
{
	Cell val;
	
	IFCKSP(e, 4, 1);
	f_find_package(e);
	POP(e, val);
	
	if (val)
		return f_open_package(e);
	
	DROP(e);
	DROP(e);
	PUSH(e, 0);
	return NO_ERROR;
}

CC(f_close_package)	/* close-package (ihandle --) 0x206 */
{
	Instance *inst;
	
	IFCKSP(e, 1, 0);
	POPT(e, inst, Instance*);
	
	if (inst == NULL)
		return NO_ERROR;
	
	(void)execute_method_name(e, inst, "close", CSTR);
	delete_instance(inst);
	return NO_ERROR;
}

/* my-self 0x203: T_ADDRVAL to &e->currinst */

C(f_my_parent)		/* my-parent (-- ihandle) 0x20A */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	if (inst == NULL)
		return E_NULL_INSTANCE;

	IFCKSP(e, 0, 1);
	PUSHP(e, inst->parent);
	return NO_ERROR;
}

C(f_i2phandle)		/* ihandle>phandle (ihandle -- phandle) 0x20B */
{
	Instance *inst;
	
	IFCKSP(e, 1, 1);
	POPT(e, inst, Instance*);

	if (inst == NULL)
		return E_NULL_INSTANCE;

	PUSHP(e, inst->package);
	return NO_ERROR;
}

/* next-property (previous-str previous-len phandle --
		false | name-str name-len true) 0x23D */
C(f_nextprop)
{
	Byte *str;
	Int len;
	Entry *ent;
	Package *pkg;
	
	IFCKSP(e, 3, 3);
	POPT(e, pkg, Package*);
	POP(e, len);
	POPT(e, str, Byte*);
	
	if (pkg == NULL)
		return E_NULL_PACKAGE;
	
	if (str == NULL || *str == '\0')
		ent = pkg->props->list;
	else
	{
		ent = find_table(pkg->props, str, len);

		if (ent && ent->link)
			ent = ent->link;
	}
	
	if (ent)
	{
		PUSHP(e, ent->name + 1);
		PUSH(e, *(uByte*)ent->name);
		PUSH(e, FTRUE);
	}
	else
		PUSH(e, FFALSE);
	
	return NO_ERROR;
}

CC(f_peer)			/* peer (phandle -- phandle.sibling) 0x23C */
{
	Package *pkg;
	
	IFCKSP(e, 1, 0);
	POPT(e, pkg, Package*);
	
	if (pkg)
		PUSHP(e, pkg->link);
	else
		PUSHP(e, e->root);
	
	return NO_ERROR;
}

CC(f_child)			/* child (phandle.parent -- phandle.child) 0x23B */
{
	Package *pkg;
	
	IFCKSP(e, 1, 0);
	POPT(e, pkg, Package*);

	if (pkg)
		PUSHP(e, pkg->children);
	else
		PUSH(e, 0);
			
	return NO_ERROR;
}


/* 5.3.4.2  Call methods from other packages */

/* find-method (method-str method-len phandle -- false | xt true) 0x207 */
C(f_find_method)
{
	Package *pkg;
	Entry *ent;
	Byte *str;
	Int len;
	
	IFCKSP(e, 3, 2);
	POPT(e, pkg, Package*);
	POP(e, len);
	POPT(e, str, Byte*);
	ent = find_table(pkg->dict, str, len);
	
	if (ent)
	{
		PUSH(e, ent->xtok);
		PUSH(e, FTRUE);
	}
	else
		PUSH(e, FFALSE);
		
	return NO_ERROR;
}

C(f_call_package)	/* call-package (... xt ihandle -- ?) 0x208 */
{
	Instance *inst;
	Int xtok;
	
	IFCKSP(e, 2, 0);
	POPT(e, inst, Instance*);
	POP(e, xtok);
	return execute_method(e, inst, xtok);
}

/* $call-method (... method-str method-len ihandle -- ?) 0x20E */
C(f_dcall_method)
{
	Instance *inst;
	Byte *str;
	Int len;
	
	IFCKSP(e, 3, 0);
	POPT(e, inst, Instance*);
	POP(e, len);
	POPT(e, str, Byte*);
	return execute_method_name(e, inst, str, len);
}

C(f_dcall_parent)	/* $call-parent (... method-str method-len -- ?) 0x209 */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Byte *str;
	Int len;
	
	if (inst == NULL)
		return E_NULL_INSTANCE;

	IFCKSP(e, 2, 0);
	inst = inst->parent;
	POP(e, len);
	POPT(e, str, Byte*);
	return execute_method_name(e, inst, str, len);
}


/* 5.3.4.3  Get local arguments */

C(f_my_address)		/* my-address (-- phys.lo ...) 0x102 */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int i;
	
	if (inst == NULL)
		return E_NULL_INSTANCE;

	IFCKSP(e, 0, inst->numunits - 1);
	
	for (i = 0; i < inst->numunits - 1; i++)
		PUSH(e, inst->probe[i]);

	return NO_ERROR;
}

C(f_my_space)		/* my-space (-- phys.hi) 0x103 */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	if (inst == NULL)
		return E_NULL_INSTANCE;

	IFCKSP(e, 0, 1);
	PUSH(e, inst->probe[inst->numunits - 1]);
	return NO_ERROR;
}

C(f_my_unit)		/* my-unit (-- phys.lo ... phys.hi) 0x20D */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Entry *ent;
	Int i, acells, plen, t;
	Byte *paddr;
	Cell unit[MAX_ADDR_CELLS];
	Retcode ret = NO_ERROR;

	if (inst == NULL)
		return E_NULL_INSTANCE;

	ent = find_table(inst->package->props, "reg", CSTR);

	if (ent != NULL)
	{
		acells = get_address_cells(inst->package->parent);
		IFCKSP(e, 0, acells);
		paddr = ent->v.array;
		plen = ent->len;
		
		for (i = acells - 1; ret == NO_ERROR && i >= 0; i--)
		{
			ret = prop_decode_int(&paddr, &plen, &t);
			unit[i] = t;
		}
	}
	else
	{
		/* no reg property - use instance-specific unit[] field */
		memcpy(unit, inst->unit, sizeof unit);
		acells = inst->numunits;
	}
	
	for (i = 0; i < acells; i++)
		PUSH(e, unit[i]);

	return ret;
}

C(f_my_args)		/* my-args (-- arg-str arg-len) 0x202 */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	if (inst == NULL)
		return E_NULL_INSTANCE;

	IFCKSP(e, 0, 2);
	PUSHP(e, inst->args + 1);
	PUSH(e, (uByte)inst->args[0]);
	return NO_ERROR;
}

/* left-parse-string (str len char -- R-str R-len L-str L-len) 0x240 */
C(f_left_parse_str)
{
	Byte c;
	Byte *str, *s;
	Int len, l;
	
	IFCKSP(e, 3, 4);
	POP(e, c);
	POP(e, len);
	POPT(e, str, Byte*);
	
	s = str;
	l = len;
	
	for (; l > 0 && *s != c; l--, s++)
		;
	
	if (l > 0)
	{
		PUSHP(e, s + 1);
		PUSH(e, l - 1);
		PUSHP(e, str);
		PUSH(e, len - l);
	}
	else
	{
		PUSHP(e, NULL);
		PUSH(e, 0);
		PUSHP(e, str);
		PUSH(e, len);
	}

	return NO_ERROR;
}

C(f_parse_2int)		/* parse-2int (str len -- val.lo val.hi) 0x11B */
{
	Byte *str;
	Int len;
	Cell vallo, valhi, err;
	
	IFCKSP(e, 2, 3);
	PUSH(e, ',');
	f_left_parse_str(e);

	POP(e, len);
	POPT(e, str, Byte*);
	parse_number(e->radix, &str, &len, &valhi, &err, TRUE);
	DO_MASK32(e, valhi);

	POP(e, len);
	POPT(e, str, Byte*);

	if (str && len)
		parse_number(e->radix, &str, &len, &vallo, &err, TRUE);
	else
		vallo = 0;

	DO_MASK32(e, vallo);

	PUSH(e, vallo);
	PUSH(e, valhi);
	return NO_ERROR;
}


/* 5.3.4.4  Mapping tools */

/* map-low (phys.lo ... size -- virt) 0x130 */

C(f_free_virtual)	/* free-virtual (virt size --) 0x105 */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Retcode ret;
	Int addr;

	if (inst == NULL || inst->parent == NULL)
		return E_NULL_INSTANCE;

	if (inst->package == NULL)
		return E_NULL_PACKAGE;

	IFCKSP(e, 2, 0);
	ret = prop_get_int(inst->package->props, "address", CSTR, &addr);

	if (ret == NO_ERROR && addr == e->sp[-1])
		delete_property(inst->package->props, "address", CSTR);

	return execute_method_name(e, inst->parent, "map-out", CSTR);
}



/* 5.3.5.1  Property array encoding */

CC(f_encode_int)		/* encode-int (n -- prop-addr prop-len) 0x111 */
{
	Int len = 0;
	Int n;
	Byte *addr;
	
	IFCKSP(e, 1, 2);
	POP(e, n);
	addr = prop_alloc(e, sizeof (Int));
	
	if (addr == NULL)
		return E_OUT_OF_MEMORY;
		
	prop_encode_int(addr, &len, n);
	PUSHP(e, addr);
	PUSH(e, len);
	return NO_ERROR;
}

C(f_encode_str)		/* encode-string (str len -- prop-addr prop-len) 0x114 */
{
	Byte *str;
	Int len;
	Int elen = 0;
	Byte *addr;
	
	IFCKSP(e, 2, 2);
	POP(e, len);
	POPT(e, str, Byte*);
	addr = prop_alloc(e, len + 1);
	
	if (addr == NULL)
		return E_OUT_OF_MEMORY;

	prop_encode_str(addr, &elen, str, len);
	PUSHP(e, addr);
	PUSH(e, elen);
	return NO_ERROR;
}

/* encode-bytes (data-addr data-len -- prop-addr prop-len) 0x115 */
C(f_encode_bytes)
{
	Byte *data;
	Int len;
	Byte *addr;
	
	IFCKSP(e, 2, 2);
	POP(e, len);
	POPT(e, data, Byte*);
	PUSHP(e, e->fbrk);
	PUSH(e, len);
	addr = prop_alloc(e, len);
	
	if (addr == NULL)
		return E_OUT_OF_MEMORY;
	
	memcpy(addr, data, len);
	return NO_ERROR;
}

/* encode-phys (phys.lo ... phys.hi -- prop-addr prop-len) 0x113 */
CC(f_encode_phys)
{
	Int plen = get_address_cells(e->currpkg->parent);
	Byte *paddr;
	Int val, len;
	Int elen = 0;
	
	IFCKSP(e, plen, 2);
	paddr = prop_alloc(e, plen * sizeof (Int));
	
	if (paddr == NULL)
		return E_OUT_OF_MEMORY;
	
	while (plen--)
	{
		POP(e, val);
		len = 0;
		prop_encode_int(paddr + elen, &len, val);
		elen += len;
	}
	
	PUSHP(e, paddr);
	PUSH(e, elen);
	return NO_ERROR;
}

/* encode+ (prop-addr1 prop-len1 prop-addr2 prop-len2 --
		prop-addr3 prop-len3) 0x112 */
C(f_encodeplus)
{
	Int len, n;
	
	/* assume 2 arrays are allocated off of e->propmem, so 1st + 2nd is really
	   just the 1st pointer, with a new length - see the spec for more details */
	IFCKSP(e, 4, 2);
	POP(e, len);
	DROP(e);
	POP(e, n);
	len += n;
	PUSH(e, len);
	return NO_ERROR;
}


/* 5.3.5.2  Property array decoding */

/* decode-int (prop-addr1 prop-len1 -- prop-addr2 prop-len2 n) 0x21B */
C(f_decode_int)
{
	Byte *paddr;
	Int plen, val;
	Retcode ret;
	
	IFCKSP(e, 2, 3);
	POP(e, plen);
	POPT(e, paddr, Byte*);
	ret = prop_decode_int(&paddr, &plen, &val);
	PUSHP(e, paddr);
	PUSH(e, plen);
	PUSH(e, val);
	return ret;
}

/* decode-phys (prop-addr1 prop-len1 --
		prop-addr2 prop-len2 phys.lo ... phys.hi) 0x128 */
CC(f_decode_phys)
{
	Byte *paddr;
	Cell *sp;
	Int plen, val;
	Int cells = get_address_cells(e->currpkg->parent);
	Retcode ret = NO_ERROR;
	
	IFCKSP(e, 2, 2 + cells);
	plen = TOP(e);
	paddr = (Byte*)(uPtr)STACK(e, 1);
	sp = e->sp += cells;
	
	/* to decode the physical address backwards without allocating memory */
	while (cells-- && ret == NO_ERROR)
	{
		ret = prop_decode_int(&paddr, &plen, &val);
		*sp-- = (uInt)val;
	}
	
	if (ret == NO_ERROR)
	{
		sp[0] = plen;
		sp[-1] = (uPtr)paddr;
	}
	else
		e->sp = sp - cells;		/* restore the stack */

	return ret;
}

/* decode-string (prop-addr1 prop-len1 -- prop-addr2 prop-len2 str len) 0x21C */
C(f_decode_str)
{
	Byte *paddr, *str;
	Int plen, slen;
	
	IFCKSP(e, 2, 4);
	POP(e, plen);
	POPT(e, paddr, Byte*);
	str = paddr;
	slen = 0;

	while (plen > 0 && *paddr)
		paddr++, plen--, slen++;
	
	if (plen > 0 && *paddr == '\0')
		paddr++, plen--;

	PUSHP(e, paddr);
	PUSH(e, plen);
	PUSHP(e, str);
	PUSH(e, slen);
	return NO_ERROR;
}


/* 5.3.5.3  Property declaration */

/* property (prop-addr prop-len name-str name-len --) 0x110 */
CC(f_property)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Byte *paddr, *name;
	Int plen, len;
	Table *props;
	
	IFCKSP(e, 4, 0);
	POP(e, len);
	POPT(e, name, Byte*);
	POP(e, plen);
	POPT(e, paddr, Byte*);

#ifdef SUN_COMPATIBILITY
	if (compare_strs(name, len, "name", CSTR) && diagnostic_mode(e))
	{
		int i = plen;

		while (i > 0 && paddr[i - 1] == '\0')
			i--;

		cprintf(e, "%S ", paddr, i);
	}
#endif
	
	if (inst)
		props = inst->package->props;
	else if (e->currpkg)
		props = e->currpkg->props;
	else
		return E_PROPERTY_ERROR;
	
	return add_property(props, name, len, paddr, plen);
}

C(f_delete_prop)	/* delete-property (name-str name-len --) 0x21E */
{
	Byte *name;
	Int len;
	
	IFCKSP(e, 2, 0);
	POP(e, len);
	POPT(e, name, Byte*);
	
	if (e->currpkg == NULL)
		return E_PROPERTY_ERROR;
		
	delete_property(e->currpkg->props, name, len);
	return NO_ERROR;
}

void
pushstr(Environ *e, char *str)
{
	PUSHP(e, str);
	PUSH(e, strlen(str));
}

C(f_device_name)	/* device-name (str len --) 0x201 */
{
	Retcode ret;
	
	ret = f_encode_str(e);
	
	if (ret != NO_ERROR)
		return ret;
	
	pushstr(e, "name");
	return f_property(e);
}

C(f_device_type)	/* device-type (str len --) 0x11A */
{
	Retcode ret;
	
	ret = f_encode_str(e);
	
	if (ret != NO_ERROR)
		return ret;
	
	pushstr(e, "device_type");
	return f_property(e);
}

#ifdef CUSTOM_F_REG
EC(f_reg);
#else
CC(f_reg)			/* reg (phys.lo ... phys.hi size --) 0x116 */
{
	Int size;
	
	IFCKSP(e, 1, 1);
	POP(e, size);
	f_encode_phys(e);
	PUSH(e, size);
	f_encode_int(e);
	f_encodeplus(e);
	pushstr(e, "reg");
	return f_property(e);
}
#endif

C(f_model)			/* model (str len --) 0x119 */
{
	f_encode_str(e);
	pushstr(e, "model");
	return f_property(e);
}


/* 5.3.5.4  Property value access */

/* get-package-property (name-str name-len phandle --
		true | prop-addr prop-len false) 0x21F */
C(f_get_pkg_prop)
{
	Package *pkg;
	Entry *ent;
	Byte *name;
	Int len;
	
	IFCKSP(e, 3, 3);
	POPT(e, pkg, Package*);
	POP(e, len);
	POPT(e, name, Byte*);
	ent = find_table(pkg->props, name, len);
	
	if (ent)
	{
		PUSHP(e, ent->v.array);
		PUSH(e, ent->len);
		PUSH(e, FFALSE);
	}
	else
		PUSH(e, FTRUE);
		
	return NO_ERROR;
}

/* get-inherited-property (name-str name-len --
		true | prop-addr prop-len false) 0x21D */
C(f_get_inh_prop)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg;
	Entry *ent = NULL;
	Byte *name;
	Int len;
	
	IFCKSP(e, 2, 3);
	POP(e, len);
	POPT(e, name, Byte*);
	
	if (inst != NULL)
		for (pkg = inst->package; pkg != NULL && ent == NULL;
				pkg = pkg->parent)
			ent = find_table(pkg->props, name, len);
	
	if (ent)
	{
		PUSHP(e, ent->v.array);
		PUSH(e, ent->len);
		PUSH(e, FFALSE);
	}
	else
		PUSH(e, FTRUE);
		
	return NO_ERROR;
}

/* get-my-property (name-str name-len --
		true | prop-addr prop-len false) 0x21A */
C(f_get_my_prop)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Entry *ent = NULL;
	Byte *name;
	Int len;
	
	IFCKSP(e, 2, 3);
	POP(e, len);
	POPT(e, name, Byte*);
	
	if (inst != NULL)
		ent = find_table(inst->package->props, name, len);
	
	if (ent)
	{
		PUSHP(e, ent->v.array);
		PUSH(e, ent->len);
		PUSH(e, FFALSE);
	}
	else
		PUSH(e, FTRUE);
		
	return NO_ERROR;
}


CC(f_new_device)		/* new-device (--) 0x11F */
{
	Package *pkg = new_package(e->currpkg);
	
	if (pkg == NULL)
		return E_PACKAGE_ERROR;
	
	e->currpkg = pkg;
	pkg->initinst->parent = (Instance*)(uPtr)e->currinst;
	e->currinst = (uPtr)pkg->initinst;

#ifdef SUN_COMPATIBILITY
	/* copy our previous sibling's instance probe & unit number data to
	   mimic Sun's version of OBP as some Fcode relies on this behavior */
	if (pkg->link)
	{
		memcpy(pkg->initinst->unit, pkg->link->initinst->unit,
				sizeof pkg->initinst->unit);
		memcpy(pkg->initinst->probe, pkg->link->initinst->probe,
				sizeof pkg->initinst->probe);
		DPRINTF(("new-device: unit=%#Cx,%#Cx probe=%#Cx,%#Cx\n",
				pkg->initinst->unit[0], pkg->initinst->unit[1],
				pkg->initinst->probe[0], pkg->initinst->probe[1]));
	}
#endif

	return NO_ERROR;
}

CC(f_finish_device)	/* finish-device (--) 0x127 */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = e->currpkg;
	Entry *ent;
	
	if (inst == NULL)
		return E_NULL_INSTANCE;

	e->currpkg = pkg->parent;
	e->currinst = (uPtr)inst->parent;
	
	/* if no name property was created, there was probably no device
	   so delete this entire package */
	if (find_table(pkg->props, "name", CSTR) == NULL)
		delete_package(pkg);
	else if (pkg->dict)
	{
		/* find and remove all headerless entries from the hashtable,
		   then free their names - the Entries stay on the linked-list */
		for (ent = pkg->dict->list; ent; ent = ent->link)
			if (ent->flags & F_HEADERLESS)
			{
				unhashent_table(pkg->dict, ent);
				free(ent->name);
				ent->name = NULL;
			}
	}

	return NO_ERROR;
}

C(f_delete_device)		/* delete-device (phandle --) [non-standard] */
{
	Package *pkg;

	IFCKSP(e, 1, 0);
	POPT(e, pkg, Package*);

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	if (pkg == e->root)
		return E_BAD_PACKAGE;

	if (e->currpkg == pkg)
		e->currpkg = pkg->parent;

	delete_package(pkg);
	return NO_ERROR;
}

CC(f_set_args)		/* set-args (arg-str arg-len unit-str unit-len --) 0x23F */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Retcode ret;
	Int len;
	Byte *str;
	
	IFCKSP(e, 4, 0);
	ret = execute_static_method_name(e, e->currpkg->parent,
			"decode-unit", CSTR);
	
	if (ret != NO_ERROR)
		return ret;
	
	len = inst->numunits;
	
	while (len--)
		POP(e, inst->probe[len]);

	DPRINTF(("set-args: unit=%#Cx,%#Cx probe=%#Cx,%#Cx\n",
			inst->unit[0], inst->unit[1],
			inst->probe[0], inst->probe[1]));

	POP(e, len);
	POPT(e, str, Byte*);
	inst->args = lstrdup(str, len);
	return NO_ERROR;
}


const Initentry init_packages[] =
{
	{ "push-package", f_push_package, 0x129, F_NONE, T_FUNC
			HELP("(phandle --)\n" \
"\tpush current package on package-stack, make phandle the current package") },
	{ "pop-package", f_pop_package, 0x12A, F_NONE, T_FUNC
			HELP("(--)\n" \
		"\tpop package off of package-stack making it the current package") },

	{ "find-package", f_find_package, 0x204, F_NONE, T_FUNC
			HELP("(name-str name-len -- false | phandle true)\n" \
					"\tlocate the named support package") },
	{ "open-package", f_open_package, 0x205, F_NONE, T_FUNC
			HELP("(arg-str arg-len phandle -- ihandle | 0)\n" \
					"\topen the support package indicated by phandle") },
	{ "$open-package", f_dopen_package, 0x20F, F_NONE, T_FUNC
			HELP("(arg-str arg-len name-str name-len -- ihandle | 0)\n" \
					"\topen the named support package") },
	{ "close-package", f_close_package, 0x206, F_NONE, T_FUNC
			HELP("(ihandle --) close the specified package instance") },
	{ "my-self", (Command)offsetof(Environ, currinst),
			0x203, F_NONE, T_ADDRVAL
			HELP("(-- ihandle) return ihandle of current instance") },
	{ "my-parent", f_my_parent, 0x20A, F_NONE, T_FUNC
			HELP("(-- ihandle) return ihandle of parent of current instance") },
	{ "ihandle>phandle", f_i2phandle, 0x20B, F_NONE, T_FUNC
			HELP("(ihandle -- phandle) return package phandle of ihandle") },
	{ "next-property", f_nextprop, 0x23D, F_NONE, T_FUNC
			HELP("(previous-str previous-len phandle --\n" \
					"\t\tfalse | name-str name-len true)\n"
				"\treturn name of property following previous of phandle") },
	{ "peer", f_peer, 0x23C, F_NONE, T_FUNC
			HELP("(phandle -- phandle.sibling) " \
					"return phandle of next sibling node") },
	{ "child", f_child, 0x23B, F_NONE, T_FUNC
			HELP("(phandle.parent -- phandle.child)\n" \
					"\treturn phandle of first child node of parent") },
	{ "find-method", f_find_method, 0x207, F_NONE, T_FUNC
			HELP("(method-str method-len phandle -- false | xt true)\n" \
					"\tfind named method in the package phandle") },
	{ "call-package", f_call_package, 0x208, F_NONE, T_FUNC
			HELP("(... xt ihandle -- ?) " \
					"execute method xt within instance ihandle") },
	{ "$call-method", f_dcall_method, 0x20E, F_NONE, T_FUNC
			HELP("(... method-str method-len ihandle -- ?)\n" \
					"\texecute named method within instance ihandle") },
	{ "$call-parent", f_dcall_parent, 0x209, F_NONE, T_FUNC
			HELP("(... method-str method-len -- ?)\n" \
					"\texecute named method within the parent instance") },
	{ "my-address", f_my_address, 0x102, F_NONE, T_FUNC
			HELP("(-- phys.lo ...)\n" \
					"\treturn low component(s) of device's physical address") },
	{ "my-space", f_my_space, 0x103, F_NONE, T_FUNC
			HELP("(-- phys.hi) " \
					"return high component of device's physical address") },
	{ "my-unit", f_my_unit, 0x20D, F_NONE, T_FUNC
			HELP("(-- phys.lo ... phys.hi)\n" \
					"\treturn the unit address of the current instance") },
	{ "my-args", f_my_args, 0x202, F_NONE, T_FUNC
			HELP("(-- arg-str arg-len)\n" \
				"\treturn the instance argument string for this instance") },
	{ "left-parse-string", f_left_parse_str, 0x240, F_NONE, T_FUNC
			HELP("(str len char -- R-str R-len L-str L-len)\n" \
					"\tsplit the string at first occurrence of char") },
	{ "parse-2int", f_parse_2int, 0x11B, F_NONE, T_FUNC
			HELP("(str len -- val.lo val.hi)\n" \
					"\tconvert a hi.lo string into a pair of values") },
	{ "map-low", (Command)"my-space swap \" map-in\" $call-parent",
			0x130, F_TOKENIZE, T_FORTH
			HELP("(phys.lo ... size -- virt)\n" \
					"\tmap the specified region, return a virtual address") },
	{ "free-virtual", f_free_virtual, 0x105, F_NONE, T_FUNC
			HELP("(virt size --) destroy mapping and address property") },
	
	{ "encode-int", f_encode_int, 0x111, F_NONE, T_FUNC
			HELP("(n -- prop-addr prop-len) " \
					"encode a number into a prop-encoded-array") },
	{ "encode-string", f_encode_str, 0x114, F_NONE, T_FUNC
			HELP("(str len -- prop-addr prop-len)\n" \
					"\tencode a string into a prop-encoded-array") },
	{ "encode-bytes", f_encode_bytes, 0x115, F_NONE, T_FUNC
			HELP("(data-addr data-len -- prop-addr prop-len)\n" \
					"\tencode a byte array into a prop-encoded-array") },
	{ "encode-phys", f_encode_phys, 0x113, F_NONE, T_FUNC
			HELP("(phys.lo ... phys.hi -- prop-addr prop-len)\n" \
					"\tencode a unit-address into a prop-encoded-array") },
	{ "encode+", f_encodeplus, 0x112, F_NONE, T_FUNC
			HELP("(prop-addr1 prop-len1 prop-addr2 prop-len2 -- " \
					"prop-addr3 prop-len3)\n" \
					"\tcatenate two prop-encoded-arrays into a single array") },

	{ "decode-int", f_decode_int, 0x21B, F_NONE, T_FUNC
			HELP("(prop-addr1 prop-len1 -- prop-addr2 prop-len2 n)\n" \
					"\tdecode a number from a prop-encoded-array") },
	{ "decode-phys", f_decode_phys, 0x128, F_NONE, T_FUNC
			HELP("(prop-addr1 prop-len1 -- " \
					"prop-addr2 prop-len2 phys.lo ... phys.hi)\n" \
					"\tdecode a unit address from a prop-encoded-array") },
	{ "decode-string", f_decode_str, 0x21C, F_NONE, T_FUNC
			HELP("(prop-addr1 prop-len1 -- prop-addr2 prop-len2 str len)\n" \
					"\tdecode a string from a prop-encoded-array") },
	{ "property", f_property, 0x110, F_NONE, T_FUNC
			HELP("(prop-addr prop-len name-str name-len --)\n" \
					"\tcreate a new property with the given name and value") },
	{ "delete-property", f_delete_prop, 0x21E, F_NONE, T_FUNC
			HELP("(name-str name-len --)\n" \
					"\tdelete the named property in the active package") },
	{ "device-name", f_device_name, 0x201, F_NONE, T_FUNC
			HELP("(str len --) " \
					"create the name property with the given value") },
	{ "name", f_device_name, 0x201, F_NONE, T_FUNC
			HELP("(str len --) " \
					"synonym for device-name - create the name property") },
			/* synonym */
	{ "device-type", f_device_type, 0x11A, F_NONE, T_FUNC
			HELP("(str len --) " \
					"create the device_type property with the given value") },
	{ "reg", f_reg, 0x116, F_NONE, T_FUNC
			HELP("(phys.lo ... phys.hi size --) " \
					"create the reg property with the given value") },
	{ "model", f_model, 0x119, F_NONE, T_FUNC
			HELP("(str len --) " \
					"create the model property with the given value") },
	{ "get-package-property", f_get_pkg_prop, 0x21F, F_NONE, T_FUNC
			HELP("(name-str name-len phandle -- true |\n" \
					"\t\tprop-addr prop-len false)\n" \
					"\treturn value for name property in package phandle") },
	{ "get-inherited-property", f_get_inh_prop, 0x21D, F_NONE, T_FUNC
			HELP("(name-str name-len -- true | prop-addr prop-len false)\n" \
					"\treturn value for given property in current " \
					"instance or its parents") },
	{ "get-my-property", f_get_my_prop, 0x21A, F_NONE, T_FUNC
			HELP("(name-str name-len -- true | prop-addr prop-len false)\n" \
					"\treturn value for given property in current instance") },

	{ "new-device", f_new_device, 0x11F, F_NONE, T_FUNC
			HELP("(--) start new package as child of current package") },
	{ "finish-device", f_finish_device, 0x127, F_NONE, T_FUNC
			HELP("(--) finish current device, set active package to parent") },
	{ "delete-device", f_delete_device, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(phandle --) delete package phandle and all its children") },
			/* [non-standard] */
	{ "set-args", f_set_args, 0x23F, F_NONE, T_FUNC
			HELP("(arg-str arg-len unit-str unit-len --)\n" \
					"\tset address and arguemnts of new device node") },

	{ NULL, NULL }
};
