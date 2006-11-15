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
// $Header: /u/cgt/cvs/bin/cc/ccnode2.cc,v 1.63 2003/01/30 02:27:56 parag Exp $

#include <stddef.h>
#include <memory.h>
#include <ctype.h>
#include <limits.h>
#include <stringx.h>
#include "cctype.h"
#include "ccsym.h"
#include "ccnode.h"
#include "globals.h"

Node_type Node::nodetype() { return N_NODE; }
Node_type Expr_node::nodetype() { return N_EXPR; }
Node_type Integer_node::nodetype() { return N_INTEGER; }
Node_type Sizeof_node::nodetype() { return N_SIZEOF; }
Node_type Float_node::nodetype() { return N_FLOAT; }
Node_type String_node::nodetype() { return N_STRING; }
Node_type Symbol_node::nodetype() { return N_SYMBOL; }
Node_type Array_ref_node::nodetype() { return N_ARRAY_REF; }
Node_type Func_call_node::nodetype() { return N_FUNC_CALL; }
Node_type Struct_ref_node::nodetype() { return N_STRUCT_REF; }
Node_type Operator_node::nodetype() { return N_OPERATOR; }
Node_type Operator2_node::nodetype() { return N_OPERATOR2; }
Node_type Operator3_node::nodetype() { return N_OPERATOR3; }
Node_type Cast_node::nodetype() { return N_CAST; }
Node_type Initializer_node::nodetype() { return N_INITIALIZER; }
Node_type Label_node::nodetype() { return N_LABEL; }
Node_type Case_node::nodetype() { return N_CASE; }
Node_type Switch_node::nodetype() { return N_SWITCH; }
Node_type Compound_node::nodetype() { return N_COMPOUND; }
Node_type If_node::nodetype() { return N_IF; }
Node_type While_node::nodetype() { return N_WHILE; }
Node_type Dowhile_node::nodetype() { return N_DOWHILE; }
Node_type For_node::nodetype() { return N_FOR; }
Node_type Goto_node::nodetype() { return N_GOTO; }
Node_type Continue_node::nodetype() { return N_CONTINUE; }
Node_type Break_node::nodetype() { return N_BREAK; }
Node_type Return_node::nodetype() { return N_RETURN; }
Node_type Asm_node::nodetype() { return N_ASM; }
Node_type New_node::nodetype() { return N_NEW; }
Node_type Delete_node::nodetype() { return N_DELETE; }

const char *Node::name()
		{ bug("Node::name() should never be invoked"); return "unknown-Node"; }
const char *Symbol_node::name()
		{ return _sym ? _sym->name() : "unknown-Symbol_node"; }
const char *Array_ref_node::name()
		{ return _arr ? _arr->name() : "unknown-Array_ref_node"; }
const char *Func_call_node::name()
		{ return _func ? _func->name() : "unknown-Func_call_node"; }
const char *Struct_ref_node::name()
		{ return _elem ? _elem->name() : "unknown-Struct_ref_node"; }
const char *Operator_node::name()
		{ return _arg ? _arg->name() : "unknown-Operator_node"; }
const char *Cast_node::name()
		{ return _cast ? _cast->name() : "unknown-Cast_node"; }

