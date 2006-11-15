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

// Copyright (c) 2001 by Parag Patel.  All Rights Reserved.

#ifndef __GENMODULA3_H_
#define __GENMODULA3_H_

#include "defs.h"
#include "bitset.h"
#include "gen.h"		// base-class

// Code generator for Modula-3.
//
class GenModula3 : public Gencode
{
	const char *inputfile;
	const char *tokenclass;
	const char *parserclass;

	inline const char *
	mkretcode(boolean saveret)
	{
		return saveret ? "w_rc = " : "";
	}

	const char *mkname(Symbol *sym);
	const char *mkiname(Symbol *sym);
	const char *mkerrname(Symbol *sym);

	void printset(const char *name, Bitset &set);
	void printcase(int indent, Bitset &set);

	inline boolean
	dotype(Symbol *sym)
	{
		if ((sym->usedret & RET_VALUE) || sym->rettype != NULL)
			return TRUE;
		else
			return FALSE;
	}

	const char *mktype(Symbol *sym,
			const char *pre = "", const char *post = "");
	const char * mkvoidtype(Symbol *sym,
			const char *pre = "", const char *post = "");

	void mkstruct(Symbol *sym);
	void mkcode(int indent, Symbol *code);
	const char *mkvarname(Symnode *start, Symnode *node);

	void printtestexpr(int indent, Bitset &set, const char *post);
	void printwhile(int indent, Bitset &set, const char *post);
	void printif(int indent, Bitset &set, const char *post);

	void loopbegin(int &indent, int doloop, Symbol *sym);
	void loopend(int &indent, int doloop, Symbol *sym);
	void genstmt(int indent, Symnode *node, boolean saveret = FALSE);

	void genheader(void);
	void genscanner(const char *header);
	void dumpexport(Symbol *sym);
	void genparser(const char *header);

public:

	GenModula3();
	virtual ~GenModula3();

	virtual void gencode(const char *infile, const char *headername,
			const char *parsername, const char *scannername);
};

#endif // __GENMODULA3_H_
