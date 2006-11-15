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

// Copyright (c) 1992,2000 by Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/bin/cc/ccrules.cc,v 1.84 2000/12/24 05:10:52 parag Exp $

#include "cpp.h"
#include "cc.h"
#include "cctype.h"
#include "ccsym.h"
#include "ccnode.h"
#include <ctype.h>
#include <stdiox.h>

long
strtochar(const char *s, boolean wide)
{
	long c = 0;
	long t;
	int num = 0;

	if (*s == '\'')
		s++;

	for (; *s != '\0' && *s != '\''; num++, s++)
	{
		c <<= wide ? g_size_wchar_t : g_size_char;

		// convert special escape char to actual char
		if (*s == '\\')
			switch (*++s)
			{
			case '\'': case '\"': case '?': case '\\':
				c += *s;
				break;

			// these are wired to the traditional C values for now
			case 'a': c += '\007'; break;		// alert (bell)
			case 'b': c += '\010'; break;		// backspace
			case 'f': c += '\014'; break;		// formfeed
			case 'n': c += '\012'; break;		// newline
			case 'r': c += '\015'; break;		// (carriage) return
			case 't': c += '\011'; break;		// (horizontal) tab
			case 'v': c += '\013'; break;		// vertical-tab

			case 'e':		// escape
				if (g_strict_iso)
					error(ILLEGAL_ESCAPE_IN_CHAR, *s);
				else
					c += '\033';
 
				break;

			case 'x':		// multi-digit hex value
				{
					t = 0;
					int digits = 1;		// to round up properly when divided by 2

					for (s++; isxdigit(*s); s++, digits++)
					{
						t <<= 4;
						t += *s < '9' ? *s - '0' : tolower(*s) - 'a' + 10;
					}

					c += t;
					int b = wide ? g_size_wchar_t / g_size_char : 1;

					if ((digits >> 1) > b)
						warn(HEX_CHARACTER_TOO_BIG, b);
				}

				break;

			// upto 3 digit octal value
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				t = *s - '0';

				if (s[1] >= '0' && s[1] <= '7')
				{
					t = ((unsigned)t << 3) + *++s - '0';

					if (s[1] >= '0' && s[1] <= '7')
						t = ((unsigned)t << 3) + *++s - '0';
				}
			
				c += t;
				break;

			default:
				if (g_strict_iso)
					error(ILLEGAL_ESCAPE_IN_CHAR, *s);
				else
					warn(ILLEGAL_ESCAPE_IN_CHAR, *s);

				break;
			}
		else
			c += *s;
	}

	if (num > 1 && (g_strict_iso || !g_macintosh))
		warn(EXTRA_JUNK_IN_CHAR);

	return c;
}

