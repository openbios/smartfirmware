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

/* Generic flash device driver */

/*#define DEBUG	*/

#include "defs.h"
#include "exe.h"
#include "flash.h"

#define WAIT_ITERATIONS 	100000

struct self
{
	Flash *type;
	Flash_methods *meths;
	int width;
	int partswide;
	uInt size;
	Int seekpos;
	Instance *disklabel;
	uByte *buf;
	void *addr;
};

typedef struct self Self;

#define FLASH_READ(s, offset, data) \
		((*(s)->meths->read)((s)->meths->private, (s)->addr, (offset), (data)))
#define FLASH_WRITE(s, offset, data) \
		((*(s)->meths->write)((s)->meths->private, (s)->addr, (offset), (data)))
#define FLASH_MAP_ADDR(s, offset) \
		((*(s)->meths->mapaddr)((s)->meths->private, (offset)))
#define FLASH_MAP_DATA(s, data) \
		((*(s)->meths->mapdata)((s)->meths->private, (data)))
	
extern Flash flash_sharp_lh28f320sktd;
extern Flash flash_intel_e28f32j3;
extern Flash flash_intel_e28f64j3;
extern Flash flash_intel_e28f128j3;

extern Flash flash_sst_sst49lf020a;
extern Flash flash_sst_sst49lf030a;
extern Flash flash_sst_sst49lf080a;

static Flash *flash_table[] =
{
	&flash_sharp_lh28f320sktd,
	&flash_intel_e28f32j3,
	&flash_intel_e28f64j3,
	&flash_intel_e28f128j3,
	&flash_sst_sst49lf020a,
	&flash_sst_sst49lf030a,
	&flash_sst_sst49lf080a,
	NULL,
};

static int
flash_wait_status(Environ *e, Self *s, int offset, uByte *mask, uByte *data)
{
	int w = s->width;
	int i;
	int j;

	/* toss away the first read since the chip state may be in flux */
	if (!FLASH_READ(s, offset, &s->buf[w]))
		return 0;

	for (i = 0; i < WAIT_ITERATIONS; i++)
	{
		if (!FLASH_READ(s, offset, s->buf))
			return 0;

		for (j = 0; j < w; j++)
			if ((s->buf[j] & mask[j]) != (data[j] & mask[j]))
				break;

		if (j >= w)
			break;
	}

	if (i >= WAIT_ITERATIONS)
		DPRINTF(("wait_status: %d: buf %02X %02X, mask %02X %02X, data %02X %02X\n", i, s->buf[0], s->buf[1], mask[0], mask[1], data[0], data[1]));

	return i < WAIT_ITERATIONS;
}

static int
flash_wait_toggle(Environ *e, Self *s, int offset, uByte *mask, uByte *data)
{
	int i;
	int j;
	int w = s->width;

//	DPRINTF(("wait_toggle: offset %X, mask %X, data %X\n", offset, mask[0], data[0]));

	/* toss away the first read since the chip state may be in flux */
	if (!FLASH_READ(s, offset, &s->buf[w]))
		return 0;

	if (!FLASH_READ(s, offset, &s->buf[w]))
		return 0;

	for (i = 0; i < WAIT_ITERATIONS; i++)
	{
		if (!FLASH_READ(s, offset, s->buf))
			return 0;

		for (j = 0; j < w; j++)
			if ((~s->buf[w + j] & mask[j]) == (s->buf[j] & mask[j]))
				break;

		if (j >= w)
			break;

		memcpy(&s->buf[w], s->buf, w);
	}

	if (i >= WAIT_ITERATIONS)
		DPRINTF(("wait_toggle: %d: buf %02X %02X, mask %02X %02X, data %02X %02X\n", i, s->buf[0], s->buf[1], mask[0], mask[1], data[0], data[1]));


	return i < WAIT_ITERATIONS;
}

