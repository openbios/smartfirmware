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

{
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

}

primary_expr <Expr_node*>
	: string_literal { $$ = $string_literal; }
	| '(' expression ')' { $$ = $expression; }
	| INTVAL
		{ $$ = new Integer_node(g_int_type, g_curr_scope, g_tok->text); }
	| UINTVAL
		{ $$ = new Integer_node(g_uint_type, g_curr_scope, g_tok->text); }
	| LONGVAL
		{ $$ = new Integer_node(g_long_type, g_curr_scope, g_tok->text); }
	| ULONGVAL
		{ $$ = new Integer_node(g_ulong_type, g_curr_scope, g_tok->text); }
	| CHARACTER
		{ $$ = new Integer_node(g_char_type, g_curr_scope,
				strtochar(g_tok->text)); }
	| WCHARACTER
		{ $$ = new Integer_node(g_wchar_t_type, g_curr_scope,
				strtochar(g_tok->text, TRUE)); }
	| FLOATVAL
		{ $$ = new Float_node(g_float_type, g_curr_scope, g_tok->text); }
	| DOUBLEVAL
		{ $$ = new Float_node(g_double_type, g_curr_scope, g_tok->text); }
	| LONGDOUBLEVAL
		{ $$ = new Float_node(g_longdouble_type, g_curr_scope,
				g_tok->text); }
	| IDENTIFIER
		{ $$ = new Symbol_node(g_curr_scope, g_tok->text,
				g_cpp.filename(), g_cpp.linenum()); }
	// !ISO - C++
	| THIS
		{ $$ = new Symbol_node(g_curr_scope, g_tok->text,
				g_cpp.filename(), g_cpp.linenum()); }
	| operator_func
		{
			$$ = new Symbol_node(g_curr_scope, $operator_func.name,
					g_cpp.filename(), g_cpp.linenum());
		}
	| qualified_object_name { $$ = $qualified_object_name; }
	| COLONCOLON IDENTIFIER
		{ $$ = new Symbol_node(g_file_scope, g_tok->text,
				g_cpp.filename(), g_cpp.linenum()); }
	// !ISO - short doubles
	| SHORTDOUBLEVAL
		{ $$ = new Float_node(g_shortdouble_type, g_curr_scope,
				g_tok->text); }
	// !ISO - long long integers
	| LONGLONGVAL
		{ $$ = new Integer_node(g_longlong_type, g_curr_scope,
				g_tok->text); }
	| ULONGLONGVAL
		{ $$ = new Integer_node(g_ulonglong_type, g_curr_scope,
				g_tok->text); }
	;

qualified_object_name <Expr_node*>
	: CLASSNAME COLONCOLON
			{
				Scope *sav = g_curr_scope;

				if (g_cplusplus)
					g_curr_scope = ((Struct_type*)g_sym->type())->scope();
			}
		(<void>
		IDENTIFIER
			{
				$$ = new Symbol_node(g_curr_scope, g_tok->text,
						g_cpp.filename(), g_cpp.linenum());
			}
		| operator_func
			{
				$$ = new Symbol_node(g_curr_scope, $operator_func.name,
						g_cpp.filename(), g_cpp.linenum());
			}
		| '~' CLASSNAME
			{
				$$ = new Symbol_node(g_curr_scope,
						make_destructor_name(g_tok->text),
						g_cpp.filename(), g_cpp.linenum());
			}
		//| new_delete_exprs
		)
			{ g_curr_scope = sav; }
	;

identifier <const char*>
	: IDENTIFIER { $$ = g_tok->text; }
	| TYPENAME { $$ = g_tok->text; }
	| CLASSNAME { $$ = g_tok->text; }
	;

string_literal <String_node*>
	: { $$ = new String_node(g_curr_scope); }
		(<void> STRING { $$->append(g_tok->text); } )+
			{ $$->finish(); }
	| { $$ = new String_node(g_curr_scope, TRUE); }
		(<void> WSTRING { $$->append(g_tok->text); } )+
			{ $$->finish(); }
	;

postfix_expr <Expr_node*>
	: primary_expr { $$ = $primary_expr; }
			(<void>
			'[' expression ']'
				{ $$ = make_array_ref_node(g_curr_scope, $$, $expression); }
			| '(' argument_expr_list ')'
				{ $$ = make_func_call_node(g_curr_scope, $$,
						$argument_expr_list); }
			| '.' member_name
				{ $$ = make_struct_ref_node(g_curr_scope, $$, $member_name,
						g_cpp.filename(), g_cpp.linenum(), FALSE); }
			| ARROW member_name
				{ $$ = make_struct_ref_node(g_curr_scope, $$, $member_name,
						g_cpp.filename(), g_cpp.linenum(), TRUE); }
			| INCR
				{ $$ = make_operator_node(g_curr_scope, INCR, $$, FALSE); }
			| DECR
				{ $$ = make_operator_node(g_curr_scope, DECR, $$, FALSE); }
			)*
	;

member_name <const char*>
	: IDENTIFIER { $$ = g_tok->text; }
	| TYPENAME { $$ = g_tok->text; }
	| operator_func { $$ = $operator_func.name; }
	//| new_delete_exprs
	| CLASSNAME { $$ = make_constructor_name(g_tok->text); }
	| '~' CLASSNAME { $$ = make_destructor_name(g_tok->text); }
	;

argument_expr_list <Expr_arr*>
	: assignment_expr
			{ $$ = new Expr_arr; $$->end() = $assignment_expr; }
		(<void> ',' assignment_expr { $$->end() = $assignment_expr; } )*
	| [] { $$ = NULL; }
	;

{
	// this is here so macro w_nexttoken() is used instead of function
	static void do_look_for_typename();

	inline void
	look_for_typename()
	{
		if (w_nexttoken() == '('/*)*/)
			do_look_for_typename();
	}
}

