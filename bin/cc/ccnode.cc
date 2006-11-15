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

// Copyright (c) 1992-2003 by Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/bin/cc/ccnode.cc,v 1.62 2003/04/29 21:56:09 parag Exp $

#if defined(__GNUC__) && !defined(__APPLE__)
#pragma implementation
#endif

#include <stddef.h>
#include <ctype.h>
#include <stringx.h>
#include "cctype.h"
#include "ccsym.h"
#include "ccnode.h"
#include "globals.h"

Integer_node::Integer_node(Int_type *t, Scope *s, const char *text) :
		Expr_node(t, s)
{
	int base = 10;

	if (*text == '0')
	{
		base = 8;
		text++;

		if (*text == 'x' || *text == 'X')
		{
			base = 16;
			text++;
		}
	}

	const char *end;
	_val = new Integer(t->ic(), text, &end, base);

	if (end == NULL)
	{
		error(BADLY_FORMED_NUMBER, '?');
		return;
	}

	int c = tolower(*end);

	if (c != 'u' && c != 'l' && c != '\0')
		error(BADLY_FORMED_NUMBER, c);
}

Integer_node::Integer_node(Int_type *t, Scope *s, long val) : Expr_node(t, s)
{
	_val = new Integer(t->ic(), val);
}

Integer_node::Integer_node(Type *t, Scope *s, Integer *val) : Expr_node(t, s)
{
	_val = val;
}

Float_node::Float_node(Float_type *t, Scope *s, const char *text)
		: Expr_node(t, s)
{
	const char *end;
	_val = new Float(t->fc(), text, &end);

	if (end == NULL)
	{
		error(BADLY_FORMED_NUMBER, '?');
		return;
	}

	int c = tolower(*end);

	if (c != 'f' && c != 'd' && c != 'f' && c != '\0')
		error(BADLY_FORMED_NUMBER, c);
}

String_node::String_node(Scope *s, boolean wide)
		: Expr_node(s)
{
	_string = NULL;
	_len = 0;
	_wide = wide;
	_pascal = FALSE;
	_forth = FALSE;
	_dup = TRUE;
	_label = NULL;
}

String_node::String_node(String_node &str)
{
	_scope = str._scope;
	_type = str._type;
	_string = str._string;
	_len = str._len;
	_wide = str._wide;
	_pascal = str._pascal;
	_pascal = str._pascal;
	_dup = str._dup;
	_label = str._label;

	if (_dup)
	{
		int c = _wide ? g_size_wchar_t / g_size_char : 1;
		size_t len = _len * c;
		char *string = strnew(len);
		memcpy(string, str._string, len);
		_string = string;
	}
}

void
String_node::finish()
{
	if (_string && _len == strlen(_string))
	{
		const char *str = _string;
		_string = strget(str);
		strfree(str);
		_dup = FALSE;
	}

	if (_forth)
	{
		if (g_forth_string_type == NULL)
		{
			Symbol *s = NULL;

			if (g_file_scope->suetypes().find(strget("$fstr")))
				s = &g_file_scope->suetypes()();
			
			if (s == NULL)
			{
				error(STRUCT_FORTH_STRING_NOT_DEFINED);
				return;
			}

			g_forth_string_type = s->type();
		}

		type(g_forth_string_type);
		return;
	}

	// build the type for this string, now that it is complete
	Type *tp = _pascal ? g_pascal_string_type :
			(_wide ? g_wstring_type : g_string_type);
	Integer len(g_size_t_type->ic(), _len + 1);
	Type *t = new Array_type(tp, len);
	t->isconst(TRUE);
	type(pool_type(t));
	t->free();
}

Symbol_node::Symbol_node(Scope *scp, const char *sym, const char *file,
		int line) : Expr_node(scp)
{
	_file = file;
	_line = line;

	// look for this symbol in "scp" or its parents
	_sym = scp->lookup_sym(sym);

	if (_sym == NULL)
	{
		// add to file scope for automatic global function externs
		_sym = &g_file_scope->symtab()[sym];
		_sym->file(file);
		_sym->line(line);

		// and add to local scope in case this is an undefined variable
		_sym = &scp->symtab()[sym];
		_sym->file(file);
		_sym->line(line);

		//our _type defaults to g_unknown_type, both _sym->type() is NULL
	}
	else
		type(_sym->type());
}