static int
flash_wait_invert(Environ *e, Self *s, int offset, uByte *mask, uByte *data)
{
	int i;
	int j;
	int w = s->width;

	DPRINTF(("wait_invert: offset %X, mask %X, data %X\n", offset, mask[0], data[0]));

	/* toss away the first read since the chip state may be in flux */
	if (!FLASH_READ(s, offset, &s->buf[w]))
		return 0;

	for (i = 0; i < WAIT_ITERATIONS; i++)
	{
		if (!FLASH_READ(s, offset, s->buf))
			return 0;

		for (j = 0; j < w; j++)
			if ((s->buf[j] & mask[j]) != (data[j] & mask[j]))
				break;

		if (i >= 2 && j >= w)
			break;
	}

//	if (i >= WAIT_ITERATIONS)
		DPRINTF(("wait_invert: %d: buf %02X %02X, mask %02X %02X, data %02X %02X\n", i, s->buf[0], s->buf[1], mask[0], mask[1], data[0], data[1]));

	return i < WAIT_ITERATIONS;
}

static void
fill_buffer(Environ *e, Self *s, uByte *dest, uInt data)
{
	int partswide = s->partswide;
	int pwidth = s->type->width;
	int i;
	int j;
	uInt d;

	for (i = 0; i < partswide; i++)
	{
		d = data;

		for (j = 0; j < pwidth; j++)
		{
		    *dest++ = data & 0xFF;
		    data >>= 8;
		}
	}
}

static int
run_sequence(Environ *e, Self *s, const Sequence *seq, int offset, 
	uByte *data)
{
	uInt off;
	int w = s->width;
	uByte *t = &s->buf[w];

	if (offset < 0 || offset >= s->meths->size)
		return 0;

	for (; seq->type; seq++)
	{
#ifdef DEBUG2
		DPRINTF(("run_seq: seq %p, type %d\n", seq, seq->type));
#endif

		if (seq->offset < 0)
			off = offset;
		else
		{
			off = seq->offset * s->partswide;

			if (!FLASH_MAP_ADDR(s, &off))
				return 0;
		}

		switch (seq->type)
		{
		case FLASH_SEQ_WRITE:
			/* fill buf with data */
			fill_buffer(e, s, t, seq->data);

			/* map data in buf */
			if (!(FLASH_MAP_DATA(s, t)))
				return 0;

			/* write data */
			if (!FLASH_WRITE(s, off, t))
				return 0;

			break;
		case FLASH_SEQ_WRITE_DATA:
			if (!FLASH_WRITE(s, off, data))
				return 0;

			break;
		case FLASH_SEQ_READ_MANUF:
		case FLASH_SEQ_READ_DATA:
			if (!FLASH_READ(s, off, data))
				return 0;

			break;
		case FLASH_SEQ_READ_DEVID:
			if (!FLASH_READ(s, off, &data[s->width]))
				return 0;

			break;
		case FLASH_SEQ_WAIT_STATUS:
		case FLASH_SEQ_WAIT_TOGGLE:
		case FLASH_SEQ_WAIT_INVERT:
			{
				uByte *mask = &s->buf[2 * w];
				uByte *opdata = &s->buf[3 * w];
				fill_buffer(e, s, mask, seq->mask);
				fill_buffer(e, s, opdata, seq->data);

				if (!FLASH_MAP_DATA(s, mask))
					return 0;

				if (!FLASH_MAP_DATA(s, opdata))
					return 0;

				if (seq->type == FLASH_SEQ_WAIT_STATUS)
				{
					if (!flash_wait_status(e, s, off, mask, opdata))
						return 0;
				}
				else if (seq->type == FLASH_SEQ_WAIT_TOGGLE)
				{
					if (!flash_wait_toggle(e, s, off, mask, opdata))
						return 0;
				}
				else
				{
					if (!flash_wait_invert(e, s, off, mask, data))
						return 0;
				}
			}

			break;
		case FLASH_SEQ_DELAY:
			u_sleep(seq->data);
			break;
		default:
			return 0;
		}
	}

	return 1;
}

#ifdef NOT_USED
static int
reset_flash(Environ *e, Self *s, int offset)
{
	return run_sequence(e, s, s->type->reset, offset, (uByte *)0);
}
#endif

static int
sector_erase(Environ *e, Self *s, int offset)
{
	return run_sequence(e, s, s->type->erase, offset, (uByte *)0);
}

static int
program_word(Environ *e, Self *s, int offset, uByte *data)
{
	int i;

	for (i = 0; i < s->width; i++)
		if (data[i] != 0xFF)
			break;

	if (i >= s->width)
		return 1;

#ifdef DEBUG2
	DPRINTF(("program_word: e %p, s %p, offset %#x, data %#x, %02X %02X\n",
		e, s, offset, data, data[0], data[1]));
#endif
	return run_sequence(e, s, s->type->write, offset, data);
}