void
String_node::append(const char *str)
{
	int c = _wide ? g_size_wchar_t / g_size_char : 1;
	size_t len = _len * c;
	char *string = strnew(len + strlen(str) * c + c);
	char *b = string + len;

	if (_string != NULL)
		memcpy(string, _string, len);

	if (*str == '"')
		str++;

	if (_string == NULL && *str == '\\')
	{
		if ((g_macintosh || g_pascal_strings) && tolower(str[1]) == 'p')
		{
			// Pascal-strings - 1st word is length, not including
			// optional null-byte
			if (g_strict_iso)
				warn(NON_ISO_PASCAL_STRING);

			_pascal = TRUE;
			str += 2;
			b += c;				// leave room for the length of the string
			_len++;
		}
		else if (!g_strict_iso && str[1] == '$')
		{
			// Forth strings - separate length - struct $fstr
			_forth = TRUE;
			str += 2;
		}
	}

	// append new string to the end of old one, handling special escapes
	for (const char *s = str; *s != '\0' && *s != '"'; b += c, s++, _len++)
	{
		unsigned long val = 0;

		if (*s == '\\')
			switch (*++s)
			{
			case '\'': case '\"': case '?': case '\\':
				val = *s;
				break;

			// these are wired to the traditional C values for now
			case 'a': val = '\007'; break;		// alert (bell)
			case 'b': val = '\010'; break;		// backspace
			case 'f': val = '\014'; break;		// formfeed
			case 'n': val = '\012'; break;		// newline
			case 'r': val = '\015'; break;		// (carriage) return
			case 't': val = '\011'; break;		// (horizontal) tab
			case 'v': val = '\013'; break;		// vertical-tab

			case 'e':		// escape
				if (g_strict_iso)
					error(ILLEGAL_ESCAPE_IN_STRING, *s);
				else
					val = '\033';

				break;

			case 'x':		// multi-digit hexadecimal value
				{
					val = 0;
					int digits = 1;		// to round up properly when divided by 2

					for (s++; isxdigit(*s); s++, digits++)
					{
						val <<= 4;
						val += *s < '9' ? *s - '0' : tolower(*s) - 'a' + 10;
					}

					if ((digits >> 1) > c)
						warn(HEX_CHARACTER_TOO_BIG, c);
				}

				break;

			// upto 3-digit octal value
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				val = *s - '0';

				if (s[1] >= '0' && s[1] <= '7')
				{
					val = ((unsigned)val << 3) + *++s - '0';

					if (s[1] >= '0' && s[1] <= '7')
						val = ((unsigned)val << 3) + *++s - '0';
				}

				break;

			default:
				if (g_strict_iso)
					error(ILLEGAL_ESCAPE_IN_STRING, *s);
				else
					warn(ILLEGAL_ESCAPE_IN_STRING, *s);

				break;
			}
		else
			val = *s;

		// wchar_t must be one of (unsigned) char, short, or long on localhost
		if (c <= (int)sizeof (unsigned char))
			*(unsigned char*)b = (unsigned char)val;
		else if (c <= (int)sizeof (unsigned short))
			*(unsigned short*)b = (unsigned short)val;
		else if (c <= (int)sizeof (unsigned long))
			*(unsigned long*)b = (unsigned long)val;
		else
			bug("String_node::append(): wchar_t elt size %d too big", c);
	}

	if (c <= (int)sizeof (unsigned char))
		*(unsigned char*)b = '\0';
	else if (c <= (int)sizeof (unsigned short))
		*(unsigned short*)b = '\0';
	else if (c <= (int)sizeof (unsigned long))
		*(unsigned long*)b = '\0';
	else
		bug("String_node::append(): wchar_t elt size %d too big", c);

	if (_pascal)
	{
		if (c <= (int)sizeof (unsigned char))
		{
			if (_len > UCHAR_MAX)
				error(PASCAL_STRING_TOO_LONG, UCHAR_MAX);
			else
				*(unsigned char*)string = _len;
		}
		else if (c <= (int)sizeof (unsigned short))
		{
			if (_len > USHRT_MAX)
				error(PASCAL_STRING_TOO_LONG, USHRT_MAX);
			else
				*(unsigned short*)string = _len;
		}
		else if (c <= (int)sizeof (unsigned long))
		{
			if (_len > ULONG_MAX)
				error(PASCAL_STRING_TOO_LONG, ULONG_MAX);
			else
				*(unsigned long*)string = _len;
		}
		else
			bug("String_node::append(): wchar_t elt size %d too big", c);
	}

	strfree(_string);
	_string = string;
}

static int
case_cmp(const void *v1, const void *v2)
{
	Case_node *n1 = *(Case_node**)v1;
	Case_node *n2 = *(Case_node**)v2;

//fprintf(stderr, "\ncase_cmp(0x%X, 0x%X): node1 is ", v1, v2);
//n1->debug();
//fprintf(stderr, "\ncase_cmp(0x%X, 0x%X): node2 is ", v1, v2);
//n2->debug();

	return n1->val1()->compare(*n2->val1());
}

