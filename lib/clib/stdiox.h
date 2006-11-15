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

/* Copyright (c) 1992,1997 by Parag Patel and Thomas J. Merritt.
   All Rights Reserved. */

#ifndef __STDIOX_H_
#define __STDIOX_H_

#include <stdarg.h>
#include <stdlibx.h>

#ifdef macintosh
#define PATH_SEP ':'
#define PATH_LIST_SEP ';'
#define CURRENT_DIR ":"
#endif

#ifdef MSDOS
# ifndef _DJGPP_SOURCE
#  define PATH_SEP '\\'
# endif
#define PATH_LIST_SEP ';'
#endif

#ifndef PATH_SEP
#define PATH_SEP '/'
#endif
#ifndef PATH_LIST_SEP
#define PATH_LIST_SEP ':'
#endif
#ifndef CURRENT_DIR
#define CURRENT_DIR "."
#endif

extern char *xgets(FILE *fp, boolean nl);
extern FILE *fileopen(char const *dir, char const *file, char const *mode);
extern FILE *pathopen(char *path, char const *file, char const *mode);
extern FILE *pathvopen(char const *pathvar, char const *file, char const *mode);
extern char *xvsprintf(char **buf, long *buflen, const char *fmt, va_list ap);
extern char const *xsprintf(const char *fmt, ...);
extern char const *dir_name(char const *name);
extern char const *base_name(char const *name);
extern boolean filelength(const char *name, size_t *size);

extern void set_debug_fp(FILE *fp);			/* use fp instead of stderr */
extern void dprintf(const char *fmt, ...);	/* for debug output */

/* define DEBUG before including this header file to use this macro */
#ifdef DEBUG
#define DPRINTF(args) dprintf args
#else
#define DPRINTF(args)
#endif

#endif /* __STDIOX_H_ */
