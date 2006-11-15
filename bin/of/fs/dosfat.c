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

/* This file has been copied and edited from parts of mtools-2.0.7, which
   appears to have no original copyright notice.  Portions of these files
   have been catenated together here and ANSI-fied:
	   msdos.h is_dir.c subdir.c dir_read.c fat_read.c file_read.c
	   init.c mread.c mdir.c parse.c
 */

#include "defs.h"
#include "fs.h"
#include "dos.h"

#define MSECTOR_SIZE	512		/* MSDOS sector size in bytes */
#define MDIR_SIZE		32		/* MSDOS directory size in bytes */
#define MAX_CLUSTER		8192	/* largest cluster size */
#define MAX_PATH		128		/* largest MSDOS path length */
#define MAX_DIR_SECS	64		/* largest directory (in sectors) */

#define NEW		1
#define OLD		0

/*#define CHK_FAT*/
#define FULL_CYL
#define WORD(x) ((boot->x)[0] + ((boot->x)[1] << 8))
#define DWORD(x) ((boot->x)[0] + ((boot->x)[1] << 8) + ((boot->x)[2] << 16) + ((boot->x)[3] << 24))

/* map mtools bootsector struct fields to BSD names */
#define bootsector bootsector50
#define secsiz bytes_per_sec
#define clsiz sec_per_clust
#define nrsvsect res_sectors
#define nfat fats
#define dirents root_dir_ents
#define psect sectors
#define descr media
#define fatlen fat_secs
#define nsect sec_per_track
#define nheads heads
#define nhs hidden_secs
#define bigsect huge_sectors
#define drivenum drive_number
#define extbootsig boot_signature
#define id volume_id
#define label volume_label
#define fstype file_sys_type
#define bootcode boot_code
#define bootsig boot_sect_sig


struct directory
{
	uChar name[8];		/* file name */
	uChar ext[3];		/* file extension */
	uChar attr;			/* attribute byte */
	uChar reserved[10];	/* ?? */
	uChar time[2];		/* time stamp */
	uChar date[2];		/* date stamp */
	uChar start[2];		/* starting cluster number */
	uChar size[4];		/* size of the file */
};


static Int dir_chain[MAX_DIR_SECS];		/* chain of sectors in directory */
static uChar *dir_buf;						/* the directory buffer */

static Int fat_len;				/* length of FAT table (in sectors) */
static uInt end_fat;			/* the end-of-chain marker */
static uInt last_fat;			/* the last in a chain marker */
static uChar *fat_buf;			/* the File Allocation Table */

static Int dir_start;				/* starting sector for directory */
static Int dir_len;				/* length of directory (in sectors) */
static Int dir_entries;			/* number of directory entries */
static Int clus_size;				/* cluster size (in sectors) */
static Int fat_error;				/* FAT error detected? */

static uInt num_clus;		/* total number of cluster */
static Int num_fat;				/* the number of FAT tables */
static Int disk_offset;			/* skip this many bytes */
static Int fat_bits;				/* the FAT encoding scheme */

static Int disk_size;
static uChar *disk_buf;
extern struct device devices[];


/* for disk_read below */
static uByte *g_buf;
static Instance *g_disk;

static Retcode
disk_read(Int start, uByte *buf, Int len)
{
	Int i;
	Retcode ret;

	for (i = 0; i < len; i += MSECTOR_SIZE)
	{
		ret = filesys_read_bytes(g_e, g_disk,
				start * MSECTOR_SIZE + i + disk_offset, MSECTOR_SIZE, g_buf);

		if (ret != NO_ERROR)
			return ret;

		memcpy(buf, g_buf, (len - i > MSECTOR_SIZE) ? MSECTOR_SIZE : len - i);
		buf += MSECTOR_SIZE;
	}

	return NO_ERROR;
}


/*
* Read a directory entry, return a pointer a static structure.
*/