unary_expr <Expr_node*>
	: postfix_expr { $$ = $postfix_expr; }
	| INCR unary_expr
			{ $$ = make_operator_node(g_curr_scope, INCR, $unary_expr); }
	| DECR unary_expr
			{ $$ = make_operator_node(g_curr_scope, DECR, $unary_expr); }
	| { int tok = w_nexttoken(); }
		(<void> '&' | '*' | '+' | '-' | '!'
		| '~'
			{
				if (w_nexttoken() == CLASSNAME)
				{
					$$ = new Symbol_node(g_file_scope,
							make_destructor_name(g_tok->text),
							g_cpp.filename(), g_cpp.linenum());
					w_skiptoken();		// CLASSNAME
					goto done_unary_expr;
				}
			}
		)
		cast_expr
			{
				$$ = make_operator_node(g_curr_scope, tok, $cast_expr);
			done_unary_expr:
				;
			}
	| SIZEOF { look_for_typename(); }
		(<void>
		X_TYPENAME '(' type_name ')'
				{ $$ = new Sizeof_node(g_curr_scope, $type_name); }
		| unary_expr
				{ $$ = new Sizeof_node(g_curr_scope, $unary_expr); }
		)
	| new_delete_exprs { $$ = $new_delete_exprs; }
	;

new_delete_exprs <Expr_node*>
	: NEW { look_for_typename(); }
		(<Type*>
		X_TYPENAME '(' new_type_name ')'  { $$ = $new_type_name; }
		| new_type_name  { $$ = $new_type_name; }
		)=type

		(<void>
		'[' mark_expr ']'
			{ $$ = new New_node(g_curr_scope, $type, NULL, $mark_expr); }
		| '(' argument_expr_list ')'
			{ $$ = new New_node(g_curr_scope, $type,
					$argument_expr_list, NULL); }
		| []
			{ $$ = new New_node(g_curr_scope, $type, NULL, NULL); }
		)
	| DELETE
		(<void> '[' opt_expr ']' cast_expr
			{ $$ = new Delete_node(g_curr_scope, $cast_expr, $opt_expr, TRUE); }
		| cast_expr
			{ $$ = new Delete_node(g_curr_scope, $cast_expr, NULL, FALSE); }
		)
	;

new_type_name <Type*>
	: specifier_qualifier_list=sq
			{
				$sq.make_type();
				$pointer = &$sq;
			}
		pointer
			{
				$sq.make_type();
				$$ = $sq.type;
			}
	;

operator_func <const char *name; Type *type>
	{ $$.type = NULL; }
	: OPERATOR
			{
				$$.name = make_operator_name(w_nexttoken());

				if (w_nexttoken() == '~')
					w_currtoken = X_TILDE;
			}
		(<void>
		operator
		| '!'
		//| '~'
		| X_TILDE
		| INCR
		| DECR
		//| '.'
		| ARROW
		| '(' ')'
		| '[' ']'
		| NEW
		| DELETE
		| new_type_name
			{
				$$.name = make_opcast_name($new_type_name);
				$$.type = $new_type_name;		// should be in the pool
			}
		)
	;

cast_expr <Expr_node*>
	{
		Type_arr *casts = NULL;
		Expr_node *ret = NULL;
	}
	: { look_for_typename(); }
		(<void>
			X_TYPENAME '(' type_name ')'
			(<void>
				'{' initializer_list '}'
				{
					ret = do_struct_display($type_name, $initializer_list);
					goto done;
				}
			| []
			)
			{
				if (casts == NULL)
					casts = new Type_arr;

				casts->end() = $type_name;
				look_for_typename();
			}
		)*
		unary_expr
			{
				ret = $unary_expr;

			done:
				if (casts != NULL)
				{
					for (int i = casts->size() - 1; i >= 0; i--)
						ret = new Cast_node(g_curr_scope,
								casts->elt(i), ret);

					delete casts;
				}

				$$ = ret;
			}
	;

expr <Expr_node *e; int stop>
	: (<void>
				{
					if (peek_op_prec() <= $$.stop)
						goto done_expr;
				}
			operator
				{
					if ($operator.op == '?')
						goto do_conditional;
				}
			cast_expr
				{
					if (peek_op_prec() <= $operator.prec && $operator.left)
					{
						$$.e = make_operator2_node(g_curr_scope,
								$operator.op, $$.e, $cast_expr);
						continue;
					}

					$expr.e = $cast_expr;
					$expr.stop = $operator.prec;

					if (!$operator.left)
						$expr.stop--;
				}
			expr
				{
					$$.e = make_operator2_node(g_curr_scope,
							$operator.op, $$.e, $expr.e);
					continue;

				do_conditional:
				}
			expression ':' conditional_expr
				{ $$.e = new Operator3_node(g_curr_scope, '?', $$.e,
						$expression, $conditional_expr); }
		)* { done_expr: ; }
	;

operator <int op; int prec; boolean left>
	{
		$$.prec = peek_op_prec();
		$$.op = w_currtoken;
		$$.left = TRUE;
	}
	: '*' | '/' | '%'
	| '+' | '-'
	| LSHIFT | RSHIFT
	| '<' | '>' | LESSEQ | GREATEQ
	| EQUAL | NOTEQ
	| '&'
	| '^'
	| '|'
	| ANDAND
	| OROR
	| '?' { $$.left = FALSE; }
	| (<void>
		'=' | MULTEQ | DIVEQ | MODEQ | PLUSEQ | MINUSEQ
			| LSHIFTEQ | RSHIFTEQ | ANDEQ | XOREQ | OREQ )
			{ $$.left = FALSE; }
	| ','
	;

expression <Expr_node*>
	: cast_expr
			{
				$expr.e = $cast_expr;
				$expr.stop = 0;
			}
		expr { $$ = $expr.e; }
	;

