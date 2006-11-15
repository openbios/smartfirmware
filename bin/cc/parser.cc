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

#line 3 "ccgram.w"

// Copyright (c) 1993,2000-2001 by Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/bin/cc/ccgram.w,v 1.37 2001/01/21 18:29:31 parag Exp $
#include "cppdefs.h"
#include "cpp.h"
#include "cc.h"
#include "cctype.h"
#include "ccsym.h"
#include "ccnode.h"
#include "globals.h"
#include <stdiox.h>
#include <stdarg.h>
#ifdef macintosh
#  include <Types.h>
#  include <CursorCtl.h>
#endif

struct Struct_decl
{
	Storage_thing *stor;
	Struct_type *type;
	int access;
};

Scope *g_curr_scope = NULL;
Scope *g_file_scope = NULL;
Scope *g_func_scope = NULL;
Node_arr *g_curr_statlist = NULL;
int g_loop_count = 0;
int g_default_linkage = NONE;

static Switch_node *g_curr_switch = NULL;
static Node *g_curr_loop = NULL;
static Node *g_curr_break = NULL;
static boolean g_old_function = FALSE;
static Cpp_node *g_tok = NULL;
static Symbol *g_sym = NULL;
static Cpp_node g_end;
static boolean g_parse_type = TRUE;		// TRUE if we should parse a type

static Cpp_node *ltok = NULL;
static Cpp_node ptok;
static boolean ptokvalid = FALSE;
static Cpp_node_list lextokbuf;
static int lexloc = 0;

// real scanning routine
static int
gettok(boolean scanning = TRUE)
{
	if (scanning)
	{
		if (ptokvalid)		// pushed-back token
		{
			DPRINTF(("gettok(scanning): ptok=%d:%s g_tok=%d:%s\n",
					ptok.token, ptok.tok2str(),
					g_tok ? g_tok->token : -1,
					g_tok ? g_tok->tok2str() : ""));

			ptokvalid = FALSE;
			g_tok = &ptok;
			return ptok.token;
		}
		else if (ltok)		// lookahead token
		{
			DPRINTF(("gettok(scanning): ltok=%d:%s g_tok=%d:%s\n",
					ltok->token, ltok->tok2str(),
					g_tok ? g_tok->token : -1,
					g_tok ? g_tok->tok2str() : ""));
			g_tok = ltok;
			ltok = NULL;
			return g_tok->token;
		}
	}
	else if (ltok)
	{
		DPRINTF(("gettok(peeking): ltok=%d:%s g_tok=%d:%s\n",
				ltok->token, ltok->tok2str(),
				g_tok ? g_tok->token : -1,
				g_tok ? g_tok->tok2str() : ""));
		return ltok->token;
	}

	// get a line of preprocessed tokens from the scanner if needed
	if (lexloc >= lextokbuf.size())
	{
		lexloc = 0;
		lextokbuf.resize();

		if (g_debug)
			printf("\n");

		if (g_cpp.scan(&lextokbuf) == END)
		{
			g_tok = &g_end;
			return g_end.token = END;
		}
	}

	Cpp_node *tok = &lextokbuf.elt(lexloc++);

	DPRINTF(("gettok(%d): tok=%d:%s g_tok=%d:%s\n",
			scanning, tok->token, tok->tok2str(),
			g_tok ? g_tok->token : -1,
			g_tok ? g_tok->tok2str() : ""));

	if (g_debug)
		printf(" %s", tok->tok2str());

	// only identifiers need to be looked for in various hashtables
	if (tok->token == IDENTIFIER)
	{
		if (g_keywords.find(tok->text))
			tok->token = g_keywords().token();
		else
		{
			Symbol *sym = g_curr_scope->lookup_sym(tok->text);

			// this is to allow redefining type names within other scopes
			if (sym &&
					(g_parse_type ||
							(g_cplusplus && gettok(FALSE) == COLONCOLON)) &&
					sym->storage() == TYPEDEF)
			{
				if (g_cplusplus && sym->type()->isclass())
					tok->token = CLASSNAME;
				else
					tok->token = TYPENAME;

				g_sym = sym;
			}
		}
	}

	if (scanning)
		g_tok = tok;
	else
		ltok = tok;

	DPRINTF(("gettok(%d) return: tok=%d:%s ltok=%d:%s g_tok=%d:%s\n",
			scanning, tok->token, tok->tok2str(),
			ltok ? ltok->token : -1, 
			ltok ? ltok->tok2str() : "",
			g_tok ? g_tok->token : -1,
			g_tok ? g_tok->tok2str() : ""));

#ifdef macintosh
	SpinCursor(1);
#endif
	return tok->token;
}

inline int
w_looknext(void)
{
	return gettok(FALSE);
}

static void
w_pushback(int tok, const char *str = NULL)
{
	DPRINTF(("pushback %d (%s)\n", tok, str ? str : "<null>"));

	if (ptokvalid)
		bug("w_pushback(): already pushed back one token");

	ptok.text = (str == NULL) ? g_tok->text : str;
	ptok.token = tok;
	ptokvalid = TRUE;
}

inline int w_gettoken(void) { return gettok(); }

void
w_err_expected(const char *token)
{
	w_numerrors++;
	error(WACCO_EXPECTED, token);
}

void
w_err_illegal(const char *token)
{
	w_numerrors++;

	// HACK - should be error() here but things seem to get out-of-hand
	// and dump core, so just quit if we ever hit one of these
	fatal(WACCO_ILLEGAL, token);
}

inline void w_err_skip(void) { }
inline void w_flusherrs(void) { }

static int peek_op_prec();
static Type *do_point_to(Storage_thing *point, Storage_thing *to);

inline Type*
point_to(Storage_thing *point, Storage_thing *to)
{
	return point == NULL ? to->type :
			(to == NULL ? point->type : do_point_to(point, to));
}


#line 210 "parser.cc"

extern YYTEXT_DECL;
int w_numerrors = 0;
int w_currtoken = -1;

struct w_resynclink
{
	struct w_resynclink *prev;
	int *resync;
};

struct W_Toperator_func
{
	const char *name; Type *type;
};

struct W_Texpr
{
	Expr_node *e; int stop;
};

struct W_Toperator
{
	int op; int prec; boolean left;
};

struct W_Tarray
{
	Type *type; boolean param;
};

#if defined(__cplusplus) || defined(__STDC__)
#ifdef __cplusplus
extern "C" {
#endif
static int w_fidentifier(struct w_resynclink *, int *, const char* *);
static int w_fstring_literal(struct w_resynclink *, int *, String_node* *);
static int w_fmember_name(struct w_resynclink *, int *, const char* *);
static int w_fargument_expr_list(struct w_resynclink *, int *, Expr_arr* *);
static int w_funary_expr(struct w_resynclink *, int *, Expr_node* *);
static int w_fnew_type_name(struct w_resynclink *, int *, Type* *);
static int w_foperator_func(struct w_resynclink *, int *, struct W_Toperator_func *);
static int w_fcast_expr(struct w_resynclink *, int *, Expr_node* *);
static int w_fexpr(struct w_resynclink *, int *, struct W_Texpr *);
static int w_foperator(struct w_resynclink *, int *, struct W_Toperator *);
static int w_fexpression(struct w_resynclink *, int *, Expr_node* *);
static int w_fassignment_expr(struct w_resynclink *, int *, Expr_node* *);
static int w_fconstant_expr(struct w_resynclink *, int *, Expr_node* *);
static int w_fdeclaration(struct w_resynclink *, int *);
static int w_fdeclaration_specifiers(struct w_resynclink *, int *, Storage_thing *);
static int w_fctor_initializer(struct w_resynclink *, int *);
static int w_fmem_initializer(struct w_resynclink *, int *);
static int w_finit_declarator(struct w_resynclink *, int *, Storage_thing *);
static int w_ftype_specifier(struct w_resynclink *, int *, Storage_thing* *);
static int w_fbase_specifier(struct w_resynclink *, int *, Struct_type* *);
static int w_fstruct_declaration_list(struct w_resynclink *, int *, struct Struct_decl *);
static int w_faccess_specifier(struct w_resynclink *, int *, int *);
static int w_fstruct_declaration(struct w_resynclink *, int *, struct Struct_decl* *);
static int w_fbit_field(struct w_resynclink *, int *, short *);
static int w_fspecifier_qualifier_list(struct w_resynclink *, int *, Storage_thing *);
static int w_fstruct_declarator(struct w_resynclink *, int *, struct Struct_decl *);
static int w_fenumerator_list(struct w_resynclink *, int *, Enum_type* *);
static int w_fenumerator(struct w_resynclink *, int *, Enum_type* *);
static int w_ftype_qualifier(struct w_resynclink *, int *, int *);
static int w_fdeclarator(struct w_resynclink *, int *, Storage_thing* *);
static int w_fpointer(struct w_resynclink *, int *, Storage_thing* *);
static int w_farray(struct w_resynclink *, int *, struct W_Tarray *);
static int w_fparameter_type_list(struct w_resynclink *, int *, Type* *);
static int w_fparameter_declaration(struct w_resynclink *, int *, Param_arr* *);
static int w_fparam_declarator(struct w_resynclink *, int *, Storage_thing* *);
static int w_ftype_name(struct w_resynclink *, int *, Type* *);
static int w_fabstract_declarator(struct w_resynclink *, int *, Storage_thing* *);
static int w_finitializer(struct w_resynclink *, int *, Expr_node* *);
static int w_finitializer_list(struct w_resynclink *, int *, Initializer_node* *);
static int w_fstatement(struct w_resynclink *, int *, Node* *);
static int w_fdeclaration_list(struct w_resynclink *, int *, boolean *);
static int w_fstatement_list(struct w_resynclink *, int *, Node_arr* *);
static int w_fmark_expr(struct w_resynclink *, int *, Expr_node* *);
static int w_fopt_expr(struct w_resynclink *, int *, Expr_node* *);
static int w_fasm_statement(struct w_resynclink *, int *, Node* *);
static int w_ftranslation_unit(struct w_resynclink *, int *);
#ifdef __cplusplus
}
#endif
#else /* K&R */
static int w_fidentifier();
static int w_fstring_literal();
static int w_fmember_name();
static int w_fargument_expr_list();
static int w_funary_expr();
static int w_fnew_type_name();
static int w_foperator_func();
static int w_fcast_expr();
static int w_fexpr();
static int w_foperator();
static int w_fexpression();
static int w_fassignment_expr();
static int w_fconstant_expr();
static int w_fdeclaration();
static int w_fdeclaration_specifiers();
static int w_fctor_initializer();
static int w_fmem_initializer();
static int w_finit_declarator();
static int w_ftype_specifier();
static int w_fbase_specifier();
static int w_fstruct_declaration_list();
static int w_faccess_specifier();
static int w_fstruct_declaration();
static int w_fbit_field();
static int w_fspecifier_qualifier_list();
static int w_fstruct_declarator();
static int w_fenumerator_list();
static int w_fenumerator();
static int w_ftype_qualifier();
static int w_fdeclarator();
static int w_fpointer();
static int w_farray();
static int w_fparameter_type_list();
static int w_fparameter_declaration();
static int w_fparam_declarator();
static int w_ftype_name();
static int w_fabstract_declarator();
static int w_finitializer();
static int w_finitializer_list();
static int w_fstatement();
static int w_fdeclaration_list();
static int w_fstatement_list();
static int w_fmark_expr();
static int w_fopt_expr();
static int w_fasm_statement();
static int w_ftranslation_unit();
#endif /* K&R */

static char *w_toknams[] =
{
	"[] <empty>",
	"INTVAL",
	"UINTVAL",
	"LONGVAL",
	"ULONGVAL",
	"CHARACTER",
	"WCHARACTER",
	"FLOATVAL",
	"DOUBLEVAL",
	"LONGDOUBLEVAL",
	"IDENTIFIER",
	"THIS",
	"COLONCOLON",
	"SHORTDOUBLEVAL",
	"LONGLONGVAL",
	"ULONGLONGVAL",
	"CLASSNAME",
	"TYPENAME",
	"STRING",
	"WSTRING",
	"ARROW",
	"INCR",
	"DECR",
	"SIZEOF",
	"X_TYPENAME",
	"NEW",
	"DELETE",
	"OPERATOR",
	"X_TILDE",
	"LSHIFT",
	"RSHIFT",
	"LESSEQ",
	"GREATEQ",
	"EQUAL",
	"NOTEQ",
	"ANDAND",
	"OROR",
	"MULTEQ",
	"DIVEQ",
	"MODEQ",
	"PLUSEQ",
	"MINUSEQ",
	"LSHIFTEQ",
	"RSHIFTEQ",
	"ANDEQ",
	"XOREQ",
	"OREQ",
	"EXTERN_C",
	"X_CTORINIT",
	"INLINE",
	"FRIEND",
	"VIRTUAL",
	"TYPEDEF",
	"EXTERN",
	"STATIC",
	"AUTO",
	"REGISTER",
	"PASCAL",
	"FORTRAN",
	"VOID",
	"CHAR",
	"SHORT",
	"INT",
	"LONG",
	"FLOAT",
	"DOUBLE",
	"SIGNED",
	"UNSIGNED",
	"STRUCT",
	"UNION",
	"CLASS",
	"PRIVATE",
	"PUBLIC",
	"PROTECTED",
	"ENUM",
	"CONST",
	"VOLATILE",
	"X_CTOR",
	"X_MEMFUNC",
	"X_DTOR",
	"DOTDOTDOT",
	"X_DECLARATION",
	"X_LABEL",
	"CASE",
	"DEFAULT",
	"IF",
	"ELSE",
	"SWITCH",
	"WHILE",
	"DO",
	"FOR",
	"X_CLASSNAME",
	"GOTO",
	"CONTINUE",
	"BREAK",
	"RETURN",
	"ASM",
};

#if defined(__cplusplus) || defined(__STDC__)
const char *
w_tokenname(int tok)
#else
char *
w_tokenname(tok)
int tok;
#endif
{
	static char buf[2];

	if (tok >= W_EMPTY)
		return w_toknams[tok - W_EMPTY];

	if (tok == W_EOI)
		return "EOI <end-of-input>";

	buf[0] = tok;
	buf[1] = '\0';
	return buf;
}

int
#if defined(__cplusplus) || defined(__STDC__)
w_nexttoken(void)
#else
w_nexttoken()
#endif
{
	return (w_currtoken < 0) ? (w_currtoken = w_gettoken()) : w_currtoken;
}

#define w_nexttoken() ((w_currtoken < 0) ? (w_currtoken = w_gettoken()) : w_currtoken)

#if defined(__cplusplus) || defined(__STDC__)
void
w_skiptoken(void)
#else
w_skiptoken()
#endif
{
	w_currtoken = -1;
}

#define w_skiptoken() (w_currtoken = -1)

static int
#if defined(__cplusplus) || defined(__STDC__)
w_scantoken(int expect, struct w_resynclink *lnk,int *resync)
#else
w_scantoken(expect, lnk, resync)
int expect;
struct w_resynclink *lnk;
int *resync;
#endif
{
	struct w_resynclink rlink;
	struct w_resynclink *link;
	int level = 1;

	rlink.prev = lnk;
	rlink.resync = resync;

	if (w_currtoken < 0)
		w_currtoken = w_gettoken();

	if (expect >= 0 && w_currtoken != expect)
		w_err_expected(w_tokenname(expect));

	while (w_currtoken != expect)
	{
		int i;
		int l = level;

		for (link = &rlink; link != NULL && l-- > 0; link = link->prev)
			for (i = 0; link->resync && link->resync[i] >= 0; i++)
				if (w_currtoken == link->resync[i])
					return -1;

		w_err_skip();
		w_currtoken = w_gettoken();

		if (w_currtoken == expect)
			break;

		level++;
	}
	
	w_currtoken = -1;
	return expect;
}

int
#if defined(__cplusplus) || defined(__STDC__)
translation_unit(void)
#else
translation_unit()
#endif
{
	int w_rval;
	int w_savnum = w_numerrors;
	struct w_resynclink w_link;
	static int w_follow[] = { W_EOI, -1 };

	w_link.prev = NULL;
	w_link.resync = NULL;
	w_numerrors = 0;
	w_rval = w_ftranslation_unit(&w_link, w_follow);

	switch (w_nexttoken())
	{
	case W_EOI:
		break;

	default:
		w_err_illegal("translation_unit");
		w_rval = W_RETERR;
		break;
	}

	if (w_numerrors > 0)
		w_rval = W_RETERR;

	w_numerrors += w_savnum;
	w_flusherrs();
	return w_rval;
}


