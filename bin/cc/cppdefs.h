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

// Copyright (c) 1992 by Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/bin/cc/cppdefs.h,v 1.21 1999/03/15 21:48:55 parag Exp $
#ifndef __CPPDEFS_H_
#define __CPPDEFS_H_

// common header files and other useful stuff for this package

#include <stddef.h>
#include <stdlib.h>
#include <dynarray.h>
#include <stdlibx.h>
#include <stringx.h>

enum Cpptok
{
	UNKNOWN = -1,
	NONE = 0,				// must be zero
	END = 0,
	// must be greater than any token in tokens.h and still fit in a short:
	WHITESPACE = 1000,
	NEWLINE,
	HASHHASH,
	INCLUDE,
	ARGUMENT,
	EXPANDED_IDENT,
	CPP_LAST_WITHOUT_COMMA
};

#include "globals.h"
#include "error.h"
#include "cc.h"
#ifndef IN_YACC_FILE
#  include "tokens.h"
#endif

// Mac filesystem swaps these two values and so do some Mac compilers
#if defined(__MWERKS__) && defined(macintosh)
#   define CR '\n'
#   define NL '\r'
#else
#   define CR '\r'
#   define NL '\n'
#endif

#define isidchar(c) (isalpha(c) || c == '_' || c == '$')
#define isidchar2(c) (isalnum(c) || c == '_' || c == '$')
#define isodigit(c) (c >= '0' && c <= '7')
#define iswhsp(c) (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == CR)

declare_array(Charvec, const char*, 1024);

#endif // __CPPDEFS_H_