assignment_expr <Expr_node*>
	: cast_expr
			{
				$expr.e = $cast_expr;
				$expr.stop = 1;		// no commas
			}
		expr { $$ = $expr.e; }
	;

conditional_expr <Expr_node*>
	: cast_expr
			{
				$expr.e = $cast_expr;
				$expr.stop = 2;		// no commas, no assignment
			}
		expr { $$ = $expr.e; }
	;

constant_expr <Expr_node*>
	: cast_expr
			{
				$expr.e = $cast_expr;
				$expr.stop = 2;		// no commas, no assignment
			}
		expr
			{
				$$ = $expr.e;

				if (!$expr.e->is_int_expr())
					error(EXPECTED_CONST_EXPR);

				$expr.e->mark_symbol(TRUE, FALSE, FALSE);
			}
	;

{
	// this is here so macro w_nexttoken() is used instead of function
	static void do_look_for_extern();

	inline void
	look_for_extern()
	{
		if (g_cplusplus && w_nexttoken() == EXTERN)
			do_look_for_extern();
	}
}

declaration
	: { look_for_extern(); }
		( real_declaration
		// !ISO
		| EXTERN_C STRING
				{
					int save = g_default_linkage;

					if (streqi(g_tok->text, "C"))
						g_default_linkage = EXTERN_C;
					else if (streqi(g_tok->text, "C++"))
						g_default_linkage = NONE;
					else if (streqi(g_tok->text, "Pascal"))
						g_default_linkage = PASCAL;
					else if (streqi(g_tok->text, "FORTRAN"))
						g_default_linkage = FORTRAN;
				}
			( declaration | '{' declaration_list '}' (';' | []) )
				{ g_default_linkage = save; }
		)
	;

real_declaration
	:			{
				Storage_thing declspec, decl;
				boolean hadspec = FALSE;
				boolean save_old_func = g_old_function;

				if (w_nexttoken() == ';')
				{
					w_skiptoken();		// ';'
					goto done_declaration;
				}
			}
		( declaration_specifiers
			{
				declspec = $declaration_specifiers;
				declspec.make_type();
				hadspec = TRUE;
			}
			( ';'
				{
					if (declspec.type != NULL && !declspec.type->issue())
					{
						error(MUST_DECLARE_A_VARIABLE_NAME);
						declspec.type->free();
					}

					goto done_declaration;
				}
			| []
			)
		| []
		)
			{
				if (!hadspec)
					declspec.make_type();

				decl = declspec;
				$declarator = &decl;
			}
		declarator
		(
				{
					boolean old_func = g_old_function;
					Scope *sav = g_curr_scope;

					if (old_func)
						do_old_function_def(decl);
				}
			declaration_list
				{
					Symbol *func;

					if (!old_func && $declaration_list1)
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
				}
			ctor_initializer
			//compound_statement
			'{' declaration_list statement_list '}'
				{
					fini_function_def(func, $statement_list);
					g_curr_scope = sav;
				}
		|
				{
					if (!hadspec)
					{
						error(MISSING_TYPE_FOR_DECLARATION);
						goto done_declaration;
					}

					decl.symbol = decl.do_declaration();
				}
			( '=' initializer
				{ do_initializer(decl.symbol, $initializer); }
			// !ISO - C++ style constructor initialization
			// - the '(' has already been stripped when looking ahead
			| X_CTORINIT argument_expr_list ')'
				{ do_ctor_init(decl.symbol, $argument_expr_list); }
			| []
				{ do_initializer(decl.symbol, NULL, save_old_func); }
			)
				{ do_tree(decl.symbol); }
			(',' { $init_declarator_list = &declspec; }
				init_declarator_list
			| []
			) ';'
		)
			{
			done_declaration:
				g_old_function = save_old_func;
			}
	;

declaration_specifiers <Storage_thing>
		{ g_parse_type = $$.type ? FALSE : TRUE; }
	: (<void>
		{ $storage_class_specifier = &$$; }
		storage_class_specifier
	| { $type_specifier = &$$; } type_specifier
			{
				g_parse_type = FALSE;

				if ($? == W_RETERR)
					goto done_d_s;
			}
	| type_qualifier
			{ merge_flags($$.flags, $type_qualifier); }
	//| function_specifiers
	// !ISO
	| INLINE { merge_flags($$.flags, S_INLINE); }
	| FRIEND { merge_flags($$.flags, S_FRIEND); }
	| VIRTUAL { merge_flags($$.flags, S_VIRTUAL); }
	)+
		{ done_d_s: g_parse_type = TRUE; }
	;

ctor_initializer
	: ':' mem_initializer ( ',' mem_initializer )*
	| []
	;

mem_initializer
	: ( CLASSNAME | IDENTIFIER )
		{ const char *sym = g_tok->text; }
	'(' argument_expr_list ')'
		{ do_member_init(sym, $argument_expr_list); }
	; 

init_declarator_list <Storage_thing*>
	: { $init_declarator = *$$; }
		init_declarator
		(<void> ',' { $init_declarator = *$$; } init_declarator )*
	;

init_declarator <Storage_thing>
	: { $declarator = &$$; }
		declarator
			{ $$.symbol = $declarator->do_declaration(); }
		(<void>
		  '=' initializer 
			{ do_initializer($$.symbol, $initializer); }
		| []
			{ do_initializer($$.symbol, NULL); }
		)
			{ do_tree($$.symbol); }
	;

storage_class_specifier <Storage_thing*>
	: TYPEDEF
			{ $$->merge_storage(TYPEDEF); }
	| EXTERN
			{ $$->merge_storage(EXTERN); }
	| STATIC
			{ $$->merge_storage(STATIC); }
	| AUTO
			{ $$->merge_storage(AUTO); }
	| REGISTER
			{ $$->merge_storage(REGISTER); }
	// !ISO - for compatibility to MPW and Think C compiler
	| PASCAL
			{ merge_flags($$->flags, S_PASCAL); }
	// !ISO - for compatibility to common C compilers
	| FORTRAN
			{ merge_flags($$->flags, S_FORTRAN); }
	;

