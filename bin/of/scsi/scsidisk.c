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

/* Driver for generic SCSI disk drives.
 */

#include "defs.h"
#include "pci.h"
#include "scsi.h"


#define BLOCK_SIZE		512


/* instance-specific vars */
struct self
{
	struct scsi_rw_big io;
	struct scsi_read_capacity_data rcd;
	uLong blocks, blocksize;
	SCSI_device *dev;
	Instance *disklabel;
	Instance *deblocker;
};
typedef struct self Self;


static Retcode
retry_cmd(Environ *e, Self *s, Byte *buf, Int buflen, Int dir, Int retries,
		Byte *cmd, Int cmdlen, Int *hwerr, Int *status,
		struct scsi_sense_data_new **sense)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Retcode ret;

	IFCKSP(e, 0, 6);

	PUSH(e, s->dev->lun);
	PUSH(e, s->dev->id);
	ret = execute_method_name(e, inst->parent,
			"set-address", CSTR);
	DPRINTF(("scsi-disk: set-address: inst=%p parent=%p ret=%d\n",
			inst, inst->parent, ret));

	if (ret != NO_ERROR)
		return ret;

	PUSHP(e, buf);
	PUSH(e, buflen);
	PUSH(e, dir);
	PUSHP(e, cmd);
	PUSH(e, cmdlen);
	PUSH(e, retries);
	ret = execute_method_name(e, inst->parent,
			"retry-command", CSTR);
	DPRINTF(("scsi-disk: retry-command: inst=%p parent=%p ret=%d\n",
			inst, inst->parent, ret));

	if (ret != NO_ERROR)
		return ret;

	POP(e, *status);
	sense = NULL;

	if (*status)
	{
		POP(e, *hwerr);

		if (*hwerr == 0)
			POPT(e, *sense, struct scsi_sense_data_new*);
		else
			*hwerr = 0;	/* ??? */
	}
	else
		*hwerr = 0;

	return ret;
}

static Retcode
get_blocksize(Environ *e, Instance *inst, Package *pkg, Self *s)
{
	struct scsi_read_capacity *rc = (struct scsi_read_capacity*)&s->io;
	struct scsi_sense_data_new *sense;
	Int hwerr, status;
	Retcode ret;

	memset(rc, 0, sizeof *rc);
	rc->op_code = READ_CAPACITY;
	ret = retry_cmd(e, s, (Byte*)&s->rcd, sizeof s->rcd, FTRUE, 7,
			(Byte *)rc, sizeof *rc, &hwerr, &status, &sense);

	DPRINTF(("scsi_get_blocksize: retry_cmd ret=%d hwerr=%#x status=%#x\n",
			ret, hwerr, status));

	if (ret != NO_ERROR)
		return ret;

	if (status == SCSI_OK && hwerr == 0)
	{
		s->blocks = (s->rcd.addr_3 << (BYTE_SIZE * 3)) |
				(s->rcd.addr_2 << (BYTE_SIZE * 2)) |
				(s->rcd.addr_1 << BYTE_SIZE) |
				s->rcd.addr_0;
		s->blocks++;
		s->blocksize = (s->rcd.length_3 << (BYTE_SIZE * 3)) |
				(s->rcd.length_2 << (BYTE_SIZE * 2)) |
				(s->rcd.length_1 << BYTE_SIZE) |
				s->rcd.length_0;
		DPRINTF(("scsi_get_blocksize: blocks=%#lx blocksize=%#lx\n",
				s->blocks, s->blocksize));
	}
	else
	{
		cprintf(e, "scsi-disk: read error: SCSI status=%#x hwerr=%#x\n",
				status, hwerr);
	}

	return NO_ERROR;
}


