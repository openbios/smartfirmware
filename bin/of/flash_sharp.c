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

/* Sharp Flash driver */

#include "defs.h"
#include "flash.h"

static const Sector sharp_lh28f320sktd_sectors[] =
{
	{ 0x000000, 0x10000 }, { 0x010000, 0x10000 },
	{ 0x020000, 0x10000 }, { 0x030000, 0x10000 },
	{ 0x040000, 0x10000 }, { 0x050000, 0x10000 },
	{ 0x060000, 0x10000 }, { 0x070000, 0x10000 },
	{ 0x080000, 0x10000 }, { 0x090000, 0x10000 },
	{ 0x0A0000, 0x10000 }, { 0x0B0000, 0x10000 },
	{ 0x0C0000, 0x10000 }, { 0x0D0000, 0x10000 },
	{ 0x0E0000, 0x10000 }, { 0x0F0000, 0x10000 },
	{ 0x100000, 0x10000 }, { 0x110000, 0x10000 },
	{ 0x120000, 0x10000 }, { 0x130000, 0x10000 },
	{ 0x140000, 0x10000 }, { 0x150000, 0x10000 },
	{ 0x160000, 0x10000 }, { 0x170000, 0x10000 },
	{ 0x180000, 0x10000 }, { 0x190000, 0x10000 },
	{ 0x1A0000, 0x10000 }, { 0x1B0000, 0x10000 },
	{ 0x1C0000, 0x10000 }, { 0x1D0000, 0x10000 },
	{ 0x1E0000, 0x10000 }, { 0x1F0000, 0x10000 },
	{ 0x200000, 0x10000 }, { 0x210000, 0x10000 },
	{ 0x220000, 0x10000 }, { 0x230000, 0x10000 },
	{ 0x240000, 0x10000 }, { 0x250000, 0x10000 },
	{ 0x260000, 0x10000 }, { 0x270000, 0x10000 },
	{ 0x280000, 0x10000 }, { 0x290000, 0x10000 },
	{ 0x2A0000, 0x10000 }, { 0x2B0000, 0x10000 },
	{ 0x2C0000, 0x10000 }, { 0x2D0000, 0x10000 },
	{ 0x2E0000, 0x10000 }, { 0x2F0000, 0x10000 },
	{ 0x300000, 0x10000 }, { 0x310000, 0x10000 },
	{ 0x320000, 0x10000 }, { 0x330000, 0x10000 },
	{ 0x340000, 0x10000 }, { 0x350000, 0x10000 },
	{ 0x360000, 0x10000 }, { 0x370000, 0x10000 },
	{ 0x380000, 0x10000 }, { 0x390000, 0x10000 },
	{ 0x3A0000, 0x10000 }, { 0x3B0000, 0x10000 },
	{ 0x3C0000, 0x10000 }, { 0x3D0000, 0x10000 },
	{ 0x3E0000, 0x10000 }, { 0x3F0000, 0x10000 },
	{ 0x400000, 0 },
};

/* reset sequence */
static const Sequence sharp_lh28f320sktd_reset[] =
{
	{ FLASH_SEQ_WRITE, 0, 0xFFFF },				/* Reset flash */
	{ FLASH_SEQ_END },
};

/* probe sequence */
static const Sequence sharp_lh28f320sktd_probe[] =
{
	{ FLASH_SEQ_WRITE, 0, 0xFFFF },				/* Reset flash */
	{ FLASH_SEQ_WRITE, 0, 0x0090 },				/* ID read mode */
	{ FLASH_SEQ_READ_MANUF, 0 },				/* Read manufactures ID */
	{ FLASH_SEQ_READ_DEVID, 2 },				/* Read device ID */
	{ FLASH_SEQ_WRITE, 0, 0xFFFF },				/* Reset flash */
	{ FLASH_SEQ_END },
};

/* read word sequence */
static const Sequence sharp_lh28f320sktd_read[] =
{
	{ FLASH_SEQ_READ_DATA, -1 },					/* Read data word */
	{ FLASH_SEQ_END },
};

/* erase sector sequence */
static const Sequence sharp_lh28f320sktd_erase[] =
{
	{ FLASH_SEQ_WRITE, -1, 0x0020 },
	{ FLASH_SEQ_WRITE, -1, 0x00D0 },
	{ FLASH_SEQ_WAIT_STATUS, -1, 0x0080, 0x0080 },
	{ FLASH_SEQ_WRITE, 0, 0xFFFF },				/* Reset flash */
	{ FLASH_SEQ_END },
};

/* program word sequence */
static const Sequence sharp_lh28f320sktd_write[] =
{
	{ FLASH_SEQ_WRITE, -1, 0x0040 },
	{ FLASH_SEQ_WRITE_DATA, -1, },
	{ FLASH_SEQ_WAIT_STATUS, -1, 0x0080, 0x0080 },
	{ FLASH_SEQ_WRITE, 0, 0xFFFF },				/* Reset flash */
	{ FLASH_SEQ_END },
};

const Flash flash_sharp_lh28f320sktd = 
{
	0x00B0,						/* manufactures ID */
	0x00D0,						/* device ID */
	"Sharp",					/* manufactures name */
	"LH28F320SKTD",				/* part number */
	4*1024*1024,				/* total size in bytes */
	2,							/* width in bytes */
	sharp_lh28f320sktd_sectors,	/* sector informaion */
	sharp_lh28f320sktd_reset,	/* reset sequence */
	sharp_lh28f320sktd_probe,	/* probe sequence */
	sharp_lh28f320sktd_read,	/* read word sequence */
	sharp_lh28f320sktd_erase,	/* erase sector sequence */
	sharp_lh28f320sktd_write,	/* program word sequence */
};
