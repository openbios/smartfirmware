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

/* Basic ISO-9660 plug-in filesystem. */

#include "defs.h"
#include "fs.h"

/* The following code contains portions copied from several FreeBSD files:
		/sys/isofs/cd9660/iso.h
	The original copyrights preceed the copied code below.
	Some stuff unnecessary for SmartFirmware has been deleted.
 */

/*-
 * Copyright (c) 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley
 * by Pace Willisson (pace@blitz.com).  The Rock Ridge Extension
 * Support code is derived from software contributed to Berkeley
 * by Atsushi Murai (amurai@spec.co.jp).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)iso.h	8.2 (Berkeley) 1/23/94
 * $Id: iso9660.c,v 1.10 2001/12/15 08:05:46 parag Exp $
 */

#define ISODCL(from, to) (to - from + 1)

struct iso_volume_descriptor {
	uByte type[ISODCL(1,1)]; /* 711 */
	uByte id[ISODCL(2,6)];
	uByte version[ISODCL(7,7)];
	uByte unused[ISODCL(8,8)];
	uByte type_sierra[ISODCL(9,9)]; /* 711 */
	uByte id_sierra[ISODCL(10,14)];
	uByte version_sierra[ISODCL(15,15)];
	uByte data[ISODCL(16,2048)];
};

/* volume descriptor types */
#define ISO_VD_PRIMARY 1
#define ISO_VD_END 255

#define ISO_STANDARD_ID "CD001"
#define ISO_ECMA_ID	"CDW01"

#define ISO_SIERRA_ID	"CDROM"

struct iso_primary_descriptor {
	uByte type			[ISODCL (  1,	1)]; /* 711 */
	uByte id				[ISODCL (  2,	6)];
	uByte version			[ISODCL (  7,	7)]; /* 711 */
	uByte unused1			[ISODCL (  8,	8)];
	uByte system_id			[ISODCL (  9,  40)]; /* achars */
	uByte volume_id			[ISODCL ( 41,  72)]; /* dchars */
	uByte unused2			[ISODCL ( 73,  80)];
	uByte volume_space_size		[ISODCL ( 81,  88)]; /* 733 */
	uByte unused3			[ISODCL ( 89, 120)];
	uByte volume_set_size		[ISODCL (121, 124)]; /* 723 */
	uByte volume_sequence_number	[ISODCL (125, 128)]; /* 723 */
	uByte logical_block_size		[ISODCL (129, 132)]; /* 723 */
	uByte path_table_size		[ISODCL (133, 140)]; /* 733 */
	uByte type_l_path_table		[ISODCL (141, 144)]; /* 731 */
	uByte opt_type_l_path_table	[ISODCL (145, 148)]; /* 731 */
	uByte type_m_path_table		[ISODCL (149, 152)]; /* 732 */
	uByte opt_type_m_path_table	[ISODCL (153, 156)]; /* 732 */
	uByte root_directory_record	[ISODCL (157, 190)]; /* 9.1 */
	uByte volume_set_id		[ISODCL (191, 318)]; /* dchars */
	uByte publisher_id		[ISODCL (319, 446)]; /* achars */
	uByte preparer_id		[ISODCL (447, 574)]; /* achars */
	uByte application_id		[ISODCL (575, 702)]; /* achars */
	uByte copyright_file_id		[ISODCL (703, 739)]; /* 7.5 dchars */
	uByte abstract_file_id		[ISODCL (740, 776)]; /* 7.5 dchars */
	uByte bibliographic_file_id	[ISODCL (777, 813)]; /* 7.5 dchars */
	uByte creation_date		[ISODCL (814, 830)]; /* 8.4.26.1 */
	uByte modification_date		[ISODCL (831, 847)]; /* 8.4.26.1 */
	uByte expiration_date		[ISODCL (848, 864)]; /* 8.4.26.1 */
	uByte effective_date		[ISODCL (865, 881)]; /* 8.4.26.1 */
	uByte file_structure_version	[ISODCL (882, 882)]; /* 711 */
	uByte unused4			[ISODCL (883, 883)];
	uByte application_data		[ISODCL (884, 1395)];
	uByte unused5			[ISODCL (1396, 2048)];
};
#define ISO_DEFAULT_BLOCK_SIZE		2048

