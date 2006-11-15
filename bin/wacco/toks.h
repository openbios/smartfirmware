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

#ifndef __TOKENS_H_
#define __TOKENS_H_

#ifndef EOF
#include <stdio.h>
#endif
#ifndef NULL
#include <stddef.h>
#endif

#define W_RETOK 1
#define W_RETERR 0

#define W_EOI 0
#define W_EMPTY 256
#define DIRECTIVE 257
#define ID 258
#define EXPORT 259
#define NORESYNC 260
#define INLINE 261
#define NOINLINE 262
#define EXITCODE 263
#define NULLSYM 264
#define STRING 265
#define CHARACTER 266
#define INT 267
#define BLOCKCODE 268
#define W_LAST_TOKEN 269

#ifdef __cplusplus
extern "C" {
#endif
#if defined(__cplusplus) || defined(__STDC__)
extern void w_closefile(void);
extern int w_openfile(char *);
extern FILE *w_getfile(void);
extern void w_setfile(FILE *);
extern int w_currcol(void);
extern int w_currline(void);
extern char *w_getcurrline(void);
extern int w_input(void);
extern int w_unput(int);
extern void w_output(int);
extern void w_scanerr(const char *fmt, ...);
#else /* K&R C */
extern w_closefile();
extern int w_openfile();
extern FILE *w_getfile();
extern w_setfile();
extern int w_currcol();
extern int w_currline();
extern char *w_getcurrline();
extern int w_input();
extern int w_unput();
extern w_output();
extern w_scanerr();
#endif /* K&R C */
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
#if defined(__cplusplus) || defined(__STDC__)
extern void w_err_expected(const char *token);
extern void w_err_illegal(const char *rule);
extern void w_err_skip(void);
extern void w_flusherrs(void);
extern int w_gettoken(void);
extern int w_nexttoken(void);
extern void w_skiptoken(void);
extern const char *w_tokenname(int);
#else /* K&R C */
extern w_err_expected();
extern w_err_illegal();
extern w_err_skip();
extern w_flusherrs();
extern int w_gettoken();
extern int w_nexttoken();
extern w_skiptoken();
extern char *w_tokenname();
#endif /* K&R C */
#ifdef __cplusplus
}
#endif

extern int w_numerrors;
extern int w_currtoken;

#ifdef __cplusplus
extern "C" {
#endif
#if defined(__cplusplus) || defined(__STDC__)
extern int program(void);
#else
extern int program();
#endif
#ifdef __cplusplus
}
#endif


#endif /* __TOKENS_H_ */
