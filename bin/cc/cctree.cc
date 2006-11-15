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
// $Header: /u/cgt/cvs/bin/cc/cctree.cc,v 1.38 2003/05/01 06:33:13 parag Exp $

#include <stddef.h>
#include <memory.h>
#include <ctype.h>
#include <pool.h>
#include <stdiox.h>
#include <stringx.h>
#include <tree.h>
#include <ir.h>
#include <storage.h>
#include <frontend.h>
#include "cctype.h"
#include "ccsym.h"
#include "ccnode.h"
#include "cctree.h"
#include "globals.h"

class Noop_node : public Node
{
public:
	Noop_node() : Node() { }
};

static Noop_node *g_noop_node = new Noop_node;

const char *
mksymbolname(Symbol &s)
{
	if (s.storage() == STATIC && s.scope() != g_file_scope)
	{
		Scope *scope = s.scope();

		while (scope != NULL && scope->symbol() == NULL)
			scope = scope->parent();

		if (scope != NULL)
			return strget(xsprintf("%s" ASMCHAR "%s", scope->symbol()->name(),
					s.name()));
	}

	return s.name();
}

const char *
mklabelname(Symbol &s)
{
	return strget(xsprintf("L" ASMCHAR "%s", s.name()));
}

TreeNode *
switch_seg(TreeNode *node, SegmentType seg, boolean ext)
{
	if (!segmentchanged(seg, ext))
			return node;

	TreeNode *segtree = mkswitchseg(seg, ext);

	if (node == NULL)
		return segtree;

	return islist(node) ? mkpair(segtree, node) : mklist(segtree, node);
}

static Treevec g_list;

void
init_tree()
{
	if (!g_do_tree || num_errors())
		return;

	g_list.end() = mkprogbegin(g_file_scope->file());
}

void
do_tree(Symbol *sym)
{
	if (sym == NULL || !g_do_tree || num_errors())
	{
		g_list.reset();
		return;
	}

	if (sym->treedone() || sym->storage() == TYPEDEF ||
			sym->storage() == ENUM || sym->parameter() ||
			(!sym->type()->isfunc() && sym->storage() != STATIC &&
					sym->scope() != g_file_scope))
		return;

	TreeNode *tn = sym->mktree();

	if (tn != NULL)
	{
		g_list.end() = tn;
		sym->treedone(TRUE);

		if (sym->node() != NULL)
		{
			// do not need the function bodies or initialization lists
			delete sym->node();
			sym->node(g_noop_node);
		}
	}
}

void
do_tree(Node *node)
{
	if (node == NULL || !g_do_tree || num_errors())
	{
		g_list.reset();
		return;
	}

	// any global asm statements
	g_list.end() = node->mktree();
	delete node;
}

static Node_arr g_string_nodes;

TreeNode *
fini_tree()
{
	if (!g_do_tree || num_errors())
	{
		g_list.reset();
		return NULL;
	}

	g_list.end() = g_file_scope->mktree();

	if (g_string_nodes.size() > 0)
	{
		// dump all the strings which are used as strings
		TreeNode *seg = switch_seg(NULL, SEG_DATA);

		if (seg != NULL)
			g_list.end() = seg;

		for (int i = 0; i < g_string_nodes.size(); i++)
		{
			String_node &sn = *(String_node*)g_string_nodes.elt(i);

			// only strings not used as initializers need to be declared
			if (sn.label() == NULL)
				bug("fini_tree(): string-node label is NULL");

			g_list.end() = mkdefinition(sn.label(),
					sn.type()->getsize(),
					sn.type()->align() / g_min_addressable);
			g_list.end() = sn.mkinit();
		}
	}

	TreeNode *seg = switch_seg(NULL, SEG_NONE);

	if (seg)
		g_list.end() = seg;

	g_list.end() = mkprogend(g_file_scope->file());
	return mklist(g_list.size(), g_list.getarr());
}

// make tree for everything within this scope
TreeNode *
Scope::mktree()
{
	if (_init == NULL)
		return mkemptynode();

	Treevec list;

	// declare all local vars in sub-scopes
	for (int i = 0; i < _init->size(); i++)
	{
		Symbol &s = *_init->elt(i);

		if (s.storage() == TYPEDEF || s.storage() == ENUM ||
				s.storage() == STATIC || s.parameter() || s.treedone())
			continue;

		TreeNode *tn = s.mktree();

		if (tn != NULL)
		{
			list.end() = tn;
			s.treedone(TRUE);

			if (s.node() != NULL)
			{
				// do not need initializers any longer
				delete s.node();
				s.node(g_noop_node);
			}
		}
	}

	if (list.size() == 0)
		return mkemptynode();

	return mklist(list.size(), list.getarr());
}