type_specifier <Storage_thing*>
	: VOID { $$->merge_basic(VOID); }
	| CHAR { $$->merge_basic(CHAR); }
	| SHORT { merge_flags($$->flags, S_SHORT); }
	| INT { $$->merge_basic(INT); }
	| LONG { merge_flags($$->flags, S_LONG); }
	| FLOAT { $$->merge_basic(FLOAT); }
	| DOUBLE { $$->merge_basic(DOUBLE); }
	| SIGNED { merge_flags($$->flags, S_SIGNED); }
	| UNSIGNED { merge_flags($$->flags, S_UNSIGNED); }
	| struct_union_specifier { $$->type = $struct_union_specifier; }
	| enum_specifier { $$->type = $enum_specifier; }
	| TYPENAME
		{
			if (g_sym == NULL)
				bug("wacco type_specifier: got %s - not TYPENAME",
						g_tok->text);

			$$->type = g_sym->type();
		}
	| CLASSNAME
		{
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

				$$->type = g_sym->type();
			}
		}
	| '~'
			{
				// this is to parse "operator~"
				if (w_nexttoken() != CLASSNAME)
				{
					w_pushback(w_currtoken);
					w_currtoken = X_TILDE;
					goto no_classname;
				}
			}
		CLASSNAME
			{
				w_currtoken = X_DTOR;
			no_classname:
				;
			}
	;

struct_union_specifier <Struct_type*>
	{ Scope *scope = g_curr_scope; }
	: struct_or_union=su
		(<void>
			identifier
			(
					{
						$$ = do_sue_definition($su, $identifier);
						$sdl.type = $base_spec = $$;
						$sdl.access = $su == CLASS ? PRIVATE : PUBLIC;

						if (g_cplusplus)
							g_curr_scope = $$->scope();
					}
				base_spec '{' struct_declaration_list=sdl '}'
			| []
					{ $$ = do_sue_declaration($su, $identifier); }
			)
		|
				{
					$$ = do_sue_definition($su, make_unnamed_name());
					$sdl.type = $$;
					$sdl.access = $su == CLASS ? PRIVATE : PUBLIC;
				}
			'{' struct_declaration_list=sdl '}'
		)
			{ g_curr_scope = scope; }
	;

struct_or_union <int>
	: STRUCT { $$ = STRUCT; }
	| UNION { $$ = UNION; }
	| CLASS { $$ = CLASS; }
	;

base_spec <Struct_type*>
	: ':'
			{
				if ($$->base() != NULL)
					error(STRUCT_ALREADY_DEFINED, $$->name(),
							$$->scope()->line());

				$base_specifier = $$;
			}
		base_specifier
		(<void> ',' // { $base_specifier = $$; }
			base_specifier
				{ error(MULTIPLE_INHERITANCE_NOT_SUPPORTED); }
		)*
	| []
	;

base_specifier <Struct_type*>
	{ boolean isclass = $$->is_class(); }
	: CLASSNAME
		{
			warn(MISSING_BASE_ACCESS_SPECIFIER);

			$$->add_base_class(g_sym->type(),
					isclass ? PRIVATE : PUBLIC, FALSE);
		}
	| VIRTUAL
		(<int> access_specifier { $$ = $access_specifier; }
		| [] { $$ = isclass ? PRIVATE : PUBLIC; }
		)
		CLASSNAME
			{ $$->add_base_class(g_sym->type(), $_, TRUE); }
	| access_specifier
		(<boolean> VIRTUAL { $$ = TRUE; }
		| [] { $$ = FALSE; }
		)
		CLASSNAME
			{ $$->add_base_class(g_sym->type(), $access_specifier, $_); }
	;

struct_declaration_list <struct Struct_decl>
	: (<void>
			{ $struct_declaration = &$$; }
		struct_declaration
			{
				if ($? != W_RETOK)
					goto done;
			}
		| access_specifier ':'
			{ $$.access = $access_specifier; }
		)+
			{ done:		; }
	;

access_specifier <int>
	: PRIVATE { $$ = PRIVATE; }
	| PUBLIC { $$ = PUBLIC; }
	| PROTECTED { $$ = PROTECTED; }
	;

// %noinline is needed to workaround cfront's yacc parser's internal limits
struct_declaration <struct Struct_decl*> %noinline
	{ Struct_type *stype = $$->type; }
	:
			{
				Storage_thing declspec;
				boolean hadspec = FALSE;

				if (w_nexttoken() == ';')
				{
					w_skiptoken();		// ';'
					return W_RETOK;
				}
			}
		(<void>
			declaration_specifiers=ds
				{
					declspec = $ds;
					declspec.make_type();
					hadspec = TRUE;
				}
			(<void>
				';'
					{
						if ($ds.type != NULL)
						{
							if ($ds.type->issu())
								do_unnamed_substruct($ds.type, stype);
							else if (!$ds.type->isenum())
							{
								error(MUST_DECLARE_A_MEMBER_NAME);
								$ds.type->free();
								return W_RETERR;
							}
						}

						return W_RETOK;		// continue;
					}
			| []
			)
		| []
		)
			{
				if (!hadspec)
					declspec.make_type();

				Storage_thing decl = declspec;
			}
		(<void>
				{ $declarator = &decl; }
			declarator
				{
					// this is since a following ':' could be a bitfield
					if (g_cplusplus && decl.type->isfunc() &&
							w_nexttoken() == ':')
					{
						w_pushback(w_currtoken);
						w_currtoken = X_CTORINIT;
					}
				}
		| []		// only for unnamed (padding) bitfields
		)
		(<void>
			bit_field
				{
					if (!hadspec)
					{
						error(MISSING_TYPE_FOR_DECLARATION);
						return W_RETERR;
					}

					//decl.symbol = decl.do_declaration();
					decl.bitfield = $bit_field;
					decl.do_member_decl(stype);
				}
			(<void>
				','
					{
						$struct_declarator_list.stor = &declspec;
						$struct_declarator_list.type = stype;
					}
				struct_declarator_list
			| []
			) ';'
		//| []		// is this necessary?
		// !ISO - inline member function
		|
				{
					if (decl.type->isfunc())
						((Function_type*)decl.type)->classname(
								stype->symbol());

					Scope *sav = g_curr_scope;
					Symbol *func = do_function_def(decl);

					if (!g_cplusplus)
						error(MEMFUNCS_ARE_NOT_ISO);
				}
			(<void>
			// inline constructor definition
			X_CTORINIT ctor_initializer
			| []
			)
			// inline member function
			'{' declaration_list statement_list '}'
				{
					fini_function_def(func, $statement_list);
					g_curr_scope = sav;
				}
		)
	;

