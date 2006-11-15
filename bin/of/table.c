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

/* (c) Copyright 1996-2000 by CodeGen, Inc.  All Rights Reserved. */

#include "defs.h"


/* Entry stuff */

Entry *
new_entry(void)
{
	Entry *self = (Entry*)malloc(sizeof *self);
	
	if (self == NULL)
	{
		error(E_OUT_OF_MEMORY);
		return NULL;
	}
	
	memset(self, 0, sizeof *self);		/* init everything to zeros */
	self->xtok = INVALID_XTOK;
	return self;
}

void
delete_entry(Entry *self)
{
	if (self == NULL)
		return;
	
	if (self->type != T_PROPERTY && !(self->flags & F_STATIC_NAME))
		free(self->name);
	
	if ((self->flags & F_MALLOC) && self->v.array)
		free(self->v.array);

	self->v.array = NULL;
	free(self);
}

Entry *
dup_entry(Entry *e)
{
	Entry *self;
	
	if (e == NULL)
		return NULL;
	
	self = new_entry();
	
	if (self == NULL)
		return NULL;
	
	*self = *e;

	if (e->type != T_PROPERTY)
		self->name = pstrdup(e->name);
	
	self->link = NULL;
	self->flags = F_NONE;

	if ((e->flags & F_MALLOC) && e->len)
	{
		self->v.array = (Byte*)malloc(self->len);

		if (self->v.array == NULL)
		{
			error(E_OUT_OF_MEMORY);
			delete_entry(self);
			return NULL;
		}

		memcpy(self->v.array, e->v.array, e->len);
	}

	self->flags = e->flags;
	return self;
}

static unsigned int
hash_str(Byte *str, Int len)
{
	unsigned int hash;

	/*setstrlen(&str, &len);*/

	if (str == NULL)
		len = 0;
	else if (len == PSTR)
	{
		len = *(uByte*)str;
		str += 1;
	}
	else if (len == CSTR)
		len = strlen(str);

	hash = len;

	while (len-- > 0)
	{
		hash = (hash << 2) + tolower(*str);
		str++;		/* avoid possible macro side-effect */
	}

	return hash ^ (hash >> 16);
}

/* Table stuff */

Table *
new_table(void)
{
	Table *self = (Table*)malloc(sizeof *self);
	
	if (self == NULL)
	{
		error(E_OUT_OF_MEMORY);
		return NULL;
	}
	
	memset(self, 0, sizeof *self);		/* init everything to zeros */

#ifdef HASH_EVERYTHING
	self = hash_table(self);
#endif

	return self;
}

void
delete_table(Table *self)
{
	Entry *ent;
	Entry *prev;

	if (self == NULL)
		return;

	if (self->hash)
		free(self->hash);

	self->hash = NULL;
	ent = self->list;

	while (ent)
	{
		prev = ent;
		ent = ent->link;
		delete_entry(prev);
	}

	self->list = NULL;
	free(self);
}

Table *
hash_table(Table *t)
{
	Entry *e;
	unsigned int h;

	if (t == NULL || t->hash != NULL)
		return t;

	t->hash = (Entry**)malloc(sizeof (Entry*) * TABLE_HASH);

	if (t->hash == NULL)
		return NULL;

	memset(t->hash, 0, sizeof (Entry*) * TABLE_HASH);

	for (e = t->list; e != NULL; e = e->link)
	{
		if (e->name == NULL || *e->name == 0)
			continue;

		h = hash_str(e->name, PSTR) % TABLE_HASH;
		e->hlink = t->hash[h];
		t->hash[h] = e;
	}

	return t;
}

Table *
dup_table(Table *t)
{
	Table *nt;
	Entry *ent;
	Entry *prev = NULL;
	
	if (t == NULL)
		return NULL;
		
	nt = new_table();
	nt->num = t->num;
	
	for (ent = t->list; ent; ent = ent->link)
	{
		if (prev)
		{
			prev->link = dup_entry(ent);
			prev = prev->link;
		}
		else
			prev = nt->list = dup_entry(ent);
	}

	if (t->hash)
		nt = hash_table(nt);
	
	return nt;
}

Bool
insert_table(Table *t, Entry *e)
{
	unsigned int h;

	if (t == NULL || e == NULL)
		return FALSE;
	
	e->link = t->list;
	t->list = e;
	t->num++;

	if (t->hash)
	{
		if (e->name && *e->name)
		{
			h = hash_str(e->name, PSTR) % TABLE_HASH;
			e->hlink = t->hash[h];
			t->hash[h] = e;
		}
	}
	else if (t->num > TABLE_HASH / 2)
		t = hash_table(t);

	return TRUE;
}

