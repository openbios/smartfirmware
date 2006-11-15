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

// Copyright (c) 1991-2002 by Parag Patel.  All Rights Reserved.

#ifndef __DEFS_H_
#define __DEFS_H_

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef NEED_MALLOC_H
#include <malloc.h>
#endif
#include <ctype.h>
#include <string.h>

#include <stdlibx.h>
#include <stringx.h>
#include <pool.h>
#include <dynarray.h>
#include <hashtable.h>
#include <bitset.h>

enum
{
    TERMINAL 	= 1,
    NONTERMINAL	= 2,
    CODE		= 3,
    ZEROPLUS	= 4,
    ONEPLUS		= 5,
    ONEZERO		= 6,

    END			= 0,
    EMPTY		= 1,
    START		= 2
};

#define EOI		0

#define RET_NONE	0x00
#define RET_VALUE	0x01
#define RET_CODE	0x02

class Symbol;
class Symnode;
class Resynclist;

class Set
{
public:
    Bitset *set;
    int id;

    Set() { set = NULL; id = 0; }
    Set(Bitset *s) { set = s; id = 0; }
    operator Bitset*() { return set; }
    boolean operator==(Set &s) { return *set == *s.set; }
    boolean operator==(Bitset *b) { return *set == *b; }
};

class Symbol
{
public:
    const char *name;
    int id;
    int parserid;
    int type;

    const char *rettype;
    union
    {
		const char *lexstr;
		const char *realname;
    };
    union
    {
		const char *code;
		Symbol *symcode;
    };
	Symbol *endcode;

    union
    {
		int usedret;
		boolean lexdef;
		int line;
    };

    int usecount;
    int toempty : 1;
    int exportsym : 1;
    int mkstruct : 1;
    int doresync : 1;
    int doinline : 1;

    Bitset *first;
    Bitset *follow;
    union
    {
		Set *resync;
		Resynclist *list;
    };

    Symnode *node;

    //Symbol();
    Symbol(const char *n);
    operator const char*() { return name; }
    boolean operator==(Symbol &s) { return strcmp(name, s.name) == 0; }
    boolean operator==(const char *k) { return strcmp(name, k) == 0; }
};

class Symnode
{
public:
    Symbol *sym;
    const char *alias;
    Symnode *next;
    Symnode *orelse;
    union
    {
		Set *resync;
		Resynclist *list;
    };

    Symnode();
};

class Resynclist
{
public:
    const char *name;
    char first;
    char follow;
    char paren;
    Resynclist *next;

    Resynclist(const char *);
};

declare_hashtable(Settab, Setiter, Set, Bitset*);
extern Settab settab;

extern boolean optimize;
extern boolean dumpcode;
extern boolean dumponlyparser;
extern boolean statonly;
extern boolean docompare;
extern boolean casesensitive;
extern boolean dargstyle;
extern boolean genlineinfo;
extern boolean doresync;
extern boolean mactabs;
extern const char *lang;
extern const char *headername;
extern const char *scannername;
extern const char *parsername;
extern int getoptions(int argc, const char *argv[]);
extern FILE *makefile(const char *fname);
extern void cmpandmv(const char *file);
extern boolean exportedname;

extern int numsymbols(void);
extern Symbol *getsymbol(int);
extern int numterms(void);
extern int numnonterms(void);
extern Symbol *getterm(int);
extern Symbol *getnonterm(int);
extern Symbol *addsymbol(const char *);
extern void addterm(Symbol *);
extern void addnonterm(Symbol *);
extern Symbol *findsymbol(const char *);
extern void initsym(void);
extern Symbol *startsymbol;
extern Symbol *startcode;

extern boolean gotlexstr;
extern void readccomment(void);
extern const char *readtype(void);
extern const char *readcode(int &);
extern const char *readblockcode(int &);
extern const char *getword(void);

extern void buildsets(void);
extern void check(void);
extern void gencode(const char *infile);

extern void quit(const char *fmt, ...);
extern int error(const char *fmt, ...);
extern void dumpset(Bitset &set);
extern void dump(void);

#endif // __DEFS_H_
