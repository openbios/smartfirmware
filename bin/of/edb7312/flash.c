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

/* EDB7312 flash device driver */

/*#define DEBUG*/

#include "defs.h"
#include "flash.h"

static int flash_probed = 0;

#ifdef DEBUG
#define LOG_SIZE	4096
Bool readops[LOG_SIZE];
uInt offsets[LOG_SIZE];
uShort values[LOG_SIZE];
int logsize = 0;

static void
add_log(uInt offset, uShort val, Bool readop)
{
	if (logsize >= LOG_SIZE)
		return;

	readops[logsize] = readop;
	offsets[logsize] = offset;
	values[logsize] = val;
	logsize++;
}

static unsigned short
inh(unsigned int addr)
{
	unsigned int t;
	__asm("	ldrh	%0, [%1, #0]" : "=r"(t) : "r"(addr));
	return t;
}

static void
outh(unsigned int addr, unsigned short val)
{
	__asm("	strh	%0, [%1, #0]" : : "r"(val), "r"(addr));
}
#endif

static int
flash_read(void *private, void *va, uInt offset, uByte *data)
{
	uDoublet v;
	uDoublet *x;
	x = (uDoublet *)((Byte *)va + offset);
	__asm("	ldrh	%0, [%1, #0]" : "=r"(v) : "r"(x));
#ifdef DEBUG
	add_log((uInt)x, (uShort)v, 1);
#endif
#ifdef DEBUG2
	DPRINTF(("flash_read: private %p, va %p, offset %#x, data %p, v %#x\n", private, va, offset, data, v));
#endif
	data[1]  = (v >> 8) & 0xFF;
	data[0]  = v & 0xFF;
	return 1;
}