static struct directory *
dir_read(Int num)
{
	uChar *offset;
	static struct directory dir;

	offset = dir_buf + (num * MDIR_SIZE);
	memcpy((Char *) &dir, (Char *) offset, MDIR_SIZE);
	return(&dir);
}

/*
* Get and decode a FAT (file allocation table) entry.  Returns the cluster
* number on success or 1 on failure.
*/

static uInt
fat_decode(uInt num)
{
	uInt fat, fat_hi, fat_low, byte_1, byte_2, start;

	if (fat_bits == 12) {
		/*
		 *	|    byte n     |   byte n+1    |   byte n+2    |
		 *	|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
		 *	| | | | | | | | | | | | | | | | | | | | | | | | |
		 *	| n+0.0 | n+0.5 | n+1.0 | n+1.5 | n+2.0 | n+2.5 |
		 *	    \_____  \____   \______/________/_____   /
		 *	      ____\______\________/   _____/  ____\_/
		 *	     /     \      \          /       /     \
		 *	| n+1.5 | n+0.0 | n+0.5 | n+2.0 | n+2.5 | n+1.0 |
		 *	|      FAT entry k      |    FAT entry k+1      |
		 */
					/* which bytes contain the entry */
		start = num * 3 / 2;
		if (start <= 2 || start + 1 > (fat_len * MSECTOR_SIZE))
			return(1);

		byte_1 = *(fat_buf + start);
		byte_2 = *(fat_buf + start + 1);
					/* (odd) not on byte boundary */
		if (num % 2) {
			fat_hi = (byte_2 & 0xff) << 4;
			fat_low = (byte_1 & 0xf0) >> 4;
		}
					/* (even) on byte boundary */
		else {
			fat_hi = (byte_2 & 0xf) << 8;
			fat_low = byte_1 & 0xff;
		}
		fat = (fat_hi + fat_low) & 0xfff;
	}
	else {
		/*
		 *	|    byte n     |   byte n+1    |
		 *	|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
		 *	| | | | | | | | | | | | | | | | |
		 *	|         FAT entry k           |
		 */
					/* which bytes contain the entry */
		start = num * 2;
		if (start <= 3 || start + 1 > (fat_len * MSECTOR_SIZE))
			return(1);

		fat = (*(fat_buf + start + 1) * 0x100) + *(fat_buf + start);
	}
	return(fat);
}

/*
* Fill in the global variable dir_chain[].  Argument is the starting
* cluster number.  Returns E_ABORT on error.
*/

static Retcode
fill_chain(uInt num)
{
	Int i, length;
	uInt next;
	uChar *offset;
	Retcode ret;

	length = 0;
	/* CONSTCOND */
	while (1) {
	dir_chain[length] = (Int) (num - 2) * clus_size + dir_start + dir_len;
	length++;
				/* sectors, not clusters! */
	for (i = 1; i < clus_size; i++) {
		dir_chain[length] = dir_chain[length - 1] + 1L;
		length++;
	}

#if 0
		if (length >= MAX_DIR_SECS) {
			cprintf(g_e, "fill_chain: directory too large\n");
			return E_ABORT;
		}
#endif
					/* get next cluster number */
		next = fat_decode(num);
		if (next == 1) {
			cprintf(g_e, "fill_chain: FAT problem\n");
			fat_error++;
			return E_ABORT;
		}
					/* end of cluster chain */
		if (next >= last_fat)
			break;
		num = next;
	}

	/* fill the dir_buf */
	dir_buf = (uChar *)realloc(dir_buf, (uInt) length * MSECTOR_SIZE);
	if (dir_buf == NULL)
		return E_OUT_OF_MEMORY;

	for (i = 0; i < length; i++) {
		offset = dir_buf + (i * MSECTOR_SIZE);
		ret = disk_read(dir_chain[i], offset, MSECTOR_SIZE);

		if (ret != NO_ERROR)
			return ret;
	}

	dir_entries = length * 16;
	return NO_ERROR;
}

/*
 * Reset the global variable dir_chain[] to the root directory.
 */

