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

/* (c) Copyright 1997 by CodeGen, Inc.  All Rights Reserved. */

/* The following code contains portions copied from several FreeBSD files:
		/sys/i386/boot/dosboot/disklabe.h
		/sys/msdos/bootsect.h
		/sys/msdos/bpb.h
		/sys/msdos/direntry.h
		/sys/msdosfs/fat.h
	The original copyrights preceed the copied code below.
	Types have been changed to fit into SmartFirmware, and field names
	have been changed to be lower-case without useless prefixes.  Much
	stuff unnecessary for SmartFirmware has been deleted.
 */

#ifndef _DOS_H_
#define _DOS_H_

/* /sys/i386/boot/dosboot/disklabe.h */
/*
 * Copyright (c) 1987, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)disklabel.h	8.1 (Berkeley) 6/2/93
 * $Id: dos.h,v 1.7 2000/02/04 01:40:22 parag Exp $
 */



/* DOS partition table -- located in boot block */

#define DOSBBSECTOR			0		/* DOS boot block relative sector number */
#define DOSPARTOFF			446
#define NDOSPART			4
#define	DOSPTYP_386BSD		0xa5	/* 386BSD partition type */
#define	MBR_PTYPE_FreeBSD	0xa5	/* FreeBSD partition type */

/* types of various DOS partitions - culled from FreeBSD's fdisk */
#define	DP_UNUSED		0x00
#define	DP_FAT12		0x01
#define	DP_XENIX_ROOT	0x02
#define	DP_XENIX_USR	0x03
#define DP_FAT16		0x04
#define DP_EXTENDED		0x05	/* extended DOS partition */
#define DP_FAT32		0x06	/* DOS >32M */
#define DP_HPFS			0x07	/* OS/2 HPFS or QNX or Advanced UNIX */
#define DP_AIX			0x08
#define DP_AIX_BOOT		0x09	/* or Coherent */
#define DP_OS2_BOOT		0x0A	/* OS/2 bootmanager or OPUS */
#define DP_OPUS			0x10
#define DP_VENIX		0x40
#define DP_DM_1			0x50
#define DP_DM_2			0x51
#define DP_CPM			0x52	/* CP/M or Microport SysV/AT */
#define DP_GB			0x56
#define DP_SPEED_1		0x61
#define DP_ISC			0x63	/* ISC UNIX, other SysV/386, GNU HURD, Mach */
#define DP_NETWARE_2	0x64	/* Novell netware 2.* */
#define DP_NETWARE_3	0x65	/* Novell netware 3.* */
#define DP_PCIX			0x75
#define DP_MINIX_1		0x80
#define DP_MINIX_15		0x81
#define DP_LINUX_SWAP	0x82
#define DP_LINUX		0x83
#define DP_AMOEBA		0x93
#define DP_AMOEBA_BBT	0x94	/* bad blocks table */
#define DP_BSD			0xA5	/* FreeBSD, NetBSD, 386BSD */
#define DP_OPENBSD		0xA6	/* OpenBSD */
#define DP_NEXTSTEP		0xA7
#define DP_OPENBSD2		0xB6	/* OpenBSD II? */
#define DP_BSDI			0xB7
#define DP_BSDI_SWAP	0xB8
#define DP_CONCURRENT	0xDB	/* Concurrent CP/M, C.DOS, CTOS */
#define DP_SPEED_2		0xE1
#define DP_SPEED_3		0xE3
#define DP_SPEED_4		0xE4
#define DP_SPEED_5		0xF1
#define DP_SECONDARY	0xF2	/* DOS 3.3+ secondary */
#define DP_SPEED_6		0xF4
#define DP_BBT			0xFF	/* bad blocks table */


struct dos_partition {
	uChar	flag;		/* bootstrap flags */
	uChar	shd;		/* starting head */
	uChar	ssect;		/* starting sector */
	uChar	scyl;		/* starting cylinder */
	uChar	typ;		/* partition type */
	uChar	ehd;		/* end head */
	uChar	esect;		/* end sector */
	uChar	ecyl;		/* end cylinder */
	uChar	start[4];	/* absolute starting sector number */
	uChar	size[4];	/* partition size in sectors */
};



/* /sys/msdos/bpb.h */
/* /sys/msdos/bootsect.h */
/*
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 *
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 *
 * This software is provided "as is".
 *
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 *
 * October 1992
 */

/*
 * Format of a boot sector.  This is the first sector on a DOS floppy disk
 * or the first sector of a partition on a hard disk.  But, it is not the
 * first sector of a partitioned hard disk.
 */
struct bootsector33 {
	uChar jump[3];				/* jump instruction E9xxxx or EBxx90 */
	Char oem_name[8];			/* OEM name and version */
	
	/* BIOS parameter block */
	uChar bytes_per_sec[2];		/* bytes per sector */
	uChar sec_per_clust;		/* sectors per cluster */
	uChar res_sectors[2];		/* number of reserved sectors */
	uChar fats;					/* number of FATs */
	uChar root_dir_ents[2];		/* number of root directory entries */
	uChar sectors[2];			/* total number of sectors */
	uChar media;				/* media descriptor */
	uChar fat_secs[2];			/* number of sectors per FAT */
	uChar sec_per_track[2];		/* sectors per track */
	uChar heads[2];				/* number of heads */
	uChar hidden_secs[2];		/* number of hidden sectors */

	uChar drive_number;			/* drive number (0x80) */
	uChar boot_code[474];		/* pad so structure is 512 bytes long */
	uChar boot_sect_sig[2];		/* boot signature (0x55AA) */
};

