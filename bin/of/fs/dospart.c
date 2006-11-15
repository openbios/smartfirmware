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

/* (c) Copyright 1997-2001 by CodeGen, Inc.  All Rights Reserved. */

#include "defs.h"
#include "fs.h"
#include "dos.h"

#define DOS_BLOCK_SIZE	512		/* MSDOS sector/block size in bytes */


struct dptype
{
	char *name;
	int type;
};
typedef struct dptype DPtype;


static DPtype g_dplist[] =
{
	{ "unused", DP_UNUSED },
	{ "fat12", DP_FAT12, },
	{ "xenix-root", DP_XENIX_ROOT },
	{ "xenix-usr", DP_XENIX_USR },
	{ "fat16", DP_FAT16, },
	{ "extended", DP_EXTENDED },
	{ "fat32", DP_FAT32, },
	{ "hpfs", DP_HPFS },
	{ "aix", DP_AIX },
	{ "aix-boot", DP_AIX_BOOT },
	{ "os2-boot", DP_OS2_BOOT },
	{ "opus", DP_OPUS },
	{ "venix", DP_VENIX },
	{ "dm1", DP_DM_1 },
	{ "dm2", DP_DM_2 },
	{ "cpm", DP_CPM },
	{ "gb", DP_GB },
	{ "speed1", DP_SPEED_1 },
	{ "isc", DP_ISC },
	{ "netware-2", DP_NETWARE_2 },
	{ "netware-3", DP_NETWARE_3 },
	{ "pcix", DP_PCIX },
	{ "minix-1", DP_MINIX_1 },
	{ "minix-15", DP_MINIX_15 },
	{ "linux-swap", DP_LINUX_SWAP },
	{ "linux", DP_LINUX },
	{ "amoeba", DP_AMOEBA },
	{ "amoeba-bbt", DP_AMOEBA_BBT },
	{ "bsd", DP_BSD },			/* FreeBSD, NetBSD, 386BSD */
	{ "bsd", DP_OPENBSD },		/* OpenBSD */
	{ "bsd", DP_OPENBSD2 },		/* OpenBSD II? */
	{ "nextstep", DP_NEXTSTEP },
	{ "bsd", 0xA9 },		/* NetBSD? */
	{ "bsdi", DP_BSDI },
	{ "bsdi-swap", DP_BSDI_SWAP },
	{ "concurrent-cpm", DP_CONCURRENT },
	{ "speed2", DP_SPEED_2 },
	{ "speed3", DP_SPEED_3 },
	{ "speed4", DP_SPEED_4 },
	{ "speed5", DP_SPEED_5 },
	{ "secondary", DP_SECONDARY },
	{ "speed6", DP_SPEED_6 },
	{ "bbt", DP_BBT },
	{ NULL }
};