static Retcode
reset_chain(code)
Int code;
{
	Int i;
	Retcode ret;

	for (i = 0; i < dir_len; i++)
		dir_chain[i] = (Int) dir_start + i;

	if (code == OLD)
		dir_buf = (uChar *)realloc(dir_buf, (uInt) dir_len * MSECTOR_SIZE);
	else
	{
		if (dir_buf != NULL)
			free(dir_buf);

		dir_buf = (uChar *)malloc((uInt) dir_len * MSECTOR_SIZE);
	}

	if (dir_buf == NULL)
		return E_OUT_OF_MEMORY;

	ret = disk_read(dir_start, dir_buf, dir_len * MSECTOR_SIZE);
	dir_entries = dir_len * 16;
	return ret;
}

/*
 * Read the entire FAT table into memory.  Crude error detection on wrong
 * FAT encoding scheme.
 */

static Retcode
fat_read(Int start)
{
	Int i;
	uInt buflen;
	Retcode ret;

	/*
	 * Let's see if the length of the FAT table is appropriate for
	 * the number of clusters and the encoding scheme
	 */
#ifdef CHK_FAT
	Int fat_size;

	fat_size = (fat_bits == 12) ? (num_clus +2) * 3 / 2 : (num_clus +2) * 2;
	fat_size = (fat_size / 512) + ((fat_size % 512) ? 1 : 0);
	if (fat_size != fat_len) {
		cprintf(g_e, "fat_read: Wrong FAT encoding?\n");
		return E_ABORT;
	}
#endif /* CHK_FAT */
					/* only the first copy of the FAT */
	buflen = fat_len * MSECTOR_SIZE;

	if (fat_buf != NULL)
		free(fat_buf);

	fat_buf = (uChar *) malloc(buflen);
	if (fat_buf == NULL) {
		return E_OUT_OF_MEMORY;
	}
					/* read the FAT sectors */
	for (i=start; i<start+fat_len; i++)
	{
		ret = disk_read((Int) i, &fat_buf[(i-start)*MSECTOR_SIZE],
				MSECTOR_SIZE);

		if (ret != NO_ERROR)
			return ret;
	}

					/* the encoding scheme */
	if (fat_bits == 12) {
		end_fat = 0xfff;
		last_fat = 0xff8;
	}
	else {
		end_fat = 0xffff;
		last_fat = 0xfff8;
	}

	return NO_ERROR;
}

/*
 * Get name component of filename.  Translates name to upper case.  Returns
 * pointer to static area.
 */

static char *
get_name(char *filename)
{
	char *s, *temp, buf[MAX_PATH + 1];
	static char ans[13];

	strcpy(buf, filename);
	temp = buf;

	/* find the last separator */
	s = strrchr(temp, '/');
	if (s)
		temp = s + 1;
	s = strrchr(temp, '\\');
	if (s)
		temp = s + 1;
					/* xlate to upper case */
	for (s = temp; *s; ++s) {
		if (islower(*s))
			*s = toupper(*s);
	}

	strcpy(ans, temp);
	return(ans);
}

/*
 * Get the path component of the filename.  Translates to upper case.
 * Returns pointer to a static area.  Doesn't alter leading separator,
 * always strips trailing separator (unless it is the path itself).
 */

static char *
get_path(char *filename)
{
	char *s, *end, *begin, buf[MAX_PATH];
	static char ans[MAX_PATH];
	int has_sep;

	strcpy(buf, filename);
	begin = buf;

	/* if absolute path */
	if (*begin == '/' || *begin == '\\')
		ans[0] = '\0';
	else
		strcpy(ans, "/");
					/* find last separator */
	has_sep = 0;
	end = begin;
	s = strrchr(end, '/');
	if (s) {
		has_sep++;
		end = s;
	}
	s = strrchr(end, '\\');
	if (s) {
		has_sep++;
		end = s;
	}
					/* zap the trailing separator */
	*end = '\0';
					/* translate to upper case */
	for (s = begin; *s; ++s) {
		if (islower(*s))
			*s = toupper(*s);
		if (*s == '\\')
			*s = '/';
	}
					/* if separator alone, put it back */
	if (!strlen(begin) && has_sep)
		strcat(ans, "/");

	strcat(ans, begin);
	return(ans);
}