struct iso_sierra_primary_descriptor {
	uByte unknown1			[ISODCL (  1,	8)]; /* 733 */
	uByte type			[ISODCL (  9,	9)]; /* 711 */
	uByte id				[ISODCL ( 10,  14)];
	uByte version			[ISODCL ( 15,  15)]; /* 711 */
	uByte unused1			[ISODCL ( 16,  16)];
	uByte system_id			[ISODCL ( 17,  48)]; /* achars */
	uByte volume_id			[ISODCL ( 49,  80)]; /* dchars */
	uByte unused2			[ISODCL ( 81,  88)];
	uByte volume_space_size		[ISODCL ( 89,  96)]; /* 733 */
	uByte unused3			[ISODCL ( 97, 128)];
	uByte volume_set_size		[ISODCL (129, 132)]; /* 723 */
	uByte volume_sequence_number	[ISODCL (133, 136)]; /* 723 */
	uByte logical_block_size		[ISODCL (137, 140)]; /* 723 */
	uByte path_table_size		[ISODCL (141, 148)]; /* 733 */
	uByte type_l_path_table		[ISODCL (149, 152)]; /* 731 */
	uByte opt_type_l_path_table	[ISODCL (153, 156)]; /* 731 */
	uByte unknown2			[ISODCL (157, 160)]; /* 731 */
	uByte unknown3			[ISODCL (161, 164)]; /* 731 */
	uByte type_m_path_table		[ISODCL (165, 168)]; /* 732 */
	uByte opt_type_m_path_table	[ISODCL (169, 172)]; /* 732 */
	uByte unknown4			[ISODCL (173, 176)]; /* 732 */
	uByte unknown5			[ISODCL (177, 180)]; /* 732 */
	uByte root_directory_record	[ISODCL (181, 214)]; /* 9.1 */
	uByte volume_set_id		[ISODCL (215, 342)]; /* dchars */
	uByte publisher_id		[ISODCL (343, 470)]; /* achars */
	uByte preparer_id		[ISODCL (471, 598)]; /* achars */
	uByte application_id		[ISODCL (599, 726)]; /* achars */
	uByte copyright_id		[ISODCL (727, 790)]; /* achars */
	uByte creation_date		[ISODCL (791, 806)]; /* ? */
	uByte modification_date		[ISODCL (807, 822)]; /* ? */
	uByte expiration_date		[ISODCL (823, 838)]; /* ? */
	uByte effective_date		[ISODCL (839, 854)]; /* ? */
	uByte file_structure_version	[ISODCL (855, 855)]; /* 711 */
	uByte unused4			[ISODCL (856, 2048)];
};

/* directory flags */
#define	ISO_HIDDEN			0x01		/* is hidden (not visible) */
#define	ISO_DIRECTORY		0x02		/* is a directory */
#define	ISO_ASSOC_FILE		0x04		/* associated file */
#define	ISO_RECORD_FMT		0x08		/* structured file */
#define	ISO_PROTECTED		0x10		/* see owner/perm's */
#define	ISO_MULT_EXTENT		0x80		/* not file's final extent? */

#define	ISO_MAX_LEN_DR		255			/* max directory length */

struct iso_directory {
	uByte length			[ISODCL (1, 1)]; /* 711 */
	uByte ext_attr_length		[ISODCL (2, 2)]; /* 711 */
	uByte extent		[ISODCL (3, 10)]; /* 733 */
	uByte size		[ISODCL (11, 18)]; /* 733 */
	uByte date			[ISODCL (19, 25)]; /* 7 by 711 */
	uByte flags			[ISODCL (26, 26)];
	uByte file_unit_size		[ISODCL (27, 27)]; /* 711 */
	uByte interleave			[ISODCL (28, 28)]; /* 711 */
	uByte volume_sequence_number	[ISODCL (29, 32)]; /* 723 */
	uByte name_len			[ISODCL (33, 33)]; /* 711 */
	uByte name			[256];
};
/* size of dir-rec minus actual name buffer */
#define ISO_DIRECTORY_SIZE	33

struct iso_extended_attributes {
	uByte owner		[ISODCL (1, 4)]; /* 723 */
	uByte group		[ISODCL (5, 8)]; /* 723 */
	uByte perm		[ISODCL (9, 10)]; /* 9.5.3 */
	uByte ctime			[ISODCL (11, 27)]; /* 8.4.26.1 */
	uByte mtime			[ISODCL (28, 44)]; /* 8.4.26.1 */
	uByte xtime			[ISODCL (45, 61)]; /* 8.4.26.1 */
	uByte ftime			[ISODCL (62, 78)]; /* 8.4.26.1 */
	uByte recfmt			[ISODCL (79, 79)]; /* 711 */
	uByte recattr			[ISODCL (80, 80)]; /* 711 */
	uByte reclen		[ISODCL (81, 84)]; /* 723 */
	uByte system_id			[ISODCL (85, 116)]; /* achars */
	uByte system_use			[ISODCL (117, 180)];
	uByte version			[ISODCL (181, 181)]; /* 711 */
	uByte len_esc			[ISODCL (182, 182)]; /* 711 */
	uByte reserved			[ISODCL (183, 246)];
	uByte len_au		[ISODCL (247, 250)]; /* 723 */
};

