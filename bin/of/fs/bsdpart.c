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

#include "defs.h"
#include "fs.h"
#include "bsd_types.h"
#include "bsd_disklabel.h"

#define BLOCK_SIZE	512

struct fstype
{
	char *name;
};
typedef struct fstype Fstype;

/* position of entry in list matches values of FS_* macros in disklabel.h */
static Fstype g_fslist[] =
{
	{ "unused" },
	{ "swap" },
	{ "v6" },
	{ "v7" },
	{ "sysv" },
	{ "v71k" },
	{ "v8" },
	{ "ffs", },
	{ "msdos" },
	{ "lfs" },
	{ "unknown" },
	{ "hpfs" },
	{ "iso9660" },
	{ "boot" },
	{ NULL }
};
#define MAX_FSLIST	(sizeof g_fslist / sizeof *g_fslist)


Retcode
bsd_partition(Environ *e, Filesys_action what, Instance *disk,
		Byte *path, uLong loc, uLong start_unused, uByte *buf, uInt size,
		uByte *retbuf, uLong *val)
{
	int i;
	int partition = -1;
	Retcode ret;
	struct disklabel d;

	DPRINTF(("bsd_partition: e=%p disk=%p loc=%#lx path=%p buf=%p\n",
			e, disk, loc, path, buf));

	if (size < BLOCK_SIZE)
		return E_BLOCKSIZE;

	ret = filesys_read_bytes(e, disk, loc + LABELSECTOR * BLOCK_SIZE,
			BLOCK_SIZE, buf);
	memcpy((char*)&d, buf + LABELOFFSET, sizeof d);	/* copy our disklabel */

	if (ret != NO_ERROR)
		return ret;

	DPRINTF(("d.d_magic=%#x (DISKMAGIC==%#x)\n", d.d_magic, DISKMAGIC));

#ifdef DEBUG
	display_memory(e, (char*)&d, sizeof d);
#endif

	if (d.d_magic != DISKMAGIC)
		return E_NO_FILESYS;

	if (what == FS_PROBE)
		strcat((char*)retbuf, ",bsd-partition");

	if (isupper(*path))
		*path = tolower(*path);

	if (*path >= 'a' && *path < 'a' + MAXPARTITIONS)
	{
		partition = *path - 'a';
		path = strchr(path, ',');

		if (path == NULL)
			path = "";
		else
			path++;
	}
	else if (*path)
	{
		cprintf(e, "bsd-partition: unknown partition %s\n", path);
		return E_ABORT;
	}

	for (i = 0; i < MAXPARTITIONS; i++)
	{
		struct partition *p = &d.d_partitions[i];
		Retcode ret;
		uInt start, size;

		/* handle alignment and endian problems */
		start = p->p_offset;
		size = p->p_size;
		DPRINTF(("bsd_partition: part=%d size=%#x start=%#x fstype=%#x\n",
				i, size, start, p->p_fstype));

		/* find matching fstype */
		if (what == FS_LIST && partition < 0)
			cprintf(e, "BSD partition %c: %s (%d)\n", i + 'a',
					(p->p_fstype >= MAX_FSLIST) ? "<unknown>" :
							g_fslist[p->p_fstype].name, p->p_fstype);

		/* make sure we have something in this partition */
		if (size == 0 || p->p_fstype == FS_UNUSED ||
				p->p_fstype == FS_SWAP || p->p_fstype == FS_BOOT)
		{
			if (i == partition && what != FS_PROBE)
				return E_NO_FILESYS;

			continue;
		}

		/* handle filesystems inside our partitions */
		switch (what)
		{
		case FS_PROBE:
			ret = file_system(e, what, disk, path, loc, start,
					buf, BLOCK_SIZE, retbuf, val);

			if (ret != NO_ERROR && ret != E_NO_FILESYS)
				return ret;

			break;

		case FS_LIST:
		case FS_LOAD:
			if (i == partition)
			{
				ret = file_system(e, what, disk, path, loc, start,
						buf, BLOCK_SIZE, retbuf, val);

				return (ret == E_NO_FILESYS) ? E_UNSUPPORTED_FILESYS : ret;
			}

			break;
		}
	}

	DPRINTF(("bsdpart: return R_END\n"));
	return R_END;
}

Filesys g_bsd_partition =
{
	"bsd-partition",
	bsd_partition,
};
