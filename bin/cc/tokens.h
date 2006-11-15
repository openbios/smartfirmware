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
#define INTVAL 257
#define UINTVAL 258
#define LONGVAL 259
#define ULONGVAL 260
#define CHARACTER 261
#define WCHARACTER 262
#define FLOATVAL 263
#define DOUBLEVAL 264
#define LONGDOUBLEVAL 265
#define IDENTIFIER 266
#define THIS 267
#define COLONCOLON 268
#define SHORTDOUBLEVAL 269
#define LONGLONGVAL 270
#define ULONGLONGVAL 271
#define CLASSNAME 272
#define TYPENAME 273
#define STRING 274
#define WSTRING 275
#define ARROW 276
#define INCR 277
#define DECR 278
#define SIZEOF 279
#define X_TYPENAME 280
#define NEW 281
#define DELETE 282
#define OPERATOR 283
#define X_TILDE 284
#define LSHIFT 285
#define RSHIFT 286
#define LESSEQ 287
#define GREATEQ 288
#define EQUAL 289
#define NOTEQ 290
#define ANDAND 291
#define OROR 292
#define MULTEQ 293
#define DIVEQ 294
#define MODEQ 295
#define PLUSEQ 296
#define MINUSEQ 297
#define LSHIFTEQ 298
#define RSHIFTEQ 299
#define ANDEQ 300
#define XOREQ 301
#define OREQ 302
#define EXTERN_C 303
#define X_CTORINIT 304
#define INLINE 305
#define FRIEND 306
#define VIRTUAL 307
#define TYPEDEF 308
#define EXTERN 309
#define STATIC 310
#define AUTO 311
#define REGISTER 312
#define PASCAL 313
#define FORTRAN 314
#define VOID 315
#define CHAR 316
#define SHORT 317
#define INT 318
#define LONG 319
#define FLOAT 320
#define DOUBLE 321
#define SIGNED 322
#define UNSIGNED 323
#define STRUCT 324
#define UNION 325
#define CLASS 326
#define PRIVATE 327
#define PUBLIC 328
#define PROTECTED 329
#define ENUM 330
#define CONST 331
#define VOLATILE 332
#define X_CTOR 333
#define X_MEMFUNC 334
#define X_DTOR 335
#define DOTDOTDOT 336
#define X_DECLARATION 337
#define X_LABEL 338
#define CASE 339
#define DEFAULT 340
#define IF 341
#define ELSE 342
#define SWITCH 343
#define WHILE 344
#define DO 345
#define FOR 346
#define X_CLASSNAME 347
#define GOTO 348
#define CONTINUE 349
#define BREAK 350
#define RETURN 351
#define ASM 352
#define W_LAST_TOKEN 353

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
extern int translation_unit(void);
#else
extern int translation_unit();
#endif
#ifdef __cplusplus
}
#endif


#endif /* __TOKENS_H_ */