boolean
Switch_node::check_cases()
{
	if (_cases.size() == 0)
	{
		warn(NO_CASES_IN_SWITCH);
		return TRUE;
	}

	Type *exprtype = arith_conv(_expr->type(), g_int_type);
	_expr = implicit_cast(_expr, exprtype);

	if (!exprtype->isint())
	{
		error(EXPECTED_INT_EXPR_FOR_SWITCH);
		return FALSE;
	}

	// first we link all Case_nodes into an original-order list
	_firstcase = (Case_node*)_cases.elt(0);
	_default = -1;
	Case_node *c;
	int i;
	// workaround gcc bug if these are declared where needed in loop below
	Const_expr ce1, ce2;

	for (i = 0; i < _cases.size(); i++)
	{
		c = (Case_node*)_cases.elt(i);

		if (i + 1 < _cases.size())
			c->nextcase((Case_node *)_cases.elt(i + 1));
		else
			c->nextcase(NULL);

		// remember which entry had the "default" label
		if (c->expr1() == NULL)
		{
			if (_default >= 0)
			{
				error(ONLY_ONE_DEFAULT_IN_SWITCH, c->line());
				return FALSE;
			}

			_default = i;
			continue;
		}

		if (!valid_cast(c->expr1()->type(), exprtype))
		{
			error(CANNOT_CAST_TO_SWITCH_EXPR, c->line());
			return FALSE;
		}

		// TODO: we could actually handle non-const exprs properly
		if (!c->expr1()->is_int_expr())
		{
			error(CASE_EXPRESSION_MUST_BE_CONST, c->line());
			return FALSE;
		}

		// this should now be safe since we made sure this is an int expr
		c->expr1(implicit_cast(c->expr1(), exprtype));
		//Const_expr ce1;
		c->expr1()->eval_const_expr(ce1);
		c->val1(new Integer(*ce1.ival));

		// GNU-style ranged-case statement
		if (c->expr2() != NULL)
		{
			if (!valid_cast(c->expr2()->type(), exprtype))
			{
				error(CANNOT_CAST_TO_SWITCH_EXPR, c->line());
				return FALSE;
			}

			if (!c->expr2()->is_int_expr())
			{
				error(CASE_EXPRESSION_MUST_BE_CONST, c->line());
				return FALSE;
			}

			// this is also safe
			c->expr2(implicit_cast(c->expr2(), exprtype));
			//Const_expr ce2;
			c->expr2()->eval_const_expr(ce2);
			c->val2(new Integer(*ce2.ival));

			// val1 is allowed to be equal to val2
			if (*c->val1() > *c->val2())
			{
				error(RANGE_ERROR_IN_CASE, c->line());
				return FALSE;
			}
		}
	}

	// put the default entry into the last slot, if it is not already there
	int last = _cases.size() - 1;

	if (_default >= 0)
	{
		if (last != _default)
		{
			Node *n = _cases.elt(_default);
			_cases.elt(_default) = _cases.elt(last);
			_cases.elt(last) = n;
		}

		last--;
	}

	// now we sort the nodes according to each "val1"
	if (last > 0)
		qsort(_cases.getarr(), last + 1, sizeof (Node*), case_cmp);

	// and check for duplicate and overlapping case statements
	for (i = 0; i < last; i++)
	{
		c = (Case_node*)_cases.elt(i);
		Integer &v = *((Case_node*)_cases.elt(i + 1))->val1();
		int l = ((Case_node*)_cases.elt(i + 1))->line();

		if (c->expr2() != NULL && *c->val2() >= v)
			error(RANGE_OVERLAPS_CASE_IN_SWITCH, integertostr(v), l);
		else if (*c->val1() == v)
			error(DUPLICATE_CASES_IN_SWITCH, integertostr(v), l);
	}

	// check to see if the last statement is a "break" - this is a bug
	// in some C compilers, so warn about the problem here
	if (!_stmt->check_break())
		warn(MISSING_FINAL_BREAK_IN_SWITCH);

	return TRUE;
}


void
Expr_node::mark_symbol(boolean /*read*/, boolean /*wrote*/, boolean /*ref*/)
{
	// to mark the SUE type as read for back-end use
	Type *t = type();

	while (t->isarr() || t->isptr())
	{
		if (t->isarr())
			t = ((Array_type*)t)->type();
		else if (t->isptr())
			t = ((Pointer_type *)t)->type();
	}

	if (t->issue())
		((Struct_type*)t)->symbol()->read(TRUE);
}

