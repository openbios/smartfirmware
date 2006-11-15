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

/* (c) Copyright 1996 by CodeGen, Inc.  All Rights Reserved. */

#include <stdio.h>
#if defined __MWERKS__ && defined macintosh
#  include <unix.h>
#else
#  include <fcntl.h>
#  include <unistd.h>
#endif
#include "defs.h"

C(f_stdio_open)
{
#if defined STANDALONE && defined O_NONBLOCK
	if (isatty(fileno(stdin)))
	    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
#endif
	IFCKSP(e, 0, 1);
	PUSH(e, FTRUE);
	e->lines = 24;
	e->cols = 80;
	return NO_ERROR;
}

C(f_stdio_close)
{
#if defined STANDALONE && defined O_NONBLOCK
	if (isatty(fileno(stdin)))
	    fcntl(0, F_SETFL, fcntl(0, F_GETFL) & ~O_NONBLOCK);
#endif
	return NO_ERROR;
}

C(f_stdio_read)
{
	Int len, actual;
	Byte *addr;
	
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	fflush(stdout);
	actual = fread(addr, sizeof (Byte), len, stdin);
	PUSH(e, actual == 0 && !feof(stdin) ? -2 : actual);
	return NO_ERROR;
}

C(f_stdio_write)
{
	Int len;
	Byte *addr;
	
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	PUSH(e, fwrite(addr, sizeof (Byte), len, stdout));
	return NO_ERROR;
}

C(f_stdio_install_abort)
{
	return NO_ERROR;
}

C(f_stdio_remove_abort)
{
	return NO_ERROR;
}

C(f_stdio_is_tty)
{
	IFCKSP(e, 0, 1);
	PUSH(e, isatty(fileno(stdin)) ? FTRUE : FFALSE);
	return NO_ERROR;
}

static const Initentry stdio_methods[] =
{
	{ "open",          f_stdio_open,          INVALID_FCODE },
	{ "close",         f_stdio_close,         INVALID_FCODE },
	{ "read",          f_stdio_read,          INVALID_FCODE },
	{ "write",         f_stdio_write,         INVALID_FCODE },
	{ "install-abort", f_stdio_install_abort, INVALID_FCODE },
	{ "remove-abort",  f_stdio_remove_abort,  INVALID_FCODE },
	{ "is-tty",        f_stdio_is_tty,        INVALID_FCODE },
	{ NULL,            NULL },
};

CC(install_stdio)
{
	Package *pkg = new_pkg_name(e->root, "stdio");
	
	prop_set_str(pkg->props, "device_type", CSTR, "serial", CSTR);
	return init_entries(e, pkg->dict, stdio_methods);
}
