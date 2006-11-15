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

// Copyright (c) 1992 by Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/bin/cc/ccinit.cc,v 1.21 1999/09/26 18:51:20 parag Exp $

#include <stddef.h>
#include <memory.h>
#include "cctype.h"
#include "ccsym.h"
#include "ccnode.h"
#include "globals.h"

boolean
Expr_node::check_init_type(Type *t, Integer &)
{
	if (valid_cast(_type, t))
		return TRUE;

	error(TYPE_MISMATCH_IN_INITIALIZER);
	return FALSE;
}

boolean
Integer_node::check_init_type(Type *t, Integer &)
{
	// for MPW/Think C style inline functions
	if (valid_cast(_type, t) ||
			(g_macintosh && !g_strict_iso && t->isfunc()) ||
			(t->typeval() == T_POINTER && _val != NULL && _val->isZero()))
		return TRUE;

	error(TYPE_MISMATCH_IN_INITIALIZER);
	return FALSE;
}

boolean
String_node::check_init_type(Type *t, Integer &newlen)
{
	if (t == NULL || _type == NULL)
		return FALSE;

	Array_type *fa = (Array_type*)_type;
	newlen = fa->len();

	switch (t->typeval())
	{
	case T_POINTER:
		{
			Pointer_type *tp = (Pointer_type*)t;

			if (tp->type()->typeval() != T_VOID &&
					valid_cast(fa->type(), tp->type(), TRUE))
				return TRUE;
		}

		break;

	// const-ness does not apply for array initialization, so we cannot
	// call valid_cast()
	case T_ARRAY:
		{
			Array_type *ta = (Array_type*)t;

			if (fa->type() == ta->type() || (ta->type()->typeval() == T_INT &&
					((Int_type*)ta->type())->width() >=
							((Int_type*)fa->type())->width() ))
			{
				// both char buf[2] = "OK"; char buf[3] = "OK"; are both legal
				--newlen;

				if (ta->len() != newlen)		// array len == string len - 1
					newlen = fa->len();

				return TRUE;
			}
		}

		break;

	default:
		break;
	}

	error(TYPE_MISMATCH_IN_INITIALIZER);
	return FALSE;
}

boolean
Operator_node::check_init_type(Type *t, Integer &)
{
	if ((_token != '&' || t->typeval() != T_ARRAY) && valid_cast(_type, t))
		return TRUE;

	error(ILLEGAL_EXPR_IN_INITIALIZER);
	return FALSE;
}

boolean
Operator2_node::check_init_type(Type *t, Integer &)
{
	if (_arg1 == NULL || _arg2 == NULL || t == NULL)
		return FALSE;

	switch (_token)
	{
	case '+':
	case '-':
		{
			Type_val tv = _arg1->type()->typeval();

			if (t->typeval() == T_ARRAY && (tv == T_POINTER || tv == T_ARRAY))
				break;
		}
		
		// fall through

	default:
		if (valid_cast(_type, t))
			return TRUE;

		break;
	}

	error(ILLEGAL_EXPR_IN_INITIALIZER);
	return FALSE;
}

static boolean check_struct_type(Struct_type *st, Expr_arr &init, int &loc);
static boolean check_union_type(Union_type *ut, Expr_arr &init, int &loc);
static boolean check_array_type(Array_type *arr, Expr_arr &init, int &loc,
		Integer &newlen);

static boolean
do_check(Expr_node *expr, Type *t, Expr_arr &init, int &loc)
{
	if (expr == NULL || t == NULL)
		return FALSE;

	if (expr->nodetype() == N_INITIALIZER)
	{
		Integer newlen(g_size_t_type->ic());

		if (!check_init_type(expr, t, newlen))
			return FALSE;

		loc++;
	}
	else
		switch (t->typeval())
		{
		case T_ARRAY:
			{
				Integer newlen(g_size_t_type->ic());

				if (!check_array_type((Array_type*)t, init, loc, newlen))
					return FALSE;
			}

			break;

		case T_STRUCT:
			if (!check_struct_type((Struct_type*)t, init, loc))
				return FALSE;

			break;

		case T_UNION:
			if (!check_union_type((Union_type*)t, init, loc))
				return FALSE;

			break;

		default:
			{
				Integer newlen(g_size_t_type->ic());

				if (!check_init_type(expr, t, newlen))
					return FALSE;

				loc++;
			}

			break;
		}

	return TRUE;
}

static boolean
check_struct_type(Struct_type *st, Expr_arr &init, int &loc)
{
	if (st == NULL)
		return FALSE;

	Symbol_arr *arr = st->memlist();

	if (!st->complete() || arr == NULL)
	{
		error(INITIALIZING_INCOMPLETE_STRUCT, st->name());
		return FALSE;
	}

	for (int n = 0; n < arr->size() && loc < init.size(); n++)
	{
		// unnamed bitfields may not be initialized, but unnamed unions
		// and unnamed substructs are OK
		if (arr->elt(n)->name()[0] == UNNAMED_CHAR &&
				arr->elt(n)->type()->isbit())
			continue;

		if (init.elt(loc) == NULL ||
				!do_check(init.elt(loc), arr->elt(n)->type(), init, loc))
			return FALSE;
	}

	return TRUE;
}

