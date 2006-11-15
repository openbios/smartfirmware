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
// $Header: /u/cgt/cvs/bin/cc/cctree2.cc,v 1.26 2003/05/01 06:33:13 parag Exp $

#include <stddef.h>
#include <memory.h>
#include <ctype.h>
#include <pool.h>
#include <stdiox.h>
#include <stringx.h>
#include <ir.h>
#include <storage.h>
#include "cctype.h"
#include "ccsym.h"
#include "ccnode.h"
#include "cctree.h"
#include "globals.h"

LValueTreeNode *
Operator2_node::mklval()
{
	if (!_arg1->type()->isptr() && !_arg1->type()->isarr())
		bug("Operator2_node::mklval(): expected array/pointer");

	TreeNode *ret = NULL;

	switch (_token)
	{
	case '+':
		// TODO
		ret = mkpadd(_arg1->mktree(), _arg2->mktree(),
				_arg2->type()->getwidth(), _arg1->type()->getsize());
		break;

	case '-':
		if (_arg2->type()->isptr() || _arg2->type()->isarr())
			bug("Operator2_node::mklval(): did not expected array/pointer");

		ret = mkpsub(_arg1->mktree(), _arg2->mktree(),
				_arg2->type()->getwidth(), _type->getsize());
		break;

	default:
		bug("Operator2_node::mklval(): illegal token %d", _token);
		break;
	}

	return mkptrlvalue(ret, _type->isvolatile());
}