bit_field <short>
	: ':' constant_expr
			{
				Const_expr width;
				$constant_expr->eval_const_expr(width);
				delete $constant_expr;

				if (width.ival != NULL)
					$$ = (short)width.ival->get();

				if ($$ < 0)
				{
					error(NEGATIVE_VALUE_FOR_BITFIELD);
					$$ = -1;
				}
			}
	| [] { $$ = -1; }
	;

specifier_qualifier_list <Storage_thing>
		{ g_parse_type = $$.type ? FALSE : TRUE; }
	: (<void>
			{ $type_specifier = &$$; }
		type_specifier
			{
				g_parse_type = FALSE;

				if ($? == W_RETERR)
					goto done_s_q_l;
			}
	  | type_qualifier
			{ merge_flags($$.flags, $type_qualifier); }
	  )+
		{ done_s_q_l: g_parse_type = TRUE; }
	;

struct_declarator_list <struct Struct_decl>
	: { $struct_declarator = $$; }
		struct_declarator
		(<void> ',' { $struct_declarator = $$; } struct_declarator)*
	;

struct_declarator <struct Struct_decl>
	: { $_ = *$$.stor; }
		(<Storage_thing> { $declarator = &$$; } declarator
		| []
		)
	bit_field
			{
				$_.bitfield = $bit_field;
				$_.do_member_decl($$.type);
			}
	;

enum_specifier <Enum_type*>
	{ Scope *scope = g_curr_scope; }
	: ENUM
		(<void>
			identifier
			( '{'
					{ $$ = $enumerator_list =
						(Enum_type*)do_sue_definition(ENUM, $identifier); }
			enumerator_list '}'
			| []
				{ $$ = (Enum_type*)do_sue_declaration(ENUM, $identifier); }
			)
		| { $$ = $enumerator_list = (Enum_type*)do_sue_definition(ENUM,
								make_unnamed_name()); }
		  '{' enumerator_list '}'
		)
			{ g_curr_scope = scope; }
	;

enumerator_list <Enum_type*>
	: { $enumerator = $$; }
		enumerator
		(<void> ','
			{
				if (w_nexttoken() == /*{*/'}'
						|| w_currtoken == W_EOI || w_currtoken == ';')
					goto done_enumerator_list;

				$enumerator = $$;
			}
			enumerator
		)* { done_enumerator_list: ; }
	;

enumerator <Enum_type*>
	: identifier
		(<void> '=' constant_expr
			{
				Const_expr val;
				val.new_ival(g_int_type->ic());

				if ($constant_expr)
					$constant_expr->eval_const_expr(val);

				if ($$)
					$$->add($identifier, g_cpp.filename(),
							g_cpp.linenum(), *val.ival);

				delete $constant_expr;
			}
		| []
			{
				if ($$)
					$$->add($identifier, g_cpp.filename(),
							g_cpp.linenum());
			}
		)
	;

type_qualifier <int>
	: CONST { $$ = S_CONST; }
	| VOLATILE { $$ = S_VOLATILE; }
	;

declarator <Storage_thing*>
	: { $pointer = $direct_declarator = $$; }
		pointer direct_declarator
	;

direct_declarator <Storage_thing*>
	:   {
			Symbol *classname = NULL;
			Type *type = $$->type;
		}

		// this is here as a type_specifier may pick up a genuine type
		// instead of just the first part of a C++ declarator name
		(<void>
		CLASSNAME
			{
				classname = g_sym;
				$name.scope = ((Struct_type*)classname->type())->scope();
			}
			COLONCOLON
		| []
		)

		(<Storage_thing>		// aliased to "name"
		IDENTIFIER { $$.sym = g_tok->text; }
		| TYPENAME { $$.sym = g_tok->text; }
		| '(' { $declarator = &$$; } declarator ')'
		// !ISO - constructor and class member (hopefully function)
		// the X_* ones are picked up by type_specifier
		| X_CTOR
				{
					classname = g_sym;
					type = g_void_type;
					$$.scope = ((Struct_type*)classname->type())->scope();
					$$.sym = make_constructor_name(classname->name());
				}
		| X_MEMFUNC
				{
					classname = g_sym;
					$$.scope = ((Struct_type*)classname->type())->scope();
				}
			COLONCOLON
				{
					if (w_nexttoken() == CLASSNAME || w_currtoken == '~')
						type = g_void_type;
				}
			(<const char *>
			IDENTIFIER { $$ = g_tok->text; }
			| TYPENAME { $$ = g_tok->text; }
			| operator_func
				{
					$$ = $operator_func.name;

					if ($operator_func.type)
						type = $operator_func.type;
				}
			| CLASSNAME { $$ = make_constructor_name(g_tok->text); }
			| '~' CLASSNAME
				{
					$$ = make_destructor_name(g_tok->text);
					type = g_void_type;
				}
			)
				{ $$.sym = $_; }
			//member_name
		| X_DTOR
			{
				classname = g_sym;
				type = g_void_type;
				$$.scope = ((Struct_type*)classname->type())->scope();
				$$.sym = make_destructor_name(g_tok->text);
			}
		// these are needed in case type_specifier picked up a real type
		| CLASSNAME
			{
				$$.sym = make_constructor_name(g_tok->text);
				type = g_void_type;
			}
		| '~' CLASSNAME
			{
				$$.sym = make_destructor_name(g_tok->text);
				type = g_void_type;
			}
		| operator_func
			{
				$$.sym = $operator_func.name;

				if ($operator_func.type)
					type = $operator_func.type;
			}
		)=name

			{
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
			}

		(<void>
				{ $array.type = type; $array.param = g_old_function; }
		array
				{ type = $array.type; }
		| '(' (
					{ $parameter_type_list = type; }
			parameter_type_list
					{ type = $parameter_type_list; }
			| identifier_list
					{
						type = new Function_type(type, $identifier_list);
						g_old_function = TRUE;
					}
			| []
					{ type = new Function_type(type, NULL); }
			) ')'
				{
					if (classname != NULL)
						// actually a C++ member-function
						((Function_type*)type)->classname(classname);
					else if (g_old_function && w_nexttoken() == CONST)
						// const is part of old-style parameter type
						goto skip_const;
				}
			(<void> CONST | [] )
				{ skip_const: ; }
		| []
		)
			{
			not_a_func:
				$$->type = type;
				$$->type = point_to(&$name, $$);
				$$->sym = $name.sym;
				$$->scope = $name.scope;
			}
	;

