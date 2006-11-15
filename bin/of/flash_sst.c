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

/* (c) Copyright 2003 by CodeGen, Inc.  All Rights Reserved. */

/* SST LPC flash driver */

#include "defs.h"
#include "flash.h"

static const Sector sst_sst49lf020a_blocks[] =
{
	{ 0x000000, 0x04000 }, { 0x004000, 0x04000 },
	{ 0x008000, 0x04000 }, { 0x00C000, 0x04000 },
	{ 0x010000, 0x04000 }, { 0x014000, 0x04000 },
	{ 0x018000, 0x04000 }, { 0x01C000, 0x04000 },
	{ 0x020000, 0x04000 }, { 0x024000, 0x04000 },
	{ 0x028000, 0x04000 }, { 0x02C000, 0x04000 },
	{ 0x030000, 0x04000 }, { 0x034000, 0x04000 },
	{ 0x038000, 0x04000 }, { 0x03C000, 0x04000 },
	{ 0x040000, 0x0 },
};

static const Sector sst_sst49lf030a_blocks[] =
{
	{ 0x000000, 0x10000 }, { 0x010000, 0x10000 },
	{ 0x020000, 0x10000 }, { 0x030000, 0x10000 },
	{ 0x040000, 0x10000 }, { 0x050000, 0x10000 },
	{ 0x060000, 0x0 },
};

static const Sector sst_sst49lf080a_blocks[] =
{
	{ 0x000000, 0x10000 }, { 0x010000, 0x10000 },
	{ 0x020000, 0x10000 }, { 0x030000, 0x10000 },
	{ 0x040000, 0x10000 }, { 0x050000, 0x10000 },
	{ 0x060000, 0x10000 }, { 0x070000, 0x10000 },
	{ 0x080000, 0x10000 }, { 0x090000, 0x10000 },
	{ 0x0A0000, 0x10000 }, { 0x0B0000, 0x10000 },
	{ 0x0C0000, 0x10000 }, { 0x0D0000, 0x10000 },
	{ 0x0E0000, 0x10000 }, { 0x0F0000, 0x10000 },
	{ 0x100000, 0x0 },
};

/* reset sequence */
static const Sequence sst_sst49lf0x0a_reset[] =
{
	{ FLASH_SEQ_WRITE, 0, 0xF0 },				/* Reset flash */
	{ FLASH_SEQ_END },
};

/* probe sequence */
static const Sequence sst_sst49lf0x0a_probe[] =
{
	{ FLASH_SEQ_WRITE, 0, 0xF0 },				/* Reset flash */
	{ FLASH_SEQ_WRITE, 0x5555, 0xAA },			/* unlock 1 */
	{ FLASH_SEQ_WRITE, 0x2AAA, 0x55 },			/* unlock 2 */
	{ FLASH_SEQ_WRITE, 0x5555, 0x90 },			/* ID read mode */
	{ FLASH_SEQ_READ_MANUF, 0 },				/* Read manufactures ID */
	{ FLASH_SEQ_READ_DEVID, 1 },				/* Read device ID */
	{ FLASH_SEQ_WRITE, 0, 0xF0 },				/* Reset flash */
	{ FLASH_SEQ_END },
};

/* read word sequence */
static const Sequence sst_sst49lf0x0a_read[] =
{
	{ FLASH_SEQ_READ_DATA, -1 },					/* Read data word */
	{ FLASH_SEQ_END },
};

/* erase block sequence */
static const Sequence sst_sst49lf0x0a_erase[] =
{
	{ FLASH_SEQ_WRITE, 0x5555, 0xAA },			/* unlock 1 */
	{ FLASH_SEQ_WRITE, 0x2AAA, 0x55 },			/* unlock 2 */
	{ FLASH_SEQ_WRITE, 0x5555, 0x80 },			/* block earse */
	{ FLASH_SEQ_WRITE, 0x5555, 0xAA },			/* unlock 1 */
	{ FLASH_SEQ_WRITE, 0x2AAA, 0x55 },			/* unlock 2 */
	{ FLASH_SEQ_WRITE, -1, 0x50 },				/* block number */
	{ FLASH_SEQ_DELAY, -1, 4, },				/* 2.7us + 1us */
	{ FLASH_SEQ_WAIT_STATUS, -1, 0x80, 0x80 },
	{ FLASH_SEQ_WRITE, 0, 0xF0 },				/* Reset flash */
	{ FLASH_SEQ_END },
};

/* program word sequence */
static const Sequence sst_sst49lf0x0a_write[] =
{
	{ FLASH_SEQ_WRITE, 0x5555, 0xAA },			/* unlock 1 */
	{ FLASH_SEQ_WRITE, 0x2AAA, 0x55 },			/* unlock 2 */
	{ FLASH_SEQ_WRITE, 0x5555, 0xA0 },			/* program byte */
	{ FLASH_SEQ_WRITE_DATA, -1, },
	{ FLASH_SEQ_DELAY, -1, 3, },				/* 1.8us + 1us */
	{ FLASH_SEQ_WAIT_INVERT, -1, -1, 0x80 },
	{ FLASH_SEQ_WRITE, 0, 0xF0 },				/* Reset flash */
	{ FLASH_SEQ_END },
};

const Flash flash_sst_sst49lf020a = 
{
	0xBF,						/* manufactures ID */
	0x52,						/* device ID */
	"SST",						/* manufactures name */
	"SST49LF020A",				/* part number */
	256*1024,					/* total size in bytes */
	1,							/* width in bytes */
	sst_sst49lf020a_blocks,		/* block informaion */
	sst_sst49lf0x0a_reset,		/* reset sequence */
	sst_sst49lf0x0a_probe,		/* probe sequence */
	sst_sst49lf0x0a_read,		/* read word sequence */
	sst_sst49lf0x0a_erase,		/* erase block sequence */
	sst_sst49lf0x0a_write,		/* program word sequence */
};

const Flash flash_sst_sst49lf030a = 
{
	0xBF,						/* manufactures ID */
	0x1C,						/* device ID */
	"SST",						/* manufactures name */
	"SST49LF030A",				/* part number */
	384*1024,					/* total size in bytes */
	1,							/* width in bytes */
	sst_sst49lf030a_blocks,		/* block informaion */
	sst_sst49lf0x0a_reset,		/* reset sequence */
	sst_sst49lf0x0a_probe,		/* probe sequence */
	sst_sst49lf0x0a_read,		/* read word sequence */
	sst_sst49lf0x0a_erase,		/* erase block sequence */
	sst_sst49lf0x0a_write,		/* program word sequence */
};

const Flash flash_sst_sst49lf080a = 
{
	0xBF,						/* manufactures ID */
	0x5B,						/* device ID */
	"SST",						/* manufactures name */
	"SST49LF080A",				/* part number */
	1*1024*1024,				/* total size in bytes */
	1,							/* width in bytes */
	sst_sst49lf080a_blocks,		/* block informaion */
	sst_sst49lf0x0a_reset,		/* reset sequence */
	sst_sst49lf0x0a_probe,		/* probe sequence */
	sst_sst49lf0x0a_read,		/* read word sequence */
	sst_sst49lf0x0a_erase,		/* erase block sequence */
	sst_sst49lf0x0a_write,		/* program word sequence */
};
