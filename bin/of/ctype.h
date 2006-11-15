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

/* (c) Copyright 1996-1997 by CodeGen, Inc.  All Rights Reserved. */

/* simple ASCII standalone replacement for standard <ctype.h> */
/* - watch out for macro side-effects */

#ifndef __CTYPE_H_
#define	__CTYPE_H_

#define	isdigit(c)	((c) >= '0' && (c) <= '9')
#define	islower(c)	((c) >= 'a' && (c) <= 'z')
#define	isupper(c)	((c) >= 'A' && (c) <= 'Z')
#define	isalpha(c)	(isupper(c) || islower(c))
#define	isprint(c)	((c) >= ' ' && (c) <= '~')
#define	isspace(c)	((c) == '\t' || (c) == '\n' || (c) == '\v' || \
					(c) == '\f' || (c) == '\r' || (c) == ' ')
#define	isxdigit(c)	(isdigit(c) || ((c) >= 'a' && (c) <= 'f') || \
					((c) >= 'A' && (c) <= 'F'))
#define	tolower(c)	(isupper(c) ? (c) - 'A' + 'a' : (c))
#define	toupper(c)	(islower(c) ? (c) - 'a' + 'A' : (c))
#define isblank(c)	((c) == ' ' || (c) == '\t')

#endif /* __CTYPE_H_ */
