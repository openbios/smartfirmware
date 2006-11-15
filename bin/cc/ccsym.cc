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
// $Header: /u/cgt/cvs/bin/cc/ccsym.cc,v 1.33 2003/04/29 21:56:09 parag Exp $

#if defined(__GNUC__) && !defined(__APPLE__)
#pragma implementation
#endif

#include <stddef.h>
#include <stdio.h>
#include <memory.h>
#include <ctype.h>
#include <stringx.h>
#include "cpp.h"
#include "cctype.h"
#include "ccsym.h"
#include "ccnode.h"
#include "globals.h"
#include "fileuse.h"

/******
	Symbol()
		{ _name = NULL; _type = NULL; _scope = NULL; _node = NULL;
				_storage = 0; _read = _wrote = 0; _referenced = FALSE; 
				_parameter = FALSE; _treedone = FALSE; _offset = NULL;
				_regnum = 0; _file = NULL; _line = 0; }
	Symbol(const char *n)
		{ _name = n; _type = NULL; _scope = NULL; _node = NULL;
				_storage = 0; _read = _wrote = 0; _referenced = FALSE; 
				_parameter = FALSE; _treedone = FALSE; _offset = NULL;
				_regnum = 0; _file = NULL; _line = 0; }
******/

Symbol::Symbol()
{
	memset((void*)this, 0, sizeof (*this));
}

Symbol::Symbol(Symbol &s)
{
	memcpy((void*)this, (void*)&s, sizeof (*this));
}

Symbol::Symbol(const char *n)
{
	memset((void*)this, 0, sizeof (*this));
	_name = n;
}

Symbol::~Symbol()
{
	if (_overloaded)
		delete _overloads;
	else
		delete _node;
}

long
Symbol::cost_overload_match(Expr_arr *a)
{
	if (!_type->isfunc())
		bug("Symbol::cost_overload_match(): expected T_FUNCTION here");

	Function_type *ft = (Function_type*)_type;
	long cost = 0;

	// check arguments of args all match, or auto-cast to types that match
	// while handling "..." properly
	Param_arr *args = ft->args();
	int i = 0;
	int asize = (a == NULL) ? 0 : a->size();

	if (args != NULL)		// no args means anything goes
	{
		for (i = 0; i < args->size(); i++)
		{
			// handle any errors that occurred while parsing the argument
			if (i < asize && a->elt(i)->type()->typeval() == T_UNKNOWN)
				break;

			// we are out of arguments and this parameter has no default arg
			if (i >= asize && args->elt(i).init() == NULL)
				return -1;

			if (args->elt(i).type() == NULL)
				bug("Func_call_node::Func_call_node(): NULL arg[%d] type", i);

			// this argument must cast to the parameter type
			if (i < asize && !valid_ecast(a->elt(i), args->elt(i).type()))
				return -1;

			// not an exact match, so the cost is higher for the cast
			if (i < asize && a->elt(i)->type() != args->elt(i).type())
				cost += 100;
			else
				// exact match or default arg - same either way
				cost++;
		}
	}

	// any leftover args at this point mean function had better take varargs
	if (i < asize)
	{
		if (ft->varargs())
			cost += 10000;
		else
			return -1;
	}

	return cost;
}

// follow argument-matching rules to return best matching overloaded
// function or NULL if no match
Symbol *
Symbol::overload_match(Expr_arr *args)
{
	if (!_overloaded)
		return this;

	Symbol *ret = NULL;
	int i;
	long cost = 0;

	for (i = 0; i < _overloads->size(); i++)
	{
		long c = _overloads->elt(i)->cost_overload_match(args);

		if (c < 0)
			continue;
		else if (cost == 0 || c < cost)
		{
			cost = c;
			ret = _overloads->elt(i);
		}
		else if (c == cost)
			return NULL;
	}

	return ret;
}

Symbol *
Symbol::exact_match(Type *type)
{
	if (!_overloaded)
		return (_type == type || *_type == *type) ? this : (Symbol*)NULL;

	Symbol *ret = NULL;
	int count = 0;

	for (int i = 0; i < _overloads->size(); i++)
		if (_overloads->elt(i)->type() == type ||
				*_overloads->elt(i)->type() == *type)
		{
			ret = _overloads->elt(i);
			count++;
		}

	return count == 1 ? ret : (Symbol*)NULL;
}