static int
register_cmp(const void *v1, const void *v2)
{
	Symbol *s1 = *(Symbol**)v1;
	Symbol *s2 = *(Symbol**)v2;

	// sort order is most-used first, least used last
	return (s2->read() + s2->wrote()) - (s1->read() + s1->wrote());
}

// calculate local var size necessary for stack
void
Scope::calclocals(StorageAllocator &stkalloc)
{
	if (_symtab != NULL)		// for local var nodes
	{
		// try to put most-used symbols into registers
		Symbol **reglist = new Symbol*[_symtab->size()];
		int i = 0;
		Symbol_iter si;

		for (si = *_symtab; si; i++, si++)
			reglist[i] = &si();

		qsort(reglist, i, sizeof reglist[0], register_cmp);

		// calculate offsets for local vars
		for (i = 0; i < _symtab->size(); i++)
		{
			Symbol &s = *reglist[i];

			if (s.storage() != TYPEDEF && s.storage() != ENUM &&
					s.storage() != STATIC && s.offset() == NULL &&
					!s.type()->isfunc())
			{
				// try to put this symbol into a register
				if (!s.referenced())
				{
					int reg;

					switch (s.type()->typeval())
					{
					case T_FLOAT:
						if (stkalloc.allocfloatreg(s.type()->getwidth(), reg))
						{
							s.regnum(reg + 1);
							continue;
						}

						break;

					case T_POINTER:
						if (stkalloc.allocptrreg(s.type()->getwidth(), reg))
						{
							s.regnum(reg + 1);
							continue;
						}

						break;

					case T_INT:
					case T_ENUM:
						if (stkalloc.allocreg(s.type()->getwidth(), reg))
						{
							s.regnum(reg + 1);
							continue;
						}

						break;

					default:
						break;
					}
				}

				if (s.parameter())
					continue;

				s.offset(new Integer(g_size_t_type->ic()));
				stkalloc.alloclocal(s.type()->getsize(),
						s.type()->align() / g_min_addressable,
						*s.offset());
			}
		}

		delete reglist;
	}

	// this overlays sub-scope variables on top of each other
	if (_children != NULL)
		for (int i = 0; i < _children->size(); i++)
		{
			stkalloc.pushscope();
			_children->elt(i)->calclocals(stkalloc);
			stkalloc.popscope();
		}
}

// hack for now, but as functions are not nested, this is not a problem
const char *g_return_label = NULL;