pointer <Storage_thing*>
	: { $_1 = $$->type; }
		(<Type*>
		'*' type_qualifier_list
			{
				$$ = new Pointer_type($$);
				$$->isconst($type_qualifier_list & S_CONST);
				$$->isvolatile($type_qualifier_list & S_VOLATILE);
			}
		)*
			{ $$->type = $_1; }
		(<void> '&' { merge_flags($$->flags, S_REFERENCE); }
		| []
		)
	;

array <Type *type; boolean param>
	: '[' (<Expr_node*> constant_expr { $$ = $constant_expr; }
				| [] { $$ = NULL; }
				) ']'
		(<void> { $array = $$; } array { $$.type = $array.type; } | [] )
			{ $$.type = do_array($_1, $$.type, $$.param); }
	;

type_qualifier_list <unsigned short>
	: { $$ = S_NONE; }
		(<void> type_qualifier
			{ merge_flags($$, $type_qualifier); }
		)*
	;

parameter_type_list <Type*>
	:
			{
				Param_arr *args = new Param_arr;
				boolean varargs = FALSE;
				$parameter_declaration = args;
			}
		parameter_declaration
		(<void> ','
			(
				{ $parameter_declaration = args; }
			parameter_declaration
			| DOTDOTDOT
				{
					varargs = TRUE;
					goto got_params;
				}
			)
		| DOTDOTDOT
			{
				if (g_strict_iso)
					warn(MISSING_COMMA_BEFORE_DOTDOTDOT);

				varargs = TRUE;
				goto got_params;
			}
		)*
		{
		got_params:
			Function_type *ft = new Function_type($$, args);
			ft->varargs(varargs);
			$$ = ft;
		}
	;

parameter_declaration <Param_arr*>
	: declaration_specifiers=ds
			{ $ds.make_type(); }
		(<void>
				{ $param_declarator = &$ds; }
			param_declarator
				{ $ds.make_type(); }
		| []
		)
			{
				Param &p = $$->end();
				p.type($ds.type);
				p.storage(($ds.flags & S_REFERENCE) ? '&' : $ds.storage);
				$ds.flags &= ~S_REFERENCE;
				p.symbol($ds.sym);

				if ($ds.storage != NONE && $ds.storage != REGISTER)
					error(ONLY_REGISTER_ALLOWED_FOR_PARAMS);
				else if ($$->size() == 1 && $$->elt(0).type()->isvoid()
						&& $$->elt(0).symbol() == NULL)
					$$->reset(0);
			}
		(<void>
		'=' initializer
				{
					p.init($initializer);
					int refcount = 0;

					if (!$initializer->is_static_init(&refcount)
							|| refcount > 1)
						error(EXPECTED_CONST_INITIALIZERS, p.symbol());

					if (!g_cplusplus && g_strict_iso)
						warn(DEFAULT_PARAM_IS_NOT_ISO);
				}
		| [] { p.init(NULL); }
		)
	;

param_declarator <Storage_thing*>
	: { $pointer = $direct_param_declarator = $$; }
		pointer direct_param_declarator
	;

direct_param_declarator <Storage_thing*>
	: (<Storage_thing>
		identifier { $$.sym = $identifier; }
	  | '('
			{
				// simple typename in parens is function taking typename
				// as a single arg and not a declarator in parens
				if (w_nexttoken() == TYPENAME && w_looknext() == /*(*/')')
				{
					Param_arr *pa = new Param_arr;
					Param &p = pa->end();
					p.type(g_sym->type());
					$$.type = new Function_type($$.type, pa);
					w_skiptoken();		// TYPENAME
					goto get_paren;
				}

				$param_declarator = &$$;
			}
		param_declarator
			{ get_paren: }
		')' (<void> CONST | [] )
	  | []		// abstract declarator
	  )
	  (<void>
			{ $array.type = $$->type; $array.param = TRUE; }
	  array
			{ $$->type = $array.type; }
	  | '(' (
				{ $parameter_type_list = $$->type; }
		  parameter_type_list
				{ $$->type = $parameter_type_list; }
		  | []
				{ $$->type = new Function_type($$->type, NULL); }
		  ) ')' (<void> CONST | [] )
	  | []
	  )
		  {
				$$->type = point_to(&$_1, $$);
				$$->sym = $_1.sym;
		  }
	;

