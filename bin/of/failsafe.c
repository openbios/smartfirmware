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

/* (c) Copyright 1996,1998 by CodeGen, Inc.  All Rights Reserved. */

/* Use failsafe_{read,write}() to do console I/O - for debugging. */

#ifdef STANDALONE
#include <stdio.h>
#if defined __MWERKS__ && defined macintosh
#  include <unix.h>
#else
#  include <fcntl.h>
#  include <unistd.h>
#endif
#endif

#include "defs.h"

C(f_failsafe_open)
{
	IFCKSP(e, 0, 1);
	PUSH(e, FTRUE);
	e->lines = 24;
	e->cols = 80;
	return NO_ERROR;
}

C(f_failsafe_close)
{
	return NO_ERROR;
}

C(f_failsafe_read)
{
	Int len;
	Int actual = 0;
	Byte *addr;
	
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	actual = failsafe_read(addr, len);
	PUSH(e, (actual == 0) ? -2 : actual);
	return NO_ERROR;
}

C(f_failsafe_write)
{
	Int len;
	Byte *addr;
	
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	len = failsafe_write(addr, len);
	PUSH(e, len);
	return NO_ERROR;
}

C(f_failsafe_install_abort)
{
	return NO_ERROR;
}

C(f_failsafe_remove_abort)
{
	return NO_ERROR;
}

#ifdef STANDALONE
C(f_failsafe_is_tty)
{
	IFCKSP(e, 0, 1);
	PUSH(e, (isatty(fileno(stdin)) && isatty(fileno(stdout))) ? FTRUE : FFALSE);
	DPRINTF(("failsafe_is_tty: %Cd\n", TOP(e)));
	return NO_ERROR;
}
#endif

static const Initentry failsafe_methods[] =
{
	{ "open",          f_failsafe_open,          INVALID_FCODE },
	{ "close",         f_failsafe_close,         INVALID_FCODE },
	{ "read",          f_failsafe_read,          INVALID_FCODE },
	{ "write",         f_failsafe_write,         INVALID_FCODE },
	{ "install-abort", f_failsafe_install_abort, INVALID_FCODE },
	{ "remove-abort",  f_failsafe_remove_abort,  INVALID_FCODE },
#ifdef STANDALONE
	{ "is-tty",        f_failsafe_is_tty,        INVALID_FCODE },
#endif
	{ NULL,            NULL },
};

CC(install_failsafe)
{
	Package *pkg = new_pkg_name(e->root, "failsafe");
	
	prop_set_str(pkg->props, "device_type", CSTR, "serial", CSTR);
	return init_entries(e, pkg->dict, failsafe_methods);
}