// make tree for this symbol - either global/extern/static data or function
TreeNode *
Symbol::mktree()
{
	// names which do not denote code or data
	if (_storage == TYPEDEF || _storage == ENUM)
		return NULL;

	if (_type->typeval() == T_FUNCTION)
	{
		// just an extern declaration
		if (_node == NULL)
		{
			if (!_read && !_wrote && !_referenced)
					return NULL;

			return switch_seg(mkdeclextern(_name, g_size_t_type->ic().zero(),
						FALSE), SEG_CODE, TRUE);
		}

		// real function definition - setup the label for return statements
		g_return_label = strget(xsprintf("R" ASMCHAR "%s", _name));

		// the size of the return value from this function
		Function_type *ft = (Function_type*)_type;
		Integer &retsize = ft->retval()->getsize();

		StorageAllocator stkalloc;

		// setup our function parameter offsets
		if (ft->args() != NULL && _scope != NULL && _scope != g_file_scope)
			for (int i = 0; i < ft->args()->size(); i++)
			{
				if (ft->args()->elt(i).storage() == DOTDOTDOT)
					break;

				Symbol &s = *_scope->init()->elt(i);

				// 1st arg symbols should be parameters
				if (!s.parameter())
					bug("Symbol::mktree - expected param %s in func %s",
							s.name(), name());

				if (s.offset() == NULL)
				{
					s.offset(new Integer(g_size_t_type->ic()));
					stkalloc.allocparam(s.type()->getsize(),
							s.type()->align() / g_min_addressable,
							*s.offset());
				}
			}

		// now figure out how big our local stack has to be
		if (_scope != NULL)
			_scope->calclocals(stkalloc);

		// calculate the appropriate sizes
		Integer localsize(g_size_t_type->ic());
		stkalloc.localsize(localsize);
		//localsize.div(*g_min_addr_size);

		// _node->mktree() will build declarations for local vars
		TreeNode *seg = switch_seg(NULL, SEG_CODE);
		TreeNode *body = (_node->nodetype() == N_COMPOUND) ?
				_node->mktree() : _node->mkinit();

		TreeNode *func = mkfunction(_name, g_return_label, retsize, localsize,
				ft->args() != NULL && ft->args()->size() > 0,
				FALSE, (uLong*)stkalloc.regmap(), FALSE, body);

		if (_storage != STATIC)
		{
			if (seg == NULL)
				seg = mkdeclglobal(_name);
			else
				seg = mklist(seg, mkdeclglobal(_name));
		}

		return (seg == NULL) ? func : mklist(seg, func);
	}		// end of function stuff

	// data var - global, static, extern, etc
	TreeNode *seg = NULL;
	TreeNode *var = NULL;

	if (_storage == STATIC)
	{
		Integer &size = _type->getsize();
		TreeNode *def = mkdefinition(mksymbolname(*this), size,
				_type->align() / g_min_addressable);

		if (_node != NULL)
		{
			seg = switch_seg(NULL, SEG_DATA);
			var = mklist(def, _node->mkinit());
		}
		else
		{
			seg = switch_seg(NULL, SEG_BSS);
			var = mklist(def, mkinit(size));
		}
	}
	else if (_storage == EXTERN)
	{
		if (!_read && !_wrote && !_referenced)
			return NULL;

		return switch_seg(mkdeclextern(mksymbolname(*this), _type->getsize(),
				TRUE), SEG_DATA, TRUE);
	}
	else if (_scope == g_file_scope)
	{
			char const *name = mksymbolname(*this);
		Integer &size = _type->getsize();

		TreeNode *decl = mkdeclglobal(name);
		TreeNode *def = mkdefinition(name, size,
				_type->align() / g_min_addressable);

		if (_node != NULL)
		{
			seg = switch_seg(NULL, SEG_DATA);
			var = mklist(decl, def, _node->mkinit(), NULL);
		}
		else
		{
			seg = switch_seg(NULL, SEG_BSS);
			var = mklist(decl, def, mkinit(size), NULL);
		}
	}
	else		// stack variable
	{
		TreeNode *decl;

		if (_regnum > 0)
			decl = mkregdef(mksymbolname(*this), _regnum - 1);
		else
		{
			if (_offset == NULL)
				bug("Symbol::mktree(): _offset for %s is NULL", _name);

			decl = mkstackdef(mksymbolname(*this), *_offset);
		}

		if (_node == NULL)
			return decl;

		TreeNode *tree;

		if (_type->issu() || _type->isarr())
			tree = _node->mkinit();
		else
		{
			LValueTreeNode *lsi;

			if (_regnum > 0)
				lsi = mkreglvalue(_regnum - 1, mksymbolname(*this),
						_type->isvolatile());
			else
				lsi = mkptrlvalue(mklocaladdr(mksymbolname(*this),
								g_size_t_type->ic().zero(), *_offset),
						_type->isvolatile());

			tree = _node->mktree();

			if (_type->isfloat())
				tree = mkcastvoid(mkfassign(lsi, tree, _type->getwidth()),
						_type->getwidth(), TRUE);
			  else
				tree = mkcastvoid(mkassign(lsi, tree, _type->getwidth()),
						_type->getwidth(), FALSE);
		}

		return mklist(decl, tree);
	}

	return (seg == NULL) ? var : mklist(seg, var);
}

// make trees from all expression and statement nodes...

TreeNode *
Node::mktree()
{
	bug("Node::mktree() should never be called for node %d", nodetype());
	return NULL;
}
// Expr_node::mktree() should never be called either

LValueTreeNode *
Expr_node::mklval()
{
	bug("Expr_node::mklval() should never be called for node %d", nodetype());
	return NULL;
}

TreeNode *
Node::mkinit()
{
	bug("Node::mkinit() should never be called for node %d", nodetype());
	return NULL;
}
// Expr_node::mkinit() should never be called either

TreeNode *
Integer_node::mktree()
{
	return _val != NULL ? mkconst(*_val) : NULL;
}

TreeNode *
Integer_node::mkinit()
{
	return _val != NULL ? mkinitconst(*_val) : NULL;
}

TreeNode *
Sizeof_node::mktree()
{
	return mkconst(_size);
}

