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
// $Header: /u/cgt/cvs/bin/cc/cctype.cc,v 1.56 2003/04/29 21:56:09 parag Exp $

#if defined(__GNUC__) && !defined(__APPLE__)
#pragma implementation
#endif

#include "cpp.h"
#include "ccsym.h"
#include "cctype.h"
#include "cc.h"

int Type::hashlen() { return sizeof *this; }

void
Type::free()
{
	if (_inpool)
		return;

	delete _size;
	delete this;
}

Type *Type::dup() { return new Type(*this); }
void Type::size(Integer &s) { s = s.integerclass().zero(); }

int
Type::align()
{
	bug("Type::align() for type %d should never be called", _typeval);
	return 0;
}

int
Type::getwidth()
{
	bug("Type::getwidth() for type %d should never be called", _typeval);
	return 0;
}

boolean Type::complete() { return FALSE; }

// return the size in min-addressible quantities instead of bytes
Integer &
Type::dosize()
{
	if (_size == NULL)
	{
		_size = new Integer(g_size_t_type->ic());
		size(*_size);		// call protected virtual size()
		_size->div(*g_min_addr_size);
	}

	return *_size;
}

boolean
Type::operator==(Type &t) 
{
	return typeval() == t.typeval() &&
			isconst() == t.isconst() && isvolatile() == t.isvolatile();
}

// default type hash is pretty straight-forward - we do not hash _size
// as it will be set later when we calculate the actual sizes of objects
//
unsigned long
Type::hash()
{
	unsigned long hash = 0;
	char *ptr = (char *)&_typeval;
	int len = hashlen() - sizeof _size;

	for (int i = 0; i < len; i++)
		hash = (hash << 3) + *ptr++;

	return hash ^ (hash >> 16);
}

Type *Void_type::dup() { return new Void_type(*this); }
int Void_type::hashlen() { return sizeof *this; }

Type *Int_type::dup() { return new Int_type(*this); }

void
Int_type::size(Integer &s)
{
	Integer t(s.integerclass(), _ic->bits());
	s = t;
}

int Int_type::hashlen() { return sizeof *this; }

// Int_type hash must avoid hashing its IntegerClass pointer
unsigned long
Int_type::hash() 
{
	unsigned long hash = 0;
	char *ptr = (char *)&_typeval;

	// hash everything upto the IntegerClass
	while (ptr < (char *)&_ic)
		hash = (hash << 3) + *ptr++;

	hash += _ic->bits();		// and hash the size of this IntegerClass
	return hash ^ (hash >> 16);
}

boolean
Int_type::operator==(Type &t)
{
	if (t.typeval() != typeval()
			|| isconst() != t.isconst() || isvolatile() != t.isvolatile())
		return FALSE;

	Int_type *i = (Int_type*)&t;
	return _ic->bits() == i->_ic->bits() && _sign == i->_sign;
}

int
Int_type::align()
{
	int width = _ic->bits();

	if (width <= g_size_char)
		return g_align_char;
	else if (width <= g_size_short)
		return g_align_short;
	else if (width <= g_size_int)
		return g_align_int;
	else if (width <= g_size_long)
		return g_align_long;

	//else if (width <= g_size_longlong)
	return g_align_longlong;
}

int Int_type::getwidth() { return _ic->bits(); }

Type *Float_type::dup() { return new Float_type(*this); }
int Float_type::hashlen() { return sizeof *this; }

// carefully avoid hashing the FloatClass pointer
unsigned long
Float_type::hash() 
{
	unsigned long hash = 0;
	char *ptr = (char *)&_typeval;

	// hash everything upto the FloatClass
	while (ptr < (char *)&_fc)
		hash = (hash << 3) + *ptr++;

	// and hash the size of this FloatClass
	hash +=  (_fc->mantissa() << 16) + _fc->exponent();
	return hash ^ (hash >> 16);
}

void
Float_type::size(Integer &s)
{
	Integer t(s.integerclass(), _fc->bits());
	s = t;
}

boolean
Float_type::operator==(Type &t)
{
	if (t.typeval() != typeval()
			|| isconst() != t.isconst() || isvolatile() != t.isvolatile())
		return FALSE;

	Float_type *f = (Float_type*)&t;
	return _fc->bits() == f->_fc->bits();
}

int
Float_type::align()
{
	int width = _fc->bits();

	if (width <= g_size_float_mantissa + g_size_float_exponent)
		return g_align_float;
	else if (width <= g_size_double_mantissa + g_size_double_exponent)
		return g_align_double;

	//else longdouble
	return g_align_longdouble;
}

int Float_type::getwidth() { return _fc->bits(); }

