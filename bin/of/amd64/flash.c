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

/* Solo flash device driver */

/*#define DEBUG*/

#include "defs.h"
#include "flash.h"
#include "updateimage.h"

static int flash_probed = 0;

#ifdef DEBUG
#define LOG_SIZE	4096
Bool readops[LOG_SIZE];
uPtr addrs[LOG_SIZE];
uByte values[LOG_SIZE];
int logsize = 0;

static void
add_log(uPtr addr, uByte val, Bool readop)
{
	if (logsize >= LOG_SIZE)
		return;

	readops[logsize] = readop;
	addrs[logsize] = addr;
	values[logsize] = val;
	logsize++;
}
#endif

static int
flash_read(void *private, void *va, uInt offset, uByte *data)
{
	uByte v;
	uByte *x;
	x = (uByte *)va + offset;
	v = *x;
#ifdef DEBUG
	add_log((uPtr)x, (uByte)v, 1);
#endif
#ifdef DEBUG2
	DPRINTF(("flash_read: private %p, va %p, offset %#x, data %p, v %#x\n", private, va, offset, data, v));
#endif
	data[0]  = v;
	return 1;
}

static int
flash_write(void *private, void *va, uInt offset, uByte *data)
{
	uByte v;
	uByte *x;
	x = (uByte *)va + offset;
	v =  data[0];
#ifdef DEBUG2
	DPRINTF(("flash_write: private %p, va %p, offset %#x, data %p, x %p, v %#x\n", private, va, offset, data, x, v));
#endif
	*x = v;
#ifdef DEBUG
	add_log((uPtr)x, (uByte)v, 0);
#endif
	return 1;
}

static int
flash_mapaddr(void *private, uInt *offset)
{
#ifdef DEBUG2
	DPRINTF(("flash_mapaddr: private %p, offset %p, *offset %#x\n", private, offset, *offset));
#endif
	return 1;
}

static int
flash_mapdata(void *private, uByte *data)
{
#ifdef DEBUG2
	DPRINTF(("flash_mapdata: private %p, *data %p, data %02X %02X\n", private, data, data[0], data[1]));
#endif
	return 1;
}

static Retcode
flash_op(Environ *e, uInt offset, uByte *buf, Int len, Bool readop)
{
	Instance *saveinst;
	Instance *inst;
	Retcode ret;
	Cell actual;

	/* open device */
	IFCKSP(e, 0, 2);
#define PATH "/flash@FFF00000"
	PUSH(e, PATH);
	PUSH(e, (sizeof PATH) - 1);
	DPRINTF(("flash_op: open-dev\n"));
	ret = execute_word(e, "open-dev");

	DPRINTF(("flash_op: open-dev: ret = %d\n", ret));
	
	if (ret != NO_ERROR)
		return ret;
	
	IFCKSP(e, 1, 0);
	POPT(e, inst, Instance*);
	saveinst = (Instance *)e->currinst;
	e->currinst = (Cell)inst;
	DPRINTF(("flash_op: inst %p\n", inst));

	/* call read-bytes or write-bytes */
	IFCKSP(e, 0, 3);
	PUSH(e, (Cell)buf);
	PUSH(e, len);
	PUSH(e, offset);

	if (readop)
		ret = execute_method_name(e, inst, "read-bytes", CSTR);
	else
		ret = execute_method_name(e, inst, "write-bytes", CSTR);

	if (ret != NO_ERROR)
	{
		IFCKSP(e, 0, 1);
		PUSH(e, (Cell)inst);
		execute_word(e, "close-dev");
		DPRINTF(("flash_op: execute-method-name: ret = %d\n", ret));
		return ret;
	}

	IFCKSP(e, 1, 0);
	POP(e, actual);

	IFCKSP(e, 0, 1);
	PUSH(e, (Cell)inst);
	ret = execute_word(e, "close-dev");
	e->currinst = (Cell)saveinst;
	DPRINTF(("flash_op: close-dev: ret = %d\n", ret));
	return actual == len ? ret :
			readop ? E_PROM_READ_ERROR : E_PROM_WRITE_ERROR;
}

/*
 * This code implements the AUTODIN II polynomial
 * The variable corresponding to the macro argument "crc" should
 * be an unsigned long.
 * Oroginal code  by Spencer Garrett <srg@quick.com>
 */