Symbol_node::Symbol_node(Scope *scp, Symbol *sym, const char *file, int line)
		: Expr_node(scp)
{
	_file = file;
	_line = line;
	_sym = sym;
	type(_sym->type());
}

Array_ref_node::Array_ref_node(Scope *s, Expr_node *arr, Expr_node *expr)
		: Expr_node(s)
{
	Type_val et = expr->type() == NULL ? T_UNKNOWN : expr->type()->typeval();

	// *&#$ C spec allows switching these so a[b] is equivalent to b[a]
	if (et == T_POINTER || et == T_ARRAY)
	{
		warn(ARRAY_EXPRESSION_IS_BACKWARDS);
		Expr_node *t = expr;
		expr = arr;
		arr = t;
		et = expr->type()->typeval();
	}

	_arr = arr;
	_expr = implicit_cast(expr, g_size_t_type);
	Type *t = arr->type();

	switch (t->typeval())
	{
	case T_ARRAY:
		type(((Array_type*)t)->type());
		break;

	case T_POINTER:
		type(((Pointer_type*)t)->type());
		break;

	case T_UNKNOWN:
		break;

	default:
		error(DEREF_OF_NON_ARRAY);
		break;
	}

	if (et != T_INT && et != T_BITFIELD && et != T_ENUM)
	{
		error(NON_INT_ARRAY_DEREF);
		type(g_unknown_type);
	}
}

Func_call_node::Func_call_node(Scope *scp, Expr_node *f, Expr_arr *a)
		: Expr_node(scp)
{
	_func = f;
	_args = a;
	_overload = NULL;

	if (f == NULL || f->type() == NULL)
	{
		type(g_unknown_type);
		return;
	}

	// check to see "f" is a function or a pointer to a function
	Function_type *ft = (Function_type*)f->type();

	// if the type of this function (or func pointer) is not known, then
	// make it a generic extern function to eliminate extra error messages
	if (ft == NULL || ft == g_unknown_type)
	{
		const char *sym = f->name();
		warn(FUNCTION_NOT_DECLARED, sym);

		if (sym == NULL || !g_file_scope->symtab().find(sym))
			return;

		// create a generic extern function type: "int func();"
		Function_type *nft = new Function_type;
		nft->retval(g_int_type);
		ft = (Function_type*)pool_type(nft);
		nft->free();

		// and set this symbol to be of this function type
		f->type(ft);
		Symbol *func = &g_file_scope->symtab()();
		func->type(ft);
		func->storage(EXTERN);

		// may also be in local scope, so set its type if it is unknown
		if (scp->symtab().find(sym))
		{
			func = &scp->symtab()();

			if (func->type() == NULL || func->type() == g_unknown_type)
			{
				func->type(ft);
				func->storage(EXTERN);
			}
		}
	}

	if (ft->isptr())
	{
		Pointer_type *tp = (Pointer_type*)ft;
		ft = (Function_type*)tp->type();
	}

	// make sure that ft is pointing to a legit Function_type
	if (ft == NULL)
	{
		type(g_unknown_type);
		return;
	}

	if (!ft->isfunc())
	{
		if (ft->isoverload())
		{
			Symbol *sym = ((Overload_type*)ft)->sym();
			sym = sym->overload_match(a);

			if (sym == NULL || !sym->type()->isfunc())
			{
				error(NO_MATCH_FOR_OVERLOADED_FUNC, f->name());
				sym = ((Overload_type*)ft)->sym()->overload_match(a);
				return;
			}

			ft = (Function_type*)sym->type();
			_overload = ft;
		}
		else
		{
			error(ATTEMPT_TO_CALL_NON_FUNCTION);
			return;
		}
	}

	// check arguments of args all match, or auto-cast to types that match
	// while handling "..." properly
	Param_arr *args = ft->args();
	int i = 0;
	int asize = (a == NULL) ? 0 : a->size();

	if (args != NULL)		// no args means anything goes
	{
		for (i = 0; i < args->size(); i++)
		{
			// handle the results if any error has occurred
			if (i < asize && a->elt(i)->type()->typeval() == T_UNKNOWN)
				break;

			// out of argument exprs and this param has no default arg
			if (i >= asize && args->elt(i).init() == NULL)
			{
				error(NOT_ENOUGH_ARGS_FOR_FUNC_CALL, f->name());
				return;
			}

			if (i < asize && args->elt(i).type() == NULL)
				bug("Func_call_node::Func_call_node(): NULL arg[%d] type", i);

			if (i < asize && !valid_ecast(a->elt(i), args->elt(i).type()))
			{
				error(FUNC_ARGUMENT_TYPE_MISMATCH, i + 1, f->name());
				return;
			}

			// make the implicit cast explicit
			if (i < asize)
				a->elt(i) = implicit_cast(a->elt(i), args->elt(i).type());
		}
	}
	
	if (i < asize && !ft->varargs())
	{
		if (g_cplusplus)
			error(TOO_MANY_ARGS_FOR_FUNC_CALL, f->name());
		else if (args != NULL)
			warn(TOO_MANY_ARGS_FOR_FUNC_CALL, f->name());
	}

	// any leftover args should be cast to "int" or "double" as appropriate
	for (; i < asize; i++)
	{
		Type *t = a->elt(i)->type();

		if (t->isfloat() &&
				((Float_type*)t)->width() < g_double_type->width())
			a->elt(i) = implicit_cast(a->elt(i), g_double_type);
		else if ((t->isint() || t->isbit()) &&
				((Int_type*)t)->width() < g_int_type->width())
			a->elt(i) = implicit_cast(a->elt(i), g_int_type);
	}

	// set this node to the return type of the function
	type(ft->retval());
}

