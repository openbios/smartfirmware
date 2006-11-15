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

/* (c) Copyright 1999,2001 by CodeGen, Inc.  All Rights Reserved.  */

/* This file has been copied and highly modified from FreeBSD:
	   /sys/i386/boot/dosboot/sys.c
   then vivisected to handle Linux ext2fs filesystems.
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
 *	[Id string removed]
 */

#include "defs.h"
#include "fs.h"

#include "bsd_types.h"
#include "bsd_lock.h"
#include "bsd_queue.h"

#include "ext2_fs2.h"
#include "ext2_fs.h"
#include "ext2_fs_sb.h"

#define BLOCK_SIZE	512
#undef DEV_BSIZE
#define DEV_BSIZE	BLOCK_SIZE
#undef MAXBSIZE
#define MAXBSIZE EXT2_MAX_BLOCK_SIZE

struct ext2_super_block *sb, sbbuf;
struct ext2_sb_info *fs, fsbuf;
static struct ext2_inode inode;
static char iobuf[MAXBSIZE];
static uByte *g_buf;
static Instance *g_disk;
static uInt g_loc;

static void
devread(uInt bnum, uInt cnt, char *iodest)
{
	uInt offset;
	Retcode ret;

	DPRINTF(("devread: bnum=%#x cnt=%#x iodest=%p\n",
			bnum, cnt, iodest));

	for (offset = 0; offset < cnt; offset += BLOCK_SIZE)
	{
//		DPRINTF(("devread:     g_disk=%p loc=%#x len=%#x buf=%p\n",
//				g_disk, bnum * BLOCK_SIZE + offset + g_loc,
//				BLOCK_SIZE, g_buf));
		ret = filesys_read_bytes(g_e, g_disk,
				bnum * BLOCK_SIZE + offset + g_loc, BLOCK_SIZE, g_buf);
//		DPRINTF(("devread:     filesys_read_bytes ret=%s (%d)\n",
//				err2str(ret), ret));

		if (ret != NO_ERROR)
			return;

//		DPRINTF(("devread: memcpy: to=%p from=%p len=%#x\n",
//				iodest + offset, g_buf, BLOCK_SIZE));
		memcpy(iodest + offset, g_buf, BLOCK_SIZE);
	}

//	DPRINTF(("devread: return\n"));
}

#define inode_bmap(inode, nr) (LE4((inode)->i_block[(nr)]))

static inline int block_bmap (uInt * b_data, int nr)
{
    return LE4(((uInt *)b_data)[nr]);
}

/****
Double- and triple-indirect block lookup code for ext2fs was contributed by
Michael Thompson <mickey@berkeley.innomedia.com>.  Thanks!
His log2() assembly code was replaced with TJ's generic C implementation.
****/

/* Return the bit position of the most significant 1 bit in a word */
static int
log2(uInt bits)
{
	int n = 0;

#if QUADLET_SIZE > 32
	if (bits & 0xFFFFFFFF00000000)
	{
		n += 32;
		bits >>= 32;
	}
#endif

	if (bits & 0xFFFF0000)
	{
		n += 16;
		bits >>= 16;
	}

	if (bits & 0xFF00)
	{
		n += 8;
		bits >>= 8;
	}

	if (bits & 0xF0)
	{
		n += 4;
		bits >>= 4;
	}

	switch (bits)
	{
	case 0:
		return -1;
	case 1:
		return n;
	case 2:
	case 3:
		return n + 1;
	case 4:
	case 5:
	case 6:
	case 7:
		return n + 2;
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		return n + 3;
	}

	return -1;
}