#define CRC32(crc, ch)	 (crc = (crc >> 8) ^ crc32tab[(crc ^ (ch)) & 0xff])

/* generated using the AUTODIN II polynomial
 *	x^32 + x^26 + x^23 + x^22 + x^16 +
 *	x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x^1 + 1
 */

static const unsigned int crc32tab[256] =
{
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
	0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
	0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
	0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
	0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
	0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
	0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
	0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
	0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
	0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
	0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
	0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
	0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
	0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
	0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
	0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
	0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
	0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
	0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
	0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
	0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
	0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
	0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
	0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};

unsigned int
crc32(unsigned char *buf, int len)
{
	unsigned int crc = ~0;
	int i;
	
	for (i = 0; i < len; i++, buf++)
	    CRC32(crc, *buf);

	return ~crc;
}


C(do_selftest)
{
	Retcode ret;
	Cell virt;
	Bool ok;
	Bool diag = diagnostic_mode(e);

	IFCKSP(e, 0, 2);
	PUSH(e, 0xFFF00000);
	PUSH(e, 0x100000);
	ret = execute_static_method_name(e, e->root, "map-in", CSTR);

	if (ret != NO_ERROR)
		return ret;

	IFCKSP(e, 1, 0);
	POP(e, virt);

	ok = TRUE;

	if (diag && !ok)
		cprintf(e, "firmware checksum failed\n");

	PUSH(e, virt);
	PUSH(e, 0x100000);
	ret = execute_static_method_name(e, e->root, "map-out", CSTR);

	return ret != NO_ERROR ? ret : (ok ? NO_ERROR : E_SELFTEST_FAILED);
}

C(f_flash_selftest)
{
	Retcode ret;
	Bool diag = diagnostic_mode(e);

	if (diag)
		cprintf(e, "flash selftest\n");

	ret = do_selftest(e);

	PUSH(e, ret);
	return NO_ERROR;
}

Bool
verify_update(Environ *e, uByte *image, int len)
{
	unsigned int crc1;
	unsigned int crc2;
	struct header *header = (struct header *)image;
	struct segheader *segs;
	int num;
	int i;

	if (header->magic != SF_UPDATE_MAGIC_REV0)
	{
		cprintf(e, "Bad magic\n");
		return FALSE;
	}

	if (header->size != len)
	{
		cprintf(e, "Bad length %d expect %d\n", len, header->size);
		return FALSE;
	}

	crc1 = header->crc;
	header->crc = 0;
	crc2 = crc32(image, len);
	header->crc = crc1;

	if (crc1 != crc2)
	{
		cprintf(e, "Bad file crc\n");
		return FALSE;
	}

	num = header->numsegments;
	segs = (struct segheader *)&image[header->hoffset];

	if (num < 0 || num > MAX_SEGMENTS)
	{
		cprintf(e, "Bad segment count\n");
		return FALSE;
	}

	if (len < header->hoffset + sizeof (struct segheader) * num)
	{
		cprintf(e, "Bad length (no segment headers)\n");
		return FALSE;
	}

	for (i = 0; i < num; i++)
	{
		if (segs[i].physaddr < 0xFFF00000ULL || 
				segs[i].physaddr + segs[i].size > 0x100000000ULL)
		{
			cprintf(e, "Bad physaddr %d, %qX\n", i, segs[i].physaddr);
			return FALSE;
		}

		if (segs[i].offset + segs[i].size > len)
		{
			cprintf(e, "Bad segment %d offset\n", i);
			return FALSE;
		}

		if (crc32(&image[segs[i].offset], segs[i].size) != segs[i].crc)
		{
			cprintf(e, "Bad segment %d crc\n", i);
			return FALSE;
		}
	}

	return TRUE;
}