TreeNode *
Operator2_node::mktree()
{
	TreeNode *t1, *t2;
	LValueTreeNode *lt1;
	boolean v2 = _arg2->type()->isvolatile();

	switch (_token)
	{
	case '*':
		t1 = _arg1->mktree();
		t2 = _arg2->mktree();

		if (_type->isfloat())
			return mkfmul(t1, t2, _type->getwidth());
		else
			return mkmul(t1, t2, _type->getwidth());

		break;

	case '/':
		t1 = _arg1->mktree();
		t2 = _arg2->mktree();

		if (_type->isfloat())
			return mkfdiv(t1, t2, _type->getwidth());
		else
			return mkdiv(t1, t2, _type->getwidth());

		break;

	case '%':
		t1 = _arg1->mktree();
		t2 = _arg2->mktree();
		return mkmod(t1, t2, _type->getwidth());

	case '+':
		if (_type->isptr() || _type->isarr())
			return mkpadd(_arg1->mktree(), _arg2->mktree(),
					_arg2->type()->getwidth(),
					((Pointer_type*)_type)->type()->getsize());

		t1 = _arg1->mktree();
		t2 = _arg2->mktree();

		if (_type->isfloat())
			return mkfadd(t1, t2, _type->getwidth());
		else
			return mkadd(t1, t2, _type->getwidth());

		break;

	case '-':
		if (_arg1->type()->isptr() || _arg1->type()->isarr())
		{
			// TODO
			if (_arg2->type()->isptr() || _arg2->type()->isarr())
				return mkppsub(_arg1->mktree(), _arg2->mktree(),
						_arg1->type()->getsize());

			return mkpsub(_arg1->mktree(), _arg2->mktree(),
					_arg2->type()->getwidth(), _type->getsize());
		}

		t1 = _arg1->mktree();
		t2 = _arg2->mktree();

		if (_type->isfloat())
			return mkfsub(t1, t2, _type->getwidth());
		else
			return mksub(t1, t2, _type->getwidth());

		break;

	case RSHIFT:
		// TODO
		if (((Int_type*)_arg1->type())->sign())
			return mkarshift(_arg1->mktree(), _arg1->type()->getwidth(),
					_arg2->mktree(), _arg2->type()->getwidth());
		else
			return mkrshift(_arg1->mktree(), _arg1->type()->getwidth(),
					_arg2->mktree(), _arg2->type()->getwidth());

		break;

	case LSHIFT:
		return mklshift(_arg1->mktree(), _arg1->type()->getwidth(),
				_arg2->mktree(), _arg2->type()->getwidth());

	case '<':
		t1 = _arg1->mktree();
		t2 = _arg2->mktree();

		if (_arg1->type()->isfloat())
			return mkflessthan(t1, t2, _arg1->type()->getwidth());
		else
			return mklessthan(t1, t2, _arg1->type()->getwidth());

		break;

	case '>':
		t1 = _arg1->mktree();
		t2 = _arg2->mktree();

		if (_arg1->type()->isfloat())
			return mkfgreaterthan(t1, t2, _arg1->type()->getwidth());
		else
			return mkgreaterthan(t1, t2, _arg1->type()->getwidth());

		break;

	case LESSEQ:
		t1 = _arg1->mktree();
		t2 = _arg2->mktree();

		if (_arg1->type()->isfloat())
			return mkfltequal(t1, t2, _arg1->type()->getwidth());
		else
			return mkltequal(t1, t2, _arg1->type()->getwidth());

		break;

	case GREATEQ:
		t1 = _arg1->mktree();
		t2 = _arg2->mktree();

		if (_arg1->type()->isfloat())
			return mkfgtequal(t1, t2, _arg1->type()->getwidth());
		else
			return mkgtequal(t1, t2, _arg1->type()->getwidth());

		break;

	case EQUAL:
		t1 = _arg1->mktree();
		t2 = _arg2->mktree();

		if (_arg1->type()->isfloat())
			return mkfequal(t1, t2, _arg1->type()->getwidth());
		else
			return mkequal(t1, t2, _arg1->type()->getwidth());

		break;

	case NOTEQ:
		t1 = _arg1->mktree();
		t2 = _arg2->mktree();

		if (_arg1->type()->isfloat())
			return mkfnotequal(t1, t2, _arg1->type()->getwidth());
		else
			return mknotequal(t1, t2, _arg1->type()->getwidth());

		break;

	case '&':
		return mkand(_arg1->mktree(), _arg2->mktree(), _type->getwidth());

	case '^':
		return mkxor(_arg1->mktree(), _arg2->mktree(), _type->getwidth());

	case '|':
		return mkor(_arg1->mktree(), _arg2->mktree(), _type->getwidth());

	case ANDAND:
		return mklogand(_arg1->mktree(), _arg1->type()->getwidth(),
				_arg1->type()->isfloat(),
				_arg2->mktree(), _arg2->type()->getwidth(),
				_arg2->type()->isfloat());

	case OROR:
		return mklogor(_arg1->mktree(), _arg1->type()->getwidth(),
				_arg1->type()->isfloat(),
				_arg2->mktree(), _arg2->type()->getwidth(),
				_arg2->type()->isfloat());

	case ',':
		return mkcomma(_arg1->mktree(), _arg1->type()->getsize(),
				_arg1->type()->isfloat(),
				_arg2->mktree(), _arg2->type()->getsize(),
				_arg2->type()->isfloat());


	// to handle bitfields for all assignments, mkas*() must be called
	// immediately after calling mkbitaddr(), thus arg2->mktree() must be
	// called before calling arg1->mklval()
	//
	// The above comment is no longer accurate now that we have
	// LValueTreeNode's and bitbields don't use magic static values.
	// TJ 1/24/93

	case '=':
		lt1 = _arg1->mklval();
		t2 = _arg2->mktree();

		if (_type->issu())
			return mkassign(lt1, t2, _type->getsize(), FALSE);
		else if (_type->isarr())
			// this doesn't seem quite right, since assignment to
			// array's isn't really defined.
			// TJ 1/24/93
			return mkblkassign(_arg1->mktree(), _arg1->type()->isvolatile(),
					t2, _type->getsize());
		else if (_type->isfloat())
			return mkfassign(lt1, t2, _type->getwidth());
		else
			return mkassign(lt1, t2, _type->getwidth());

		break;

	case MULTEQ:
		return mkasmul(_arg1->mklval(), _type->getwidth(), _type->isfloat(),
				_type->sign(), _arg2->mktree(), _arg2->type()->getwidth(),
				_arg2->type()->isfloat(), _arg2->type()->sign());

	case DIVEQ:
		return mkasdiv(_arg1->mklval(), _type->getwidth(), _type->isfloat(),
				_type->sign(), _arg2->mktree(), _arg2->type()->getwidth(),
				_arg2->type()->isfloat(), _arg2->type()->sign());

	case MODEQ:
		return mkasmod(_arg1->mklval(), _type->getwidth(), _type->sign(),
				_arg2->mktree(), _arg2->type()->getwidth(),
				_arg2->type()->sign());

	case PLUSEQ:
		lt1 = _arg1->mklval();
		t2 = _arg2->mktree();

		if (_type->isptr() || _type->isarr())
			return mkaspadd(lt1, t2, _arg2->type()->getwidth(),
					((Pointer_type*)_type)->type()->getsize());
		else
			return mkasadd(lt1, _type->getwidth(), _type->isfloat(),
					_type->sign(), t2, _arg2->type()->getwidth(),
					_arg2->type()->isfloat());

		break;

	case MINUSEQ:
		lt1 = _arg1->mklval();
		t2 = _arg2->mktree();

		if (_type->isptr() || _type->isarr())
			return mkaspsub(lt1, t2, _arg2->type()->getwidth(),
					((Pointer_type*)_type)->type()->getsize());
		else
			return mkassub(lt1, _type->getwidth(), _type->isfloat(),
					_type->sign(), t2, _arg2->type()->getwidth(),
					_arg2->type()->isfloat());

		break;

	case LSHIFTEQ:
		return mkaslshift(_arg1->mklval(), _arg1->type()->getwidth(),
				_arg2->mktree(), _arg2->type()->getwidth());

	case RSHIFTEQ:
		// TODO
		lt1 = _arg1->mklval();
		t2 = _arg2->mktree();

		if (((Int_type*)_arg1->type())->sign())
			return mkasarshift(lt1, _arg1->type()->getwidth(), t2,
					_arg2->type()->getwidth());
		else
			return mkasrshift(lt1, _arg1->type()->getwidth(), t2,
					_arg2->type()->getwidth());

		break;

	case ANDEQ:
		return mkasand(_arg1->mklval(), _type->getwidth(), _type->sign(),
				_arg2->mktree(), _arg2->type()->getwidth());

	case XOREQ:
		return mkasxor(_arg1->mklval(), _type->getwidth(), _type->sign(),
				_arg2->mktree(), _arg2->type()->getwidth());

	case OREQ:
		return mkasor(_arg1->mklval(), _type->getwidth(), _type->sign(),
				_arg2->mktree(), _arg2->type()->getwidth());

	default:
		bug("Operator2_node::mktree() illegal token %d", _token);
		break;
	}

	return NULL;
}