/*
 * Get rid of spaces in an MSDOS 'raw' name (one that has come from the
 * directory structure) so that it can be used for regular expression
 * matching with a unix filename.  Also used to 'unfix' a name that has
 * been altered by dos_name().  Returns a pointer a static buffer.
 */

static char *
unix_name(uChar *name, uChar *ext)
{
	char *s, tname[9], text[4];
	static char ans[13];

	strncpy(tname, (char *) name, 8);
	tname[8] = '\0';
	s = strchr(tname, ' ');
	if (s)
		*s = '\0';

	strncpy(text, (char *) ext, 3);
	text[3] = '\0';
	s = strchr(text, ' ');
	if (s)
		*s = '\0';

	if (*text) {
		strcpy(ans, tname);
		strcat(ans, ".");
		strcat(ans, text);
	}
	else
		strcpy(ans, tname);
	return(ans);
}

/*
 * Find the directory and load a new dir_chain[].  A null directory
 * is ok.  Returns a 1 on error.
 */

static int
descend(char *path)
{
	int entry;
	unsigned int start;
	char *newname;
	struct directory *dir;

	/* nothing required */
	if (*path == '\0')
		return(0);

	for (entry = 0; entry < dir_entries; entry++) {
		dir = dir_read(entry);
					/* if empty */
		if (dir->name[0] == 0x0)
			break;
					/* if erased */
		if (dir->name[0] == 0xe5)
			continue;
					/* skip if not a directory */
		if (!(dir->attr & 0x10))
			continue;

		newname = unix_name(dir->name, dir->ext);

		/*
		 * Be careful not to match '.' and '..' with wildcards
		 */
		if (*newname == '.' && *path != '.')
			continue;

		if (compare_strs(newname, CSTR, path, CSTR)) {
			start = dir->start[1] * 0x100 + dir->start[0];

					/* if '..' points to root */
			if (!start && !strcmp(path, "..")) {
				reset_chain(OLD);
				return(0);
			}
					/* fill in the directory chain */
			if (fill_chain(start))
				return(1);

			return(0);
		}
	}

	/*
	 * If path is '.' or '..', but they weren't found, then you must be
	 * at root.
	 */
	if (!strcmp(path, ".") || !strcmp(path, "..")) {
		reset_chain(OLD);
		return(0);
	}

	cprintf(g_e, "Path component \"%s\" is not a directory\n", path);
	return(1);
}

/*
 * Descends the directory tree.  Returns 1 on error.
 */

static int
subdir(char *pathname)
{
	char *s, *tmp, tbuf[MAX_PATH], *path;
	int code;

	strcpy(tbuf, pathname);

	/* start at root */
	reset_chain(OLD);

	/* separate the parts */
	tmp = &tbuf[1];
	for (s = tmp; *s; ++s) {
		if (*s == '/' || *s == '\\') {
			path = tmp;
			*s = '\0';
			if (descend(path))
				return(1);
			tmp = s + 1;
		}
	}

	code = descend(tmp);
	return(code);
}

/*
 * Read the boot sector.  We glean the disk parameters from this sector.
 */

static struct bootsector50 *
read_boot(void)
{
	static struct bootsector50 boot;
	Retcode ret;

	ret = disk_read(0, (uByte *)&boot, MSECTOR_SIZE);

	if (ret != NO_ERROR)
	{
		DPRINTF((g_e, "read_boot: cannot read boot sector: %s (%d)\n",
				err2str(ret), ret));
		return NULL;
	}

	if (boot.bootsig[0] != BOOTSIG0 || boot.bootsig[1] != BOOTSIG1)
		return NULL;

	DPRINTF(("dosfat: read_boot: fstype=%S\n", boot.fstype, 6));

	if (memcmp(boot.fstype, "FAT12 ", 6) == 0)
		fat_bits = 12;
	else if (memcmp(boot.fstype, "FAT16 ", 6) == 0)
		fat_bits = 16;
	else if (memcmp(boot.fstype, "FAT32 ", 6) == 0 ||
			memcmp(boot.fstype, "FAT   ", 6) == 0)
		fat_bits = 16;		/* TODO - bogus hack for now */
	else
		return NULL;

	return &boot;
}