static int w_resync1[] = { GOTO, CONTINUE, BREAK, RETURN, -1 };
static int w_resync2[] = { W_EMPTY, CLASSNAME, VIRTUAL, PRIVATE, PUBLIC, PROTECTED, -1 };
static int w_resync3[] = { '*', -1 };
static int w_resync4[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, '&', '*', OPERATOR, '{', '}', EXTERN_C, ';', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync5[] = { W_EMPTY, IDENTIFIER, CLASSNAME, TYPENAME, '{', ':', -1 };
static int w_resync6[] = { IDENTIFIER, CLASSNAME, TYPENAME, '{', ENUM, -1 };
static int w_resync7[] = { DOUBLE, -1 };
static int w_resync8[] = { '<', -1 };
static int w_resync9[] = { INLINE, -1 };
static int w_resync10[] = { '(', ')', STRING, WSTRING, ';', -1 };
static int w_resync11[] = { LONG, -1 };
static int w_resync12[] = { COLONCOLON, X_MEMFUNC, -1 };
static int w_resync13[] = { TYPEDEF, -1 };
static int w_resync14[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, ':', DOTDOTDOT, CASE, -1 };
static int w_resync15[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, ']', INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '=', -1 };
static int w_resync16[] = { WHILE, DO, FOR, -1 };
static int w_resync17[] = { STATIC, -1 };
static int w_resync18[] = { '(', ')', CLASSNAME, '~', TYPENAME, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync19[] = { '(', ')', CLASSNAME, '~', TYPENAME, X_TYPENAME, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync20[] = { W_EMPTY, '(', ')', '[', '&', '*', -1 };
static int w_resync21[] = { IDENTIFIER, CLASSNAME, '~', TYPENAME, '.', OPERATOR, -1 };
static int w_resync22[] = { W_EMPTY, IDENTIFIER, CLASSNAME, TYPENAME, '=', -1 };
static int w_resync23[] = { '-', -1 };
static int w_resync24[] = { CLASSNAME, '~', TYPENAME, ',', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, DOTDOTDOT, -1 };
static int w_resync25[] = { ')', CLASSNAME, '~', TYPENAME, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync26[] = { ANDAND, -1 };
static int w_resync27[] = { NEW, DELETE, -1 };
static int w_resync28[] = { REGISTER, -1 };
static int w_resync29[] = { W_EMPTY, CLASSNAME, '~', TYPENAME, '&', '*', VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync30[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', TYPENAME, STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', ';', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_DECLARATION, X_LABEL, CASE, DEFAULT, IF, ELSE, SWITCH, WHILE, DO, FOR, GOTO, CONTINUE, BREAK, RETURN, ASM, -1 };
static int w_resync31[] = { X_TILDE, -1 };
static int w_resync32[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, ',', '&', '*', OPERATOR, '{', ':', ';', X_CTORINIT, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync33[] = { CHAR, -1 };
static int w_resync34[] = { IDENTIFIER, COLONCOLON, '~', OPERATOR, -1 };
static int w_resync35[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, '&', '*', OPERATOR, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync36[] = { ')', ';', -1 };
static int w_resync37[] = { W_EMPTY, '(', ')', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, X_CTORINIT, -1 };
static int w_resync38[] = { '^', -1 };
static int w_resync39[] = { W_EMPTY, ':', -1 };
static int w_resync40[] = { W_EMPTY, ')', CONST, -1 };
static int w_resync41[] = { W_EMPTY, '(', ')', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, ';', -1 };
static int w_resync42[] = { '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, ',', '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, -1 };
static int w_resync43[] = { '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, ':', -1 };
static int w_resync44[] = { ',', '&', '*', '+', '-', '/', '%', LSHIFT, RSHIFT, '<', '>', LESSEQ, GREATEQ, EQUAL, NOTEQ, '^', '|', ANDAND, OROR, '?', '=', MULTEQ, DIVEQ, MODEQ, PLUSEQ, MINUSEQ, LSHIFTEQ, RSHIFTEQ, ANDEQ, XOREQ, OREQ, -1 };
static int w_resync45[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, '[', INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', '}', -1 };
static int w_resync46[] = { '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, '[', INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, -1 };
static int w_resync47[] = { IDENTIFIER, '~', OPERATOR, -1 };
static int w_resync48[] = { '?', -1 };
static int w_resync49[] = { '=', -1 };
static int w_resync50[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, ']', INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, -1 };
static int w_resync51[] = { '{', -1 };
static int w_resync52[] = { '%', -1 };
static int w_resync53[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, '[', '&', '*', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync54[] = { CLASSNAME, '~', -1 };
static int w_resync55[] = { DOUBLEVAL, -1 };
static int w_resync56[] = { MULTEQ, -1 };
static int w_resync57[] = { '>', -1 };
static int w_resync58[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', TYPENAME, STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, ';', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CLASSNAME, -1 };
static int w_resync59[] = { ',', -1 };
static int w_resync60[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, ',', '&', '*', OPERATOR, '{', ':', ';', X_CTORINIT, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync61[] = { ULONGVAL, -1 };
static int w_resync62[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, '[', OPERATOR, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync63[] = { CLASSNAME, '~', TYPENAME, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync64[] = { LONGLONGVAL, -1 };
static int w_resync65[] = { INCR, -1 };
static int w_resync66[] = { DIVEQ, -1 };
static int w_resync67[] = { W_EMPTY, '(', ')', IDENTIFIER, CLASSNAME, TYPENAME, '[', '&', '*', CONST, -1 };
static int w_resync68[] = { CONST, VOLATILE, -1 };
static int w_resync69[] = { '(', STRING, WSTRING, ASM, -1 };
static int w_resync70[] = { IDENTIFIER, CLASSNAME, TYPENAME, '{', '}', -1 };
static int w_resync71[] = { GREATEQ, -1 };
static int w_resync72[] = { IDENTIFIER, CLASSNAME, TYPENAME, ';', GOTO, -1 };
static int w_resync73[] = { '(', ')', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, -1 };
static int w_resync74[] = { ';', BREAK, -1 };
static int w_resync75[] = { FORTRAN, -1 };
static int w_resync76[] = { IDENTIFIER, CLASSNAME, ',', -1 };
static int w_resync77[] = { W_EMPTY, CONST, -1 };
static int w_resync78[] = { NEW, -1 };
static int w_resync79[] = { STRING, -1 };
static int w_resync80[] = { W_EMPTY, ':', X_CTORINIT, -1 };
static int w_resync81[] = { CLASSNAME, VIRTUAL, PRIVATE, PUBLIC, PROTECTED, -1 };
static int w_resync82[] = { SIGNED, -1 };
static int w_resync83[] = { '&', -1 };
static int w_resync84[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, '&', '*', OPERATOR, '{', ':', EXTERN_C, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync85[] = { CLASSNAME, '~', TYPENAME, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, DOTDOTDOT, -1 };
static int w_resync86[] = { W_EMPTY, IDENTIFIER, CLASSNAME, TYPENAME, '{', -1 };
static int w_resync87[] = { MODEQ, -1 };
static int w_resync88[] = { FRIEND, -1 };
static int w_resync89[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, '&', '*', OPERATOR, EXTERN_C, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, X_DECLARATION, -1 };
static int w_resync90[] = { ASM, -1 };
static int w_resync91[] = { UINTVAL, -1 };
static int w_resync92[] = { FLOAT, -1 };
static int w_resync93[] = { '(', ')', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, ';', -1 };
static int w_resync94[] = { INT, -1 };
static int w_resync95[] = { '+', -1 };
static int w_resync96[] = { W_EMPTY, ',', '&', '*', '+', '-', '/', '%', LSHIFT, RSHIFT, '<', '>', LESSEQ, GREATEQ, EQUAL, NOTEQ, '^', '|', ANDAND, OROR, '?', '=', MULTEQ, DIVEQ, MODEQ, PLUSEQ, MINUSEQ, LSHIFTEQ, RSHIFTEQ, ANDEQ, XOREQ, OREQ, -1 };
static int w_resync97[] = { OROR, -1 };
static int w_resync98[] = { EXTERN, -1 };
static int w_resync99[] = { TYPENAME, -1 };
static int w_resync100[] = { ')', -1 };
static int w_resync101[] = { ANDEQ, -1 };
static int w_resync102[] = { W_EMPTY, '(', ')', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', TYPENAME, STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', ';', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_DECLARATION, X_LABEL, CASE, DEFAULT, IF, ELSE, SWITCH, WHILE, DO, FOR, GOTO, CONTINUE, BREAK, RETURN, ASM, -1 };
static int w_resync103[] = { '~', -1 };
static int w_resync104[] = { CLASSNAME, '~', TYPENAME, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, -1 };
static int w_resync105[] = { W_EMPTY, ',', ';', -1 };
static int w_resync106[] = { LONGDOUBLEVAL, -1 };
static int w_resync107[] = { IDENTIFIER, COLONCOLON, CLASSNAME, '~', TYPENAME, OPERATOR, -1 };
static int w_resync108[] = { IDENTIFIER, COLONCOLON, CLASSNAME, '~', OPERATOR, -1 };
static int w_resync109[] = { '}', -1 };
static int w_resync110[] = { W_EMPTY, '=', -1 };
static int w_resync111[] = { AUTO, -1 };
static int w_resync112[] = { CLASSNAME, '~', TYPENAME, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync113[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, ',', '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '/', '%', LSHIFT, RSHIFT, '<', '>', LESSEQ, GREATEQ, EQUAL, NOTEQ, '^', '|', ANDAND, OROR, '?', '=', MULTEQ, DIVEQ, MODEQ, PLUSEQ, MINUSEQ, LSHIFTEQ, RSHIFTEQ, ANDEQ, XOREQ, OREQ, -1 };
static int w_resync114[] = { THIS, -1 };
static int w_resync115[] = { X_DTOR, -1 };
static int w_resync116[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, ',', '&', '*', OPERATOR, '{', ':', ';', X_CTORINIT, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, PRIVATE, PUBLIC, PROTECTED, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync117[] = { RSHIFT, -1 };
static int w_resync118[] = { W_EMPTY, '&', -1 };
static int w_resync119[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', TYPENAME, STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', ':', ';', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, DOTDOTDOT, X_DECLARATION, X_LABEL, CASE, DEFAULT, IF, SWITCH, WHILE, DO, FOR, GOTO, CONTINUE, BREAK, RETURN, ASM, -1 };
static int w_resync120[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', TYPENAME, STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', ':', ';', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_DECLARATION, X_LABEL, CASE, DEFAULT, IF, SWITCH, WHILE, DO, FOR, GOTO, CONTINUE, BREAK, RETURN, ASM, -1 };
static int w_resync121[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', TYPENAME, STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', ';', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_DECLARATION, X_LABEL, CASE, DEFAULT, IF, SWITCH, WHILE, DO, FOR, GOTO, CONTINUE, BREAK, RETURN, ASM, -1 };
static int w_resync122[] = { W_EMPTY, '(', '[', -1 };
static int w_resync123[] = { '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '=', -1 };
static int w_resync124[] = { ')', STRING, WSTRING, ';', -1 };
static int w_resync125[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', TYPENAME, STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, ';', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, FOR, X_CLASSNAME, -1 };
static int w_resync126[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, '[', ']', INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '=', -1 };
static int w_resync127[] = { IDENTIFIER, CLASSNAME, TYPENAME, ';', -1 };
static int w_resync128[] = { '|', -1 };
static int w_resync129[] = { CHARACTER, -1 };
static int w_resync130[] = { '(', ')', -1 };
static int w_resync131[] = { W_EMPTY, '(', '[', '&', '*', -1 };
static int w_resync132[] = { IDENTIFIER, CLASSNAME, '~', TYPENAME, OPERATOR, -1 };
static int w_resync133[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', TYPENAME, STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', EXTERN_C, ';', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, X_DECLARATION, X_LABEL, CASE, DEFAULT, IF, SWITCH, WHILE, DO, FOR, GOTO, CONTINUE, BREAK, RETURN, ASM, -1 };
static int w_resync134[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', TYPENAME, STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', EXTERN_C, ';', X_CTORINIT, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, X_DECLARATION, X_LABEL, CASE, DEFAULT, IF, SWITCH, WHILE, DO, FOR, GOTO, CONTINUE, BREAK, RETURN, ASM, -1 };
static int w_resync135[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', TYPENAME, STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', ':', EXTERN_C, ';', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, X_DECLARATION, X_LABEL, CASE, DEFAULT, IF, SWITCH, WHILE, DO, FOR, GOTO, CONTINUE, BREAK, RETURN, ASM, -1 };
static int w_resync136[] = { '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, NEW, DELETE, OPERATOR, -1 };
static int w_resync137[] = { '!', -1 };
static int w_resync138[] = { ULONGLONGVAL, -1 };
static int w_resync139[] = { NOTEQ, -1 };
static int w_resync140[] = { LSHIFTEQ, -1 };
static int w_resync141[] = { ':', -1 };
static int w_resync142[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, '&', '*', OPERATOR, EXTERN_C, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync143[] = { STRUCT, UNION, CLASS, -1 };
static int w_resync144[] = { PASCAL, -1 };
static int w_resync145[] = { '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, -1 };
static int w_resync146[] = { PLUSEQ, -1 };
static int w_resync147[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, '&', '*', OPERATOR, '=', X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync148[] = { '/', -1 };
static int w_resync149[] = { CLASS, -1 };
static int w_resync150[] = { W_EMPTY, '(', CLASSNAME, '~', TYPENAME, '[', X_TYPENAME, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync151[] = { W_EMPTY, '(', CLASSNAME, '~', TYPENAME, '[', X_TYPENAME, NEW, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync152[] = { DOTDOTDOT, -1 };
static int w_resync153[] = { '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, '[', ']', INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, -1 };
static int w_resync154[] = { W_EMPTY, '{', ':', -1 };
static int w_resync155[] = { SHORT, -1 };
static int w_resync156[] = { IDENTIFIER, COLONCOLON, -1 };
static int w_resync157[] = { X_CLASSNAME, -1 };
static int w_resync158[] = { STRUCT, -1 };
static int w_resync159[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, '&', '*', OPERATOR, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync160[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, OPERATOR, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync161[] = { IDENTIFIER, CLASSNAME, -1 };
static int w_resync162[] = { '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, ',', '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', -1 };
static int w_resync163[] = { PUBLIC, -1 };
static int w_resync164[] = { W_EMPTY, '(', ')', CLASSNAME, '~', TYPENAME, '{', VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync165[] = { '(', CLASSNAME, '~', TYPENAME, '[', ARROW, INCR, DECR, ',', '&', '*', '+', '-', '!', NEW, DELETE, X_TILDE, '/', '%', LSHIFT, RSHIFT, '<', '>', LESSEQ, GREATEQ, EQUAL, NOTEQ, '^', '|', ANDAND, OROR, '?', '=', MULTEQ, DIVEQ, MODEQ, PLUSEQ, MINUSEQ, LSHIFTEQ, RSHIFTEQ, ANDEQ, XOREQ, OREQ, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync166[] = { '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, WHILE, -1 };
static int w_resync167[] = { TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, -1 };
static int w_resync168[] = { XOREQ, -1 };
static int w_resync169[] = { W_EMPTY, CLASSNAME, VIRTUAL, -1 };
static int w_resync170[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, ',', '&', '*', OPERATOR, '{', ':', '=', EXTERN_C, ';', X_CTORINIT, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync171[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, '[', INCR, DECR, ',', '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', -1 };
static int w_resync172[] = { W_EMPTY, '(', ')', CLASSNAME, '~', TYPENAME, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync173[] = { W_EMPTY, '(', ')', IDENTIFIER, CLASSNAME, '~', TYPENAME, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync174[] = { VOLATILE, -1 };
static int w_resync175[] = { W_EMPTY, ']', '=', -1 };
static int w_resync176[] = { '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, ',', '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '/', '%', LSHIFT, RSHIFT, '<', '>', LESSEQ, GREATEQ, EQUAL, NOTEQ, '^', '|', ANDAND, OROR, '?', '=', MULTEQ, DIVEQ, MODEQ, PLUSEQ, MINUSEQ, LSHIFTEQ, RSHIFTEQ, ANDEQ, XOREQ, OREQ, -1 };
static int w_resync177[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, STRING, '&', '*', OPERATOR, '{', EXTERN_C, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync178[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, '&', '*', OPERATOR, '{', EXTERN_C, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync179[] = { ':', PRIVATE, PUBLIC, PROTECTED, -1 };
static int w_resync180[] = { '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', '=', -1 };
static int w_resync181[] = { LSHIFT, -1 };
static int w_resync182[] = { W_EMPTY, CLASSNAME, PRIVATE, PUBLIC, PROTECTED, -1 };
static int w_resync183[] = { ';', CONTINUE, -1 };
static int w_resync184[] = { '(', ')', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, IF, -1 };
static int w_resync185[] = { W_EMPTY, ')', CLASSNAME, '~', TYPENAME, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync186[] = { W_EMPTY, ')', IDENTIFIER, CLASSNAME, '~', TYPENAME, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync187[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, ';', RETURN, -1 };
static int w_resync188[] = { X_CTOR, -1 };
static int w_resync189[] = { FLOATVAL, -1 };
static int w_resync190[] = { LESSEQ, -1 };
static int w_resync191[] = { W_EMPTY, '(', ')', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, -1 };
static int w_resync192[] = { IDENTIFIER, ',', -1 };
static int w_resync193[] = { W_EMPTY, ')', '{', -1 };
static int w_resync194[] = { IDENTIFIER, CLASSNAME, ',', ':', -1 };
static int w_resync195[] = { LONGVAL, -1 };
static int w_resync196[] = { W_EMPTY, '}', ';', -1 };
static int w_resync197[] = { SHORTDOUBLEVAL, -1 };
static int w_resync198[] = { W_EMPTY, CONST, VOLATILE, -1 };
static int w_resync199[] = { W_EMPTY, '*', CONST, VOLATILE, -1 };
static int w_resync200[] = { STRING, WSTRING, -1 };
static int w_resync201[] = { WSTRING, -1 };
static int w_resync202[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, TYPENAME, '[', '&', '*', '=', -1 };
static int w_resync203[] = { IF, SWITCH, -1 };
static int w_resync204[] = { COLONCOLON, CLASSNAME, -1 };
static int w_resync205[] = { IDENTIFIER, CLASSNAME, TYPENAME, '{', -1 };
static int w_resync206[] = { '[', -1 };
static int w_resync207[] = { INTVAL, -1 };
static int w_resync208[] = { W_EMPTY, '(', ')', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', TYPENAME, STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', ';', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_DECLARATION, X_LABEL, CASE, DEFAULT, IF, SWITCH, WHILE, DO, FOR, GOTO, CONTINUE, BREAK, RETURN, ASM, -1 };
static int w_resync209[] = { IDENTIFIER, -1 };
static int w_resync210[] = { W_EMPTY, CLASSNAME, '~', TYPENAME, ';', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync211[] = { ']', -1 };
static int w_resync212[] = { '=', MULTEQ, DIVEQ, MODEQ, PLUSEQ, MINUSEQ, LSHIFTEQ, RSHIFTEQ, ANDEQ, XOREQ, OREQ, -1 };
static int w_resync213[] = { EQUAL, -1 };
static int w_resync214[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, ',', '&', '*', OPERATOR, ':', X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync215[] = { W_EMPTY, '&', '*', -1 };
static int w_resync216[] = { ENUM, -1 };
static int w_resync217[] = { '(', '[', '.', ARROW, INCR, DECR, -1 };
static int w_resync218[] = { RSHIFTEQ, -1 };
static int w_resync219[] = { W_EMPTY, '[', -1 };
static int w_resync220[] = { W_EMPTY, ',', ':', ';', -1 };
static int w_resync221[] = { '[', ']', -1 };
static int w_resync222[] = { ';', -1 };
static int w_resync223[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, ',', '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, ':', '/', '%', LSHIFT, RSHIFT, '<', '>', LESSEQ, GREATEQ, EQUAL, NOTEQ, '^', '|', ANDAND, OROR, '?', '=', MULTEQ, DIVEQ, MODEQ, PLUSEQ, MINUSEQ, LSHIFTEQ, RSHIFTEQ, ANDEQ, XOREQ, OREQ, -1 };
static int w_resync224[] = { DELETE, -1 };
static int w_resync225[] = { '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, STRING, WSTRING, '[', '.', ARROW, INCR, DECR, OPERATOR, -1 };
static int w_resync226[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, ';', -1 };
static int w_resync227[] = { MINUSEQ, -1 };
static int w_resync228[] = { ',', DOTDOTDOT, -1 };
static int w_resync229[] = { W_EMPTY, ',', '{', ':', ';', X_CTORINIT, -1 };
static int w_resync230[] = { WCHARACTER, -1 };
static int w_resync231[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, '&', '*', OPERATOR, EXTERN_C, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, X_CLASSNAME, -1 };
static int w_resync232[] = { '(', ')', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, WHILE, -1 };
static int w_resync233[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, '&', '*', OPERATOR, ':', X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync234[] = { IDENTIFIER, CLASSNAME, TYPENAME, '{', STRUCT, UNION, CLASS, -1 };
static int w_resync235[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, -1 };
static int w_resync236[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, ',', '&', '*', OPERATOR, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync237[] = { '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, ']', INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, -1 };
static int w_resync238[] = { COLONCOLON, -1 };
static int w_resync239[] = { IDENTIFIER, CLASSNAME, '~', TYPENAME, ARROW, OPERATOR, -1 };
static int w_resync240[] = { W_EMPTY, '{', -1 };
static int w_resync241[] = { DECR, -1 };
static int w_resync242[] = { W_EMPTY, '[', ']', -1 };
static int w_resync243[] = { IDENTIFIER, CLASSNAME, TYPENAME, ',', -1 };
static int w_resync244[] = { UNION, -1 };
static int w_resync245[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', TYPENAME, STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', '}', ';', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_DECLARATION, X_LABEL, CASE, DEFAULT, IF, SWITCH, WHILE, DO, FOR, GOTO, CONTINUE, BREAK, RETURN, ASM, -1 };
static int w_resync246[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, ',', '&', '*', OPERATOR, '{', '}', ':', ';', X_CTORINIT, INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, PRIVATE, PUBLIC, PROTECTED, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync247[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, '[', ']', INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, -1 };
static int w_resync248[] = { IDENTIFIER, CLASSNAME, TYPENAME, -1 };
static int w_resync249[] = { IDENTIFIER, CLASSNAME, TYPENAME, '}', -1 };
static int w_resync250[] = { CLASSNAME, -1 };
static int w_resync251[] = { PRIVATE, -1 };
static int w_resync252[] = { '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', -1 };
static int w_resync253[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, '&', '*', OPERATOR, '}', EXTERN_C, ';', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync254[] = { '(', ')', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, SWITCH, -1 };
static int w_resync255[] = { VOID, -1 };
static int w_resync256[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', TYPENAME, STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', '}', EXTERN_C, ';', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, X_DECLARATION, X_LABEL, CASE, DEFAULT, IF, SWITCH, WHILE, DO, FOR, GOTO, CONTINUE, BREAK, RETURN, ASM, -1 };
static int w_resync257[] = { PROTECTED, -1 };
static int w_resync258[] = { CONST, -1 };
static int w_resync259[] = { '(', CLASSNAME, '~', TYPENAME, '[', ARROW, INCR, DECR, ',', '&', '*', '+', '-', '!', NEW, DELETE, OPERATOR, X_TILDE, '/', '%', LSHIFT, RSHIFT, '<', '>', LESSEQ, GREATEQ, EQUAL, NOTEQ, '^', '|', ANDAND, OROR, '?', '=', MULTEQ, DIVEQ, MODEQ, PLUSEQ, MINUSEQ, LSHIFTEQ, RSHIFTEQ, ANDEQ, XOREQ, OREQ, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync260[] = { W_EMPTY, '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, '[', INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, '{', -1 };
static int w_resync261[] = { '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, '~', STRING, WSTRING, INCR, DECR, '&', '*', '+', '-', '!', SIZEOF, X_TYPENAME, NEW, DELETE, OPERATOR, DOTDOTDOT, -1 };
static int w_resync262[] = { W_EMPTY, ELSE, -1 };
static int w_resync263[] = { PRIVATE, PUBLIC, PROTECTED, -1 };
static int w_resync264[] = { W_EMPTY, '(', ')', IDENTIFIER, CLASSNAME, '~', TYPENAME, '&', '*', OPERATOR, X_CTOR, X_MEMFUNC, X_DTOR, -1 };
static int w_resync265[] = { W_EMPTY, '(', CLASSNAME, '~', TYPENAME, '[', '&', '*', VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };
static int w_resync266[] = { X_LABEL, CASE, DEFAULT, -1 };
static int w_resync267[] = { ARROW, -1 };
static int w_resync268[] = { W_EMPTY, ',', '=', ';', X_CTORINIT, -1 };
static int w_resync269[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, TYPENAME, '[', -1 };
static int w_resync270[] = { OPERATOR, -1 };
static int w_resync271[] = { OREQ, -1 };
static int w_resync272[] = { UNSIGNED, -1 };
static int w_resync273[] = { W_EMPTY, ';', -1 };
static int w_resync274[] = { '(', INTVAL, UINTVAL, LONGVAL, ULONGVAL, CHARACTER, WCHARACTER, FLOATVAL, DOUBLEVAL, LONGDOUBLEVAL, IDENTIFIER, THIS, COLONCOLON, SHORTDOUBLEVAL, LONGLONGVAL, ULONGLONGVAL, CLASSNAME, STRING, WSTRING, OPERATOR, -1 };
static int w_resync275[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, TYPENAME, '[', '&', '*', -1 };
static int w_resync276[] = { VIRTUAL, -1 };
static int w_resync277[] = { W_EMPTY, '(', IDENTIFIER, CLASSNAME, '~', TYPENAME, '&', '*', OPERATOR, EXTERN_C, ';', INLINE, FRIEND, VIRTUAL, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, PASCAL, FORTRAN, VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, X_CTOR, X_MEMFUNC, X_DTOR, ASM, -1 };
static int w_resync278[] = { CLASSNAME, ',', VIRTUAL, PRIVATE, PUBLIC, PROTECTED, -1 };
static int w_resync279[] = { CLASSNAME, ':', VIRTUAL, PRIVATE, PUBLIC, PROTECTED, -1 };
static int w_resync280[] = { W_EMPTY, ')', CLASSNAME, '~', TYPENAME, '{', VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, STRUCT, UNION, CLASS, ENUM, CONST, VOLATILE, -1 };

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fidentifier(struct w_resynclink *w_lnk, int *w_resync, const char* *w_rr)
#else
w_fidentifier(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
const char* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case IDENTIFIER:
		{
			w_scantoken(IDENTIFIER, &w_link, w_resync209);
#line 292 "ccgram.w"
			 (*w_rr) = g_tok->text; 
#line 877 "parser.cc"
		}

		break;

	case TYPENAME:
		{
			w_scantoken(TYPENAME, &w_link, w_resync99);
#line 293 "ccgram.w"
			 (*w_rr) = g_tok->text; 
#line 887 "parser.cc"
		}

		break;

	case CLASSNAME:
		{
			w_scantoken(CLASSNAME, &w_link, w_resync250);
#line 294 "ccgram.w"
			 (*w_rr) = g_tok->text; 
#line 897 "parser.cc"
		}

		break;

	default:
		w_err_illegal("identifier");
		return W_RETERR;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fstring_literal(struct w_resynclink *w_lnk, int *w_resync, String_node* *w_rr)
#else
w_fstring_literal(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
String_node* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case STRING:
		{
#line 298 "ccgram.w"
			 (*w_rr) = new String_node(g_curr_scope); 
#line 931 "parser.cc"

			{/*__P2*/
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync79;

				do
				{
					{
						w_scantoken(STRING, &w_link, w_resync79);
#line 299 "ccgram.w"
						 (*w_rr)->append(g_tok->text); 
#line 946 "parser.cc"
					}

				} while (w_nexttoken() == STRING);
			}/*__P2*/

#line 300 "ccgram.w"
			 (*w_rr)->finish(); 
#line 954 "parser.cc"
		}

		break;

	case WSTRING:
		{
#line 301 "ccgram.w"
			 (*w_rr) = new String_node(g_curr_scope, TRUE); 
#line 963 "parser.cc"

			{/*__P3*/
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync201;

				do
				{
					{
						w_scantoken(WSTRING, &w_link, w_resync201);
#line 302 "ccgram.w"
						 (*w_rr)->append(g_tok->text); 
#line 978 "parser.cc"
					}

				} while (w_nexttoken() == WSTRING);
			}/*__P3*/

#line 303 "ccgram.w"
			 (*w_rr)->finish(); 
#line 986 "parser.cc"
		}

		break;

	default:
		w_err_illegal("string_literal");
		return W_RETERR;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fmember_name(struct w_resynclink *w_lnk, int *w_resync, const char* *w_rr)
#else
w_fmember_name(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
const char* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case IDENTIFIER:
		{
			w_scantoken(IDENTIFIER, &w_link, w_resync209);
#line 328 "ccgram.w"
			 (*w_rr) = g_tok->text; 
#line 1021 "parser.cc"
		}

		break;

	case TYPENAME:
		{
			w_scantoken(TYPENAME, &w_link, w_resync99);
#line 329 "ccgram.w"
			 (*w_rr) = g_tok->text; 
#line 1031 "parser.cc"
		}

		break;

	case OPERATOR:
		{
			struct W_Toperator_func w_rvoperator_func;
			w_foperator_func(&w_link, w_resync270, &w_rvoperator_func);
#line 330 "ccgram.w"
			 (*w_rr) = w_rvoperator_func.name; 
#line 1042 "parser.cc"
		}

		break;

	case CLASSNAME:
		{
			w_scantoken(CLASSNAME, &w_link, w_resync250);
#line 332 "ccgram.w"
			 (*w_rr) = make_constructor_name(g_tok->text); 
#line 1052 "parser.cc"
		}

		break;

	case '~':
		{
			w_scantoken('~', &w_link, w_resync54);
			w_scantoken(CLASSNAME, &w_link, w_resync250);
#line 333 "ccgram.w"
			 (*w_rr) = make_destructor_name(g_tok->text); 
#line 1063 "parser.cc"
		}

		break;

	default:
		w_err_illegal("member_name");
		return W_RETERR;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fargument_expr_list(struct w_resynclink *w_lnk, int *w_resync, Expr_arr* *w_rr)
#else
w_fargument_expr_list(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Expr_arr* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case '(':
	case INTVAL:
	case UINTVAL:
	case LONGVAL:
	case ULONGVAL:
	case CHARACTER:
	case WCHARACTER:
	case FLOATVAL:
	case DOUBLEVAL:
	case LONGDOUBLEVAL:
	case IDENTIFIER:
	case THIS:
	case COLONCOLON:
	case SHORTDOUBLEVAL:
	case LONGLONGVAL:
	case ULONGLONGVAL:
	case CLASSNAME:
	case '~':
	case STRING:
	case WSTRING:
	case INCR:
	case DECR:
	case '&':
	case '*':
	case '+':
	case '-':
	case '!':
	case SIZEOF:
	case X_TYPENAME:
	case NEW:
	case DELETE:
	case OPERATOR:
		{
			Expr_node* w_rvassignment_expr;
			w_fassignment_expr(&w_link, w_resync42, &w_rvassignment_expr);
#line 338 "ccgram.w"
			 (*w_rr) = new Expr_arr; (*w_rr)->end() = w_rvassignment_expr; 
#line 1130 "parser.cc"

			{/*__P5*/
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync59;

				while (w_nexttoken() == ',')
				{
					{
						Expr_node* w_rvassignment_expr;
						w_scantoken(',', &w_link, w_resync42);
						w_fassignment_expr(&w_link, w_resync145, &w_rvassignment_expr);
#line 339 "ccgram.w"
						 (*w_rr)->end() = w_rvassignment_expr; 
#line 1147 "parser.cc"
					}

				}
			}/*__P5*/

		}

		break;

	default:
		{
#line 340 "ccgram.w"
			 (*w_rr) = NULL; 
#line 1161 "parser.cc"
		}

		break;
	}

	return W_RETOK;
}

#line 343 "ccgram.w"

	// this is here so macro w_nexttoken() is used instead of function
	static void do_look_for_typename();

	inline void
	look_for_typename()
	{
		if (w_nexttoken() == '('/*)*/)
			do_look_for_typename();
	}

#line 1182 "parser.cc"
static int
#if defined(__cplusplus) || defined(__STDC__)
w_funary_expr(struct w_resynclink *w_lnk, int *w_resync, Expr_node* *w_rr)
#else
w_funary_expr(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Expr_node* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case '(':
	case INTVAL:
	case UINTVAL:
	case LONGVAL:
	case ULONGVAL:
	case CHARACTER:
	case WCHARACTER:
	case FLOATVAL:
	case DOUBLEVAL:
	case LONGDOUBLEVAL:
	case IDENTIFIER:
	case THIS:
	case COLONCOLON:
	case SHORTDOUBLEVAL:
	case LONGLONGVAL:
	case ULONGLONGVAL:
	case CLASSNAME:
	case STRING:
	case WSTRING:
	case OPERATOR:
		{
			Expr_node* w_rvpostfix_expr;

			{/*postfix_expr*/
				Expr_node* *w_rr = &w_rvpostfix_expr;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync274;

				{
					Expr_node* w_rvprimary_expr;

					{/*primary_expr*/
						Expr_node* *w_rr = &w_rvprimary_expr;
						struct w_resynclink *w_lnk = &w_link;
						struct w_resynclink w_link;

						w_link.prev = w_lnk;
						w_link.resync = w_resync225;

						switch (w_nexttoken())
						{
							case STRING:
							case WSTRING:
								{
									String_node* w_rvstring_literal;
									w_fstring_literal(&w_link, w_resync200, &w_rvstring_literal);
#line 209 "ccgram.w"
									 (*w_rr) = w_rvstring_literal; 
#line 1251 "parser.cc"
								}

								break;

							case '(':
								{
									Expr_node* w_rvexpression;
									w_scantoken('(', &w_link, w_resync73);
									w_fexpression(&w_link, w_resync73, &w_rvexpression);
									w_scantoken(')', &w_link, w_resync100);
#line 210 "ccgram.w"
									 (*w_rr) = w_rvexpression; 
#line 1264 "parser.cc"
								}

								break;

							case INTVAL:
								{
									w_scantoken(INTVAL, &w_link, w_resync207);
#line 212 "ccgram.w"
									 (*w_rr) = new Integer_node(g_int_type, g_curr_scope, g_tok->text); 
#line 1274 "parser.cc"
								}

								break;

							case UINTVAL:
								{
									w_scantoken(UINTVAL, &w_link, w_resync91);
#line 214 "ccgram.w"
									 (*w_rr) = new Integer_node(g_uint_type, g_curr_scope, g_tok->text); 
#line 1284 "parser.cc"
								}

								break;

							case LONGVAL:
								{
									w_scantoken(LONGVAL, &w_link, w_resync195);
#line 216 "ccgram.w"
									 (*w_rr) = new Integer_node(g_long_type, g_curr_scope, g_tok->text); 
#line 1294 "parser.cc"
								}

								break;

							case ULONGVAL:
								{
									w_scantoken(ULONGVAL, &w_link, w_resync61);
#line 218 "ccgram.w"
									 (*w_rr) = new Integer_node(g_ulong_type, g_curr_scope, g_tok->text); 
#line 1304 "parser.cc"
								}

								break;

							case CHARACTER:
								{
									w_scantoken(CHARACTER, &w_link, w_resync129);
#line 220 "ccgram.w"
									 (*w_rr) = new Integer_node(g_char_type, g_curr_scope,
				strtochar(g_tok->text)); 
#line 1315 "parser.cc"
								}

								break;

							case WCHARACTER:
								{
									w_scantoken(WCHARACTER, &w_link, w_resync230);
#line 223 "ccgram.w"
									 (*w_rr) = new Integer_node(g_wchar_t_type, g_curr_scope,
				strtochar(g_tok->text, TRUE)); 
#line 1326 "parser.cc"
								}

								break;

							case FLOATVAL:
								{
									w_scantoken(FLOATVAL, &w_link, w_resync189);
#line 226 "ccgram.w"
									 (*w_rr) = new Float_node(g_float_type, g_curr_scope, g_tok->text); 
#line 1336 "parser.cc"
								}

								break;

							case DOUBLEVAL:
								{
									w_scantoken(DOUBLEVAL, &w_link, w_resync55);
#line 228 "ccgram.w"
									 (*w_rr) = new Float_node(g_double_type, g_curr_scope, g_tok->text); 
#line 1346 "parser.cc"
								}

								break;

							case LONGDOUBLEVAL:
								{
									w_scantoken(LONGDOUBLEVAL, &w_link, w_resync106);
#line 230 "ccgram.w"
									 (*w_rr) = new Float_node(g_longdouble_type, g_curr_scope,
				g_tok->text); 
#line 1357 "parser.cc"
								}

								break;

							case IDENTIFIER:
								{
									w_scantoken(IDENTIFIER, &w_link, w_resync209);
#line 233 "ccgram.w"
									 (*w_rr) = new Symbol_node(g_curr_scope, g_tok->text,
				g_cpp.filename(), g_cpp.linenum()); 
#line 1368 "parser.cc"
								}

								break;

							case THIS:
								{
									w_scantoken(THIS, &w_link, w_resync114);
#line 237 "ccgram.w"
									 (*w_rr) = new Symbol_node(g_curr_scope, g_tok->text,
				g_cpp.filename(), g_cpp.linenum()); 
#line 1379 "parser.cc"
								}

								break;

							case OPERATOR:
								{
									struct W_Toperator_func w_rvoperator_func;
									w_foperator_func(&w_link, w_resync270, &w_rvoperator_func);
#line 240 "ccgram.w"
									
			(*w_rr) = new Symbol_node(g_curr_scope, w_rvoperator_func.name,
					g_cpp.filename(), g_cpp.linenum());
		
#line 1393 "parser.cc"
								}

								break;

							case CLASSNAME:
								{
									Expr_node* w_rvqualified_object_name;

									{/*qualified_object_name*/
										Expr_node* *w_rr = &w_rvqualified_object_name;
										struct w_resynclink *w_lnk = &w_link;
										struct w_resynclink w_link;

										w_link.prev = w_lnk;
										w_link.resync = w_resync250;

										{
											w_scantoken(CLASSNAME, &w_link, w_resync108);
											w_scantoken(COLONCOLON, &w_link, w_resync34);
#line 263 "ccgram.w"
											
				Scope *sav = g_curr_scope;

				if (g_cplusplus)
					g_curr_scope = ((Struct_type*)g_sym->type())->scope();
			
#line 1420 "parser.cc"

											{/*__P1*/
												struct w_resynclink *w_lnk = &w_link;
												struct w_resynclink w_link;

												w_link.prev = w_lnk;
												w_link.resync = w_resync47;

												switch (w_nexttoken())
												{
													case IDENTIFIER:
														{
															w_scantoken(IDENTIFIER, &w_link, w_resync209);
#line 271 "ccgram.w"
															
				(*w_rr) = new Symbol_node(g_curr_scope, g_tok->text,
						g_cpp.filename(), g_cpp.linenum());
			
#line 1439 "parser.cc"
														}

														break;

													case OPERATOR:
														{
															struct W_Toperator_func w_rvoperator_func;
															w_foperator_func(&w_link, w_resync270, &w_rvoperator_func);
#line 276 "ccgram.w"
															
				(*w_rr) = new Symbol_node(g_curr_scope, w_rvoperator_func.name,
						g_cpp.filename(), g_cpp.linenum());
			
#line 1453 "parser.cc"
														}

														break;

													case '~':
														{
															w_scantoken('~', &w_link, w_resync54);
															w_scantoken(CLASSNAME, &w_link, w_resync250);
#line 281 "ccgram.w"
															
				(*w_rr) = new Symbol_node(g_curr_scope,
						make_destructor_name(g_tok->text),
						g_cpp.filename(), g_cpp.linenum());
			
#line 1468 "parser.cc"
														}

														break;

													default:
														w_err_illegal("qualified_object_name");
														break;
												}
											}/*__P1*/

#line 288 "ccgram.w"
											 g_curr_scope = sav; 
#line 1481 "parser.cc"
										}

									}/*qualified_object_name*/

#line 244 "ccgram.w"
									 (*w_rr) = w_rvqualified_object_name; 
#line 1488 "parser.cc"
								}

								break;

							case COLONCOLON:
								{
									w_scantoken(COLONCOLON, &w_link, w_resync156);
									w_scantoken(IDENTIFIER, &w_link, w_resync209);
#line 246 "ccgram.w"
									 (*w_rr) = new Symbol_node(g_file_scope, g_tok->text,
				g_cpp.filename(), g_cpp.linenum()); 
#line 1500 "parser.cc"
								}

								break;

							case SHORTDOUBLEVAL:
								{
									w_scantoken(SHORTDOUBLEVAL, &w_link, w_resync197);
#line 250 "ccgram.w"
									 (*w_rr) = new Float_node(g_shortdouble_type, g_curr_scope,
				g_tok->text); 
#line 1511 "parser.cc"
								}

								break;

							case LONGLONGVAL:
								{
									w_scantoken(LONGLONGVAL, &w_link, w_resync64);
#line 254 "ccgram.w"
									 (*w_rr) = new Integer_node(g_longlong_type, g_curr_scope,
				g_tok->text); 
#line 1522 "parser.cc"
								}

								break;

							case ULONGLONGVAL:
								{
									w_scantoken(ULONGLONGVAL, &w_link, w_resync138);
#line 257 "ccgram.w"
									 (*w_rr) = new Integer_node(g_ulonglong_type, g_curr_scope,
				g_tok->text); 
#line 1533 "parser.cc"
								}

								break;

							default:
								w_err_illegal("primary_expr");
								break;
						}
					}/*primary_expr*/

#line 307 "ccgram.w"
					 (*w_rr) = w_rvprimary_expr; 
#line 1546 "parser.cc"

					{/*__P4*/
						struct w_resynclink *w_lnk = &w_link;
						struct w_resynclink w_link;

						w_link.prev = w_lnk;
						w_link.resync = w_resync217;

						for (;;)
						{
							switch (w_nexttoken())
							{
							case '(':
							case '[':
							case '.':
							case ARROW:
							case INCR:
							case DECR:
								switch (w_nexttoken())
								{
									case '[':
										{
											Expr_node* w_rvexpression;
											w_scantoken('[', &w_link, w_resync153);
											w_fexpression(&w_link, w_resync237, &w_rvexpression);
											w_scantoken(']', &w_link, w_resync211);
#line 310 "ccgram.w"
											 (*w_rr) = make_array_ref_node(g_curr_scope, (*w_rr), w_rvexpression); 
#line 1575 "parser.cc"
										}

										break;

									case '(':
										{
											Expr_arr* w_rvargument_expr_list;
											w_scantoken('(', &w_link, w_resync191);
											w_fargument_expr_list(&w_link, w_resync191, &w_rvargument_expr_list);
											w_scantoken(')', &w_link, w_resync100);
#line 312 "ccgram.w"
											 (*w_rr) = make_func_call_node(g_curr_scope, (*w_rr),
						w_rvargument_expr_list); 
#line 1589 "parser.cc"
										}

										break;

									case '.':
										{
											const char* w_rvmember_name;
											w_scantoken('.', &w_link, w_resync21);
											w_fmember_name(&w_link, w_resync132, &w_rvmember_name);
#line 315 "ccgram.w"
											 (*w_rr) = make_struct_ref_node(g_curr_scope, (*w_rr), w_rvmember_name,
						g_cpp.filename(), g_cpp.linenum(), FALSE); 
#line 1602 "parser.cc"
										}

										break;

									case ARROW:
										{
											const char* w_rvmember_name;
											w_scantoken(ARROW, &w_link, w_resync239);
											w_fmember_name(&w_link, w_resync132, &w_rvmember_name);
#line 318 "ccgram.w"
											 (*w_rr) = make_struct_ref_node(g_curr_scope, (*w_rr), w_rvmember_name,
						g_cpp.filename(), g_cpp.linenum(), TRUE); 
#line 1615 "parser.cc"
										}

										break;

									case INCR:
										{
											w_scantoken(INCR, &w_link, w_resync65);
#line 321 "ccgram.w"
											 (*w_rr) = make_operator_node(g_curr_scope, INCR, (*w_rr), FALSE); 
#line 1625 "parser.cc"
										}

										break;

									case DECR:
										{
											w_scantoken(DECR, &w_link, w_resync241);
#line 323 "ccgram.w"
											 (*w_rr) = make_operator_node(g_curr_scope, DECR, (*w_rr), FALSE); 
#line 1635 "parser.cc"
										}

										break;

									default:
										w_err_illegal("postfix_expr");
										break;
								}
								continue;

							default:
								break;
							}

							break; /*for(;;)*/
						}

					}/*__P4*/

				}

			}/*postfix_expr*/

#line 356 "ccgram.w"
			 (*w_rr) = w_rvpostfix_expr; 
#line 1661 "parser.cc"
		}

		break;

	case INCR:
		{
			Expr_node* w_rvunary_expr;
			w_scantoken(INCR, &w_link, w_resync136);
			w_funary_expr(&w_link, w_resync136, &w_rvunary_expr);
#line 358 "ccgram.w"
			 (*w_rr) = make_operator_node(g_curr_scope, INCR, w_rvunary_expr); 
#line 1673 "parser.cc"
		}

		break;

	case DECR:
		{
			Expr_node* w_rvunary_expr;
			w_scantoken(DECR, &w_link, w_resync136);
			w_funary_expr(&w_link, w_resync136, &w_rvunary_expr);
#line 360 "ccgram.w"
			 (*w_rr) = make_operator_node(g_curr_scope, DECR, w_rvunary_expr); 
#line 1685 "parser.cc"
		}

		break;

	case '~':
	case '&':
	case '*':
	case '+':
	case '-':
	case '!':
		{
			Expr_node* w_rvcast_expr;
#line 361 "ccgram.w"
			 int tok = w_nexttoken(); 
#line 1700 "parser.cc"

			{/*__P6*/
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync145;

				switch (w_nexttoken())
				{
					case '&':
						{
							w_scantoken('&', &w_link, w_resync83);
						}

						break;

					case '*':
						{
							w_scantoken('*', &w_link, w_resync3);
						}

						break;

					case '+':
						{
							w_scantoken('+', &w_link, w_resync95);
						}

						break;

					case '-':
						{
							w_scantoken('-', &w_link, w_resync23);
						}

						break;

					case '!':
						{
							w_scantoken('!', &w_link, w_resync137);
						}

						break;

					case '~':
						{
							w_scantoken('~', &w_link, w_resync103);
#line 364 "ccgram.w"
							
				if (w_nexttoken() == CLASSNAME)
				{
					(*w_rr) = new Symbol_node(g_file_scope,
							make_destructor_name(g_tok->text),
							g_cpp.filename(), g_cpp.linenum());
					w_skiptoken();		// CLASSNAME
					goto done_unary_expr;
				}
			
#line 1760 "parser.cc"
						}

						break;

					default:
						w_err_illegal("unary_expr");
						break;
				}
			}/*__P6*/

			w_fcast_expr(&w_link, w_resync145, &w_rvcast_expr);
#line 376 "ccgram.w"
			
				(*w_rr) = make_operator_node(g_curr_scope, tok, w_rvcast_expr);
			done_unary_expr:
				;
			
#line 1778 "parser.cc"
		}

		break;

	case SIZEOF:
		{
			w_scantoken(SIZEOF, &w_link, w_resync145);
#line 381 "ccgram.w"
			 look_for_typename(); 
#line 1788 "parser.cc"

			{/*__P7*/
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync145;

				switch (w_nexttoken())
				{
					case X_TYPENAME:
						{
							Type* w_rvtype_name;
							w_scantoken(X_TYPENAME, &w_link, w_resync19);
							w_scantoken('(', &w_link, w_resync18);
							w_ftype_name(&w_link, w_resync25, &w_rvtype_name);
							w_scantoken(')', &w_link, w_resync100);
#line 384 "ccgram.w"
							 (*w_rr) = new Sizeof_node(g_curr_scope, w_rvtype_name); 
#line 1808 "parser.cc"
						}

						break;

					case '(':
					case INTVAL:
					case UINTVAL:
					case LONGVAL:
					case ULONGVAL:
					case CHARACTER:
					case WCHARACTER:
					case FLOATVAL:
					case DOUBLEVAL:
					case LONGDOUBLEVAL:
					case IDENTIFIER:
					case THIS:
					case COLONCOLON:
					case SHORTDOUBLEVAL:
					case LONGLONGVAL:
					case ULONGLONGVAL:
					case CLASSNAME:
					case '~':
					case STRING:
					case WSTRING:
					case INCR:
					case DECR:
					case '&':
					case '*':
					case '+':
					case '-':
					case '!':
					case SIZEOF:
					case NEW:
					case DELETE:
					case OPERATOR:
						{
							Expr_node* w_rvunary_expr;
							w_funary_expr(&w_link, w_resync136, &w_rvunary_expr);
#line 386 "ccgram.w"
							 (*w_rr) = new Sizeof_node(g_curr_scope, w_rvunary_expr); 
#line 1849 "parser.cc"
						}

						break;

					default:
						w_err_illegal("unary_expr");
						break;
				}
			}/*__P7*/

		}

		break;

	case NEW:
	case DELETE:
		{
			Expr_node* w_rvnew_delete_exprs;

			{/*new_delete_exprs*/
				Expr_node* *w_rr = &w_rvnew_delete_exprs;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync27;

				switch (w_nexttoken())
				{
					case NEW:
						{
							Type* w_rvtype;
							w_scantoken(NEW, &w_link, w_resync151);
#line 392 "ccgram.w"
							 look_for_typename(); 
#line 1885 "parser.cc"

							{/*__P8*/
								Type* *w_rr = &w_rvtype;
								struct w_resynclink *w_lnk = &w_link;
								struct w_resynclink w_link;

								w_link.prev = w_lnk;
								w_link.resync = w_resync150;

								switch (w_nexttoken())
								{
									case X_TYPENAME:
										{
											Type* w_rvnew_type_name;
											w_scantoken(X_TYPENAME, &w_link, w_resync19);
											w_scantoken('(', &w_link, w_resync18);
											w_fnew_type_name(&w_link, w_resync25, &w_rvnew_type_name);
											w_scantoken(')', &w_link, w_resync100);
#line 394 "ccgram.w"
											 (*w_rr) = w_rvnew_type_name; 
#line 1906 "parser.cc"
										}

										break;

									case CLASSNAME:
									case '~':
									case TYPENAME:
									case VOID:
									case CHAR:
									case SHORT:
									case INT:
									case LONG:
									case FLOAT:
									case DOUBLE:
									case SIGNED:
									case UNSIGNED:
									case STRUCT:
									case UNION:
									case CLASS:
									case ENUM:
									case CONST:
									case VOLATILE:
										{
											Type* w_rvnew_type_name;
											w_fnew_type_name(&w_link, w_resync63, &w_rvnew_type_name);
#line 395 "ccgram.w"
											 (*w_rr) = w_rvnew_type_name; 
#line 1934 "parser.cc"
										}

										break;

									default:
										w_err_illegal("new_delete_exprs");
										break;
								}
							}/*__P8*/


							{/*__P9*/
								struct w_resynclink *w_lnk = &w_link;
								struct w_resynclink w_link;

								w_link.prev = w_lnk;
								w_link.resync = w_resync122;

								switch (w_nexttoken())
								{
									case '[':
										{
											Expr_node* w_rvmark_expr;
											w_scantoken('[', &w_link, w_resync153);
											w_fmark_expr(&w_link, w_resync237, &w_rvmark_expr);
											w_scantoken(']', &w_link, w_resync211);
#line 400 "ccgram.w"
											 (*w_rr) = new New_node(g_curr_scope, w_rvtype, NULL, w_rvmark_expr); 
#line 1963 "parser.cc"
										}

										break;

									case '(':
										{
											Expr_arr* w_rvargument_expr_list;
											w_scantoken('(', &w_link, w_resync191);
											w_fargument_expr_list(&w_link, w_resync191, &w_rvargument_expr_list);
											w_scantoken(')', &w_link, w_resync100);
#line 402 "ccgram.w"
											 (*w_rr) = new New_node(g_curr_scope, w_rvtype,
					w_rvargument_expr_list, NULL); 
#line 1977 "parser.cc"
										}

										break;

									default:
										{
#line 405 "ccgram.w"
											 (*w_rr) = new New_node(g_curr_scope, w_rvtype, NULL, NULL); 
#line 1986 "parser.cc"
										}

										break;
								}
							}/*__P9*/

						}

						break;

					case DELETE:
						{
							w_scantoken(DELETE, &w_link, w_resync46);

							{/*__P10*/
								struct w_resynclink *w_lnk = &w_link;
								struct w_resynclink w_link;

								w_link.prev = w_lnk;
								w_link.resync = w_resync46;

								switch (w_nexttoken())
								{
									case '[':
										{
											Expr_node* w_rvopt_expr;
											Expr_node* w_rvcast_expr;
											w_scantoken('[', &w_link, w_resync247);
											w_fopt_expr(&w_link, w_resync50, &w_rvopt_expr);
											w_scantoken(']', &w_link, w_resync237);
											w_fcast_expr(&w_link, w_resync145, &w_rvcast_expr);
#line 409 "ccgram.w"
											 (*w_rr) = new Delete_node(g_curr_scope, w_rvcast_expr, w_rvopt_expr, TRUE); 
#line 2020 "parser.cc"
										}

										break;

									case '(':
									case INTVAL:
									case UINTVAL:
									case LONGVAL:
									case ULONGVAL:
									case CHARACTER:
									case WCHARACTER:
									case FLOATVAL:
									case DOUBLEVAL:
									case LONGDOUBLEVAL:
									case IDENTIFIER:
									case THIS:
									case COLONCOLON:
									case SHORTDOUBLEVAL:
									case LONGLONGVAL:
									case ULONGLONGVAL:
									case CLASSNAME:
									case '~':
									case STRING:
									case WSTRING:
									case INCR:
									case DECR:
									case '&':
									case '*':
									case '+':
									case '-':
									case '!':
									case SIZEOF:
									case X_TYPENAME:
									case NEW:
									case DELETE:
									case OPERATOR:
										{
											Expr_node* w_rvcast_expr;
											w_fcast_expr(&w_link, w_resync145, &w_rvcast_expr);
#line 411 "ccgram.w"
											 (*w_rr) = new Delete_node(g_curr_scope, w_rvcast_expr, NULL, FALSE); 
#line 2062 "parser.cc"
										}

										break;

									default:
										w_err_illegal("new_delete_exprs");
										break;
								}
							}/*__P10*/

						}

						break;

					default:
						w_err_illegal("new_delete_exprs");
						break;
				}
			}/*new_delete_exprs*/

#line 388 "ccgram.w"
			 (*w_rr) = w_rvnew_delete_exprs; 
#line 2085 "parser.cc"
		}

		break;

	default:
		w_err_illegal("unary_expr");
		return W_RETERR;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fnew_type_name(struct w_resynclink *w_lnk, int *w_resync, Type* *w_rr)
#else
w_fnew_type_name(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Type* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Storage_thing w_rvsq;
		Storage_thing* w_rvpointer;
		w_fspecifier_qualifier_list(&w_link, w_resync29, &w_rvsq);
#line 417 "ccgram.w"
		
				w_rvsq.make_type();
				w_rvpointer = &w_rvsq;
			
#line 2122 "parser.cc"
		w_fpointer(&w_link, w_resync215, &w_rvpointer);
#line 422 "ccgram.w"
		
				w_rvsq.make_type();
				(*w_rr) = w_rvsq.type;
			
#line 2129 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_foperator_func(struct w_resynclink *w_lnk, int *w_resync, struct W_Toperator_func *w_rr)
#else
w_foperator_func(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
struct W_Toperator_func *w_rr;
#endif
{
	struct w_resynclink w_link;
#line 429 "ccgram.w"
	 (*w_rr).type = NULL; 
#line 2148 "parser.cc"

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		w_scantoken(OPERATOR, &w_link, w_resync259);
#line 431 "ccgram.w"
		
				(*w_rr).name = make_operator_name(w_nexttoken());

				if (w_nexttoken() == '~')
					w_currtoken = X_TILDE;
			
#line 2162 "parser.cc"

		{/*__P11*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync165;

			switch (w_nexttoken())
			{
				case ',':
				case '&':
				case '*':
				case '+':
				case '-':
				case '/':
				case '%':
				case LSHIFT:
				case RSHIFT:
				case '<':
				case '>':
				case LESSEQ:
				case GREATEQ:
				case EQUAL:
				case NOTEQ:
				case '^':
				case '|':
				case ANDAND:
				case OROR:
				case '?':
				case '=':
				case MULTEQ:
				case DIVEQ:
				case MODEQ:
				case PLUSEQ:
				case MINUSEQ:
				case LSHIFTEQ:
				case RSHIFTEQ:
				case ANDEQ:
				case XOREQ:
				case OREQ:
					{
						struct W_Toperator w_rvoperator;
						w_foperator(&w_link, w_resync44, &w_rvoperator);
					}

					break;

				case '!':
					{
						w_scantoken('!', &w_link, w_resync137);
					}

					break;

				case X_TILDE:
					{
						w_scantoken(X_TILDE, &w_link, w_resync31);
					}

					break;

				case INCR:
					{
						w_scantoken(INCR, &w_link, w_resync65);
					}

					break;

				case DECR:
					{
						w_scantoken(DECR, &w_link, w_resync241);
					}

					break;

				case ARROW:
					{
						w_scantoken(ARROW, &w_link, w_resync267);
					}

					break;

				case '(':
					{
						w_scantoken('(', &w_link, w_resync130);
						w_scantoken(')', &w_link, w_resync100);
					}

					break;

				case '[':
					{
						w_scantoken('[', &w_link, w_resync221);
						w_scantoken(']', &w_link, w_resync211);
					}

					break;

				case NEW:
					{
						w_scantoken(NEW, &w_link, w_resync78);
					}

					break;

				case DELETE:
					{
						w_scantoken(DELETE, &w_link, w_resync224);
					}

					break;

				case CLASSNAME:
				case '~':
				case TYPENAME:
				case VOID:
				case CHAR:
				case SHORT:
				case INT:
				case LONG:
				case FLOAT:
				case DOUBLE:
				case SIGNED:
				case UNSIGNED:
				case STRUCT:
				case UNION:
				case CLASS:
				case ENUM:
				case CONST:
				case VOLATILE:
					{
						Type* w_rvnew_type_name;
						w_fnew_type_name(&w_link, w_resync63, &w_rvnew_type_name);
#line 451 "ccgram.w"
						
				(*w_rr).name = make_opcast_name(w_rvnew_type_name);
				(*w_rr).type = w_rvnew_type_name;		// should be in the pool
			
#line 2302 "parser.cc"
					}

					break;

				default:
					w_err_illegal("operator_func");
					break;
			}
		}/*__P11*/

	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fcast_expr(struct w_resynclink *w_lnk, int *w_resync, Expr_node* *w_rr)
#else
w_fcast_expr(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Expr_node* *w_rr;
#endif
{
	struct w_resynclink w_link;
#line 459 "ccgram.w"
	
		Type_arr *casts = NULL;
		Expr_node *ret = NULL;
	
#line 2334 "parser.cc"

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Expr_node* w_rvunary_expr;
#line 463 "ccgram.w"
		 look_for_typename(); 
#line 2343 "parser.cc"

		{/*__P12*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync145;

			while (w_nexttoken() == X_TYPENAME)
			{
				{
					Type* w_rvtype_name;
					w_scantoken(X_TYPENAME, &w_link, w_resync19);
					w_scantoken('(', &w_link, w_resync164);
					w_ftype_name(&w_link, w_resync280, &w_rvtype_name);
					w_scantoken(')', &w_link, w_resync193);

					{/*__P13*/
						struct w_resynclink *w_lnk = &w_link;
						struct w_resynclink w_link;

						w_link.prev = w_lnk;
						w_link.resync = w_resync240;

						switch (w_nexttoken())
						{
							case '{':
								{
									Initializer_node* w_rvinitializer_list;
									w_scantoken('{', &w_link, w_resync45);
									w_finitializer_list(&w_link, w_resync45, &w_rvinitializer_list);
									w_scantoken('}', &w_link, w_resync109);
#line 468 "ccgram.w"
									
					ret = do_struct_display(w_rvtype_name, w_rvinitializer_list);
					goto done;
				
#line 2381 "parser.cc"
								}

								break;

							default:
								{
								}

								break;
						}
					}/*__P13*/

#line 474 "ccgram.w"
					
				if (casts == NULL)
					casts = new Type_arr;

				casts->end() = w_rvtype_name;
				look_for_typename();
			
#line 2402 "parser.cc"
				}

			}
		}/*__P12*/

		w_funary_expr(&w_link, w_resync136, &w_rvunary_expr);
#line 483 "ccgram.w"
		
				ret = w_rvunary_expr;

			done:
				if (casts != NULL)
				{
					for (int i = casts->size() - 1; i >= 0; i--)
						ret = new Cast_node(g_curr_scope,
								casts->elt(i), ret);

					delete casts;
				}

				(*w_rr) = ret;
			
#line 2425 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fexpr(struct w_resynclink *w_lnk, int *w_resync, struct W_Texpr *w_rr)
#else
w_fexpr(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
struct W_Texpr *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{

		{/*__P14*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync44;

			for (;;)
			{
				switch (w_nexttoken())
				{
				case ',':
				case '&':
				case '*':
				case '+':
				case '-':
				case '/':
				case '%':
				case LSHIFT:
				case RSHIFT:
				case '<':
				case '>':
				case LESSEQ:
				case GREATEQ:
				case EQUAL:
				case NOTEQ:
				case '^':
				case '|':
				case ANDAND:
				case OROR:
				case '?':
				case '=':
				case MULTEQ:
				case DIVEQ:
				case MODEQ:
				case PLUSEQ:
				case MINUSEQ:
				case LSHIFTEQ:
				case RSHIFTEQ:
				case ANDEQ:
				case XOREQ:
				case OREQ:
					{
						struct W_Toperator w_rvoperator;
						Expr_node* w_rvcast_expr;
						struct W_Texpr w_rvexpr;
						Expr_node* w_rvexpression;
						Expr_node* w_rvconditional_expr;
#line 502 "ccgram.w"
						
					if (peek_op_prec() <= (*w_rr).stop)
						goto done_expr;
				
#line 2501 "parser.cc"
						w_foperator(&w_link, w_resync176, &w_rvoperator);
#line 507 "ccgram.w"
						
					if (w_rvoperator.op == '?')
						goto do_conditional;
				
#line 2508 "parser.cc"
						w_fcast_expr(&w_link, w_resync113, &w_rvcast_expr);
#line 512 "ccgram.w"
						
					if (peek_op_prec() <= w_rvoperator.prec && w_rvoperator.left)
					{
						(*w_rr).e = make_operator2_node(g_curr_scope,
								w_rvoperator.op, (*w_rr).e, w_rvcast_expr);
						continue;
					}

					w_rvexpr.e = w_rvcast_expr;
					w_rvexpr.stop = w_rvoperator.prec;

					if (!w_rvoperator.left)
						w_rvexpr.stop--;
				
#line 2525 "parser.cc"
						w_fexpr(&w_link, w_resync223, &w_rvexpr);
#line 527 "ccgram.w"
						
					(*w_rr).e = make_operator2_node(g_curr_scope,
							w_rvoperator.op, (*w_rr).e, w_rvexpr.e);
					continue;

				do_conditional:
				
#line 2535 "parser.cc"
						w_fexpression(&w_link, w_resync43, &w_rvexpression);
						w_scantoken(':', &w_link, w_resync43);

						{/*conditional_expr*/
							Expr_node* *w_rr = &w_rvconditional_expr;
							struct w_resynclink *w_lnk = &w_link;
							struct w_resynclink w_link;

							w_link.prev = w_lnk;
							w_link.resync = w_resync145;

							{
								Expr_node* w_rvcast_expr;
								struct W_Texpr w_rvexpr;
								w_fcast_expr(&w_link, w_resync113, &w_rvcast_expr);
#line 584 "ccgram.w"
								
				w_rvexpr.e = w_rvcast_expr;
				w_rvexpr.stop = 2;		// no commas, no assignment
			
#line 2556 "parser.cc"
								w_fexpr(&w_link, w_resync96, &w_rvexpr);
#line 588 "ccgram.w"
								 (*w_rr) = w_rvexpr.e; 
#line 2560 "parser.cc"
							}

						}/*conditional_expr*/

#line 535 "ccgram.w"
						 (*w_rr).e = new Operator3_node(g_curr_scope, '?', (*w_rr).e,
						w_rvexpression, w_rvconditional_expr); 
#line 2568 "parser.cc"
					}

					continue;

				default:
					break;
				}

				break; /*for(;;)*/
			}

		}/*__P14*/

#line 537 "ccgram.w"
		 done_expr: ; 
#line 2584 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_foperator(struct w_resynclink *w_lnk, int *w_resync, struct W_Toperator *w_rr)
#else
w_foperator(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
struct W_Toperator *w_rr;
#endif
{
	struct w_resynclink w_link;
#line 541 "ccgram.w"
	
		(*w_rr).prec = peek_op_prec();
		(*w_rr).op = w_currtoken;
		(*w_rr).left = TRUE;
	
#line 2607 "parser.cc"

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case '*':
		{
			w_scantoken('*', &w_link, w_resync3);
		}

		break;

	case '/':
		{
			w_scantoken('/', &w_link, w_resync148);
		}

		break;

	case '%':
		{
			w_scantoken('%', &w_link, w_resync52);
		}

		break;

	case '+':
		{
			w_scantoken('+', &w_link, w_resync95);
		}

		break;

	case '-':
		{
			w_scantoken('-', &w_link, w_resync23);
		}

		break;

	case LSHIFT:
		{
			w_scantoken(LSHIFT, &w_link, w_resync181);
		}

		break;

	case RSHIFT:
		{
			w_scantoken(RSHIFT, &w_link, w_resync117);
		}

		break;

	case '<':
		{
			w_scantoken('<', &w_link, w_resync8);
		}

		break;

	case '>':
		{
			w_scantoken('>', &w_link, w_resync57);
		}

		break;

	case LESSEQ:
		{
			w_scantoken(LESSEQ, &w_link, w_resync190);
		}

		break;

	case GREATEQ:
		{
			w_scantoken(GREATEQ, &w_link, w_resync71);
		}

		break;

	case EQUAL:
		{
			w_scantoken(EQUAL, &w_link, w_resync213);
		}

		break;

	case NOTEQ:
		{
			w_scantoken(NOTEQ, &w_link, w_resync139);
		}

		break;

	case '&':
		{
			w_scantoken('&', &w_link, w_resync83);
		}

		break;

	case '^':
		{
			w_scantoken('^', &w_link, w_resync38);
		}

		break;

	case '|':
		{
			w_scantoken('|', &w_link, w_resync128);
		}

		break;

	case ANDAND:
		{
			w_scantoken(ANDAND, &w_link, w_resync26);
		}

		break;

	case OROR:
		{
			w_scantoken(OROR, &w_link, w_resync97);
		}

		break;

	case '?':
		{
			w_scantoken('?', &w_link, w_resync48);
#line 556 "ccgram.w"
			 (*w_rr).left = FALSE; 
#line 2745 "parser.cc"
		}

		break;

	case '=':
	case MULTEQ:
	case DIVEQ:
	case MODEQ:
	case PLUSEQ:
	case MINUSEQ:
	case LSHIFTEQ:
	case RSHIFTEQ:
	case ANDEQ:
	case XOREQ:
	case OREQ:
		{

			{/*__P15*/
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync212;

				switch (w_nexttoken())
				{
					case '=':
						{
							w_scantoken('=', &w_link, w_resync49);
						}

						break;

					case MULTEQ:
						{
							w_scantoken(MULTEQ, &w_link, w_resync56);
						}

						break;

					case DIVEQ:
						{
							w_scantoken(DIVEQ, &w_link, w_resync66);
						}

						break;

					case MODEQ:
						{
							w_scantoken(MODEQ, &w_link, w_resync87);
						}

						break;

					case PLUSEQ:
						{
							w_scantoken(PLUSEQ, &w_link, w_resync146);
						}

						break;

					case MINUSEQ:
						{
							w_scantoken(MINUSEQ, &w_link, w_resync227);
						}

						break;

					case LSHIFTEQ:
						{
							w_scantoken(LSHIFTEQ, &w_link, w_resync140);
						}

						break;

					case RSHIFTEQ:
						{
							w_scantoken(RSHIFTEQ, &w_link, w_resync218);
						}

						break;

					case ANDEQ:
						{
							w_scantoken(ANDEQ, &w_link, w_resync101);
						}

						break;

					case XOREQ:
						{
							w_scantoken(XOREQ, &w_link, w_resync168);
						}

						break;

					case OREQ:
						{
							w_scantoken(OREQ, &w_link, w_resync271);
						}

						break;

					default:
						w_err_illegal("operator");
						break;
				}
			}/*__P15*/

#line 560 "ccgram.w"
			 (*w_rr).left = FALSE; 
#line 2857 "parser.cc"
		}

		break;

	case ',':
		{
			w_scantoken(',', &w_link, w_resync59);
		}

		break;

	default:
		w_err_illegal("operator");
		return W_RETERR;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fexpression(struct w_resynclink *w_lnk, int *w_resync, Expr_node* *w_rr)
#else
w_fexpression(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Expr_node* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Expr_node* w_rvcast_expr;
		struct W_Texpr w_rvexpr;
		w_fcast_expr(&w_link, w_resync113, &w_rvcast_expr);
#line 566 "ccgram.w"
		
				w_rvexpr.e = w_rvcast_expr;
				w_rvexpr.stop = 0;
			
#line 2901 "parser.cc"
		w_fexpr(&w_link, w_resync96, &w_rvexpr);
#line 570 "ccgram.w"
		 (*w_rr) = w_rvexpr.e; 
#line 2905 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fassignment_expr(struct w_resynclink *w_lnk, int *w_resync, Expr_node* *w_rr)
#else
w_fassignment_expr(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Expr_node* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Expr_node* w_rvcast_expr;
		struct W_Texpr w_rvexpr;
		w_fcast_expr(&w_link, w_resync113, &w_rvcast_expr);
#line 575 "ccgram.w"
		
				w_rvexpr.e = w_rvcast_expr;
				w_rvexpr.stop = 1;		// no commas
			
#line 2935 "parser.cc"
		w_fexpr(&w_link, w_resync96, &w_rvexpr);
#line 579 "ccgram.w"
		 (*w_rr) = w_rvexpr.e; 
#line 2939 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fconstant_expr(struct w_resynclink *w_lnk, int *w_resync, Expr_node* *w_rr)
#else
w_fconstant_expr(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Expr_node* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Expr_node* w_rvcast_expr;
		struct W_Texpr w_rvexpr;
		w_fcast_expr(&w_link, w_resync113, &w_rvcast_expr);
#line 593 "ccgram.w"
		
				w_rvexpr.e = w_rvcast_expr;
				w_rvexpr.stop = 2;		// no commas, no assignment
			
#line 2969 "parser.cc"
		w_fexpr(&w_link, w_resync96, &w_rvexpr);
#line 598 "ccgram.w"
		
				(*w_rr) = w_rvexpr.e;

				if (!w_rvexpr.e->is_int_expr())
					error(EXPECTED_CONST_EXPR);

				w_rvexpr.e->mark_symbol(TRUE, FALSE, FALSE);
			
#line 2980 "parser.cc"
	}

	return W_RETOK;
}

#line 608 "ccgram.w"

	// this is here so macro w_nexttoken() is used instead of function
	static void do_look_for_extern();

	inline void
	look_for_extern()
	{
		if (g_cplusplus && w_nexttoken() == EXTERN)
			do_look_for_extern();
	}

#line 2998 "parser.cc"
static int
#if defined(__cplusplus) || defined(__STDC__)
w_fdeclaration(struct w_resynclink *w_lnk, int *w_resync)
#else
w_fdeclaration(w_lnk, w_resync)
struct w_resynclink *w_lnk;
int *w_resync;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
#line 621 "ccgram.w"
		 look_for_extern(); 
#line 3016 "parser.cc"

		{/*__P16*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync142;

			switch (w_nexttoken())
			{
				case '(':
				case IDENTIFIER:
				case CLASSNAME:
				case '~':
				case TYPENAME:
				case '&':
				case '*':
				case OPERATOR:
				case INLINE:
				case FRIEND:
				case VIRTUAL:
				case TYPEDEF:
				case EXTERN:
				case STATIC:
				case AUTO:
				case REGISTER:
				case PASCAL:
				case FORTRAN:
				case VOID:
				case CHAR:
				case SHORT:
				case INT:
				case LONG:
				case FLOAT:
				case DOUBLE:
				case SIGNED:
				case UNSIGNED:
				case STRUCT:
				case UNION:
				case CLASS:
				case ENUM:
				case CONST:
				case VOLATILE:
				case X_CTOR:
				case X_MEMFUNC:
				case X_DTOR:
					{

						{/*real_declaration*/
							struct w_resynclink *w_lnk = &w_link;
							struct w_resynclink w_link;

							w_link.prev = w_lnk;
							w_link.resync = w_resync159;

							{
								Storage_thing* w_rvdeclarator;
#line 643 "ccgram.w"
								
				Storage_thing declspec, decl;
				boolean hadspec = FALSE;
				boolean save_old_func = g_old_function;

				if (w_nexttoken() == ';')
				{
					w_skiptoken();		// ';'
					goto done_declaration;
				}
			
#line 3086 "parser.cc"

								{/*__P19*/
									struct w_resynclink *w_lnk = &w_link;
									struct w_resynclink w_link;

									w_link.prev = w_lnk;
									w_link.resync = w_resync170;

									switch (w_nexttoken())
									{
										case CLASSNAME:
										case '~':
										case TYPENAME:
										case INLINE:
										case FRIEND:
										case VIRTUAL:
										case TYPEDEF:
										case EXTERN:
										case STATIC:
										case AUTO:
										case REGISTER:
										case PASCAL:
										case FORTRAN:
										case VOID:
										case CHAR:
										case SHORT:
										case INT:
										case LONG:
										case FLOAT:
										case DOUBLE:
										case SIGNED:
										case UNSIGNED:
										case STRUCT:
										case UNION:
										case CLASS:
										case ENUM:
										case CONST:
										case VOLATILE:
											{
												Storage_thing w_rvdeclaration_specifiers;
												w_fdeclaration_specifiers(&w_link, w_resync210, &w_rvdeclaration_specifiers);
#line 655 "ccgram.w"
												
				declspec = w_rvdeclaration_specifiers;
				declspec.make_type();
				hadspec = TRUE;
			
#line 3134 "parser.cc"

												{/*__P20*/
													struct w_resynclink *w_lnk = &w_link;
													struct w_resynclink w_link;

													w_link.prev = w_lnk;
													w_link.resync = w_resync273;

													switch (w_nexttoken())
													{
														case ';':
															{
																w_scantoken(';', &w_link, w_resync222);
#line 661 "ccgram.w"
																
					if (declspec.type != NULL && !declspec.type->issue())
					{
						error(MUST_DECLARE_A_VARIABLE_NAME);
						declspec.type->free();
					}

					goto done_declaration;
				
#line 3158 "parser.cc"
															}

															break;

														default:
															{
															}

															break;
													}
												}/*__P20*/

											}

											break;

										default:
											{
											}

											break;
									}
								}/*__P19*/

#line 674 "ccgram.w"
								
				if (!hadspec)
					declspec.make_type();

				decl = declspec;
				w_rvdeclarator = &decl;
			
#line 3191 "parser.cc"
								w_fdeclarator(&w_link, w_resync170, &w_rvdeclarator);

								{/*__P21*/
									struct w_resynclink *w_lnk = &w_link;
									struct w_resynclink w_link;

									w_link.prev = w_lnk;
									w_link.resync = w_resync170;

									switch (w_nexttoken())
									{
										case '(':
										case IDENTIFIER:
										case CLASSNAME:
										case '~':
										case TYPENAME:
										case '&':
										case '*':
										case OPERATOR:
										case '{':
										case ':':
										case EXTERN_C:
										case INLINE:
										case FRIEND:
										case VIRTUAL:
										case TYPEDEF:
										case EXTERN:
										case STATIC:
										case AUTO:
										case REGISTER:
										case PASCAL:
										case FORTRAN:
										case VOID:
										case CHAR:
										case SHORT:
										case INT:
										case LONG:
										case FLOAT:
										case DOUBLE:
										case SIGNED:
										case UNSIGNED:
										case STRUCT:
										case UNION:
										case CLASS:
										case ENUM:
										case CONST:
										case VOLATILE:
										case X_CTOR:
										case X_MEMFUNC:
										case X_DTOR:
											{
												boolean w_rvdeclaration_list1;
												boolean w_rvdeclaration_list2;
												Node_arr* w_rvstatement_list;
#line 683 "ccgram.w"
												
					boolean old_func = g_old_function;
					Scope *sav = g_curr_scope;

					if (old_func)
						do_old_function_def(decl);
				
#line 3254 "parser.cc"
												w_fdeclaration_list(&w_link, w_resync84, &w_rvdeclaration_list1);
#line 691 "ccgram.w"
												
					Symbol *func;

					if (!old_func && w_rvdeclaration_list1)
					{
						error(BOGUS_FUNCTION_DEFINITION);
						goto done_declaration;
					}
					else if (old_func)
					{
						func = check_old_function_def(decl);
						g_old_function = FALSE;
					}
					else
						func = do_function_def(decl);
				
#line 3273 "parser.cc"
												w_fctor_initializer(&w_link, w_resync135);
												w_scantoken('{', &w_link, w_resync256);
												w_fdeclaration_list(&w_link, w_resync256, &w_rvdeclaration_list2);
												w_fstatement_list(&w_link, w_resync245, &w_rvstatement_list);
												w_scantoken('}', &w_link, w_resync109);
#line 710 "ccgram.w"
												
					fini_function_def(func, w_rvstatement_list);
					g_curr_scope = sav;
				
#line 3284 "parser.cc"
											}

											break;

										case ',':
										case '=':
										case ';':
										case X_CTORINIT:
											{
#line 715 "ccgram.w"
												
					if (!hadspec)
					{
						error(MISSING_TYPE_FOR_DECLARATION);
						goto done_declaration;
					}

					decl.symbol = decl.do_declaration();
				
#line 3304 "parser.cc"

												{/*__P22*/
													struct w_resynclink *w_lnk = &w_link;
													struct w_resynclink w_link;

													w_link.prev = w_lnk;
													w_link.resync = w_resync268;

													switch (w_nexttoken())
													{
														case '=':
															{
																Expr_node* w_rvinitializer;
																w_scantoken('=', &w_link, w_resync180);
																w_finitializer(&w_link, w_resync252, &w_rvinitializer);
#line 725 "ccgram.w"
																 do_initializer(decl.symbol, w_rvinitializer); 
#line 3322 "parser.cc"
															}

															break;

														case X_CTORINIT:
															{
																Expr_arr* w_rvargument_expr_list;
																w_scantoken(X_CTORINIT, &w_link, w_resync37);
																w_fargument_expr_list(&w_link, w_resync191, &w_rvargument_expr_list);
																w_scantoken(')', &w_link, w_resync100);
#line 729 "ccgram.w"
																 do_ctor_init(decl.symbol, w_rvargument_expr_list); 
#line 3335 "parser.cc"
															}

															break;

														default:
															{
#line 731 "ccgram.w"
																 do_initializer(decl.symbol, NULL, save_old_func); 
#line 3344 "parser.cc"
															}

															break;
													}
												}/*__P22*/

#line 733 "ccgram.w"
												 do_tree(decl.symbol); 
#line 3353 "parser.cc"

												{/*__P23*/
													struct w_resynclink *w_lnk = &w_link;
													struct w_resynclink w_link;

													w_link.prev = w_lnk;
													w_link.resync = w_resync105;

													switch (w_nexttoken())
													{
														case ',':
															{
																Storage_thing* w_rvinit_declarator_list;
																w_scantoken(',', &w_link, w_resync236);
#line 734 "ccgram.w"
																 w_rvinit_declarator_list = &declspec; 
#line 3370 "parser.cc"

																{/*init_declarator_list*/
																	Storage_thing* *w_rr = &w_rvinit_declarator_list;
																	struct w_resynclink *w_lnk = &w_link;
																	struct w_resynclink w_link;

																	w_link.prev = w_lnk;
																	w_link.resync = w_resync35;

																	{
																		Storage_thing w_rvinit_declarator;
#line 781 "ccgram.w"
																		 w_rvinit_declarator = *(*w_rr); 
#line 3384 "parser.cc"
																		w_finit_declarator(&w_link, w_resync236, &w_rvinit_declarator);

																		{/*__P27*/
																			struct w_resynclink *w_lnk = &w_link;
																			struct w_resynclink w_link;

																			w_link.prev = w_lnk;
																			w_link.resync = w_resync59;

																			while (w_nexttoken() == ',')
																			{
																				{
																					Storage_thing w_rvinit_declarator;
																					w_scantoken(',', &w_link, w_resync236);
#line 783 "ccgram.w"
																					 w_rvinit_declarator = *(*w_rr); 
#line 3401 "parser.cc"
																					w_finit_declarator(&w_link, w_resync35, &w_rvinit_declarator);
																				}

																			}
																		}/*__P27*/

																	}

																}/*init_declarator_list*/

															}

															break;

														default:
															{
															}

															break;
													}
												}/*__P23*/

												w_scantoken(';', &w_link, w_resync222);
											}

											break;
									}
								}/*__P21*/

#line 739 "ccgram.w"
								
			done_declaration:
				g_old_function = save_old_func;
			
#line 3436 "parser.cc"
							}

						}/*real_declaration*/

					}

					break;

				case EXTERN_C:
					{
						w_scantoken(EXTERN_C, &w_link, w_resync177);
						w_scantoken(STRING, &w_link, w_resync177);
#line 625 "ccgram.w"
						
					int save = g_default_linkage;

					if (streqi(g_tok->text, "C"))
						g_default_linkage = EXTERN_C;
					else if (streqi(g_tok->text, "C++"))
						g_default_linkage = NONE;
					else if (streqi(g_tok->text, "Pascal"))
						g_default_linkage = PASCAL;
					else if (streqi(g_tok->text, "FORTRAN"))
						g_default_linkage = FORTRAN;
				
#line 3462 "parser.cc"

						{/*__P17*/
							struct w_resynclink *w_lnk = &w_link;
							struct w_resynclink w_link;

							w_link.prev = w_lnk;
							w_link.resync = w_resync178;

							switch (w_nexttoken())
							{
								case '(':
								case IDENTIFIER:
								case CLASSNAME:
								case '~':
								case TYPENAME:
								case '&':
								case '*':
								case OPERATOR:
								case EXTERN_C:
								case INLINE:
								case FRIEND:
								case VIRTUAL:
								case TYPEDEF:
								case EXTERN:
								case STATIC:
								case AUTO:
								case REGISTER:
								case PASCAL:
								case FORTRAN:
								case VOID:
								case CHAR:
								case SHORT:
								case INT:
								case LONG:
								case FLOAT:
								case DOUBLE:
								case SIGNED:
								case UNSIGNED:
								case STRUCT:
								case UNION:
								case CLASS:
								case ENUM:
								case CONST:
								case VOLATILE:
								case X_CTOR:
								case X_MEMFUNC:
								case X_DTOR:
									{
										w_fdeclaration(&w_link, w_resync142);
									}

									break;

								case '{':
									{
										boolean w_rvdeclaration_list;
										w_scantoken('{', &w_link, w_resync4);
										w_fdeclaration_list(&w_link, w_resync253, &w_rvdeclaration_list);
										w_scantoken('}', &w_link, w_resync196);

										{/*__P18*/
											struct w_resynclink *w_lnk = &w_link;
											struct w_resynclink w_link;

											w_link.prev = w_lnk;
											w_link.resync = w_resync273;

											switch (w_nexttoken())
											{
												case ';':
													{
														w_scantoken(';', &w_link, w_resync222);
													}

													break;

												default:
													{
													}

													break;
											}
										}/*__P18*/

									}

									break;
							}
						}/*__P17*/

#line 638 "ccgram.w"
						 g_default_linkage = save; 
#line 3555 "parser.cc"
					}

					break;
			}
		}/*__P16*/

	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fdeclaration_specifiers(struct w_resynclink *w_lnk, int *w_resync, Storage_thing *w_rr)
#else
w_fdeclaration_specifiers(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Storage_thing *w_rr;
#endif
{
	struct w_resynclink w_link;
#line 746 "ccgram.w"
	 g_parse_type = (*w_rr).type ? FALSE : TRUE; 
#line 3580 "parser.cc"

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{

		{/*__P24*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;
			int w_rc = W_RETOK;

			w_link.prev = w_lnk;
			w_link.resync = w_resync112;

			goto w_oneplus_1;

			for (;;)
			{
				switch (w_nexttoken())
				{
				w_oneplus_1:
				case CLASSNAME:
				case '~':
				case TYPENAME:
				case INLINE:
				case FRIEND:
				case VIRTUAL:
				case TYPEDEF:
				case EXTERN:
				case STATIC:
				case AUTO:
				case REGISTER:
				case PASCAL:
				case FORTRAN:
				case VOID:
				case CHAR:
				case SHORT:
				case INT:
				case LONG:
				case FLOAT:
				case DOUBLE:
				case SIGNED:
				case UNSIGNED:
				case STRUCT:
				case UNION:
				case CLASS:
				case ENUM:
				case CONST:
				case VOLATILE:
					switch (w_nexttoken())
					{
						case TYPEDEF:
						case EXTERN:
						case STATIC:
						case AUTO:
						case REGISTER:
						case PASCAL:
						case FORTRAN:
							{
								Storage_thing* w_rvstorage_class_specifier;
#line 748 "ccgram.w"
								 w_rvstorage_class_specifier = &(*w_rr); 
#line 3643 "parser.cc"

								{/*storage_class_specifier*/
									Storage_thing* *w_rr = &w_rvstorage_class_specifier;
									struct w_resynclink *w_lnk = &w_link;
									struct w_resynclink w_link;

									w_link.prev = w_lnk;
									w_link.resync = w_resync167;

									switch (w_nexttoken())
									{
										case TYPEDEF:
											{
												w_scantoken(TYPEDEF, &w_link, w_resync13);
#line 801 "ccgram.w"
												 (*w_rr)->merge_storage(TYPEDEF); 
#line 3660 "parser.cc"
											}

											break;

										case EXTERN:
											{
												w_scantoken(EXTERN, &w_link, w_resync98);
#line 803 "ccgram.w"
												 (*w_rr)->merge_storage(EXTERN); 
#line 3670 "parser.cc"
											}

											break;

										case STATIC:
											{
												w_scantoken(STATIC, &w_link, w_resync17);
#line 805 "ccgram.w"
												 (*w_rr)->merge_storage(STATIC); 
#line 3680 "parser.cc"
											}

											break;

										case AUTO:
											{
												w_scantoken(AUTO, &w_link, w_resync111);
#line 807 "ccgram.w"
												 (*w_rr)->merge_storage(AUTO); 
#line 3690 "parser.cc"
											}

											break;

										case REGISTER:
											{
												w_scantoken(REGISTER, &w_link, w_resync28);
#line 809 "ccgram.w"
												 (*w_rr)->merge_storage(REGISTER); 
#line 3700 "parser.cc"
											}

											break;

										case PASCAL:
											{
												w_scantoken(PASCAL, &w_link, w_resync144);
#line 812 "ccgram.w"
												 merge_flags((*w_rr)->flags, S_PASCAL); 
#line 3710 "parser.cc"
											}

											break;

										case FORTRAN:
											{
												w_scantoken(FORTRAN, &w_link, w_resync75);
#line 815 "ccgram.w"
												 merge_flags((*w_rr)->flags, S_FORTRAN); 
#line 3720 "parser.cc"
											}

											break;

										default:
											w_err_illegal("storage_class_specifier");
											w_rc = W_RETERR;
											break;
									}
									w_rc = W_RETOK;
								}/*storage_class_specifier*/

							}

							break;

						case CLASSNAME:
						case '~':
						case TYPENAME:
						case VOID:
						case CHAR:
						case SHORT:
						case INT:
						case LONG:
						case FLOAT:
						case DOUBLE:
						case SIGNED:
						case UNSIGNED:
						case STRUCT:
						case UNION:
						case CLASS:
						case ENUM:
							{
								Storage_thing* w_rvtype_specifier;
#line 750 "ccgram.w"
								 w_rvtype_specifier = &(*w_rr); 
#line 3757 "parser.cc"
								w_rc = w_ftype_specifier(&w_link, w_resync104, &w_rvtype_specifier);
#line 751 "ccgram.w"
								
				g_parse_type = FALSE;

				if (w_rc == W_RETERR)
					goto done_d_s;
			
#line 3766 "parser.cc"
							}

							break;

						case CONST:
						case VOLATILE:
							{
								int w_rvtype_qualifier;
								w_rc = w_ftype_qualifier(&w_link, w_resync68, &w_rvtype_qualifier);
#line 758 "ccgram.w"
								 merge_flags((*w_rr).flags, w_rvtype_qualifier); 
#line 3778 "parser.cc"
							}

							break;

						case INLINE:
							{
								w_rc = w_scantoken(INLINE, &w_link, w_resync9);
#line 761 "ccgram.w"
								 merge_flags((*w_rr).flags, S_INLINE); 
#line 3788 "parser.cc"
							}

							break;

						case FRIEND:
							{
								w_rc = w_scantoken(FRIEND, &w_link, w_resync88);
#line 762 "ccgram.w"
								 merge_flags((*w_rr).flags, S_FRIEND); 
#line 3798 "parser.cc"
							}

							break;

						case VIRTUAL:
							{
								w_rc = w_scantoken(VIRTUAL, &w_link, w_resync276);
#line 763 "ccgram.w"
								 merge_flags((*w_rr).flags, S_VIRTUAL); 
#line 3808 "parser.cc"
							}

							break;

						default:
							w_err_illegal("declaration_specifiers");
							break;
					}
					continue;

				default:
					break;
				}

				break; /*for(;;)*/
			}

		}/*__P24*/

#line 765 "ccgram.w"
		 done_d_s: g_parse_type = TRUE; 
#line 3830 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fctor_initializer(struct w_resynclink *w_lnk, int *w_resync)
#else
w_fctor_initializer(w_lnk, w_resync)
struct w_resynclink *w_lnk;
int *w_resync;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case ':':
		{
			w_scantoken(':', &w_link, w_resync194);
			w_fmem_initializer(&w_link, w_resync76);

			{/*__P25*/
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync59;

				while (w_nexttoken() == ',')
				{
					{
						w_scantoken(',', &w_link, w_resync76);
						w_fmem_initializer(&w_link, w_resync161);
					}

				}
			}/*__P25*/

		}

		break;

	default:
		{
		}

		break;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fmem_initializer(struct w_resynclink *w_lnk, int *w_resync)
#else
w_fmem_initializer(w_lnk, w_resync)
struct w_resynclink *w_lnk;
int *w_resync;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Expr_arr* w_rvargument_expr_list;

		{/*__P26*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync235;

			switch (w_nexttoken())
			{
				case CLASSNAME:
					{
						w_scantoken(CLASSNAME, &w_link, w_resync250);
					}

					break;

				case IDENTIFIER:
					{
						w_scantoken(IDENTIFIER, &w_link, w_resync209);
					}

					break;

				default:
					w_err_illegal("mem_initializer");
					break;
			}
		}/*__P26*/

#line 775 "ccgram.w"
		 const char *sym = g_tok->text; 
#line 3936 "parser.cc"
		w_scantoken('(', &w_link, w_resync191);
		w_fargument_expr_list(&w_link, w_resync191, &w_rvargument_expr_list);
		w_scantoken(')', &w_link, w_resync100);
#line 777 "ccgram.w"
		 do_member_init(sym, w_rvargument_expr_list); 
#line 3942 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_finit_declarator(struct w_resynclink *w_lnk, int *w_resync, Storage_thing *w_rr)
#else
w_finit_declarator(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Storage_thing *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Storage_thing* w_rvdeclarator;
#line 787 "ccgram.w"
		 w_rvdeclarator = &(*w_rr); 
#line 3967 "parser.cc"
		w_fdeclarator(&w_link, w_resync147, &w_rvdeclarator);
#line 789 "ccgram.w"
		 (*w_rr).symbol = w_rvdeclarator->do_declaration(); 
#line 3971 "parser.cc"

		{/*__P28*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync110;

			switch (w_nexttoken())
			{
				case '=':
					{
						Expr_node* w_rvinitializer;
						w_scantoken('=', &w_link, w_resync180);
						w_finitializer(&w_link, w_resync252, &w_rvinitializer);
#line 792 "ccgram.w"
						 do_initializer((*w_rr).symbol, w_rvinitializer); 
#line 3989 "parser.cc"
					}

					break;

				default:
					{
#line 794 "ccgram.w"
						 do_initializer((*w_rr).symbol, NULL); 
#line 3998 "parser.cc"
					}

					break;
			}
		}/*__P28*/

#line 796 "ccgram.w"
		 do_tree((*w_rr).symbol); 
#line 4007 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_ftype_specifier(struct w_resynclink *w_lnk, int *w_resync, Storage_thing* *w_rr)
#else
w_ftype_specifier(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Storage_thing* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case VOID:
		{
			w_scantoken(VOID, &w_link, w_resync255);
#line 819 "ccgram.w"
			 (*w_rr)->merge_basic(VOID); 
#line 4035 "parser.cc"
		}

		break;

	case CHAR:
		{
			w_scantoken(CHAR, &w_link, w_resync33);
#line 820 "ccgram.w"
			 (*w_rr)->merge_basic(CHAR); 
#line 4045 "parser.cc"
		}

		break;

	case SHORT:
		{
			w_scantoken(SHORT, &w_link, w_resync155);
#line 821 "ccgram.w"
			 merge_flags((*w_rr)->flags, S_SHORT); 
#line 4055 "parser.cc"
		}

		break;

	case INT:
		{
			w_scantoken(INT, &w_link, w_resync94);
#line 822 "ccgram.w"
			 (*w_rr)->merge_basic(INT); 
#line 4065 "parser.cc"
		}

		break;

	case LONG:
		{
			w_scantoken(LONG, &w_link, w_resync11);
#line 823 "ccgram.w"
			 merge_flags((*w_rr)->flags, S_LONG); 
#line 4075 "parser.cc"
		}

		break;

	case FLOAT:
		{
			w_scantoken(FLOAT, &w_link, w_resync92);
#line 824 "ccgram.w"
			 (*w_rr)->merge_basic(FLOAT); 
#line 4085 "parser.cc"
		}

		break;

	case DOUBLE:
		{
			w_scantoken(DOUBLE, &w_link, w_resync7);
#line 825 "ccgram.w"
			 (*w_rr)->merge_basic(DOUBLE); 
#line 4095 "parser.cc"
		}

		break;

	case SIGNED:
		{
			w_scantoken(SIGNED, &w_link, w_resync82);
#line 826 "ccgram.w"
			 merge_flags((*w_rr)->flags, S_SIGNED); 
#line 4105 "parser.cc"
		}

		break;

	case UNSIGNED:
		{
			w_scantoken(UNSIGNED, &w_link, w_resync272);
#line 827 "ccgram.w"
			 merge_flags((*w_rr)->flags, S_UNSIGNED); 
#line 4115 "parser.cc"
		}

		break;

	case STRUCT:
	case UNION:
	case CLASS:
		{
			Struct_type* w_rvstruct_union_specifier;

			{/*struct_union_specifier*/
				Struct_type* *w_rr = &w_rvstruct_union_specifier;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;
#line 887 "ccgram.w"
				 Scope *scope = g_curr_scope; 
#line 4132 "parser.cc"

				w_link.prev = w_lnk;
				w_link.resync = w_resync143;

				{
					int w_rvsu;

					{/*struct_or_union*/
						int *w_rr = &w_rvsu;
						struct w_resynclink *w_lnk = &w_link;
						struct w_resynclink w_link;

						w_link.prev = w_lnk;
						w_link.resync = w_resync234;

						switch (w_nexttoken())
						{
							case STRUCT:
								{
									w_scantoken(STRUCT, &w_link, w_resync158);
#line 916 "ccgram.w"
									 (*w_rr) = STRUCT; 
#line 4155 "parser.cc"
								}

								break;

							case UNION:
								{
									w_scantoken(UNION, &w_link, w_resync244);
#line 917 "ccgram.w"
									 (*w_rr) = UNION; 
#line 4165 "parser.cc"
								}

								break;

							case CLASS:
								{
									w_scantoken(CLASS, &w_link, w_resync149);
#line 918 "ccgram.w"
									 (*w_rr) = CLASS; 
#line 4175 "parser.cc"
								}

								break;

							default:
								w_err_illegal("struct_or_union");
								break;
						}
					}/*struct_or_union*/


					{/*__P29*/
						struct w_resynclink *w_lnk = &w_link;
						struct w_resynclink w_link;

						w_link.prev = w_lnk;
						w_link.resync = w_resync205;

						switch (w_nexttoken())
						{
							case IDENTIFIER:
							case CLASSNAME:
							case TYPENAME:
								{
									const char* w_rvidentifier;
									w_fidentifier(&w_link, w_resync5, &w_rvidentifier);

									{/*__P30*/
										struct w_resynclink *w_lnk = &w_link;
										struct w_resynclink w_link;

										w_link.prev = w_lnk;
										w_link.resync = w_resync154;

										switch (w_nexttoken())
										{
											case '{':
											case ':':
												{
													Struct_type* w_rvbase_spec;
													struct Struct_decl w_rvsdl;
#line 892 "ccgram.w"
													
						(*w_rr) = do_sue_definition(w_rvsu, w_rvidentifier);
						w_rvsdl.type = w_rvbase_spec = (*w_rr);
						w_rvsdl.access = w_rvsu == CLASS ? PRIVATE : PUBLIC;

						if (g_cplusplus)
							g_curr_scope = (*w_rr)->scope();
					
#line 4226 "parser.cc"

													{/*base_spec*/
														Struct_type* *w_rr = &w_rvbase_spec;
														struct w_resynclink *w_lnk = &w_link;
														struct w_resynclink w_link;

														w_link.prev = w_lnk;
														w_link.resync = w_resync246;

														switch (w_nexttoken())
														{
															case ':':
																{
																	Struct_type* w_rvbase_specifier;
																	w_scantoken(':', &w_link, w_resync279);
#line 923 "ccgram.w"
																	
				if ((*w_rr)->base() != NULL)
					error(STRUCT_ALREADY_DEFINED, (*w_rr)->name(),
							(*w_rr)->scope()->line());

				w_rvbase_specifier = (*w_rr);
			
#line 4250 "parser.cc"
																	w_fbase_specifier(&w_link, w_resync278, &w_rvbase_specifier);

																	{/*__P31*/
																		struct w_resynclink *w_lnk = &w_link;
																		struct w_resynclink w_link;

																		w_link.prev = w_lnk;
																		w_link.resync = w_resync59;

																		while (w_nexttoken() == ',')
																		{
																			{
																				Struct_type* w_rvbase_specifier;
																				w_scantoken(',', &w_link, w_resync278);
																				w_fbase_specifier(&w_link, w_resync81, &w_rvbase_specifier);
#line 933 "ccgram.w"
																				 error(MULTIPLE_INHERITANCE_NOT_SUPPORTED); 
#line 4268 "parser.cc"
																			}

																		}
																	}/*__P31*/

																}

																break;

															default:
																{
																}

																break;
														}
													}/*base_spec*/

													w_scantoken('{', &w_link, w_resync246);
													w_fstruct_declaration_list(&w_link, w_resync246, &w_rvsdl);
													w_scantoken('}', &w_link, w_resync109);
												}

												break;

											default:
												{
#line 902 "ccgram.w"
													 (*w_rr) = do_sue_declaration(w_rvsu, w_rvidentifier); 
#line 4297 "parser.cc"
												}

												break;
										}
									}/*__P30*/

								}

								break;

							case '{':
								{
									struct Struct_decl w_rvsdl;
#line 905 "ccgram.w"
									
					(*w_rr) = do_sue_definition(w_rvsu, make_unnamed_name());
					w_rvsdl.type = (*w_rr);
					w_rvsdl.access = w_rvsu == CLASS ? PRIVATE : PUBLIC;
				
#line 4317 "parser.cc"
									w_scantoken('{', &w_link, w_resync246);
									w_fstruct_declaration_list(&w_link, w_resync246, &w_rvsdl);
									w_scantoken('}', &w_link, w_resync109);
								}

								break;

							default:
								w_err_illegal("struct_union_specifier");
								break;
						}
					}/*__P29*/

#line 912 "ccgram.w"
					 g_curr_scope = scope; 
#line 4333 "parser.cc"
				}

			}/*struct_union_specifier*/

#line 828 "ccgram.w"
			 (*w_rr)->type = w_rvstruct_union_specifier; 
#line 4340 "parser.cc"
		}

		break;

	case ENUM:
		{
			Enum_type* w_rvenum_specifier;

			{/*enum_specifier*/
				Enum_type* *w_rr = &w_rvenum_specifier;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;
#line 1148 "ccgram.w"
				 Scope *scope = g_curr_scope; 
#line 4355 "parser.cc"

				w_link.prev = w_lnk;
				w_link.resync = w_resync216;

				{
					w_scantoken(ENUM, &w_link, w_resync6);

					{/*__P44*/
						struct w_resynclink *w_lnk = &w_link;
						struct w_resynclink w_link;

						w_link.prev = w_lnk;
						w_link.resync = w_resync205;

						switch (w_nexttoken())
						{
							case IDENTIFIER:
							case CLASSNAME:
							case TYPENAME:
								{
									const char* w_rvidentifier;
									w_fidentifier(&w_link, w_resync86, &w_rvidentifier);

									{/*__P45*/
										struct w_resynclink *w_lnk = &w_link;
										struct w_resynclink w_link;

										w_link.prev = w_lnk;
										w_link.resync = w_resync240;

										switch (w_nexttoken())
										{
											case '{':
												{
													Enum_type* w_rvenumerator_list;
													w_scantoken('{', &w_link, w_resync70);
#line 1153 "ccgram.w"
													 (*w_rr) = w_rvenumerator_list =
						(Enum_type*)do_sue_definition(ENUM, w_rvidentifier); 
#line 4395 "parser.cc"
													w_fenumerator_list(&w_link, w_resync249, &w_rvenumerator_list);
													w_scantoken('}', &w_link, w_resync109);
												}

												break;

											default:
												{
#line 1157 "ccgram.w"
													 (*w_rr) = (Enum_type*)do_sue_declaration(ENUM, w_rvidentifier); 
#line 4406 "parser.cc"
												}

												break;
										}
									}/*__P45*/

								}

								break;

							case '{':
								{
									Enum_type* w_rvenumerator_list;
#line 1159 "ccgram.w"
									 (*w_rr) = w_rvenumerator_list = (Enum_type*)do_sue_definition(ENUM,
								make_unnamed_name()); 
#line 4423 "parser.cc"
									w_scantoken('{', &w_link, w_resync70);
									w_fenumerator_list(&w_link, w_resync249, &w_rvenumerator_list);
									w_scantoken('}', &w_link, w_resync109);
								}

								break;

							default:
								w_err_illegal("enum_specifier");
								break;
						}
					}/*__P44*/

#line 1163 "ccgram.w"
					 g_curr_scope = scope; 
#line 4439 "parser.cc"
				}

			}/*enum_specifier*/

#line 829 "ccgram.w"
			 (*w_rr)->type = w_rvenum_specifier; 
#line 4446 "parser.cc"
		}

		break;

	case TYPENAME:
		{
			w_scantoken(TYPENAME, &w_link, w_resync99);
#line 831 "ccgram.w"
			
			if (g_sym == NULL)
				bug("wacco type_specifier: got %s - not TYPENAME",
						g_tok->text);

			(*w_rr)->type = g_sym->type();
		
#line 4462 "parser.cc"
		}

		break;

	case CLASSNAME:
		{
			w_scantoken(CLASSNAME, &w_link, w_resync250);
#line 839 "ccgram.w"
			
			const char *name = g_tok->text;

			if (w_nexttoken() == '('/*)*/ &&
					g_curr_scope->symbol() &&
					g_curr_scope->symbol()->name() == name)
			{
				// constructor declaration within its class
				w_pushback(w_currtoken);
				w_currtoken = X_CTOR;
				g_tok->text = name;
			}
			else if (w_currtoken == COLONCOLON)
			{
				// member function declaration outside a class
				w_pushback(w_currtoken);
				w_currtoken = X_MEMFUNC;
				g_tok->text = name;
			}
			else
			{
				// some other use of this type
				if (g_sym == NULL)
					bug("wacco type_specifier: got %s - not TYPENAME",
							g_tok->text);

				(*w_rr)->type = g_sym->type();
			}
		
#line 4500 "parser.cc"
		}

		break;

	case '~':
		{
			w_scantoken('~', &w_link, w_resync54);
#line 869 "ccgram.w"
			
				// this is to parse "operator~"
				if (w_nexttoken() != CLASSNAME)
				{
					w_pushback(w_currtoken);
					w_currtoken = X_TILDE;
					goto no_classname;
				}
			
#line 4518 "parser.cc"
			w_scantoken(CLASSNAME, &w_link, w_resync250);
#line 879 "ccgram.w"
			
				w_currtoken = X_DTOR;
			no_classname:
				;
			
#line 4526 "parser.cc"
		}

		break;

	default:
		w_err_illegal("type_specifier");
		return W_RETERR;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fbase_specifier(struct w_resynclink *w_lnk, int *w_resync, Struct_type* *w_rr)
#else
w_fbase_specifier(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Struct_type* *w_rr;
#endif
{
	struct w_resynclink w_link;
#line 939 "ccgram.w"
	 boolean isclass = (*w_rr)->is_class(); 
#line 4552 "parser.cc"

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case CLASSNAME:
		{
			w_scantoken(CLASSNAME, &w_link, w_resync250);
#line 941 "ccgram.w"
			
			warn(MISSING_BASE_ACCESS_SPECIFIER);

			(*w_rr)->add_base_class(g_sym->type(),
					isclass ? PRIVATE : PUBLIC, FALSE);
		
#line 4569 "parser.cc"
		}

		break;

	case VIRTUAL:
		{
			int w_rv_;
			w_scantoken(VIRTUAL, &w_link, w_resync2);

			{/*__P32*/
				int *w_rr = &w_rv_;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync182;

				switch (w_nexttoken())
				{
					case PRIVATE:
					case PUBLIC:
					case PROTECTED:
						{
							int w_rvaccess_specifier;
							w_faccess_specifier(&w_link, w_resync263, &w_rvaccess_specifier);
#line 948 "ccgram.w"
							 (*w_rr) = w_rvaccess_specifier; 
#line 4597 "parser.cc"
						}

						break;

					default:
						{
#line 949 "ccgram.w"
							 (*w_rr) = isclass ? PRIVATE : PUBLIC; 
#line 4606 "parser.cc"
						}

						break;
				}
			}/*__P32*/

			w_scantoken(CLASSNAME, &w_link, w_resync250);
#line 952 "ccgram.w"
			 (*w_rr)->add_base_class(g_sym->type(), w_rv_, TRUE); 
#line 4616 "parser.cc"
		}

		break;

	case PRIVATE:
	case PUBLIC:
	case PROTECTED:
		{
			int w_rvaccess_specifier;
			boolean w_rv_;
			w_faccess_specifier(&w_link, w_resync2, &w_rvaccess_specifier);

			{/*__P33*/
				boolean *w_rr = &w_rv_;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync169;

				switch (w_nexttoken())
				{
					case VIRTUAL:
						{
							w_scantoken(VIRTUAL, &w_link, w_resync276);
#line 954 "ccgram.w"
							 (*w_rr) = TRUE; 
#line 4644 "parser.cc"
						}

						break;

					default:
						{
#line 955 "ccgram.w"
							 (*w_rr) = FALSE; 
#line 4653 "parser.cc"
						}

						break;
				}
			}/*__P33*/

			w_scantoken(CLASSNAME, &w_link, w_resync250);
#line 958 "ccgram.w"
			 (*w_rr)->add_base_class(g_sym->type(), w_rvaccess_specifier, w_rv_); 
#line 4663 "parser.cc"
		}

		break;

	default:
		w_err_illegal("base_specifier");
		return W_RETERR;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fstruct_declaration_list(struct w_resynclink *w_lnk, int *w_resync, struct Struct_decl *w_rr)
#else
w_fstruct_declaration_list(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
struct Struct_decl *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{

		{/*__P34*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;
			int w_rc = W_RETOK;

			w_link.prev = w_lnk;
			w_link.resync = w_resync116;

			goto w_oneplus_2;

			for (;;)
			{
				switch (w_nexttoken())
				{
				w_oneplus_2:
				case '(':
				case IDENTIFIER:
				case CLASSNAME:
				case '~':
				case TYPENAME:
				case ',':
				case '&':
				case '*':
				case OPERATOR:
				case '{':
				case ':':
				case ';':
				case X_CTORINIT:
				case INLINE:
				case FRIEND:
				case VIRTUAL:
				case TYPEDEF:
				case EXTERN:
				case STATIC:
				case AUTO:
				case REGISTER:
				case PASCAL:
				case FORTRAN:
				case VOID:
				case CHAR:
				case SHORT:
				case INT:
				case LONG:
				case FLOAT:
				case DOUBLE:
				case SIGNED:
				case UNSIGNED:
				case STRUCT:
				case UNION:
				case CLASS:
				case PRIVATE:
				case PUBLIC:
				case PROTECTED:
				case ENUM:
				case CONST:
				case VOLATILE:
				case X_CTOR:
				case X_MEMFUNC:
				case X_DTOR:
					switch (w_nexttoken())
					{
						case '(':
						case IDENTIFIER:
						case CLASSNAME:
						case '~':
						case TYPENAME:
						case ',':
						case '&':
						case '*':
						case OPERATOR:
						case '{':
						case ':':
						case ';':
						case X_CTORINIT:
						case INLINE:
						case FRIEND:
						case VIRTUAL:
						case TYPEDEF:
						case EXTERN:
						case STATIC:
						case AUTO:
						case REGISTER:
						case PASCAL:
						case FORTRAN:
						case VOID:
						case CHAR:
						case SHORT:
						case INT:
						case LONG:
						case FLOAT:
						case DOUBLE:
						case SIGNED:
						case UNSIGNED:
						case STRUCT:
						case UNION:
						case CLASS:
						case ENUM:
						case CONST:
						case VOLATILE:
						case X_CTOR:
						case X_MEMFUNC:
						case X_DTOR:
							{
								struct Struct_decl* w_rvstruct_declaration;
#line 963 "ccgram.w"
								 w_rvstruct_declaration = &(*w_rr); 
#line 4799 "parser.cc"
								w_rc = w_fstruct_declaration(&w_link, w_resync60, &w_rvstruct_declaration);
#line 965 "ccgram.w"
								
				if (w_rc != W_RETOK)
					goto done;
			
#line 4806 "parser.cc"
							}

							break;

						case PRIVATE:
						case PUBLIC:
						case PROTECTED:
							{
								int w_rvaccess_specifier;
								w_rc = w_faccess_specifier(&w_link, w_resync179, &w_rvaccess_specifier);
								w_rc = w_scantoken(':', &w_link, w_resync141);
#line 970 "ccgram.w"
								 (*w_rr).access = w_rvaccess_specifier; 
#line 4820 "parser.cc"
							}

							break;
					}
					continue;

				default:
					break;
				}

				break; /*for(;;)*/
			}

		}/*__P34*/

#line 972 "ccgram.w"
		 done:		; 
#line 4838 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_faccess_specifier(struct w_resynclink *w_lnk, int *w_resync, int *w_rr)
#else
w_faccess_specifier(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
int *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case PRIVATE:
		{
			w_scantoken(PRIVATE, &w_link, w_resync251);
#line 976 "ccgram.w"
			 (*w_rr) = PRIVATE; 
#line 4866 "parser.cc"
		}

		break;

	case PUBLIC:
		{
			w_scantoken(PUBLIC, &w_link, w_resync163);
#line 977 "ccgram.w"
			 (*w_rr) = PUBLIC; 
#line 4876 "parser.cc"
		}

		break;

	case PROTECTED:
		{
			w_scantoken(PROTECTED, &w_link, w_resync257);
#line 978 "ccgram.w"
			 (*w_rr) = PROTECTED; 
#line 4886 "parser.cc"
		}

		break;

	default:
		w_err_illegal("access_specifier");
		return W_RETERR;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fstruct_declaration(struct w_resynclink *w_lnk, int *w_resync, struct Struct_decl* *w_rr)
#else
w_fstruct_declaration(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
struct Struct_decl* *w_rr;
#endif
{
	struct w_resynclink w_link;
#line 983 "ccgram.w"
	 Struct_type *stype = (*w_rr)->type; 
#line 4912 "parser.cc"

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
#line 985 "ccgram.w"
		
				Storage_thing declspec;
				boolean hadspec = FALSE;

				if (w_nexttoken() == ';')
				{
					w_skiptoken();		// ';'
					return W_RETOK;
				}
			
#line 4929 "parser.cc"

		{/*__P35*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync60;

			switch (w_nexttoken())
			{
				case CLASSNAME:
				case '~':
				case TYPENAME:
				case INLINE:
				case FRIEND:
				case VIRTUAL:
				case TYPEDEF:
				case EXTERN:
				case STATIC:
				case AUTO:
				case REGISTER:
				case PASCAL:
				case FORTRAN:
				case VOID:
				case CHAR:
				case SHORT:
				case INT:
				case LONG:
				case FLOAT:
				case DOUBLE:
				case SIGNED:
				case UNSIGNED:
				case STRUCT:
				case UNION:
				case CLASS:
				case ENUM:
				case CONST:
				case VOLATILE:
					{
						Storage_thing w_rvds;
						w_fdeclaration_specifiers(&w_link, w_resync210, &w_rvds);
#line 997 "ccgram.w"
						
					declspec = w_rvds;
					declspec.make_type();
					hadspec = TRUE;
				
#line 4977 "parser.cc"

						{/*__P36*/
							struct w_resynclink *w_lnk = &w_link;
							struct w_resynclink w_link;

							w_link.prev = w_lnk;
							w_link.resync = w_resync273;

							switch (w_nexttoken())
							{
								case ';':
									{
										w_scantoken(';', &w_link, w_resync222);
#line 1004 "ccgram.w"
										
						if (w_rvds.type != NULL)
						{
							if (w_rvds.type->issu())
								do_unnamed_substruct(w_rvds.type, stype);
							else if (!w_rvds.type->isenum())
							{
								error(MUST_DECLARE_A_MEMBER_NAME);
								w_rvds.type->free();
								return W_RETERR;
							}
						}

						return W_RETOK;		// continue;
					
#line 5007 "parser.cc"
									}

									break;

								default:
									{
									}

									break;
							}
						}/*__P36*/

					}

					break;

				default:
					{
					}

					break;
			}
		}/*__P35*/

#line 1023 "ccgram.w"
		
				if (!hadspec)
					declspec.make_type();

				Storage_thing decl = declspec;
			
#line 5039 "parser.cc"

		{/*__P37*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync32;

			switch (w_nexttoken())
			{
				case '(':
				case IDENTIFIER:
				case CLASSNAME:
				case '~':
				case TYPENAME:
				case '&':
				case '*':
				case OPERATOR:
				case X_CTOR:
				case X_MEMFUNC:
				case X_DTOR:
					{
						Storage_thing* w_rvdeclarator;
#line 1030 "ccgram.w"
						 w_rvdeclarator = &decl; 
#line 5065 "parser.cc"
						w_fdeclarator(&w_link, w_resync35, &w_rvdeclarator);
#line 1032 "ccgram.w"
						
					// this is since a following ':' could be a bitfield
					if (g_cplusplus && decl.type->isfunc() &&
							w_nexttoken() == ':')
					{
						w_pushback(w_currtoken);
						w_currtoken = X_CTORINIT;
					}
				
#line 5077 "parser.cc"
					}

					break;

				default:
					{
					}

					break;
			}
		}/*__P37*/


		{/*__P38*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync229;

			switch (w_nexttoken())
			{
				case ',':
				case ':':
				case ';':
					{
						short w_rvbit_field;
						w_fbit_field(&w_link, w_resync220, &w_rvbit_field);
#line 1045 "ccgram.w"
						
					if (!hadspec)
					{
						error(MISSING_TYPE_FOR_DECLARATION);
						return W_RETERR;
					}

					//decl.symbol = decl.do_declaration();
					decl.bitfield = w_rvbit_field;
					decl.do_member_decl(stype);
				
#line 5118 "parser.cc"

						{/*__P39*/
							struct w_resynclink *w_lnk = &w_link;
							struct w_resynclink w_link;

							w_link.prev = w_lnk;
							w_link.resync = w_resync105;

							switch (w_nexttoken())
							{
								case ',':
									{
										struct Struct_decl w_rvstruct_declarator_list;
										w_scantoken(',', &w_link, w_resync214);
#line 1058 "ccgram.w"
										
						w_rvstruct_declarator_list.stor = &declspec;
						w_rvstruct_declarator_list.type = stype;
					
#line 5138 "parser.cc"

										{/*struct_declarator_list*/
											struct Struct_decl *w_rr = &w_rvstruct_declarator_list;
											struct w_resynclink *w_lnk = &w_link;
											struct w_resynclink w_link;

											w_link.prev = w_lnk;
											w_link.resync = w_resync214;

											{
												struct Struct_decl w_rvstruct_declarator;
#line 1130 "ccgram.w"
												 w_rvstruct_declarator = (*w_rr); 
#line 5152 "parser.cc"
												w_fstruct_declarator(&w_link, w_resync214, &w_rvstruct_declarator);

												{/*__P42*/
													struct w_resynclink *w_lnk = &w_link;
													struct w_resynclink w_link;

													w_link.prev = w_lnk;
													w_link.resync = w_resync59;

													while (w_nexttoken() == ',')
													{
														{
															struct Struct_decl w_rvstruct_declarator;
															w_scantoken(',', &w_link, w_resync214);
#line 1132 "ccgram.w"
															 w_rvstruct_declarator = (*w_rr); 
#line 5169 "parser.cc"
															w_fstruct_declarator(&w_link, w_resync233, &w_rvstruct_declarator);
														}

													}
												}/*__P42*/

											}

										}/*struct_declarator_list*/

									}

									break;

								default:
									{
									}

									break;
							}
						}/*__P39*/

						w_scantoken(';', &w_link, w_resync222);
					}

					break;

				case '{':
				case X_CTORINIT:
					{
						boolean w_rvdeclaration_list;
						Node_arr* w_rvstatement_list;
#line 1068 "ccgram.w"
						
					if (decl.type->isfunc())
						((Function_type*)decl.type)->classname(
								stype->symbol());

					Scope *sav = g_curr_scope;
					Symbol *func = do_function_def(decl);

					if (!g_cplusplus)
						error(MEMFUNCS_ARE_NOT_ISO);
				
#line 5214 "parser.cc"

						{/*__P40*/
							struct w_resynclink *w_lnk = &w_link;
							struct w_resynclink w_link;

							w_link.prev = w_lnk;
							w_link.resync = w_resync134;

							switch (w_nexttoken())
							{
								case X_CTORINIT:
									{
										w_scantoken(X_CTORINIT, &w_link, w_resync80);
										w_fctor_initializer(&w_link, w_resync39);
									}

									break;

								default:
									{
									}

									break;
							}
						}/*__P40*/

						w_scantoken('{', &w_link, w_resync256);
						w_fdeclaration_list(&w_link, w_resync256, &w_rvdeclaration_list);
						w_fstatement_list(&w_link, w_resync245, &w_rvstatement_list);
						w_scantoken('}', &w_link, w_resync109);
#line 1086 "ccgram.w"
						
					fini_function_def(func, w_rvstatement_list);
					g_curr_scope = sav;
				
#line 5250 "parser.cc"
					}

					break;
			}
		}/*__P38*/

	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fbit_field(struct w_resynclink *w_lnk, int *w_resync, short *w_rr)
#else
w_fbit_field(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
short *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case ':':
		{
			Expr_node* w_rvconstant_expr;
			w_scantoken(':', &w_link, w_resync43);
			w_fconstant_expr(&w_link, w_resync145, &w_rvconstant_expr);
#line 1095 "ccgram.w"
			
				Const_expr width;
				w_rvconstant_expr->eval_const_expr(width);
				delete w_rvconstant_expr;

				if (width.ival != NULL)
					(*w_rr) = (short)width.ival->get();

				if ((*w_rr) < 0)
				{
					error(NEGATIVE_VALUE_FOR_BITFIELD);
					(*w_rr) = -1;
				}
			
#line 5299 "parser.cc"
		}

		break;

	default:
		{
#line 1109 "ccgram.w"
			 (*w_rr) = -1; 
#line 5308 "parser.cc"
		}

		break;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fspecifier_qualifier_list(struct w_resynclink *w_lnk, int *w_resync, Storage_thing *w_rr)
#else
w_fspecifier_qualifier_list(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Storage_thing *w_rr;
#endif
{
	struct w_resynclink w_link;
#line 1113 "ccgram.w"
	 g_parse_type = (*w_rr).type ? FALSE : TRUE; 
#line 5330 "parser.cc"

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{

		{/*__P41*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;
			int w_rc = W_RETOK;

			w_link.prev = w_lnk;
			w_link.resync = w_resync63;

			goto w_oneplus_3;

			for (;;)
			{
				switch (w_nexttoken())
				{
				w_oneplus_3:
				case CLASSNAME:
				case '~':
				case TYPENAME:
				case VOID:
				case CHAR:
				case SHORT:
				case INT:
				case LONG:
				case FLOAT:
				case DOUBLE:
				case SIGNED:
				case UNSIGNED:
				case STRUCT:
				case UNION:
				case CLASS:
				case ENUM:
				case CONST:
				case VOLATILE:
					switch (w_nexttoken())
					{
						case CLASSNAME:
						case '~':
						case TYPENAME:
						case VOID:
						case CHAR:
						case SHORT:
						case INT:
						case LONG:
						case FLOAT:
						case DOUBLE:
						case SIGNED:
						case UNSIGNED:
						case STRUCT:
						case UNION:
						case CLASS:
						case ENUM:
							{
								Storage_thing* w_rvtype_specifier;
#line 1115 "ccgram.w"
								 w_rvtype_specifier = &(*w_rr); 
#line 5392 "parser.cc"
								w_rc = w_ftype_specifier(&w_link, w_resync104, &w_rvtype_specifier);
#line 1117 "ccgram.w"
								
				g_parse_type = FALSE;

				if (w_rc == W_RETERR)
					goto done_s_q_l;
			
#line 5401 "parser.cc"
							}

							break;

						case CONST:
						case VOLATILE:
							{
								int w_rvtype_qualifier;
								w_rc = w_ftype_qualifier(&w_link, w_resync68, &w_rvtype_qualifier);
#line 1124 "ccgram.w"
								 merge_flags((*w_rr).flags, w_rvtype_qualifier); 
#line 5413 "parser.cc"
							}

							break;

						default:
							w_err_illegal("specifier_qualifier_list");
							break;
					}
					continue;

				default:
					break;
				}

				break; /*for(;;)*/
			}

		}/*__P41*/

#line 1126 "ccgram.w"
		 done_s_q_l: g_parse_type = TRUE; 
#line 5435 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fstruct_declarator(struct w_resynclink *w_lnk, int *w_resync, struct Struct_decl *w_rr)
#else
w_fstruct_declarator(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
struct Struct_decl *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Storage_thing w_rv_;
		short w_rvbit_field;
#line 1136 "ccgram.w"
		 w_rv_ = *(*w_rr).stor; 
#line 5461 "parser.cc"

		{/*__P43*/
			Storage_thing *w_rr = &w_rv_;
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync233;

			switch (w_nexttoken())
			{
				case '(':
				case IDENTIFIER:
				case CLASSNAME:
				case '~':
				case TYPENAME:
				case '&':
				case '*':
				case OPERATOR:
				case X_CTOR:
				case X_MEMFUNC:
				case X_DTOR:
					{
						Storage_thing* w_rvdeclarator;
#line 1137 "ccgram.w"
						 w_rvdeclarator = &(*w_rr); 
#line 5488 "parser.cc"
						w_fdeclarator(&w_link, w_resync35, &w_rvdeclarator);
					}

					break;

				default:
					{
					}

					break;
			}
		}/*__P43*/

		w_fbit_field(&w_link, w_resync39, &w_rvbit_field);
#line 1141 "ccgram.w"
		
				w_rv_.bitfield = w_rvbit_field;
				w_rv_.do_member_decl((*w_rr).type);
			
#line 5508 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fenumerator_list(struct w_resynclink *w_lnk, int *w_resync, Enum_type* *w_rr)
#else
w_fenumerator_list(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Enum_type* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Enum_type* w_rvenumerator;
#line 1167 "ccgram.w"
		 w_rvenumerator = (*w_rr); 
#line 5533 "parser.cc"
		w_fenumerator(&w_link, w_resync243, &w_rvenumerator);

		{/*__P46*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync59;

			while (w_nexttoken() == ',')
			{
				{
					Enum_type* w_rvenumerator;
					w_scantoken(',', &w_link, w_resync243);
#line 1170 "ccgram.w"
					
				if (w_nexttoken() == /*{*/'}'
						|| w_currtoken == W_EOI || w_currtoken == ';')
					goto done_enumerator_list;

				w_rvenumerator = (*w_rr);
			
#line 5556 "parser.cc"
					w_fenumerator(&w_link, w_resync248, &w_rvenumerator);
				}

			}
		}/*__P46*/

#line 1178 "ccgram.w"
		 done_enumerator_list: ; 
#line 5565 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fenumerator(struct w_resynclink *w_lnk, int *w_resync, Enum_type* *w_rr)
#else
w_fenumerator(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Enum_type* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		const char* w_rvidentifier;
		w_fidentifier(&w_link, w_resync22, &w_rvidentifier);

		{/*__P47*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync110;

			switch (w_nexttoken())
			{
				case '=':
					{
						Expr_node* w_rvconstant_expr;
						w_scantoken('=', &w_link, w_resync123);
						w_fconstant_expr(&w_link, w_resync145, &w_rvconstant_expr);
#line 1184 "ccgram.w"
						
				Const_expr val;
				val.new_ival(g_int_type->ic());

				if (w_rvconstant_expr)
					w_rvconstant_expr->eval_const_expr(val);

				if ((*w_rr))
					(*w_rr)->add(w_rvidentifier, g_cpp.filename(),
							g_cpp.linenum(), *val.ival);

				delete w_rvconstant_expr;
			
#line 5618 "parser.cc"
					}

					break;

				default:
					{
#line 1198 "ccgram.w"
						
				if ((*w_rr))
					(*w_rr)->add(w_rvidentifier, g_cpp.filename(),
							g_cpp.linenum());
			
#line 5631 "parser.cc"
					}

					break;
			}
		}/*__P47*/

	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_ftype_qualifier(struct w_resynclink *w_lnk, int *w_resync, int *w_rr)
#else
w_ftype_qualifier(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
int *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case CONST:
		{
			w_scantoken(CONST, &w_link, w_resync258);
#line 1207 "ccgram.w"
			 (*w_rr) = S_CONST; 
#line 5665 "parser.cc"
		}

		break;

	case VOLATILE:
		{
			w_scantoken(VOLATILE, &w_link, w_resync174);
#line 1208 "ccgram.w"
			 (*w_rr) = S_VOLATILE; 
#line 5675 "parser.cc"
		}

		break;

	default:
		w_err_illegal("type_qualifier");
		return W_RETERR;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fdeclarator(struct w_resynclink *w_lnk, int *w_resync, Storage_thing* *w_rr)
#else
w_fdeclarator(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Storage_thing* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Storage_thing* w_rvpointer;
		Storage_thing* w_rvdirect_declarator;
#line 1212 "ccgram.w"
		 w_rvpointer = w_rvdirect_declarator = (*w_rr); 
#line 5708 "parser.cc"
		w_fpointer(&w_link, w_resync35, &w_rvpointer);

		{/*direct_declarator*/
			Storage_thing* *w_rr = &w_rvdirect_declarator;
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync160;

			{
				Storage_thing w_rvname;
#line 1217 "ccgram.w"
				
			Symbol *classname = NULL;
			Type *type = (*w_rr)->type;
		
#line 5726 "parser.cc"

				{/*__P48*/
					struct w_resynclink *w_lnk = &w_link;
					struct w_resynclink w_link;

					w_link.prev = w_lnk;
					w_link.resync = w_resync62;

					switch (w_nexttoken())
					{
						case CLASSNAME:
							{
								w_scantoken(CLASSNAME, &w_link, w_resync204);
#line 1226 "ccgram.w"
								
				classname = g_sym;
				w_rvname.scope = ((Struct_type*)classname->type())->scope();
			
#line 5745 "parser.cc"
								w_scantoken(COLONCOLON, &w_link, w_resync238);
							}

							break;

						default:
							{
							}

							break;
					}
				}/*__P48*/


				{/*__P49*/
					Storage_thing *w_rr = &w_rvname;
					struct w_resynclink *w_lnk = &w_link;
					struct w_resynclink w_link;

					w_link.prev = w_lnk;
					w_link.resync = w_resync62;

					switch (w_nexttoken())
					{
						case IDENTIFIER:
							{
								w_scantoken(IDENTIFIER, &w_link, w_resync209);
#line 1235 "ccgram.w"
								 (*w_rr).sym = g_tok->text; 
#line 5775 "parser.cc"
							}

							break;

						case TYPENAME:
							{
								w_scantoken(TYPENAME, &w_link, w_resync99);
#line 1236 "ccgram.w"
								 (*w_rr).sym = g_tok->text; 
#line 5785 "parser.cc"
							}

							break;

						case '(':
							{
								Storage_thing* w_rvdeclarator;
								w_scantoken('(', &w_link, w_resync264);
#line 1237 "ccgram.w"
								 w_rvdeclarator = &(*w_rr); 
#line 5796 "parser.cc"
								w_fdeclarator(&w_link, w_resync264, &w_rvdeclarator);
								w_scantoken(')', &w_link, w_resync100);
							}

							break;

						case X_CTOR:
							{
								w_scantoken(X_CTOR, &w_link, w_resync188);
#line 1241 "ccgram.w"
								
					classname = g_sym;
					type = g_void_type;
					(*w_rr).scope = ((Struct_type*)classname->type())->scope();
					(*w_rr).sym = make_constructor_name(classname->name());
				
#line 5813 "parser.cc"
							}

							break;

						case X_MEMFUNC:
							{
								const char * w_rv_;
								w_scantoken(X_MEMFUNC, &w_link, w_resync12);
#line 1248 "ccgram.w"
								
					classname = g_sym;
					(*w_rr).scope = ((Struct_type*)classname->type())->scope();
				
#line 5827 "parser.cc"
								w_scantoken(COLONCOLON, &w_link, w_resync107);
#line 1253 "ccgram.w"
								
					if (w_nexttoken() == CLASSNAME || w_currtoken == '~')
						type = g_void_type;
				
#line 5834 "parser.cc"

								{/*__P50*/
									const char * *w_rr = &w_rv_;
									struct w_resynclink *w_lnk = &w_link;
									struct w_resynclink w_link;

									w_link.prev = w_lnk;
									w_link.resync = w_resync132;

									switch (w_nexttoken())
									{
										case IDENTIFIER:
											{
												w_scantoken(IDENTIFIER, &w_link, w_resync209);
#line 1258 "ccgram.w"
												 (*w_rr) = g_tok->text; 
#line 5851 "parser.cc"
											}

											break;

										case TYPENAME:
											{
												w_scantoken(TYPENAME, &w_link, w_resync99);
#line 1259 "ccgram.w"
												 (*w_rr) = g_tok->text; 
#line 5861 "parser.cc"
											}

											break;

										case OPERATOR:
											{
												struct W_Toperator_func w_rvoperator_func;
												w_foperator_func(&w_link, w_resync270, &w_rvoperator_func);
#line 1261 "ccgram.w"
												
					(*w_rr) = w_rvoperator_func.name;

					if (w_rvoperator_func.type)
						type = w_rvoperator_func.type;
				
#line 5877 "parser.cc"
											}

											break;

										case CLASSNAME:
											{
												w_scantoken(CLASSNAME, &w_link, w_resync250);
#line 1267 "ccgram.w"
												 (*w_rr) = make_constructor_name(g_tok->text); 
#line 5887 "parser.cc"
											}

											break;

										case '~':
											{
												w_scantoken('~', &w_link, w_resync54);
												w_scantoken(CLASSNAME, &w_link, w_resync250);
#line 1269 "ccgram.w"
												
					(*w_rr) = make_destructor_name(g_tok->text);
					type = g_void_type;
				
#line 5901 "parser.cc"
											}

											break;

										default:
											w_err_illegal("direct_declarator");
											break;
									}
								}/*__P50*/

#line 1274 "ccgram.w"
								 (*w_rr).sym = w_rv_; 
#line 5914 "parser.cc"
							}

							break;

						case X_DTOR:
							{
								w_scantoken(X_DTOR, &w_link, w_resync115);
#line 1277 "ccgram.w"
								
				classname = g_sym;
				type = g_void_type;
				(*w_rr).scope = ((Struct_type*)classname->type())->scope();
				(*w_rr).sym = make_destructor_name(g_tok->text);
			
#line 5929 "parser.cc"
							}

							break;

						case CLASSNAME:
							{
								w_scantoken(CLASSNAME, &w_link, w_resync250);
#line 1285 "ccgram.w"
								
				(*w_rr).sym = make_constructor_name(g_tok->text);
				type = g_void_type;
			
#line 5942 "parser.cc"
							}

							break;

						case '~':
							{
								w_scantoken('~', &w_link, w_resync54);
								w_scantoken(CLASSNAME, &w_link, w_resync250);
#line 1290 "ccgram.w"
								
				(*w_rr).sym = make_destructor_name(g_tok->text);
				type = g_void_type;
			
#line 5956 "parser.cc"
							}

							break;

						case OPERATOR:
							{
								struct W_Toperator_func w_rvoperator_func;
								w_foperator_func(&w_link, w_resync270, &w_rvoperator_func);
#line 1295 "ccgram.w"
								
				(*w_rr).sym = w_rvoperator_func.name;

				if (w_rvoperator_func.type)
					type = w_rvoperator_func.type;
			
#line 5972 "parser.cc"
							}

							break;

						default:
							w_err_illegal("direct_declarator");
							break;
					}
				}/*__P49*/

#line 1303 "ccgram.w"
				
				// !ISO - if this is a declaration within a function, to
				// minimize ambiguities with global and class declarations
				if (g_cplusplus && g_func_scope && w_nexttoken() == '('/*)*/)
					switch (w_looknext())
					{
					case VOID: case CHAR: case SHORT: case INT: case LONG:
					case SIGNED: case UNSIGNED: case FLOAT: case DOUBLE:
					case INLINE: case FRIEND: case VIRTUAL: case TYPEDEF:
					case EXTERN: case STATIC: case AUTO: case REGISTER:
					case PASCAL: case FORTRAN: case CLASS: case STRUCT:
					case UNION: case ENUM: case CONST: case VOLATILE:
					case CLASSNAME: case TYPENAME:
					case /*(*/')':
					// IDENTIFIER could be an old-style decl, but these are
					// not allowed in C+- code, so assume it is a constructor
						break;

					default:
						// replace open-paren with this magic token to
						// remove ambiguity when parsing this declaration
						w_currtoken = X_CTORINIT;
						goto not_a_func;
					}
			
#line 6009 "parser.cc"

				{/*__P51*/
					struct w_resynclink *w_lnk = &w_link;
					struct w_resynclink w_link;

					w_link.prev = w_lnk;
					w_link.resync = w_resync122;

					switch (w_nexttoken())
					{
						case '[':
							{
								struct W_Tarray w_rvarray;
#line 1330 "ccgram.w"
								 w_rvarray.type = type; w_rvarray.param = g_old_function; 
#line 6025 "parser.cc"
								w_farray(&w_link, w_resync206, &w_rvarray);
#line 1332 "ccgram.w"
								 type = w_rvarray.type; 
#line 6029 "parser.cc"
							}

							break;

						case '(':
							{
								w_scantoken('(', &w_link, w_resync173);

								{/*__P52*/
									struct w_resynclink *w_lnk = &w_link;
									struct w_resynclink w_link;

									w_link.prev = w_lnk;
									w_link.resync = w_resync186;

									switch (w_nexttoken())
									{
										case CLASSNAME:
										case '~':
										case TYPENAME:
										case INLINE:
										case FRIEND:
										case VIRTUAL:
										case TYPEDEF:
										case EXTERN:
										case STATIC:
										case AUTO:
										case REGISTER:
										case PASCAL:
										case FORTRAN:
										case VOID:
										case CHAR:
										case SHORT:
										case INT:
										case LONG:
										case FLOAT:
										case DOUBLE:
										case SIGNED:
										case UNSIGNED:
										case STRUCT:
										case UNION:
										case CLASS:
										case ENUM:
										case CONST:
										case VOLATILE:
											{
												Type* w_rvparameter_type_list;
#line 1334 "ccgram.w"
												 w_rvparameter_type_list = type; 
#line 6079 "parser.cc"
												w_fparameter_type_list(&w_link, w_resync112, &w_rvparameter_type_list);
#line 1336 "ccgram.w"
												 type = w_rvparameter_type_list; 
#line 6083 "parser.cc"
											}

											break;

										case IDENTIFIER:
											{
												Param_arr* w_rvidentifier_list;

												{/*identifier_list*/
													Param_arr* *w_rr = &w_rvidentifier_list;
													struct w_resynclink *w_lnk = &w_link;
													struct w_resynclink w_link;

													w_link.prev = w_lnk;
													w_link.resync = w_resync209;

													{
														w_scantoken(IDENTIFIER, &w_link, w_resync192);
#line 1521 "ccgram.w"
														
				(*w_rr) = new Param_arr;
				Param &p = (*w_rr)->end();
				p.symbol(g_tok->text);
			
#line 6108 "parser.cc"

														{/*__P68*/
															struct w_resynclink *w_lnk = &w_link;
															struct w_resynclink w_link;

															w_link.prev = w_lnk;
															w_link.resync = w_resync59;

															while (w_nexttoken() == ',')
															{
																{
																	w_scantoken(',', &w_link, w_resync192);
																	w_scantoken(IDENTIFIER, &w_link, w_resync209);
#line 1527 "ccgram.w"
																	
				Param &p = (*w_rr)->end();
				p.symbol(g_tok->text);
			
#line 6127 "parser.cc"
																}

															}
														}/*__P68*/

													}

												}/*identifier_list*/

#line 1338 "ccgram.w"
												
						type = new Function_type(type, w_rvidentifier_list);
						g_old_function = TRUE;
					
#line 6142 "parser.cc"
											}

											break;

										default:
											{
#line 1343 "ccgram.w"
												 type = new Function_type(type, NULL); 
#line 6151 "parser.cc"
											}

											break;
									}
								}/*__P52*/

								w_scantoken(')', &w_link, w_resync40);
#line 1345 "ccgram.w"
								
					if (classname != NULL)
						// actually a C++ member-function
						((Function_type*)type)->classname(classname);
					else if (g_old_function && w_nexttoken() == CONST)
						// const is part of old-style parameter type
						goto skip_const;
				
#line 6168 "parser.cc"

								{/*__P53*/
									struct w_resynclink *w_lnk = &w_link;
									struct w_resynclink w_link;

									w_link.prev = w_lnk;
									w_link.resync = w_resync77;

									switch (w_nexttoken())
									{
										case CONST:
											{
												w_scantoken(CONST, &w_link, w_resync258);
											}

											break;

										default:
											{
											}

											break;
									}
								}/*__P53*/

#line 1354 "ccgram.w"
								 skip_const: ; 
#line 6196 "parser.cc"
							}

							break;

						default:
							{
							}

							break;
					}
				}/*__P51*/

#line 1357 "ccgram.w"
				
			not_a_func:
				(*w_rr)->type = type;
				(*w_rr)->type = point_to(&w_rvname, (*w_rr));
				(*w_rr)->sym = w_rvname.sym;
				(*w_rr)->scope = w_rvname.scope;
			
#line 6217 "parser.cc"
			}

		}/*direct_declarator*/

	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fpointer(struct w_resynclink *w_lnk, int *w_resync, Storage_thing* *w_rr)
#else
w_fpointer(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Storage_thing* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Type* w_rv_1;
#line 1367 "ccgram.w"
		 w_rv_1 = (*w_rr)->type; 
#line 6246 "parser.cc"

		{/*__P54*/
			Type* *w_rr = &w_rv_1;
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync215;

			while (w_nexttoken() == '*')
			{
				{
					unsigned short w_rvtype_qualifier_list;
					w_scantoken('*', &w_link, w_resync199);

					{/*type_qualifier_list*/
						unsigned short *w_rr = &w_rvtype_qualifier_list;
						struct w_resynclink *w_lnk = &w_link;
						struct w_resynclink w_link;

						w_link.prev = w_lnk;
						w_link.resync = w_resync198;

						{
#line 1391 "ccgram.w"
							 (*w_rr) = S_NONE; 
#line 6273 "parser.cc"

							{/*__P58*/
								struct w_resynclink *w_lnk = &w_link;
								struct w_resynclink w_link;

								w_link.prev = w_lnk;
								w_link.resync = w_resync68;

								while (w_nexttoken() == CONST ||
										w_currtoken == VOLATILE)
								{
									{
										int w_rvtype_qualifier;
										w_ftype_qualifier(&w_link, w_resync68, &w_rvtype_qualifier);
#line 1393 "ccgram.w"
										 merge_flags((*w_rr), w_rvtype_qualifier); 
#line 6290 "parser.cc"
									}

								}
							}/*__P58*/

						}

					}/*type_qualifier_list*/

#line 1370 "ccgram.w"
					
				(*w_rr) = new Pointer_type((*w_rr));
				(*w_rr)->isconst(w_rvtype_qualifier_list & S_CONST);
				(*w_rr)->isvolatile(w_rvtype_qualifier_list & S_VOLATILE);
			
#line 6306 "parser.cc"
				}

			}
		}/*__P54*/

#line 1376 "ccgram.w"
		 (*w_rr)->type = w_rv_1; 
#line 6314 "parser.cc"

		{/*__P55*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync118;

			switch (w_nexttoken())
			{
				case '&':
					{
						w_scantoken('&', &w_link, w_resync83);
#line 1377 "ccgram.w"
						 merge_flags((*w_rr)->flags, S_REFERENCE); 
#line 6330 "parser.cc"
					}

					break;

				default:
					{
					}

					break;
			}
		}/*__P55*/

	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_farray(struct w_resynclink *w_lnk, int *w_resync, struct W_Tarray *w_rr)
#else
w_farray(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
struct W_Tarray *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Expr_node* w_rv_1;
		w_scantoken('[', &w_link, w_resync247);

		{/*__P56*/
			Expr_node* *w_rr = &w_rv_1;
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync247;

			switch (w_nexttoken())
			{
				case '(':
				case INTVAL:
				case UINTVAL:
				case LONGVAL:
				case ULONGVAL:
				case CHARACTER:
				case WCHARACTER:
				case FLOATVAL:
				case DOUBLEVAL:
				case LONGDOUBLEVAL:
				case IDENTIFIER:
				case THIS:
				case COLONCOLON:
				case SHORTDOUBLEVAL:
				case LONGLONGVAL:
				case ULONGLONGVAL:
				case CLASSNAME:
				case '~':
				case STRING:
				case WSTRING:
				case INCR:
				case DECR:
				case '&':
				case '*':
				case '+':
				case '-':
				case '!':
				case SIZEOF:
				case X_TYPENAME:
				case NEW:
				case DELETE:
				case OPERATOR:
					{
						Expr_node* w_rvconstant_expr;
						w_fconstant_expr(&w_link, w_resync145, &w_rvconstant_expr);
#line 1383 "ccgram.w"
						 (*w_rr) = w_rvconstant_expr; 
#line 6414 "parser.cc"
					}

					break;

				default:
					{
#line 1384 "ccgram.w"
						 (*w_rr) = NULL; 
#line 6423 "parser.cc"
					}

					break;
			}
		}/*__P56*/

		w_scantoken(']', &w_link, w_resync242);

		{/*__P57*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync219;

			switch (w_nexttoken())
			{
				case '[':
					{
						struct W_Tarray w_rvarray;
#line 1386 "ccgram.w"
						 w_rvarray = (*w_rr); 
#line 6446 "parser.cc"
						w_farray(&w_link, w_resync206, &w_rvarray);
#line 1386 "ccgram.w"
						 (*w_rr).type = w_rvarray.type; 
#line 6450 "parser.cc"
					}

					break;

				default:
					{
					}

					break;
			}
		}/*__P57*/

#line 1387 "ccgram.w"
		 (*w_rr).type = do_array(w_rv_1, (*w_rr).type, (*w_rr).param); 
#line 6465 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fparameter_type_list(struct w_resynclink *w_lnk, int *w_resync, Type* *w_rr)
#else
w_fparameter_type_list(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Type* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Param_arr* w_rvparameter_declaration;
#line 1399 "ccgram.w"
		
				Param_arr *args = new Param_arr;
				boolean varargs = FALSE;
				w_rvparameter_declaration = args;
			
#line 6494 "parser.cc"
		w_fparameter_declaration(&w_link, w_resync24, &w_rvparameter_declaration);

		{/*__P59*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync228;

			while (w_nexttoken() == ',' ||
					w_currtoken == DOTDOTDOT)
			{
				switch (w_nexttoken())
				{
					case ',':
						{
							w_scantoken(',', &w_link, w_resync24);

							{/*__P60*/
								struct w_resynclink *w_lnk = &w_link;
								struct w_resynclink w_link;

								w_link.prev = w_lnk;
								w_link.resync = w_resync85;

								switch (w_nexttoken())
								{
									case CLASSNAME:
									case '~':
									case TYPENAME:
									case INLINE:
									case FRIEND:
									case VIRTUAL:
									case TYPEDEF:
									case EXTERN:
									case STATIC:
									case AUTO:
									case REGISTER:
									case PASCAL:
									case FORTRAN:
									case VOID:
									case CHAR:
									case SHORT:
									case INT:
									case LONG:
									case FLOAT:
									case DOUBLE:
									case SIGNED:
									case UNSIGNED:
									case STRUCT:
									case UNION:
									case CLASS:
									case ENUM:
									case CONST:
									case VOLATILE:
										{
											Param_arr* w_rvparameter_declaration;
#line 1407 "ccgram.w"
											 w_rvparameter_declaration = args; 
#line 6554 "parser.cc"
											w_fparameter_declaration(&w_link, w_resync112, &w_rvparameter_declaration);
										}

										break;

									case DOTDOTDOT:
										{
											w_scantoken(DOTDOTDOT, &w_link, w_resync152);
#line 1410 "ccgram.w"
											
					varargs = TRUE;
					goto got_params;
				
#line 6568 "parser.cc"
										}

										break;

									default:
										w_err_illegal("parameter_type_list");
										break;
								}
							}/*__P60*/

						}

						break;

					case DOTDOTDOT:
						{
							w_scantoken(DOTDOTDOT, &w_link, w_resync152);
#line 1416 "ccgram.w"
							
				if (g_strict_iso)
					warn(MISSING_COMMA_BEFORE_DOTDOTDOT);

				varargs = TRUE;
				goto got_params;
			
#line 6594 "parser.cc"
						}

						break;

					default:
						w_err_illegal("parameter_type_list");
						break;
				}
			}
		}/*__P59*/

#line 1424 "ccgram.w"
		
		got_params:
			Function_type *ft = new Function_type((*w_rr), args);
			ft->varargs(varargs);
			(*w_rr) = ft;
		
#line 6613 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fparameter_declaration(struct w_resynclink *w_lnk, int *w_resync, Param_arr* *w_rr)
#else
w_fparameter_declaration(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Param_arr* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Storage_thing w_rvds;
		w_fdeclaration_specifiers(&w_link, w_resync53, &w_rvds);
#line 1434 "ccgram.w"
		 w_rvds.make_type(); 
#line 6639 "parser.cc"

		{/*__P61*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync202;

			switch (w_nexttoken())
			{
				case '(':
				case IDENTIFIER:
				case CLASSNAME:
				case TYPENAME:
				case '[':
				case '&':
				case '*':
					{
						Storage_thing* w_rvparam_declarator;
#line 1436 "ccgram.w"
						 w_rvparam_declarator = &w_rvds; 
#line 6661 "parser.cc"
						w_fparam_declarator(&w_link, w_resync275, &w_rvparam_declarator);
#line 1438 "ccgram.w"
						 w_rvds.make_type(); 
#line 6665 "parser.cc"
					}

					break;

				default:
					{
					}

					break;
			}
		}/*__P61*/

#line 1441 "ccgram.w"
		
				Param &p = (*w_rr)->end();
				p.type(w_rvds.type);
				p.storage((w_rvds.flags & S_REFERENCE) ? '&' : w_rvds.storage);
				w_rvds.flags &= ~S_REFERENCE;
				p.symbol(w_rvds.sym);

				if (w_rvds.storage != NONE && w_rvds.storage != REGISTER)
					error(ONLY_REGISTER_ALLOWED_FOR_PARAMS);
				else if ((*w_rr)->size() == 1 && (*w_rr)->elt(0).type()->isvoid()
						&& (*w_rr)->elt(0).symbol() == NULL)
					(*w_rr)->reset(0);
			
#line 6692 "parser.cc"

		{/*__P62*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync110;

			switch (w_nexttoken())
			{
				case '=':
					{
						Expr_node* w_rvinitializer;
						w_scantoken('=', &w_link, w_resync180);
						w_finitializer(&w_link, w_resync252, &w_rvinitializer);
#line 1456 "ccgram.w"
						
					p.init(w_rvinitializer);
					int refcount = 0;

					if (!w_rvinitializer->is_static_init(&refcount)
							|| refcount > 1)
						error(EXPECTED_CONST_INITIALIZERS, p.symbol());

					if (!g_cplusplus && g_strict_iso)
						warn(DEFAULT_PARAM_IS_NOT_ISO);
				
#line 6720 "parser.cc"
					}

					break;

				default:
					{
#line 1467 "ccgram.w"
						 p.init(NULL); 
#line 6729 "parser.cc"
					}

					break;
			}
		}/*__P62*/

	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fparam_declarator(struct w_resynclink *w_lnk, int *w_resync, Storage_thing* *w_rr)
#else
w_fparam_declarator(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Storage_thing* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Storage_thing* w_rvpointer;
		Storage_thing* w_rvdirect_param_declarator;
#line 1472 "ccgram.w"
		 w_rvpointer = w_rvdirect_param_declarator = (*w_rr); 
#line 6761 "parser.cc"
		w_fpointer(&w_link, w_resync275, &w_rvpointer);

		{/*direct_param_declarator*/
			Storage_thing* *w_rr = &w_rvdirect_param_declarator;
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync269;

			{
				Storage_thing w_rv_1;

				{/*__P63*/
					Storage_thing *w_rr = &w_rv_1;
					struct w_resynclink *w_lnk = &w_link;
					struct w_resynclink w_link;

					w_link.prev = w_lnk;
					w_link.resync = w_resync269;

					switch (w_nexttoken())
					{
						case IDENTIFIER:
						case CLASSNAME:
						case TYPENAME:
							{
								const char* w_rvidentifier;
								w_fidentifier(&w_link, w_resync248, &w_rvidentifier);
#line 1478 "ccgram.w"
								 (*w_rr).sym = w_rvidentifier; 
#line 6793 "parser.cc"
							}

							break;

						case '(':
							{
								Storage_thing* w_rvparam_declarator;
								w_scantoken('(', &w_link, w_resync275);
#line 1480 "ccgram.w"
								
				// simple typename in parens is function taking typename
				// as a single arg and not a declarator in parens
				if (w_nexttoken() == TYPENAME && w_looknext() == /*(*/')')
				{
					Param_arr *pa = new Param_arr;
					Param &p = pa->end();
					p.type(g_sym->type());
					(*w_rr).type = new Function_type((*w_rr).type, pa);
					w_skiptoken();		// TYPENAME
					goto get_paren;
				}

				w_rvparam_declarator = &(*w_rr);
			
#line 6818 "parser.cc"
								w_fparam_declarator(&w_link, w_resync67, &w_rvparam_declarator);
#line 1496 "ccgram.w"
								 get_paren: 
#line 6822 "parser.cc"
								w_scantoken(')', &w_link, w_resync40);

								{/*__P64*/
									struct w_resynclink *w_lnk = &w_link;
									struct w_resynclink w_link;

									w_link.prev = w_lnk;
									w_link.resync = w_resync77;

									switch (w_nexttoken())
									{
										case CONST:
											{
												w_scantoken(CONST, &w_link, w_resync258);
											}

											break;

										default:
											{
											}

											break;
									}
								}/*__P64*/

							}

							break;

						default:
							{
							}

							break;
					}
				}/*__P63*/


				{/*__P65*/
					struct w_resynclink *w_lnk = &w_link;
					struct w_resynclink w_link;

					w_link.prev = w_lnk;
					w_link.resync = w_resync122;

					switch (w_nexttoken())
					{
						case '[':
							{
								struct W_Tarray w_rvarray;
#line 1501 "ccgram.w"
								 w_rvarray.type = (*w_rr)->type; w_rvarray.param = TRUE; 
#line 6876 "parser.cc"
								w_farray(&w_link, w_resync206, &w_rvarray);
#line 1503 "ccgram.w"
								 (*w_rr)->type = w_rvarray.type; 
#line 6880 "parser.cc"
							}

							break;

						case '(':
							{
								w_scantoken('(', &w_link, w_resync172);

								{/*__P66*/
									struct w_resynclink *w_lnk = &w_link;
									struct w_resynclink w_link;

									w_link.prev = w_lnk;
									w_link.resync = w_resync185;

									switch (w_nexttoken())
									{
										case CLASSNAME:
										case '~':
										case TYPENAME:
										case INLINE:
										case FRIEND:
										case VIRTUAL:
										case TYPEDEF:
										case EXTERN:
										case STATIC:
										case AUTO:
										case REGISTER:
										case PASCAL:
										case FORTRAN:
										case VOID:
										case CHAR:
										case SHORT:
										case INT:
										case LONG:
										case FLOAT:
										case DOUBLE:
										case SIGNED:
										case UNSIGNED:
										case STRUCT:
										case UNION:
										case CLASS:
										case ENUM:
										case CONST:
										case VOLATILE:
											{
												Type* w_rvparameter_type_list;
#line 1505 "ccgram.w"
												 w_rvparameter_type_list = (*w_rr)->type; 
#line 6930 "parser.cc"
												w_fparameter_type_list(&w_link, w_resync112, &w_rvparameter_type_list);
#line 1507 "ccgram.w"
												 (*w_rr)->type = w_rvparameter_type_list; 
#line 6934 "parser.cc"
											}

											break;

										default:
											{
#line 1509 "ccgram.w"
												 (*w_rr)->type = new Function_type((*w_rr)->type, NULL); 
#line 6943 "parser.cc"
											}

											break;
									}
								}/*__P66*/

								w_scantoken(')', &w_link, w_resync40);

								{/*__P67*/
									struct w_resynclink *w_lnk = &w_link;
									struct w_resynclink w_link;

									w_link.prev = w_lnk;
									w_link.resync = w_resync77;

									switch (w_nexttoken())
									{
										case CONST:
											{
												w_scantoken(CONST, &w_link, w_resync258);
											}

											break;

										default:
											{
											}

											break;
									}
								}/*__P67*/

							}

							break;

						default:
							{
							}

							break;
					}
				}/*__P65*/

#line 1513 "ccgram.w"
				
				(*w_rr)->type = point_to(&w_rv_1, (*w_rr));
				(*w_rr)->sym = w_rv_1.sym;
		  
#line 6993 "parser.cc"
			}

		}/*direct_param_declarator*/

	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_ftype_name(struct w_resynclink *w_lnk, int *w_resync, Type* *w_rr)
#else
w_ftype_name(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Type* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Storage_thing w_rvsq;
		w_fspecifier_qualifier_list(&w_link, w_resync265, &w_rvsq);
#line 1537 "ccgram.w"
		 w_rvsq.make_type(); 
#line 7023 "parser.cc"

		{/*__P69*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync131;

			switch (w_nexttoken())
			{
				case '(':
				case '[':
				case '&':
				case '*':
					{
						Storage_thing* w_rvabstract_declarator;
#line 1539 "ccgram.w"
						 w_rvabstract_declarator = &w_rvsq; 
#line 7042 "parser.cc"
						w_fabstract_declarator(&w_link, w_resync131, &w_rvabstract_declarator);
					}

					break;

				default:
					{
					}

					break;
			}
		}/*__P69*/

#line 1543 "ccgram.w"
		
				w_rvsq.make_type();
				(*w_rr) = w_rvsq.type;
			
#line 7061 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fabstract_declarator(struct w_resynclink *w_lnk, int *w_resync, Storage_thing* *w_rr)
#else
w_fabstract_declarator(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Storage_thing* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Storage_thing* w_rvpointer;
		Storage_thing* w_rvdirect_abstract_declarator;
#line 1550 "ccgram.w"
		 w_rvpointer = w_rvdirect_abstract_declarator = (*w_rr); 
#line 7087 "parser.cc"
		w_fpointer(&w_link, w_resync131, &w_rvpointer);

		{/*direct_abstract_declarator*/
			Storage_thing* *w_rr = &w_rvdirect_abstract_declarator;
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync122;

			{
				Storage_thing w_rv_1;

				{/*__P70*/
					Storage_thing *w_rr = &w_rv_1;
					struct w_resynclink *w_lnk = &w_link;
					struct w_resynclink w_link;

					w_link.prev = w_lnk;
					w_link.resync = w_resync122;

					switch (w_nexttoken())
					{
						case '(':
							{
								Storage_thing* w_rvabstract_declarator;
								w_scantoken('(', &w_link, w_resync20);
#line 1556 "ccgram.w"
								 w_rvabstract_declarator = &(*w_rr); 
#line 7117 "parser.cc"
								w_fabstract_declarator(&w_link, w_resync20, &w_rvabstract_declarator);
								w_scantoken(')', &w_link, w_resync100);
							}

							break;

						default:
							{
							}

							break;
					}
				}/*__P70*/


				{/*__P71*/
					struct w_resynclink *w_lnk = &w_link;
					struct w_resynclink w_link;

					w_link.prev = w_lnk;
					w_link.resync = w_resync122;

					switch (w_nexttoken())
					{
						case '[':
							{
								struct W_Tarray w_rvarray;
#line 1560 "ccgram.w"
								 w_rvarray.type = (*w_rr)->type; w_rvarray.param = FALSE; 
#line 7147 "parser.cc"
								w_farray(&w_link, w_resync206, &w_rvarray);
#line 1562 "ccgram.w"
								 (*w_rr)->type = w_rvarray.type; 
#line 7151 "parser.cc"
							}

							break;

						case '(':
							{
								w_scantoken('(', &w_link, w_resync172);

								{/*__P72*/
									struct w_resynclink *w_lnk = &w_link;
									struct w_resynclink w_link;

									w_link.prev = w_lnk;
									w_link.resync = w_resync185;

									switch (w_nexttoken())
									{
										case CLASSNAME:
										case '~':
										case TYPENAME:
										case INLINE:
										case FRIEND:
										case VIRTUAL:
										case TYPEDEF:
										case EXTERN:
										case STATIC:
										case AUTO:
										case REGISTER:
										case PASCAL:
										case FORTRAN:
										case VOID:
										case CHAR:
										case SHORT:
										case INT:
										case LONG:
										case FLOAT:
										case DOUBLE:
										case SIGNED:
										case UNSIGNED:
										case STRUCT:
										case UNION:
										case CLASS:
										case ENUM:
										case CONST:
										case VOLATILE:
											{
												Type* w_rvparameter_type_list;
#line 1564 "ccgram.w"
												 w_rvparameter_type_list = (*w_rr)->type; 
#line 7201 "parser.cc"
												w_fparameter_type_list(&w_link, w_resync112, &w_rvparameter_type_list);
#line 1566 "ccgram.w"
												 (*w_rr)->type = w_rvparameter_type_list; 
#line 7205 "parser.cc"
											}

											break;

										default:
											{
#line 1568 "ccgram.w"
												 (*w_rr)->type = new Function_type((*w_rr)->type, NULL); 
#line 7214 "parser.cc"
											}

											break;
									}
								}/*__P72*/

								w_scantoken(')', &w_link, w_resync40);

								{/*__P73*/
									struct w_resynclink *w_lnk = &w_link;
									struct w_resynclink w_link;

									w_link.prev = w_lnk;
									w_link.resync = w_resync77;

									switch (w_nexttoken())
									{
										case CONST:
											{
												w_scantoken(CONST, &w_link, w_resync258);
											}

											break;

										default:
											{
											}

											break;
									}
								}/*__P73*/

							}

							break;

						default:
							{
							}

							break;
					}
				}/*__P71*/

#line 1572 "ccgram.w"
				 (*w_rr)->type = point_to(&w_rv_1, (*w_rr)); 
#line 7261 "parser.cc"
			}

		}/*direct_abstract_declarator*/

	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_finitializer(struct w_resynclink *w_lnk, int *w_resync, Expr_node* *w_rr)
#else
w_finitializer(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Expr_node* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case '(':
	case INTVAL:
	case UINTVAL:
	case LONGVAL:
	case ULONGVAL:
	case CHARACTER:
	case WCHARACTER:
	case FLOATVAL:
	case DOUBLEVAL:
	case LONGDOUBLEVAL:
	case IDENTIFIER:
	case THIS:
	case COLONCOLON:
	case SHORTDOUBLEVAL:
	case LONGLONGVAL:
	case ULONGLONGVAL:
	case CLASSNAME:
	case '~':
	case STRING:
	case WSTRING:
	case INCR:
	case DECR:
	case '&':
	case '*':
	case '+':
	case '-':
	case '!':
	case SIZEOF:
	case X_TYPENAME:
	case NEW:
	case DELETE:
	case OPERATOR:
		{
			Expr_node* w_rvassignment_expr;
			w_fassignment_expr(&w_link, w_resync145, &w_rvassignment_expr);
#line 1577 "ccgram.w"
			
			(*w_rr) = w_rvassignment_expr;

			if ((*w_rr))
				(*w_rr)->mark_symbol(TRUE, FALSE, FALSE);
		
#line 7330 "parser.cc"
		}

		break;

	case '{':
		{
			Initializer_node* w_rvinitializer_list;
			w_scantoken('{', &w_link, w_resync45);
			w_finitializer_list(&w_link, w_resync45, &w_rvinitializer_list);
			w_scantoken('}', &w_link, w_resync109);
#line 1583 "ccgram.w"
			 (*w_rr) = w_rvinitializer_list; 
#line 7343 "parser.cc"
		}

		break;

	default:
		w_err_illegal("initializer");
		return W_RETERR;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_finitializer_list(struct w_resynclink *w_lnk, int *w_resync, Initializer_node* *w_rr)
#else
w_finitializer_list(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Initializer_node* *w_rr;
#endif
{
	struct w_resynclink w_link;
#line 1587 "ccgram.w"
	 Expr_node *expr; 
#line 7369 "parser.cc"

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Expr_node* w_rvinitializer;

		{/*__P74*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync260;

			switch (w_nexttoken())
			{
				case '[':
					{
						Expr_node* w_rvconstant_expr;
						w_scantoken('[', &w_link, w_resync126);
						w_fconstant_expr(&w_link, w_resync15, &w_rvconstant_expr);
						w_scantoken(']', &w_link, w_resync175);

						{/*__P75*/
							struct w_resynclink *w_lnk = &w_link;
							struct w_resynclink w_link;

							w_link.prev = w_lnk;
							w_link.resync = w_resync110;

							switch (w_nexttoken())
							{
								case '=':
									{
										w_scantoken('=', &w_link, w_resync49);
									}

									break;

								default:
									{
									}

									break;
							}
						}/*__P75*/

#line 1589 "ccgram.w"
						 expr = w_rvconstant_expr; 
#line 7419 "parser.cc"
					}

					break;

				default:
					{
#line 1590 "ccgram.w"
						 expr = NULL; 
#line 7428 "parser.cc"
					}

					break;
			}
		}/*__P74*/

		w_finitializer(&w_link, w_resync162, &w_rvinitializer);
#line 1593 "ccgram.w"
		
				(*w_rr) = new Initializer_node(g_curr_scope);
				(*w_rr)->append(w_rvinitializer, expr);
			
#line 7441 "parser.cc"

		{/*__P76*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync59;

			while (w_nexttoken() == ',')
			{
				{
					Expr_node* w_rvinitializer;
					w_scantoken(',', &w_link, w_resync171);
#line 1598 "ccgram.w"
					
					if (w_nexttoken() == /*{*/'}' ||
							w_currtoken == W_EOI || w_currtoken == ';')
						goto done_initializer_list;
				
#line 7461 "parser.cc"

					{/*__P77*/
						struct w_resynclink *w_lnk = &w_link;
						struct w_resynclink w_link;

						w_link.prev = w_lnk;
						w_link.resync = w_resync260;

						switch (w_nexttoken())
						{
							case '[':
								{
									Expr_node* w_rvconstant_expr;
									w_scantoken('[', &w_link, w_resync126);
									w_fconstant_expr(&w_link, w_resync15, &w_rvconstant_expr);
									w_scantoken(']', &w_link, w_resync175);

									{/*__P78*/
										struct w_resynclink *w_lnk = &w_link;
										struct w_resynclink w_link;

										w_link.prev = w_lnk;
										w_link.resync = w_resync110;

										switch (w_nexttoken())
										{
											case '=':
												{
													w_scantoken('=', &w_link, w_resync49);
												}

												break;

											default:
												{
												}

												break;
										}
									}/*__P78*/

#line 1603 "ccgram.w"
									 expr = w_rvconstant_expr; 
#line 7505 "parser.cc"
								}

								break;

							default:
								{
#line 1604 "ccgram.w"
									 expr = NULL; 
#line 7514 "parser.cc"
								}

								break;
						}
					}/*__P77*/

					w_finitializer(&w_link, w_resync252, &w_rvinitializer);
#line 1607 "ccgram.w"
					 (*w_rr)->append(w_rvinitializer, expr); 
#line 7524 "parser.cc"
				}

			}
		}/*__P76*/

#line 1608 "ccgram.w"
		 done_initializer_list: ; 
#line 7532 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fstatement(struct w_resynclink *w_lnk, int *w_resync, Node* *w_rr)
#else
w_fstatement(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Node* *w_rr;
#endif
{
	struct w_resynclink w_link;
#line 1612 "ccgram.w"
	
		switch (w_nexttoken())
		{
		case IDENTIFIER:
			if (w_looknext() == ':')
			{
				w_pushback(w_currtoken);
				w_currtoken = X_LABEL;
			}

			break;

		case TYPENAME:
			if (w_looknext() == ':')
			{
				w_pushback(w_currtoken);
				w_currtoken = X_LABEL;
				break;
			}

			goto do_decl;
			//break;

		case CLASSNAME:
			if (w_looknext() == ':')
			{
				w_pushback(w_currtoken);
				w_currtoken = X_LABEL;
			}
			else if (w_looknext() != COLONCOLON)
				goto do_decl;

			break;

		case VOID: case CHAR: case SHORT: case INT: case LONG:
		case SIGNED: case UNSIGNED: case FLOAT: case DOUBLE:
		case INLINE: case FRIEND: case VIRTUAL: case TYPEDEF: case EXTERN:
		case STATIC: case AUTO: case REGISTER: case PASCAL: case FORTRAN:
		case CLASS: case STRUCT: case UNION: case ENUM:
		case CONST: case VOLATILE:
			// definitely a declaration
			goto do_decl;
			//break;

		default:
			break;
		}
	
#line 7598 "parser.cc"

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case X_LABEL:
	case CASE:
	case DEFAULT:
		{
			Node* w_rvlabeled_statement;

			{/*labeled_statement*/
				Node* *w_rr = &w_rvlabeled_statement;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;
#line 1689 "ccgram.w"
				 int line = g_cpp.linenum(); 
#line 7617 "parser.cc"

				w_link.prev = w_lnk;
				w_link.resync = w_resync266;

				switch (w_nexttoken())
				{
					case X_LABEL:
						{
							const char* w_rvidentifier;
							Node* w_rvstatement;
							w_scantoken(X_LABEL, &w_link, w_resync120);
							w_fidentifier(&w_link, w_resync120, &w_rvidentifier);
							w_scantoken(':', &w_link, w_resync120);
							w_fstatement(&w_link, w_resync121, &w_rvstatement);
#line 1691 "ccgram.w"
							
				if (g_func_scope == NULL)
					fatal(UNEXPECTED_LABEL);

				if (g_func_scope->symbol()->labels().find(w_rvidentifier))
				{
					if (g_func_scope->symbol()->labels()().scope() != NULL)
						error(LABEL_ALREADY_DEFINED, g_func_scope->file(),
								g_func_scope->symbol()->labels()().line());
				}

				Symbol &sym = g_func_scope->symbol()->labels()[w_rvidentifier];
				sym.scope(g_func_scope);
				sym.file(g_cpp.filename());
				sym.line(line);
				sym.node(w_rvstatement);
				(*w_rr) = new Label_node(g_curr_scope, &sym, w_rvstatement);
			
#line 7651 "parser.cc"
						}

						break;

					case CASE:
						{
							Expr_node* w_rvconstant_expr;
							Expr_node* w_rv_;
							Node* w_rvstatement;
							w_scantoken(CASE, &w_link, w_resync14);
							w_fconstant_expr(&w_link, w_resync119, &w_rvconstant_expr);

							{/*__P79*/
								Expr_node* *w_rr = &w_rv_;
								struct w_resynclink *w_lnk = &w_link;
								struct w_resynclink w_link;

								w_link.prev = w_lnk;
								w_link.resync = w_resync119;

								switch (w_nexttoken())
								{
									case DOTDOTDOT:
										{
											Expr_node* w_rvconstant_expr;
											w_scantoken(DOTDOTDOT, &w_link, w_resync261);
											w_fconstant_expr(&w_link, w_resync145, &w_rvconstant_expr);
#line 1711 "ccgram.w"
											
					if (g_strict_iso)
						warn(RANGED_CASE_IS_NOT_ISO);

					(*w_rr) = w_rvconstant_expr;
				
#line 7686 "parser.cc"
										}

										break;

									default:
										{
#line 1717 "ccgram.w"
											 (*w_rr) = NULL; 
#line 7695 "parser.cc"
										}

										break;
								}
							}/*__P79*/

							w_scantoken(':', &w_link, w_resync120);
							w_fstatement(&w_link, w_resync121, &w_rvstatement);
#line 1720 "ccgram.w"
							
			(*w_rr) = new Case_node(g_curr_scope, w_rvconstant_expr,
					w_rv_, w_rvstatement, g_curr_switch, line);
			g_curr_switch->cases().end() = (*w_rr);
		
#line 7710 "parser.cc"
						}

						break;

					case DEFAULT:
						{
							Node* w_rvstatement;
							w_scantoken(DEFAULT, &w_link, w_resync120);
							w_scantoken(':', &w_link, w_resync120);
							w_fstatement(&w_link, w_resync121, &w_rvstatement);
#line 1726 "ccgram.w"
							
			(*w_rr) = new Case_node(g_curr_scope, NULL, NULL, w_rvstatement,
					g_curr_switch, line);
			g_curr_switch->cases().end() = (*w_rr);
		
#line 7727 "parser.cc"
						}

						break;

					default:
						w_err_illegal("labeled_statement");
						break;
				}
			}/*labeled_statement*/

#line 1660 "ccgram.w"
			 (*w_rr) = w_rvlabeled_statement; 
#line 7740 "parser.cc"
		}

		break;

	case '{':
		{
			Node* w_rvcompound_statement;

			{/*compound_statement*/
				Node* *w_rr = &w_rvcompound_statement;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync51;

				{
					boolean w_rvdeclaration_list;
					Node_arr* w_rvstatement_list;
					w_scantoken('{', &w_link, w_resync133);
#line 1735 "ccgram.w"
					
				Scope *scope = g_curr_scope;
				Scope *newscope = new Scope(g_cpp.filename(),
						g_cpp.linenum(), scope);
				g_curr_scope = newscope;
			
#line 7768 "parser.cc"
					w_fdeclaration_list(&w_link, w_resync256, &w_rvdeclaration_list);
					w_fstatement_list(&w_link, w_resync245, &w_rvstatement_list);
					w_scantoken('}', &w_link, w_resync109);
#line 1742 "ccgram.w"
					
				g_curr_scope = scope;
				(*w_rr) = new Compound_node(newscope, w_rvstatement_list);
				newscope->node((*w_rr));
				check_unused_vars(newscope);
				newscope->cleanup();
			
#line 7780 "parser.cc"
				}

			}/*compound_statement*/

#line 1661 "ccgram.w"
			 (*w_rr) = w_rvcompound_statement; 
#line 7787 "parser.cc"
		}

		break;

	case '(':
	case INTVAL:
	case UINTVAL:
	case LONGVAL:
	case ULONGVAL:
	case CHARACTER:
	case WCHARACTER:
	case FLOATVAL:
	case DOUBLEVAL:
	case LONGDOUBLEVAL:
	case IDENTIFIER:
	case THIS:
	case COLONCOLON:
	case SHORTDOUBLEVAL:
	case LONGLONGVAL:
	case ULONGLONGVAL:
	case CLASSNAME:
	case '~':
	case STRING:
	case WSTRING:
	case INCR:
	case DECR:
	case '&':
	case '*':
	case '+':
	case '-':
	case '!':
	case SIZEOF:
	case X_TYPENAME:
	case NEW:
	case DELETE:
	case OPERATOR:
	case ';':
		{
			Node* w_rvexpression_statement;

			{/*expression_statement*/
				Node* *w_rr = &w_rvexpression_statement;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync226;

				{
					Expr_node* w_rvopt_expr;
					w_fopt_expr(&w_link, w_resync226, &w_rvopt_expr);
					w_scantoken(';', &w_link, w_resync222);
#line 1788 "ccgram.w"
					 (*w_rr) = implicit_cast(w_rvopt_expr, g_void_type); 
#line 7842 "parser.cc"
				}

			}/*expression_statement*/

#line 1662 "ccgram.w"
			 (*w_rr) = w_rvexpression_statement; 
#line 7849 "parser.cc"
		}

		break;

	case IF:
	case SWITCH:
		{
			Node* w_rvselection_statement;

			{/*selection_statement*/
				Node* *w_rr = &w_rvselection_statement;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync203;

				switch (w_nexttoken())
				{
					case IF:
						{
							Expr_node* w_rvmark_expr;
							Node* w_rvstatement;
							Node* w_rv_;
							w_scantoken(IF, &w_link, w_resync184);
							w_scantoken('(', &w_link, w_resync208);
							w_fmark_expr(&w_link, w_resync102, &w_rvmark_expr);
							w_scantoken(')', &w_link, w_resync102);
							w_fstatement(&w_link, w_resync30, &w_rvstatement);

							{/*__P82*/
								Node* *w_rr = &w_rv_;
								struct w_resynclink *w_lnk = &w_link;
								struct w_resynclink w_link;

								w_link.prev = w_lnk;
								w_link.resync = w_resync262;

								switch (w_nexttoken())
								{
									case ELSE:
										{
											Node* w_rvstatement;
											w_scantoken(ELSE, &w_link, w_resync30);
											w_fstatement(&w_link, w_resync121, &w_rvstatement);
#line 1793 "ccgram.w"
											 (*w_rr) = w_rvstatement; 
#line 7897 "parser.cc"
										}

										break;

									default:
										{
#line 1794 "ccgram.w"
											 (*w_rr) = NULL; 
#line 7906 "parser.cc"
										}

										break;
								}
							}/*__P82*/

#line 1796 "ccgram.w"
							 (*w_rr) = new If_node(g_curr_scope, w_rvmark_expr, w_rvstatement, w_rv_); 
#line 7915 "parser.cc"
						}

						break;

					case SWITCH:
						{
							Expr_node* w_rvmark_expr;
							Node* w_rvstatement;
							w_scantoken(SWITCH, &w_link, w_resync254);
							w_scantoken('(', &w_link, w_resync73);
							w_fmark_expr(&w_link, w_resync208, &w_rvmark_expr);
							w_scantoken(')', &w_link, w_resync208);
#line 1798 "ccgram.w"
							
				Switch_node *savsw = g_curr_switch;
				Node *savbr = g_curr_break;
				g_curr_switch = new Switch_node(g_curr_scope, w_rvmark_expr);
				g_curr_break = (*w_rr) = g_curr_switch;
			
#line 7935 "parser.cc"
							w_fstatement(&w_link, w_resync121, &w_rvstatement);
#line 1805 "ccgram.w"
							
				g_curr_switch->stmt(w_rvstatement);
				g_curr_switch->check_cases();
				g_curr_switch = savsw;
				g_curr_break = savbr;
			
#line 7944 "parser.cc"
						}

						break;

					default:
						w_err_illegal("selection_statement");
						break;
				}
			}/*selection_statement*/

#line 1663 "ccgram.w"
			 (*w_rr) = w_rvselection_statement; 
#line 7957 "parser.cc"
		}

		break;

	case WHILE:
	case DO:
	case FOR:
		{
			Node* w_rviteration_statement;

			{/*iteration_statement*/
				Node* *w_rr = &w_rviteration_statement;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync16;

				switch (w_nexttoken())
				{
					case WHILE:
						{
							Expr_node* w_rvmark_expr;
							Node* w_rvstatement;
							w_scantoken(WHILE, &w_link, w_resync166);
#line 1814 "ccgram.w"
							 g_loop_count++; 
#line 7985 "parser.cc"
							w_scantoken('(', &w_link, w_resync73);
							w_fmark_expr(&w_link, w_resync208, &w_rvmark_expr);
							w_scantoken(')', &w_link, w_resync208);
#line 1816 "ccgram.w"
							
				Node *savlp = g_curr_loop;
				Node *savbr = g_curr_break;
				While_node *w = new While_node(g_curr_scope, w_rvmark_expr);
				(*w_rr) = g_curr_loop = g_curr_break = w;
			
#line 7996 "parser.cc"
							w_fstatement(&w_link, w_resync121, &w_rvstatement);
#line 1823 "ccgram.w"
							
				w->expr(w_rvmark_expr);
				w->stmt(w_rvstatement);
				g_curr_loop = savlp;
				g_curr_break = savbr;
				g_loop_count--;
			
#line 8006 "parser.cc"
						}

						break;

					case DO:
						{
							Node* w_rvstatement;
							Expr_node* w_rvmark_expr;
							w_scantoken(DO, &w_link, w_resync121);
#line 1831 "ccgram.w"
							
				Node *savlp = g_curr_loop;
				Node *savbr = g_curr_break;
				Dowhile_node *d = new Dowhile_node(g_curr_scope);
				(*w_rr) = g_curr_loop = g_curr_break = d;
				g_loop_count++;
			
#line 8024 "parser.cc"
							w_fstatement(&w_link, w_resync121, &w_rvstatement);
							w_scantoken(WHILE, &w_link, w_resync232);
							w_scantoken('(', &w_link, w_resync93);
							w_fmark_expr(&w_link, w_resync93, &w_rvmark_expr);
							w_scantoken(')', &w_link, w_resync36);
							w_scantoken(';', &w_link, w_resync222);
#line 1839 "ccgram.w"
							
				d->stmt(w_rvstatement);
				d->expr(w_rvmark_expr);
				g_curr_loop = savlp;
				g_curr_break = savbr;
				g_loop_count--;
			
#line 8039 "parser.cc"
						}

						break;

					case FOR:
						{
							Expr_node* w_rv_;
							Expr_node* w_rvopt_expr;
							Expr_node* w_rvopt_expr_no_mark;
							Node* w_rvstatement;
							w_scantoken(FOR, &w_link, w_resync125);
							w_scantoken('(', &w_link, w_resync58);
#line 1847 "ccgram.w"
							
				if (g_cplusplus && w_nexttoken() == CLASSNAME)
					w_currtoken = X_CLASSNAME;
			
#line 8057 "parser.cc"

							{/*__P83*/
								Expr_node* *w_rr = &w_rv_;
								struct w_resynclink *w_lnk = &w_link;
								struct w_resynclink w_link;

								w_link.prev = w_lnk;
								w_link.resync = w_resync58;

								switch (w_nexttoken())
								{
									case '(':
									case INTVAL:
									case UINTVAL:
									case LONGVAL:
									case ULONGVAL:
									case CHARACTER:
									case WCHARACTER:
									case FLOATVAL:
									case DOUBLEVAL:
									case LONGDOUBLEVAL:
									case IDENTIFIER:
									case THIS:
									case COLONCOLON:
									case SHORTDOUBLEVAL:
									case LONGLONGVAL:
									case ULONGLONGVAL:
									case CLASSNAME:
									case '~':
									case STRING:
									case WSTRING:
									case INCR:
									case DECR:
									case '&':
									case '*':
									case '+':
									case '-':
									case '!':
									case SIZEOF:
									case X_TYPENAME:
									case NEW:
									case DELETE:
									case OPERATOR:
									case ';':
										{
											Expr_node* w_rvopt_expr;
											w_fopt_expr(&w_link, w_resync226, &w_rvopt_expr);
											w_scantoken(';', &w_link, w_resync222);
#line 1851 "ccgram.w"
											 (*w_rr) = w_rvopt_expr; 
#line 8108 "parser.cc"
										}

										break;

									case TYPENAME:
									case INLINE:
									case FRIEND:
									case VIRTUAL:
									case TYPEDEF:
									case EXTERN:
									case STATIC:
									case AUTO:
									case REGISTER:
									case PASCAL:
									case FORTRAN:
									case VOID:
									case CHAR:
									case SHORT:
									case INT:
									case LONG:
									case FLOAT:
									case DOUBLE:
									case SIGNED:
									case UNSIGNED:
									case STRUCT:
									case UNION:
									case CLASS:
									case ENUM:
									case CONST:
									case VOLATILE:
									case X_CLASSNAME:
										{
#line 1853 "ccgram.w"
											 int tok = w_nexttoken(); 
#line 8143 "parser.cc"

											{/*__P84*/
												struct w_resynclink *w_lnk = &w_link;
												struct w_resynclink w_link;

												w_link.prev = w_lnk;
												w_link.resync = w_resync231;

												switch (w_nexttoken())
												{
													case VOID:
														{
															w_scantoken(VOID, &w_link, w_resync255);
														}

														break;

													case CHAR:
														{
															w_scantoken(CHAR, &w_link, w_resync33);
														}

														break;

													case SHORT:
														{
															w_scantoken(SHORT, &w_link, w_resync155);
														}

														break;

													case INT:
														{
															w_scantoken(INT, &w_link, w_resync94);
														}

														break;

													case LONG:
														{
															w_scantoken(LONG, &w_link, w_resync11);
														}

														break;

													case SIGNED:
														{
															w_scantoken(SIGNED, &w_link, w_resync82);
														}

														break;

													case UNSIGNED:
														{
															w_scantoken(UNSIGNED, &w_link, w_resync272);
														}

														break;

													case FLOAT:
														{
															w_scantoken(FLOAT, &w_link, w_resync92);
														}

														break;

													case DOUBLE:
														{
															w_scantoken(DOUBLE, &w_link, w_resync7);
														}

														break;

													case INLINE:
														{
															w_scantoken(INLINE, &w_link, w_resync9);
														}

														break;

													case FRIEND:
														{
															w_scantoken(FRIEND, &w_link, w_resync88);
														}

														break;

													case VIRTUAL:
														{
															w_scantoken(VIRTUAL, &w_link, w_resync276);
														}

														break;

													case TYPEDEF:
														{
															w_scantoken(TYPEDEF, &w_link, w_resync13);
														}

														break;

													case EXTERN:
														{
															w_scantoken(EXTERN, &w_link, w_resync98);
														}

														break;

													case STATIC:
														{
															w_scantoken(STATIC, &w_link, w_resync17);
														}

														break;

													case AUTO:
														{
															w_scantoken(AUTO, &w_link, w_resync111);
														}

														break;

													case REGISTER:
														{
															w_scantoken(REGISTER, &w_link, w_resync28);
														}

														break;

													case PASCAL:
														{
															w_scantoken(PASCAL, &w_link, w_resync144);
														}

														break;

													case FORTRAN:
														{
															w_scantoken(FORTRAN, &w_link, w_resync75);
														}

														break;

													case CLASS:
														{
															w_scantoken(CLASS, &w_link, w_resync149);
														}

														break;

													case STRUCT:
														{
															w_scantoken(STRUCT, &w_link, w_resync158);
														}

														break;

													case UNION:
														{
															w_scantoken(UNION, &w_link, w_resync244);
														}

														break;

													case ENUM:
														{
															w_scantoken(ENUM, &w_link, w_resync216);
														}

														break;

													case CONST:
														{
															w_scantoken(CONST, &w_link, w_resync258);
														}

														break;

													case VOLATILE:
														{
															w_scantoken(VOLATILE, &w_link, w_resync174);
														}

														break;

													case TYPENAME:
														{
															w_scantoken(TYPENAME, &w_link, w_resync99);
														}

														break;

													case X_CLASSNAME:
														{
															w_scantoken(X_CLASSNAME, &w_link, w_resync157);
														}

														break;

													default:
														w_err_illegal("iteration_statement");
														break;
												}
											}/*__P84*/

#line 1862 "ccgram.w"
											 w_currtoken = (tok == X_CLASSNAME) ? CLASSNAME : tok; 
#line 8351 "parser.cc"
											w_fdeclaration(&w_link, w_resync142);
#line 1864 "ccgram.w"
											
					(*w_rr) = NULL;

					if (!g_cplusplus && g_strict_iso)
						warn(DECL_IN_FOR_IS_NOT_ISO);
				
#line 8360 "parser.cc"
										}

										break;
								}
							}/*__P83*/

#line 1871 "ccgram.w"
							 g_loop_count++; 
#line 8369 "parser.cc"
							w_fopt_expr(&w_link, w_resync41, &w_rvopt_expr);
							w_scantoken(';', &w_link, w_resync41);

							{/*opt_expr_no_mark*/
								Expr_node* *w_rr = &w_rvopt_expr_no_mark;
								struct w_resynclink *w_lnk = &w_link;
								struct w_resynclink w_link;

								w_link.prev = w_lnk;
								w_link.resync = w_resync208;

								switch (w_nexttoken())
								{
									case '(':
									case INTVAL:
									case UINTVAL:
									case LONGVAL:
									case ULONGVAL:
									case CHARACTER:
									case WCHARACTER:
									case FLOATVAL:
									case DOUBLEVAL:
									case LONGDOUBLEVAL:
									case IDENTIFIER:
									case THIS:
									case COLONCOLON:
									case SHORTDOUBLEVAL:
									case LONGLONGVAL:
									case ULONGLONGVAL:
									case CLASSNAME:
									case '~':
									case STRING:
									case WSTRING:
									case INCR:
									case DECR:
									case '&':
									case '*':
									case '+':
									case '-':
									case '!':
									case SIZEOF:
									case X_TYPENAME:
									case NEW:
									case DELETE:
									case OPERATOR:
										{
											Expr_node* w_rvexpression;
											w_fexpression(&w_link, w_resync145, &w_rvexpression);
#line 1918 "ccgram.w"
											 (*w_rr) = w_rvexpression; 
#line 8420 "parser.cc"
										}

										break;

									default:
										{
#line 1919 "ccgram.w"
											 (*w_rr) = NULL; 
#line 8429 "parser.cc"
										}

										break;
								}
							}/*opt_expr_no_mark*/

							w_scantoken(')', &w_link, w_resync208);
#line 1873 "ccgram.w"
							
				Node *savlp = g_curr_loop;
				Node *savbr = g_curr_break;
				For_node *f = new For_node(g_curr_scope,
						implicit_cast(w_rv_, g_void_type),
						w_rvopt_expr,
						implicit_cast(w_rvopt_expr_no_mark, g_void_type));
				(*w_rr) = g_curr_loop = g_curr_break = f;
			
#line 8447 "parser.cc"
							w_fstatement(&w_link, w_resync121, &w_rvstatement);
#line 1883 "ccgram.w"
							
				f->stmt(w_rvstatement);
				g_curr_loop = savlp;
				g_curr_break = savbr;
				g_loop_count--;

				// this is here as incr/decr vars may be initialized in
				// the body of the for
				if (w_rvopt_expr_no_mark)
					w_rvopt_expr_no_mark->mark_symbol(TRUE, FALSE, FALSE);
			
#line 8461 "parser.cc"
						}

						break;

					default:
						w_err_illegal("iteration_statement");
						break;
				}
			}/*iteration_statement*/

#line 1664 "ccgram.w"
			 (*w_rr) = w_rviteration_statement; 
#line 8474 "parser.cc"
		}

		break;

	case GOTO:
	case CONTINUE:
	case BREAK:
	case RETURN:
		{
			Node* w_rvjump_statement;

			{/*jump_statement*/
				Node* *w_rr = &w_rvjump_statement;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync1;

				switch (w_nexttoken())
				{
					case GOTO:
						{
							const char* w_rvidentifier;
							w_scantoken(GOTO, &w_link, w_resync72);
							w_fidentifier(&w_link, w_resync127, &w_rvidentifier);
#line 1924 "ccgram.w"
							
				Symbol &sym = g_func_scope->symbol()->labels()[w_rvidentifier];
				(*w_rr) = new Goto_node(g_curr_scope, &sym, g_cpp.linenum());
				sym.referenced(TRUE);

				// set the line-number to first used in case of error
				if (sym.scope() == NULL && sym.line() == 0)
					sym.line(g_cpp.linenum());
			
#line 8511 "parser.cc"
							w_scantoken(';', &w_link, w_resync222);
						}

						break;

					case CONTINUE:
						{
							w_scantoken(CONTINUE, &w_link, w_resync183);
							w_scantoken(';', &w_link, w_resync222);
#line 1935 "ccgram.w"
							
			if (g_curr_loop == NULL)
			{
				error(CONTINUE_OUTSIDE_LOOP);
				(*w_rr) = NULL;
			}
			else
				(*w_rr) = new Continue_node(g_curr_scope, g_curr_loop);
		
#line 8531 "parser.cc"
						}

						break;

					case BREAK:
						{
							w_scantoken(BREAK, &w_link, w_resync74);
							w_scantoken(';', &w_link, w_resync222);
#line 1945 "ccgram.w"
							
			if (g_curr_break == NULL)
			{
				error(BREAK_OUTSIDE_LOOP_OR_SWITCH);
				(*w_rr) = NULL;
			}
			else
				(*w_rr) = new Break_node(g_curr_scope, g_curr_break);
		
#line 8550 "parser.cc"
						}

						break;

					case RETURN:
						{
							Expr_node* w_rvopt_expr;
							w_scantoken(RETURN, &w_link, w_resync187);
							w_fopt_expr(&w_link, w_resync226, &w_rvopt_expr);
							w_scantoken(';', &w_link, w_resync222);
#line 1955 "ccgram.w"
							 (*w_rr) = new Return_node(g_curr_scope, w_rvopt_expr); 
#line 8563 "parser.cc"
						}

						break;

					default:
						w_err_illegal("jump_statement");
						break;
				}
			}/*jump_statement*/

#line 1665 "ccgram.w"
			 (*w_rr) = w_rvjump_statement; 
#line 8576 "parser.cc"
		}

		break;

	case ASM:
		{
			Node* w_rvasm_statement;
			w_fasm_statement(&w_link, w_resync90, &w_rvasm_statement);
#line 1667 "ccgram.w"
			 (*w_rr) = w_rvasm_statement; 
#line 8587 "parser.cc"
		}

		break;

	case X_DECLARATION:
		{
			w_scantoken(X_DECLARATION, &w_link, w_resync89);
#line 1669 "ccgram.w"
			
			do_decl:
				(*w_rr) = NULL;

				if (!g_cplusplus && g_strict_iso)
					warn(DECL_AS_STMT_IS_NOT_ISO);
			
#line 8603 "parser.cc"
			w_fdeclaration(&w_link, w_resync142);
		}

		break;

	case VOID:
		{
			w_scantoken(VOID, &w_link, w_resync255);
		}

		break;

	case CHAR:
		{
			w_scantoken(CHAR, &w_link, w_resync33);
		}

		break;

	case SHORT:
		{
			w_scantoken(SHORT, &w_link, w_resync155);
		}

		break;

	case INT:
		{
			w_scantoken(INT, &w_link, w_resync94);
		}

		break;

	case LONG:
		{
			w_scantoken(LONG, &w_link, w_resync11);
		}

		break;

	case SIGNED:
		{
			w_scantoken(SIGNED, &w_link, w_resync82);
		}

		break;

	case UNSIGNED:
		{
			w_scantoken(UNSIGNED, &w_link, w_resync272);
		}

		break;

	case FLOAT:
		{
			w_scantoken(FLOAT, &w_link, w_resync92);
		}

		break;

	case DOUBLE:
		{
			w_scantoken(DOUBLE, &w_link, w_resync7);
		}

		break;

	case INLINE:
		{
			w_scantoken(INLINE, &w_link, w_resync9);
		}

		break;

	case FRIEND:
		{
			w_scantoken(FRIEND, &w_link, w_resync88);
		}

		break;

	case VIRTUAL:
		{
			w_scantoken(VIRTUAL, &w_link, w_resync276);
		}

		break;

	case TYPEDEF:
		{
			w_scantoken(TYPEDEF, &w_link, w_resync13);
		}

		break;

	case EXTERN:
		{
			w_scantoken(EXTERN, &w_link, w_resync98);
		}

		break;

	case STATIC:
		{
			w_scantoken(STATIC, &w_link, w_resync17);
		}

		break;

	case AUTO:
		{
			w_scantoken(AUTO, &w_link, w_resync111);
		}

		break;

	case REGISTER:
		{
			w_scantoken(REGISTER, &w_link, w_resync28);
		}

		break;

	case PASCAL:
		{
			w_scantoken(PASCAL, &w_link, w_resync144);
		}

		break;

	case FORTRAN:
		{
			w_scantoken(FORTRAN, &w_link, w_resync75);
		}

		break;

	case CLASS:
		{
			w_scantoken(CLASS, &w_link, w_resync149);
		}

		break;

	case STRUCT:
		{
			w_scantoken(STRUCT, &w_link, w_resync158);
		}

		break;

	case UNION:
		{
			w_scantoken(UNION, &w_link, w_resync244);
		}

		break;

	case ENUM:
		{
			w_scantoken(ENUM, &w_link, w_resync216);
		}

		break;

	case CONST:
		{
			w_scantoken(CONST, &w_link, w_resync258);
		}

		break;

	case VOLATILE:
		{
			w_scantoken(VOLATILE, &w_link, w_resync174);
		}

		break;

	case TYPENAME:
		{
			w_scantoken(TYPENAME, &w_link, w_resync99);
		}

		break;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fdeclaration_list(struct w_resynclink *w_lnk, int *w_resync, boolean *w_rr)
#else
w_fdeclaration_list(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
boolean *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
#line 1752 "ccgram.w"
		 (*w_rr) = FALSE; 
#line 8813 "parser.cc"

		{/*__P80*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync142;

			for (;;)
			{
				switch (w_nexttoken())
				{
				case '(':
				case IDENTIFIER:
				case CLASSNAME:
				case '~':
				case TYPENAME:
				case '&':
				case '*':
				case OPERATOR:
				case EXTERN_C:
				case INLINE:
				case FRIEND:
				case VIRTUAL:
				case TYPEDEF:
				case EXTERN:
				case STATIC:
				case AUTO:
				case REGISTER:
				case PASCAL:
				case FORTRAN:
				case VOID:
				case CHAR:
				case SHORT:
				case INT:
				case LONG:
				case FLOAT:
				case DOUBLE:
				case SIGNED:
				case UNSIGNED:
				case STRUCT:
				case UNION:
				case CLASS:
				case ENUM:
				case CONST:
				case VOLATILE:
				case X_CTOR:
				case X_MEMFUNC:
				case X_DTOR:
					{
#line 1754 "ccgram.w"
						
				// global scope decls may begin with IDENTIFIER
				// - if we have a paren or a pointer ('*' or '&'),
				// then this must be an expression - also a '~' begins
				// an expr if this is C code, else assume a destructor
				if (w_nexttoken() == '('/*)*/ ||
						w_currtoken == '*' ||
						(!g_cplusplus && w_currtoken == '~') ||
						w_currtoken == '&' ||
						(g_curr_scope != g_file_scope &&
								w_currtoken == IDENTIFIER))
					goto done_declaration_list;
			
#line 8878 "parser.cc"
						w_fdeclaration(&w_link, w_resync142);
#line 1767 "ccgram.w"
						 (*w_rr) = TRUE; 
#line 8882 "parser.cc"
					}

					continue;

				default:
					break;
				}

				break; /*for(;;)*/
			}

		}/*__P80*/

#line 1768 "ccgram.w"
		 done_declaration_list: ; 
#line 8898 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fstatement_list(struct w_resynclink *w_lnk, int *w_resync, Node_arr* *w_rr)
#else
w_fstatement_list(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Node_arr* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
#line 1773 "ccgram.w"
		
				Node_arr *sav = g_curr_statlist;
				g_curr_statlist = (*w_rr) = new Node_arr;
			
#line 8925 "parser.cc"

		{/*__P81*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync121;

			for (;;)
			{
				switch (w_nexttoken())
				{
				case '(':
				case INTVAL:
				case UINTVAL:
				case LONGVAL:
				case ULONGVAL:
				case CHARACTER:
				case WCHARACTER:
				case FLOATVAL:
				case DOUBLEVAL:
				case LONGDOUBLEVAL:
				case IDENTIFIER:
				case THIS:
				case COLONCOLON:
				case SHORTDOUBLEVAL:
				case LONGLONGVAL:
				case ULONGLONGVAL:
				case CLASSNAME:
				case '~':
				case TYPENAME:
				case STRING:
				case WSTRING:
				case INCR:
				case DECR:
				case '&':
				case '*':
				case '+':
				case '-':
				case '!':
				case SIZEOF:
				case X_TYPENAME:
				case NEW:
				case DELETE:
				case OPERATOR:
				case '{':
				case ';':
				case INLINE:
				case FRIEND:
				case VIRTUAL:
				case TYPEDEF:
				case EXTERN:
				case STATIC:
				case AUTO:
				case REGISTER:
				case PASCAL:
				case FORTRAN:
				case VOID:
				case CHAR:
				case SHORT:
				case INT:
				case LONG:
				case FLOAT:
				case DOUBLE:
				case SIGNED:
				case UNSIGNED:
				case STRUCT:
				case UNION:
				case CLASS:
				case ENUM:
				case CONST:
				case VOLATILE:
				case X_DECLARATION:
				case X_LABEL:
				case CASE:
				case DEFAULT:
				case IF:
				case SWITCH:
				case WHILE:
				case DO:
				case FOR:
				case GOTO:
				case CONTINUE:
				case BREAK:
				case RETURN:
				case ASM:
					{
						Node* w_rvstatement;
						w_fstatement(&w_link, w_resync121, &w_rvstatement);
#line 1778 "ccgram.w"
						
				if (w_rvstatement != NULL && w_rvstatement != g_null_node)
					(*w_rr)->end() = w_rvstatement;
			
#line 9020 "parser.cc"
					}

					continue;

				default:
					break;
				}

				break; /*for(;;)*/
			}

		}/*__P81*/

#line 1783 "ccgram.w"
		 g_curr_statlist = sav; 
#line 9036 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fmark_expr(struct w_resynclink *w_lnk, int *w_resync, Expr_node* *w_rr)
#else
w_fmark_expr(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Expr_node* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Expr_node* w_rvexpression;
		w_fexpression(&w_link, w_resync145, &w_rvexpression);
#line 1898 "ccgram.w"
		
			(*w_rr) = w_rvexpression;

			if ((*w_rr))
				(*w_rr)->mark_symbol(TRUE, FALSE, FALSE);
		
#line 9067 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fopt_expr(struct w_resynclink *w_lnk, int *w_resync, Expr_node* *w_rr)
#else
w_fopt_expr(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Expr_node* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case '(':
	case INTVAL:
	case UINTVAL:
	case LONGVAL:
	case ULONGVAL:
	case CHARACTER:
	case WCHARACTER:
	case FLOATVAL:
	case DOUBLEVAL:
	case LONGDOUBLEVAL:
	case IDENTIFIER:
	case THIS:
	case COLONCOLON:
	case SHORTDOUBLEVAL:
	case LONGLONGVAL:
	case ULONGLONGVAL:
	case CLASSNAME:
	case '~':
	case STRING:
	case WSTRING:
	case INCR:
	case DECR:
	case '&':
	case '*':
	case '+':
	case '-':
	case '!':
	case SIZEOF:
	case X_TYPENAME:
	case NEW:
	case DELETE:
	case OPERATOR:
		{
			Expr_node* w_rvexpression;
			w_fexpression(&w_link, w_resync145, &w_rvexpression);
#line 1908 "ccgram.w"
			
			(*w_rr) = w_rvexpression;

			if ((*w_rr))
				(*w_rr)->mark_symbol(TRUE, FALSE, FALSE);
		
#line 9132 "parser.cc"
		}

		break;

	default:
		{
#line 1914 "ccgram.w"
			 (*w_rr) = NULL; 
#line 9141 "parser.cc"
		}

		break;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fasm_statement(struct w_resynclink *w_lnk, int *w_resync, Node* *w_rr)
#else
w_fasm_statement(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Node* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		String_node* w_rvstring_literal;
		w_scantoken(ASM, &w_link, w_resync69);
#line 1959 "ccgram.w"
		 int line = g_cpp.linenum(); 
#line 9170 "parser.cc"
		w_scantoken('(', &w_link, w_resync10);
		w_fstring_literal(&w_link, w_resync124, &w_rvstring_literal);
		w_scantoken(')', &w_link, w_resync36);
		w_scantoken(';', &w_link, w_resync222);
#line 1961 "ccgram.w"
		 (*w_rr) = new Asm_node(g_curr_scope, w_rvstring_literal,
					g_cpp.filename(), line); 
#line 9178 "parser.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_ftranslation_unit(struct w_resynclink *w_lnk, int *w_resync)
#else
w_ftranslation_unit(w_lnk, w_resync)
struct w_resynclink *w_lnk;
int *w_resync;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{

		{/*__P85*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync277;

			goto w_oneplus_4;

			for (;;)
			{
				switch (w_nexttoken())
				{
				w_oneplus_4:
				case '(':
				case IDENTIFIER:
				case CLASSNAME:
				case '~':
				case TYPENAME:
				case '&':
				case '*':
				case OPERATOR:
				case EXTERN_C:
				case ';':
				case INLINE:
				case FRIEND:
				case VIRTUAL:
				case TYPEDEF:
				case EXTERN:
				case STATIC:
				case AUTO:
				case REGISTER:
				case PASCAL:
				case FORTRAN:
				case VOID:
				case CHAR:
				case SHORT:
				case INT:
				case LONG:
				case FLOAT:
				case DOUBLE:
				case SIGNED:
				case UNSIGNED:
				case STRUCT:
				case UNION:
				case CLASS:
				case ENUM:
				case CONST:
				case VOLATILE:
				case X_CTOR:
				case X_MEMFUNC:
				case X_DTOR:
				case ASM:
					switch (w_nexttoken())
					{
						case '(':
						case IDENTIFIER:
						case CLASSNAME:
						case '~':
						case TYPENAME:
						case '&':
						case '*':
						case OPERATOR:
						case EXTERN_C:
						case INLINE:
						case FRIEND:
						case VIRTUAL:
						case TYPEDEF:
						case EXTERN:
						case STATIC:
						case AUTO:
						case REGISTER:
						case PASCAL:
						case FORTRAN:
						case VOID:
						case CHAR:
						case SHORT:
						case INT:
						case LONG:
						case FLOAT:
						case DOUBLE:
						case SIGNED:
						case UNSIGNED:
						case STRUCT:
						case UNION:
						case CLASS:
						case ENUM:
						case CONST:
						case VOLATILE:
						case X_CTOR:
						case X_MEMFUNC:
						case X_DTOR:
							{
								w_fdeclaration(&w_link, w_resync142);
							}

							break;

						case ASM:
							{
								Node* w_rvasm_statement;
								w_fasm_statement(&w_link, w_resync90, &w_rvasm_statement);
#line 1969 "ccgram.w"
								 do_tree(w_rvasm_statement); 
#line 9304 "parser.cc"
							}

							break;

						case ';':
							{
								w_scantoken(';', &w_link, w_resync222);
							}

							break;
					}
					continue;

				default:
					break;
				}

				break; /*for(;;)*/
			}

		}/*__P85*/

	}

	return W_RETOK;
}

#line 1974 "ccgram.w"


static void
do_look_for_extern()
{
	int t = w_looknext();

	if (t == STRING || t == WSTRING)
		w_currtoken = EXTERN_C;
}

static void
do_look_for_typename()
{
	if (w_nexttoken() == '('/*)*/)
		switch (w_looknext())
		{
		case CLASSNAME:
		case TYPENAME:
		case VOID:
		case CHAR:
		case SHORT:
		case INT:
		case LONG:
		case FLOAT:
		case DOUBLE:
		case SIGNED:
		case UNSIGNED:
		case STRUCT:
		case UNION:
		case ENUM:
		case CONST:
		case VOLATILE:
		case PASCAL:
		case FORTRAN:
			w_pushback('('); //)
			w_currtoken = X_TYPENAME;
			break;
		
		default:
			break;
		}
}

static int
peek_op_prec()
{
	switch (w_nexttoken())
	{
	case '*': case '/': case '%':
		return 13;

	case '+': case '-':
		return 12;

	case LSHIFT: case RSHIFT:
		return 11;

	case '<': case '>': case LESSEQ: case GREATEQ:
		return 10;

	case EQUAL: case NOTEQ:
		return 9;

	case '&':
		return 8;

	case '^':
		return 7;

	case '|':
		return 6;

	case ANDAND:
		return 5;

	case OROR:
		return 4;

	case '?':
		return 3;

	case '=': case MULTEQ: case DIVEQ: case MODEQ: case PLUSEQ: case MINUSEQ:
	case LSHIFTEQ: case RSHIFTEQ: case ANDEQ: case XOREQ: case OREQ:
		return 2;

	case ',':
		return 1;

	default:
		return 0;
	}
}

static Type*
do_point_to(Storage_thing *spoint, Storage_thing *sto)
{
	Type *point = spoint->type;
	Type *to = sto->type;

	if (point == NULL)
		return to;

	if (to == NULL)
		return point;

	Type *tp = point;
	Type *t = NULL;

	do
	{
		switch (tp->typeval())
		{
		case T_POINTER:
			t = ((Pointer_type*)tp)->type();

			if (t == NULL)
				((Pointer_type*)tp)->type(to);

			break;

		case T_ARRAY:
			t = ((Array_type*)tp)->type();

			if (t == NULL)
			{
				if (to->isvoid())
					error(CANNOT_DECLARE_ARRAY_OF_VOIDS);
				else
					((Array_type*)tp)->type(to);
			}

			break;

		case T_FUNCTION:
			t = ((Function_type*)tp)->retval();

			if (t == NULL)
			{
				((Function_type*)tp)->retval(to);
				((Function_type*)tp)->retref(sto->flags & S_REFERENCE);
				sto->flags &= ~S_REFERENCE;
			}

			break;

		default:
			bug("point_to(): illegal type %d", tp->typeval());
			break;
		}

		tp = t;
	} while (t != NULL);

	return point;
}


#line 9491 "parser.cc"