TreeNode *
Operator2_node::mkinit()
{
	Const_expr e;
	eval_const_expr(e);

	if (e.sym != NULL)
	{
		if (e.ival != NULL)
			return mkinitptr(mksymbolname(*e.sym), *e.ival);
		else
			return mkinitptr(mksymbolname(*e.sym), 0);
	}
	else if (e.isint)
		return mkinitconst(*e.ival);
	else
		return mkinitfconst(*e.fval);
}

LValueTreeNode *
Operator3_node::mklval()
{
	if (_token != '?')
		bug("Operator3_node::mktree() illegal token %d", _token);

	return mkptrlvalue(Operator3_node::mktree(), _type->isvolatile());
}

TreeNode *
Operator3_node::mktree()
{
	if (_token != '?')
		bug("Operator3_node::mktree() illegal token %d", _token);

	return mkcond(_arg1->mktree(), _arg1->type()->getwidth(),
			_arg1->type()->isfloat(), _arg2->mktree(), _arg3->mktree(), 
			_type->getsize(), _type->isfloat());
}

TreeNode *
Operator3_node::mkinit()
{
	Const_expr e;
	eval_const_expr(e);

	if (e.sym != NULL)
	{
		if (e.ival != NULL)
			return mkinitptr(mksymbolname(*e.sym), *e.ival);
		else
			return mkinitptr(mksymbolname(*e.sym), 0);
	}
	else if (e.isint)
		return mkinitconst(*e.ival);
	else
		return mkinitfconst(*e.fval);
}