// this constructor makes gcc 2.3.3 croak if they are made inline
Array_type::Array_type(Array_type &a) : Type(a),
		_len(g_size_t_type->ic(), a._len)
	{ _type = a._type; _complete = a._complete; }
Array_type::Array_type(Type *t, Integer &l, boolean c) :
		Type(T_ARRAY), _len(g_size_t_type->ic(), l)
	{ _type = t; _complete = c; }

void
Array_type::free()
{
	if (_inpool)
		return;

	if (_type != NULL)
		_type->free();

	delete _size;
	delete this;
}

Type *Array_type::dup() { return new Array_type(*this); }
int Array_type::hashlen() { return sizeof *this; }
void Array_type::size(Integer &s) { _type->size(s); s *= _len; }
int Array_type::align() { return _type->align(); }
boolean Array_type::complete()
		{ return _complete && _type && _type->complete(); }

boolean
Array_type::operator==(Type &t)
{
	if (t.typeval() != typeval()
			|| isconst() != t.isconst() || isvolatile() != t.isvolatile())
		return FALSE;

	Array_type *a = (Array_type*)&t;
	return *_type == *a->_type && _len == a->_len;
}

void
Pointer_type::free()
{
	if (_inpool)
		return;

	if (_type != NULL)
		_type->free();

	delete _size;
	delete this;
}

Type *Pointer_type::dup() { return new Pointer_type(*this); }
int Pointer_type::hashlen() { return sizeof *this; }

void
Pointer_type::size(Integer &s)
{
	Integer t(s.integerclass(), g_size_pointer);
	s = t;
}

int Pointer_type::align() { return g_align_pointer; }
int Pointer_type::getwidth() { return g_size_pointer; }

boolean
Pointer_type::operator==(Type &t)
{
	if (t.typeval() != typeval()
			|| isconst() != t.isconst() || isvolatile() != t.isvolatile())
		return FALSE;

	Pointer_type *p = (Pointer_type*)&t;
	return *_type == *p->_type;
}

void
Struct_info::free()
{
	if (g_cplusplus)
		delete _scope;
	else
		delete _members;

	if (_has_virtual)
		delete _virtlist;
	else
		delete _next;

	delete _memlist;

	if (_base)
		_base->free();

	delete this;
}

Struct_info::Struct_info()
{
	_ref = 1;
	_symbol = NULL;
	_complete = FALSE;
	_memlist = NULL;
	_virtlist = NULL;		// also _next
	_align = 0;
	_is_class = FALSE;
	_has_virtual = FALSE;
	_has_constructor = FALSE;
	_has_destructor = FALSE;
	_has_operator = FALSE;
	_base = NULL;

	if (g_cplusplus)
	{
		_scope = new Scope(g_cpp.filename(), g_cpp.linenum(), g_curr_scope);
		_members = &_scope->symtab();
	}
	else
	{
		_scope = g_curr_scope;
		_members = new Symbol_tab;
	}
}

int
Struct_info::do_align()
{
	if (_align > 0)
		return _align;

	if (_memlist != NULL)
		for (int i = 0; i < _memlist->size(); i++)
		{
			// struct alignment is the max alignment of all its elements
			int a = _memlist->elt(i)->type()->align();

			if (a > _align)
				_align = a;
		}

	if (_has_virtual && _align < g_align_pointer)
		_align = g_align_pointer;

	return _align;
}