static int
read_word(Environ *e, Self *s, int offset, uByte *data)
{
	return run_sequence(e, s, s->type->read, offset, data);
}

static int
compare_flash(Environ *e, Self *s, int offset, uByte *data, int len, int ver)
{
	int z = 0;
	int i;
	int j;
	int width = s->meths->width;
	int mask = width - 1;
	uByte *buf = s->buf;
	int ok;
	int n;
	uInt b;
	uInt d;

	if (offset & mask)
	{
		ok = read_word(e, s, offset & ~mask, buf);
		n = width - (offset & mask);

		if (n > len)
			n = len;

		for (i = 0; i < n; i++)
		{
			b = buf[(offset & mask) + i];
			d = data[i];

			if (ver && (b != d))
			{
				cprintf(e, "Verify failed at offset %#X, read %#X, expected %#X\n", offset + i, b, d);
				ver = 0;
			}

			if (~b & d)
				return 2;

			if (b & ~d)
				z = 1;
		}
	}
	else
		i = 0;

	for ( ; i < len; i += width)
	{
		ok = read_word(e, s, (offset + i) & ~mask, buf);
		n = ((len - i) < width) ? (len - i) : width;

		for (j = 0; j < n; j++)
		{
			b = buf[j];
			d = data[i + j];

			if (ver && (b != d))
			{
				cprintf(e, "Verify failed at offset %#X, read %#X, expected %#X\n", offset + i + j, b, d);
				ver = 0;
			}

			if (~b & d)
				return 2;

			if (b & ~d)
				z = 1;
		}
	}

	return z;
}