TreeNode *
do_cast(Expr_node *expr, Type *from, Type *to)
{
	if (to == from)
		return expr->mktree();

	boolean tsigned = TRUE;
	boolean fsigned = TRUE;
	Type_val tt = T_FLOAT;
	Type_val ft = T_FLOAT;
	TreeNode *tree;

	if (from->isint() || from->isbit())
	{
		fsigned = ((Int_type*)from)->sign();
		ft = T_INT;
	}
	else if (from->isptr() || from->isfunc())
		ft = T_POINTER;
	else if (from->isarr())
		ft = T_ARRAY;

	if (to->isint())
	{
		tsigned = ((Int_type*)to)->sign();
		tt = T_INT;
	}
	else if (to->isptr() || to->isarr() || to->isfunc())
		tt = T_POINTER;

	// cast struct pointer into substruct pointer
	if (tt == T_POINTER && tt == ft)
	{
		Pointer_type *fp = (Pointer_type*)from;
		Pointer_type *tp = (Pointer_type*)to;
		tree = expr->mktree();

		if (!fp->type()->isstruct() || !tp->type()->isstruct())
			return tree;

		Struct_type *fs = (Struct_type*)fp->type();
		Struct_type *ts = (Struct_type*)tp->type();

		// look for "from" in the member list of "to"
		Symbol **mem = fs->memlist()->getarr();

		for (int i = 0; i < fs->memlist()->size(); i++)
			if (mem[i]->type() == tp && mem[i]->name()[0] == UNNAMED_CHAR)
				break;

		if (i >= fs->memlist()->size())
			bug("do_cast: autocast of struct* --> substruct* failed");

		// element is union
		if (mem[i]->offset() == NULL)
			return tree;

		// add offset struct* to make it a substruct*
		return mkpadd(tree, mkconst(*mem[i]->offset()),
				g_size_pointer, *g_min_addr_size);
	}

	if (tt == T_POINTER && ft == T_ARRAY)
	{
		boolean v;
		lvalueptr(expr->mklval(), tree, v);
		return tree;
	}

	int twidth = to->getwidth();
	int fwidth =  from->getwidth();
	tree = expr->mktree();

	if (fwidth == twidth)
		return tree;

	return mkcast(tree, from->getwidth(), from->isfloat(), fsigned,
			to->getwidth(), to->isfloat(), tsigned);
}

TreeNode *
Cast_node::mktree()
{
	if (_type->isvoid())
		return mkcastvoid(_cast->mktree(), _cast->type()->getsize(),
				_cast->type()->isfloat());
	else if (_type->isptr() && _cast->type()->isptr())
		return _cast->mktree();
	else
		return do_cast(_cast, _cast->type(), _type);
}

LValueTreeNode *
Cast_node::mklval()
{
	return mkptrlvalue(Cast_node::mktree(), _type->isvolatile());
}

TreeNode *
Cast_node::mkinit()
{
	return _cast->mkinit();
}

TreeNode *
mkzero(Integer count)
{
	Treevec list;

	for (; count.isPos(); count--)
		list.end() = mkconst(0, g_min_addressable);

	return mklist(list.size(), list.getarr());
}

void
mk_struct_init(Struct_type *ts, Treevec &list, Expr_arr &init, boolean isinit)
{
	int i = 0;
	int j = 0;
	Symbol **mem = ts->memlist()->getarr();
	Integer *word = NULL;
	int wordwidth = 0;
	int width = 0;

	for (; i < init.size() && j < ts->memlist()->size(); j++)
	{
		if (init.elt(i) == NULL)
		{
			i++;
			continue;
		}

		if (mem[j]->type()->isbit())
		{
			Bitfield_type &tb = *(Bitfield_type*)mem[j]->type();

			// bitfield zero means we are done with this word
			if (tb.width() == 0)
			{
				if (word != NULL)
				{
					list.end() = isinit ? mkinitconst(*word) : mkconst(*word);
					delete word;
					word = NULL;
					width = wordwidth = 0;
				}

				continue;
			}
			else
			{
				// out of space
				if (tb.width() > width)
				{
					if (word != NULL)
					{
						list.end() = isinit ? mkinitconst(*word) :
								mkconst(*word);
						delete word;
					}

					word = new Integer(tb.ic());
					width = wordwidth = tb.ic().bits();
				}

				// unnamed bitfields may not be initialized
				if (mem[j]->name != NULL)
				{
					Const_expr e;
					init.elt(i)->eval_const_expr(e);

					if (e.sym != NULL || !e.isint)
						bug("init_struct - expected int for elt %d", i);

					addbitinit(*word, *e.ival, tb.width(), wordwidth - width);
				}

				if (mem[j]->pad() != NULL)
					list.end() = isinit ? mkinitzero(*mem[j]->pad()) :
							mkzero(*mem[j]->pad());

				width -= tb.width();
			}
		}
		else
		{
			if (word != NULL)
			{
				list.end() = isinit ? mkinitconst(*word) : mkconst(*word);
				delete word;
				word = NULL;
				width = wordwidth = 0;
			}

			list.end() = isinit ? init.elt(i)->mkinit() : init.elt(i)->mktree();

			if (mem[j]->pad() != NULL)
				list.end() = isinit ? mkinitzero(*mem[j]->pad()) :
						mkzero(*mem[j]->pad());
		}

		i++;
	}

	if (word != NULL)
	{
		list.end() = isinit ? mkinitconst(*word) : mkconst(*word);
		delete word;
		word = NULL;
	}

	if (!isinit)
		return;

	for (; j < ts->memlist()->size(); j++)
	{
		if (mem[j]->type()->isbit())
		{
			Bitfield_type &tb = *(Bitfield_type*)mem[j]->type();

			// bitfield zero means we are done with this word
			if (tb.width() == 0)
			{
				if (wordwidth)
					list.end() = mkinitzero(wordwidth);

				width = wordwidth = 0;
				continue;
			}
			else
			{
				// out of space
				if (tb.width() > width)
				{
					if (wordwidth)
						list.end() = mkinitzero(wordwidth);

					width = wordwidth = tb.ic().bits();
				}

				if (mem[j]->pad() != NULL)
					list.end() = mkinitzero(*mem[j]->pad());

				width -= tb.width();
			}
		}
		else
		{
			if (wordwidth)
			{
				list.end() = mkinitzero(wordwidth);
				width = wordwidth = 0;
			}

			list.end() = mkinitzero(mem[j]->type()->getsize());

			if (mem[j]->pad() != NULL)
				list.end() = mkinitzero(*mem[j]->pad());
		}
	}
}