TreeNode *
Sizeof_node::mkinit()
{
	return mkinitconst(_size);
}

TreeNode *
Float_node::mktree()
{
	return mkfconst(*_val);
}

TreeNode *
Float_node::mkinit()
{
	return mkinitfconst(*_val);
}

TreeNode *
String_node::mktree()
{
	if (_label == NULL)
	{
		// first use of this string makes a dummy name for it, and puts
		// in the global list so it can be dumped out later
		_label = mkasmlabel("S" ASMCHAR);
		g_string_nodes.end() = new String_node(*this);
	}

	return mkglobaladdr(_label, 0);
}

TreeNode *
String_node::mkinit()
{
	if (_wide)
		return mkinitwstring((short*)_string, _len + 1);
	else
		return mkinitstring(_string, _len + 1);
}

LValueTreeNode *
String_node::mklval()
{
	return mkptrlvalue(String_node::mktree(), FALSE);
}

LValueTreeNode *
Symbol_node::mklval()
{
	if (_sym == NULL || _sym->type()->isfunc() || _sym->storage() == ENUM)
		return NULL;

	// global
	if (_sym->scope() == g_file_scope || _sym->storage() == STATIC)
		return mkptrlvalue(mkglobaladdr(mksymbolname(*_sym), 0),
				_sym->type()->isvolatile());

	// register
	if (_sym->regnum() > 0)
		return mkreglvalue(_sym->regnum() - 1, mksymbolname(*_sym),
				_sym->type()->isvolatile());

	// local symbol - param or var
	return mkptrlvalue(mklocaladdr(mksymbolname(*_sym),
			g_size_t_type->ic().zero(), *_sym->offset()),
			_sym->type()->isvolatile());
}

TreeNode *
Symbol_node::mktree()
{
	if (_sym == NULL)
		return NULL;

	if (_sym->storage() == ENUM)
		return mkconst(*_sym->member()->enumval());

	if (_sym->type()->isfunc())
		return mkfunctionaddr(mksymbolname(*_sym));

	if (_sym->type()->isarr())
	{
		// global array
		if (_sym->scope() == g_file_scope || _sym->storage() == STATIC)
			return mkglobaladdr(mksymbolname(*_sym), 0);

		// local array
		return mklocaladdr(mksymbolname(*_sym),
				g_size_t_type->ic().zero(), *_sym->offset());
	}

	return mkderef(Symbol_node::mklval(), _type->getsize(),
			_type->isfloat());
}

TreeNode *
Symbol_node::mkinit()
{
	if (_sym == NULL)
		return NULL;

	if (_sym->storage() == ENUM)
		return mkinitconst(*_sym->member()->enumval());
	
	return mkinitptr(mksymbolname(*_sym), 0);
}

LValueTreeNode *
Array_ref_node::mklval()
{
	Type *t;

	if (_arr->type()->isptr())
		t = ((Pointer_type*)_arr->type())->type();
	else if (_arr->type()->isarr())
		t = ((Array_type*)_arr->type())->type();
	else
		bug("Array_ref_node::mklval: illegal type for _arr");

	return mkptrlvalue(mkpadd(_arr->mktree(), _expr->mktree(), g_size_pointer,
			t->getsize()), t->isvolatile());
}

TreeNode *
Array_ref_node::mktree()
{
	Type *t;

	if (_arr->type()->isptr())
		t = ((Pointer_type*)_arr->type())->type();
	else if (_arr->type()->isarr())
		t = ((Array_type*)_arr->type())->type();
	else
		bug("Array_ref_node::mklval: illegal type for _arr");

	if (t->isarr())
		return mkpadd(_arr->mktree(), _expr->mktree(), g_size_pointer,
				t->getsize());

	return mkderef(Array_ref_node::mklval(), t->getsize(), t->isfloat());
}

LValueTreeNode *
Func_call_node::mklval()
{
	return mkptrlvalue(Func_call_node::mktree(), _type->isvolatile());
}

