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
// $Header: /u/cgt/cvs/bin/cc/cceval.cc,v 1.19 1999/04/25 04:02:32 parag Exp $

#include <stddef.h>
#include "cctype.h"
#include "ccsym.h"
#include "ccnode.h"
#include "globals.h"

boolean Expr_node::is_int_expr() { return FALSE; }
boolean Integer_node::is_int_expr() { return TRUE; }
boolean Sizeof_node::is_int_expr() { return TRUE; }
boolean String_node::is_int_expr() { return TRUE; }

boolean
Symbol_node::is_int_expr()
{
	return _sym->storage() == ENUM;
}

boolean
Operator_node::is_int_expr()
{
	switch (_token)
	{
	case INCR: case DECR:
	case '&': case '*':
		return FALSE;
	
	default:
		break;
	}

	return _arg->is_int_expr();
}

boolean Operator2_node::is_int_expr()
{
	switch (_token)
	{
	case '=':
	case MULTEQ: case DIVEQ: case MODEQ:
	case PLUSEQ: case MINUSEQ:
	case LSHIFTEQ: case RSHIFTEQ:
	case ANDEQ: case XOREQ: case OREQ:
		return FALSE;

	default:
		break;
	}

	return _arg1->is_int_expr() && _arg2->is_int_expr();
}

boolean Operator3_node::is_int_expr()
		{ return _arg1->is_int_expr()
					&& _arg2->is_int_expr() && _arg3->is_int_expr(); }

boolean
Cast_node::is_int_expr() 
{
	return _cast->is_int_expr() || _cast->nodetype() == N_FLOAT;
}

void
Expr_node::eval_const_expr(Const_expr &)
{
	bug("Expr_node::eval_const_expr() - should never be called for node %d",
			nodetype());
}

void
Integer_node::eval_const_expr(Const_expr &ret)
{
	ret.set_ival(*_val);
}

void
Sizeof_node::eval_const_expr(Const_expr &ret)
{
	ret.set_ival(_size);
}

// assume this is only called for operator '&' if not an enum
void
Symbol_node::eval_const_expr(Const_expr &ret)
{
	if (_sym != NULL && _sym->storage() == ENUM)
	{
		ret.set_ival(*_sym->member()->enumval());
	}
	else
	{
		ret.sym = _sym;

		// offset should be allocated and zero
		if (ret.ival == NULL)
			ret.new_ival(g_size_t_type->ic());
	}
}

void
Float_node::eval_const_expr(Const_expr &ret)
{
	if (ret.fval == NULL)
		ret.fval = new Float(*_val);
	else
		*ret.fval = *_val;

	ret.isint = FALSE;
}

// assume this is only called for operator '&'
void
Array_ref_node::eval_const_expr(Const_expr &ret)
{
	_arr->eval_const_expr(ret);				// should set "sym"
	_expr->eval_const_expr(ret);		// should set "ival"
	ret.ival->mult(_type->getsize());
}

// assume this is only called for operator '&'
void
Struct_ref_node::eval_const_expr(Const_expr &ret)
{
	_structref->eval_const_expr(ret);				// should set "sym"
	_structref->type()->getsize();				// to calculate size of struct

	if (_elem->offset() != NULL)
	{
		ret.set_ival(*_elem->offset());				// for offset
		ret.ival->div(*g_min_addr_size);		// make it in "bytes"
	}
}

void
Cast_node::eval_const_expr(Const_expr &ret)
{
	if (_cast == NULL)
		return;

	Const_expr ce;
	_cast->eval_const_expr(ce);

	// convert the expression to the cast return type
	if (_type->isfloat())
	{
		ret.fval = new Float(((Float_type*)_type)->fc());
		ret.isint = FALSE;
		extern Float integertofloat(Integer &i);

		if (ce.isint)
			*ret.fval = integertofloat(*ce.ival);
		else
			*ret.fval = *ce.fval;
	}
	else if (_type->isint() || _type->isbit())
	{
		ret.new_ival(((Int_type*)_type)->ic());
		ret.isint = TRUE;
		extern Integer floattointeger(Float &f);

		if (ce.isint)
			*ret.ival = *ce.ival;
		else
			*ret.ival = floattointeger(*ce.fval);
	}
	else // ptr, arr, function
	{
		ret = ce;
		ce.ival = NULL;
	}
}