void
mk_array_init(Array_type *ta, Treevec &list, Expr_arr &init, boolean isinit)
{
	Integer &eltsize = ta->type()->getsize();
	int i = 0;

	for (; i < init.size(); i++)
		if (init.elt(i) != NULL)
			list.end() = isinit ? init.elt(i)->mkinit() :
					init.elt(i)->mktree();
		else
			list.end() = mkinitzero(eltsize);

	if (!isinit)
		return;

	Integer len = ta->len();
	Integer ii(len.integerclass(), i);
	len.sub(ii);

	if (len.isPos())
	{
		len.mult(eltsize);
		list.end() = mkinitzero(len);
	}
}

TreeNode *
Initializer_node::mkinit()
{
	Treevec list;

	switch (_type->typeval())
	{
	case T_STRUCT:
		mk_struct_init((Struct_type*)_type, list, _init, TRUE);
		break;

	case T_UNION:
		list.end() = _init.elt(0)->mkinit();
		break;

	case T_ARRAY:
	case T_FUNCTION:
		mk_array_init((Array_type*)_type, list, _init, TRUE);
		break;

	default:
		bug("Initializer_node::mkinit(): unexpected type %d", _type);
	}

	return mklist(list.size(), list.getarr());
}

TreeNode *
Initializer_node::mktree()
{
	Treevec list;

	switch (_type->typeval())
	{
	case T_STRUCT:
		mk_struct_init((Struct_type*)_type, list, _init, FALSE);
		break;

	case T_UNION:
		list.end() = _init.elt(0)->mktree();
		break;

	case T_ARRAY:
	case T_FUNCTION:
		mk_array_init((Array_type*)_type, list, _init, FALSE);
		break;

	default:
		bug("Initializer_node::mkinit(): unexpected type %d", _type);
	}

	return mklist(list.size(), list.getarr());
}

TreeNode *
Label_node::mktree()
{
	if (_stmt == NULL)
		return mklabel(mklabelname(*_sym));
	else
		return mklist(mklabel(mklabelname(*_sym)), _stmt->mktree());
}

TreeNode *
Case_node::mktree()
{
	if (_stmt == NULL)
		return mklabel(_label);
	else
		return mklist(mklabel(_label), _stmt->mktree());
}

