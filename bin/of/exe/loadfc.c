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

/* (c) Copyright 1997-1998 by CodeGen, Inc.  All Rights Reserved. */

/* Forth and Fcode program loaders.
 */

#include "defs.h"
#include "exe.h"

Bool
forth_is_exec(Environ *e, uByte *load, uInt loadlen)
{
	if (loadlen > 2 && load[0] == '\\' && load[1] == ' ')
		return TRUE;

	return FALSE;
}

/* run the Forth image - loading makes no sense for Forth
 */
Retcode
forth_load(Environ *e, uByte *load, uInt loadlen, uLong *entrypoint)
{
	*entrypoint = -1;
	return interp_text(e, (Byte*)load, (Int)loadlen);
}

uInt 
forth_length(Environ *e, uByte *load, uInt loadlen)
{
	/* no way to know the length of arbitrary text so return all ones */
	return ~0;
}

const Exec_entry exec_forth =
{
	"Forth",
	forth_is_exec,
	forth_load,
	forth_length
};

Bool
fcode_is_exec(Environ *e, uByte *load, uInt loadlen)
{
	DPRINTF(("fcode_is_exec: load=%p loadlen=%d\n", load, loadlen));
	#ifdef DEBUG_FCODE
	display_memory(e, load, loadlen);
	#endif

	/* make sure that we begin with the right magic numbers */
	if (loadlen <= 2 ||
			(load[0] != 0xF0 && load[0] != 0xF1 && load[0] != 0xF2 &&
					load[0] != 0xF3 && load[0] != 0xFD) ||
			(load[1] != 0x08 && load[1] != 3))
		return FALSE;

	DPRINTF(("fcode_is_exec: good!\n"));
	return TRUE;
}

/* run the Fcode image - loading makes no sense for Fcode
 */
Retcode
fcode_load(Environ *e, uByte *load, uInt loadlen, uLong *entrypoint)
{
	*entrypoint = -1;
	DPRINTF(("fcode_load: load=%p\n", load));
	return interp_fcode(e, (Byte*)load, MAGIC_FNEXT);
}

/* return the expected length of the FCode image
 */
uInt
fcode_length(Environ *e, uByte *load, uInt loadlen)
{
	uInt len = 0;

	/* return a zero length if not enough has been downloaded yet */
	if (loadlen < 8)
		len = 0;
	else
		/* extract the length from the header */
		len = (load[4] << (BYTE_SIZE * 3)) |
				(load[5] << (BYTE_SIZE * 2)) |
				(load[6] << BYTE_SIZE) |
				load[7];
	DPRINTF(("fcode_length: len=%d loadlen=%d\n", len, loadlen));
	return len;
}

const Exec_entry exec_fcode =
{
	"Fcode",
	fcode_is_exec,
	fcode_load,
	fcode_length
};