identifier_list <Param_arr*>
	: IDENTIFIER
			{
				$$ = new Param_arr;
				Param &p = $$->end();
				p.symbol(g_tok->text);
			}
		(<void> ',' IDENTIFIER
			{
				Param &p = $$->end();
				p.symbol(g_tok->text);
			}
		)*
	; 

// 3.5.5
type_name <Type*>
	: specifier_qualifier_list=sq
			{ $sq.make_type(); }
		(<void>
				{ $abstract_declarator = &$sq; }
			abstract_declarator
		| []
		)
			{
				$sq.make_type();
				$$ = $sq.type;
			}
	;

abstract_declarator <Storage_thing*>
	: { $pointer = $direct_abstract_declarator = $$; }
		pointer direct_abstract_declarator
	;

direct_abstract_declarator <Storage_thing*>
	: (<Storage_thing>
		'(' { $abstract_declarator = &$$; } abstract_declarator ')'
		| []
		)
		(<void>
				{ $array.type = $$->type; $array.param = FALSE; }
		array
				{ $$->type = $array.type; }
		| '(' (
					{ $parameter_type_list = $$->type; }
			parameter_type_list
					{ $$->type = $parameter_type_list; }
			| [] 
					{ $$->type = new Function_type($$->type, NULL); }
			) ')' (<void> CONST | [] )
		| []
		)
			{ $$->type = point_to(&$_1, $$); }
	;

initializer <Expr_node*>
	: assignment_expr
		{
			$$ = $assignment_expr;

			if ($$)
				$$->mark_symbol(TRUE, FALSE, FALSE);
		}
	| '{' initializer_list '}' { $$ = $initializer_list; }
	;

initializer_list <Initializer_node*>
	{ Expr_node *expr; }
	: (<void>
		'[' constant_expr ']' ('='|[]) { expr = $constant_expr; }
		| [] { expr = NULL; }
		)
		initializer
			{
				$$ = new Initializer_node(g_curr_scope);
				$$->append($initializer, expr);
			}
		(<void> ','
				{
					if (w_nexttoken() == /*{*/'}' ||
							w_currtoken == W_EOI || w_currtoken == ';')
						goto done_initializer_list;
				}
			( '[' constant_expr ']' ('='|[]) { expr = $constant_expr; }
			| [] { expr = NULL; }
			)
			initializer
				{ $$->append($initializer, expr); }
		)* { done_initializer_list: ; }
	;

statement <Node*>
	{
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
	}
	: labeled_statement { $$ = $labeled_statement; }
	| compound_statement { $$ = $compound_statement; }
	| expression_statement { $$ = $expression_statement; }
	| selection_statement { $$ = $selection_statement; }
	| iteration_statement { $$ = $iteration_statement; }
	| jump_statement { $$ = $jump_statement; }
	// !ISO
	| asm_statement { $$ = $asm_statement; }
	| X_DECLARATION
			{
			do_decl:
				$$ = NULL;

				if (!g_cplusplus && g_strict_iso)
					warn(DECL_AS_STMT_IS_NOT_ISO);
			}
		declaration
	// force these tokens into the first set for "statement" to
	// mix declarations among statements - these will actually be
	// handled up above in the "declaration" rule
	| VOID | CHAR | SHORT | INT | LONG
	| SIGNED | UNSIGNED | FLOAT | DOUBLE
	| INLINE | FRIEND | VIRTUAL | TYPEDEF | EXTERN
	| STATIC | AUTO | REGISTER | PASCAL | FORTRAN
	| CLASS | STRUCT | UNION | ENUM | CONST | VOLATILE
	| TYPENAME //| IDENTIFIER | CLASSNAME
	;

labeled_statement <Node*>
	{ int line = g_cpp.linenum(); }
	: X_LABEL identifier ':' statement
			{
				if (g_func_scope == NULL)
					fatal(UNEXPECTED_LABEL);

				if (g_func_scope->symbol()->labels().find($identifier))
				{
					if (g_func_scope->symbol()->labels()().scope() != NULL)
						error(LABEL_ALREADY_DEFINED, g_func_scope->file(),
								g_func_scope->symbol()->labels()().line());
				}

				Symbol &sym = g_func_scope->symbol()->labels()[$identifier];
				sym.scope(g_func_scope);
				sym.file(g_cpp.filename());
				sym.line(line);
				sym.node($statement);
				$$ = new Label_node(g_curr_scope, &sym, $statement);
			}
	| CASE constant_expr
			(<Expr_node*> DOTDOTDOT constant_expr
				{
					if (g_strict_iso)
						warn(RANGED_CASE_IS_NOT_ISO);

					$$ = $constant_expr;
				}
			| [] { $$ = NULL; }
			)
			':' statement
		{
			$$ = new Case_node(g_curr_scope, $constant_expr,
					$_, $statement, g_curr_switch, line);
			g_curr_switch->cases().end() = $$;
		}
	| DEFAULT ':' statement
		{
			$$ = new Case_node(g_curr_scope, NULL, NULL, $statement,
					g_curr_switch, line);
			g_curr_switch->cases().end() = $$;
		}
	;

compound_statement <Node*>
	: '{'
			{
				Scope *scope = g_curr_scope;
				Scope *newscope = new Scope(g_cpp.filename(),
						g_cpp.linenum(), scope);
				g_curr_scope = newscope;
			}
		declaration_list statement_list '}'
			{
				g_curr_scope = scope;
				$$ = new Compound_node(newscope, $statement_list);
				newscope->node($$);
				check_unused_vars(newscope);
				newscope->cleanup();
			}
	;

declaration_list <boolean>
	: { $$ = FALSE; }
		(<void>
			{
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
			}
		declaration { $$ = TRUE; }
		)* { done_declaration_list: ; }
	;