static int
program_flash(Environ *e, Self *s, int offset, uByte *data, int len)
{
	Flash *flash = s->type;
	int i;
	uByte *buf = 0;
	int buflen = 0;
	uByte *bp;
	int bpoff;
	int bpsize;
	Bool diag = diagnostic_mode(e);
	int width = s->width;
	int partswide = s->partswide;
	const Sector *sectors = flash->sectors;
	int mask = width - 1;

	if (offset < 0)
		return 0;

	DPRINTF(("program_flash: e %p, s %p, offset %#x, data %p, len %d\n",
			e, s, offset, data, len));

	while (len)
	{
		int sector;
		int soffset;
		int ssize;
		int off;
		int sz;
		int diffs;

		for (i = 1; sectors[i].size; i++)
			if (sectors[i].start * partswide > offset)
				break;

		sector = i - 1;
		soffset = sectors[sector].start * partswide;
		ssize = sectors[sector].size * partswide;
		DPRINTF(("program_flash: sector %d, offset %#x, size %d\n",
				sector, soffset, ssize));

		/* check for write past end of flash */
		if (soffset + ssize <= offset)
		{
			if (buf)
				free(buf);

			return 0;
		}

		off = offset - soffset;
		sz = len < (ssize - off) ? len : (ssize - off);

		DPRINTF(("program_flash: off %#x, sz %d\n", off, sz));

		if (sz < 0 || sz > ssize)
		{
			cprintf(e, "sz out of range, %d\n", sz);

			if (buf)
				free(buf);

			return 0;
		}

		/* check to see if we need to erase or program*/
		diffs = compare_flash(e, s, offset, data, sz, 0);
		DPRINTF(("program_flash: compare, %#x, %d, %d\n", offset, sz, diffs));

		if (diffs & 2)
		{
			diffs = 1;

			if (off || sz < ssize)
			{
				/* preserve the sector contents */
				if (buflen < ssize)
				{
					if (buf)
						free(buf);

					buf = (uByte *)malloc(ssize);
					buflen = ssize;

					if (!buf)
					{
						cprintf(e, "Unable to allocate sector buffer\n");
						return 0;
					}

					DPRINTF(("buf = %p\n", buf));
				}

				for (i = 0; i < ssize; i += width)
					read_word(e, s, soffset + i, &buf[i]); 

				for (i = 0; i < sz; i++)
					buf[off + i] = data[i];

				bp = buf;
			}
			else
				bp = data;

			bpoff = soffset;
			bpsize = ssize;
			DPRINTF(("program_flash: bp %p, bpoff %#x, bpsize %d\n",
					bp, bpoff, bpsize));

			/* erase sector */
			if (diag)
				cprintf(e, "Erasing sector %d\n", sector);
			else
				spin_cursor(e);

			DPRINTF(("program_flash: erasing sector %#x\n", soffset));
			sector_erase(e, s, soffset);
		}
		else
		{
			bp = data;
			bpoff = offset;
			bpsize = sz;
			DPRINTF(("program_flash: 2 bp %p, bpoff %#x, bpsize %d\n",
					bp, bpoff, bpsize));
		}

		if (diffs & 1)
		{
			if (diag)
				cprintf(e, "Programming sector %d\n", sector);
			else
				spin_cursor(e);

			DPRINTF(("program_flash: programming offset %#x, size %d\n", bpoff, bpsize));

			/* program */
			if (bpoff & mask)
			{
				/* handle odd bytes */
				int n = width - (bpoff & mask);

				if (n > bpsize)
					n = bpsize;

				DPRINTF(("program_flash: %d misaligned bytes at %#x\n", n, bpoff, bp));

				read_word(e, s, bpoff & ~mask, s->buf);

				for (i = 0; i < n; i++)
					s->buf[(bpoff & mask) + i] = bp[i];

				program_word(e, s, bpoff & ~mask, s->buf);
			}
			else
				i = 0;

			for (; i + width - 1 < bpsize; i += width)
			{
				if ((((i / width) + 1) % (4*1024)) == 0)
					spin_cursor(e);

				program_word(e, s, (bpoff + i) & ~mask, &bp[i]);
			}

			DPRINTF(("program_flash: %d aligned bytes remainder at %#x, %p\n", i, bpoff, bp));

			if (i < bpsize)
			{
				/* handle odd bytes at the tail */
				int j;

				DPRINTF(("program_flash: %d misaligned bytes remainder at %#x, %p\n", bpsize - i, bpoff + i, bp + i));

				read_word(e, s, (bpoff + i) & ~mask, s->buf);

				DPRINTF(("program_flash: buf[0] %#x, buf[1] %#x\n", s->buf[0], s->buf[1]));

				for (j = 0; i < bpsize; j++, i++)
					s->buf[j] = bp[i];

				DPRINTF(("program_flash: new buf[0] %#x, buf[1] %#x\n", s->buf[0], s->buf[1]));

				program_word(e, s, (bpoff + i) & ~mask, s->buf);
			}

			/* verify bytes */
			diffs = compare_flash(e, s, bpoff, bp, bpsize, 1);
			DPRINTF(("program_flash: compare: bpoff %p, bp %#x, bpsize %d, diffs %d\n",
					bpoff, bp, bpsize, diffs));
		}

		offset += sz;
		data += sz;
		len -= sz;
	}

	if (buf)
		free(buf);

	DPRINTF(("buf = %p free\n", buf));
	DPRINTF(("program_flash: offset %#x, data %#x, len %d\n",
					offset, data, len));

	/* return OK */
	return 1;
}

static int
read_flash(Environ *e, Self *s, int offset, uByte *data, int len)
{
	int actual = 0;
	int width = s->meths->width;
	int mask = width - 1;
	int i;

	DPRINTF(("read_flash: offset %#x, data %#x, len %d\n", offset, data, len));

	if (offset & mask)
	{
		int n = width - (offset & mask);
		DPRINTF(("read_flash: %d unaligned bytes at %#x\n", n, offset));

		if (n > len)
			n = len;

		read_word(e, s, offset, s->buf);

		for (i = 0; i < n; i++)
			data[i] = s->buf[(offset & mask) + i];

		data += n;
		len -= n;
		actual += n;
		offset += n;
	}

	while (len >= width)
	{
		read_word(e, s, offset, data);
#if 0
		DPRINTF(("read_flash: %d bytes at %#x, %#x %#x\n", width, offset, data[0], data[1]));
#endif
		data += width;
		len -= width;
		actual += width;
		offset += width;
	}

	if (len)
	{
		read_word(e, s, offset, s->buf);
		DPRINTF(("read_flash: %d trailing bytes at %#x, %#x\n", len, offset, s->buf[0]));

		for (i = 0; i < len; i++)
			data[i] = s->buf[i];

		actual += len;
	}

	DPRINTF(("read_flash: actual = %d\n", actual));
	return actual;
}

