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

/* (c) Copyright 2001 by CodeGen, Inc.  All Rights Reserved. */

/* Intel Flash driver */

#include "defs.h"
#include "flash.h"

static const Sector intel_e28f32j3_sectors[] =
{
	{ 0x000000, 0x20000 }, { 0x020000, 0x20000 },
	{ 0x040000, 0x20000 }, { 0x060000, 0x20000 },
	{ 0x080000, 0x20000 }, { 0x0A0000, 0x20000 },
	{ 0x0C0000, 0x20000 }, { 0x0E0000, 0x20000 },
	{ 0x100000, 0x20000 }, { 0x120000, 0x20000 },
	{ 0x140000, 0x20000 }, { 0x160000, 0x20000 },
	{ 0x180000, 0x20000 }, { 0x1A0000, 0x20000 },
	{ 0x1C0000, 0x20000 }, { 0x1E0000, 0x20000 },
	{ 0x200000, 0x20000 }, { 0x220000, 0x20000 },
	{ 0x240000, 0x20000 }, { 0x260000, 0x20000 },
	{ 0x280000, 0x20000 }, { 0x2A0000, 0x20000 },
	{ 0x2C0000, 0x20000 }, { 0x2E0000, 0x20000 },
	{ 0x300000, 0x20000 }, { 0x320000, 0x20000 },
	{ 0x340000, 0x20000 }, { 0x360000, 0x20000 },
	{ 0x380000, 0x20000 }, { 0x3A0000, 0x20000 },
	{ 0x3C0000, 0x20000 }, { 0x3E0000, 0x20000 },
	{ 0x400000, 0x0 },
};

static const Sector intel_e28f64j3_sectors[] =
{
	{ 0x000000, 0x20000 }, { 0x020000, 0x20000 },
	{ 0x040000, 0x20000 }, { 0x060000, 0x20000 },
	{ 0x080000, 0x20000 }, { 0x0A0000, 0x20000 },
	{ 0x0C0000, 0x20000 }, { 0x0E0000, 0x20000 },
	{ 0x100000, 0x20000 }, { 0x120000, 0x20000 },
	{ 0x140000, 0x20000 }, { 0x160000, 0x20000 },
	{ 0x180000, 0x20000 }, { 0x1A0000, 0x20000 },
	{ 0x1C0000, 0x20000 }, { 0x1E0000, 0x20000 },
	{ 0x200000, 0x20000 }, { 0x220000, 0x20000 },
	{ 0x240000, 0x20000 }, { 0x260000, 0x20000 },
	{ 0x280000, 0x20000 }, { 0x2A0000, 0x20000 },
	{ 0x2C0000, 0x20000 }, { 0x2E0000, 0x20000 },
	{ 0x300000, 0x20000 }, { 0x320000, 0x20000 },
	{ 0x340000, 0x20000 }, { 0x360000, 0x20000 },
	{ 0x380000, 0x20000 }, { 0x3A0000, 0x20000 },
	{ 0x3C0000, 0x20000 }, { 0x3E0000, 0x20000 },
	{ 0x400000, 0x20000 }, { 0x420000, 0x20000 },
	{ 0x440000, 0x20000 }, { 0x460000, 0x20000 },
	{ 0x480000, 0x20000 }, { 0x4A0000, 0x20000 },
	{ 0x4C0000, 0x20000 }, { 0x4E0000, 0x20000 },
	{ 0x500000, 0x20000 }, { 0x520000, 0x20000 },
	{ 0x540000, 0x20000 }, { 0x560000, 0x20000 },
	{ 0x580000, 0x20000 }, { 0x5A0000, 0x20000 },
	{ 0x5C0000, 0x20000 }, { 0x5E0000, 0x20000 },
	{ 0x600000, 0x20000 }, { 0x620000, 0x20000 },
	{ 0x640000, 0x20000 }, { 0x660000, 0x20000 },
	{ 0x680000, 0x20000 }, { 0x6A0000, 0x20000 },
	{ 0x6C0000, 0x20000 }, { 0x6E0000, 0x20000 },
	{ 0x700000, 0x20000 }, { 0x720000, 0x20000 },
	{ 0x740000, 0x20000 }, { 0x760000, 0x20000 },
	{ 0x780000, 0x20000 }, { 0x7A0000, 0x20000 },
	{ 0x7C0000, 0x20000 }, { 0x7E0000, 0x20000 },
	{ 0x800000, 0x0 },
};