statement_list <Node_arr*>
	:
			{
				Node_arr *sav = g_curr_statlist;
				g_curr_statlist = $$ = new Node_arr;
			}
		(<void> statement
			{
				if ($statement != NULL && $statement != g_null_node)
					$$->end() = $statement;
			}
		)*
			{ g_curr_statlist = sav; }
	;

expression_statement <Node*>
	: opt_expr ';'
			{ $$ = implicit_cast($opt_expr, g_void_type); }
	;

selection_statement <Node*>
	: IF '(' mark_expr ')' statement
			( ELSE statement { $$ = $statement; }
			| [] { $$ = NULL; }
			)
		{ $$ = new If_node(g_curr_scope, $mark_expr, $statement, $_); }
	| SWITCH '(' mark_expr ')'
			{
				Switch_node *savsw = g_curr_switch;
				Node *savbr = g_curr_break;
				g_curr_switch = new Switch_node(g_curr_scope, $mark_expr);
				g_curr_break = $$ = g_curr_switch;
			}
		statement
			{
				g_curr_switch->stmt($statement);
				g_curr_switch->check_cases();
				g_curr_switch = savsw;
				g_curr_break = savbr;
			}
	;

iteration_statement <Node*>
	: WHILE { g_loop_count++; }
		'(' mark_expr ')'
			{
				Node *savlp = g_curr_loop;
				Node *savbr = g_curr_break;
				While_node *w = new While_node(g_curr_scope, $mark_expr);
				$$ = g_curr_loop = g_curr_break = w;
			}
		statement
			{
				w->expr($mark_expr);
				w->stmt($statement);
				g_curr_loop = savlp;
				g_curr_break = savbr;
				g_loop_count--;
			}
	| DO
			{
				Node *savlp = g_curr_loop;
				Node *savbr = g_curr_break;
				Dowhile_node *d = new Dowhile_node(g_curr_scope);
				$$ = g_curr_loop = g_curr_break = d;
				g_loop_count++;
			}
		statement WHILE '(' mark_expr ')' ';'
			{
				d->stmt($statement);
				d->expr($mark_expr);
				g_curr_loop = savlp;
				g_curr_break = savbr;
				g_loop_count--;
			}
	| FOR '('
			{
				if (g_cplusplus && w_nexttoken() == CLASSNAME)
					w_currtoken = X_CLASSNAME;
			}
		(<Expr_node*> opt_expr ';' { $$ = $opt_expr; }
		|
				{ int tok = w_nexttoken(); }
			(<void>
				VOID | CHAR | SHORT | INT | LONG
				| SIGNED | UNSIGNED | FLOAT | DOUBLE
				| INLINE | FRIEND | VIRTUAL | TYPEDEF | EXTERN
				| STATIC | AUTO | REGISTER | PASCAL | FORTRAN
				| CLASS | STRUCT | UNION | ENUM | CONST | VOLATILE
				| TYPENAME | X_CLASSNAME //| CLASSNAME
			)
				{ w_currtoken = (tok == X_CLASSNAME) ? CLASSNAME : tok; }
			declaration
				{
					$$ = NULL;

					if (!g_cplusplus && g_strict_iso)
						warn(DECL_IN_FOR_IS_NOT_ISO);
				}
		)
			{ g_loop_count++; }
		opt_expr ';' opt_expr_no_mark ')'
			{
				Node *savlp = g_curr_loop;
				Node *savbr = g_curr_break;
				For_node *f = new For_node(g_curr_scope,
						implicit_cast($_, g_void_type),
						$opt_expr,
						implicit_cast($opt_expr_no_mark, g_void_type));
				$$ = g_curr_loop = g_curr_break = f;
			}
		statement
			{
				f->stmt($statement);
				g_curr_loop = savlp;
				g_curr_break = savbr;
				g_loop_count--;

				// this is here as incr/decr vars may be initialized in
				// the body of the for
				if ($opt_expr_no_mark)
					$opt_expr_no_mark->mark_symbol(TRUE, FALSE, FALSE);
			}
	;

mark_expr <Expr_node*>
	: expression
		{
			$$ = $expression;

			if ($$)
				$$->mark_symbol(TRUE, FALSE, FALSE);
		}
	;

opt_expr <Expr_node*>
	: expression
		{
			$$ = $expression;

			if ($$)
				$$->mark_symbol(TRUE, FALSE, FALSE);
		}
	| [] { $$ = NULL; }
	;

opt_expr_no_mark <Expr_node*>
	: expression { $$ = $expression; }
	| [] { $$ = NULL; }
	;

jump_statement <Node*>
	: GOTO identifier
			{
				Symbol &sym = g_func_scope->symbol()->labels()[$identifier];
				$$ = new Goto_node(g_curr_scope, &sym, g_cpp.linenum());
				sym.referenced(TRUE);

				// set the line-number to first used in case of error
				if (sym.scope() == NULL && sym.line() == 0)
					sym.line(g_cpp.linenum());
			}
		';'
	| CONTINUE ';'
		{
			if (g_curr_loop == NULL)
			{
				error(CONTINUE_OUTSIDE_LOOP);
				$$ = NULL;
			}
			else
				$$ = new Continue_node(g_curr_scope, g_curr_loop);
		}
	| BREAK ';'
		{
			if (g_curr_break == NULL)
			{
				error(BREAK_OUTSIDE_LOOP_OR_SWITCH);
				$$ = NULL;
			}
			else
				$$ = new Break_node(g_curr_scope, g_curr_break);
		}
	| RETURN opt_expr ';'
		{ $$ = new Return_node(g_curr_scope, $opt_expr); }
	;

asm_statement <Node*>
	: ASM { int line = g_cpp.linenum(); }
		'(' string_literal ')' ';'
			{ $$ = new Asm_node(g_curr_scope, $string_literal,
					g_cpp.filename(), line); }
	;

translation_unit %export
	: (
		declaration
		// !ISO
		| asm_statement { do_tree($asm_statement); }
		| ';'
	  )+
	;

{

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

}