/*
 * Initialize an MSDOS diskette.  Read the boot sector, and switch to the
 * proper floppy disk device to match the format on the disk.  Sets a bunch
 * of global variables.
 */

static Retcode
init(void)
{
	Int fat_start, tracks, heads, sectors, old_dos;
	struct bootsector50 *boot;
	Retcode ret;

	/* read the boot sector */
	if ((boot = read_boot()) == NULL)
		return E_NO_FILESYS;

	heads = WORD(nheads);
	sectors = WORD(nsect);
	if (heads && sectors)
		tracks = WORD(psect) / (unsigned) (heads * sectors);

		/* sanity checking */
	old_dos = 0;
	fat_start = 0;			/* silence compiler */
	tracks = 0;				/* silence compiler */
	DPRINTF(("dosfat: init: heads=%d sectors=%d tracks=%d clsiz=%d\n",
			heads, sectors, tracks, boot->clsiz));

	if (!heads || heads > 255 || !sectors || sectors > 500 ||
			tracks > 5000 || !boot->clsiz)
	{
		/*
		 * The above technique will fail on diskettes that
		 * have been formatted with very old MSDOS, so we
		 * resort to the old table-driven method using the
		 * media signature (first byte in FAT).
		 */
		uChar temp[MSECTOR_SIZE];
		ret = disk_read(0, temp, MSECTOR_SIZE);

		if (ret != NO_ERROR)
			return ret;

		switch (temp[0])
		{
		case 0xfe:	/* 160k */
			tracks = 40;
			sectors = 8;
			heads = 1;
			dir_start = 3;
			dir_len = 4;
			clus_size = 1;
			fat_len = 1;
			num_clus = 313;
			break;

		case 0xfc:	/* 180k */
			tracks = 40;
			sectors = 9;
			heads = 1;
			dir_start = 5;
			dir_len = 4;
			clus_size = 1;
			fat_len = 2;
			num_clus = 351;
			break;

		case 0xff:	/* 320k */
			tracks = 40;
			sectors = 8;
			heads = 2;
			dir_start = 3;
			dir_len = 7;
			clus_size = 2;
			fat_len = 1;
			num_clus = 315;
			break;

		case 0xfd:	/* 360k */
			tracks = 40;
			sectors = 9;
			heads = 2;
			dir_start = 5;
			dir_len = 7;
			clus_size = 2;
			fat_len = 2;
			num_clus = 354;
			break;

		default:
			cprintf(g_e, "Probable non-MSDOS disk: sig=%#x\n", temp[0]);
			return E_ABORT;
		}

		fat_start = 1;
		num_fat = 2;
		old_dos = 1;
	}

	/*
	 * all numbers are in sectors, except num_clus (which is in clusters)
	 */
	if (!old_dos) {
		clus_size = boot->clsiz;
		fat_start = WORD(nrsvsect);
		fat_len = WORD(fatlen);
		dir_start = fat_start + (boot->nfat * fat_len);
		dir_len = WORD(dirents) * MDIR_SIZE / (unsigned) MSECTOR_SIZE;

		/*
		 * For DOS partitions > 32M
		 */
		if (WORD(psect) == 0)
			num_clus = (uInt) (DWORD(bigsect) - dir_start - dir_len) /
					clus_size;
		else
			num_clus = (uInt) (WORD(psect) - dir_start - dir_len) / clus_size;

		num_fat = boot->nfat;
	}

#if 0
	/* more sanity checking */
	if (clus_size * MSECTOR_SIZE > MAX_CLUSTER) {
		cprintf(g_e, "Cluster size of %d is larger than max of %d\n",
				clus_size * MSECTOR_SIZE, MAX_CLUSTER);
		return E_ABORT;
	}
#endif

	if (!old_dos && WORD(secsiz) != MSECTOR_SIZE) {
		cprintf(g_e, "Sector size of %d is not supported\n", WORD(secsiz));
		return E_ABORT;
	}
					/* full cylinder buffering */
#ifdef FULL_CYL
	disk_size = tracks ? (sectors * heads) : 1;
#else /* FULL_CYL */
	disk_size = tracks ? sectors : 1;
#endif /* FULL_CYL */

	if (disk_buf != NULL)
		free(disk_buf);

	disk_buf = (uChar *)malloc((uInt) disk_size * MSECTOR_SIZE);
	if (disk_buf == NULL)
		return E_OUT_OF_MEMORY;

	/* read the FAT sectors */
	fat_error = 0;
	fat_read(fat_start);

	/* set dir_chain[] to root directory */
	reset_chain(NEW);
	return NO_ERROR;
}