static uInt block_map(uInt block)
{
	static char smapbuf[MAXBSIZE];
	static char dmapbuf[MAXBSIZE];
	static char tmapbuf[MAXBSIZE];
	static uInt smapblock = 0;
	static uInt dmapblock = 0;
	static uInt tmapblock = 0;
	int addr_per_block = EXT2_ADDR_PER_BLOCK(sb);
	int addr_per_block_bits = log2(EXT2_ADDR_PER_BLOCK(sb));
	uInt i;

	if (block < EXT2_NDIR_BLOCKS)
		return inode_bmap(&inode, block);

	block -= EXT2_NDIR_BLOCKS;

	if (block < addr_per_block)
	{
		i = inode_bmap(&inode, EXT2_IND_BLOCK);

		if (i != smapblock)
		{						// only devread if new mapblock
			smapblock = i;
			devread(fsbtodb(fs, i), blksize(fs, &inode, i), smapbuf);
		}

		return block_bmap((uInt *)smapbuf, block);
	}

	block -= addr_per_block;

	if (block < (1 << (addr_per_block_bits * 2)))
	{
		i = inode_bmap(&inode, EXT2_DIND_BLOCK);

		if (i != dmapblock)
		{						// only devread if new mapblock
			dmapblock = i;
			devread(fsbtodb(fs, i), blksize(fs, &inode, i), dmapbuf);
		}

		i = block_bmap((uInt *)dmapbuf, block >> addr_per_block_bits);

		if (i != smapblock)
		{
			smapblock = i;
			devread(fsbtodb(fs, i), blksize(fs, &inode, i), smapbuf);
		}

		return block_bmap((uInt *)smapbuf, block & (addr_per_block - 1));
	}

	block -= (1 << (addr_per_block_bits * 2));
	i = inode_bmap(&inode, EXT2_TIND_BLOCK);

	if (i != tmapblock)
	{
		tmapblock = i;
		devread(fsbtodb(fs, i), blksize(fs, &inode, i), tmapbuf);
	}

	i = block_bmap((uInt *)tmapbuf, block >> (addr_per_block_bits * 2));

	if (i != dmapblock)
	{
		dmapblock = i;
		devread(fsbtodb(fs, i), blksize(fs, &inode, i), dmapbuf);
	}

	i = block_bmap((uInt *)dmapbuf, block >> ((addr_per_block_bits >>
					addr_per_block_bits) & (addr_per_block - 1)));

	if (i != smapblock)
	{
		smapblock = i;
		devread(fsbtodb(fs, i), blksize(fs, &inode, i), smapbuf);
	}

	return block_bmap((uInt *)smapbuf, block & (addr_per_block - 1));
}

static void
ext2fs_read(char *buffer, uInt count, uInt poff)
{
	uInt logno, off, size;
	uInt cnt2, bnum2;

	DPRINTF(("ext2fs_read: enter\n"));
	while (count) {
		off = blkoff(fs, poff);
		logno = lblkno(fs, poff);
		cnt2 = size = blksize(fs, &inode, logno);
		bnum2 = fsbtodb(fs, block_map(logno));
		if ((!off)  && (size <= count))
		{
			devread(bnum2, cnt2, buffer);
		}
		else
		{
			size -= off;
			if (size > count)
				size = count;
			devread(bnum2, cnt2, iobuf);
			DPRINTF(("ext2fs_read: memcpy: to=%p from=%p len=%#x\n",
					buffer, iobuf + off, size));
			memcpy(buffer, iobuf+off, size);
		}
		buffer += size;
		count -= size;
		poff += size;
	}
}

