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

/* "disk-label" package - special CodeGen verison supports plug-in filesystems
	Uses following methods of its caller:
		read seek
	Uses following methods of its caller's parent:
		dma-alloc dma-free
 */

#include "defs.h"
#include "fs.h"


struct self
{
	uInt blocksize;		/* size as returned by device */
	uByte *buf;			/* DMA buffer of blocksize length */
	uLong start;		/* start of partition selected at "open" */
	Byte partitions[STR_SIZE];		/* space to append partition types */
	Byte args[STR_SIZE];			/* args passed into "open" method */
};
typedef struct self Self;


/* convert a partition-relative disk offset to absolute position */
static Retcode
rel2abs(Environ *e, Self *s, Int *relhi, Int *rello)
{
#ifdef __LONGLONG
	uLong t = ((uLong)*relhi << QUADLET_SIZE) | *rello;
#else
	uLong t = *rello;
#endif

	t += s->start;

#ifdef __LONGLONG
	*relhi = (t >> QUADLET_SIZE) & QUADLET_MASK;
#endif
	*rello = t & QUADLET_MASK;
	return NO_ERROR;
}

C(f_disklbl_load)		/* load (addr -- size) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	uByte *addr;
	uLong len = 0;
	Retcode ret = NO_ERROR;

	if (inst == NULL || inst->self == NULL)
		return E_BAD_INSTANCE;

	s = inst->self;
	IFCKSP(e, 1, 1);
	POPT(e, addr, uByte*);

	if (e->loadargs && *e->loadargs)
	{
		if (s->args && *s->args)
			strcat(s->args, ",");

		strcat(s->args, e->loadargs);
	}

	DPRINTF(("disk-label load: addr=%p loadargs=%s args=%s\n",
			addr, e->loadargs, s->args));

	ret = file_system(e, FS_LOAD, inst->parent, s->args, 0, 0,
			s->buf, s->blocksize, addr, &len);

	if (ret != NO_ERROR && ret != R_END)
		return ret;

	PUSH(e, len);
	DPRINTF(("disk-label return len=%ld ret=%s (%d)\n",
			len, err2str(ret), ret));
	return NO_ERROR;
}

/* this method is actually inserted into the disk instance and is not
   executed as a part of the disk-label package
   - this allows listing files on any disk device by that disk device
	 without having to know anything about this package (beyond the spec)
 */
C(f_disklbl_list_files)		/* list-files (args alen --) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	Int val;
	Retcode ret;
	Int alen;
	Byte *args;
	Entry *ent;

	if (inst == NULL)
		return E_NULL_INSTANCE;

	ent = find_table(inst->dict, "disk-label-self", CSTR);

	if (ent == NULL)
		return E_BAD_INSTANCE;

	s = (Self*)(uPtr)ent->v.val;

	if (s == NULL)
		return E_BAD_INSTANCE;

	/* make sure we have a C-style null-terminated string */
	IFCKSP(e, 2, 0);
	POP(e, alen);
	POPT(e, args, Byte*);

	if (alen && args)
	{
		if (s->args && *s->args)
			strcat(s->args, ",");

		val = strlen(s->args);
		memcpy(s->args + val, args, alen);
		s->args[val + alen] = '\0';
	}

	DPRINTF(("disk-label list-files: self=%p args=%s\n", s, s->args));
	ret = file_system(e, FS_LIST, inst, s->args, 0, 0,
			s->buf, s->blocksize, NULL, NULL);

	if (ret != NO_ERROR && ret != R_END)
		return ret;

	DPRINTF(("disk-label return ret=%s (%d)\n", err2str(ret), ret));
	return NO_ERROR;
}

static const Initentry disklabel_disk_methods[] =
{
	{ "list-files", f_disklbl_list_files, INVALID_FCODE },
	{ NULL, NULL }
};

static const Initentry disklabel_instance_methods[] =
{
	{ "disk-label-self", (Command)NULL, INVALID_FCODE, F_NONE, T_VALUE },
	{ NULL, NULL }
};