Bool
unhashent_table(Table *tab, Entry *ent)
{
	Entry *p = NULL;
	Entry *e;
	unsigned int h;
	
	if (tab == NULL || ent == NULL)
		return FALSE;
	
	if (tab->hash && ent->name && *ent->name)
	{
		h = hash_str(ent->name, PSTR) % TABLE_HASH;
		p = NULL;

		for (e = tab->hash[h]; e && ent != e; e = e->hlink)
			p = e;

		if (e)
		{
			if (p)
				p->hlink = ent->hlink;
			else
				tab->hash[h] = ent->hlink;
		}
	}

	return TRUE;
}

Bool
drop_table(Table *tab, Entry *ent)
{
	Entry *p = NULL;
	Entry *e;
	
	if (tab == NULL || ent == NULL)
		return FALSE;
	
	for (e = tab->list; e && ent != e; e = e->link)
		p = e;
	
	if (e == NULL)
		return FALSE;
	
	if (p)
		p->link = ent->link;
	else
		tab->list = ent->link;

	tab->num--;
	return unhashent_table(tab, ent);
}

Entry *
find_table(Table *table, Byte *name, Int len)
{
	Entry *ent;

	if (table == NULL)
		return NULL;

	setstrlen(&name, &len);

	if (name == NULL || len == 0)
		return NULL;

	if (table->hash)
	{
		ent = table->hash[hash_str(name, len) % TABLE_HASH];

		for (; ent; ent = ent->hlink)
			if (compare_strs(ent->name, PSTR, name, len))
				return ent;
	}
	else
	{
		for (ent = table->list; ent; ent = ent->link)
			if (compare_strs(ent->name, PSTR, name, len))
				return ent;
	}

	return NULL;
}

Table *
clear_table_xtoks(Table *table)
{
	Entry *ent;

	if (table == NULL)
		return NULL;

	/* clear all the xtoks that point at Entries within this table */
	for (ent = table->list; ent; ent = ent->link)
		if (ent->xtok >= 0 && ent->xtok < g_e->numxtoks &&
				g_e->xtoks[ent->xtok] == ent)
			g_e->xtoks[ent->xtok] = g_e->xtoks[0xFC];	/* ferror */

	return table;
}

/* Package stuff */

Package *
new_package(Package *parent)
{
	Package *self;
	
	self = (Package*)malloc(sizeof *self);
	
	if (self == NULL)
	{
		error(E_OUT_OF_MEMORY);
		return NULL;
	}
	
	memset(self, 0, sizeof *self);		/* init everything to zeros */
	self->parent = parent;
	
	if (parent)
	{
		self->link = parent->children;
		parent->children = self; 
	}
	
	self->dict = new_table();
	self->props = new_table();
	self->initinst = new_instance(self);
	return self;
}

void
delete_package(Package *self)
{
	Package *pkg, *prev, *next;

	if (self == NULL)
		return;

	if (self->parent)
	{
		pkg = self->parent->children;
		prev = NULL;

		while (pkg && pkg != self)
		{
			prev = pkg;
			pkg = pkg->link;
		}

		if (pkg)
		{
			if (prev)
				prev->link = self->link;
			else
				self->parent->children = self->link;
		}
	}

	pkg = self->children;

	while (pkg)
	{
		next = pkg->link;
		delete_package(pkg);
		pkg = next;
	}

	delete_instance(self->initinst);
	delete_table(self->props);
	delete_table(clear_table_xtoks(self->dict));

	if (self->disp)
		free(self->disp);

	free(self);
}

Package *
find_package(Package *pkg, Byte *name, Int len)
{
	Package *p;
	Byte *str;
	Int slen;
	
	for (p = pkg->children; p; p = p->link)
		if (prop_get_str(p->props, "name", CSTR, &str, &slen) == NO_ERROR &&
				compare_strs(str, slen, name, len))
			return p;
	
	return NULL;
}


/* Instance stuff */

Instance *
new_instance(Package *pkg)
{
	Instance *self = (Instance*)malloc(sizeof *self);
#ifdef DEBUG
	char buf[STR_SIZE];
	
	get_device_name(g_e, pkg, buf);
	dprintf("new_instance: pkg=%s inst=%p\n", buf, self);
#endif
	
	if (self == NULL)
	{
		error(E_OUT_OF_MEMORY);
		return NULL;
	}
	
	memset(self, 0, sizeof *self);		/* init everything to zeros */
	self->package = pkg;
	self->numunits = get_address_cells(pkg->parent);
	return self;
}

void
delete_instance(Instance *self)
{
#ifdef DEBUG
	char buf[STR_SIZE];

	get_device_name(g_e, self->package, buf);
#endif
	
	if (self == NULL)
		return;

	if (self->package && self->package->initinst)
	{
		if (self->package->initinst == self)
			clear_table_xtoks(self->dict);
		else if (self->package->initinst->dict)
		{
			Entry *ent;

			/* point xtoks to init inst table so see continues to work */
			for (ent = self->package->initinst->dict->list; ent;
					ent = ent->link)
				g_e->xtoks[ent->xtok] = ent;
		}
	}

	if (self->dict)
		delete_table(self->dict);

	if (self->args)
		free(self->args);

#ifdef DEBUG
	dprintf("delete_instance: pkg=%s inst=%p dict=%p args=%p\n",
			buf, self, self->dict, self->args);
#endif

	free(self);
}