static const Sector intel_e28f128j3_sectors[] =
{
	{ 0x000000, 0x20000 }, { 0x020000, 0x20000 },
	{ 0x040000, 0x20000 }, { 0x060000, 0x20000 },
	{ 0x080000, 0x20000 }, { 0x0A0000, 0x20000 },
	{ 0x0C0000, 0x20000 }, { 0x0E0000, 0x20000 },
	{ 0x100000, 0x20000 }, { 0x120000, 0x20000 },
	{ 0x140000, 0x20000 }, { 0x160000, 0x20000 },
	{ 0x180000, 0x20000 }, { 0x1A0000, 0x20000 },
	{ 0x1C0000, 0x20000 }, { 0x1E0000, 0x20000 },
	{ 0x200000, 0x20000 }, { 0x220000, 0x20000 },
	{ 0x240000, 0x20000 }, { 0x260000, 0x20000 },
	{ 0x280000, 0x20000 }, { 0x2A0000, 0x20000 },
	{ 0x2C0000, 0x20000 }, { 0x2E0000, 0x20000 },
	{ 0x300000, 0x20000 }, { 0x320000, 0x20000 },
	{ 0x340000, 0x20000 }, { 0x360000, 0x20000 },
	{ 0x380000, 0x20000 }, { 0x3A0000, 0x20000 },
	{ 0x3C0000, 0x20000 }, { 0x3E0000, 0x20000 },
	{ 0x400000, 0x20000 }, { 0x420000, 0x20000 },
	{ 0x440000, 0x20000 }, { 0x460000, 0x20000 },
	{ 0x480000, 0x20000 }, { 0x4A0000, 0x20000 },
	{ 0x4C0000, 0x20000 }, { 0x4E0000, 0x20000 },
	{ 0x500000, 0x20000 }, { 0x520000, 0x20000 },
	{ 0x540000, 0x20000 }, { 0x560000, 0x20000 },
	{ 0x580000, 0x20000 }, { 0x5A0000, 0x20000 },
	{ 0x5C0000, 0x20000 }, { 0x5E0000, 0x20000 },
	{ 0x600000, 0x20000 }, { 0x620000, 0x20000 },
	{ 0x640000, 0x20000 }, { 0x660000, 0x20000 },
	{ 0x680000, 0x20000 }, { 0x6A0000, 0x20000 },
	{ 0x6C0000, 0x20000 }, { 0x6E0000, 0x20000 },
	{ 0x700000, 0x20000 }, { 0x720000, 0x20000 },
	{ 0x740000, 0x20000 }, { 0x760000, 0x20000 },
	{ 0x780000, 0x20000 }, { 0x7A0000, 0x20000 },
	{ 0x7C0000, 0x20000 }, { 0x7E0000, 0x20000 },
	{ 0x800000, 0x20000 }, { 0x820000, 0x20000 },
	{ 0x840000, 0x20000 }, { 0x860000, 0x20000 },
	{ 0x880000, 0x20000 }, { 0x8A0000, 0x20000 },
	{ 0x8C0000, 0x20000 }, { 0x8E0000, 0x20000 },
	{ 0x900000, 0x20000 }, { 0x920000, 0x20000 },
	{ 0x940000, 0x20000 }, { 0x960000, 0x20000 },
	{ 0x980000, 0x20000 }, { 0x9A0000, 0x20000 },
	{ 0x9C0000, 0x20000 }, { 0x9E0000, 0x20000 },
	{ 0xA00000, 0x20000 }, { 0xA20000, 0x20000 },
	{ 0xA40000, 0x20000 }, { 0xA60000, 0x20000 },
	{ 0xA80000, 0x20000 }, { 0xAA0000, 0x20000 },
	{ 0xAC0000, 0x20000 }, { 0xAE0000, 0x20000 },
	{ 0xB00000, 0x20000 }, { 0xB20000, 0x20000 },
	{ 0xB40000, 0x20000 }, { 0xB60000, 0x20000 },
	{ 0xB80000, 0x20000 }, { 0xBA0000, 0x20000 },
	{ 0xBC0000, 0x20000 }, { 0xBE0000, 0x20000 },
	{ 0xC00000, 0x20000 }, { 0xC20000, 0x20000 },
	{ 0xC40000, 0x20000 }, { 0xC60000, 0x20000 },
	{ 0xC80000, 0x20000 }, { 0xCA0000, 0x20000 },
	{ 0xCC0000, 0x20000 }, { 0xCE0000, 0x20000 },
	{ 0xD00000, 0x20000 }, { 0xD20000, 0x20000 },
	{ 0xD40000, 0x20000 }, { 0xD60000, 0x20000 },
	{ 0xD80000, 0x20000 }, { 0xDA0000, 0x20000 },
	{ 0xDC0000, 0x20000 }, { 0xDE0000, 0x20000 },
	{ 0xE00000, 0x20000 }, { 0xE20000, 0x20000 },
	{ 0xE40000, 0x20000 }, { 0xE60000, 0x20000 },
	{ 0xE80000, 0x20000 }, { 0xEA0000, 0x20000 },
	{ 0xEC0000, 0x20000 }, { 0xEE0000, 0x20000 },
	{ 0xF00000, 0x20000 }, { 0xF20000, 0x20000 },
	{ 0xF40000, 0x20000 }, { 0xF60000, 0x20000 },
	{ 0xF80000, 0x20000 }, { 0xFA0000, 0x20000 },
	{ 0xFC0000, 0x20000 }, { 0xFE0000, 0x20000 },
	{ 0x1000000, 0x0 },
};
/*	awk 'BEGIN { for (i = 0; i <= 16*1024*1024; i += 256*1024) printf("\t{ 0x%06X, 0x20000 }, { 0x%06X, 0x20000 },\n", i, i + 128*1024); exit; }' */