static int find(char *path)
{
	uInt ino = EXT2_ROOT_INO;
	char *rest, ch;
	uInt off;
	uInt group;
	uInt block;
	uInt loc;
	struct ext2_dir_entry *dp;
	struct ext2_group_desc *gp;

loop:
	/* first read group_desc containing inode */
	group = ino_to_cg(fs, ino);
	devread(fsbtodb(fs, group / EXT2_DESC_PER_BLOCK(sb) +
			LE4(sb->s_first_data_block) + 1), LE4(fs->s_blocksize), iobuf);

	/* then read the inode */
	off = ((ino - 1) % EXT2_INODES_PER_GROUP(sb)) * EXT2_INODE_SIZE;
	gp = (struct ext2_group_desc *)iobuf;
	gp += group % EXT2_DESC_PER_BLOCK(sb);
	block = LE4(gp->bg_inode_table) + (off >> EXT2_BLOCK_SIZE_BITS(sb));
	off &= EXT2_BLOCK_SIZE(sb) - 1;

	DPRINTF(("find: ino=%#x group=%#x off=%#x block=%#x blocks_count=%#x\n",
			ino, group, off, block, blksize(fs, &inode, block)));
	devread(fsbtodb(fs, block), blksize(fs, &inode, block), iobuf);

	DPRINTF(("find: memcpy: to=%p from=%p len=%#x\n",
			&inode, &((struct ext2_inode *)iobuf)[(ino - 1) % INOPB(fs)],
			sizeof (struct ext2_inode)));
	memcpy(&inode, &((struct ext2_inode *)iobuf)[(ino - 1) % INOPB(fs)],
		  sizeof (struct ext2_inode));

	if (!*path)
		return 1;

	while (*path == '/')
		path++;

	if (!*path)
		return 1;

	if (!inode.i_size || ((LE2(inode.i_mode) & S_IFMT) != S_IFDIR))
		return 0;

	for (rest = path; (ch = *rest) && ch != '/'; rest++)
		;

	*rest = 0;
	loc = 0;

	do
	{
		if (loc >= LE4(inode.i_size))
			return 0;

		if (!(off = blkoff(fs, loc)))
		{
			block = lblkno(fs, loc);
			devread(fsbtodb(fs, block_map(block)),
					blksize(fs, &inode, block), iobuf);
		}

		dp = (struct ext2_dir_entry *)(iobuf + off);
		loc += LE2(dp->rec_len);

	} while (dp->inode == 0 || dp->name_len == 0 ||
			strlen(path) != dp->name_len ||
			memcmp(path, dp->name, dp->name_len));

	ino = LE4(dp->inode);
	*(path = rest) = ch;

	goto loop;
}

static int list(char *path)
{
	uInt block, loc, off;
	struct ext2_dir_entry *dp;

	DPRINTF(("list: enter\n"));

	if (!find(path))
		return 0;

	DPRINTF(("list: inode: i_size=%#x\n", inode.i_size));
	if (!inode.i_size || ((LE2(inode.i_mode) & S_IFMT) != S_IFDIR))
		return 0;
	loc = 0;
	do {
		if (loc >= LE4(inode.i_size))
			break;
		if (!(off = blkoff(fs, loc))) {
			block = lblkno(fs, loc);
			DPRINTF(("list: devread: off=%#x block=%#x block_map=%#x\n",
					off, block, block_map(block)));
			devread(fsbtodb(fs, block_map(block)),
					blksize(fs, &inode, block), iobuf);
		}
		dp = (struct ext2_dir_entry *)(iobuf + off);
		DPRINTF(("list: loc=%#x off=%#x rec_len=%#x name=%S\n",
				loc, off, dp->rec_len, dp->name, dp->name_len));
		loc += LE2(dp->rec_len);

		if (dp->inode == 0 || dp->name_len == 0)
			continue;

		//if ((in->i_mode & S_IFMT) != S_IFDIR)
		cprintf(g_e, "%S\n", dp->name, dp->name_len);
	} while (!dp->inode ||
			strlen(path) != dp->name_len ||
			memcmp(path, dp->name, dp->name_len));
	return 1;
}