Instance *
dup_instance(Instance *self)
{
	Instance *ret;
	
	if (self == NULL)
		return NULL;
	
	ret = new_instance(self->package);

	if (ret == NULL)
		return NULL;

	*ret = *self;		/* copy everything, then dup the important stuff */
	ret->args = pstrdup(self->args);
	ret->dict = dup_table(self->dict);
	return ret;
}


/* Environ stuff */

Environ *
init_environ(Environ *self)
{
	Retcode ret;

	if (self == NULL)
		return NULL;

	memset(self, 0, sizeof *self);		/* init everything to zeros */
	self->sp = self->stack;
	self->rsp = self->rstack;
	self->radix = 16;			/* recommended but not required by 1275 */
	self->numlen = -1;
	self->escape = 0x1D;		/* ^] like for telnet */
	self->keylen = 0;
	self->keyloc = 0;

	/* default pagination for all output */
#ifdef SUN_COMPATIBILITY
	self->paginate = FALSE;
#else
	self->paginate = TRUE;
#endif

	/* allow customizing Forth memory initialization */
#ifdef FORTH_MEM_ALLOC
	self->fmem = (Byte*)FORTH_MEM_ALLOC;
#else
	/* use malloc to allocate a memory arena */
	self->fmem = (Byte*)malloc(MEM_SIZE);
#endif
	self->fbrk = self->fmem;

	if (self->fmem == NULL)
	{
		error(E_OUT_OF_MEMORY);
		return NULL;
	}

	DPRINTF(("\ninit_environ 2\n"));

#ifdef PROPERTY_MEM
	self->propmem = (Byte*)malloc(PROPERTY_MEM);

	if (self->propmem == NULL)
	{
		error(E_OUT_OF_MEMORY);
		return NULL;
	}

	DPRINTF(("\ninit_environ 2\n"));
#endif

#ifdef XTOK_ALLOC
	XTOK_ALLOC		/* allow overriding how this is allocated */
#else
	/* allocate execution-token table */
	self->maxxtoks = NUM_FCODES + 0x300;
	self->xtoks = (Entry**)malloc(self->maxxtoks * sizeof *self->xtoks);
#endif
	/* everything before this is allocated in the spec */
	self->numxtoks = NUM_FCODES;

	if (self->xtoks == NULL)
	{
		error(E_OUT_OF_MEMORY);
		return NULL;
	}

	DPRINTF(("\ninit_environ 3\n"));

	/* allocate and clear the output capture buffer */
	self->capture_buf = (Byte*)malloc(CAPTURE_BUF_SIZE);

	if (self->capture_buf == NULL)
	{
		error(E_OUT_OF_MEMORY);
		return NULL;
	}

	DPRINTF(("\ninit_environ 4\n"));
	memset(self->capture_buf, 0, CAPTURE_BUF_SIZE);

	/* create the input stream buffer and set it up */
	self->in.buf = (Byte*)malloc(STR_SIZE);
	
	if (self->in.buf == NULL)
	{
		error(E_OUT_OF_MEMORY);
		return NULL;
	}
	
	DPRINTF(("\ninit_environ 5\n"));
	self->in.type = I_STREAM;
	
	/* hash the main symbol table and only that table */
	self->names = hash_table(new_table());

	if (self->names == NULL)
	{
		error(E_OUT_OF_MEMORY);
		return NULL;
	}

	DPRINTF(("\ninit_environ 6\n"));
	ret = init_names(self);

	if (ret == NO_ERROR)
		ret = install_packages(self);

	return (ret == NO_ERROR) ? self : NULL;
}

Environ *
destroy_environ(Environ *self)
{
	DPRINTF(("destroy_environ: self=0x%X\n"));
	self->currpkg = NULL;
	self->currinst = 0;
	DPRINTF(("destroy_environ: curr* set to NULL\n"));

	/* close the screen first to revert debug output to failsafe */
	if (self->screen)
	{
		PUSH(self, self->screen);
		self->screen = 0;
		(void)execute_word(self, "close-dev");
		DPRINTF(("destroy_environ: close screen\n"));
	}

	/* then the keyboard, otherwise we may try to poll it for pagination */
	if (self->keyboard)
	{
		PUSH(self, self->keyboard);
		self->keyboard = 0;
		(void)execute_word(self, "close-dev");
		DPRINTF(("destroy_environ: close keyboard\n"));
	}

	delete_package(self->root);
	self->root = NULL;
	DPRINTF(("destroy_environ: deleted self->root\n"));
	delete_table(clear_table_xtoks(self->names));
	self->names = NULL;
	DPRINTF(("destroy_environ: freed self->names\n"));
	free(self->capture_buf);
	self->capture_buf = NULL;
	DPRINTF(("destroy_environ: freed capture buffer\n"));
#ifndef XTOK_ALLOC
	free(self->xtoks);
	self->xtoks = NULL;
	DPRINTF(("destroy_environ: freed xtoks\n"));
#endif
#ifndef FORTH_MEM_ALLOC
	free(self->fmem);
	self->fmem = NULL;
	DPRINTF(("destroy_environ: freed fmem\n"));
#endif
#ifdef PROPERTY_MEM
	free(e->propmem);
	e->propmem = NULL;
#endif
	return self;
}