/* reset sequence */
static const Sequence intel_e28f128j3_reset[] =
{
	{ FLASH_SEQ_WRITE, 0, 0xFFFF },				/* Reset flash */
	{ FLASH_SEQ_END },
};

/* probe sequence */
static const Sequence intel_e28f128j3_probe[] =
{
	{ FLASH_SEQ_WRITE, 0, 0xFFFF },				/* Reset flash */
	{ FLASH_SEQ_WRITE, 0, 0x0090 },				/* ID read mode */
	{ FLASH_SEQ_READ_MANUF, 0 },				/* Read manufactures ID */
	{ FLASH_SEQ_READ_DEVID, 2 },				/* Read device ID */
	{ FLASH_SEQ_WRITE, 0, 0xFFFF },				/* Reset flash */
	{ FLASH_SEQ_END },
};

/* read word sequence */
static const Sequence intel_e28f128j3_read[] =
{
	{ FLASH_SEQ_READ_DATA, -1 },					/* Read data word */
	{ FLASH_SEQ_END },
};

/* erase sector sequence */
static const Sequence intel_e28f128j3_erase[] =
{
	{ FLASH_SEQ_WRITE, -1, 0x0020 },
	{ FLASH_SEQ_WRITE, -1, 0x00D0 },
	{ FLASH_SEQ_WAIT_STATUS, -1, 0x0080, 0x0080 },
	{ FLASH_SEQ_WRITE, -1, 0xFFFF },			/* Reset flash */
	{ FLASH_SEQ_END },
};

/* program word sequence */
static const Sequence intel_e28f128j3_write[] =
{
	{ FLASH_SEQ_WRITE, -1, 0x0040 },
	{ FLASH_SEQ_WRITE_DATA, -1, },
	{ FLASH_SEQ_WAIT_STATUS, -1, 0x0080, 0x0080 },
	{ FLASH_SEQ_WRITE, -1, 0xFFFF },			/* Reset flash */
	{ FLASH_SEQ_END },
};

const Flash flash_intel_e28f32j3 = 
{
	0x0089,						/* manufactures ID */
	0x0016,						/* device ID */
	"Intel",					/* manufactures name */
	"E28F32J3",					/* part number */
	4*1024*1024,				/* total size in bytes */
	2,							/* width in bytes */
	intel_e28f32j3_sectors,		/* sector informaion */
	intel_e28f128j3_reset,		/* reset sequence */
	intel_e28f128j3_probe,		/* probe sequence */
	intel_e28f128j3_read,		/* read word sequence */
	intel_e28f128j3_erase,		/* erase sector sequence */
	intel_e28f128j3_write,		/* program word sequence */
};

const Flash flash_intel_e28f64j3 = 
{
	0x0089,						/* manufactures ID */
	0x0017,						/* device ID */
	"Intel",					/* manufactures name */
	"E28F64J3",					/* part number */
	8*1024*1024,				/* total size in bytes */
	2,							/* width in bytes */
	intel_e28f64j3_sectors,		/* sector informaion */
	intel_e28f128j3_reset,		/* reset sequence */
	intel_e28f128j3_probe,		/* probe sequence */
	intel_e28f128j3_read,		/* read word sequence */
	intel_e28f128j3_erase,		/* erase sector sequence */
	intel_e28f128j3_write,		/* program word sequence */
};

const Flash flash_intel_e28f128j3 = 
{
	0x0089,						/* manufactures ID */
	0x0018,						/* device ID */
	"Intel",					/* manufactures name */
	"E28F128J3",				/* part number */
	16*1024*1024,				/* total size in bytes */
	2,							/* width in bytes */
	intel_e28f128j3_sectors,	/* sector informaion */
	intel_e28f128j3_reset,		/* reset sequence */
	intel_e28f128j3_probe,		/* probe sequence */
	intel_e28f128j3_read,		/* read word sequence */
	intel_e28f128j3_erase,		/* erase sector sequence */
	intel_e28f128j3_write,		/* program word sequence */
};
