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

/* (c) Copyright 2000 by CodeGen, Inc.  All Rights Reserved. */

#ifndef __FLASH_H__
#define __FLASH_H__

struct flash_methods
{
    void *private;	/* private data for flash access functions */
    int width;		/* width of flash array in bytes */
    int (*read)(void *private, void *va, uInt offset, uByte *data);
    int (*write)(void *private, void *va, uInt offset, uByte *data);
    int (*mapaddr)(void *private, uInt *offset);
    int (*mapdata)(void *private, uByte *data);
    uInt bias;		/* seek offset zero relative to flash offset zero */
    uInt size;		/* size of flash space */
    int frag;		/* 2^number of low address bits mapped one to one */
    uInt unit[MAX_ADDR_CELLS];
    int unitcells;	/* number of cells used in array above */
};

typedef struct flash_methods Flash_methods;

struct sector
{
	int start;
	int size;
};

typedef struct sector Sector;

struct sequence
{
	int type;
	Int offset;
	uInt data;
	uInt mask;
};

typedef struct sequence Sequence;

#define FLASH_SEQ_END		0
#define FLASH_SEQ_WRITE		1
#define FLASH_SEQ_WRITE_DATA	2
#define FLASH_SEQ_READ_MANUF	3
#define FLASH_SEQ_READ_DEVID	4
#define FLASH_SEQ_READ_DATA	5
#define FLASH_SEQ_WAIT_STATUS	6
#define FLASH_SEQ_WAIT_TOGGLE	7
#define FLASH_SEQ_WAIT_INVERT	8
#define FLASH_SEQ_DELAY		9

struct flash
{
	Int manufid;
	Int devid;
	char *manufname;
	char *devname;
	Int size;
	Int width;			/* width in bytes */
	const Sector *sectors;
	const Sequence *reset;
	const Sequence *probe;
	const Sequence *read;
	const Sequence *erase;
	const Sequence *write;
};

typedef struct flash Flash;

extern Retcode generic_install_flash(Environ *e, Flash_methods *methods);

#endif /* __FLASH_H__ */
