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

// Copyright (c) 1992,1999 by Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/bin/cc/ccnode3.cc,v 1.7 1999/09/26 18:51:21 parag Exp $

#include "cpp.h"
#include "cc.h"
#include "cctype.h"
#include "ccsym.h"
#include "ccnode.h"
#include "globals.h"
#include <pool.h>
#include <stdiox.h>

const char *
make_unnamed_name()
{
	static long count = 1;
	static char buf[25];
	sprintf(buf, "%cunnamed_%ld", UNNAMED_CHAR, count++);
	return strget(buf);
}

const char *
make_destructor_name(const char *classname)
{
	//return strget("~destructor");
	return strget(xsprintf("~%s", classname));
}

const char *
make_constructor_name(const char *classname)
{
	//return strget("^constructor");
	return strget(xsprintf("^%s", classname));
}

const char *
make_operator_name(int op)
{
	return strget(xsprintf("operator%s", op2str(op)));
}

const char *
make_memfunc_name(const char *classname, const char *memfunc)
{
	return strget(xsprintf("%s::%s", classname, memfunc));
}

static const char *
type2str(Type *type)
{
	boolean sign;
	const char *s, *ret;

	switch (type->typeval())
	{
	case T_UNKNOWN:
		ret = "<unknown-type>";
		break;

	case T_VOID:
		ret = "void";
		break;

	case T_INT:
		sign = ((Int_type*)type)->sign();

		if (type == g_char_type)
			ret = sign ? "char" : "unsigned char";
		else if (type == g_short_type)
			ret = sign ? "short" : "unsigned short";
		else if (type == g_int_type)
			ret = sign ? "int" : "unsigned int";
		else if (type == g_long_type)
			ret = sign ? "long" : "unsigned long";
		else if (type == g_longlong_type)
			ret = sign ? "long long" : "unsigned long long";
		else
			ret = sign ? "<unknown-int>" : "unsigned <unknown-int>";

		break;

	case T_FLOAT:
		if (type == g_float_type)
			ret = "float";
		else if (type == g_double_type)
			ret = "double";
		else if (type == g_shortdouble_type)
			ret = "short double";
		else if (type == g_longdouble_type)
			ret = "long double";
		else
			ret = "<unknown-float>";

		break;

	case T_ARRAY:
		s = type2str(((Array_type*)type)->type());
		ret = strget(xsprintf("%s[]", s));
		break;

	case T_POINTER:
		s = type2str(((Pointer_type*)type)->type());
		ret = strget(xsprintf("%s*", s));
		break;

	case T_STRUCT:
		s = ((Struct_type*)type)->name();

		if (((Struct_type*)type)->isclass())
			ret = strget(xsprintf("class %s", s));

		ret = strget(xsprintf("struct %s", s));
		break;

	case T_UNION:
		s = ((Union_type*)type)->name();
		ret = strget(xsprintf("union %s", s));
		break;

	default:
		error(ILLEGAL_TYPE_IN_CAST_OPERATOR);
		return "<illegal>";
	}

	if (type->isconst())
		return strget(xsprintf("const %s", ret));
	else if (type->isvolatile())
		return strget(xsprintf("volatile %s", ret));

	return ret;
}

const char *
make_opcast_name(Type *type)
{
	const char *s = type2str(type);
	return strget(xsprintf("operator %s", s));
}

const char *
op2str(int tok)
{
	switch (tok)
	{
	case ARROW:
		return "->";
	
	case INCR:
		return "++";
	
	case DECR:
		return "--";

	case '!':
		return "!";

	case X_TILDE:
	case '~':
		return "~";
	
	case '(':
	case ')':
		return "()";
	
	case '[':
	case ']':
		return "[]";

	case NEW:
		return " new";

	case DELETE:
		return " delete";
	}

	Cpp_node cpptok;
	cpptok.token = tok;
	return cpptok.tok2str();
}

static Expr_node *
find_operator(Expr_node *obj, int op, Expr_node *arg, Expr_arr *args = NULL);

static Expr_node *
find_operator(Expr_node *obj, int op, Expr_node *arg, Expr_arr *args)
{
	Type *t = obj->type();

	if (!t->isclass())
		return NULL;

	Struct_type *st = (Struct_type*)t;
	Scope *scope = st->scope();

	if (scope == NULL)
		scope = g_curr_scope;

	Symbol *s = st->scope()->lookup_member(make_operator_name(op));

	if (s == NULL)
		return NULL;

	Expr_arr *alist;

	if (args)
		alist = new Expr_arr(*args);
	else
		alist = new Expr_arr;

	if (arg)
		alist->end() = arg;

	s = s->overload_match(alist);

	if (s == NULL)
	{
		delete alist;
		return NULL;
	}

	return new Func_call_node(scope,
			new Struct_ref_node(scope, obj, s->name(),
					g_cpp.filename(), g_cpp.linenum(), FALSE),
			alist);
}

