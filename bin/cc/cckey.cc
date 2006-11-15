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

#include "globals.h"
#include "cc.h"
#include "tokens.h"
#include <pool.h>

implement_hashtable(C_keyword_tab, C_keyword_iter, C_keyword, const char*, ptrhash);
implement_hashtableiter(C_keyword_tab, C_keyword_iter, C_keyword, const char*, ptrhash);

C_keyword_tab g_keywords;

#ifdef COLONCOLON
static void
add_cxx_keywords()
{
	g_keywords[strget("asm")] = ASM;
	g_keywords[strget("class")] = CLASS;
	g_keywords[strget("new")] = NEW;
	g_keywords[strget("delete")] = DELETE;
	g_keywords[strget("friend")] = FRIEND;
	g_keywords[strget("virtual")] = VIRTUAL;
	g_keywords[strget("inline")] = INLINE;
	g_keywords[strget("this")] = THIS;
	g_keywords[strget("private")] = PRIVATE;
	g_keywords[strget("public")] = PUBLIC;
	g_keywords[strget("protected")] = PROTECTED;
	g_keywords[strget("operator")] = OPERATOR;
	g_keywords[strget("bool")] = INT;				// hack
	//g_keywords[strget("bool")] = BOOL;
	//g_keywords[strget("try")] = TRY;
	//g_keywords[strget("catch")] = CATCH;
	//g_keywords[strget("thrown")] = THROW;
	//g_keywords[strget("template")] = TEMPLATE;
}
#endif

void
init_keywords()
{
	g_keywords.clear();
	g_keywords[strget("sizeof")] = SIZEOF;
	g_keywords[strget("typedef")] = TYPEDEF;
	g_keywords[strget("extern")] = EXTERN;
	g_keywords[strget("static")] = STATIC;
	g_keywords[strget("auto")] = AUTO;
	g_keywords[strget("register")] = REGISTER;
	g_keywords[strget("void")] = VOID;
	g_keywords[strget("char")] = CHAR;
	g_keywords[strget("short")] = SHORT;
	g_keywords[strget("int")] = INT;
	g_keywords[strget("long")] = LONG;
	g_keywords[strget("float")] = FLOAT;
	g_keywords[strget("double")] = DOUBLE;
	g_keywords[strget("signed")] = SIGNED;
	g_keywords[strget("unsigned")] = UNSIGNED;
	g_keywords[strget("struct")] = STRUCT;
	g_keywords[strget("union")] = UNION;
	g_keywords[strget("enum")] = ENUM;
	g_keywords[strget("const")] = CONST;
	g_keywords[strget("volatile")] = VOLATILE;
	g_keywords[strget("case")] = CASE;
	g_keywords[strget("default")] = DEFAULT;
	g_keywords[strget("if")] = IF;
	g_keywords[strget("else")] = ELSE;
	g_keywords[strget("switch")] = SWITCH;
	g_keywords[strget("while")] = WHILE;
	g_keywords[strget("do")] = DO;
	g_keywords[strget("for")] = FOR;
	g_keywords[strget("goto")] = GOTO;
	g_keywords[strget("continue")] = CONTINUE;
	g_keywords[strget("break")] = BREAK;
	g_keywords[strget("return")] = RETURN;

	g_keywords[strget("__asm__")] = ASM;
	g_keywords[strget("__pascal__")] = PASCAL;
	g_keywords[strget("__fortran__")] = FORTRAN;
	g_keywords[strget("__inline__")] = INLINE;

	if (!g_strict_iso)
	{
		g_keywords[strget("asm")] = ASM;
		g_keywords[strget("fortran")] = FORTRAN;
		g_keywords[strget("inline")] = INLINE;
	}

	if (g_macintosh)
		g_keywords[strget("pascal")] = PASCAL;

// for C++ keywords
#ifdef COLONCOLON
	if (g_cplusplus)
		add_cxx_keywords();
#endif
}