static Flash *
flash_probe(Environ *e, Self *s)
{
	Flash **ft;
	Flash *fp;
	uByte *buf;
	uByte *manuf;
	uByte *dev;
	int i;
	int w = s->width;

	buf = (uByte *)malloc(4 * w);

	if (buf == (uByte *)0)
		return (Flash *)0;

	manuf = &buf[2 * w];
	dev = &buf[3 * w];

	for (ft = flash_table; (fp = *ft); ft++)
	{
		s->type = fp;
		s->partswide = s->width / fp->width;

		/* read id information from flash */
		run_sequence(e, s, fp->probe, 0, buf);

		/* compute id match information */
		fill_buffer(e, s, manuf, fp->manufid);
		fill_buffer(e, s, dev, fp->devid);

		DPRINTF(("buf = %02X %02X %02X %02X %02X %02X %02X %02X\n",
				buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]));

		/* compare ids */
		for (i = 0; i < w; i++)
			if (buf[i] != manuf[i] || buf[w + i] != dev[i])
				    break;

		if (i == w)
			break;
	}

	DPRINTF(("flash_probe: fp = 0x%X\n", fp));
	free((void *)buf);
	return fp;
}

C(f_gflash_open)			/* open (-- okay?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	Flash *fp;
	Flash_methods *meths;
	Retcode ret;
	Cell virt;
	int i;

	DPRINTF(("flash open\n"));
	IFCKSP(e, 0, 5);

	if (inst == NULL)
	{
		PUSH(e, FFALSE);
		return E_BAD_INSTANCE;
	}

	if (inst->package == NULL)
	{
		PUSH(e, FFALSE);
		return E_BAD_PACKAGE;
	}

	meths = (Flash_methods *)inst->package->self;
	s = (Self *)malloc(sizeof *s + 4 * meths->width);

	if (!s)
	{
		PUSH(e, FFALSE);
		return E_OUT_OF_MEMORY;
	}

	memset(s, 0, sizeof *s + 4 * meths->width);
	s->meths = meths;
	s->width = meths->width;
	s->buf = (uByte *)(s + 1);

	/* map-in the device */
	for (i = meths->unitcells - 1; i >= 0; i--)
		PUSH(e, meths->unit[i]);

	PUSH(e, meths->size);
	DPRINTF(("flash open unit[0] %#x,%#x, size %#x\n", meths->unit[0], meths->unit[1], meths->size));

	ret = execute_method_name(e, inst->parent, "map-in", CSTR);

	if (ret != NO_ERROR)
	{
		free(s);
		PUSH(e, FFALSE);
		return ret;
	}

	POP(e, virt);
	s->addr = (void *)virt;
	DPRINTF(("flash open virt addr %#Cx\n", virt));

#ifdef DEBUG
	PUSH(e, virt);
	execute_word(e, "map?");
#endif

	if (s->addr == (void *)0)
	{
		DPRINTF(("flash open: unable to map-in\n"));
		free(s);
		PUSH(e, FFALSE);
		return E_NO_DEVICE;
	}

	fp = flash_probe(e, s);

	if (fp == NULL)
	{
		free(s);
		PUSH(e, FFALSE);
		return E_NO_DEVICE;
	}

	DPRINTF(("flash open fp %p\n", fp));

	inst->self = s;
	s->type = fp;
	s->seekpos = 0;
	s->partswide = s->width / fp->width;

	/* create an instance of /packages/disk-label to support load/etc */
	PUSHP(e, inst->args + 1);		/* arg-str */
	PUSH(e, *(uByte *)inst->args);	/* arg-len */
	PUSHP(e, "disk-label");			/* name-str */
	PUSH(e, strlen("disk-label"));	/* name-len */
	ret = execute_word(e, "$open-package");

	if (ret != NO_ERROR)
	{
		free(s);
		PUSH(e, FFALSE);
		return ret;
	}

	POPT(e, s->disklabel, Instance*);

	if (s->disklabel == NULL)
	{
		free(s);
		PUSH(e, FFALSE);
		return E_NULL_INSTANCE;
	}

	DPRINTF(("flash open disklabel %p\n", s->disklabel));
	PUSH(e, FTRUE);
	return NO_ERROR;
}