void
Symbol_node::mark_symbol(boolean read, boolean wrote, boolean ref)
{
	if (_sym == NULL)
		return;

	if (_sym->type() == NULL || _sym->type() == g_unknown_type)
	{
		error(SYMBOL_NOT_DEFINED, _sym->name());
		_sym = NULL;
		return;
	}

	if (!read && !wrote && !ref && _sym->type()->isvolatile())
		read = TRUE;

	// all this for a crummy warning which still misses some cases
	if (read && !wrote && !ref &&
			!_sym->read() && !_sym->wrote() && !_sym->referenced() &&
			_sym->node() == NULL && _sym->scope() &&
			_sym->scope() != g_file_scope && !_sym->parameter() &&
			!_sym->type()->isvolatile() &&
			_sym->storage() != '&' && _sym->storage() != STATIC &&
			_sym->storage() != EXTERN && _sym->storage() != ENUM &&
			_sym->type()->typeval() != T_FUNCTION &&
			_sym->type()->typeval() != T_ARRAY)
		warn(SYMBOL_USED_BEFORE_INITIALIZED, _sym->name());

	if (ref && !_sym->referenced() && _sym->storage() == REGISTER)
	{
		warn(REF_OF_REGISTER_SYMBOL, _sym->name());
		_sym->storage(NONE);
	}

	int bump = 1;

	// use pow(3, loop depth) to adjust read/wrote counts for register usage
	for (int i = 0; i < g_loop_count; i++)
		bump *= 3;

	if (read)
		_sym->read(_sym->read() + bump);

	if (wrote)
		_sym->wrote(_sym->wrote() + bump);

	if (ref)
		_sym->referenced(TRUE);

	// this is just to mark the type as used for back-end use
	Expr_node::mark_symbol(read, wrote, ref);
}

void
Array_ref_node::mark_symbol(boolean read, boolean wrote, boolean /*ref*/)
{
	if (_arr == NULL || _expr == NULL ||
			_arr->type() == g_unknown_type ||
			_expr->type() == g_unknown_type)
		return;

	_arr->mark_symbol(read, wrote, TRUE);
	_expr->mark_symbol(TRUE, FALSE, FALSE);
}

void
Func_call_node::mark_symbol(boolean, boolean, boolean)
{
	if (_func == NULL || _func->type() == g_unknown_type)
		return;

	_func->mark_symbol(TRUE, FALSE, FALSE);

	if (_args == NULL)
		return;

	Function_type *ft = (Function_type*)_func->type();

	if (ft->isoverload() && _overload)
		ft = (Function_type*)_overload;

	if (ft == NULL || !ft->isfunc())
		return;

	Param_arr *fargs = ft->args();

	for (int i = 0; i < _args->size(); i++)
		if (_args->elt(i) != NULL && _args->elt(i)->type() != g_unknown_type)
		{
			if (fargs && fargs->size() > i && fargs->elt(i).storage() == '&')
				_args->elt(i)->mark_symbol(TRUE, TRUE, FALSE);
			else
				_args->elt(i)->mark_symbol(TRUE, FALSE, FALSE);
		}
}

void
Struct_ref_node::mark_symbol(boolean read, boolean wrote, boolean /*ref*/)
{
	if (_structref && _structref->type() != g_unknown_type)
		_structref->mark_symbol(read, wrote, FALSE);
}

void
Operator_node::mark_symbol(boolean read, boolean wrote, boolean ref)
{
	if (_arg == NULL || _arg->type() == g_unknown_type)
		return;

	switch (_token)
	{
	case '&':
		ref = TRUE;		// but NOT necessarily read
		break;

	case INCR:
	case DECR:
		read = wrote = TRUE;
		break;

	default:
		read = TRUE;
		break;
	}

	_arg->mark_symbol(read, wrote, ref);
}

