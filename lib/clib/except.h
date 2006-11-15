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

#ifndef __EXCEPT_H_
#define __EXCEPT_H_

#include <stddef.h>
#include <setjmp.h>
#include <stdlibx.h>

typedef const char *Exception;

struct __exception
{
	boolean raised, accepted;
	struct __exception *prev;
	Exception val;
	jmp_buf jmpbuf;
	void *arg;
};

extern struct __exception *__curr_exception;
extern void __except_init(struct __exception *self);
extern void __except_destroy(struct __exception *self);
extern boolean __except_accept(struct __exception *self, Exception v);
extern boolean __except_recover(struct __exception *self);
extern void __except_raise(Exception val, void *arg);

#define TRY	\
	{	\
	struct __exception __new_exception;	\
	__except_init(&__new_exception);	\
	if (setjmp(__new_exception.jmpbuf) == 0)

#define HANDLE(exception)	\
	else if (__except_accept(&__new_exception, exception))

#define RECOVER	\
	else if (__except_recover(&__new_exception))

#define ENDTRY	\
	__except_destroy(&__new_exception);	\
	}

#define RAISE(e)	__except_raise((e), 0)
#define RAISE_ARG(e,a)	__except_raise((e), (void*)(a))

#define EXCEPTION	(__curr_exception ? __curr_exception->val : NULL)
#define EXCEPTION_ARG	(__curr_exception ? __curr_exception->arg : NULL)


#endif /* __EXCEPT_H_ */