// prototyped function definition - build a type and defines the func
Symbol *
do_function_def(Storage_thing &func)
{
	boolean isinline = (func.flags & S_INLINE) ? TRUE : FALSE;
	func.make_type();
	Symbol *sym = NULL;

	if (func.type == NULL)
		return NULL;

	if (func.type->typeval() != T_FUNCTION)
	{
		error(SYMBOL_MUST_BE_FUNCTION_TYPE);
		return NULL;
	}

	if (func.storage != STATIC && func.storage != EXTERN &&
			func.storage != TYPEDEF && func.storage != NONE)
	{
		error(BAD_STORAGE_FOR_FUNCTION, func.sym);
		func.storage = NONE;
	}

	Function_type *ft = (Function_type*)func.type;

	// func.sym will be NULL for an abstract function declaration
	if (func.sym != NULL)
	{
		if (func.scope->symtab().find(func.sym))
		{
			// function is already declared
			sym = &func.scope->symtab()();

			// C++ function names are overloaded
			if (g_cplusplus)
			{
				Symbol *s = sym->exact_match(ft);

				if (s == NULL)
				{
					sym = sym->add_overload(ft);

					if (sym == NULL)
						bug("do_function_def(): overload error for %s",
								func.sym);
				}
				else
					sym = s;
			}

			// it already has a body
			if (sym->node() != NULL)
			{
				error(FUNCTION_ALREADY_DEFINED, func.sym,
						sym->line(), sym->file());
				return NULL;
			}

			if (!equivalent(sym->type(), ft))
			{
				error(INCOMPATIBLE_TYPES_FOR_IDENT, func.sym,
						sym->line(), sym->file());
				return NULL;
			}

			if (sym->storage() == NONE)
				sym->storage(func.storage);
			else if (func.storage == STATIC && sym->storage() != STATIC)
			{
				warn(INCOMPATIBLE_STORAGE_FOR_IDENT, func.sym,
						sym->line(), sym->file());
				sym->storage(STATIC);
			}
		}
		else
		{
			// no such function - just stick in a definition for it
			func.scope->symtab().insert(func.sym);
			sym = &func.scope->symtab()();
			sym->type(ft);
			sym->storage(func.storage);
		}

		// create a new scope for this function
		g_func_scope = g_curr_scope = new Scope(g_cpp.filename(),
				g_cpp.linenum(), func.scope);
		g_func_scope->symbol(sym);
		sym->new_labels();

		// update the file/line nums to where the function really is
		sym->file(g_cpp.filename());
		sym->line(g_cpp.linenum());

		// add "this" into the scope if this is a member function
		if (g_cplusplus && ft->ismemfunc())
		{
			Symbol &s = g_func_scope->symtab()[strget("this")];
			Type *t = new Pointer_type(ft->classname()->type());
			s.type(pool_type(t));
			t->free();
			s.scope(g_func_scope);
			s.parameter(TRUE);
			s.file(g_cpp.filename());
			s.line(g_cpp.linenum());
			s.read(1);

			// add this to the init list for this scope - this will be
			// the first entry in the list for tree generation
			if (g_func_scope->init() == NULL)
				g_func_scope->new_init();

			g_func_scope->init()->end() = &s;
		}

		// check and insert _args into the new scope
		if (ft->args() != NULL)
		{
			Param_arr *args = ft->args();
			Symbol_tab &tab = g_func_scope->symtab();

			for (int i = 0; i < args->size(); i++)
			{
				// void arguments are illegal - func taking no args
				// will have "args" be size 0
				if (args->elt(i).type()->isvoid())
				{
					error(PARAMETER_CANNOT_BE_VOID, args->elt(i).symbol());
					return NULL;
				}

				// unnamed args are useful to eliminate unused sym warnings
				if (args->elt(i).symbol() == NULL)
				{
					if (g_strict_iso)
						warn(NO_NAME_FOR_FUNC_PARAMETER, i + 1, func.sym);

					args->elt(i).symbol(make_unnamed_name());
				}

				// see if this arg is already defined, which would be bad
				if (tab.find(args->elt(i).symbol()) && tab().scope() != NULL)
				{
					error(PARAMETER_ALREADY_DEFINED, args->elt(i).symbol());
					return NULL;
				}

				// put this parameter in the symbol table
				Symbol &s = tab[args->elt(i).symbol()];
				s.type(args->elt(i).type());
				s.storage(args->elt(i).storage());
				s.scope(g_func_scope);
				s.parameter(TRUE);
				s.file(g_cpp.filename());
				s.line(g_cpp.linenum());

				// add it to the init list for this scope - these will be
				// the first entries within the list
				if (g_func_scope->init() == NULL)
					g_func_scope->new_init();

				g_func_scope->init()->end() = &s;
			}
		}
	}

	if (isinline)
		sym->is_inline(TRUE);

	//fprintf(stderr, "function %s is ", func.sym);		// DEBUG
	//ret.type->debug();		// DEBUG

	return sym;
}

// define an old-style function, but we cannot check it until later
void
do_old_function_def(Storage_thing &func)
{
	if (func.basic == UNKNOWN || func.flags == S_UNKNOWN)
		return;

	if (func.sym == NULL || func.type == NULL)
		bug("do_old_function_def(): NULL function name or type");

	if (func.storage != STATIC && func.storage != EXTERN &&
			func.storage != TYPEDEF && func.storage != NONE)
	{
		error(BAD_STORAGE_FOR_FUNCTION, func.sym);
		func.storage = NONE;
	}

	Symbol *sym;

	if (g_file_scope->symtab().find(func.sym))
	{
		sym = &g_file_scope->symtab()();

		if (sym->node() != NULL)
		{
			error(FUNCTION_ALREADY_DEFINED, func.sym,
					sym->line(), sym->file());
			return;
		}
	}
	else
	{
		g_file_scope->symtab().insert(func.sym);
		sym = &g_file_scope->symtab()();
		sym->storage(func.storage);
		// sym->type is not set since it is not yet complete
	}

	if (sym->node() != NULL)
		bug("do_old_function_def(): node defined for func %s", func.sym);

	g_curr_scope = g_func_scope = new Scope(g_cpp.filename(),
			g_cpp.linenum(), g_curr_scope);
	g_curr_scope->symbol(sym);
	sym->new_labels();
}