Struct_ref_node::Struct_ref_node(Scope *scp, Expr_node *sref, const char *tag,
		const char *file, int line, boolean pointer) : Expr_node(scp)
{
	_structref = NULL;
	_elem = NULL;
	_file = file;
	_line = line;

	if (sref == NULL || sref->type() == NULL ||
			sref->type()->typeval() == T_UNKNOWN)
		return;

	Struct_type *st = (Struct_type*)sref->type();

	// if this is a pointer lookup ("->", not "."), get its type
	if (pointer)
	{
		if (st->typeval() != T_POINTER)
		{
			error(DEREF_OF_NON_POINTER);
			return;
		}

		Pointer_type *tp = (Pointer_type*)st;
		st = (Struct_type*)tp->type();
	}

	if (st->typeval() != T_STRUCT && st->typeval() != T_UNION)
	{
		error(STRUCT_REF_OF_NON_STRUCT, tag);
		return;
	}

	if (!st->complete())
	{
		error(STRUCT_REF_OF_INCOMPLETE, tag);
		return;
	}

	// at this point, we know st is pointing to a legit Struct_type/union


	// look for this tag in this class and all its base classes
	if (!st->members()->find(tag))
	{
		if (st->typeval() == T_UNION)
			error(NO_SUCH_MEMBER_IN_UNION, tag, st->name());
		else
			error(NO_SUCH_MEMBER_IN_STRUCT, tag, st->name());

		return;
	}

	// if type is of pointer type, then this was a "->" ref, so we
	// need to dereference it before we can access it
	if (pointer)
		_structref = new Operator_node(scp, '*', sref);
	else
		_structref = sref;

	_elem = &(*st->members())();
	type(_elem->type());
}