Expr_node *
make_array_ref_node(Scope *s, Expr_node *arr, Expr_node *expr)
{
	if (arr && arr->type() && arr->type()->isclass())
	{
		Expr_node *ret = find_operator(arr, '['/*]*/, expr);

		if (ret != NULL)
			return ret;
	}

	return new Array_ref_node(s, arr, expr);
}

Expr_node *
make_func_call_node(Scope *s, Expr_node *f, Expr_arr *a)
{
	if (f && f->type() && f->type()->isclass())
	{
		Expr_node *ret = find_operator(f, '('/*)*/, NULL, a);

		if (ret != NULL)
			return ret;
	}

	return new Func_call_node(s, f, a);
}

Expr_node *
make_struct_ref_node(Scope *s, Expr_node *sref, const char *tag,
		const char *file, int line, boolean pointer)
{
	if (sref && sref->type() && sref->type()->isclass())
	{
		Expr_node *ret = find_operator(sref, pointer ? ARROW : '.', sref);

		if (ret != NULL)
			sref = ret;
	}

	return new Struct_ref_node(s, sref, tag, file, line, pointer);
}

Expr_node *
make_operator_node(Scope *s, int tok, Expr_node *a, boolean p)
{
	if (a->type() && a->type()->isclass())
	{
		Expr_node *prefix = p ? (Expr_node*)NULL : new Integer_node(g_int_type, s, "0");
		Expr_node *ret = find_operator(a, tok, prefix);

		if (ret != NULL)
			return ret;
		
		delete prefix;
	}

	return new Operator_node(s, tok, a, p);
}

Expr_node *
make_operator2_node(Scope *s, int tok, Expr_node *a1, Expr_node *a2)
{
	if (a1 && a1->type() && a1->type()->isclass())
	{
		Expr_node *ret = find_operator(a1, tok, a2);

		if (ret != NULL)
			return ret;
	}

	return new Operator2_node(s, tok, a1, a2);
}

Expr_node *
do_implicit_cast(Expr_node *e, Type *type)
{
	if (e == NULL)
		return g_null_expr;

	if (e->type() == NULL ||
			e->type() == type || e->type() == g_unknown_type ||
			type == NULL || type == g_unknown_type ||
			base_type(e->type()) == base_type(type))
		return e;

	return new Cast_node(g_curr_scope, type, e);
}

// see if expression "efrom" may be cast into "to" - this is to handle
// conversions of zero integer constants into pointer types
boolean
valid_ecast(Expr_node *&efrom, Type *to)
{
	Type *from = efrom->type();

	if (from == NULL || to == NULL)
		return FALSE;

	if (from == to || valid_cast(from, to))
		return TRUE;

	Type_val f = from->typeval();
	Type_val t = to->typeval();

	if (f == T_OVERLOAD)
	{
		if (efrom->nodetype() != N_SYMBOL)
			return FALSE;

		Symbol *sym = ((Symbol_node*)efrom)->sym();

		return (sym->overloaded() && sym->exact_match(to) != NULL) ?
				TRUE : FALSE;
	}

	if ((f != T_INT && f != T_BITFIELD && f != T_ENUM) ||
			(t != T_POINTER && t != T_ARRAY))
		return FALSE;

	if (!efrom->is_int_expr())
		return FALSE;

	Const_expr ce;
	efrom->eval_const_expr(ce);

	if (!ce.isint || !ce.ival->isZero())
		return FALSE;

	if (efrom->nodetype() == N_INTEGER)
		return TRUE;

	Expr_node *e = new Integer_node(from, efrom->scope(), ce.ival);
	ce.ival = NULL;   // do not delete it when we destruct "ce"
	delete efrom;
	efrom = e;
	return TRUE;
}

boolean
valid_opcast(Type *from, Type *to)
{
	if (!g_cplusplus || to == NULL || from == NULL || !from->isclass())
		return FALSE;

	Struct_type *st = (Struct_type*)from;

	// look for a matching base class
	for (Struct_type *s = st; s != NULL; s = s->base())
		if (s == to || valid_cast(s, to))
			return TRUE;

	// look for a cast operator function
	if (st->scope() != NULL &&
			st->scope()->lookup_member(make_opcast_name(to)) != NULL)
		return TRUE;

	return FALSE;
}

void
init_nodes()
{
	g_null_expr = new Expr_node;
	g_null_node = g_null_expr;
}
