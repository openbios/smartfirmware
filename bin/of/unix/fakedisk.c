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

/* Driver for faking a disk drive in standalone environment.
   May be pointed to a raw disk parition such as /dev/rsd0a
   or to a regular file which will be treated as a virtual disk.
   The filename may be wired into this code, or may be set using
   the Unix environment variable FAKE_DISK_PATH.
 */

#include <stdio.h>
#include "defs.h"


/* override this in the Makefile to point it elsewhere if desired */
#ifndef FAKE_DISK_PATH
#define FAKE_DISK_PATH "fake-disk"
#endif

#define BLOCK_SIZE		512


/* instance-specific vars */
struct self
{
	char *filename;
	FILE *fp;
	long size;
	Instance *disklabel;
	Instance *deblocker;
	uByte buf[BLOCK_SIZE];
};
typedef struct self Self;


C(f_fake_disk_open)			/* open (-- okay?) */
{
	Retcode ret;
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg;
	Self *s;

	DPRINTF(("fake_disk_open: pkg=%p parent=%p\n", pkg, pkg->parent));
	IFCKSP(e, 0, 4);

	if (inst == NULL)
		return E_NULL_INSTANCE;

	pkg = inst->package;

	if (!pkg)
		return E_NULL_PACKAGE;

	/* allocate and clear a self object */
	s = (Self*)malloc(sizeof *s);

	if (s == NULL)
		return E_OUT_OF_MEMORY;

	memset(s, 0, sizeof *s);
	inst->self = s;

	/* set our fake filename */
	s->filename = getenv("FAKE_DISK_PATH");

	if (s->filename == NULL || s->filename[0] == '\0')
		s->filename = FAKE_DISK_PATH;

	/* parse optional arguments */
	if (inst->args && *inst->args)
	{
		/* TODO */
		DPRINTF(("fake_disk_open: args='%s'\n", inst->args));
		;
	}

	/* now open our fake file for reading and seeking, but not writing */
	s->fp = fopen(s->filename, "rb+");

	if (s->fp == NULL)
		return E_NO_FILE;

	/* seek to the end to figure out the file size */
	if (fseek(s->fp, 0, SEEK_END) < 0)
		return E_FILE_IO;

	s->size = ftell(s->fp);
	(void)fseek(s->fp, 0, SEEK_SET);	/* back to beginning */

	/* create an instance of /packages/deblocker to support read/write/etc */
	PUSHP(e, "");	/* arg-str */
	PUSH(e, 0);		/* arg-len */
	PUSHP(e, "deblocker");			/* name-str */
	PUSH(e, strlen("deblocker"));	/* name-len */
	ret = execute_word(e, "$open-package");
	DPRINTF(("fake_disk_open: $open-package deblocker ret=%d:%s\n",
			ret, err2str(ret)));

	if (ret != NO_ERROR)
		return ret;

	POPT(e, s->deblocker, Instance*);
	DPRINTF(("fake_disk_open: deblocker=%p\n", s->deblocker));

	if (s->deblocker == NULL)
		return E_NULL_INSTANCE;

	/* create an instance of /packages/disk-label to support load/etc */
	PUSHP(e, inst->args + 1);		/* arg-str */
	PUSH(e, *(uByte*)inst->args);	/* arg-len */
	PUSHP(e, "disk-label");			/* name-str */
	PUSH(e, strlen("disk-label"));	/* name-len */
	ret = execute_word(e, "$open-package");
	DPRINTF(("fake_disk_open: $open-package disk-label ret=%d:%s\n",
			ret, err2str(ret)));

	if (ret != NO_ERROR)
		return ret;

	POPT(e, s->disklabel, Instance*);
	DPRINTF(("fake_disk_open: disklabel=%p\n", s->disklabel));

	if (s->disklabel == NULL)
		return E_NULL_INSTANCE;

	PUSH(e, FTRUE);
	return ret;
}

C(f_fake_disk_close)			/* close (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;

	DPRINTF(("fake_disk_close\n"));

	if (inst == NULL)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s || !s->deblocker || !s->disklabel || !s->fp)
		return E_BAD_INSTANCE;

	/* close up our helper packages */
	IFCKSP(e, 0, 1);
	PUSHP(e, s->disklabel);
	(void)execute_word(e, "close-package");
	PUSHP(e, s->deblocker);
	(void)execute_word(e, "close-package");

	/* close our fake file */
	fclose(s->fp);

	/* free our self block */
	free(s);

	return NO_ERROR;
}