void
Struct_info::size(Integer &retsize)
{
	if (!_complete)
	{
		error(CANNOT_GET_SIZE_OF_INCOMPLETE);
		return;
	}

	if (_memlist == NULL)		// completely empty struct
		return;

	// calculate the correct size, with padding, alignment, & bitfields
	Integer salign(g_size_t_type->ic(), align());
	Integer pad(salign);
	Symbol **mem = _memlist->getarr();
	boolean prevbit = FALSE;
	Integer bitsize(g_size_t_type->ic());
	int i, p;

	for (i = 0, p = 0; i < _memlist->size(); p = i++)
	{
	#ifdef DBG
		fprintf(stderr, "field=%s", mem[i]->name());
	#endif
		Type &t = *mem[i]->type();
		int ta = t.align();		// to check for and pack bitfields
		Integer a(g_size_t_type->ic(), ta);
		Integer s(g_size_t_type->ic());
		boolean isbit = t.isbit();
		t.size(s);				// need size in bits
		boolean iszero = s.isZero();

	#ifdef DBG
		fprintf(stderr, " a=%s", integertostr(a));
		fprintf(stderr, " s=%s", integertostr(s));
	#endif

		if ((!isbit && (prevbit || a > pad)) ||
				(!prevbit && isbit && bitsize > pad))
		{
			retsize += pad;

			// save the amount of padding to pack initializers
			if (i && mem[p]->pad() == NULL && (isbit || !prevbit))
				mem[p]->pad(new Integer(pad));

			pad = salign;
		}
		else if ((!isbit && a != pad) || iszero)
		{
			// bitfield :0 means pad out this word and go to the next word
			Integer mod(pad);
			mod %= a;
		#ifdef DBG
			fprintf(stderr, " mod=%s", integertostr(mod));
		#endif
			retsize += mod;

			// save the amount of padding to pack initializers
			if (!iszero && i && mem[p]->pad() == NULL)
				mem[p]->pad(new Integer(mod));

			pad -= mod;
		}

	#ifdef DBG
		fprintf(stderr, " retsize=%s", integertostr(retsize));
		fprintf(stderr, " pad=%s", integertostr(pad));
	#endif

		// the offset for this member is the current position in this struct
		if (!iszero && mem[i]->offset() == NULL)
		{
			mem[i]->offset(new Integer(retsize));

			if (i && mem[p]->pad() != NULL)
			{
				mem[p]->pad()->div(*g_min_addr_size);

				if (mem[p]->pad()->isZero())
				{
					delete mem[p]->pad();
					mem[p]->pad(NULL);
				}
			}
		}

		if (isbit)
		{
			Integer t(g_size_t_type->ic(),
					((Bitfield_type*)mem[i]->type())->ic().bits());

			// different size, so pad it out the next time around
			if (bitsize == t)
				prevbit = FALSE;
			else
			{
				// same size word for bitfield, so try to squeeze it in
				prevbit = TRUE;
				bitsize = t;
			}
		}
		else
			prevbit = FALSE;

		retsize += s;
		pad -= s;

		while (!pad.isPos())
			pad += salign;

	#ifdef DBG
		fprintf(stderr, " retsize=%s", integertostr(retsize));
		fprintf(stderr, " pad=%s", integertostr(pad));
		fprintf(stderr, "\n");
	#endif
	}

	if (pad < salign)
	{
		retsize += pad;

		// save the amount of padding to pack initializers
		if (mem[p]->pad() == NULL)
		{
			mem[p]->pad(new Integer(pad));
			mem[p]->pad()->div(*g_min_addr_size);

			if (mem[p]->pad()->isZero())
			{
				delete mem[p]->pad();
				mem[p]->pad(NULL);
			}
		}
	}

	// now deal with offsets for any unnamed substruct fields
	Symbol **ma = memlist()->getarr();

	for (i = 0; i < memlist()->size(); i++)
		if (ma[i]->unnamed() != NULL && ma[i]->offset() == NULL)
		{
			//Symbol &m = mi();
			Symbol &m = *ma[i];
			Symbol &um = *m.unnamed();
			Symbol &sm = (*((Struct_type*)um.type())->members())[m.name()];

			if (um.offset() == NULL)
				bug("Struct_type::size(): missing substruct offset");

			// calculate offset of this unnamed substruct field
			m.offset(new Integer(*um.offset()));

			if (sm.offset() != NULL)		// will be NULL for unions
				m.offset()->add(*sm.offset());
		}
}

Type *Struct_type::dup() { return new Struct_type(*this); }

// hash struct info name and not its pointer
unsigned long
Struct_type::hash()
{
	unsigned long hash = 0;
	char *ptr = (char *)&_typeval;

	// hash everything upto the info
	while (ptr < (char *)&_info)
		hash = (hash << 3) + *ptr++;

	hash += ptrhash(_info->name());

	for (Scope *s = _info->scope(); s != NULL; s = s->parent())
		if (s->symbol() != NULL)
			hash += ptrhash(s->symbol()->name());

	return hash ^ (hash >> 16);
}

void
Struct_type::free()
{
	if (_inpool)
		return;

	_info->unref();
	delete this;
}

boolean Struct_type::complete() { return _info->complete(); }

int
Struct_type::align()
{
	return _info->align();
}

void
Struct_type::size(Integer &retsize)
{
	_info->size(retsize);
}

boolean
Struct_type::operator==(Type &t)
{
	if (t.typeval() != typeval() ||
			isconst() != t.isconst() || isvolatile() != t.isvolatile())
		return FALSE;

	Struct_type *s = (Struct_type*)&t;
	//return s->name() == name();
	return s->_info == _info || (s->_info->name() == _info->name() &&
			s->_info->scope() == _info->scope());
}