Operator_node::Operator_node(Scope *s, int tok, Expr_node *a, boolean p)
		: Expr_node(s)
{
	_token = tok;
	_arg = a;
	_prefix = p;

	Type *at = a ? a->type() : g_unknown_type;

	if (at == NULL)
		at = g_unknown_type;

	Type_val av = at->typeval();

	if (av == T_UNKNOWN)
		return;

	if (av == T_ENUM || av == T_BITFIELD)
		av = T_INT;

	switch (tok)
	{
	case '&':		// address of
		if (av != T_FUNCTION && av != T_ARRAY &&
				(at->typeval() == T_BITFIELD || !a->is_lvalue()))
			error(CANNOT_TAKE_ADDR_OF_OBJ);
		else
		{
			Type *t = new Pointer_type(at);
			t->isconst(TRUE);
			type(pool_type(t));
			t->free();
		}

		break;

	case '*':		// pointer
		if (av == T_POINTER)
			type(((Pointer_type*)at)->type());
		else if (av == T_ARRAY)
			type(((Array_type*)at)->type());
		else
			error(DEREF_OF_NON_POINTER);

		break;

	case INCR: case DECR:
		if (av == T_POINTER || av == T_INT || av == T_FLOAT)
		{
			if (at->isconst())
				error(CANNOT_INCR_DECR_CONST);
			else 
			{
				if (at->isvolatile())
					warn(SHOULD_NOT_INCR_DECR_VOLATILE);

				type(base_type(at));
			}
		}
		else
			error(EXPECTED_SCALAR_EXPR);

		break;

	case '+': case '-':
		if (av == T_INT || av == T_FLOAT)
		{
			type(arith_conv(at, g_int_type));
			_arg = implicit_cast(_arg, _type);
		}
		else
			error(EXPECTED_INT_OR_FLOAT);

		break;

	case '~':
		if (av == T_INT)
		{
			type(arith_conv(at, g_int_type));
			_arg = implicit_cast(_arg, _type);
		}
		else
			error(EXPECTED_INT);

		break;

	case '!':
		if (av == T_INT || av == T_FLOAT || av == T_POINTER)
			type(g_int_type);
		else
			error(EXPECTED_SCALAR_EXPR);

		break;

	default:
		bug("Operator_node constructor: token %d (%c)", tok, tok);
		break;
	}
}