TreeNode *
Switch_node::mktree()
{
//	int size = (_default >= 0) ? _cases.size() - 1 : _cases.size();
//	int size = _cases.size() - (_default >= 0) ? 1 : 0);
	int size = _cases.size() - (_default >= 0);
	Integer const **mins = new Integer const *[size];
	Integer const **maxs = new Integer const *[size];
	const char **labels = new char const *[size];
	const char *deflabel = (_default >= 0) ? mkasmlabel("D" ASMCHAR) : NULL;
	_brklabel = mkasmlabel("B" ASMCHAR);

	for (int i = 0; i < size; i++)
	{
		Case_node *c = (Case_node*)_cases.elt(i);
		mins[i] = c->val1();
		maxs[i] = (c->val2() == NULL) ? c->val1() : c->val2();
		labels[i] = mkasmlabel("K" ASMCHAR);
		c->label(labels[i]);
	}

	if (_default >= 0)
			((Case_node*)_cases.elt(size))->label(deflabel);

	TreeNode *st = (_stmt == NULL) ? mkemptynode() : _stmt->mktree();
	TreeNode *ret = mkswitch(_expr->mktree(), _expr->type()->getwidth(),
			size, mins, maxs, labels, deflabel, _brklabel, st);
	//delete mins;
	//delete maxs;
	//delete labels;
	return ret;
}

TreeNode *
Compound_node::mktree()
{
	if (_stmts == NULL)
		return mkemptynode();

	Treevec list;
	TreeNode *n = switch_seg(NULL, SEG_CODE);

	if (n != NULL)
		list.end() = n;

	list.end() = _scope->mktree();

	for (int i = 0; i < _stmts->size(); i++)
		list.end() = _stmts->elt(i)->mktree();

	return mklist(list.size(), list.getarr());
}

TreeNode *
If_node::mktree()
{
	TreeNode *tn = (_then == NULL) ? NULL : _then->mktree();

	if (_else == NULL)
		return mkif(_expr->mktree(), _expr->type()->getwidth(),
				_expr->type()->isfloat(), tn);
	else
		return mkif(_expr->mktree(), _expr->type()->getwidth(), 
				_expr->type()->isfloat(), tn, _else->mktree());
}

TreeNode *
While_node::mktree()
{
	_brklabel = mkasmlabel("B" ASMCHAR);
	_contlabel = mkasmlabel("C" ASMCHAR);
	TreeNode *st = (_stmt == NULL) ? mkemptynode() : _stmt->mktree();
	return mkwhile(_expr->mktree(), _expr->type()->getwidth(),
			_expr->type()->isfloat(), _brklabel, _contlabel, st);
}

TreeNode *
Dowhile_node::mktree()
{
	_brklabel = mkasmlabel("B" ASMCHAR);
	_contlabel = mkasmlabel("C" ASMCHAR);
	TreeNode *st = (_stmt == NULL) ? mkemptynode() : _stmt->mktree();
	return mkdowhile(_expr->mktree(), _expr->type()->getwidth(),
			_expr->type()->isfloat(), _brklabel, _contlabel, st);
}

TreeNode *
For_node::mktree()
{
	_brklabel = mkasmlabel("B" ASMCHAR);
	_contlabel = mkasmlabel("C" ASMCHAR);

	TreeNode *t1 = (_expr1 == NULL) ? NULL : _expr1->mktree();
	TreeNode *t2 = NULL;
	TreeNode *t3 = (_expr3 == NULL) ? NULL : _expr3->mktree();
	TreeNode *st = (_stmt == NULL) ? mkemptynode() : _stmt->mktree();
	int w2 = 0;
	boolean f2 = FALSE;

	if (_expr2 != NULL)
	{
		t2 = _expr2->mktree();
		w2 = _expr2->type()->getwidth();
		f2 = _expr2->type()->isfloat();
	}

	return mkfor(t1, t2, w2, f2, t3, _brklabel, _contlabel, st);
}

TreeNode *
Goto_node::mktree()
{
	return mkgoto(mklabelname(*_sym));
}

TreeNode *
Continue_node::mktree()
{
	return mkgoto(_loop->contlabel());
}

TreeNode *
Break_node::mktree()
{
	return mkgoto(_stmt->brklabel());
}

TreeNode *
Return_node::mktree()
{
	Type *ret = ((Function_type*)_func->type())->retval();
	extern const char *g_return_label;
	TreeNode *tr = (_expr == NULL) ? NULL : _expr->mktree();
	Integer &size = ret->getsize();
	return mkreturn(tr, size, ret->isfloat(), g_return_label);
}

TreeNode *
Asm_node::mktree()
{
	return mkasmcode(_asmstr->string());
}