static boolean
check_union_type(Union_type *ut, Expr_arr &init, int &loc)
{
	if (ut == NULL)
		return FALSE;

	Symbol_arr *arr = ut->memlist();

	if (!ut->complete() || arr == NULL)
	{
		error(INITIALIZING_INCOMPLETE_UNION, ut->name());
		return FALSE;
	}

	// we are only allowed to initialize the 1st union member
	if (init.elt(loc) == NULL ||
			!do_check(init.elt(loc), arr->elt(loc)->type(), init, loc))
		return FALSE;

	return TRUE;
}

static boolean
check_array_type(Array_type *arr, Expr_arr &init, int &loc, Integer &newlen)
{
	if (arr == NULL)
		return FALSE;

	Integer n(g_size_t_type->ic());

	for (; (arr->len().isZero() || n < arr->len()) &&
			loc < init.size(); n.add(n.integerclass().one()))
	{
		if (init.elt(loc) == NULL ||
				!do_check(init.elt(loc), arr->type(), init, loc))
			return FALSE;
	}

	newlen = n;
	return TRUE;
}

boolean
Initializer_node::check_init_type(Type *t, Integer &newlen)
{
	if (_init.size() == 0 || t == NULL)
		return FALSE;

	// using initialisation index syntax implies this better be an array
	if (_next >= 0 && !t->isarr())
	{
		error(TYPE_MISMATCH_IN_INITIALIZER);
		return FALSE;
	}

	int loc = 0;

	switch (t->typeval())
	{
	case T_STRUCT:
		{
			Struct_type *st = (Struct_type*)t;

			if (!check_struct_type(st, _init, loc))
				return FALSE;

			if (loc < _init.size())
			{
				error(TOO_MANY_INITIALIZERS_FOR_STRUCT, st->name());
				return FALSE;
			}
		}

		break;

	case T_UNION:
		{
			Union_type *ut = (Union_type*)t;

			if (!check_union_type(ut, _init, loc))
				return FALSE;

			if (loc < _init.size())
			{
				error(TOO_MANY_INITIALIZERS_FOR_UNION, ut->name());
				return FALSE;
			}
		}

		break;

	case T_ARRAY:
		{
			Array_type *at = (Array_type*)t;

			if (_next >= 0)
				for (int i = 0; i < _init.size(); i++)
					if (_init.elt(i) == NULL)
					{
						int j;
						
						for (j = i + 1; j < _init.size(); j++)
							if (_init.elt(j) != NULL)
								break;

						if (i == --j)
							warn(MISSING_INIT_ENTRY, i);
						else
							warn(MISSING_INIT_ENTRIES, i, j);
						
						i = j;
					}

			// strings can be either char* and char[], causing much trouble
			if (_init.size() == 1 && _init.elt(0)->nodetype() == N_STRING)
				return ::check_init_type(_init.elt(0), at, newlen);

			if (!check_array_type(at, _init, loc, newlen))
				return FALSE;

			if (!at->len().isZero() && loc < _init.size())
			{
				error(TOO_MANY_INITIALIZERS_FOR_ARRAY);
				return FALSE;
			}
		}

		break;

	case T_FUNCTION:
		// for MPW/Think C style inline functions
		if (g_macintosh && !g_strict_iso)
			for (; loc < _init.size(); loc++)
				if (_init.elt(loc)->nodetype() != N_INTEGER)
					break;

		if (loc < _init.size())
		{
			error(TYPE_MISMATCH_IN_INITIALIZER);
			return FALSE;
		}

		break;

	default: ;
		if (_init.size() != 1)
		{
			error(TOO_MANY_INITIALIZERS);
			return FALSE;
		}

		if (!::check_init_type(_init.elt(0), t, newlen))
			return FALSE;

		break;
	}

	type(t);
	return TRUE;
}

boolean
Symbol_node::check_init_type(Type *t, Integer &)
{
	if (valid_cast(_type, t))
		return TRUE;

	if (t->isptr())
		t = ((Pointer_type*)t)->type();

	if (_sym && _sym->overloaded() && _sym->exact_match(t) != NULL)
		return TRUE;

	error(TYPE_MISMATCH_IN_INITIALIZER);
	return FALSE;
}

boolean
check_init_type(Expr_node *&expr, Type *t, Integer &newlen)
{
	if (!expr->check_init_type(t, newlen))
		return FALSE;

	if (t->issu() || t->isfunc())
		return TRUE;

	expr = implicit_cast(expr, t);
	return TRUE;
}