static int
flash_write(void *private, void *va, uInt offset, uByte *data)
{
	uDoublet v;
	uDoublet *x;
	x = (uDoublet *)((Byte *)va + offset);
	v =  (data[1] << 8) | data[0];
#ifdef DEBUG2
	DPRINTF(("flash_write: private %p, va %p, offset %#x, data %p, x %p, v %#x\n", private, va, offset, data, x, v));
#endif
	__asm("	strh	%0, [%1, #0]" : : "r"(v), "r"(x));
#ifdef DEBUG
	add_log((uInt)x, (uShort)v, 0);
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
	PUSH(e, "/flash@0");
	PUSH(e, 8);
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

unsigned int
checksum(unsigned char *buf, int len)
{
	unsigned int sum;
	int i;

	sum = 0;

	for (i = 0; i < len; i += 4)
		sum += *(unsigned int *)(buf + i);

	return -sum;
}

Bool
verify_image(uByte *image)
{
	int len;
	unsigned int chksum;
	unsigned int crc1;
	unsigned int crc2;

	len = (image[387] << 24) | (image[386] << 16) |
			(image[385] << 8) | image[384];

	if (len < 1024 || len > 0x100000)
		return FALSE;

	chksum = checksum((unsigned char *)image, len);

	if (chksum != 0)
		return FALSE;

	crc1 = crc32((unsigned char *)image, len);
	crc2 = (image[len + 3] << 24) | (image[len + 2] << 16) |
			(image[len + 1] << 8) | image[len + 0];

	if (crc1 != crc2)
		return FALSE;

	return TRUE;
}

C(do_selftest)
{
	Retcode ret;
	Cell virt;
	Bool ok;
	Bool diag = diagnostic_mode(e);

	IFCKSP(e, 0, 2);
	PUSH(e, 0);
	PUSH(e, 0x100000);
	ret = execute_static_method_name(e, e->root, "map-in", CSTR);

	if (ret != NO_ERROR)
		return ret;

	IFCKSP(e, 1, 0);
	POP(e, virt);

	ok = verify_image((uByte *)virt);

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

C(f_flash_update)
{
	Retcode ret;
	Bool diag = diagnostic_mode(e);
	uByte *buf = (uByte *)e->load;
	uInt buflen = (uInt)e->loadlen;
	int i;

	if (buflen < 128 ||
			buf[0] != 0x38 || buf[1] != 0xF0 ||
			buf[2] != 0x9F || buf[3] != 0xE5 ||
			buf[4] != 0x38 || buf[5] != 0xF0 ||
			buf[6] != 0x9F || buf[7] != 0xE5 ||
			buf[8] != 0x38 || buf[9] != 0xF0 ||
			buf[10] != 0x9F || buf[11] != 0xE5 ||
			buf[12] != 0x38 || buf[13] != 0xF0 ||
			buf[14] != 0x9F || buf[15] != 0xE5)
	{
		/* verify that load area has a valid image */
		cprintf(e, "%02X %02X %02X %02X\n", buf[0], buf[1], buf[2], buf[3]);
		cprintf(e, "%02X %02X %02X %02X\n", buf[4], buf[5], buf[6], buf[7]);
		cprintf(e, "%02X %02X %02X %02X\n", buf[8], buf[9], buf[10], buf[11]);
		cprintf(e, "%02X %02X %02X %02X\n", buf[12], buf[13], buf[14], buf[15]);
		cprintf(e, "Unrecognized load image format: flash unchanged\n");
		return E_BAD_IMAGE;
	}

	if (!verify_image((uByte *)buf))
	{
		cprintf(e, "Load image checksum failed: flash unchanged\n");
		return E_BAD_IMAGE;
	}

	if (diag)
		cprintf(e, "Image base address %#x, size %dK\n", buf, buflen / 1024);

	/* write flash */
	ret = flash_op(e, 0, buf, buflen, FALSE);

	/* verify flash */
	ret = flash_op(e, 0, buf + buflen, buflen, TRUE);

	for (i = 0; i < buflen; i++)
		if (buf[i] != buf[buflen + i])
			break;

	if (i < buflen)
	{
		cprintf(e, "Verify failed at offset %d (%#x)\n", i, i);
		return E_PROM_WRITE_ERROR;
	}

	return ret;
}

C(f_fs_update)
{
	/* verify that the load area has valid filesystem image */

	/* write flash */
	return flash_op(e, 0x100000, e->load, e->loadlen, FALSE);
}

#ifdef DEBUG
C(f_program_word)		/* (val offset -- ok ) */
{
	Cell offset;
	Cell val;
	unsigned short t;
	unsigned short stats[256];
	int nstats;
	int i;
	
	IFCKSP(e, 2, 1);
	POP(e, offset);
	POP(e, val);

	offset &= ~1;		/* clear LSB */
	outh(offset, 0x40);	/* write program command */
	outh(offset, val);	/* write data */
	t = inh(offset);
	stats[0] = t;
	nstats = 1;

	for (i = 0; i < 1000000; i++)
	{
		if (t & 0x80)
			break;

		t = inh(offset);

		if (nstats < 256 && t != stats[nstats - 1])
		{
			stats[nstats] = t;
			nstats++;
		}
	}

	outh(offset, 0xFFFF);	/* reset */
	t = inh(offset);	/* verify data */

	DPRINTF(("program_word: final status %#x, iterations %d\n", stats[nstats - 1], i));

	for (i = 0; i < nstats; i++)
	{
		DPRINTF(("program_word: status[%d] = %#x\n", i, stats[i]));
	}

	if (t != val)
	{
		DPRINTF(("program_word: verify failed, wrote %#x, read %#x\n", val, t));
	}

	if (i < 1000000 && t == val)
		PUSH(e, FTRUE);
	else
		PUSH(e, FFALSE);

	return NO_ERROR;
}

C(f_show_flash_log)		/* ( -- ) */
{
	int i;

	for (i = 0; i < logsize; i++)
		cprintf(e, "[%d] %s %#x %s %#x\n", i, readops[i] ? " read" : "write", offsets[i], readops[i] ? "-->" : "<--", values[i]);

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
	{ "fs-update",     f_fs_update,           INVALID_FCODE },

#ifdef DEBUG
	{ "program-word",  f_program_word,        INVALID_FCODE },
	{ "show-flash-log",f_show_flash_log,      INVALID_FCODE },
#endif

	{ NULL, NULL }
};

static Flash_methods edb7312_flash =
{
	(void *)0,		/* private info */
	2,			/* 16-bits wide */
	flash_read,
	flash_write,
	flash_mapaddr,
	flash_mapdata,
	0x100000,		/* reserve the first one-meg for bootcode */
	0x1000000,		/* 16MB flash address window */
	0x1000000,		/* 16MB same as above */
	{ 0 },			/* phys address == 0 */
	1,			/* number of words in phys address */
};

CC(install_flash)
{
	Retcode ret;
	DPRINTF(("install_flash: begin\n"));

	flash_probed = 1;
	ret = generic_install_flash(e, &edb7312_flash);

	DPRINTF(("install_flash: ret = %d\n", ret));

	if (ret == NO_ERROR)
		ret = init_entries(e, e->names, flash_words);

	DPRINTF(("install_flash: init_entries ret = %d\n", ret));

	if (ret == NO_ERROR)
	{
		Instance *inst;

		PUSH(e, "/flash@0");
		PUSH(e, 8);
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
	}

	return ret;
}

#define NVRAM_SIZE	EDB7312_SZ_NVRAM
#define NVRAM_BASE	0xE0000

uInt
machine_nvram_size(Environ *e)
{
	return NVRAM_SIZE;
}

static Retcode
nvram_read(Environ *e, uInt offset, uByte *buf, uInt len)
{
	uByte *nvram = (uByte *)EDB7312_VA_NVRAM;
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

		if (ret != NO_ERROR)
			return ret;

		sz = (tmp[2] << 8) | tmp[3];

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

	DPRINTF((e, "nvram_write: lastoff %d, off %d, len %d\n", lastoff, off, len));
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
	static Bool ok = FALSE;
	extern uInt g_boot_switches;

	DPRINTF(("nvram_read: e %p, buf %p, len = %p, *len = %d\n", e, buf, len, l));

	if (!ok && (g_boot_switches & 0x10))
	{
		ok = TRUE;
		return E_PROM_READ_ERROR;
	}

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