void
Operator_node::eval_const_expr(Const_expr &ret)
{
	if (_arg == NULL)
		return;

	_arg->eval_const_expr(ret);

	switch (_token)
	{
	case '&':		// _arg eval should have set "sym+-offset" properly
	case '*':		// should have '&' around it somewhere anyway
	case '+':
		break;

	case '-':
		if (ret.isint)
			ret.ival->neg();
		else
			ret.fval->neg();

		break;

	case '~':
		ret.ival->complement();
		break;

	case '!':
		{
			Integer *i = new Integer(g_int_type->ic());

			if ((ret.isint && ret.ival->isZero()) ||
					!ret.isint && ret.fval->isZero())
				*i = i->integerclass().one();
			else
				*i = i->integerclass().zero();

			ret.free();
			ret.ival = i;
			ret.isint = TRUE;
		}

		break;

	default:
		bug("Operator_node::eval_const_expr(): unexpected op %d", _token);
		break;
	}
}

void
Operator2_node::eval_const_expr(Const_expr &val1)
{
	if (_arg1 == NULL || _arg2 == NULL)
		return;

	// this automatically handles implicit casts and conversions
	Const_expr val2;
	_arg1->eval_const_expr(val1);
	_arg2->eval_const_expr(val2);

	if (_type->isfloat() && (val1.isint || val2.isint))
		bug("Operator2_node::eval_const_expr(): arg exprs should be float");
	else if (!val1.isint || !val2.isint)
		bug("Operator2_node::eval_const_expr(): arg exprs should be int");

	boolean sign1 = (_arg1->type()->typeval() != T_INT &&
				_arg1->type()->typeval() != T_BITFIELD) ||
				((Int_type*)_arg1->type())->sign();
	boolean sign2 = (_arg2->type()->typeval() != T_INT &&
				_arg2->type()->typeval() != T_BITFIELD) ||
				((Int_type*)_arg2->type())->sign();
	boolean f = _type->isfloat();

	// these operators all modify and return the value in val1
	switch (_token)
	{
	case '*':
		if (f)
			val1.fval->mult(*val2.fval);
		else if (sign1 && sign2)
			val1.ival->mult(*val2.ival);
		else
			val1.ival->umult(*val2.ival);

		return;

	case '/':
		if (f)
			val1.fval->div(*val2.fval);
		else if (sign1 && sign2)
			val1.ival->div(*val2.ival);
		else
			val1.ival->udiv(*val2.ival);

		return;

	case '%':
		if (sign1 && sign2)
			val1.ival->rem(*val2.ival);
		else
			val1.ival->urem(*val2.ival);

		return;

	case '+':
		if (f)
			val1.fval->add(*val2.fval);
		else if (_arg1->type()->isptr())
		{
			val2.ival->mult(((Pointer_type*)_arg1->type())->type()->getsize());
			val1.ival->add(*val2.ival);
		}
		else if (_arg1->type()->isarr())
		{
			val2.ival->mult(((Array_type*)_arg1->type())->type()->getsize());
			val1.ival->add(*val2.ival);
		}
		else
			val1.ival->add(*val2.ival);

		return;

	case '-':
		if (f)
			val1.fval->sub(*val2.fval);
		else if (_arg1->type()->isptr())
		{
			val2.ival->mult(((Pointer_type*)_arg1->type())->type()->getsize());
			val1.ival->sub(*val2.ival);
		}
		else if (_arg1->type()->isarr())
		{
			val2.ival->mult(((Array_type*)_arg1->type())->type()->getsize());
			val1.ival->sub(*val2.ival);
		}
		else
			val1.ival->sub(*val2.ival);

		return;

	case LSHIFT:
		val1.ival->lshift((int)val2.ival->get());
		return;

	case RSHIFT:
		if (sign1)
			val1.ival->rshift((int)val2.ival->get());
		else
			val1.ival->urshift((int)val2.ival->get());

		return;

	case '&':
		val1.ival->b_and(*val2.ival);
		return;

	case '|':
		val1.ival->b_or(*val2.ival);
		return;

	case '^':
		val1.ival->b_xor(*val2.ival);
		return;

	default:
		break;
	}

	// now handle all operators which return "int" boolean expressions
	Integer *i = new Integer(g_int_type->ic());

	switch (_token)
	{
	case '<':
		if (f)
		{
			if (*val1.fval < *val2.fval)
				*i = i->integerclass().one();
			else
				*i = i->integerclass().zero();
		}
		else if (sign1 && sign2)
		{
			if (*val1.ival < *val2.ival)
				*i = i->integerclass().one();
			else
				*i = i->integerclass().zero();
		}
		else
		{
			if (val1.ival->ucompare(*val2.ival) < 0)
				*i = i->integerclass().one();
			else
				*i = i->integerclass().zero();
		}

		break;

	case '>':
		if (f)
		{
			if (*val1.fval > *val2.fval)
				*i = i->integerclass().one();
			else
				*i = i->integerclass().zero();
		}
		else if (sign1 && sign2)
		{
			if (val1.ival > val2.ival)
				*i = i->integerclass().one();
			else
				*i = i->integerclass().zero();
		}
		else
		{
			if (val1.ival->ucompare(*val2.ival) > 0)
				*i = i->integerclass().one();
			else
				*i = i->integerclass().zero();
		}

		break;

	case LESSEQ:
		if (f)
		{
			if (*val1.fval <= *val2.fval)
				*i = i->integerclass().one();
			else
				*i = i->integerclass().zero();
		}
		else if (sign1 && sign2)
		{
			if (val1.ival <= val2.ival)
				*i = i->integerclass().one();
			else
				*i = i->integerclass().zero();
		}
		else
		{
			if (val1.ival->ucompare(*val2.ival) <= 0)
				*i = i->integerclass().one();
			else
				*i = i->integerclass().zero();
		}

		break;

	case GREATEQ:
		if (f)
		{
			if (*val1.fval >= *val2.fval)
				*i = i->integerclass().one();
			else
				*i = i->integerclass().zero();
		}
		else if (sign1 && sign2)
		{
			if (val1.ival >= val2.ival)
				*i = i->integerclass().one();
			else
				*i = i->integerclass().zero();
		}
		else
		{
			if (val1.ival->ucompare(*val2.ival) >= 0)
				*i = i->integerclass().one();
			else
				*i = i->integerclass().zero();
		}

		break;

	case EQUAL:
		if (f)
		{
			if (*val1.fval == *val2.fval)
				*i = i->integerclass().one();
			else
				*i = i->integerclass().zero();
		}
		else if (val1.ival == val2.ival)
			*i = i->integerclass().one();
		else
			*i = i->integerclass().zero();

		break;

	case NOTEQ:
		if (f)
		{
			if (*val1.fval != *val2.fval)
				*i = i->integerclass().one();
			else
				*i = i->integerclass().zero();
		}
		else if (val1.ival != val2.ival)
			*i = i->integerclass().one();
		else
			*i = i->integerclass().zero();

		break;

	case ANDAND:
		*i = i->integerclass().zero();

		if ((val1.isint && val1.ival->isZero()) ||
				(val2.isint && val2.ival->isZero()))
			break;

		if ((!val1.isint && val1.fval->isZero()) ||
				(!val2.isint && val2.fval->isZero()))
			break;

		*i = i->integerclass().one();
		break;

	case OROR:
		*i = i->integerclass().one();

		if ((val1.isint && !val1.ival->isZero()) ||
				(val2.isint && !val2.ival->isZero()))
			break;

		if ((!val1.isint && !val1.fval->isZero()) ||
				(!val2.isint && !val2.fval->isZero()))
			break;

		*i = i->integerclass().zero();
		break;

	default:
		bug("Operator2_node::eval_const_expr(): unexpected op %d", _token);
		break;
	}

	val1.free();
	val1.ival = i;
	val1.isint = TRUE;
}

void
Operator3_node::eval_const_expr(Const_expr &ret)
{
	if (_token != '?')
		bug("Operator3::eval_const_expr(): unexpected operator");

	if (_arg1 == NULL || _arg2 == NULL || _arg3 == NULL)
		return;

	Const_expr arg;
	_arg1->eval_const_expr(arg);

	Expr_node *e = _arg1;

	if ((arg.isint && arg.ival->isZero()) ||
			(!arg.isint && arg.fval->isZero()))
		e = _arg2;

	e->eval_const_expr(ret);
}