/*
 * Read the clusters given the beginning FAT entry.  Returns 0 on success.
 */

static Retcode
file_read(uInt fat, Int size, uByte *addr)
{
	Int in_len, out_len;
	Int current, start;
	Retcode ret;
	uChar in_buf[MAX_CLUSTER];

	current = 0L;
	in_len = clus_size * MSECTOR_SIZE;

	/* a zero length file? */
	if (fat == 0)
		return NO_ERROR;

	while (1)
	{
		start = (Int) (fat - 2) * clus_size + dir_start + dir_len;
		ret = disk_read(start, in_buf, in_len);

		if (ret != NO_ERROR)
			return ret;

		out_len = (size - current > in_len) ? in_len : size - current;
		memcpy(addr + current, in_buf, out_len);
		current += out_len;
		if (current >= size)
			break;

		/* get next cluster number */
		fat = fat_decode(fat);
		if (fat == 1) {
			cprintf(g_e, "file_read: FAT problem\n");
			fat_error++;
			return E_ABORT;
		}
					/* end of cluster chain */
		if (fat >= last_fat)
			break;
	}

	return NO_ERROR;
}

static Retcode
load(char *arg, uByte *addr, uLong *retsize)
{
	int ismatch, entry;
	unsigned int fat;
	long size;
	char *filename, *newfile, *pathname;
	struct directory *dir;
	Retcode ret;

	filename = get_name(arg);
	pathname = get_path(arg);
	ismatch = 0;
	ret = init();

	if (ret != NO_ERROR)
		return ret;

	if (subdir(pathname))
		return E_ABORT;

	for (entry = 0; entry < dir_entries; entry++)
	{
		dir = dir_read(entry);

		/* if empty */
		if (dir->name[0] == 0x0)
			break;

		/* if erased */
		if (dir->name[0] == 0xe5)
			continue;

		/* if dir or volume label */
		if ((dir->attr & 0x10) || (dir->attr & 0x08))
			continue;

		newfile = unix_name(dir->name, dir->ext);

		if (compare_strs(newfile, CSTR, filename, CSTR))
		{
			fat = dir->start[1] * 0x100 + dir->start[0];
			size = dir->size[3] * 0x1000000L + dir->size[2] * 0x10000L +
					dir->size[1] * 0x100 + dir->size[0];

			ret = file_read(fat, size, addr);

			if (ret != NO_ERROR)
				return ret;

			*retsize = size;
			ismatch = 1;
			break;
		}
	}

	if (fat_error)
	{
		cprintf(g_e, "fat-load: FAT error\n");
		return E_ABORT;
	}

	if (!ismatch)
	{
		cprintf(g_e, "fat-load: file \"%s\" not found\n", filename);
		return E_ABORT;
	}

	return NO_ERROR;
}

/*
 * Get the amount of free space on the diskette
 */

