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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "except.h"

struct __exception *__curr_exception = NULL;

void
__except_init(struct __exception *self)
{
	self->raised = FALSE;
	self->accepted = FALSE;
	self->val = NULL;
	self->prev = __curr_exception;
	self->arg = 0;
	__curr_exception = self;
}

void
__except_destroy(struct __exception *self)
{
	__curr_exception = self->prev;

	if (self->raised && !self->accepted)
		__except_raise(self->val, self->arg);
}

boolean
__except_accept(struct __exception *self, Exception v)
{
	__curr_exception = self;

	if (self->val != v)
		return FALSE;

	return self->accepted = TRUE;
}

boolean
__except_recover(struct __exception *self)
{
	__curr_exception = self;
	return self->accepted = TRUE;
}

#ifdef NEED_CRASH
static void
crash(const char *err)
{
	write(2, err, strlen(err));
	write(2, "\r\n", 2);
}
#endif

void
__except_raise(Exception v, void *a)
{
	if (__curr_exception != NULL)
	{
		__curr_exception->raised = TRUE;
		__curr_exception->val = v;
		__curr_exception->arg = a;
		longjmp(__curr_exception->jmpbuf, 42);
	}

	if (v == NULL)
		crash("caught NULL exception");
	else
		crash(v);

	abort();
}