Operator2_node::Operator2_node(Scope *s, int tok, Expr_node *a1, Expr_node *a2)
		: Expr_node(s)
{
	_token = tok;
	_arg1 = a1;
	_arg2 = a2;

	Type *at1 = a1->type();
	Type *at2 = a2->type();
	Type_val av1 = at1->typeval();
	Type_val av2 = at2->typeval();

	if (av1 == T_UNKNOWN || av2 == T_UNKNOWN)
		return;

	// zero, enums, and bitfields are considered ints for type-checks

	if (av1 == T_ENUM || av1 == T_BITFIELD)
		av1 = T_INT;

	if (av2 == T_ENUM || av2 == T_BITFIELD)
		av2 = T_INT;

	switch (tok)
	{
	// int, float
	case '*': case '/':
		if ((av1 == T_INT || av1 == T_FLOAT)
				&& (av2 == T_INT || av2 == T_FLOAT))
		{
			type(arith_conv(at1, at2));
			_arg1 = implicit_cast(_arg1, _type);
			_arg2 = implicit_cast(_arg2, _type);
		}
		else
			error(EXPECTED_ARITH_EXPR);

		break;

	// ptr&ptr, int, float, ptr&int
	case '-':
		if ((av1 == T_POINTER || av1 == T_ARRAY) &&
				(av2 == T_POINTER || av2 == T_ARRAY))
		{
			Type *t1 = (av1 == T_POINTER) ? ((Pointer_type*)at1)->type()
						: ((Array_type*)at1)->type();
			Type *t2 = (av2 == T_POINTER) ? ((Pointer_type*)at2)->type()
						: ((Array_type*)at2)->type();

			if (t1 == t2 || (valid_cast(t2, t1) || valid_cast(t1, t2)))
				type(g_ptrdiff_t_type);
			else
				error(EXPECTED_SCALAR_EXPR);

			break;
		}
		else if ((av1 == T_POINTER || av1 == T_ARRAY) && av2 == T_INT)
			type(at1);
		else if (av1 == T_INT && (av2 == T_POINTER || av2 == T_ARRAY))
		{
			type(at2);
			_arg1 = a2;				// swap args so 1st is always pointer/array
			_arg2 = a1;
		}
		else if ((av1 == T_INT || av1 == T_FLOAT)
				&& (av2 == T_INT || av2 == T_FLOAT))
		{
			type(arith_conv(at1, at2));
			_arg1 = implicit_cast(_arg1, _type);
			_arg2 = implicit_cast(_arg2, _type);
		}
		else
			error(EXPECTED_SCALAR_EXPR);

		break;

	// int, float, ptr&int
	case '+':
		if ((av1 == T_POINTER || av1 == T_ARRAY) && av2 == T_INT)
			type(at1);
		else if (av1 == T_INT && (av2 == T_POINTER || av2 == T_ARRAY))
		{
			type(at2);
			_arg1 = a2;				// swap args so 1st is always pointer/array
			_arg2 = a1;
		}
		else if ((av1 == T_INT || av1 == T_FLOAT)
				&& (av2 == T_INT || av2 == T_FLOAT))
		{
			type(arith_conv(at1, at2));
			_arg1 = implicit_cast(_arg1, _type);
			_arg2 = implicit_cast(_arg2, _type);
		}
		else
			error(EXPECTED_SCALAR_EXPR);

		break;

	// int
	case '%':
	case LSHIFT: case RSHIFT:
	case '&': case '^': case '|':
		if (av1 == T_INT && av2 == T_INT)
		{
			type(arith_conv(at1, at2));
			_arg1 = implicit_cast(_arg1, _type);
			_arg2 = implicit_cast(_arg2, _type);
		}
		else
			error(EXPECTED_INT_EXPR);

		break;

	// int, ptr
	case ANDAND: case OROR:
		if ((av1 == T_INT || av1 == T_FLOAT ||
						av1 == T_POINTER || av1 == T_ARRAY)
				&& (av2 == T_INT || av2 == T_FLOAT ||
						av2 == T_POINTER || av2 == T_ARRAY))
		{
			if (av1 == T_ARRAY || av2 == T_ARRAY)
				warn(COMPARING_POINTER_CONSTANTS);

			type(g_int_type);
		}
		else
			error(EXPECTED_SCALAR_EXPR);

		break;

	// int, float, ptr
	case '<': case '>': case LESSEQ: case GREATEQ:
		if ((av1 == T_INT || av1 == T_FLOAT)
				&& (av2 == T_INT || av2 == T_FLOAT))
		{
			type(g_int_type);
			Type *t = arith_conv(at1, at2);
			_arg1 = implicit_cast(_arg1, t);
			_arg2 = implicit_cast(_arg2, t);
		}
		else if ((av1 == T_POINTER || av1 == T_ARRAY) &&
				(av2 == T_POINTER || av2 == T_ARRAY))
		{
			Pointer_type *ap1 = (Pointer_type*)at1;
			Pointer_type *ap2 = (Pointer_type*)at2;

			if (ap1->type() == ap2->type() &&
					ap1->type()->typeval() != T_VOID)
				type(g_int_type);
			else if (base_type(ap1->type()) == base_type(ap2->type()) &&
					ap1->type()->typeval() != T_VOID)
			{
				warn(INCOMPATIBLE_POINTER_TYPES);
				type(g_int_type);
			}
			else
				error(INCOMPATIBLE_POINTER_TYPES);
		}
		else
			error(EXPECTED_SCALAR_EXPR);

		break;

	// int, float, ptr, zero
	case EQUAL: case NOTEQ:
		if ((av1 == T_INT || av1 == T_FLOAT)
				&& (av2 == T_INT || av2 == T_FLOAT))
		{
			type(g_int_type);
			Type *t = arith_conv(at1, at2);
			_arg1 = implicit_cast(_arg1, t);
			_arg2 = implicit_cast(_arg2, t);
		}
		else if ((av1 == T_POINTER || av1 == T_ARRAY) &&
				(av2 == T_POINTER || av2 == T_ARRAY))
		{
			Pointer_type *ap1 = (Pointer_type*)at1;
			Pointer_type *ap2 = (Pointer_type*)at2;

			if (ap1->type() == ap2->type()
					|| ap1->type()->typeval() == T_VOID
					|| ap2->type()->typeval() == T_VOID
					|| base_type(ap1->type()) == base_type(ap2->type())
					|| (g_cplusplus &&
							(valid_opcast(ap1->type(), ap2->type()) ||
							valid_opcast(ap2->type(), ap1->type()))))
			{
				if (av1 == T_ARRAY && av2 == T_ARRAY)
					warn(COMPARING_POINTER_CONSTANTS);

				type(g_int_type);
			}
			else
				error(INCOMPATIBLE_POINTER_TYPES);
		}
		else if (av1 == T_POINTER && (av2 == T_FUNCTION || av2 == T_OVERLOAD))
		{
			Pointer_type *ap1 = (Pointer_type*)at1;

			if (ap1->type() != at2)
				error(INCOMPATIBLE_POINTER_TYPES);
			else
				type(g_int_type);
		}
		else if ((av1 == T_FUNCTION || av1 == T_OVERLOAD) && av2 == T_POINTER)
		{
			Pointer_type *ap2 = (Pointer_type*)at2;

			if (at1 != ap2->type())
				error(INCOMPATIBLE_POINTER_TYPES);
			else
				type(g_int_type);
		}
		else if ((av1 == T_POINTER || av1 == T_ARRAY) && a2->is_int_expr())
		{
			if (!valid_ecast(_arg2, at1))
				error(EXPECTED_SCALAR_EXPR);
			else
			{
				if (av1 == T_ARRAY)
					warn(COMPARING_POINTER_CONSTANTS);

				type(g_int_type);
			}
		}
		else if (a1->is_int_expr() && (av2 == T_POINTER || av2 == T_ARRAY))
		{
			_arg1 = a2;				// swap args so 1st is always pointer/array
			_arg2 = a1;

			if (!valid_ecast(_arg2, at1))
				error(EXPECTED_SCALAR_EXPR);
			else
			{
				if (av2 == T_ARRAY)
					warn(COMPARING_POINTER_CONSTANTS);

				type(g_int_type);
			}
		}
		else
			error(EXPECTED_SCALAR_EXPR);

		break;

	// any non-void
	case '=':
		if (!a1->is_lvalue())
			error(CANNOT_ASSIGN_TO_NON_LVALUE);
		else if (at1 == at2 || (av1 != T_VOID && av2 != T_VOID &&
				valid_ecast(_arg2, at1)))
		{
			if (at1->isconst())
				warn(CANNOT_ASSIGN_TO_CONST_OBJECT);

			type(base_type(at1));
			_arg2 = implicit_cast(_arg2, _type);
		}
		else
			error(ILLEGAL_TYPES_IN_ASSIGN);

		break;

	// int, float
	case MULTEQ: case DIVEQ:
		if (at1->isconst())
			warn(CANNOT_ASSIGN_TO_CONST_OBJECT);

		if (!a1->is_lvalue())
			error(CANNOT_ASSIGN_TO_NON_LVALUE);
		else if ((av1 == T_INT || av1 == T_FLOAT)
				&& (av2 == T_INT || av2 == T_FLOAT))
		{
			type(base_type(at1));
			Type *t = arith_conv(at1, at2);
			_arg2 = implicit_cast(_arg2, t);
		}
		else
			error(EXPECTED_ARITH_EXPR);

		break;

	// int, float, ptr
	case PLUSEQ: case MINUSEQ:
		if (at1->isconst())
			warn(CANNOT_ASSIGN_TO_CONST_OBJECT);

		if (!a1->is_lvalue())
			error(CANNOT_ASSIGN_TO_NON_LVALUE);
		else if (av1 == T_POINTER && av2 == T_INT)
			type(base_type(at1));
		else if ((av1 == T_INT || av1 == T_FLOAT)
				&& (av2 == T_INT || av2 == T_FLOAT))
		{
			type(base_type(at1));
			Type *t = arith_conv(at1, at2);
			_arg2 = implicit_cast(_arg2, t);
		}
		else // (av1 == T_INT && av2 == T_POINTER) --> result is not int
			error(EXPECTED_SCALAR_EXPR);

		break;

	// int
	case MODEQ:
	case LSHIFTEQ: case RSHIFTEQ:
	case ANDEQ: case XOREQ: case OREQ:
		if (at1->isconst())
			warn(CANNOT_ASSIGN_TO_CONST_OBJECT);

		if (!a1->is_lvalue())
			error(CANNOT_ASSIGN_TO_NON_LVALUE);
		else if (av1 == T_INT && av2 == T_INT)
		{
			type(base_type(at1));
			Type *t = arith_conv(at1, at2);
			_arg2 = implicit_cast(_arg2, t);
		}
		else
			error(EXPECTED_INT_EXPR);

		break;

	// anything
	case ',':
		type(at2);
		break;

	default:
		bug("bogus operator2 token %d in Operator2_node constructor", tok);
		break;
	}
}