C(f_fake_disk_read_blocks)		/* read (addr block# #blocks -- #read) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	Byte *addr;
	uInt block, count, seek;
	uInt actual = 0;

	DPRINTF(("fake_disk_read_blocks: inst=%p s=%p\n", inst, s));

	if (inst == NULL)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s || !s->deblocker || !s->fp)
		return E_BAD_INSTANCE;

	IFCKSP(e, 3, 1);
	POP(e, count);
	POP(e, block);
	POPT(e, addr, Byte*);
	seek = fseek(s->fp, block * BLOCK_SIZE, SEEK_SET);

	if (seek == 0)
		actual = fread(addr, count, BLOCK_SIZE, s->fp);

	PUSH(e, actual);

	DPRINTF(("fake_disk_read_blocks: count=%u block=%u addr=%p "
			"seek=%u actual=%u\n",
			count, block, addr, seek, actual));
	return (seek == 0 && actual > 0) ? NO_ERROR : E_FILE_IO;
}

C(f_fake_disk_write_blocks)		/* write (addr block# #blocks -- #written) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	Byte *addr;
	uInt block, count;

	DPRINTF(("fake_disk_write_blocks: inst=%p s=%p\n", inst, s));

	if (inst == NULL)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s || !s->deblocker || !s->fp)
		return E_BAD_INSTANCE;

	IFCKSP(e, 3, 1);
	POP(e, count);
	POP(e, block);
	POPT(e, addr, Byte*);
	DPRINTF(("fake_disk_write_blocks: return\n"));
	return E_UNIMPLEMENTED;		/* too dangerous to implement */
}

C(f_fake_disk_block_size)		/* block-size (-- block-len) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, BLOCK_SIZE);
	return NO_ERROR;
}

C(f_fake_disk_max_transfer)		/* max-transfer (-- max-len) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, BLOCK_SIZE * 4);
	return NO_ERROR;
}

C(f_fake_disk_size)				/* size (-- d) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;

	if (inst == NULL)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (s == NULL)
		return E_BAD_INSTANCE;

	IFCKSP(e, 0, 2);
	PUSH(e, s->size);
	PUSH(e, 0);
	return NO_ERROR;
}

C(f_fake_disk_blocks)			/* #blocks (-- u) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;

	if (inst == NULL)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (s == NULL)
		return E_BAD_INSTANCE;

	IFCKSP(e, 0, 1);
	PUSH(e, (s->size + BLOCK_SIZE - 1) / BLOCK_SIZE);
	return NO_ERROR;
}


enum call_package
{
	C_NONE,
	C_DEBLOCKER,
	C_DISKLABEL
};
typedef enum call_package Call_package;

static Retcode
call_package(Environ *e, Call_package pkgid, Byte *method)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Instance *subpkg;
	Self *s;

	if (!inst)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s)
		return E_BAD_INSTANCE;

	subpkg = (pkgid == C_DEBLOCKER) ? s->deblocker : s->disklabel;

	if (!subpkg)
		return E_NULL_PACKAGE;

	return execute_method_name(e, subpkg, method, CSTR);
}

C(f_fake_disk_read)				/* read (addr len -- actual) */
{
	return call_package(e, C_DEBLOCKER, "read");
}

C(f_fake_disk_write)			/* write (addr len -- actual) */
{
	return call_package(e, C_DEBLOCKER, "write");
}

C(f_fake_disk_seek)			/* seek (pos.lo pos.hi -- status) */
{
	return call_package(e, C_DEBLOCKER, "seek");
}

C(f_fake_disk_load)				/* load (addr -- size) */
{
	return call_package(e, C_DISKLABEL, "load");
}

C(f_fake_disk_selftest)			/* selftest (-- 0|error-code) */
{
	Bool diag = diagnostic_mode(e);
	Retcode ret = NO_ERROR;

	DPRINTF(("fake_disk selftest: enter\n"));

	if (diag)
		cprintf(e, "fake_disk selftest...\n");

	PUSH(e, ret);			/* successful */
	return NO_ERROR;
}


static const Initentry fake_disk_methods[] =
{
	{ "open",           f_fake_disk_open,          INVALID_FCODE },
	{ "close",          f_fake_disk_close,         INVALID_FCODE },

	{ "read-blocks",    f_fake_disk_read_blocks,   INVALID_FCODE },
	{ "write-blocks",   f_fake_disk_write_blocks,  INVALID_FCODE },
	{ "block-size",     f_fake_disk_block_size,    INVALID_FCODE },
	{ "max-transfer",   f_fake_disk_max_transfer,  INVALID_FCODE },

	{ "size",           f_fake_disk_size,          INVALID_FCODE },
	{ "#blocks",        f_fake_disk_blocks,        INVALID_FCODE },

	{ "read",           f_fake_disk_read,          INVALID_FCODE },
	{ "write",          f_fake_disk_write,         INVALID_FCODE },
	{ "seek",           f_fake_disk_seek,          INVALID_FCODE },
	{ "load",           f_fake_disk_load,          INVALID_FCODE },
	{ "selftest",       f_fake_disk_selftest,      INVALID_FCODE },

	{ NULL,             NULL },
};


CC(install_fake_disk_driver)
{
	Package *pkg = new_pkg_name(e->root, "disk");
	Retcode ret;

	DPRINTF(("install_fake_disk_driver: pkg=%p\n", pkg));

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	ret = init_entries(e, pkg->dict, fake_disk_methods);

	if (ret != NO_ERROR)
		return ret;

	prop_set_str(pkg->props, "device_type", CSTR, "block", CSTR);

	DPRINTF(("install_fake_disk_driver: return %s (%d)\n", err2str(ret), ret));
	return ret;
}