/* to allocate contiguous Forth memory - this memory is never freed
   - allow overriding it to customize if necessary
 */
#ifndef falloc
Byte *
falloc(Environ *e, uInt size)
{
	Byte *brk = e->fbrk;

	if (brk + size >= e->fmem + MEM_SIZE)
		return NULL;

	e->fbrk += size;
	return brk;
}
#endif	/* falloc */


/* property list routines */

/* allocate memory out of a special memory pool separate from malloc space */
#ifndef prop_alloc
Byte *
prop_alloc(Environ *e, Int len)
{
#ifdef PROPERTY_MEM
	Byte *mem = e->propmem + e->proplen;
	
	if (e->proplen + len >= PROPERTY_MEM)
		return NULL;

	e->proplen += len;
	return mem;
#else
	return falloc(e, len);
#endif
}
#endif	/* prop_alloc */

static Retcode
add_prop(Table *props, Byte *name, Int len, Byte *paddr, Int plen, Bool alloc)
{
	Entry *ent = find_table(props, name, len);
	
	DPRINTF(("add_prop: props=%p name=%s ent=%p paddr=%p plen=%d alloc=%d\n",
			props, name, ent, paddr, plen, alloc));
	if (ent == NULL)
	{
		ent = new_entry();
		setstrlen(&name, &len);
		DPRINTF(("add_prop: prop_alloc %d\n", len + 2));
		ent->name = prop_alloc(g_e, len + 2);
		
		if (ent->name == NULL)
			return E_OUT_OF_MEMORY;			
			
		DPRINTF(("add_prop: memcpy %S\n", name, len));
		memcpy(ent->name + 1, name, len);
		*(uByte*)ent->name = len;
		ent->name[len + 1] = '\0';
		ent->type = T_PROPERTY;
		DPRINTF(("add_prop: insert_table\n"));

		if (!insert_table(props, ent))
			return E_TABLE_ERROR;
	}
	else if (ent->type != T_PROPERTY)
		return E_PROPERTY_ERROR;
	
	if ((ent->flags & F_MALLOC) && ent->v.array)
		free(ent->v.array);

	ent->v.array = paddr;
	ent->len = plen;

	if (alloc)
		ent->flags |= F_MALLOC;
	else
		ent->flags &= ~F_MALLOC;

	DPRINTF(("add_prop: done\n"));
	return NO_ERROR;
}

Retcode
prop_decode_int(Byte **arr, Int *len, Int *val)
{
	Byte *a;
	Int v;
	
	if (*len < sizeof (Int))
		return E_PROPERTY_ERROR;
	
	a = *arr;
	v = (uByte)*a++;
	v = (v << BYTE_SIZE) | (uByte)*a++;
	v = (v << BYTE_SIZE) | (uByte)*a++;
	v = (v << BYTE_SIZE) | (uByte)*a++;

	if (val)
		*val = v;

	*arr = a;
	*len -= sizeof (Int);
	return NO_ERROR;
}

void
prop_encode_int(Byte *arr, Int *len, Int val)
{
	*arr++ = (uByte)(val >> (BYTE_SIZE * 3));
	*arr++ = (uByte)(val >> (BYTE_SIZE * 2));
	*arr++ = (uByte)(val >> BYTE_SIZE);
	*arr++ = (uByte)(val);
	*len += sizeof (Int);
}

Retcode
prop_get_int(Table *props, Byte *name, Int len, Int *val)
{
	Entry *ent = find_table(props, name, len);
	Byte *arr;
	Int alen;
	
	if (ent == NULL || props == NULL)
		return E_NO_PROPERTY;
	
	if (ent->type != T_PROPERTY)
		return E_BAD_PROPERTY_TYPE;
	
	arr = ent->v.array;
	alen = ent->len;
	return prop_decode_int(&arr, &alen, val);
}

Retcode
prop_set_int(Table *props, Byte *name, Int len, Int val)
{
	Byte *arr;
	Int alen;
	
	arr = prop_alloc(g_e, sizeof (Int));

	if (arr == NULL)
		return E_OUT_OF_MEMORY;
	
	alen = 0;
	prop_encode_int(arr, &alen, val);
	return add_prop(props, name, len, arr, alen, FALSE);
}

