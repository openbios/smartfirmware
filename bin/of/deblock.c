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

/* (c) Copyright 1996-1998 by CodeGen, Inc.  All Rights Reserved. */

/* "deblocker" package
	Uses following methods of its caller:
		block-size max-transfer read-blocks write-blocks
	Uses following methods of its caller's parent:
		dma-alloc dma-free
 */

#include "defs.h"

struct self
{
	uInt blocksize;		/* size of block supported by device */
	uInt xferblocks;	/* max number of blocks to transfer at one time */
	uInt blocknum;		/* next block number */
	uInt offset;		/* offset within block */
	uInt currblock;		/* current block in buffer */
	Byte *block;		/* block buffer */
	Bool modified;		/* needs to be written before doing anything */
};
typedef struct self Self;


C(f_deblock_open)		/* open (-- okay?) */
{
	Retcode ret;
	Self *s;
	Instance *inst = (Instance*)(uPtr)e->currinst;
	
	DPRINTF(("deblocker open\n"));
	IFCKSP(e, 0, 1);
	
	if (inst == (uPtr)NULL || inst->self != NULL)
		return E_BAD_INSTANCE;
		
	s = (Self*)malloc(sizeof (Self));
	
	if (s == NULL)
		return E_OUT_OF_MEMORY;

	inst->self = s;
	memset(s, 0, sizeof (Self));
	ret = execute_method_name(e, inst->parent, "block-size", CSTR);
	
	if (ret != NO_ERROR)
	{
		free(s);
		return ret;
	}

	POP(e, s->blocksize);
	ret = execute_method_name(e, inst->parent, "max-transfer", CSTR);
	
	if (ret != NO_ERROR)
	{
		free(s);
		return ret;
	}

	POP(e, s->xferblocks);			/* max transfer size in bytes */
	DPRINTF(("deblocker open: block-size=%#x max-transfer=%#x\n",
			s->blocksize, s->xferblocks));
	
	if (s->blocksize < 1 || s->xferblocks < 1 || s->xferblocks % s->blocksize)
	{
		free(s);
		return E_BLOCKSIZE;
	}
	
	/* allocate space for the buffer in a possibly bus-specific way,
		as per OF errata */
	PUSH(e, s->xferblocks);
	ret = execute_method_name(e, inst->parent->parent,
			"dma-alloc", CSTR);
	
	if (ret != NO_ERROR)
	{
		free(s);
		return ret;
	}
	
	POPT(e, s->block, Byte*);
	
	if (s->block == NULL)
	{
		free(s);
		return E_OUT_OF_MEMORY;
	}
	
	s->xferblocks /= s->blocksize;		/* convert bytes to number of blocks */
	s->blocknum = 0;
	s->offset = 0;
	s->currblock = -1;
	s->modified = FALSE;

	PUSH(e, FTRUE);
	DPRINTF(("deblocker open: return %d\n", ret));
	return ret;
}

static Retcode
flushbuf(Environ *e, Self *s)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	uInt actual;
	Retcode ret;
	
	DPRINTF(("deblocker flushbuf\n"));
	if (!s->modified)
		return NO_ERROR;
		
	PUSHP(e, s->block);
	PUSH(e, s->currblock);
	PUSH(e, s->xferblocks);
	ret = execute_method_name(e, inst->parent, "write-blocks", CSTR);
	DPRINTF((
		"deblocker flushbuf: write-blocks block=%p currblock=%#x ret=%d\n",
			s->block, s->currblock, ret));
	
	if (ret != NO_ERROR)
		return ret;
	
	POP(e, actual);
	
	if (actual != s->xferblocks)
		return E_WRITE_ERROR;
	
	s->modified = FALSE;		
	DPRINTF(("deblocker flushbuf: return %d\n", ret));
	return ret;
}

C(f_deblock_close)		/* close (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Retcode ret;
	
	DPRINTF(("deblocker close\n"));

	if (inst && inst->self)
	{
		Self *s = inst->self;
		flushbuf(e, s);
		PUSHP(e, s->block);
		PUSH(e, s->xferblocks * s->blocksize);
		ret = execute_method_name(e, inst->parent->parent,
				"dma-free", CSTR);
		free(s);
		inst->self = NULL;
	}
	
	return NO_ERROR;
}

C(f_deblock_seek)		/* seek (pos.lo pos.hi -- status) */
{
	Instance *inst;
	Self *s;
	uCell poshi, poslo;
#ifdef __LONGLONG
	uOctlet loc;
#endif
	
	DPRINTF(("deblocker seek: e=%p\n", e));
	inst = (Instance*)(uPtr)e->currinst;
	DPRINTF(("deblocker seek: inst=%p\n", inst));
	IFCKSP(e, 2, 1);
	
	if (inst == NULL || inst->self == NULL)
		return E_BAD_INSTANCE;

	s = inst->self;
	POP(e, poshi);
	POP(e, poslo);
	DPRINTF(("deblocker seek: hi=%#Cx lo=%#Cx\n", poshi, poslo));

#ifdef __LONGLONG
	/* only one of poshi or upper 32-bits of poslo may be non-zero */
	if (poshi && ((uOctlet)poslo >> QUADLET_SIZE))
	{
		PUSH(e, -1);		/* seek failure */
		return NO_ERROR;
	}

	loc = ((uOctlet)poshi << QUADLET_SIZE) + poslo;
	s->blocknum = loc / s->blocksize;
	s->offset = loc % s->blocksize;
#else
	/* if the high word of the position has a value, we cannot seek to it */
	if (poshi)
	{
		PUSH(e, -1);		/* seek failure */
		return NO_ERROR;
	}
	
	s->blocknum = poslo / s->blocksize;
	s->offset = poslo % s->blocksize;
#endif
	DPRINTF(("deblocker seek: block=%#x offset=%#x currblock=%#x modified=%d\n",
			s->blocknum, s->offset, s->currblock, s->modified));
	
	/* if this loc is not already in memory, may have to flush the buffer */
	if (s->modified && (s->blocknum < s->currblock ||
			s->blocknum >= s->currblock + s->xferblocks))
		return flushbuf(e, s);

	PUSH(e, 0);			/* 0 or 1 are seek success */
	return NO_ERROR;
}