void
Operator2_node::mark_symbol(boolean read, boolean wrote, boolean ref)
{
	if (_arg1 == NULL || _arg2 == NULL ||
			_arg1->type() == g_unknown_type ||
			_arg2->type() == g_unknown_type)
		return;

	boolean r = read;
	boolean w = wrote;
	boolean f = ref;

	switch (_token)
	{
	case '=':
		w = read = TRUE;
		break;

	case MULTEQ: case DIVEQ: case MODEQ:
	case PLUSEQ: case MINUSEQ:
	case LSHIFTEQ: case RSHIFTEQ:
	case ANDEQ: case XOREQ: case OREQ:
		r = w = read = TRUE;
		break;

	case ',':
		r = w = f = FALSE;
		break;

	default:
		r = read = TRUE;
		break;
	}

	_arg1->mark_symbol(r, w, f);
	_arg2->mark_symbol(read, wrote, ref);
}

void
Operator3_node::mark_symbol(boolean read, boolean wrote, boolean ref)
{
	if (_arg1 == NULL || _arg2 == NULL || _arg3 == NULL ||
			_arg1->type() == g_unknown_type ||
			_arg2->type() == g_unknown_type ||
			_arg3->type() == g_unknown_type)
		return;

	// this is only valid for operator?:
	_arg1->mark_symbol(TRUE, FALSE, FALSE);
	_arg2->mark_symbol(read, wrote, ref);
	_arg3->mark_symbol(read, wrote, ref);
}

void
Cast_node::mark_symbol(boolean read, boolean wrote, boolean ref)
{
	if (_cast == NULL || _cast->type() == g_unknown_type)
		return;

	_cast->mark_symbol(read, wrote, ref);
}

void
New_node::mark_symbol(boolean /*read*/, boolean /*wrote*/, boolean /*ref*/)
{
	if (_args == NULL)
		return;

	if (_arrlen != NULL && _arrlen->type() != g_unknown_type)
		_arrlen->mark_symbol(TRUE, FALSE, FALSE);

	for (int i = 0; i < _args->size(); i++)
		if (_args->elt(i) != NULL && _args->elt(i)->type() != g_unknown_type)
			_args->elt(i)->mark_symbol(TRUE, FALSE, FALSE);
}

void
Delete_node::mark_symbol(boolean /*read*/, boolean /*wrote*/, boolean /*ref*/)
{
	if (_expr == NULL || _expr->type() == g_unknown_type)
		return;

	_expr->mark_symbol(TRUE, FALSE, FALSE);
}


Node_type Node::check_return() { return nodetype(); }

Node_type Label_node::check_return()
{
	if (_stmt)
		return _stmt->check_return();
	
	return N_LABEL;
}

Node_type Case_node::check_return()
{
	if (_stmt)
		return _stmt->check_return();
	
	return N_CASE;
}

Node_type
Switch_node::check_return()
{
	// all cases must end with a return
	for (int i = 0; i < _cases.size(); i++)
		if (_cases.elt(i)->check_return() != N_RETURN)
			return N_SWITCH;

	return N_RETURN;
}

Node_type
Compound_node::check_return()
{
	// is the last statement a return?
	if (_stmts && _stmts->size() > 0)
		return _stmts->elt(_stmts->size() - 1)->check_return();
	
	return N_COMPOUND;
}

Node_type
If_node::check_return()
{
	Node_type ret;

	// do both branches return?
	if (_then && _else)
	{
		ret = _then->check_return();

		if (ret != N_RETURN)
			return ret;

		return _else->check_return();
	}
	
	return N_IF;
}

Node_type
While_node::check_return()
{
	if (_expr && _expr->is_int_expr())
	{
		Const_expr val;
		_expr->eval_const_expr(val);

		if (val.isint && !val.ival->isZero())
			return _stmt->check_return();
	}

	return N_WHILE;
}

Node_type
Dowhile_node::check_return()
{
	if (_expr && _expr->is_int_expr())
	{
		Const_expr val;
		_expr->eval_const_expr(val);

		if (val.isint && !val.ival->isZero())
			return _stmt->check_return();
	}

	return N_DOWHILE;
}

Node_type
For_node::check_return()
{
	if (_expr2 && _expr2->is_int_expr())
	{
		Const_expr val;
		_expr2->eval_const_expr(val);

		if (val.isint && !val.ival->isZero())
			return _stmt->check_return();
	}

	return N_FOR;
}

Node_type Goto_node::check_return() { return N_RETURN; }
Node_type Continue_node::check_return() { return N_RETURN; }
Node_type Return_node::check_return() { return N_RETURN; }