Operator3_node::Operator3_node(Scope *s, int tok, Expr_node *a1,
		Expr_node *a2, Expr_node *a3) : Expr_node(s)
{
	_token = tok;
	_arg1 = a1;
	_arg2 = a2;
	_arg3 = a3;
	Type *at1 = a1->type();
	Type *at2 = a2->type();
	Type *at3 = a3->type();

	if (at2->isarr())
	{
		Array_type *at = (Array_type*)at2;
		Type *t = new Pointer_type(at->type());
		t->isconst(TRUE);
		at2 = pool_type(t);
		t->free();
		_arg2 = implicit_cast(_arg2, at2);
	}

	if (at3->isarr())
	{
		Array_type *at = (Array_type*)at3;
		Type *t = new Pointer_type(at->type());
		t->isconst(TRUE);
		at3 = pool_type(t);
		t->free();
		_arg3 = implicit_cast(_arg3, at3);
	}

	Type_val av1 = at1->typeval();
	Type_val av2 = at2->typeval();
	Type_val av3 = at3->typeval();

	if (av1 == T_UNKNOWN || av2 == T_UNKNOWN || av2 == T_UNKNOWN)
		return;

	// zero, enums, and bitfields are considered ints for type-checks

	if (av1 == T_ENUM || av1 == T_BITFIELD)
		av1 = T_INT;

	if (av2 == T_ENUM || av2 == T_BITFIELD)
		av2 = T_INT;

	if (av3 == T_ENUM || av3 == T_BITFIELD)
		av3 = T_INT;

	if (av1 != T_INT && av1 != T_FLOAT && av1 != T_POINTER)
		error(EXPECTED_SCALAR_EXPR);

	if (at2 == at3)
		type(at2);
	else if ((av2 == T_INT && av3 == T_FLOAT) ||
			(av2 == T_FLOAT && av3 == T_INT))
	{
		type(arith_conv(at2, at3));
		_arg2 = implicit_cast(_arg2, _type);
		_arg3 = implicit_cast(_arg3, _type);
	}
	else if (valid_ecast(_arg2, at3))
	{
		type(at3);
		_arg2 = implicit_cast(_arg2, _type);
	}
	else if (valid_ecast(_arg3, at2))
	{
		type(at2);
		_arg3 = implicit_cast(_arg3, _type);
	}
	else
		error(MISMATCHED_EXPRS);
}