static Int
getfree(void)
{
	uInt i;
	Int total;
	extern uInt num_clus;

	total = 0L;
	for (i = 2; i < num_clus + 2; i++) {
					/* if fat_decode returns zero */
		if (!fat_decode(i))
			total += clus_size;
	}
	return(total);
}


static Retcode
list(char *dirname)
{
	Int entry, files;
	Int size, blocks;
	char *newfile, volume[12];
	struct directory *dir;
	Retcode ret;

	files = 0;

	if (dirname == NULL || *dirname == 0)
		dirname = "\\";

	ret = init();

	if (ret != NO_ERROR)
		return ret;

	files = 0;

	/* find the volume label */
	volume[0] = '\0';
	for (entry = 0; entry < dir_entries; entry++)
	{
		dir = dir_read(entry);

		/* if empty */
		if (dir->name[0] == 0x0)
			break;

		/* if erased */
		if (dir->name[0] == 0xe5)
			continue;

		/* if not volume label */
		if (!(dir->attr & 0x08))
			continue;

		strncpy(volume, (char *) dir->name, 8);
		volume[8] = '\0';
		strncat(volume, (char *) dir->ext, 3);
		volume[11] = '\0';
		break;
	}

	if (volume[0] == '\0')
		cprintf(g_e, " Volume has no label\n");
	else
		cprintf(g_e, " Volume is %s\n", volume);

	/*
	 * Move to "first guess" subdirectory, so that is_dir() can
	 * search to see if dirname is also a directory.
	 */
	if (subdir(dirname))
		return E_ABORT;

	for (entry = 0; entry < dir_entries; entry++)
	{
		dir = dir_read(entry);
				/* if empty */
		if (dir->name[0] == 0x0)
			break;
				/* if erased */
		if (dir->name[0] == 0xe5)
			continue;
				/* if a volume label */
		if (dir->attr & 0x08)
			continue;

		newfile = unix_name(dir->name, dir->ext);

		files++;
		size = dir->size[3] * 0x1000000L + dir->size[2] * 0x10000L +
				dir->size[1] * 0x100 + dir->size[0];

		if (dir->attr & 0x10)		/* it is a subdirectory? */
			cprintf(g_e, "[%s]\n", newfile);
		else
			cprintf(g_e, "%-15s%d\n", newfile, size);
	}

	blocks = getfree() * MSECTOR_SIZE;

	if (!files)
		cprintf(g_e, "Dir \"%s\" not found\n", dirname);
	else
		cprintf(g_e, "     %3d File(s)     %6d bytes free\n", files, blocks);

	return NO_ERROR;
}

Retcode
dos_fat(Environ *e, Filesys_action what, Instance *disk,
		Byte *path, uLong loc, uLong start, uByte *buf, uInt size,
		uByte *retbuf, uLong *val)
{
	Retcode ret = NO_ERROR;

	DPRINTF(("dos_fat: enter\n"));

	if (size < MSECTOR_SIZE)
		return E_BLOCKSIZE;

	g_buf = buf;
	g_disk = disk;
	disk_offset = loc;
	ret = init();

	if (ret != NO_ERROR)
		return ret;

	switch (what)
	{
	case FS_PROBE:
		strcat((char*)retbuf, ",dos-fat");
		*val = loc;
		break;

	case FS_LIST:
		ret = list(path);
		break;

	case FS_LOAD:
		if (path && *path)		/* load a file */
			ret = load(path, retbuf, val);
		else				/* no file specified - load in 1st (boot) sector */
			ret = filesys_read_bytes(e, disk, loc, *val = MSECTOR_SIZE, retbuf);

		break;
	}

	/* free up memory */
	free(dir_buf);
	free(fat_buf);
	free(disk_buf);
	dir_buf = NULL;
	fat_buf = NULL;
	disk_buf = NULL;

	/* no other filesystems below us */
	DPRINTF(("dos_fat: return R_END\n"));
	return R_END;
}

Filesys g_dos_fat =
{
	"dos-fat",
	dos_fat,
};