// now we can check the old-style function argument parameters
Symbol *
check_old_function_def(Storage_thing &func)
{
	if (func.sym == NULL || func.type == NULL)
		bug("do_old_function_def(): NULL function name or type");

	Function_type *ft = (Function_type*)func.type;

	if (!g_file_scope->symtab().find(func.sym))
		bug("do_old_function_def(): function missing in scope?!?");

	Symbol *sym = &g_file_scope->symtab()();

	if (sym->node() != NULL)
		return NULL;

	// the only items inserted into the function symtab should be
	// old-style parameter definitions - ft->args() should be the
	// as yet incomplete parameter names

	// check that all _args are in the symtab - any missing ones are made "int"
	Param_arr *args = ft->args();
	Symbol_tab &tab = g_func_scope->symtab();

	if (args != NULL)
	{
		for (int i = 0; i < args->size(); i++)
		{
			if (args->elt(i).symbol() == NULL)
				bug("check_old_function_def(): NULL args symbol %d", i);

			// insert this symbol into the table if it is not already there
			Symbol &s = tab[args->elt(i).symbol()];

			// if it has no type, make it "int"
			if (s.type() == NULL)
			{
				s.type(g_int_type);
				s.file(g_cpp.filename());
				s.line(g_cpp.linenum());
			}

			s.scope(g_func_scope);		// may or may not already be set
			s.parameter(TRUE);

			// now we can finally initialize the function args data
			args->elt(i).type(s.type());
			args->elt(i).storage(s.storage());

			// add it to the init list for this scope - these will be
			// the first entries within the list
			if (g_func_scope->init() == NULL)
				g_func_scope->new_init();

			g_func_scope->init()->end() = &s;
		}
	}

	// any items in the symtab which are not in the arg list are errors
	for (Symbol_iter ti = tab; ti; ++ti)
	{
		Symbol &s = ti();
		int i;

		// linear search for now
		for (i = 0; i < args->size(); i++)
			if (args->elt(i).symbol() == s.name())
				break;

		if (i >= args->size())
		{
			error(OLD_STYLE_ARG_DECL_NOT_SPECIFIED, s.name());
			return NULL;
		}
	}

	// now we can finally build the final type for the pool, then check it
	func.make_type();

	if (sym->type() == NULL)
		sym->type(func.type);
	else if (!equivalent(sym->type(), func.type))
	{
		// should never be called, but may as well check anyway...
		error(INCOMPATIBLE_TYPES_FOR_IDENT, func.sym,
				sym->line(), sym->file());
		return NULL;
	}

	if (sym->storage() == NONE)
		sym->storage(func.storage);
	else if (func.storage == STATIC && sym->storage() != STATIC)
	{
		warn(INCOMPATIBLE_STORAGE_FOR_IDENT, func.sym,
				sym->line(), sym->file());
		sym->storage(STATIC);
	}

	//fprintf(stderr, "function %s is ", func.sym);		// DEBUG
	//ret.type->debug();		// DEBUG
	return sym;
}

// look for unused vars within this function
void
check_unused_vars(Scope *scope, Symbol *func)
{
	for (Symbol_iter i = scope->symtab(); i; ++i)
	{
		if (i().type() == NULL)
			error(SYMBOL_NOT_DECLARED, i().name(), i().line());
		else if (!i().unused() &&
				!i().read() &&
				!i().wrote() &&
				!i().referenced() &&
				i().name()[0] != UNNAMED_CHAR &&
				i().type() != g_unknown_type &&
				!i().type()->isanyfunc())
		{
			if (func)
				warn(SYMBOL_NEVER_USED_IN_FUNC, i().name(), i().line(),
						func->name());
			else
				warn(SYMBOL_NEVER_USED_IN_SCOPE, i().name(), i().line());
		}
	}
}

// check for a return at the end of a function - not completely accurate
void
check_for_return(Symbol *func)
{
	Function_type *ft = (Function_type*)func->type();

	if (ft == NULL || ft->typeval() != T_FUNCTION)
		return;

	Node_type last = func->node()->check_return();

	if (ft->retval() == NULL || ft->retval() == g_void_type)
	{
		if (last == N_RETURN)
			warn(USELESS_RETURN_AT_FUNC_END, func->name());
	}
	else if (last != N_RETURN && last != N_ASM)
		warn(MISSING_RETURN_AT_FUNC_END, func->name());
}