boolean Expr_node::is_static_init(int*) { return FALSE; }
boolean Integer_node::is_static_init(int*) { return TRUE; }
boolean Sizeof_node::is_static_init(int*) { return TRUE; }
boolean Float_node::is_static_init(int*) { return TRUE; }
boolean String_node::is_static_init(int*) { return TRUE; }

boolean
Array_ref_node::is_static_init(int *ref)
{
	return _arr != NULL && _arr->is_static_init(ref) && 
			_expr != NULL && _expr->is_static_init(ref);
}

boolean
Operator_node::is_static_init(int *ref)
{
	if (_arg == NULL)
		return FALSE;

	switch (_token)
	{
	case '&':
		if (ref != NULL)
			*ref += 1;

		return _arg->is_static_init(ref);

	case '*':
		return (ref == NULL || *ref == 1) && _arg->is_static_init(ref);

	case INCR: case DECR:
		return FALSE;

	default:
		break;
	}

	return (ref == NULL || *ref == 0) && _arg->is_static_init(ref);
}

boolean
Operator2_node::is_static_init(int *ref)
{
	if (_arg1 == NULL || _arg2 == NULL)
		return FALSE;

	switch (_token)
	{
	case '+':
	case '-':
		{
			Type_val tv = _arg1->type()->typeval();

			if (tv == T_POINTER || tv == T_ARRAY)
			{
				Type_val tv2 = _arg2->type()->typeval();

				if (tv2 == T_POINTER || tv2 == T_ARRAY)
					return FALSE;

				return _arg1->is_static_init(ref) && _arg2->is_static_init(ref);
			}
		}

		break;

	case ',':
		return _arg1->is_static_init(ref) && _arg2->is_static_init(ref);

	case '=':
	case MULTEQ: case DIVEQ: case MODEQ:
	case PLUSEQ: case MINUSEQ:
	case LSHIFTEQ: case RSHIFTEQ:
	case ANDEQ: case XOREQ: case OREQ:
		return FALSE;

	default:
		break;
	}

	return (ref == NULL || *ref == 0) &&
			_arg1->is_static_init(ref) && _arg2->is_static_init(ref);
}

boolean
Operator3_node::is_static_init(int *ref)
{
	return _arg1 && _arg1->is_int_expr() &&
			_arg2 && _arg2->is_static_init(ref) &&
			_arg2 && _arg3->is_static_init(ref);
}

boolean Cast_node::is_static_init(boolean *ref)
		{ return _cast && _cast->is_static_init(ref); }

boolean
Symbol_node::is_static_init(int *ref)
{
	// global symbols must, by definition, have values determinable
	// at compile time, so if this is a global symbol, then it may be
	// used within another global's initialization, but only as a reference

	if (_sym == NULL)
		return FALSE;

	if (_sym->storage() == ENUM || _type->isfunc())
		return TRUE;

	if (_sym->scope() == g_file_scope || _sym->storage() == STATIC)
		return ref != NULL && (*ref == 1 || (*ref == 0 &&
					(_type->isarr() || _type->issu())));

	return FALSE;
}

boolean
Struct_ref_node::is_static_init(int *ref)
{
	return _structref && _elem && _structref->is_static_init(ref) &&
			_elem->type()->typeval() != T_BITFIELD &&
			(ref == NULL || *ref == 1);
}

boolean
Initializer_node::is_static_init(int*)
{
	for (int i = 0; i < _init.size(); i++)
	{
		int rc = 0;

		if (_init.elt(i) != NULL &&
				(!_init.elt(i)->is_static_init(&rc) || rc > 1))
			return FALSE;
	}

	return TRUE;
}

boolean Expr_node::is_lvalue() { return FALSE; }

boolean
Symbol_node::is_lvalue()
{
	Type_val tv = T_UNKNOWN;

	return _sym && _sym->storage() != ENUM &&
			(tv = _sym->type()->typeval()) != T_FUNCTION && tv != T_ARRAY;
}

boolean
Func_call_node::is_lvalue()
{
	return _func ? TRUE : FALSE;
}

boolean
Array_ref_node::is_lvalue()
{
	if (_arr == NULL)
		return FALSE;

	Type_val tv = _arr->type()->typeval();

	if (tv == T_ARRAY || tv == T_POINTER)
		return TRUE;

	return FALSE;
}

boolean
Struct_ref_node::is_lvalue()
{
	return _structref &&
			(_structref->type()->isptr() || _structref->is_lvalue());
}

boolean
Operator_node::is_lvalue()
{
	if (_arg == NULL)
		return FALSE;

	switch (_token)
	{
	case '*':
		{
			Type_val tv = _arg->type()->typeval();

			if (tv == T_ARRAY || tv == T_POINTER)
				return TRUE;
		}

		break;

	default:
		break;
	}

	return FALSE;
}
