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

/* (c) Copyright 1997 by CodeGen, Inc.  All Rights Reserved. */

/* Generic program loader to support plug-in binary image loaders. */

#ifndef __EXE_H__
#define __EXE_H__

#define SYM_TEXT	1
#define SYM_DATA	2

struct sym_ent
{
	Byte *name;		/* name of symbol entry */
	uByte type;		/* type - either text or data for now */
	uLong addr;		/* address of symbol */
};
typedef struct sym_ent Sym_ent;

struct sym_table
{
	Sym_ent *list;		/* unordered */
	uInt num;
};
typedef struct sym_table Sym_table;

struct exec_entry
{
	const char *name;
	Bool (*is_exec)(Environ *e, uByte *load, uInt len);
	Retcode (*load)(Environ *e, uByte *load, uInt len, uLong *entry);
	uInt (*length)(Environ *e, uByte *load, uInt loadlen);
	Sym_table *(*symbols)(Environ *e, uByte *load, uInt len);
};
typedef struct exec_entry Exec_entry;


/* list of supported binary formats - defined in machdep.c */
extern Exec_entry *g_exec_list[];


/* routines in exe.c */
extern Bool exec_is_exec(Environ *e);
extern Retcode exec_load(Environ *e);
extern Bool exec_length(Environ *e, uInt *len);
extern Sym_table *exec_load_symbols(Environ *e);
void exec_free_symbols(Environ *e, Sym_table *tab);
extern Sym_ent *exec_sym2addr(Environ *e, Sym_table *tab, Byte *sym, Int slen);
extern Sym_ent *exec_addr2sym(Environ *e, Sym_table *tab, uLong addr);


#endif /* __EXE_H__ */