// check for missing labels used in "goto" statements but never defined,
// and warn about unused labels
void
check_labels(Symbol *func)
{
	if (!func->has_labels())
		return;

	for (Symbol_iter si = func->labels(); si; ++si)
		if (si().scope() == NULL)
			error(LABEL_NOT_DEFINED, si().name(), si().line());
		else if (!si().referenced())
			warn(LABEL_NOT_USED, si().name(), si().line());
}

void
fini_function_def(Symbol *func, Node_arr *body)
{
	if (func == NULL)
		return;

	Scope *scope = g_func_scope;

	if (scope == NULL)
		return;

	g_curr_scope = scope->parent();
	g_func_scope = NULL;

	Node *cn = new Compound_node(scope, body);
	scope->node(cn);
	func->node(cn);
	func->scope(scope);
	check_unused_vars(scope, func);
	check_for_return(func);
	scope->cleanup();
	func->cleanup_labels();
	check_labels(func);
	do_tree(func);
}

// this is a constructor initialization for a variable
void
do_ctor_init(Symbol *sym, Expr_arr *init)
{
	if (sym == NULL || sym->storage() == EXTERN)
		return;

	if (!sym->type()->isclass())
	{
		error(EXPECTED_CLASS_INSTANCE, sym->name());
		return;
	}

	Struct_type *st = (Struct_type*)sym->type();
	Scope *scope = st->scope();

	if (scope == NULL)
		scope = g_curr_scope;

	// first setup for destruction at the end of this scope
	//Symbol *s = st->scope()->lookup_member(make_destructor_name(st->name()));

	if (sym->storage() != '&')
	{
		if (scope->fini() == NULL)
			scope->new_fini();

		scope->fini()->end() = sym;
	}

	// now setup for initialization, if any
	if (init)
		for (int i = 0; i < init->size(); i++)
			init->elt(i)->mark_symbol(TRUE, FALSE, FALSE);

	// all local symbols are added to init list for original decl order
	if (scope->init() == NULL)
		scope->new_init();

	scope->init()->end() = sym;

	if (sym->storage() == '&')
		return;

	// find and call constructor func to initialize this var
	Symbol *s = st->scope()->lookup_member(make_constructor_name(st->name()));

	if (s == NULL)
	{
		if (init && init->size() > 0)
			error(NO_CONSTRUCTORS_FOR_CLASS, st->name());

		return;
	}

	s = s->overload_match(init);

	if (s == NULL)
	{
		error(NO_MATCH_FOR_CONSTRUCTOR, st->name());
		return;
	}

	sym->node(new Func_call_node(scope,
			new Symbol_node(scope, s, g_cpp.filename(), g_cpp.linenum()),
			init));
}

// handle the initializer for a variable
void
do_initializer(Symbol *sym, Expr_node *init, boolean isparam)
{
	if (sym == NULL)		// error in do_declaration(), so punt
		return;

	if (sym->type()->isclass())
	{
		Expr_arr *arr = new Expr_arr;

		if (init)
			(*arr)[0] = init;

		do_ctor_init(sym, arr);
		return;
	}

	Scope *scope = sym->scope();

	if (scope == NULL)
		scope = g_curr_scope;

	// all local symbols are added to init list for original decl order
	if (scope->init() == NULL)
		scope->new_init();

	scope->init()->end() = sym;

	if (init == NULL)		// no initializer for this symbol
	{
		// C++ references must be initialized
		if (sym->storage() == '&' && !sym->parameter())
			error(REFERENCE_IS_NOT_INITIALIZED, sym->name());
		// any global or local that is "const" ought to be initialized,
		// but watch out for function parameters, new and old-style
		else if (sym->type()->isconst() && sym->node() == NULL &&
				sym->storage() != EXTERN && sym->storage() != TYPEDEF &&
				!sym->parameter() && !isparam)
			warn(CONST_SYMBOL_IS_NOT_INITIALIZED, sym->name());

		// dummy symbol to mark this type as read for back-end
		if (!sym->type()->issue())
			return;

		Symbol_node symnode(scope, sym, "", 1);
		symnode.mark_symbol(TRUE, TRUE, FALSE);
		return;
	}

	// extern symbols should not be initialized in local scopes
	if (sym->storage() == EXTERN && !sym->type()->isanyfunc())
	{
		if (scope != g_file_scope)
		{
			error(EXTERN_SYMBOL_IS_INITIALIZED, sym->name());
			return;
		}

		warn(EXTERN_SYMBOL_IS_INITIALIZED, sym->name());
		sym->storage(NONE);
	}

	Integer newlen(g_size_t_type->ic());

	if (!check_init_type(init, sym->type(), newlen))
		return;

	if (sym->node() != NULL)
	{
		error(SYMBOL_IS_ALREADY_INITIALIZED, sym->name(), sym->line());
		return;
	}

	// if this symbol is of array type, and that type is incomplete,
	// then since it has an initializer, we can complete it

	if (sym->type()->typeval() == T_ARRAY && newlen.isPos())
	{
		Array_type *at = (Array_type*)sym->type();

		if (at->len().isZero())
		{
			Type *t = new Array_type(at->type(), newlen); // complete
			sym->type(pool_type(t));
			t->free();
		}
		else if (newlen > at->len())
		{
			error(TOO_MANY_INITIALIZERS_FOR_ARRAY);
			return;
		}
	}

	sym->node(init);

	// statics, globals, and arrays must have constant initializers
	if (scope == g_file_scope || sym->storage() == STATIC ||
			sym->type()->isarr() || sym->type()->isfunc())
	{
		int refcount = 0;

		if (!init->is_static_init(&refcount) || refcount > 1)
			error(EXPECTED_CONST_INITIALIZERS, sym->name());

		// function with initializer is asm of some form
		if (sym->type()->isfunc())
			sym->is_asm(TRUE);
	}

	if (init)
		init->mark_symbol(TRUE, FALSE, FALSE);
}