boolean Node::check_break() { return FALSE; }
boolean Label_node::check_break() { return _stmt && _stmt->check_break(); }
boolean Case_node::check_break() { return _stmt && _stmt->check_break(); }

boolean
Compound_node::check_break()
{
	// is the last statement a return?
	return _stmts && _stmts->size() > 0 &&
			_stmts->elt(_stmts->size() - 1)->check_break();
}

boolean
If_node::check_break()
{
	// do both branches return?
	return _then && _else && _then->check_break() && _else->check_break();
}

boolean Break_node::check_break() { return TRUE; }
boolean Continue_node::check_break() { return TRUE; }
boolean Return_node::check_break() { return TRUE; }
boolean Goto_node::check_break() { return TRUE; }


const char *
Node::brklabel()
{
	bug("Node::brklabel() should never be called");
	return NULL;
}

const char *
Node::contlabel()
{
	bug("Node::contlabel() should never be called");
	return NULL;
}

const char *Switch_node::brklabel() { return _brklabel; }
const char *While_node::brklabel() { return _brklabel; }
const char *While_node::contlabel() { return _contlabel; }
const char *Dowhile_node::brklabel() { return _brklabel; }
const char *Dowhile_node::contlabel() { return _contlabel; }
const char *For_node::brklabel() { return _brklabel; }
const char *For_node::contlabel() { return _contlabel; }

void
Initializer_node::append(Expr_node *elt, Expr_node *indx)
{
	if (indx != NULL)
	{
		if (g_strict_iso)
			warn(INIT_INDEX_IS_NOT_ISO);

		Const_expr loc;
		loc.new_ival(g_size_t_type->ic());
		indx->eval_const_expr(loc);
		_next = (int)loc.ival->get();// TODO - some range checks would be nice

		if (_next < 0)
			error(ILLEGAL_INIT_INDEX, _next);
		else if (_next < _init.size() && _init[_next] != NULL)
			error(DUPLICATE_INIT_INDEX, _next);
		else if (_next >= _init.size())
			// clear unused elements, or we will dump core later
			for (int i = _init.size(); i < _next; i++)
				_init[i] = NULL;
	}

	if (_next < 0)
		_init.end() = elt;
	else
		_init[_next++] = elt;
}

Node::~Node() { }

String_node::~String_node()
{
	if (_dup)
		strfree(_string);
}

Array_ref_node::~Array_ref_node()
{
	delete _arr;
	delete _expr;
}

Func_call_node::~Func_call_node()
{
	if (_args != NULL)
	{
		for (int i = 0; i < _args->size(); i++)
			delete _args->elt(i);

		delete _args;
	}

	delete _func;
}

Struct_ref_node::~Struct_ref_node() { delete _structref; }
Operator_node::~Operator_node() { delete _arg; }
Operator2_node::~Operator2_node() { delete _arg1; delete _arg2; }
Operator3_node::~Operator3_node() { delete _arg1; delete _arg2; delete _arg3; }
Cast_node::~Cast_node() { delete _cast; }

Initializer_node::~Initializer_node()
{
	for (int i = 0; i < _init.size(); i++)
		delete _init.elt(i);
}

Label_node::~Label_node() { delete _stmt; }
Case_node::~Case_node() { delete _expr1; delete _expr2; delete _stmt; }
Switch_node::~Switch_node() { delete _expr; delete _stmt; }

Compound_node::~Compound_node()
{
	if (_stmts == NULL)
		return;

	for (int i = 0; i < _stmts->size(); i++)
		delete _stmts->elt(i);

	delete _stmts;
}

If_node::~If_node() { delete _expr; delete _then; delete _else; }
While_node::~While_node() { delete _expr; delete _stmt; }
Dowhile_node::~Dowhile_node() { delete _stmt; delete _expr; }
For_node::~For_node()
		{ delete _expr1; delete _expr2; delete _expr3; delete _stmt; }
Return_node::~Return_node() { delete _expr; }
Asm_node::~Asm_node() { delete _asmstr; }

New_node::~New_node()
{
	delete _args;
	delete _arrlen;
}

Delete_node::~Delete_node()
{
	delete _expr;
}