C(f_gflash_close)			/* close (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	Retcode ret;

	DPRINTF(("flash close\n"));

	if (inst == NULL)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s || !s->type || !s->disklabel)
		return E_BAD_INSTANCE;

	/* close up our helper packages */
	IFCKSP(e, 0, 1);
	PUSHP(e, s->disklabel);
	(void)execute_word(e, "close-package");

	/* map-out */
	PUSH(e, s->addr);
	PUSH(e, s->meths->size);
	ret = execute_method_name(e, inst->parent, "map-out", CSTR);

	/* free our self block */
	free(s);
	return ret;
}

C(f_gflash_selftest)		/* selftest (-- 0|error-code) */
{
	DPRINTF(("flash selftest\n"));
	PUSH(e, 0);
	return NO_ERROR;
}

C(f_gflash_read_bytes)			/* read-bytes (addr len off -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int off;
	Int len;
	Int actual = 0;
	Byte *addr;

	DPRINTF(("flash read-bytes\n"));

	if (inst == NULL || inst->self == NULL)
		return E_BAD_INSTANCE;

	if (inst->package == NULL)
		return E_BAD_PACKAGE;

	IFCKSP(e, 3, 1);
	POP(e, off);
	POP(e, len);
	POPT(e, addr, Byte*);

	DPRINTF(("flash read-bytes: off %#x, addr %#x, len %d\n", off, addr, len));
	actual = read_flash(e, inst->self, off, addr, len);
	DPRINTF(("flash read-bytes: actual = %d\n", actual));

	PUSH(e, actual == 0 ? -2 : actual);
	return NO_ERROR;
}

C(f_gflash_write_bytes)			/* write-bytes (addr len off -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int off;
	Int len;
	Byte *addr;
	Self *s;
	DPRINTF(("flash write-bytes\n"));

	if (inst == NULL || inst->self == NULL)
		return E_BAD_INSTANCE;

	if (inst->package == NULL)
		return E_BAD_PACKAGE;

	s = inst->self;

	IFCKSP(e, 3, 1);
	POP(e, off);
	POP(e, len);
	POPT(e, addr, Byte*);

	if (!program_flash(e, s, off, addr, len))
	    len = -2;

	PUSH(e, len);
	return NO_ERROR;
}

C(f_gflash_read)			/* read (addr len -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int off;
	Int len;
	Int actual = 0;
	Byte *addr;
	Self *s;
	DPRINTF(("flash read\n"));

	if (inst == NULL || inst->self == NULL)
		return E_BAD_INSTANCE;

	if (inst->package == NULL)
		return E_BAD_PACKAGE;

	s = inst->self;

	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	off = s->seekpos;
	DPRINTF(("flash read off = %d (%d), buf = %#X\n", off, off - 64*1024, addr));

	actual = read_flash(e, s, off, addr, len);

	s->seekpos += actual;
	PUSH(e, actual == 0 ? -2 : actual);
	return NO_ERROR;
}

C(f_gflash_write)			/* write (addr len -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int off;
	Int len;
	Byte *addr;
	Self *s;
	DPRINTF(("flash write\n"));

	if (inst == NULL || inst->self == NULL)
		return E_BAD_INSTANCE;

	if (inst->package == NULL)
		return E_BAD_PACKAGE;

	s = inst->self;

	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	off = s->seekpos;

	if (!program_flash(e, s, off, addr, len))
	    len = -2;

	if (len > 0)
		s->seekpos = off + len;

	PUSH(e, len);
	return NO_ERROR;
}

C(f_gflash_seek)			/* seek (pos.hi pos.lo -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	uInt poshi, poslo;
	uInt bias;
	
	DPRINTF(("flash seek\n"));
	IFCKSP(e, 2, 1);

	if (inst == NULL || inst->self == NULL)
		return E_BAD_INSTANCE;

	s = inst->self;
	bias = s->meths->bias;
	POP(e, poshi);
	POP(e, poslo);

	DPRINTF(("flash seek: hi=%u lo=%u\n", poshi, poslo));

	poslo += bias;

	if (poslo < bias)		/* propogate carry */
	    poshi++;

	if (poshi != 0U || poslo > s->type->size * 4)
	{
		PUSH(e, -1);		/* seek failure */
		return NO_ERROR;
	}

	s->seekpos = poslo;

	PUSH(e, 0);			/* 0 or 1 are seek success */
	return NO_ERROR;
}