void
do_member_init(const char *symstr, Expr_arr *init)
{
	if (symstr == NULL)
		return;

	Symbol *func = g_curr_scope->symbol();

	if (func == NULL)
		return;

	if (!func->type()->isfunc())
	{
		error(ILLEGAL_EXPR_IN_CTOR);
		return;
	}

	Function_type *ft = (Function_type*)func->type();

	if (ft->classname() == NULL || !ft->classname()->type()->isclass())
	{
		error(CTOR_INIT_IN_NON_MEMBER);
		return;
	}

	Struct_type *st = (Struct_type*)ft->classname()->type();
	const char *member;
	Symbol *sym = g_curr_scope->lookup_sym(symstr);

	if (sym->type()->isclass())
	{
		Struct_type *sst;
		
		// base-class initialization
		for (sst = st->base(); sst; sst = sst->base())
			if (symstr == sst->name())
				break;
		
		if (sst != NULL)
			member = make_constructor_name(sst->name());
		else
			member = symstr;
	}
	else
	{
		// class member initialization
		member = symstr;
	}

	Scope *scope = st->scope();
	Symbol *s = scope->lookup_member(member);

	if (s == NULL)
	{
		if (init)
			error(NO_MEMBER_FOR_CLASS, member, st->name());

		return;
	}

	if (s->type()->isoverload())
	{
		s = s->overload_match(init);

		if (s == NULL)
		{
			error(NO_MATCH_FOR_MEMBER, member, st->name());
			return;
		}
	}

	s = new Symbol(*s);
	s->init(init);

	if (scope->ctor() == NULL)
		scope->new_ctor();
	
	scope->ctor()->end() = s;

	if (init)
		for (int i = 0; i < init->size(); i++)
			init->elt(i)->mark_symbol(TRUE, FALSE, FALSE);
}

Type *
do_array(Expr_node *expr, Type *type, boolean isparam)
{
	Const_expr isize;
	isize.new_ival(g_size_t_type->ic());

	if (expr != NULL)
		expr->eval_const_expr(isize);

	if (isize.ival->isPos() || (expr == NULL &&
					(type == NULL || !type->isarr())))
	{
		// for sloppy function params, "type p[]" is changed to "type *p".
		if (isparam && g_sloppy_args && expr == NULL)
			type = new Pointer_type(type);
		else
			type = new Array_type(type, *isize.ival, expr != NULL);
	}
	else
		error(ARRAY_MUST_HAVE_POSITIVE_SIZE);

	return type;
}

Expr_node *
do_struct_display(Type *cast, Initializer_node *init)
{
	if (g_strict_iso)
		warn(STRUCT_DISPLAY_IS_NOT_ISO);

	// create a temp var in the current scope which is inited
	// to this init list, then return a symbol node to the new var
	const char *id = make_unnamed_name();
	Storage_thing st;
	st.sym = id;
	st.type = cast;
	Symbol *sym = st.do_declaration();
	do_initializer(sym, init);
	do_tree(sym);
	return new Symbol_node(g_curr_scope, sym,
			g_cpp.filename(), g_cpp.linenum());
}

void
init_parser()
{
	g_file_scope = new Scope(g_cpp.filename(), g_cpp.linenum(), NULL);
	g_curr_scope = g_file_scope;
	g_loop_count = 0;
}