Retcode
prop_get_ptr(Table *props, Byte *name, Int len, uPtr *val)
{
	Entry *ent = find_table(props, name, len);
	Byte *arr;
	Int alen;
	Retcode ret;
	
	if (ent == NULL || props == NULL)
		return E_NO_PROPERTY;
	
	if (ent->type != T_PROPERTY)
		return E_BAD_PROPERTY_TYPE;
	
	arr = ent->v.array;
	alen = ent->len;
	DPRINTF(("prop_get_ptr: arr %p alen %d x[0] %#x x[1] %#x\n", arr, alen,
		((uInt *)arr)[0], ((uInt *)arr)[1]));

	if (sizeof (uPtr) > sizeof (uInt))
	{
	    uInt vals[2];
	    ret = prop_decode_int(&arr, &alen, &vals[1]);

	    if (ret == NO_ERROR)
		ret = prop_decode_int(&arr, &alen, &vals[0]);

	    DPRINTF(("prop_get_ptr: ret %d vals[0] %#x vals[1] %#x\n", ret, vals[0], vals[1]));
	    *val = ((uPtr)vals[1] << 32) | vals[0];
	    return ret;
	}
	else
	    return prop_decode_int(&arr, &alen, (uInt *)val);
}

Retcode
prop_set_ptr(Table *props, Byte *name, Int len, uPtr val)
{
	Byte *arr;
	Int alen;
	
	arr = prop_alloc(g_e, sizeof (uPtr));

	if (arr == NULL)
		return E_OUT_OF_MEMORY;
	
	alen = 0;

	if (sizeof (uPtr) > sizeof (uInt))
	{
	    prop_encode_int(arr + alen, &alen, val >> 32);
	    prop_encode_int(arr + alen, &alen, val);
	}
	else
	    prop_encode_int(arr, &alen, val);

	return add_prop(props, name, len, arr, alen, FALSE);
}

Int
get_address_cells(Package *pkg)
{
	Retcode ret;
	Int plen;
	
	if (pkg == NULL)
		return 2;
	
	ret = prop_get_int(pkg->props, "#address-cells", CSTR, &plen);

	if (ret == NO_ERROR)
		return plen;
	
	return 2;
}

Int
get_size_cells(Package *pkg)
{
	Retcode ret;
	Int plen;
	
	if (pkg == NULL)
		return 2;
	
	ret = prop_get_int(pkg->props, "#size-cells", CSTR, &plen);

	if (ret == NO_ERROR)
		return plen;
	
	return 1;
}

Retcode
prop_decode_str(Byte **arr, Int *len, Byte **str, Int *slen)
{
	*str = *arr;
	*slen = 0;

	while (*len > 0 && **arr)
	{
		(*arr)++;
		(*len)--;
		(*slen)++;
	}

	return NO_ERROR;
}

void
prop_encode_str(Byte *arr, Int *alen, Byte *str, Int slen)
{
	setstrlen(&str, &slen);
	
	while (slen--)
	{
		*arr++ = *str++;
		(*alen)++;
	}
	
	*arr = '\0';
	(*alen)++;
}

Retcode
prop_get_str(Table *props, Byte *name, Int len, Byte **str, Int *slen)
{
	Entry *ent = find_table(props, name, len);
	Byte *arr;
	Int alen;

	*str = NULL;
	*slen = 0;

	if (ent == NULL)
		return E_NO_PROPERTY;

	if (ent->type != T_PROPERTY)
		return E_BAD_PROPERTY_TYPE;

	arr = ent->v.array;
	alen = ent->len;
	return prop_decode_str(&arr, &alen, str, slen);
}

Retcode
prop_set_str(Table *props, Byte *name, Int len, Byte *str, Int slen)
{
	Byte *arr;
	Int alen;
	
	setstrlen(&str, &slen);
	arr = prop_alloc(g_e, slen + 2);
	
	if (arr == NULL)
		return E_OUT_OF_MEMORY;
		
	alen = 0;
	prop_encode_str(arr, &alen, str, slen);
	return add_prop(props, name, len, arr, alen, FALSE);
}

Retcode
add_property(Table *props, Byte *name, Int len, Byte *paddr, Int plen)
{
	return add_prop(props, name, len, paddr, plen, FALSE);
}

Retcode
set_property(Table *props, Byte *name, Int len, Byte *paddr, Int plen)
{
	Entry *ent = find_table(props, name, len);
	Retcode ret = NO_ERROR;

	if (ent && plen <= ent->len)
		/* copy in the new data over the old */
		memcpy(ent->v.array, paddr, plen);
	else
	{
		Byte *addr = prop_alloc(g_e, plen);

		if (addr == NULL)
			return E_OUT_OF_MEMORY;

		memcpy(addr, paddr, plen);
		ret = add_prop(props, name, len, addr, plen, FALSE);
	}

	return ret;
}

void
delete_property(Table *props, Byte *name, Int len)
{
	Entry *ent = find_table(props, name, len);
	
	if (ent == NULL)
		return;
	
	drop_table(props, ent);
}


/* manipulate configuration variables as stored in /options and nvram */