struct bootsector50 {
	uChar jump[3];				/* jump instruction E9xxxx or EBxx90 */
	Char oem_name[8];			/* OEM name and version */

	/* BIOS parameter block */
	uChar bytes_per_sec[2];		/* bytes per sector */
	uChar sec_per_clust;		/* sectors per cluster */
	uChar res_sectors[2];		/* number of reserved sectors */
	uChar fats;					/* number of FATs */
	uChar root_dir_ents[2];		/* number of root directory entries */
	uChar sectors[2];			/* total number of sectors */
	uChar media;				/* media descriptor */
	uChar fat_secs[2];			/* number of sectors per FAT */
	uChar sec_per_track[2];		/* sectors per track */
	uChar heads[2];				/* number of heads */
	uChar hidden_secs[4];		/* number of hidden sectors */
	uChar huge_sectors[4];		/* number of sectrs if bpbSectors == 0 */

	uChar drive_number;			/* drive number (0x80) */
	uChar reserved1;				/* reserved */
	uChar boot_signature;		/* extended boot signature (0x29) */
	Char volume_id[4];			/* volume ID number */
	Char volume_label[11];		/* volume label */
	Char file_sys_type[8];		/* file system type (FAT12 or FAT16) */
	uChar boot_code[448];		/* pad so structure is 512 bytes long */
	uChar boot_sect_sig[2];		/* boot signature (0x55AA) */
};

#define	EXBOOTSIG	0x29u
#define	BOOTSIG0	0x55u
#define	BOOTSIG1	0xaau

union bootsector {
	struct bootsector33 bs33;
	struct bootsector50 bs50;
};



/* /sys/msdos/direntry.h */
/* /sys/msdosfs/fat.h */
/*-
 * Copyright (C) 1994 Wolfgang Solfrank.
 * Copyright (C) 1994 TooLs GmbH.
 * All rights reserved.
 * Original code by Paul Popelka (paulp@uts.amdahl.com) (see below).
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 *
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 *
 * This software is provided "as is".
 *
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 *
 * October 1992
 */

/*
 * Structure of a dos directory entry.
 */
struct direntry {
	uChar name[8];				/* filename, blank filled */
#define	SLOT_EMPTY		0x00		/* slot has never been used */
#define	SLOT_E5			0x05		/* the real value is 0xe5 */
#define	SLOT_DELETED	0xe5		/* file in this slot deleted */
	uChar extension[3];			/* extension, blank filled */
	uChar attributes;			/* file attributes */
#define	ATTR_NORMAL		0x00		/* normal file */
#define	ATTR_READONLY	0x01		/* file is readonly */
#define	ATTR_HIDDEN		0x02		/* file is hidden */
#define	ATTR_SYSTEM		0x04		/* file is a system file */
#define	ATTR_VOLUME		0x08		/* entry is a volume label */
#define	ATTR_DIRECTORY	0x10		/* entry is a directory name */
#define	ATTR_ARCHIVE	0x20		/* file is new or modified */
	uChar reserved[10];			/* reserved */
	uChar time[2];				/* create/last update time */
	uChar date[2];				/* create/last update date */
	uChar start_cluster[2];		/* starting cluster of file */
	uChar file_size[4];			/* size of file in bytes */
};

/*
 * This is the format of the contents of the deTime field in the direntry
 * structure.
 * We don't use bitfields because we don't know how compilers for
 * arbitrary machines will lay them out.
 */
#define DT_2SECONDS_MASK	0x1F	/* seconds divided by 2 */
#define DT_2SECONDS_SHIFT	0
#define DT_MINUTES_MASK		0x7E0	/* minutes */
#define DT_MINUTES_SHIFT	5
#define DT_HOURS_MASK		0xF800	/* hours */
#define DT_HOURS_SHIFT		11

/*
 * This is the format of the contents of the deDate field in the direntry
 * structure.
 */
#define DD_DAY_MASK		0x1F	/* day of month */
#define DD_DAY_SHIFT		0
#define DD_MONTH_MASK		0x1E0	/* month */
#define DD_MONTH_SHIFT		5
#define DD_YEAR_MASK		0xFE00	/* year - 1980 */
#define DD_YEAR_SHIFT		9



/*
 * Some useful cluster numbers.
 */
#define	MSDOSFSROOT	0		/* cluster 0 means the root dir */
#define	CLUST_FREE	0		/* cluster 0 also means a free cluster */
#define	MSDOSFSFREE	CLUST_FREE
#define	CLUST_FIRST	2		/* first legal cluster number */
#define	CLUST_RSRVS	0xfff0	/* start of reserved cluster range */
#define	CLUST_RSRVE	0xfff6	/* end of reserved cluster range */
#define	CLUST_BAD	0xfff7	/* a cluster with a defect */
#define	CLUST_EOFS	0xfff8	/* start of eof cluster range */
#define	CLUST_EOFE	0xffff	/* end of eof cluster range */

#define	FAT12_MASK	0x0fff	/* mask for 12 bit cluster numbers */
#define	FAT16_MASK	0xffff	/* mask for 16 bit cluster numbers */

/*
 * Return true if filesystem uses 12 bit fats. Microsoft Programmer's
 * Reference says if the maximum cluster number in a filesystem is greater
 * than 4086 then we've got a 16 bit fat filesystem.
 */
#define	FAT12(cluster)	((cluster) <= 4086)
#define	FAT16(cluster)	((cluster) >  4086)

#define	MSDOSFSEOF(cn)	(((cn) & 0xfff8) == 0xfff8)


/* END of copied code */


#endif /* _DOS_H_ */