/* watch out for side-effects - functions converted to macros */
#define isonum_711(p)	(*(p))
#define isonum_712(p) 	(*(p))
#define isonum_721(p)	(*(p) | ((uByte)(p)[1] << 8))
#define isonum_722(p)	(((uByte)*(p) << 8) | (p)[1])
#define isonum_723(p)	isonum_721(p)
#define isonum_731(p)	(*(p) | ((p)[1] << 8) | ((p)[2] << 16) | ((p)[3] << 24))
#define isonum_732(p)	((*(p) << 24) | ((p)[1] << 16) | ((p)[2] << 8) | (p)[3])
#define isonum_733(p)	isonum_731(p)

/*
 * Associated files have a leading '='.
 */
#define	ASSOCCHAR	'='

/* END OF COPIED CODE */


#define BLOCK_SIZE	ISO_DEFAULT_BLOCK_SIZE


Retcode
iso_read_blocks(Environ *e, Instance *disk, uLong loc, uInt len,
		uByte *buf, uInt iosize, uByte *iobuf)
{
	uInt l;
	Retcode ret = NO_ERROR;

	if (iosize > BLOCK_SIZE)
		iosize = BLOCK_SIZE;

	do 
	{
		ret = filesys_read_bytes(e, disk, loc, iosize, iobuf);

		if (ret != NO_ERROR)
			return ret;

		l = (iosize < len) ? iosize : len;
		memcpy(buf, iobuf, l);
		len -= l;
		buf += l;
		loc += l;
	} while (len > 0);

	return ret;
}

Int
iso_strlen(uByte *str, Int len)
{
	while (len > 0 && str[len - 1] == ' ')
		len--;

	return len;
}

void
iso_print_str(Environ *e, uByte *str, Int len)
{
	display_text(e, (Byte*)str, iso_strlen(str, len));
}

Retcode
iso_walk(Environ *e, Filesys_action what, Instance *disk,
		struct iso_primary_descriptor *ipd, Byte *path, uLong loc,
		uInt blocksize, uByte *iobuf, uInt iosize,
		uByte *retbuf, uLong *retval)
{
	struct iso_directory *dir =
			(struct iso_directory*)ipd->root_directory_record;
	uInt length = 0;
	uInt extent, size;
	uInt sz;
	Retcode ret;
	uByte *buf = NULL;
	Byte *s, name[STR_SIZE];

	cprintf(e, "ISO-9660 filesystem:  System-ID: \"");
	iso_print_str(e, ipd->system_id, sizeof ipd->system_id);
	cprintf(e, "\"  Volume-ID: \"");
	iso_print_str(e, ipd->volume_id, sizeof ipd->volume_id);
	cprintf(e, "\"\n");

	/* walk directories looking for the next element of path */
	extent = isonum_733(dir->extent);
	size = isonum_733(dir->size);

#ifdef DEBUG
	cprintf(e, "Root dir: \"");
	display_text(e, dir->name, isonum_711(dir->name_len));
	cprintf(e, "\" flags=%#x extent=%#x size=%#x\n",
			isonum_711(dir->flags), extent, size);
#endif

	if (*path == '/' || *path == '\\')	/* root dir */
		path++;

	while (isonum_711(dir->flags) & ISO_DIRECTORY)
	{
		s = strchr(path, '/');

		if (s == NULL)
			s = strchr(path, '\\');

		if (s)
		{
			strncpy(name, path, s - path);
			name[s - path] = '\0';
			path = s + 1;
		}
		else
		{
			strcpy(name, path);
			path = "";
		}

		if (*name)
		{
			/* allocate space for this entire directory then read it in */
			if (buf)
				free(buf);

			buf = (uByte*)malloc(size);

			if (buf == NULL)
				return E_OUT_OF_MEMORY;

			ret = iso_read_blocks(e, disk, extent * blocksize, size,
					buf, iosize, iobuf);
			DPRINTF(("iso_walk: read: name=\"%s\" extent=%#x size=%#x ret=%s\n",
					name, extent, size, err2str(ret)));

			if (ret != NO_ERROR)
				break;

			/* walk the directory looking for this path component */
			for (sz = 0; sz < size; sz += length)
			{
				dir = (struct iso_directory*)(buf + sz);
				length = isonum_711(dir->length);

				if (compare_strs((Byte*)dir->name, isonum_711(dir->name_len),
						name, CSTR))
					break;

				dir = (struct iso_directory*)((Byte*)dir + length);

				/* skip to next block if end of this one */
				if (isonum_711(dir->length) == 0 && sz < size)
					length = blocksize - (sz % blocksize);
			}

			if (length == 0 || sz >= size)		/* no match */
				break;
		}

		extent = isonum_733(dir->extent);
		size = isonum_733(dir->size);

		if (*path == '\0' && (isonum_711(dir->flags) & ISO_DIRECTORY))
		{
			if (what != FS_LIST)
				break;

			free(buf);
			buf = (uByte*)malloc(size);

			if (buf == NULL)
				return E_OUT_OF_MEMORY;

			ret = iso_read_blocks(e, disk, extent * blocksize, size,
					buf, iosize, iobuf);
			DPRINTF(("iso_walk: read: name=\"%s\" extent=%#x size=%#x ret=%s\n",
					name, extent, size, err2str(ret)));

			if (ret != NO_ERROR)
				break;

			/* walk the directory printing all contents */
			dir = (struct iso_directory*)buf;
			length = isonum_711(dir->length);

			/* skip the 1st two entries (. and ..) */
			sz = length;
			dir = (struct iso_directory*)((Byte*)dir + length);
			length = isonum_711(dir->length);
			sz += length;
			dir = (struct iso_directory*)((Byte*)dir + length);
			length = isonum_711(dir->length);

			for (; sz < size; sz += length)
			{
				dir = (struct iso_directory*)(buf + sz);

				if (isonum_711(dir->flags) & ISO_DIRECTORY)
					cprintf(e, "          [");
				else
					cprintf(e, "%8d  ", isonum_733(dir->size));

				display_text(e, (Byte*)dir->name, isonum_711(dir->name_len));

				if (isonum_711(dir->flags) & ISO_DIRECTORY)
					cprintf(e, "]\n");
				else
					cprintf(e, "\n");

				length = isonum_711(dir->length);
				dir = (struct iso_directory*)((Byte*)dir + length);

				/* skip to next block if end of this one */
				if (isonum_711(dir->length) == 0 && sz < size)
					length = blocksize - (sz % blocksize);
			}

			free(buf);
			return NO_ERROR;
		}
		else if (*path == '\0')
		{
			if (what == FS_LIST)
			{
				cprintf(e, "%8d  ", size);
				display_text(e, (Byte*)dir->name, isonum_711(dir->name_len));
				cprintf(e, "\n");
			}
			else
			{
				ret = iso_read_blocks(e, disk, extent * blocksize, size,
						retbuf, iosize, iobuf);
				DPRINTF(("iso_walk: read: name=\"%s\" extent=%#x size=%#x"
						" ret=%s\n", name, extent, size, err2str(ret)));

				if (ret != NO_ERROR)
					break;

				*retval = size;
			}

			free(buf);
			return NO_ERROR;
		}
	}

	if (buf)
		free(buf);

	return E_NO_FILE;
}