Symbol *
Struct_type::add(char const *name, Type *type, int line,
		int storage, int access, boolean isinline, Symbol *unnamed)
{
	Symbol *sym;

	if (_info->memlist() == NULL)
		_info->alloc_memlist();

	if (members()->find(name))
	{
		sym = &members()->cur();

		if (!g_cplusplus || !sym->type()->isanyfunc() || !type->isfunc())
			error(FIELD_ALREADY_IN_STRUCT, name, sym->line());

		Symbol *s = sym->add_overload(type);

		if (s == NULL)
			error(FIELD_ALREADY_IN_STRUCT, name, sym->line());
		else
			sym = s;
	}
	else
	{
		sym = &(*members())[name];
		sym->type(type);
		sym->line(line);

		// unnamed substruct entries are not added to our memlist, as the
		// actual substruct should already be entered with a bogus name
		if (unnamed != NULL)
			sym->unnamed(unnamed);
		else if (!type->isanyfunc())
			memlist()->end() = sym;

		// if this element is an unnamed substruct, add its elements to
		// our _members list
		if (name[0] == UNNAMED_CHAR && type->issu())
		{
			Struct_type *substruct = (Struct_type*)type;
			Symbol **mem = substruct->memlist()->getarr();

			// then add each of its members, each pointing to its rightful owner
			// - this should recursively add subsubstructs and so on
			for (int i = 0; i < substruct->memlist()->size(); i++)
			{
				Symbol *m = mem[i];

				add(m->name(), m->type(), m->line(), m->storage(),
						m->access(), m->is_inline(), sym);
			}
		}
	}

	sym->storage(storage);
	sym->access(access);
	sym->is_inline(isinline);

#ifdef VIRTUAL
	if (storage == VIRTUAL)
	{
		if (_info->virtlist() == NULL)
			_info->alloc_virtlist();
		
		_info->virtlist()->end() = sym;
		_info->has_virtual(TRUE);
	}
#endif

	return sym;
}

void
Struct_type::add_friend(Symbol *sym)
{
	if (_info->friends() == NULL)
		_info->alloc_friends();
	
	_info->friends()->end() = sym;
}

void
Struct_type::add_base_class(Type *base, int access, boolean /*isvirtual*/)
{
	if (!base->isstruct())
	{
		error(CAN_ONLY_INHERIT_FROM_CLASS);
		return;
	}

	Struct_type *bt = (Struct_type*)base;
	_info->base(bt);
	Symbol **mem = bt->memlist()->getarr();

	// add each of baseclass members into our symtab
	for (int i = 0; i < bt->memlist()->size(); i++)
	{
		Symbol *m = mem[i];

#ifdef PRIVATE
		if (m->access() == PRIVATE ||
				(!m->type()->isanyfunc() && members()->find(m->name())))
			continue;

		add(m->name(), m->type(), m->line(), m->storage(),
				access == PRIVATE ? PRIVATE : m->access(),
				m->is_inline(), NULL);
#else
		add(m->name(), m->type(), m->line(), m->storage(),
				m->access(), m->is_inline(), NULL);
#endif
	}

	// now add all member functions from the base class into ourselves
	for (Symbol_iter mi = *bt->members(); mi; ++mi)
		if (mi().type()->isanyfunc())
		{
			Symbol *m = &mi();

#ifdef PRIVATE
			if (m->access() == PRIVATE ||
					(!m->type()->isanyfunc() && members()->find(m->name())))
				continue;

			add(m->name(), m->type(), m->line(), m->storage(),
					access == PRIVATE ? PRIVATE : m->access(),
					m->is_inline(), NULL);
#else
			add(m->name(), m->type(), m->line(), m->storage(),
					m->access(), m->is_inline(), NULL);
#endif
		}
}

void
Union_type::size(Integer &retsize)
{
	if (!complete())
	{
		error(CANNOT_GET_SIZE_OF_INCOMPLETE);
		return;
	}

	// union size is the max size of all of its elements
	for (int i = 0; i < memlist()->size(); i++)
	{
		Integer s(g_size_t_type->ic());

		// inside a union, bitfield size is size of its base type
		if (memlist()->elt(i)->type()->isbit())
		{
			Integer t(g_size_t_type->ic(),
					((Bitfield_type*)memlist()->elt(i)->type())->ic().bits());
			s = t;
		}
		else
			memlist()->elt(i)->type()->size(s);  // need size in bits

		if (s > retsize)
			retsize = s;
	}
}

//Type *Enum_type::dup() { }

//void Enum_type::free() { }

void
Enum_type::size(Integer &s)
{
	Integer t(s.integerclass(), g_int_type->width());
	s = t;
}