C(f_scsi_disk_open)			/* open (-- okay?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s;
	Retcode ret;

	DPRINTF(("scsi_disk_open: pkg=%p parent=%p\n", pkg, pkg->parent));
	IFCKSP(e, 0, 4);

	if (!pkg)
		return E_NULL_PACKAGE;

	/* allocate and clear a self object */
	PUSH(e, sizeof *s);
	ret = execute_method_name(e, inst->parent, "dma-alloc", CSTR);
	DPRINTF(("scsi_disk_open: dma-alloc ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	POPT(e, s, Self*);
	DPRINTF(("scsi_disk_open: dma-alloc s=%d\n", s));

	if (s == NULL)
		return E_OUT_OF_MEMORY;

	memset(s, 0, sizeof *s);
	inst->self = s;
	s->dev = (struct SCSI_device*)pkg->self;	/* get our device info */

	DPRINTF(("scsi_disk_open: ID=%#x LUN=%#x\n", s->dev->id, s->dev->lun));

	/* parse optional arguments */
	if (inst->args && *inst->args)
	{
		Byte *str;
		str = inst->args + 1;
		DPRINTF(("scsi_disk_open: args=\"%s\"\n", str));		
	}

#if 0
	PUSH(e, 5000);		/* change timeout - may have a CD */
	ret = execute_method_name(e, inst->parent, "set-timeout", CSTR);

	if (ret != NO_ERROR)
		return ret;
#endif

	/* get the number of blocks and the size of each block on this disk */
	ret = get_blocksize(e, inst, pkg, s);

	if (ret != NO_ERROR)
		return ret;

	/* create an instance of /packages/deblocker to support read/write/etc */
	PUSH(e, "");	/* arg-str */
	PUSH(e, 0);		/* arg-len */
	PUSHP(e, "deblocker");			/* name-str */
	PUSH(e, strlen("deblocker"));	/* name-len */
	ret = execute_word(e, "$open-package");
	DPRINTF(("scsi_disk_open: $open-package deblocker ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	POPT(e, s->deblocker, Instance*);
	DPRINTF(("scsi_disk_open: deblocker=%p\n", s->deblocker));

	if (s->deblocker == NULL)
		return E_NULL_INSTANCE;

	/* create an instance of /packages/disk-label to support load/etc */
	PUSHP(e, inst->args + 1);		/* arg-str */
	PUSH(e, *(uByte*)inst->args);	/* arg-len */
	PUSHP(e, "disk-label");			/* name-str */
	PUSH(e, strlen("disk-label"));	/* name-len */
	ret = execute_word(e, "$open-package");
	DPRINTF(("scsi_disk_open: $open-package disk-label ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	POPT(e, s->disklabel, Instance*);
	DPRINTF(("scsi_disk_open: disklabel=%p\n", s->disklabel));

	if (s->disklabel == NULL)
		return E_NULL_INSTANCE;

	PUSH(e, ret == NO_ERROR ? FTRUE : FFALSE);
	return ret;
}

C(f_scsi_disk_close)			/* close (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s = inst->self;
	Retcode ret;

	DPRINTF(("scsi_disk_close\n"));

	if (!pkg)
		return E_NULL_PACKAGE;

	if (!s)
		return E_BAD_INSTANCE;

	/* close up our helper packages */
	IFCKSP(e, 0, 1);
	PUSHP(e, s->deblocker);
	(void)execute_word(e, "close-package");
	PUSHP(e, s->disklabel);
	(void)execute_word(e, "close-package");

	/* free our self block */
	PUSHP(e, s);
	PUSH(e, sizeof *s);
	ret = execute_method_name(e, inst->parent, "dma-free", CSTR);
	inst->self = NULL;

	return ret;
}

C(f_scsi_disk_read_blocks)		/* read (addr block# #blocks -- #read) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg;
	Self *s;
	Byte *addr;
	uInt block, count, actual;
	struct scsi_rw *rw;
	struct scsi_rw_big *rwb;
	Bool isbig = FALSE;
	Int hwerr, status;
	struct scsi_sense_data_new *sense;
	Retcode ret;

	DPRINTF(("scsi_disk_read_blocks: enter\n"));

	if (!inst)
		return E_NULL_INSTANCE;

	pkg = inst->package;
	s = inst->self;

	if (!pkg)
		return E_NULL_PACKAGE;

	if (!s || !s->deblocker)
		return E_BAD_INSTANCE;

	rwb = &s->io;
	rw = (struct scsi_rw*)rwb;

	IFCKSP(e, 3, 1);
	POP(e, count);
	POP(e, block);
	POPT(e, addr, Byte*);
	DPRINTF(("scsi_disk_read_blocks: count=%d block=%d addr=%p\n",
			count, block, addr));

	/* use bigger read command only if necessary */
	if ((block & ~0x1FFFFF) || (count & ~0xFF))
	{
		DPRINTF(("scsi_disk_read_blocks: big read\n"));
		isbig = TRUE;
		rwb->op_code = READ_BIG;
		rwb->addr_3 = (block >> 24) & 0xFF;
		rwb->addr_2 = (block >> 16) & 0xFF;
		rwb->addr_1 = (block >> 8) & 0xFF;
		rwb->addr_0 = block & 0xFF;
		rwb->length1 = (count >> 8) & 0xFF;
		rwb->length0 = count & 0xFF;
		rwb->control = 0;
	}
	else
	{
		rw->op_code = READ_COMMAND;
		rw->addr_2 = (block >> 16) & SRW_TOPADDR;
		rw->addr_1 = (block >> 8) & 0xFF;
		rw->addr_0 = block & 0xFF;
		rw->length = count;
		rw->control = 0;
	}

	ret = retry_cmd(e, s, addr, count * s->blocksize, FTRUE, 5,
			(Byte*)rwb, isbig ? sizeof *rwb : sizeof *rw,
			&hwerr, &status, &sense);
	DPRINTF(("scsi_disk_read_blocks: retry_cmd ret=%d hwerr=%#x status=%#x\n",
			ret, hwerr, status));

	if (ret != NO_ERROR)
		return ret;

	if (status == SCSI_OK && hwerr == 0)
		actual = count;
	else
	{
		actual = 0;
		cprintf(e, "scsi-disk: read error: SCSI status=%#x hwerr=%#x\n",
				status, hwerr);
	}

	PUSH(e, actual);
	DPRINTF(("scsi_disk_read_blocks: return actual=%d\n", actual));
	return NO_ERROR;
}

C(f_scsi_disk_write_blocks)		/* write (addr block# #blocks -- #written) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg;
	Self *s;
	Byte *addr;
	uInt block, count;

	DPRINTF(("scsi_disk_write_blocks: enter\n"));

	if (!inst)
		return E_NULL_INSTANCE;

	pkg = inst->package;
	s = inst->self;

	if (!pkg)
		return E_NULL_PACKAGE;

	if (!s || !s->deblocker)
		return E_BAD_INSTANCE;

	IFCKSP(e, 3, 1);
	POP(e, count);
	POP(e, block);
	POPT(e, addr, Byte*);
	DPRINTF(("scsi_disk_write_blocks: return\n"));
	return E_UNIMPLEMENTED;		/* too dangerous to implement */

#if 0
	/* TODO - write count blocks from addr here */
	Int actual;
	actual = count;

	PUSH(e, actual);
	return NO_ERROR;
#endif
}

C(f_scsi_disk_block_size)		/* block-size (-- block-len) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;

	if (!inst)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s)
		return E_BAD_INSTANCE;

	IFCKSP(e, 0, 1);
	PUSH(e, s->blocksize);
	return NO_ERROR;
}

C(f_scsi_disk_max_transfer)		/* max-transfer (-- max-len) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	if (!inst)
		return E_NULL_INSTANCE;

	DPRINTF(("scsi_disk_max_transfer\n"));
	return execute_method_name(e, inst->parent, "max-transfer", CSTR);
}

C(f_scsi_disk_size)				/* size (-- d) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
#ifdef __LONGLONG
	Octlet size;
#endif

	if (!inst)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s)
		return E_BAD_INSTANCE;

	IFCKSP(e, 0, 2);
#ifdef __LONGLONG
	size = s->blocks * s->blocksize;
	PUSH(e, size & QUADLET_MASK);
	PUSH(e, size >> QUADLET_SIZE);
#else
	PUSH(e, s->blocks * s->blocksize);
	PUSH(e, 0);
#endif

	return NO_ERROR;
}

C(f_scsi_disk_blocks)			/* #blocks (-- u) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;

	if (!inst)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s)
		return E_BAD_INSTANCE;

	IFCKSP(e, 0, 1);
	PUSH(e, s->blocks);
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
		return E_NULL_INSTANCE;

	subpkg = (pkgid == C_DEBLOCKER) ? s->deblocker : s->disklabel;

	if (!subpkg)
		return E_BAD_INSTANCE;

	return execute_method_name(e, subpkg, method, CSTR);
}

C(f_scsi_disk_read)				/* read (addr len -- actual) */
{
	return call_package(e, C_DEBLOCKER, "read");
}

C(f_scsi_disk_write)			/* write (addr len -- actual) */
{
	return call_package(e, C_DEBLOCKER, "write");
}

C(f_scsi_disk_seek)			/* seek (pos.lo pos.hi -- status) */
{
	return call_package(e, C_DEBLOCKER, "seek");
}

C(f_scsi_disk_load)				/* load (addr -- size) */
{
	return call_package(e, C_DISKLABEL, "load");
}


C(f_scsi_disk_selftest)			/* selftest (-- 0|error-code) */
{
	Bool diag = diagnostic_mode(e);
	Retcode ret = NO_ERROR;

	DPRINTF(("scsi_disk selftest: enter\n"));

	if (diag)
		cprintf(e, "scsi_disk selftest...\n");

	/* TODO */

	PUSH(e, ret);			/* successful */
	return NO_ERROR;
}

static const Initentry scsi_disk_methods[] =
{
	{ "open",           f_scsi_disk_open,          INVALID_FCODE },
	{ "close",          f_scsi_disk_close,         INVALID_FCODE },

	{ "read-blocks",    f_scsi_disk_read_blocks,   INVALID_FCODE },
	{ "write-blocks",   f_scsi_disk_write_blocks,  INVALID_FCODE },
	{ "block-size",     f_scsi_disk_block_size,    INVALID_FCODE },
	{ "max-transfer",   f_scsi_disk_max_transfer,  INVALID_FCODE },

	{ "size",           f_scsi_disk_size,          INVALID_FCODE },
	{ "#blocks",        f_scsi_disk_blocks,        INVALID_FCODE },

	{ "read",           f_scsi_disk_read,          INVALID_FCODE },
	{ "write",          f_scsi_disk_write,         INVALID_FCODE },
	{ "seek",           f_scsi_disk_seek,          INVALID_FCODE },
	{ "load",           f_scsi_disk_load,          INVALID_FCODE },

	{ "selftest",       f_scsi_disk_selftest,      INVALID_FCODE },

	{ NULL,           NULL },
};

SCSIDEV(install_scsi_disk_driver)
{
	Retcode ret;
	SCSI_device *newdev;

	DPRINTF(("install_scsi_disk_driver: pkg=%p dev=%p ID=%d LUN=%d\n",
			pkg, dev, id, lun));

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	/* allocate space and stash a copy of our dev for our methods */
	newdev = (SCSI_device*)malloc(sizeof *newdev);

	if (newdev == NULL)
		return E_OUT_OF_MEMORY;

	*newdev = *dev;							/* copy it */
	pkg->self = (struct pself*)newdev;		/* and save it */
	newdev->id = id;
	newdev->lun = lun;

	ret = init_entries(e, pkg->dict, scsi_disk_methods);
	DPRINTF(("install_scsi_disk_driver: return %s (%d)\n", err2str(ret), ret));
	return ret;
}