Retcode
iso9660(Environ *e, Filesys_action what, Instance *disk,
		Byte *path, uLong loc, uLong start, uByte *buf, uInt size,
		uByte *retbuf, uLong *val)
{
	int bnum;
	Retcode ret = NO_ERROR;
	struct iso_primary_descriptor *ipd, ipbuf;

	DPRINTF(("iso9660: e=%p disk=%p loc=0x%lX path=%p buf=%p\n",
			e, disk, loc, path, buf));
	ipd = &ipbuf;

	/* look for an ID block at the beginning of the CD */
	for (bnum = 16; bnum < 32; bnum++)
	{
		ret = iso_read_blocks(e, disk, loc + bnum * BLOCK_SIZE, 
				sizeof *ipd, (uByte*)ipd, size, buf);

		if (ret != NO_ERROR)
			return ret;

		/* only look for ISO-9660 at the moment - no Sierra or RockRidge */
		if (!memcmp(ipd->id, ISO_STANDARD_ID, sizeof ipd->id))
			break;
	}

	if (bnum >= 32)
		return E_NO_FILESYS;

	switch (what)
	{
	case FS_PROBE:
		strcat((char*)retbuf, ",iso9660");
		*val = loc;
		break;

	case FS_LIST:
		ret = iso_walk(e, what, disk, ipd, path, loc,
				isonum_723(ipd->logical_block_size), buf, size,
				retbuf, val);

		if (ret == E_NO_FILE)
			cprintf(e, "no such file or directory %s\n", path);

		break;

	case FS_LOAD:
		if (path && *path)		/* load a file */
		{
			ret = iso_walk(e, what, disk, ipd, path, loc,
					isonum_723(ipd->logical_block_size), buf, size,
					retbuf, val);

			if (ret == E_NO_FILE)
				cprintf(e, "no such file %s\n", path);
		}
		else			/* no file specified - load in 1st (boot) sector */
			cprintf(e, "no such path or directory %s\n", path);

		break;
	}


	DPRINTF(("iso9660: return R_END\n"));
	return ret == NO_ERROR ? R_END : ret;
}

Filesys g_iso9660_fs =
{
	"iso9660",
	iso9660,
};