Retcode
linux_ext2fs(Environ *e, Filesys_action what, Instance *disk,
		Byte *path, uLong loc, uLong start, uByte *buf, uInt size,
		uByte *retbuf, uLong *val)
{
	DPRINTF(("linux_ext2fs: enter\n"));

	if (size < BLOCK_SIZE)
		return E_BLOCKSIZE;

	g_disk = disk;
	g_buf = buf;
	g_loc = loc;
	sb = &sbbuf;
	fs = &fsbuf;
	devread(SBLOCK, SBSIZE, (char*)sb);
	DPRINTF(("linux_ext2fs: magic=%#x\n", sb->s_magic));

	if (LE2(sb->s_magic) != EXT2_SUPER_MAGIC)
		return E_NO_FILESYS;

    fs->s_blocksize = LE4(EXT2_MIN_BLOCK_SIZE << LE4(sb->s_log_block_size));
    fs->s_bshift = LE4(EXT2_MIN_BLOCK_LOG_SIZE + LE4(sb->s_log_block_size));
    fs->s_fsbtodb = LE4(LE4(sb->s_log_block_size) + 1);
    fs->s_qbmask = LE4(LE4(fs->s_blocksize) - 1);
    fs->s_blocksize_bits = LE4(EXT2_BLOCK_SIZE_BITS(sb));
    fs->s_frag_size = LE4(EXT2_MIN_FRAG_SIZE << LE4(sb->s_log_frag_size));
	DPRINTF(("linux_ext2fs: blocksize=%#x bshift=%#x fstodb=%#x\n",
			LE4(fs->s_blocksize), LE4(fs->s_bshift), LE4(fs->s_fsbtodb)));

    if (fs->s_frag_size)
		fs->s_frags_per_block = LE4(LE4(fs->s_blocksize) /
				LE4(fs->s_frag_size));

	fs->s_blocks_per_group = sb->s_blocks_per_group;
    fs->s_frags_per_group = sb->s_frags_per_group;
    fs->s_inodes_per_group = sb->s_inodes_per_group;
    fs->s_inodes_per_block = LE4(LE4(fs->s_blocksize) / EXT2_INODE_SIZE);
    fs->s_itb_per_group = LE4(LE4(fs->s_inodes_per_group) /
			LE4(fs->s_inodes_per_block));
    fs->s_desc_per_block = LE4(LE4(fs->s_blocksize) /
			sizeof (struct ext2_group_desc));
    fs->s_groups_count = LE4((LE4(sb->s_blocks_count) -
			LE4(sb->s_first_data_block) + EXT2_BLOCKS_PER_GROUP(fs) - 1) /
			EXT2_BLOCKS_PER_GROUP(fs));
    fs->s_db_per_group = LE4((LE4(fs->s_groups_count) +
			EXT2_DESC_PER_BLOCK(sb) - 1) / EXT2_DESC_PER_BLOCK(sb));
	DPRINTF(("linux_ext2fs: blocks_per_group=%#x inodes_per_group=%#x\n",
			LE4(fs->s_blocks_per_group), LE4(fs->s_inodes_per_group)));
	DPRINTF(("linux_ext2fs: groups_count=%#x db_per_group=%#x\n",
			LE4(fs->s_groups_count), LE4(fs->s_db_per_group)));
	DPRINTF(("linux_ext2fs: blocksize=%#x\n", LE4(fs->s_blocksize)));
	
	switch (what)
	{
	case FS_PROBE:
		strcat((char*)retbuf, ",linux-ext2fs");
		*val = loc;
		break;

	case FS_LIST:
		if (!list(path))
			cprintf(e, "no such path or directory %s\n", path);

		break;

	case FS_LOAD:
		if (path && *path)		/* load a file */
		{
			if (!find(path) || !inode.i_size ||
					((LE2(inode.i_mode) & S_IFMT) != S_IFREG))
			{
				cprintf(e, "no such file %s\n", path);
				return E_ABORT;
			}

			ext2fs_read(retbuf, LE4(inode.i_size), 0);
			*val = LE4(inode.i_size);
		}
		else			/* no file specified - load in 1st (boot) sector */
			cprintf(e, "no such path or directory %s\n", path);

		break;
	}

	/* no other filesystems below us */
	DPRINTF(("linux_ext2fs: return\n"));
	return R_END;
}

Filesys g_linux_ext2fs =
{
	"linux-ext2fs",
	linux_ext2fs,
};