Retcode
get_config_val(Environ *e, Byte *param, Int plen, Byte **str, Int *slen)
{
	if (prop_get_str(e->options->props, param, plen, str, slen) != NO_ERROR)
	{
		*str = get_nvram(e, param, plen);
		*slen = *str ? strlen(*str) : 0;
	}

	return NO_ERROR;
}

Byte *
get_config(Environ *e, Byte *param, Int plen)
{
	Byte *str = NULL;
	Int slen = 0;
	static Byte strbuf[STR_SIZE];

	*strbuf = '\0';
	(void)get_config_val(e, param, plen, &str, &slen);

	if (str == NULL || slen == 0)
	{
		/* property exists but has no value */
		if (find_table(e->options->props, param, plen))
			return strbuf;

		return NULL;
	}

	memcpy(strbuf, str, slen);
	strbuf[slen] = '\0';
	return strbuf;
}

Retcode
save_config(Environ *e, Byte *param, Int plen, Byte *value, Int vlen,
		Bool nvsave)
{
	Retcode ret = prop_set_str(e->options->props, param, plen, value, vlen);

	if (nvsave && ret == NO_ERROR)
		ret = set_nvram(e, param, plen, value, vlen);

	return ret;
}

Int
get_config_bool(Environ *e, Byte *param, Int plen)
{
	Byte *str;
	Int slen, val;
	
	if (get_config_val(e, param, plen, &str, &slen) == NO_ERROR &&
			str && slen && (*str == 't' || *str == 'T'))
		val = FTRUE;
	else /* if (*str == 'f' || *str == 'F') */
		val = FFALSE;
		
	return val;
}

Cell
get_config_int(Environ *e, Byte *param, Int plen)
{
	Byte *str;
	Int len;
	Cell val = 0;
	Cell err;
	Int radix;
	
	if (get_config_val(e, param, plen, &str, &len) != NO_ERROR)
		return 0;

	if (str == NULL || len == 0)
		return 0;

	if (len >= 2 && str[0] == '0' && str[1] == 'x')
	{
		radix = 16;
		str += 2;
		len -= 2;
	}
	else if (str[0] == '0')
		radix = 8;
	else
		radix = 10;
		
	parse_number(radix, &str, &len, &val, &err, TRUE);
	return val;
}


/* Initentry stuff */
static Bool g_is_builtin = FALSE;

Retcode
init_entries(Environ *e, Table *table, const Initentry initnames[])
{
	const Initentry *ie;
	Entry *ent;

	DPRINTF(("\ninit_entries\n"));

	if (table == NULL)
		return E_TABLE_ERROR;

	for (ie = initnames; ie->name; ie++)
	{
		ent = new_entry();
#ifdef USE_STATIC_NAMES
		ent->name = ie->name;
		ent->flags = ie->flags | F_STATIC_NAME;

		{
			int len = strlen(ent->name);
			int i;

			for (i = len - 1; i >= 0; i--)
				ent->name[i + 1] = ent->name[i];

			*(uByte*)ent->name = len;
		}
#else
		ent->name = cstrdup(ie->name);
		ent->flags = ie->flags;
#endif
		DPRINTF((" %s", ie->name));

		if (e->fcode32)
			ent->flags |= F_FCODE32;

		if (g_is_builtin)
			ent->flags |= F_BUILTIN;

		if (ie->type == 0)
		{
			ent->type = T_FUNC;
			ent->v.cfunc = ie->func;
		}
		else
		{
			ent->type = ie->type;
			
			if (ie->type == T_ADDR || ie->type == T_ADDRVAL)
				ent->v.addr = (Byte*)e + (size_t)(uPtr)ie->func;
			else if (ie->type == T_CONST)
				/* negative numbers may need to be sign-extended */
				ent->v.val = (Int)(uPtr)ie->func;
			else	/* just copy in whatever is there */
				ent->v.cfunc = ie->func;
		}

#ifdef DETAILED_HELP
		ent->len = (Ptr)ie->help;
#endif

		/* now set the execution token value */
		if (ie->fcode < 0)		/* invalid means not defined, so make one */
			set_xtok(e, ent);
		else if (ie->fcode >= NUM_FCODES)
			return E_TABLE_ERROR;
		else
		{
			ent->xtok = ie->fcode;
			e->xtoks[ie->fcode] = ent;
			e->fcodes[ie->fcode] = ie->fcode;
		}
		
		if (!insert_table(table, ent))
			return E_TABLE_ERROR;
	}

	DPRINTF(("\n"));
	return NO_ERROR;
}

static Retcode
f_ferror(Environ *e)			/* xtok/fcode is 0xFC */
{
	return E_ILLEGAL_FCODE;
}