C(f_gflash_load)				/* load (addr -- size) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;

	if (!inst)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s)
		return E_BAD_INSTANCE;

	if (!s->disklabel)
		return E_NULL_PACKAGE;

	return execute_method_name(e, s->disklabel, "load", CSTR);
}

static void
dumpbuf(uByte *addr, uInt off, int len)
{
	int i;

	for (i = 0; i < len; off++, i++)
	{
		if ((i & 0xF) == 0)
			DPRINTF(("%06X:  ", off));

		DPRINTF(("%02X ", addr[i]));

		if ((i & 0xF) == 0xF)
			DPRINTF(("\n"));
		else
		{

			if ((i & 3) == 3)
				DPRINTF((" "));

			if ((i & 7) == 7)
				DPRINTF((" "));
		}
	}
}

#ifdef DEBUG
C(f_gflash_ftest)				/* ftest ( -- ) */
{
	Instance *saveinst = (Instance*)(uPtr)e->currinst;
	Instance *inst;
	Byte name[STR_SIZE];
	Retcode ret;
	Retcode ret2;
#define TESTSIZE 16
	uByte buf1[TESTSIZE];
	uByte buf2[TESTSIZE];
	int i;
	Cell actual;
	uInt bias;

	IFCKSP(e, 0, 3);
	DPRINTF(("ftest: begin\n"));

	if (e->currpkg == NULL || !get_device_name(e, e->currpkg, name))
		return E_BAD_PACKAGE;

	DPRINTF(("ftest: path %s\n", name));

	PUSHP(e, name);
	PUSH(e, strlen(name) + 1);
	ret = execute_word(e, "open-dev");

	if (ret != NO_ERROR)
		return ret;

	POPT(e, inst, Instance *);
	e->currinst = (Cell)inst;
	bias = inst->self->meths->bias;

	DPRINTF(("ftest: instance %#x\n", inst));

	/* call read-bytes */
	PUSH(e, buf1);
	PUSH(e, TESTSIZE);
	PUSH(e, bias);
	ret = f_gflash_read_bytes(e);
	POP(e, actual);
	DPRINTF(("ftest: read-bytes %d (%d)\n", actual, ret));
	dumpbuf(buf1, bias, TESTSIZE);

	/* modify array */
	for (i = 0; i < TESTSIZE; i++)
		if (buf1[i])
		{
			buf1[i] = 0;
			break;
		}

	if (i >= TESTSIZE)
	{
		buf1[0] = 0;

		for (i = 1; i < TESTSIZE; i++)
			buf1[i] = 0xFF;
	}

	DPRINTF(("ftest: revised buffer\n"));
	dumpbuf(buf1, bias, TESTSIZE);

	/* call write-bytes */
	PUSH(e, buf1);
	PUSH(e, TESTSIZE);
	PUSH(e, bias);

	ret2 = f_gflash_write_bytes(e);
	ret = (ret != NO_ERROR) ? ret : ret2;

	POP(e, actual);
	DPRINTF(("ftest: write-bytes %d (%d)\n", actual, ret));

	/* call read-bytes, different array */
	PUSH(e, buf2);
	PUSH(e, TESTSIZE);
	PUSH(e, bias);

	ret2 = f_gflash_read_bytes(e);
	ret = (ret != NO_ERROR) ? ret : ret2;

	POP(e, actual);
	DPRINTF(("ftest: read-bytes %d (%d)\n", actual, ret));
	dumpbuf(buf2, bias, TESTSIZE);

	/* verify that arrays match */
	for (i = 0; i < TESTSIZE; i++)
		if (buf1[i] != buf2[i])
		{
			cprintf(e, "ftest: data mismatch at offset %d\n", i);
			break;
		}

	/* close devices */
	PUSH(e, inst);
	ret2 = execute_word(e, "close-dev");
	e->currinst = (Cell)saveinst;
	return ret != NO_ERROR ? ret : ret2;
}
#endif