Sizeof_node::Sizeof_node(Scope *s, Type *t)
		: Expr_node(g_size_t_type, s), _size(g_size_t_type->ic())
{
	Type_val tv = t->typeval();

	if (tv == T_VOID)
	{
		error(ILLEGAL_TYPE_IN_SIZEOF);
		type(g_unknown_type);
	}
	else if (tv != T_UNKNOWN)
		// sizeof a type
		_size = t->getsize();
}

Sizeof_node::Sizeof_node(Scope *s, Expr_node *obj)
		: Expr_node(g_size_t_type, s), _size(g_size_t_type->ic())
{
	Type_val tv = obj->type() ? obj->type()->typeval() : T_UNKNOWN;

	if (tv == T_VOID)
	{
		error(ILLEGAL_TYPE_IN_SIZEOF);
		type(g_unknown_type);
	}
	else if (tv != T_UNKNOWN)
		// sizeof an expression (expr is never actually evaluated)
		_size = obj->type()->getsize();
}

Cast_node::Cast_node(Scope *s, Type *t, Expr_node *cast) :
		Expr_node(t, s)
{
	Type_val tv = t->typeval();
	Type_val cv = cast->type()->typeval();

	if (tv == T_UNKNOWN || cv == T_UNKNOWN)
		return;

	_cast = cast;

	if (tv == T_VOID && (cv == T_VOID || cv == T_STRUCT || cv == T_UNION))
		return;

	if ((tv == T_VOID || tv == T_POINTER || tv == T_ARRAY || tv == T_FLOAT ||
					tv == T_INT || tv == T_ENUM || tv == T_BITFIELD) &&
			(cv == T_POINTER || cv == T_ARRAY || cv == T_INT ||
					cv == T_ENUM || cv == T_BITFIELD || cv == T_FLOAT ||
					cv == T_FUNCTION || cv == T_OVERLOAD))
		return;

	//if (g_cplusplus && t->isclass() && cast->type()->isclass())
	if (g_cplusplus && valid_opcast(cast->type(), t))
		return;

	error(ILLEGAL_TYPE_FOR_CAST);
}