Retcode
dos_partition(Environ *e, Filesys_action what, Instance *disk,
		Byte *path, uLong loc, uLong start_unused, uByte *buf, uInt size,
		uByte *retbuf, uLong *val)
{
	int i, j;
	Retcode ret = NO_ERROR;
	int partition = -1;
	struct bootsector50 *bs = (struct bootsector50*)buf;
	struct dos_partition part[NDOSPART];
	static int indent = 0;

	DPRINTF(("\ndos_partition: enter\n"));

	if (size < DOS_BLOCK_SIZE)
		return E_BLOCKSIZE;

	/* first see if this is a DOS boot-block */
	ret = filesys_read_bytes(e, disk, loc, size, buf);

	if (ret != NO_ERROR)
		return ret;

	DPRINTF(("dos-partition: boot_sect_sig0/1=%#x.%#x jump=%#x"
			" boot_signature=%#x drive_number=%#x\n",
			bs->boot_sect_sig[0], bs->boot_sect_sig[1],
			bs->jump[0], bs->boot_signature, bs->drive_number));

	if (bs->boot_sect_sig[0] != BOOTSIG0 ||
		bs->boot_sect_sig[1] != BOOTSIG1 ||
		((bs->jump[0] == 0xE9 || bs->jump[0] == 0xEB) &&
			bs->boot_signature != 0x29 &&
			bs->boot_signature != 0x13 &&
			bs->boot_signature != 0x7C &&
			bs->boot_signature != 0xBC))
		return E_NO_FILESYS;

	/* copy the partition info */
	memcpy((void*)part, buf + DOSPARTOFF, sizeof part);

	/* see if we have any partitions - may not be a partition table */
	for (i = 0; i < 4; i++)
	{
		if (part[i].typ != DP_UNUSED)
			break;
	}

	if (i >= 4)
		return E_NO_FILESYS;

	if (*path >= '0' && *path <= '3')
	{
		partition = *path - '0';
		path = strchr(path, ',');

		if (path == NULL)
			path = "";
		else
			path++;
	}
	else if (*path)
	{
		cprintf(e, "dos-partition: unknown partition %s\n", path);
		return E_ABORT;
	}

	if (what == FS_PROBE)
		strcat((char*)retbuf, ",dos-partition");

	for (i = 0; i < 4; i++)
	{
		struct dos_partition *p = &part[i];
		uInt start, size;

		/* handle alignment and endian problems */
		start = p->start[0] |
				(p->start[1] << BYTE_SIZE) |
				(p->start[2] << BYTE_SIZE * 2) |
				(p->start[3] << BYTE_SIZE * 3);
		size = p->size[0] |
				(p->size[1] << BYTE_SIZE) |
				(p->size[2] << BYTE_SIZE * 2) |
				(p->size[3] << BYTE_SIZE * 3);
		DPRINTF(("dos_partition: partition=%d size=%#x start=%#x typ=%#x\n"
				"\tflag=%#x shd=%#x ssect=%#x scyl=%#x ehd=%#x"
				" esect=%#x ecyl=%#x\n",
				i, size, start, p->typ, p->flag, p->shd, p->ssect, p->scyl,
				p->ehd, p->esect, p->ecyl));

		/* find matching fstype */
		if (what == FS_LIST && partition < 0)
		{
			DPtype *fs;

			for (fs = g_dplist; fs->name; fs++)
				if (fs->type == p->typ)
					break;

			for (j = 0; j < indent; j++)
				cprintf(e, " ");

			cprintf(e, "DOS partition %d: %s (%#x)\n", i,
					fs->name ? fs->name : "<unknown>", p->typ);
		}

		/* make sure we have something in this partition */
		if (size == 0 || p->typ == DP_UNUSED)
		{
			if (i == partition && what != FS_PROBE)
				return E_NO_FILESYS;

			continue;
		}

		if (p->typ == DP_EXTENDED)
		{
			indent += 4;
			ret = dos_partition(e, what, disk, path,
					loc + start * DOS_BLOCK_SIZE, start,
					buf, DOS_BLOCK_SIZE, retbuf, val);
			indent -= 4;

			if (ret != NO_ERROR && ret != E_NO_FILESYS && ret != R_END)
				return ret;

			continue;
		}

		/* handle filesystems inside our partitions */
		switch (what)
		{
		case FS_PROBE:
			ret = file_system(e, what, disk, path,
					loc + start * DOS_BLOCK_SIZE, start,
					buf, DOS_BLOCK_SIZE, retbuf, val);

			if (ret != NO_ERROR && ret != E_NO_FILESYS)
				return ret;

			break;

		case FS_LIST:
			if (i == partition)
			{
				ret = file_system(e, what, disk, path,
						loc + start * DOS_BLOCK_SIZE, start,
						buf, DOS_BLOCK_SIZE, retbuf, val);

				return (ret == E_NO_FILESYS) ?  E_UNSUPPORTED_FILESYS : ret;
			}

			break;

		case FS_LOAD:
			if (i == partition)
			{
				if (*path)
				{
					ret = file_system(e, what, disk, path,
							loc + start * DOS_BLOCK_SIZE, start,
							buf, DOS_BLOCK_SIZE, retbuf, val);

					return (ret == E_NO_FILESYS) ?  E_UNSUPPORTED_FILESYS : ret;
				}

				/* no path specified - try to load this partition's bootblock */
				return filesys_read_bytes(e, disk, loc + start * DOS_BLOCK_SIZE,
						*val = DOS_BLOCK_SIZE, retbuf);
			}
			else if (i < 0)
				/* no partition specified - try to load MBR */
				return filesys_read_bytes(e, disk, loc * DOS_BLOCK_SIZE,
						*val = DOS_BLOCK_SIZE, retbuf);

			break;
		}
	}

	/* no other partitions or filesystems should be on this volume */
	DPRINTF(("dos_partition: return R_END\n"));
	return R_END;
}

Filesys g_dos_partition =
{
	"dos-partition",
	dos_partition,
};