Symbol *
Symbol::add_overload(Type *type)
{
	Symbol *sym;

	if (!g_cplusplus || (_type && !_type->isanyfunc()) || !type->isfunc())
		return NULL;

	if (!_overloaded)
	{
		if (equivalent(_type, type))
			return this;

		sym = new Symbol(*this);
		_overloads = new Symbol_arr;
		_overloads->end() = sym;
		Type *t = new Overload_type(this);
		_type = pool_type(t);
		t->free();
		_overloaded = TRUE;
	}

	sym = exact_match(type);

	if (sym != NULL)
		return sym;

	sym = new Symbol(*this);
	_overloads->end() = sym;
	sym->_type = type;
	sym->_node = NULL;
	return sym;
}

void
Symbol::cleanup_labels()
{
	if (_labels && _labels->size() == 0)
	{
		delete _labels;
		_labels = NULL;
	}
}

Scope::Scope(const char *fname, int fline, Scope *parent)
{
	_symtab = new Symbol_tab;
	_suetypes = new Symbol_tab;
	_fname = fname;
	_fline = fline;
	_parent = parent;
	_children = NULL;
	_init = NULL;
	_fini = NULL;
	_symbol = NULL;
	_node = NULL;
	_ctor = NULL;
	_id = 0;

	if (parent == NULL)
		return;

	if (parent->children() == NULL)
		parent->new_children();

	parent->children()->end() = this;
}

Scope::~Scope()
{
	delete _node;
	delete _symtab;
	delete _suetypes;
	delete _init;
	delete _children;
}

// free up any empty (unused) symbol tables from this scope
void
Scope::cleanup()
{
	if (_symtab && _symtab->size() == 0)
	{
		delete _symtab;
		_symtab = NULL;
	}

	if (_suetypes && _suetypes->size() == 0)
	{
		delete _suetypes;
		_suetypes = NULL;
	}

	if (_init && _init->size() == 0)
	{
		delete _init;
		_init = NULL;
	}

	if (_children && _children->size() == 0)
	{
		delete _children;
		_children = NULL;
	}
}

// look for a symbol in this scope, searching backwards through its parents
Symbol *
Scope::lookup_sym(const char *name)
{
	for (Scope *scope = this; scope != NULL; scope = scope->parent())
	{
		// we do not need to lookup baseclass scopes explicity as all
		// baseclass members are always copied into derived classes

		if (scope->has_symtab() && scope->symtab().find(name))
		{
			Symbol *sym = &scope->symtab()();

			if (g_verbose)
			{
				bump_file_use(sym->file(), g_cpp.filename());

				// this may also reference the struct type of this sym
				if (sym->type() && sym->type()->issue())
					bump_file_use(((Struct_type*)sym->type())->symbol()->file(),
							g_cpp.filename());
			}

			return sym;
		}
	}

	return NULL;
}

// look for an sue in this scope, searching backwards through its parents
Symbol *
Scope::lookup_sue(const char *name)
{
	for (Scope *scope = this; scope != NULL; scope = scope->parent())
		if (scope->has_suetypes() && scope->suetypes().find(name))
		{
			Symbol *sym = &scope->suetypes()();

			if (g_verbose)
				bump_file_use(sym->file(), g_cpp.filename());

			return sym;
		}

	return NULL;
}

// look for a class member in this scope, which ought to be a class scope
Symbol *
Scope::lookup_member(const char *name)
{
	if (_symbol == NULL || !_symbol->type()->isclass())
		return NULL;

	Scope *scope = ((Struct_type*)_symbol->type())->scope();

	if (scope && scope->has_symtab() && scope->symtab().find(name))
	{
		Symbol *sym = &scope->symtab()();

		if (g_verbose)
		{
			// mark this use of this member
			bump_file_use(sym->file(), g_cpp.filename());

			// mark this use of this class
			if (scope->symbol())
				bump_file_use(scope->symbol()->file(), g_cpp.filename());

			// this may also reference the struct type of this sym
			if (sym->type() && sym->type()->issue())
				bump_file_use(((Struct_type*)sym->type())->symbol()->file(),
						g_cpp.filename());
		}

		return sym;
	}

	return NULL;
}


// this is so that the __FUNC__ macro works from within Cpp
const char *
get_current_func()
{
	return g_func_scope ? g_func_scope->symbol()->name() : "";
}
