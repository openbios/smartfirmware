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

/* This initializes the /chosen node. */

/*#define DEBUG*/

#include "defs.h"

C(f_chosen_open)			/* open (-- okay?) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, FTRUE);
	return NO_ERROR;
}

C(f_chosen_close)			/* close (--) */
{
	return NO_ERROR;
}


static const Initentry chosen_methods[] =
{
	{ "open",          f_chosen_open,           INVALID_FCODE },
	{ "close",         f_chosen_close,          INVALID_FCODE },
	{ NULL,            NULL },
};

/* initialize and setup the "/chosen" package, for lack of a
   better place to put it
 */
CC(install_chosen)
{
	Package *pkg = new_pkg_name(e->root, "chosen");
	Retcode ret;
	
	e->chosen = pkg;
	DPRINTF(("install_chosen: init_entries...\n"));
	ret = init_entries(e, pkg->dict, chosen_methods);
	
	if (ret == NO_ERROR)
		ret = prop_set_ptr(pkg->props, "stdin", CSTR, 0);

	if (ret == NO_ERROR)
		ret = prop_set_ptr(pkg->props, "stdout", CSTR, 0);

	if (ret == NO_ERROR)
		ret = prop_set_str(pkg->props, "bootpath", CSTR, "", CSTR);	

	if (ret == NO_ERROR)
		ret = prop_set_str(pkg->props, "bootargs", CSTR, "", CSTR);	

	if (ret == NO_ERROR)
		ret = prop_set_ptr(pkg->props, "memory", CSTR, 0);

	if (ret == NO_ERROR)
		ret = prop_set_ptr(pkg->props, "mmu", CSTR, 0);

	DPRINTF(("install_chosen: done\n"));
	return ret;
}