Initializer_node::Initializer_node(Scope *s) : Expr_node(s)
{
	_next = -1;
}

New_node::New_node(Scope *scp, Type *t, Expr_arr *args, Expr_node *arrlen) :
		Expr_node(scp)
{
	_args = args;
	_arrlen = arrlen;
	_ctor = NULL;

	Struct_type *st = NULL;

	if (t->isclass())
		st = (Struct_type*)t;
	else if (args)
		error(EXPECTED_CLASS_TYPE);

	if (t->isarr())
	{
		t = ((Array_type*)t)->type();
		t = new Pointer_type(t);
	}

	t = new Pointer_type(t);
	type(pool_type(t));
	t->free();

	if (st == NULL)
		return;

	Symbol *s = st->scope()->lookup_member(make_constructor_name(st->name()));

	if (s == NULL)
	{
		if (args && args->size() > 0)
			error(NO_CONSTRUCTORS_FOR_CLASS, st->name());

		return;
	}

	_ctor = s->overload_match(args);

	if (_ctor == NULL)
	{
		error(NO_MATCH_FOR_CONSTRUCTOR, st->name());
		return;
	}
}

Delete_node::Delete_node(Scope *s, Expr_node *e, Expr_node *ae, boolean a) :
		Expr_node(g_void_type, s)
{
	_expr = e;
	_arrexpr = ae;
	_array = a;
	_dtor = NULL;

	if (!e->type()->isptr())
	{
		error(EXPECTED_POINTER_TYPE);
		return;
	}

	Type *t = ((Pointer_type*)e->type())->type();

	if (t->isclass())
	{
		Struct_type *st = (Struct_type*)t;
		Scope *scope = st->scope();

		if (scope == NULL)
			scope = g_curr_scope;

		// setup for destruction here
		_dtor = st->scope()->lookup_member(make_destructor_name(st->name()));
	}
}

Return_node::Return_node(Scope *s, Expr_node *expr) : Node(s)
{
	_expr = expr;

	if (g_func_scope == NULL)
		return;

	_func = g_func_scope->symbol();

	if (_func == NULL || _func->type() == NULL)
		return;

	if (_func->type()->typeval() != T_FUNCTION)
		bug("Return_node::Return_node(): g_func_scope is not a function");

	Type *ret = ((Function_type*)_func->type())->retval();

	if (ret == g_unknown_type)
		return;

	if ((ret == NULL || ret->typeval() == T_VOID) && expr != NULL)
		error(RETURN_EXPR_FOR_VOID_FUNC, _func->name());
	else if (expr == NULL)
	{
		// missing expression for function which returns a value
		if (ret == g_int_type)
			warn(RETURN_FOR_INT_FUNC, _func->name());
		else if (!ret->isvoid())
			error(RETURN_FOR_NON_VOID_FUNC, _func->name());
	}
	else if (valid_ecast(_expr, ret))
		_expr = implicit_cast(_expr, ret);
	else
		error(BAD_TYPE_FOR_RETURN_EXPR, _func->name());
}