C(f_flash_update)
{
	Retcode ret;
	Bool diag = diagnostic_mode(e);
	uByte *buf = (uByte *)e->load;
	uInt buflen = (uInt)e->loadlen;
	struct header *header = (struct header *)buf;
	struct segheader *segs;
	int num;
	int i;

	if (buflen >= sizeof *header && header->magic != SF_UPDATE_MAGIC_REV0)
	{
		exec_load(e);			/* decompress if necessary */
		buflen = (uInt)e->loadlen;
	}

	if (buflen < sizeof *header || header->magic != SF_UPDATE_MAGIC_REV0)
	{
		/* verify that load area has a valid image */
		cprintf(e, "Unrecognized update image format: flash unchanged\n");
		return E_BAD_IMAGE;
	}

	if (!verify_update(e, (uByte *)buf, buflen))
	{
		cprintf(e, "Update image checksum failed: flash unchanged\n");
		return E_BAD_IMAGE;
	}

	num = header->numsegments;
	segs = (struct segheader *)&buf[header->hoffset];

	if (diag)
		cprintf(e, "Image base address %#x, size %dK\n", buf, buflen / 1024);

	for (i = 0; i < num; i++)
	{
	    uByte *seg = &buf[segs[i].offset];
	    int size = segs[i].size;
	    uInt offset = (uInt)(segs[i].physaddr - 0xFFF00000ULL);
	    int j;

	    if (diag)
		cprintf(e, "Writing %d bytes at offset %#X from %p\n", size, offset, seg);

	    /* write flash */
	    ret = flash_op(e, offset, seg, size, FALSE);

	    if (ret != NO_ERROR)
		return ret;

	    /* read flash */
	    ret = flash_op(e, offset, buf + buflen, size, TRUE);

	    if (ret != NO_ERROR)
		return ret;

	    /* compare results */
	    for (j = 0; j < size; j++)
		    if (seg[i] != buf[buflen + i])
			    break;

	    if (j < size)
	    {
		j += segs[i].offset;
		cprintf(e, "Verify failed at offset %d (%#x)\n", j, j);
		return E_PROM_WRITE_ERROR;
	    }
	}

	return NO_ERROR;
}

#ifdef DEBUG
C(f_show_flash_log)		/* ( -- ) */
{
	int i;

	for (i = 0; i < logsize; i++)
		cprintf(e, "[%d] %s %#x %s %#x\n", i, readops[i] ? " read" : "write", addrs[i], readops[i] ? "-->" : "<--", values[i]);

	return NO_ERROR;
}
#endif

static const Initentry flash_add_methods[] =
{
	{ "selftest",      f_flash_selftest,      INVALID_FCODE },
	{ NULL, NULL }
};

static const Initentry flash_words[] =
{
	{ "flash-update",  f_flash_update,        INVALID_FCODE },

#ifdef DEBUG
	{ "show-flash-log",f_show_flash_log,      INVALID_FCODE },
#endif

	{ NULL, NULL }
};

static Flash_methods solo_flash =
{
	(void *)0,		/* private info */
	1,			/* 8-bits wide */
	flash_read,
	flash_write,
	flash_mapaddr,
	flash_mapdata,
//	0x100000,		/* reserve the first one-meg for bootcode */
	0x0e1000,		/* reserve the first 900K for bootcode */
	0x100000,		/* 1MB flash address window */
	0x100000,		/* 1MB same as above */
	{ 0x0000000, 0xFFF00000 }, /* phys address == 0xFFF00000 */
	2,			/* number of words in phys address */
};

CC(install_flash)
{
	Retcode ret;
	DPRINTF(("install_flash: begin\n"));

	e->currpkg = e->root;

	flash_probed = 1;
	ret = generic_install_flash(e, &solo_flash);

	DPRINTF(("install_flash: ret = %d\n", ret));

	if (ret == NO_ERROR)
		ret = init_entries(e, e->names, flash_words);

	DPRINTF(("install_flash: init_entries ret = %d\n", ret));

	if (ret == NO_ERROR)
	{
#if 0
		Instance *inst;

		PUSH(e, "/flash@FFF00000");
		PUSH(e, 14);
		DPRINTF(("flash_op: open-dev\n"));
		ret = execute_word(e, "open-dev");

		if (ret == NO_ERROR)
		{
			IFCKSP(e, 1, 0);
			POPT(e, inst, Instance*);

			ret = init_entries(e, inst->package->dict, flash_add_methods);

			PUSH(e, inst);
			ret = execute_word(e, "close-dev");
		}
#endif
	}

	return ret;
}

#define NVRAM_SIZE	0x2000
#define NVRAM_BASE	0xEE000
#define HAMMER_VA_FLASH	0xFFF00000
#define HAMMER_VA_NVRAM	(HAMMER_VA_FLASH + NVRAM_BASE)