static Retcode
do_io(Environ *e, Bool isread)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Byte *addr;
	uInt len, actual, xfer, offset, l;
	Retcode ret = NO_ERROR;
	Self *s;
	uInt total = 0;
	
	DPRINTF(("deblocker do_io %s\n", isread ? "read" : "write"));
	IFCKSP(e, 2, 1);
	
	if (inst == NULL || inst->self == NULL)
		return E_BAD_INSTANCE;

	s = inst->self;
	POP(e, len);
	POPT(e, addr, Byte*);
	DPRINTF(("deblocker do_io: addr=%p len=%#x\n", addr, len));
	
	for (;;)
	{
		DPRINTF(("deblocker do_io: currblock=%#x blocknum=%#x xferblocks=%#x\n",
				s->currblock, s->blocknum, s->xferblocks));

		/* is part or all of it already in memory? */
		if (s->currblock != -1 &&
				s->currblock <= s->blocknum &&
				s->blocknum < s->currblock + s->xferblocks)
		{
			/* pretend we just transfered it */
			actual = s->xferblocks - (s->blocknum - s->currblock);
			DPRINTF(("deblocker do_io: (in memory) actual=%#x\n", actual));
		}
		else
		{
			ret = flushbuf(e, s);		/* write out the buffer if modified */
			
			if (ret != NO_ERROR)
				return ret;
			
			/* read the chunk only if it will not be completely overwritten */
			if (isread || s->offset || len % s->blocksize)
			{
				PUSHP(e, s->block);
				PUSH(e, s->blocknum);
				PUSH(e, s->xferblocks);
				ret = execute_method_name(e, inst->parent,
						"read-blocks", CSTR);
				DPRINTF(("deblocker do_io: read-blocks: "
						"block=%p blocknum=%#x ret=%d\n",
						s->block, s->blocknum, ret));
				
				if (ret != NO_ERROR)
				{
					len = 0;	/* only push amount transferred so far */
					break;		/* and return */
				}
				
				POP(e, actual);

				if (actual == 0)
				{
					len = 0;
					break;
				}

				s->currblock = s->blocknum;
			}
			else
				actual = s->xferblocks;
		}

		/* number of bytes transferred; amount left to copy;
			offset from start of "block" */
		xfer = actual * s->blocksize;
		l = xfer - s->offset;
		offset = (s->blocknum - s->currblock) * s->blocksize + s->offset;
		DPRINTF(("deblocker do_io: xfer=%#x l=%#x offset=%#x len=%#x\n",
				xfer, l, offset, len));
		
		if (len > l)
		{
			/* requested length is longer than the amount we have available */
			if (isread)
			{
				DPRINTF(("deblocker do_io/r: memcpy to=%p from=%p l=%#x\n",
						addr, s->block + offset, l));
				memcpy(addr, s->block + offset, l);
			}
			else
			{
				DPRINTF(("deblocker do_io/w: memcpy to=%p from=%p l=%#x\n",
						s->block + offset, addr, l));
				memcpy(s->block + offset, addr, l);
				s->modified = TRUE;
			}
		
			addr += l;
			len -= l;
			total += l;
			s->offset = 0;
			s->blocknum = s->currblock + s->xferblocks;
			DPRINTF(("deblocker do_io: addr=%p len=%#x total=%#x\n",
					addr, len, total));
		}
		else if (len)
		{
			/* fits in our buffer */
			if (isread)
			{
				DPRINTF(("deblocker do_io/r: memcpy to=%p from=%p len=%#x\n",
						addr, s->block + offset, len));
				memcpy(addr, s->block + offset, len);
			}
			else
			{
				DPRINTF(("deblocker do_io/w: memcpy to=%p from=%p len=%#x\n",
						s->block + offset, addr, len));
				memcpy(s->block + offset, addr, len);
				s->modified = TRUE;
			}

			s->offset += len;
			DPRINTF(("deblocker do_io: offset=%#x\n", offset));
			break;
		}
	}
	
	PUSH(e, total + len);
	DPRINTF(("deblocker do_io: total=%#x len=%#x ret=%d\n", total, len, ret));
	return ret;
}

C(f_deblock_read)		/* read (addr len -- actual) */
{
	return do_io(e, TRUE);
}

C(f_deblock_write)		/* write (addr len -- actual) */
{
	return do_io(e, FALSE);
}

static const Initentry deblocker_methods[] =
{
	{ "open", f_deblock_open, INVALID_FCODE },
	{ "close", f_deblock_close, INVALID_FCODE },
	{ "read", f_deblock_read, INVALID_FCODE },
	{ "write", f_deblock_write, INVALID_FCODE },
	{ "seek", f_deblock_seek, INVALID_FCODE },
	{ NULL, NULL }
};

CC(install_deblocker)
{
	Package *pkg = new_pkg_name(e->packages, "deblocker");
	
	return init_entries(e, pkg->dict, deblocker_methods);
}