TreeNode *
Func_call_node::mktree()
{
	Integer paramsize(g_size_t_type->ic());
	TreeNode *params = NULL;
	Type *t = _func->type();
	TreeNode *func;

	if (t->isptr())
	{
		func = mkderef(_func->mklval(), t->getsize(), FALSE);
			t = ((Pointer_type *)t)->type();
	}
	else
		func = _func->mktree();

	Function_type *f = (Function_type*)t;
	ParamAllocator palloc;

	if (f->typeval() != T_FUNCTION)
		bug("Func_call_node::mktree(): expected function type");

	if (_args != NULL)
	{
		params = mknulllist();

		for (int i = 0; i < _args->size(); i++)
		{
			Expr_node *n = _args->elt(i);
			Type *type = n->type();
			TreeNode *tree = NULL;

			switch (type->typeval())
			{
			case T_ARRAY:
				// convert array arguments into const pointer args
				type = ((Array_type*)type)->type();
				Pointer_type *tp = new Pointer_type(type);
				tp->isconst(TRUE);
				type = pool_type(tp);
				tp->free();
				break;

			case T_FUNCTION:
				// convert function arg into const pointer to func
				Pointer_type *fp = new Pointer_type(type);
				fp->isconst(TRUE);
				type = pool_type(fp);
				fp->free();
				break;

			case T_INT:
				if (f->args() == NULL &&
						((Int_type*)type)->width() < g_int_type->width())
					tree = do_cast(n, type, g_int_type);

				break;

			case T_FLOAT:
				if (f->args() == NULL &&
						((Float_type*)type)->width() < g_double_type->width())
					tree = do_cast(n, type, g_double_type);

				break;

			default:
				break;
			}

			if (tree == NULL)
				tree = n->mktree();

			Integer &size = type->getsize();
			palloc.allocparam(size, type->align() / g_min_addressable);
			params = mkpair(mkfuncparam(tree, size, type->isfloat()), params);
		}
	}

	palloc.paramsize(paramsize);
	return mkcall(f->retval()->getsize(), f->retval()->isfloat(),
			paramsize, params, func);
}

LValueTreeNode *
Struct_ref_node::mklval()
{
	if (_elem->offset() == NULL)				// for union entries
		return _structref->mklval();

	Integer elemoff = *_elem->offset();
	elemoff.div(*g_min_addr_size);

	if (_elem->type()->isbit())
		return mkbitfieldlvalue(_structref->mktree(),
				_elem->type()->isvolatile(), elemoff, 0,
				((Bitfield_type*)_elem->type())->width(),
				((Bitfield_type*)_elem->type())->sign());
	else
	{
		LValueTreeNode *lval = _structref->mklval();

		if (elemoff.isZero())
			return lval;

		TreeNode *ptr;
		boolean vol;
		lvalueptr(lval, ptr, vol);
		return mkptrlvalue(mkpadd(ptr, mkconst(elemoff),
				g_size_pointer, g_size_t_type->ic().one()), vol);
	}
}

TreeNode *
Struct_ref_node::mktree()
{
	Integer elemoff(g_size_t_type->ic());

	if (_elem->offset() != NULL)
		elemoff = *_elem->offset();

	elemoff.div(*g_min_addr_size);

	if (_elem->type()->isbit())
		return mkbitextract(_structref->mktree(),
				_structref->type()->getsize(), elemoff, 0,
				((Bitfield_type*)_elem->type())->width(),
				((Bitfield_type*)_elem->type())->sign());
	else
	{
		if (_elem->type()->isarr())
		{
			TreeNode *ptr;
			boolean vol;
			lvalueptr(_structref->mklval(), ptr, vol);
			return mkpadd(ptr, mkconst(elemoff),
					g_size_pointer, g_size_t_type->ic().one());
		}

		if (elemoff.isZero() &&
				_structref->type()->getsize() == _elem->type()->getsize()) 
			return _structref->mktree();

		return mkextract(_structref->mktree(), _structref->type()->getsize(),
				_elem->type()->getsize(), elemoff, _elem->type()->isfloat());
	}
}

TreeNode *
Struct_ref_node::mkinit()
{
	return mktree();
}

