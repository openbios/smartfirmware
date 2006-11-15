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

/* (c) Copyright 1998 by CodeGen, Inc.  All Rights Reserved. */

/* Gzipped image (un)loader.  The idea is to ungzip a gzipped image
   using the zlib package, copy that image back to the load addr,
   then re-execute the generic exe code again.
   zlib's deflate and gzip are documented in NIC's rfc1951 and rfc1952.
 */

#include "defs.h"
#include "exe.h"

/* macros to eliminate typedef conflicts with zlib.h */
#define Byte zByte
#define uInt zuInt
#define uLong zuLong
#include "zlib.h"
#undef Byte
#undef uInt
#undef uLong

#define GZIP_MAGIC_0	0x1F
#define GZIP_MAGIC_1	0x8B

/* gzip flag byte */
#define FTEXT		0x01	/* file probably ascii text */
#define FHCRC		0x02	/* header CRC present */
#define FEXTRA		0x04	/* extra field present */
#define FNAME		0x08	/* name present */
#define FCOMMENT	0x10	/* comment present */
#define FRESERVED	0xE0	/* reserved */


/* This is like the "uncompress" routine in the zlib package, but it
   passes the undocumented -MAX_WBITS so that it performs a gunzip.
   Otherwise zlib will not decompress a gzipped binary but only an image
   compressed using zlib.
 */
static int
ungzip(Bytef *dest, uLongf *destLen, const Bytef *source, zuLong sourceLen)
{
	z_stream stream;
	int err;

	memset(&stream, 0, sizeof stream);
	stream.next_in = (Bytef *)source;
	stream.avail_in = (uInt)sourceLen;
	stream.next_out = dest;
	stream.avail_out = (uInt)*destLen;

	/* this bit is magic to do proper gunzipping */
	err = inflateInit2(&stream, -MAX_WBITS);

	if (err != Z_OK)
		return err;

	err = inflate(&stream, Z_FINISH);

	if (err != Z_STREAM_END)
	{
		inflateEnd(&stream);
		return err == Z_OK ? Z_BUF_ERROR : err;
	}

	*destLen = stream.total_out;

	err = inflateEnd(&stream);
	return err;
}


Bool
gzip_is_exec(Environ *e, uByte *load, uInt loadlen)
{
	if (loadlen > 3 &&
			load[0] == GZIP_MAGIC_0 &&
			load[1] == GZIP_MAGIC_1 &&
			load[2] == Z_DEFLATED)
		return TRUE;

	return FALSE;
}

/* uncompress the image at load, then copy it back to load, then
   try the generic loader again
 */
Retcode
gzip_load(Environ *e, uByte *load, uInt loadlen, uLong *entrypoint)
{
	int ret, flags, len;
	uLongf destlen = loadlen * 10;

	if (!gzip_is_exec(e, load, loadlen))		/* sanity check */
		return E_BAD_IMAGE;

	*entrypoint = -1;

	/* get the gzip header flags and skip past the header */
	flags = load[3];
	load += 10;
	loadlen -= 10;

	if (flags & FEXTRA)
	{
		len = *load++;
		len += *load++ << 8;
		loadlen -= 2;

		while (len-- && loadlen > 0)
			load++, loadlen--;
	}

	if (flags & FNAME)
	{
		while (*load && loadlen > 0)
			load++, loadlen--;

		load++;
		loadlen--;
	}

	if (flags & FCOMMENT)
	{
		while (*load && loadlen > 0)
			load++, loadlen--;

		load++;
		loadlen--;
	}

	if (flags & FHCRC)
	{
		load += 2;
		loadlen -= 2;
	}

	DPRINTF(("gzip_load: flags=%x load=%p loadlen=%d\n",
			flags, load, loadlen));

	if (diagnostic_mode(e))
		cprintf(e, "inflating image from %d bytes to ", loadlen);

	/* uncompress this image right after itself - assume we have
	   plenty of space for the decompressed image */
	ret = ungzip((Bytef*)load + loadlen, &destlen, (Bytef*)load, loadlen);

	switch (ret)
	{
	case Z_OK:
		if (diagnostic_mode(e))
			cprintf(e, "%d bytes\n", destlen);

		break;

	case Z_MEM_ERROR:
		cprintf(e, "zlib: not enough memory\n");
		return E_OUT_OF_MEMORY;

	case Z_BUF_ERROR:
		cprintf(e, "zlib: output buffer too small\n");
		return E_OUT_OF_MEMORY;

	case Z_DATA_ERROR:
		cprintf(e, "zlib: input appears to be corrupt or not gzipped\n");
		return E_BAD_IMAGE;

	default:
		cprintf(e, "zlib: unknown error %d\n", ret);
		return E_ABORT;
	}

	/* good - copy the image back up to the load addr */
	memmove(e->load, load + loadlen, destlen);
	e->loadlen = destlen;

	/* then try to load the image again */
	if (!exec_is_exec(e))
		return E_BAD_IMAGE;

	exec_free_symbols(e, e->loadsyms);
	e->loadsyms = exec_load_symbols(e);
	return exec_load(e);
}

uInt 
gzip_length(Environ *e, uByte *load, uInt loadlen)
{
	/* no way to know the length of compressed image so return all ones */
	return ~0;
}

const Exec_entry exec_gzip =
{
	"Gzip",
	gzip_is_exec,
	gzip_load,
	gzip_length
};