Retcode
init_names(Environ *e)
{
	Entry *ferr;
	Int i;
	Retcode ret;
	/* defined in "machdep.c" - used only here */
	extern const Initentry *init_list[];
	
	/* first create and insert our error routine, so we have a pointer 
	   to it for later */
	ferr = new_entry();
	ferr->name = "\006ferror";
	ferr->flags = F_STATIC_NAME | F_BUILTIN;
	ferr->xtok = 0xFC;
	ferr->type = T_FUNC;
	ferr->v.cfunc = f_ferror;
	
	if (!insert_table(e->names, ferr))
		return E_TABLE_ERROR;

	/* initialize the entire xtoks table (including entry 0xFC) to point
	   to ferror */
	for (i = 0; i < e->maxxtoks; i++)
		e->xtoks[i] = ferr;

	/* initialize entire fcodes table to invalid entries */
	for (i = 0; i < NUM_FCODES; i++)
		e->fcodes[i] = INVALID_FCODE;

	/* finally initialize all our names file by file */
	g_is_builtin = TRUE;

	for (i = 0; init_list[i]; i++)
		if ((ret = init_entries(e, e->names, init_list[i])) != NO_ERROR)
			return ret;

	g_is_builtin = FALSE;
	return NO_ERROR;
}

Package *
new_pkg_name(Package *parent, char *name)
{	
	Package *pkg = new_package(parent);

	if (pkg)
		prop_set_str(pkg->props, "name", CSTR, name, CSTR);

	return pkg;
}

Retcode
install_packages(Environ *e)
{
	Package *pkg;
	Int i;
	Retcode ret;
	Byte buf[STR_SIZE];
	extern const Command install_list[];
	
	bprintf(buf, "%s,%s", MANUFACTURER, MACHINE_TYPE);
	e->root = new_pkg_name(NULL, buf);
	pkg = new_pkg_name(e->root, "openprom");

	if (e->root == NULL || pkg == NULL)
		return E_OUT_OF_MEMORY;

	prop_set_str(pkg->props, "CodeGen-copyright", CSTR,
"SmartFirmware(tm) Copyright 1996-2001 by CodeGen, Inc.  All Rights Reserved.",
			CSTR);
	prop_set_str(pkg->props, "SmartFirmware-version", CSTR,
			SMARTFIRMWARE_REV, CSTR);
	bprintf(buf, "%s,%s", MACHINE_TYPE, FIRMWARE_REV);	
	prop_set_str(pkg->props, "model", CSTR, buf, CSTR);	
	add_property(pkg->props, "relative-addressing", CSTR, NULL, 0);	

	e->aliases = new_pkg_name(e->root, "aliases");
	e->options = new_pkg_name(e->root, "options");

	if (e->aliases == NULL || e->options == NULL)
		return E_OUT_OF_MEMORY;
	e->packages = new_pkg_name(e->root, "packages");

	if (e->packages == NULL)
		return E_OUT_OF_MEMORY;

	/* call all the other initialization routines that setup packages */
	for (i = 0; install_list[i]; i++)
	{
		ret = (*install_list[i])(e);
		
		if (ret != NO_ERROR)
			return ret;
	}
	
	return NO_ERROR;
}


/* Input stuff */

void
push_input(Environ *e, Input *in)
{
	if (in == NULL)
		return;
	
	/* copy the current input, then push it */
	*in = e->in;
	e->in.link = in;
	
	/* now the rest of e->input may be safely initialized */
	e->in.type = I_NONE;
	e->in.buf = NULL;
	e->in.len = 0;
	e->in.loc = 0;
	e->in.fspread = 1;			/* fcodes are 1 byte apart */
	e->in.fnext = MAGIC_FNEXT;	/* magic value as specified in the standard */
	e->in.foffset = TRUE;		/* 16-bit offsets */
}

void
pop_input(Environ *e)
{
	Input *in = e->in.link;
	
	/* copy the previous input */
	if (in)
		e->in = *in;
}


/* misc stuff */

void
setstrlen(Byte **str, Int *len)
{
	if (str == NULL || len == NULL)
		return;
	
	if (*str == NULL)
		*len = 0;
	else if (*len == PSTR)
	{
		*len = *(uByte*)*str;
		*str += 1;
	}
	else if (*len == CSTR)
		*len = strlen(*str);
}

/* dup a C-string, Pascal-string, or Forth string
   - return a '\0'-terminated Pascal-string (usable as a C-string)
 */
Byte *
lstrdup(Byte *str, Int len)
{
	Byte *nstr;
	
	if (str == NULL)
		return NULL;
	
	setstrlen(&str, &len);
	nstr = (Byte*)malloc(len + 2);

	if (nstr == NULL)
	{
		error(E_OUT_OF_MEMORY);
		return NULL;
	}
	
	*(uByte*)nstr = len;
	memcpy(nstr + 1, str, len);
	nstr[len + 1] = '\0';
	return nstr;
}

/* copy a C-string, Pascal-string, or Forth string into a buffer
   as a '\0'-terminated Pascal-string (usable as a C-string)
 */
