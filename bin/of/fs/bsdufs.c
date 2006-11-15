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

/* (c) Copyright 1997-1998,2001 by CodeGen, Inc.  All Rights Reserved.  */

/* This file has been copied and highly modified from FreeBSD:
	   /sys/i386/boot/dosboot/sys.c
   The original copyrights follow:
 */

/*
 * Mach Operating System
 * Copyright (c) 1992, 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 *
 *	from: Mach, Revision 2.2  92/04/04  11:36:34  rpd
 *	$Id: bsdufs.c,v 1.14 2003/04/09 06:37:21 parag Exp $
 */
#include "string.h"

#define bcopy(a,b,c)	memcpy(b,a,c)

#include "defs.h"
#include "fs.h"

#include "bsd_types.h"
#include "bsd_queue.h"
#include "bsd_fs.h"
#include "bsd_lock.h"
#include "bsd_inode.h"
#include "bsd_dirent.h"

static char *devs[], *iodest;
static struct fs *fs;
static struct inode inode;
static uInt poff, bnum, cnt;

#define BLOCK_SIZE	512
#define BUFSIZE 4096
#undef MAXBSIZE
#define MAXBSIZE 8192

static char fsbuf[SBSIZE], iobuf[MAXBSIZE];
static char mapbuf[MAXBSIZE];
static uInt mapblock = 0;

static uByte *g_buf;
static Instance *g_disk;
static uInt g_loc;
static uInt g_boff;

static void
devread(void)
{
	uInt offset;
	uInt sector = bnum;
	Retcode ret;

	if (fs && fs->fs_size && bnum >= fsbtodb(fs, fs->fs_size) + g_boff)
	{
		DPRINTF(("BUG: bsd-ufs-read: block %#x > fs-size %#x\n",
				sector, fsbtodb(fs, fs->fs_size)));
		return;
	}

	for (offset = 0; offset < cnt; offset += BLOCK_SIZE)
	{
		ret = filesys_read_bytes(g_e, g_disk,
				sector * BLOCK_SIZE + offset, BLOCK_SIZE, g_buf);

		if (ret != NO_ERROR)
			return;

		memcpy(iodest + offset, g_buf, BLOCK_SIZE);
	}
}

static uInt block_map(uInt file_block)
{
	if (file_block < NDADDR)
		return(inode.i_db[file_block]);
	bnum = fsbtodb(fs, inode.i_ib[0]) + g_boff;
	if (bnum != mapblock) {
		iodest = mapbuf;
		cnt = fs->fs_bsize;
		devread();
		mapblock = bnum;
	}
	return (((uInt *)mapbuf)[(file_block - NDADDR) % NINDIR(fs)]);
}

static void ufs_read(char *buffer, uInt count)
{
	uInt logno, off, size;
	uInt cnt2, bnum2;

	while (count) {
		off = blkoff(fs, poff);
		logno = lblkno(fs, poff);
		cnt2 = size = blksize(fs, &inode, logno);
		bnum2 = fsbtodb(fs, block_map(logno)) + g_boff;
		cnt = cnt2;
		bnum = bnum2;
		DPRINTF(("ufs_read: count=%#x bnum=%#x cnt=%#x\n",count, bnum, cnt));
		if (	(!off)  && (size <= count))
		{
			iodest = buffer;
			devread();
		}
		else
		{
			iodest = iobuf;
			size -= off;
			if (size > count)
				size = count;
			devread();
			bcopy(iodest+off,buffer,size);
		}
		buffer += size;
		count -= size;
		poff += size;
	}

	DPRINTF(("ufs_read: return\n"));
}

#define itog(fs, x)     ((x) / (fs)->fs_ipg)
#define itod(fs, x) \
        ((daddr_t)(cgimin(fs, itog(fs, x)) + \
        (blkstofrags((fs), (((x) % (fs)->fs_ipg) / INOPB(fs))))))

