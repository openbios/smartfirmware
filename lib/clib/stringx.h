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

#ifndef __STRINGX_H_
#define __STRINGX_H_

#include <stdlibx.h>
#include <string.h>
/*#include <malloc.h>*/

extern const char **strsplit(const char *str, const char *sep, boolean whsp,
	int *argc);
extern const char *strjoin(const char **vec, const char *sep);

extern unsigned long strhash(const char *str);
extern unsigned long strhashi(const char *str);

extern const char *strbldf(const char *fmt, ...);
extern const char *strbld(const char *arg1, ...);

extern char *strapp(char *s1, const char *s2);
extern char *strnapp(char *s1, const char *s2, int n);

extern int strcmpi(const char *s1, const char *s2);

#define strfree(str)	((str) == NULL ? 0 : (free((void*)(str)), 0))
#define strnew(len)	((char*)malloc((len) + 1))
#define strend(str)	((str) + strlen(str))
#define strdup(str)	(((str) == NULL) ? NULL : \
				strcpy((char*)malloc(strlen(str) + 1), (str)))
#define streq(s1, s2)	(strcmp(s1, s2) == 0)
#define streqi(s1, s2)	(strcmpi(s1, s2) == 0)

extern char const *strget(char const *str);
extern char const *strgeti(char const *str);
extern void strdrop(char const *str);
extern void strdropi(char const *str);
extern void clear_string_pool(void);

#define strput strdrop
#define strputi strdropi

#endif /*__STRINGX_H_*/