void
lstrcpy(Byte *nstr, Byte *str, Int len)
{
	if (str == NULL)
	{
		str = "";
		len = 0;
	}
	else
		setstrlen(&str, &len);

	*(uByte*)nstr = len;
	memcpy(nstr + 1, str, len);
	nstr[len + 1] = '\0';
}

Bool
compare_strs(Byte *s1, Int l1, Byte *s2, Int l2)
{
	int c1, c2;
	
	/*setstrlen(&s1, &l1);*/
	/*setstrlen(&s2, &l2);*/

	if (s1 == NULL)
		l1 = 0;
	else if (l1 == PSTR)
	{
		l1 = *(uByte*)s1;
		s1 += 1;
	}
	else if (l1 == CSTR)
		l1 = strlen(s1);

	if (s2 == NULL)
		l2 = 0;
	else if (l2 == PSTR)
	{
		l2 = *(uByte*)s2;
		s2 += 1;
	}
	else if (l2 == CSTR)
		l2 = strlen(s2);

	if (l1 != l2)
		return FALSE;

	if (s1 == NULL || s2 == NULL)
		return FALSE;

	c1 = tolower(*s1);
	c2 = tolower(*s2);

	while (l1 > 0 && c1 == c2)
	{
		l1--;
		s1++;
		s2++;
		c1 = tolower(*s1);
		c2 = tolower(*s2);
	}
	
	return l1 == 0;
}

Int
next_xtok(Environ *e)
{
	/* allocate fcodes out of user-defined range if we are tokenizing */
	if (e->istokenizing)
	{
		if (e->tokfcodes >= NUM_FCODES)
			return error(E_OUT_OF_USER_FCODES);
			
		e->xtoks[e->tokfcodes] = NULL;
		return e->tokfcodes++;
	}
	
	/* otherwise allocate outside of ranges specified in OF spec */
	if (e->numxtoks >= e->maxxtoks)
	{
	#ifdef XTOK_GROW
		XTOK_GROW
	#else
		Int newmax = e->maxxtoks + 0x100;
		Entry **xtoks = (Entry**)malloc(newmax * sizeof *xtoks);
		Entry *ferr = e->xtoks[0xFC];		/* the pointer to ferror */
		Int i;
		
		if (xtoks == NULL)
			return error(E_OUT_OF_MEMORY);
		
		/* copy the old ones */
		memcpy(xtoks, e->xtoks, e->maxxtoks * sizeof *xtoks);

		/* and initialize the new ones */
		for (i = e->maxxtoks; i < newmax; i++)
			xtoks[i] = ferr;

		free(e->xtoks);
		e->xtoks = xtoks;
		e->maxxtoks = newmax;
	#endif /* XTOK_GROW */
	}
	
	e->xtoks[e->numxtoks] = NULL;
	return e->numxtoks++;
}

void
set_xtok(Environ *e, Entry *tok)
{
	tok->xtok = next_xtok(e);
	e->xtoks[tok->xtok] = tok;
}

Entry *
lookup_sym(Environ *e, Byte *str, Int len)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Entry *ent;
	
	/* first try the current instance's symbol table */
	if (inst && inst->dict)
	{
		ent = find_table(inst->dict, str, len);
		
		if (ent)
			return ent;
	}
	
	/* try the current package's symbol table */
	if (e->currpkg && e->currpkg->dict)
	{
		ent = find_table(e->currpkg->dict, str, len);
		
		if (ent)
			return ent;
	}
	
	/* last, try the global symbol table */
	if (e->names)
	{
		ent = find_table(e->names, str, len);
		
		if (ent)
			return ent;
	}
	
	return NULL;
}

Retcode
forget_sym(Environ *e, Byte *str, Int len, Bool all)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Table *tab = NULL;
	Entry *ent = NULL;
	Entry *en;

	/* first try the current instance's symbol table */
	if (inst && inst->dict)
	{
		tab = inst->dict;
		ent = find_table(tab, str, len);
	}

	/* try the current package's symbol table */
	if (ent == NULL && e->currpkg && e->currpkg->dict)
	{
		tab = e->currpkg->dict;
		ent = find_table(tab, str, len);
	}

	/* last, try the global symbol table */
	if (ent == NULL && e->names)
	{
		tab = e->names;
		ent = find_table(tab, str, len);
	}

	if (ent == NULL || tab == NULL || (ent->flags & F_BUILTIN))
		return E_INVALID_FORGET;

	if (all)
	{
		/* Forth-style - forget everything defined after this symbol */
		for (en = tab->list; en != ent; en = en->link)
			if (drop_table(tab, en))
				e->xtoks[en->xtok] = e->xtoks[0xFC];	/* ferror */
	}

	/* forget this symbol */
	if (drop_table(tab, ent))
		e->xtoks[ent->xtok] = e->xtoks[0xFC];	/* ferror */

	return NO_ERROR;
}


/* functions to workaround side-effects of macros */
uInt
swap4(uInt v)
{
	return SWAP4(v);
}

uShort
swap2(uShort v)
{
	return SWAP2(v);
}