static int find(char *path)
{
	char *rest, ch;
	uInt block, off, loc, ino = ROOTINO;
	struct dirent *dp;
loop:	iodest = iobuf;
	cnt = fs->fs_bsize;
	bnum = fsbtodb(fs, itod(fs, ino)) + g_boff;
	devread();
	bcopy(&((struct dinode *)iodest)[ino % fs->fs_inopb],
	      &inode.i_din,
	      sizeof (struct dinode));
	if (!*path)
		return 1;
	while (*path == '/')
		path++;
	if (!*path)
		return 1;
	if (!inode.i_size || ((inode.i_mode&IFMT) != IFDIR))
		return 0;
	for (rest = path; (ch = *rest) && ch != '/'; rest++) ;
	*rest = 0;
	loc = 0;
	do {
		if (loc >= inode.i_size)
			return 0;
		if (!(off = blkoff(fs, loc))) {
			block = lblkno(fs, loc);
			cnt = blksize(fs, &inode, block);
			bnum = fsbtodb(fs, block_map(block)) + g_boff;
			iodest = iobuf;
			devread();
		}
		dp = (struct dirent *)(iodest + off);
		loc += dp->d_reclen;
	} while (!dp->d_fileno || strcmp(path, dp->d_name));
	ino = dp->d_fileno;
	*(path = rest) = ch;
	goto loop;
}

static int list(char *path)
{
	uInt block, loc, off;
	struct dirent *dp;

	if (!find(path))
		return 0;

	if (!inode.i_size || ((inode.i_mode&IFMT) != IFDIR))
		return 0;
	loc = 0;
	do {
		if (loc >= inode.i_size)
			break;
		if (!(off = blkoff(fs, loc))) {
			block = lblkno(fs, loc);
			cnt = blksize(fs, &inode, block);
			bnum = fsbtodb(fs, block_map(block)) + g_boff;
			iodest = iobuf;
			devread();
		}
		dp = (struct dirent *)(iodest + off);
		loc += dp->d_reclen;

		if (dp->d_type == DT_REG)
			cprintf(g_e, "%s\n", dp->d_name);
		else if (dp->d_type == DT_DIR)
			cprintf(g_e, "[%s]\n", dp->d_name);

	} while (!dp->d_fileno || strcmp(path, dp->d_name));
	return 1;
}

Retcode
bsd_ufs(Environ *e, Filesys_action what, Instance *disk,
		Byte *path, uLong loc, uLong start, uByte *buf, uInt size,
		uByte *retbuf, uLong *val)
{
	Retcode ret = NO_ERROR;

	DPRINTF(("bsd_ufs: enter\n"));

	if (size < BLOCK_SIZE)
		return E_BLOCKSIZE;

	g_disk = disk;
	g_buf = buf;
	g_loc = loc;
	g_boff = start;
	cnt = SBSIZE;
	bnum = SBLOCK + g_boff;
	iodest = fsbuf;
	fs = (struct fs *)fsbuf;
	devread();
	memset(fs->fs_csp, 0, sizeof fs->fs_csp);

#ifdef DEBUG
	display_memory(e, (char*)&fs->fs_nrpos, 32 * sizeof (int));
#endif
	DPRINTF(("bsd_ufs: g_boff=%#x fs_magic=%#x\n", SBLOCK + g_boff,
			fs->fs_magic));

	if (fs->fs_magic != FS_MAGIC)
		return E_NO_FILESYS;

	if (ret != NO_ERROR)
		return ret;

	switch (what)
	{
	case FS_PROBE:
		strcat((char*)retbuf, ",bsd-ufs");
		*val = loc;
		break;

	case FS_LIST:
		if (!list(path))
			cprintf(e, "no such path or directory %s\n", path);

		break;

	case FS_LOAD:
		if (path && *path)		/* load a file */
		{
			if (!find(path) || !inode.i_size || ((inode.i_mode&IFMT) != IFREG))
			{
				cprintf(e, "no such file %s\n", path);
				return E_ABORT;
			}

			poff = 0;
			DPRINTF(("bsd_ufs: calling ufs_read\n"));
			ufs_read(retbuf, inode.i_size);
			DPRINTF(("bsd_ufs: return from ufs_read: val=%p size=%#lx\n",
					val, inode.i_size));
		#ifdef BROKEN_64BIT_INTS
			memcpy((char*)val, (char*)&inode.i_size, sizeof inode.i_size);
		#else
			*val = inode.i_size;
		#endif
			DPRINTF(("bsd_ufs: set val\n"));
		}
		else			/* no file specified - load in 1st (boot) sector */
			cprintf(e, "no such path or directory %s\n", path);

		break;
	}

	/* no other filesystems below us */
	DPRINTF(("bsd_ufs: return R_END\n"));
	return R_END;
}

Filesys g_bsd_ufs =
{
	"bsd-ufs",
	bsd_ufs,
};