LValueTreeNode *
Operator_node::mklval()
{
	boolean fp = _arg->type()->isfloat();
	TreeNode *ret = NULL;

	switch (_token)
	{
	case '*':
		ret = _arg->mktree();
		break;

	// not strictly lvalues, but could appear embedded within an lvalue
	case '&':
		return _arg->mklval();
		break;

	case INCR:
		if (_prefix)
		{
			if (fp)
				ret = mkfpreinc(_arg->mklval(), _arg->type()->getwidth());
			else if (_arg->type()->isptr())
				ret = mkppreinc(_arg->mklval(),
						((Pointer_type*)_arg->type())->type()->getsize());
			else
				ret = mkpreinc(_arg->mklval(), _arg->type()->getwidth());
		}
		else
		{
			if (fp)
				ret = mkfpostinc(_arg->mklval(), _arg->type()->getwidth());
			else if (_arg->type()->isptr())
				ret = mkppostinc(_arg->mklval(),
						((Pointer_type*)_arg->type())->type()->getsize());
			else
				ret = mkpostinc(_arg->mklval(), _arg->type()->getwidth());
		}

		break;

	case DECR:
		if (_prefix)
		{
			if (fp)
				ret = mkfpredec(_arg->mklval(), _arg->type()->getwidth());
			else if (_arg->type()->isptr())
				ret = mkppredec(_arg->mklval(),
						((Pointer_type*)_arg->type())->type()->getsize());
			else
				ret = mkpredec(_arg->mklval(), _arg->type()->getwidth());
		}
		else
		{
			if (fp)
				ret = mkfpostdec(_arg->mklval(), _arg->type()->getwidth());
			else if (_arg->type()->isptr())
				ret = mkppostdec(_arg->mklval(),
						((Pointer_type*)_arg->type())->type()->getsize());
			else
				ret = mkpostdec(_arg->mklval(), _arg->type()->getwidth());
		}

		break;

	default:
		bug("Operator_node::mklval(): illegal token %d", _token);
		break;
	}

	return mkptrlvalue(ret, _arg->type()->isvolatile());
}

TreeNode *
Operator_node::mktree()
{
	boolean fp = _arg->type()->isfloat();

	switch (_token)
	{
	case INCR:
		if (_prefix)
		{
			if (fp)
				return mkfpreinc(_arg->mklval(), _arg->type()->getwidth());
			else if (_arg->type()->isptr())
				return mkppreinc(_arg->mklval(),
						((Pointer_type*)_arg->type())->type()->getsize());
			else
				return mkpreinc(_arg->mklval(), _arg->type()->getwidth());
		}
		else
		{
			if (fp)
				return mkfpostinc(_arg->mklval(), _arg->type()->getwidth());
			else if (_arg->type()->isptr())
				return mkppostinc(_arg->mklval(),
						((Pointer_type*)_arg->type())->type()->getsize());
			else
				return mkpostinc(_arg->mklval(), _arg->type()->getwidth());
		}

		break;

	case DECR:
		if (_prefix)
		{
			if (fp)
				return mkfpredec(_arg->mklval(), _arg->type()->getwidth());
			else if (_arg->type()->isptr())
				return mkppredec(_arg->mklval(),
						((Pointer_type*)_arg->type())->type()->getsize());
			else
				return mkpredec(_arg->mklval(), _arg->type()->getwidth());
		}
		else
		{
			if (fp)
				return mkfpostdec(_arg->mklval(), _arg->type()->getwidth());
			else if (_arg->type()->isptr())
				return mkppostdec(_arg->mklval(),
						((Pointer_type*)_arg->type())->type()->getsize());
			else
				return mkpostdec(_arg->mklval(), _arg->type()->getwidth());
		}

		break;

	case '&':
		if (_arg->nodetype() == N_OPERATOR &&
				((Operator_node*)_arg)->token() == '*')
			return ((Operator_node*)_arg)->arg()->mktree();

		TreeNode *ptr;
		boolean v;
		lvalueptr(_arg->mklval(), ptr, v);
		return ptr;
		break;

	case '*':
		if (_arg->nodetype() == N_OPERATOR &&
				((Operator_node*)_arg)->token() == '&')
			return ((Operator_node*)_arg)->arg()->mktree();

		if (_type->isfunc())
			return _arg->mktree();

		return mkderef(Operator_node::mklval(), _type->getsize(),
				_type->isfloat());
		break;

	case '+':
		return _arg->mktree();
		break;

	case '-':
		if (fp)
			return mkfneg(_arg->mktree(), _arg->type()->getwidth());
		else
			return mkneg(_arg->mktree(), _arg->type()->getwidth());

		break;

	case '~':
		return mkcompl(_arg->mktree(), _arg->type()->getwidth());
		break;

	case '!':
		if (fp)
			return mkflogneg(_arg->mktree(), _arg->type()->getwidth());
		else
			return mklogneg(_arg->mktree(), _arg->type()->getwidth());

		break;

	default:
		bug("Operator_node::mktree() illegal token %d", _token);
		break;
	}

	return NULL;
}

TreeNode *
Operator_node::mkinit()
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
New_node::mktree()
{
	return NULL;
}

TreeNode *
Delete_node::mktree()
{
	return NULL;
}