C(f_disklbl_open)			/* open (-- okay?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	Retcode ret;

	DPRINTF(("disk-label open\n"));
	IFCKSP(e, 0, 1);

	if (inst == NULL || inst->self != NULL)
		return E_BAD_INSTANCE;

	/* first allocate space for ourself */
	s = (Self*)malloc(sizeof *s);

	if (s == NULL)
		return E_OUT_OF_MEMORY;

	memset(s, 0, sizeof *s);

	/* get the actual blocksize of our device */
	ret = execute_method_name(e, inst->parent, "block-size", CSTR);

	if (ret == NO_ERROR)
		POP(e, s->blocksize);
	else
		s->blocksize = 512;		/* guess */

	/* allocate space for our DMA-capable data buffer */
	PUSH(e, s->blocksize);
	ret = execute_method_name(e, inst->parent->parent,
			"dma-alloc", CSTR);

	if (ret != NO_ERROR)
	{
		free(s);
		return ret;
	}

	POPT(e, s->buf, uByte*);

	if (s->buf == NULL)
	{
		free(s);
		return E_OUT_OF_MEMORY;
	}

	DPRINTF(("disk-label open: self=%p s->buf=%p\n", s, s->buf));
	inst->self = s;

	/* copy open args for load and list-files methods */
	if (inst->args)
	{
		memcpy(s->args, inst->args + 1, *(uByte*)inst->args);
		s->args[*(uByte*)inst->args] = '\0';
	}

	/* insert a word to be used to get to our Self object */
	if (inst->parent->dict == NULL)
		inst->parent->dict = new_table();

	if (inst->parent->dict == NULL)
	{
		PUSHP(e, s->buf);
		PUSH(e, s->blocksize);
		ret = execute_method_name(e, inst->parent->parent,
				"dma-free", CSTR);
		free(s);
		return E_OUT_OF_MEMORY;
	}

	if (ret == NO_ERROR)
	{
		Entry *ent;

		ret = init_entries(e, inst->parent->dict, disklabel_instance_methods);

		if (ret != NO_ERROR)
			return ret;

		ent = find_table(inst->parent->dict, "disk-label-self", CSTR);

		if (ent == NULL)
		{
			cprintf(e, "disklabel: open: unexpected NULL entry table lookup");
			return E_ABORT;
		}

		ent->v.val = (uPtr)s;
	}

	/* already probed for filesystems? */
	if (!find_table(inst->parent->package->dict, "list-files", CSTR))
	{
		/* hack in our list-files methods into the disk instance's package */
		ret = init_entries(e, inst->parent->package->dict,
				disklabel_disk_methods);

		if (ret != NO_ERROR)
		{
			PUSHP(e, s->buf);
			PUSH(e, s->blocksize);
			ret = execute_method_name(e, inst->parent->parent,
					"dma-free", CSTR);
			free(s);
			return ret;
		}
	}

	PUSH(e, ret == NO_ERROR ? FTRUE : FFALSE);
	DPRINTF(("disk-label open: return %d\n", ret));
	return ret;
}

C(f_disklbl_close)		/* close (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Retcode ret = NO_ERROR;

	DPRINTF(("disk-label close:\n", ret));

	if (inst && inst->self)
	{
		Self *s = inst->self;

		PUSHP(e, s->buf);
		PUSH(e, s->blocksize);
		ret = execute_method_name(e, inst->parent->parent,
				"dma-free", CSTR);
		free(s);
	}

	return ret;
}

C(f_disklbl_offset)		/* offset (d.rel -- d.abs) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int rello, relhi;
	Retcode ret;

	IFCKSP(e, 2, 2);

	if (inst == NULL || inst->self == NULL)
		return E_BAD_INSTANCE;

	POP(e, relhi);
	POP(e, rello);
	ret = rel2abs(e, inst->self, &relhi, &rello);
	PUSH(e, rello);
	PUSH(e, relhi);
	return ret;
}

C(f_disklbl_size)		/* size (-- d) */
{
	IFCKSP(e, 0, 2);

	/* TODO - how to determine size, and of what entity - disk or label? */

	PUSH(e, -1);
	PUSH(e, -1);
	return NO_ERROR;
}

C(f_disklbl_blocks)		/* #blocks (-- u) */
{
	IFCKSP(e, 0, 1);

	/* TODO - how to determine #blocks, and of what entity - disk or label? */

	PUSH(e, -1);
	return NO_ERROR;
}

static const Initentry disklabel_methods[] =
{
	{ "open", f_disklbl_open, INVALID_FCODE },
	{ "close", f_disklbl_close, INVALID_FCODE },
	{ "load", f_disklbl_load, INVALID_FCODE },
	{ "offset", f_disklbl_offset, INVALID_FCODE },
	{ "size", f_disklbl_size, INVALID_FCODE },
	{ "#blocks", f_disklbl_blocks, INVALID_FCODE },
	{ NULL, NULL }
};

CC(install_disklabel)
{
	Package *pkg = new_pkg_name(e->packages, "disk-label");

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	return init_entries(e, pkg->dict, disklabel_methods);
}