int Enum_type::align() { return g_align_int; }
int Enum_type::getwidth() { return g_int_type->width(); }

void
Enum_type::add(char const *name, const char *file, int line)
{
	if (next() == NULL)
		next(new Integer(g_int_type->ic()));

	add(name, file, line, *next());
}

void
Enum_type::add(char const *name, const char *file, int line, Integer &id)
{
	if (next() == NULL)
		next(new Integer(g_int_type->ic()));

	*next() = id;
	Symbol &member = (*members())[name];
	member.line(line);
	member.type(this);		// back-link from member to enum

	// attempt to re-define enum name?
	if (member.enumval() != NULL)
		error(ENUM_ALREADY_DEFINED, name, member.line());
	else
		member.enumval(new Integer(*next()));

	next()->add(next()->integerclass().one());		// bump enum val count
	Scope *scp = g_cplusplus ? scope()->parent() : scope();

	// stick this enum name into the parent scope's symbol table
	if (scp->symtab().find(name))
	{
		Symbol &s = scp->symtab()();

		if (s.storage() != ENUM || s.member() != &member)
			error(ENUM_ALREADY_DEFINED, name, s.line());
	}
	else
	{
		scp->symtab().insert(name);
		Symbol &s = scp->symtab()();
		s.type(g_int_type);
		s.file(file);
		s.line(line);
		s.storage(ENUM);		// marks this symbol as an enum entry
		s.member(&member);
		//s.scope(g_curr_scope);
		s.scope(scope());
	}
}

Type *Bitfield_type::dup() { return new Bitfield_type(*this); }
int Bitfield_type::align() { return Int_type::align(); }
int Bitfield_type::getwidth() { return _width; }

void
Bitfield_type::size(Integer &s)
{
	Integer t(s.integerclass(), _width);
	s = t;
}

int Bitfield_type::hashlen() { return sizeof *this; }
unsigned long Bitfield_type::hash() { return Int_type::hash() + _width; }

// this is defined for the typepool - use equivalent() for real checks
boolean
Function_type::operator==(Type &t)
{
	if (t.typeval() != typeval()
			|| isconst() != t.isconst() || isvolatile() != t.isvolatile())
		return FALSE;

	Function_type *f = (Function_type*)&t;

	if ((_classname && f->_classname &&
			_classname->name() != f->_classname->name()) ||
			(_classname && !f->_classname) ||
			(!_classname && f->_classname))
		return FALSE;

	if (*_retval != *f->_retval ||
			_retref != f->_retref || _varargs != f->_varargs)
		return FALSE;

	if (_args == NULL && f->_args == NULL)
		return TRUE;

	if (_args == NULL || f->_args == NULL || _args->size() != f->_args->size())
		return FALSE;

	// note that this compares parameter types - not their names
	return compare_args(_args, f->_args);
}

void
Function_type::free()
{
	if (_inpool)
		return;

	if (_retval != NULL)
		_retval->free();

	if (_args != NULL)
	{
		for (int i = 0; i < _args->size(); i++)
			if (_args->elt(i).type() != NULL)
				_args->elt(i).type()->free();

		delete _args;
	}

	delete this;
}

Function_type::Function_type(Function_type &f) : Type(f)
{
	_classname = f._classname;
	_retref = f._retref;
	_varargs = f._varargs;
	_retval = f._retval->dup();

	if (f._args != NULL)
	{
		_args = new Param_arr(*f._args);

		for (int i = 0; i < f._args->size(); i++)
			_args->elt(i).type(f._args->elt(i).type()->dup());
	}
}

Type *Function_type::dup()
{
	if (_inpool)
		return this;

	return new Function_type(*this);
}


// hash function args based solely upon their types and not their names
unsigned long
Function_type::hash() 
{
	unsigned long hash = 0;
	char *ptr = (char *)&_typeval;

	// hash everything upto the args
	while (ptr < (char *)&_args)
		hash = (hash << 3) + *ptr++;

	// now hash each arg
	if (_args != NULL)
	{
		if (_args->size() > 0)
			for (int i = 0; i < _args->size(); i++)
				hash += _args->elt(i).type()->hash();
		else
			hash += g_void_type->hash();
	}

	return hash ^ (hash >> 16);
}

Type *Overload_type::dup() { return new Overload_type(*this); }
int Overload_type::hashlen() { return sizeof *this; }

boolean
Overload_type::operator==(Type &t)
{
	if (t.typeval() != typeval()
			|| isconst() != t.isconst() || isvolatile() != t.isvolatile())
		return FALSE;

	Overload_type *o = (Overload_type*)&t;
	return _sym == o->_sym;
}