static const Initentry flash_methods[] =
{
	{ "open",          f_gflash_open,          INVALID_FCODE },
	{ "close",         f_gflash_close,         INVALID_FCODE },
	{ "selftest",      f_gflash_selftest,      INVALID_FCODE },
	{ "read-bytes",    f_gflash_read_bytes,    INVALID_FCODE },
	{ "write-bytes",   f_gflash_write_bytes,   INVALID_FCODE },
	{ "read",          f_gflash_read,          INVALID_FCODE },
	{ "write",         f_gflash_write,         INVALID_FCODE },
	{ "seek",          f_gflash_seek,          INVALID_FCODE },
	{ "load",          f_gflash_load,          INVALID_FCODE },
#ifdef DEBUG
	{ "ftest",         f_gflash_ftest,         INVALID_FCODE },
#endif
	{ NULL,            NULL },
};

Retcode
generic_install_flash(Environ *e, Flash_methods *methods)
{
	Retcode ret;
	Package *pkg;
	Byte *prop;
	Flash *f;
	Int plen = 0;
	Self self;
	int i;

	DPRINTF(("generic_install_flash: e=%#x\n", e));

	memset(&self, 0, sizeof self);
	self.meths = methods;
	self.width = methods->width;
	self.buf = (uByte *)malloc(4 * self.width);

	if (self.buf == NULL)
		return NO_ERROR;

	for (i = methods->unitcells - 1; i >= 0; i--)
		PUSH(e, methods->unit[i]);

	PUSH(e, methods->size);
	DPRINTF(("generic_install_flash: about to map-in\n", ret));
	ret = execute_static_method_name(e, e->root, "map-in", CSTR);
	DPRINTF(("generic_install_flash: map-in returns %d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	POPT(e, self.addr, void *);
	DPRINTF(("generic_install_flash: self.addr %p\n", self.addr));

	if (self.addr == (void *)0)
	{
		DPRINTF(("generic_install_flash: unable to map-in\n"));
		return E_NO_DEVICE;
	}

	/* show the mapping */
	//PUSH(e, (Cell)self.addr);
	//f_mapq(e);

	f = flash_probe(e, &self);
	DPRINTF(("flash_probe: ret=%p\n", f));
	free((void *)self.buf);
	PUSH(e, self.addr);
	PUSH(e, methods->size);
	ret = execute_static_method_name(e, e->root, "map-out", CSTR);
	DPRINTF(("generic_install_flash: map-out returns %d\n", ret));

	if (!f)
		return E_NO_DEVICE;

	IFCKSP(e, 0, 3);
	pkg = new_pkg_name(e->currpkg, "flash");

	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	pkg->self = (struct pself*)methods;

	/* set the type of this device */
	prop_set_str(pkg->props, "device_type", CSTR, "block", CSTR);

	/* encode "reg" property for unit address */
	prop = prop_alloc(e, methods->unitcells * 2 * sizeof (Int));

	if (prop == NULL)
		return E_OUT_OF_MEMORY;

	for (i = 0; i < methods->unitcells; i++)
		prop_encode_int(prop + plen, &plen, methods->unit[i]);

	for (i = 0; i < methods->unitcells - 1; i++)
		prop_encode_int(prop + plen, &plen, 0);

	prop_encode_int(prop + plen, &plen, f->size * (methods->width / f->width));

	ret = add_property(pkg->props, "reg", CSTR, prop, plen);

	/* add device specific information */
	prop_set_str(pkg->props, "manufacture", CSTR, f->manufname, CSTR);
	prop_set_str(pkg->props, "device", CSTR, f->devname, CSTR);
	prop_set_int(pkg->props, "id", CSTR, (f->manufid << 16) | f->devid);
	prop_set_int(pkg->props, "bias", CSTR, methods->bias);

	if (ret == NO_ERROR)
		ret = init_entries(e, pkg->dict, flash_methods);

	return ret;
}

#if 0
This is the basic structure of the technology independent flash programming
drivers.  The flash descriptor indicates the following.

	word_width		Width of the largest accessible word of flash
				Either 8, 16 or 32.
	array_width		Width of the array in words
	flash_window		Maximum number of bytes of address decoded
				for flash array.

	To probe for flash we assume that 

	if ((uInt)data & 7) != (offset & 7))
	{
		allocate buffer
		align buffer
		fixup first and last words
		copy data to buffer
	}
#endif