uInt
machine_nvram_size(Environ *e)
{
	return NVRAM_SIZE;
}

static Retcode
nvram_read(Environ *e, uInt offset, uByte *buf, uInt len)
{
	uByte *nvram = (uByte *)HAMMER_VA_NVRAM;
	int i;

	if (offset + len > NVRAM_SIZE)
		return E_PROM_READ_ERROR;

	for (i = 0; i < len; i++)
		buf[i] = nvram[offset + i];

	return NO_ERROR;
}

static Retcode
nvram_offset(Environ *e, uInt *offset)
{
	uInt off = 0;
	uInt lastoff = 0;
	uByte tmp[6];
	uInt sz;
	Retcode ret;

	while (1)
	{
		ret = nvram_read(e, off, tmp, 6);
		DPRINTF(("nvram_offset: read header at %X, ret %d\n", off, ret));

		if (ret != NO_ERROR)
			return ret;

		sz = (tmp[2] << 8) | tmp[3];
		DPRINTF(("nvram_offset: block size %X\n", sz));

		if (tmp[0] != 0xBE || tmp[1] != 0xEF || off + sz > NVRAM_SIZE - 7)
		{
			if (off == 0)
				return E_PROM_READ_ERROR;

			if (offset)
				*offset = lastoff;

			return NO_ERROR;
		}

		lastoff = off;
		off += sz + 7;
	}
}

Retcode
machine_nvram_write(Environ *e, uByte *buf, uInt len)
{
	uInt off;
	uInt sz;
	Bool erase = FALSE;
	uByte tmp[6];
	static uInt lastoff = NVRAM_SIZE;

	if (nvram_offset(e, &off) != NO_ERROR)
		erase = TRUE;
	else if (off + len > NVRAM_SIZE)
		erase = TRUE;
	else if (nvram_read(e, off, tmp, 6) != NO_ERROR)
		erase = TRUE;
	else
	{
		if (lastoff != NVRAM_SIZE && lastoff != off)
			erase = TRUE;

		sz = (tmp[2] << 8) | tmp[3];
		off += sz + 7;

		if (off + len > NVRAM_SIZE)
			erase = TRUE;
	}

	if (erase)
	{
		uByte *b;

		off = 0;

		/* erase flash area */ 
		b = malloc(NVRAM_SIZE);

		if (b == NULL)
			return E_PROM_WRITE_ERROR;

		memset(b, 0xFF, NVRAM_SIZE);
		flash_op(e, NVRAM_BASE, b, NVRAM_SIZE, FALSE);
		free(b);
	}

	DPRINTF(("nvram_write: lastoff %d, off %d, len %d\n", lastoff, off, len));
	lastoff = off;
	return flash_op(e, NVRAM_BASE + off, buf, len, FALSE);
}

Retcode
machine_nvram_read(Environ *e, uByte *buf, uInt *len)
{
	int x;
	uInt l = *len;
	uByte tmp[2];
	uInt off;
	Retcode ret;

	DPRINTF(("nvram_read: e %p, buf %p, len = %p, *len = %d\n", e, buf, len, l));

	ret = nvram_offset(e, &off);

	if (ret != NO_ERROR)
		return ret;

	ret = nvram_read(e, off + 2, tmp, 2);
	DPRINTF(("nvram_read: read: ret = %d\n", ret));

	if (ret != NO_ERROR)
		return E_PROM_READ_ERROR;

	x = (tmp[0] << 8) | tmp[1];
	DPRINTF(("nvram_read: %02X %02X\n", tmp[0], tmp[1]));

	if (x < 0 || x > NVRAM_SIZE - 7)
		return E_PROM_READ_ERROR;

	if (l > NVRAM_SIZE)
		l = NVRAM_SIZE;

	if (l < x + 7)
		x = l - 7;

	DPRINTF(("nvram_read: x = %d\n", x));

	ret = nvram_read(e, off, buf, x + 7);

	x = (buf[2] << 8) | buf[3];
	x += 7;

	if (buf[0] != 0xBE || buf[1] != 0xEF || x >= NVRAM_SIZE)
	{
		*len = 0;
		return E_PROM_READ_ERROR;
	}

	*len = x;
	DPRINTF(("nvram_read: *len = %d\n", x));
	return NO_ERROR;
}
