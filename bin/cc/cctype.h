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
// $Header: /u/cgt/cvs/bin/cc/cctype.h,v 1.47 2003/04/29 21:56:09 parag Exp $
#ifndef __CCTYPES_H_
#define __CCTYPES_H_

#if defined(__GNUC__) && !defined(__APPLE__)
#pragma interface
#endif

#include <memory.h>
#include <stdlibx.h>
#include <dynarray.h>
#include <hashtable.h>
#include <pool.h>
#include <integer.h>
#include <floatc.h>
#include "ccsym.h"
#include "error.h"
#include "globals.h"

class Symbol;
class Symbol_tab;
class Symbol_arr;
class Expr_node;

enum Type_val
{
	T_UNKNOWN = 0,
	T_VOID,
	T_INT,
	T_FLOAT,
	T_ARRAY,
	T_POINTER,
	T_STRUCT,
	T_UNION,
	T_ENUM,
	T_BITFIELD,
	T_FUNCTION,				// also member functions
	T_OVERLOAD,				// overloaded functions
	T_NUM_TYPES
};

class Type
{
protected:
	Integer *_size;			// not part of hash
	Type_val _typeval;
	boolean _const;
	boolean _volatile;
	boolean _inpool;

	Type(Type &t)
		{ _size = NULL; _typeval = t._typeval; _const = t._const;
				_volatile = t._volatile; _inpool = FALSE; }
	Type(Type_val t)
		{ _size = NULL; _typeval = t; _const = FALSE;
				_volatile = FALSE; _inpool = FALSE; }
	virtual ~Type() { }
	Integer &dosize();		// size in min_addressable chunks

public:
	Type()
		{ _size = NULL; _typeval = T_UNKNOWN; _const = FALSE;
				_volatile = FALSE; _inpool = FALSE; }
	virtual void free();
	virtual void debug();
	virtual Type *dup();
	virtual int hashlen();
	virtual unsigned long hash();
	virtual boolean operator==(Type &t);
	boolean operator!=(Type &t) { return !(*this == t); }
	virtual void size(Integer &s);
	virtual int align();
	virtual int getwidth();
	virtual boolean complete();

	Type_val typeval() { return _typeval; }
	boolean isunknown() { return _typeval == T_UNKNOWN; }
	boolean isvoid() { return _typeval == T_VOID; }
	boolean isfunc() { return _typeval == T_FUNCTION; }
	boolean ismemfunc();
	boolean isoverload() { return _typeval == T_OVERLOAD; }
	boolean isanyfunc()
		{ return _typeval == T_FUNCTION || _typeval == T_OVERLOAD; }
	boolean isfloat() { return _typeval == T_FLOAT; }
	boolean isint() { return _typeval == T_INT; }
	boolean sign();		// TRUE for signed integer types
	boolean isbit() { return _typeval == T_BITFIELD; }
	boolean isptr() { return _typeval == T_POINTER; }
	boolean isarr() { return _typeval == T_ARRAY; }
	boolean isenum() { return _typeval == T_ENUM; }
	boolean isintegral()
		{ return _typeval == T_INT || _typeval == T_BITFIELD ||
				_typeval == T_ENUM; }
	boolean isclass();
	boolean isstruct() { return _typeval == T_STRUCT; }
	boolean isunion() { return _typeval == T_UNION; }
	boolean issu() { return _typeval == T_STRUCT || _typeval == T_UNION; }
	boolean issue()
		{ return _typeval == T_STRUCT || _typeval == T_UNION ||
				_typeval == T_ENUM; }

	boolean isconst() { return _const; }
	void isconst(int c) { _const = c ? TRUE : FALSE; }
	boolean isvolatile() { return _volatile; }
	void isvolatile(int v) { _volatile = v ? TRUE : FALSE; }
	boolean inpool() { return _inpool; }
	void inpool(boolean p) { _inpool = p; }
	Integer &getsize() { return (_size != NULL) ? *_size : dosize(); }
};
declare_array(Type_arr, Type*, 64);

extern Type *g_unknown_type;

class Void_type : public Type
{
	Void_type(Void_type &v) : Type(v) { }

public:
	Void_type() : Type(T_VOID) { }
	virtual void debug();
	virtual Type *dup();
	//virtual boolean operator==(Type &t);
	//virtual void size(Integer &s);		// use Type::size()
	//virtual int align();		// use Type::align()
	virtual int hashlen();
};

class Int_type : public Type
{
protected:
	boolean _sign;
	IntegerClass *_ic;

	Int_type(Int_type &it) : Type(it)
			{ _ic = it._ic; _sign = it._sign; }
	Int_type(Type_val t, IntegerClass *ic, boolean s) : Type(t)
			{ _ic = ic; _sign = s; }

public:
	Int_type(IntegerClass *ic, boolean s) : Type(T_INT)
			{ _ic = ic; _sign = s; }
	virtual void debug();
	virtual Type *dup();
	virtual int hashlen();
	virtual unsigned long hash();
	virtual boolean operator==(Type &t);
	virtual void size(Integer &s);
	virtual int align();
	virtual int getwidth();
	int width() { return _ic->bits(); }
	boolean sign() { return _sign; }
	IntegerClass &ic() { return *_ic; }
};

class Float_type : public Type
{
protected:
	FloatClass *_fc;

	Float_type(Float_type &ft) : Type(ft) { _fc = ft._fc; }

public:
	Float_type(FloatClass *fc) : Type(T_FLOAT) { _fc = fc; }
	virtual void debug();
	virtual Type *dup();
	virtual int hashlen();
	virtual unsigned long hash();
	virtual boolean operator==(Type &t);
	virtual void size(Integer &s);
	virtual int align();
	virtual int getwidth();
	int width() { return _fc->bits(); }
	FloatClass &fc() { return *_fc; }
};

class Array_type : public Type
{
protected:
	Type *_type;
	Integer _len;
	boolean _complete;

	Array_type(Array_type &at);

public:
	Array_type(Type *t, Integer &l, boolean c = TRUE);
	virtual void free();
	virtual void debug();
	virtual Type *dup();
	virtual int hashlen();
	virtual void size(Integer &s);
	virtual int align();
	virtual boolean operator==(Type &t);
	virtual boolean complete();
	Type *type() { return _type; }
	void type(Type *t) { _type = (t == NULL) ? g_unknown_type : t; }
	Integer &len() { return _len; }
	void complete(boolean c) { _complete = c; }
};

class Pointer_type : public Type
{
protected:
	Type *_type;

	Pointer_type(Pointer_type &pt) : Type(pt) { _type = pt._type; }

public:
	Pointer_type(Type *t) : Type(T_POINTER) { _type = t; }
	virtual void free();
	virtual void debug();
	virtual Type *dup();
	virtual int hashlen();
	virtual void size(Integer &s);
	virtual int align();
	virtual int getwidth();
	virtual boolean operator==(Type &p);
	Type *type() { return _type; }
	void type(Type *t) { _type = (t == NULL) ? g_unknown_type : t; }
};

class Struct_info;
class Struct_type;

// not entered into type_pool
class Struct_info
{
	int _ref;
	Symbol *_symbol;
	Scope *_scope;
	Symbol_tab *_members;		// all members in struct
	Struct_type *_base;				// pointer to base class type
	Symbol_arr *_memlist;		// data members only
	Symbol_arr *_friends;		// pointers to friend classes/funcs/memfuncs
	union
	{
		// if _has_virtual, then _virtlist is valid, else _next is
		Symbol_arr *_virtlist;		// virtual table pointers?
		Integer *_next;				// for enums only
	};
	int _align;
	int _complete : 1;
	int _is_class : 1;
	int _has_virtual : 1;
	int _has_constructor : 1;
	int _has_destructor : 1;
	int _has_operator : 1;

public:
	Struct_info();
	void free();
	void ref() { _ref++; }
	void unref() { if (!--_ref) free(); }
	void size(Integer &s);
	int do_align();
	int align() { return _align == 0 ? do_align() : _align; }
	void debug();
	Symbol *symbol() { return _symbol; }
	void symbol(Symbol *s) { _symbol = s; }
	const char *name() { return _symbol ? _symbol->name() : (const char*)NULL; }
	//void name(const char *n) { _name = n; }
	Symbol_tab *members() { return _members; }
	Symbol_arr *memlist() { return _memlist; }
	Struct_type *base() { return _base; }
	void base(Struct_type *b) { _base = b; }
	Symbol_arr *friends() { return _friends; }
	Symbol_arr *virtlist() { return _virtlist; }
	void alloc_memlist() { _memlist = new Symbol_arr; }
	void alloc_friends() { _friends = new Symbol_arr; }
	void alloc_virtlist() { _virtlist = new Symbol_arr; }
	Scope *scope() { return _scope; }
	//void scope(Scope *s) { _scope = s; }
	Integer *next() { return _next; }
	void next(Integer *n) { _next = n; }
	boolean complete() { return _complete; }
	void complete(boolean c) { _complete = c; }
	boolean is_class() { return _is_class; }
	void is_class(boolean c) { _is_class = c; }
	boolean has_virtual() { return _has_virtual; }
	void has_virtual(boolean c) { _has_virtual = c; }
	boolean has_constructor() { return _has_constructor; }
	void has_constructor(boolean c) { _has_constructor = c; }
	boolean has_destructor() { return _has_destructor; }
	void has_destructor(boolean c) { _has_destructor = c; }
	boolean has_operator() { return _has_operator; }
	void has_operator(boolean c) { _has_operator = c; }
};

// structs are also classes
class Struct_type : public Type
{
protected:
	Struct_info *_info;

	Struct_type(Type_val t) : Type(t)
			{ _info = new Struct_info; }
	Struct_type(Struct_type &s) : Type(s)
			{ _info = s._info; _info->ref(); }
	Struct_type(Type_val t, Struct_info *i) : Type(t)
			{ _info = i; _info->ref(); }

public:
	Struct_type() : Type(T_STRUCT) { _info = new Struct_info; }
	virtual void free();
	virtual void size(Integer &s);
	virtual int align();
	virtual boolean complete();
	//virtual int hashlen();
	virtual unsigned long hash();
	virtual void debug();
	virtual Type *dup();
	virtual boolean operator==(Type &t);
	Struct_info *info() { return _info; }
	void info(Struct_info *i) { i->ref(); _info->unref(); _info = i; }
	Symbol *symbol() { return _info->symbol(); }
	void symbol(Symbol *s) { _info->symbol(s); }
	const char *name() { return _info->name(); }
	//void name(const char *n) { _info->name(n); }
	void complete(boolean c) { _info->complete(c); }
	boolean iscomplete() { return _info->complete(); }
	Symbol *add(char const *member, Type *type, int line,
			int storage = NONE, int access = NONE,
			boolean isinline = FALSE, Symbol *unnamed = NULL);
	void add_friend(Symbol *sym);
	Symbol_tab *members() { return _info->members(); }
	Symbol_arr *memlist() { return _info->memlist(); }
	Struct_type *base() { return _info->base(); }
	Symbol_arr *virtlist() { return _info->virtlist(); }
	Scope *scope() { return _info->scope(); }
	//void scope(Scope *s) { _info->scope(s); }
	boolean is_class() { return _info->is_class(); }
	void is_class(boolean c) { _info->is_class(c); }
	boolean has_virtual() { return _info->has_virtual(); }
	void has_virtual(boolean c) { _info->has_virtual(c); }
	void add_base_class(Type *base, int access, boolean isvirtual);
	void fini() { if (g_cplusplus) _info->scope()->cleanup(); }
};

inline boolean
Type::isclass()
{
	return _typeval == T_STRUCT && ((Struct_type*)this)->is_class();
}

class Union_type : public Struct_type
{
public:
	Union_type() : Struct_type(T_UNION) { }
	virtual void debug();
	//virtual Type *dup();
	virtual void size(Integer &s);
};

class Enum_type : public Struct_type
{
public:
	Enum_type() : Struct_type(T_ENUM) { }
	//virtual void free();
	virtual void size(Integer &s);
	virtual int align();
	virtual int getwidth();
	//virtual boolean complete();
	virtual void debug();
	//virtual Type *dup();
	//virtual int hashlen();
	//virtual boolean operator==(Type &t);
	void add(char const *name, const char *file, int line, Integer &id);
	void add(char const *name, const char *file, int line);
	Integer *next() { return _info->next(); }
	void next(Integer *n) { _info->next(n); }
};

class Bitfield_type : public Int_type
{
protected:
	int _width;

	Bitfield_type(Bitfield_type &b) : Int_type(b) { _width = b._width; }

public:
	Bitfield_type(int w, Int_type *base) :
			Int_type(T_BITFIELD, &base->ic(), base->sign()) { _width = w; }
	virtual void debug();
	virtual Type *dup();
	virtual int hashlen();
	virtual unsigned long hash();
	virtual void size(Integer &s);
	virtual int align();
	virtual int getwidth();
	//virtual boolean operator==(Type &t);
	int width() { return _width; }
};

inline boolean
Type::sign()
{
	return (_typeval == T_INT || _typeval == T_BITFIELD) &&
				((Int_type *)this)->sign();
}

class Param
{
	const char *_symbol;
	Type *_type;
	int _storage;
	Expr_node *_init;

public:
	Param() { _symbol = NULL; _type = g_unknown_type; _storage = NONE;
				_init = NULL; }
	Param(const Param &p)
			{ _symbol = p._symbol; _type = p._type; _storage = p._storage;
				_init = p._init; }
	void operator=(const Param &p)
			{ _symbol = p._symbol; _type = p._type; _storage = p._storage;
				if (p._init) _init = p._init; }
	boolean operator==(const Param &p)
			{ return _type == p._type || *_type == *p._type; }
	boolean operator!=(const Param &p) { return !(*this == p); }

	const char *symbol() { return _symbol; }
	void symbol(const char *s) { _symbol = s; }
	Type *type() { return _type; }
	void type(Type *t) { _type = (t == NULL) ? g_unknown_type : t; }
	int storage() { return _storage; }
	void storage(int s) { _storage = s; }
	Expr_node *init() { return _init; }
	void init(Expr_node *i) { _init = i; }
};
declare_array(Param_arr, Param, 32);

class Function_type : public Type
{
protected:
	Symbol *_classname;				// for member-functions only
	Type *_retval;
	boolean _retref;				// returning a reference or not?
	boolean _varargs;
	int _functype;				// Pascal, FORTRAN, extern "C"
	Param_arr *_args;				// needs to be last element for hash()

	Function_type(Function_type &f);

public:
	Function_type() : Type(T_FUNCTION)
			{ _classname = NULL; _retval = NULL; _args = NULL;
					_retref = FALSE; _varargs = FALSE; _functype = NONE; }
	Function_type(Type *rval, Param_arr *args, Symbol *c = NULL) :
			Type(T_FUNCTION) { _classname = c; _retval = rval; _args = args;
					_retref = FALSE; _varargs = FALSE; _functype = NONE; }
	virtual void free();
	virtual void debug();
	virtual Type *dup();
	virtual unsigned long hash();
	virtual boolean operator==(Type &t);
	//virtual void size(Integer &s);
	//virtual int align();
	//virtual boolean complete();
	Type *retval() { return _retval; }
	void retval(Type *r) { _retval = (r == NULL) ? g_unknown_type : r; }
	boolean retref() { return _retref; }
	void retref(int r) { _retref = r ? TRUE : FALSE; }
	boolean varargs() { return _varargs; }
	void varargs(boolean v) { _varargs = v; }
	int functype() { return _functype; }
	void functype(int f) { _functype = f; }
	Param_arr *args() { return _args; }
	void args(Param_arr *a) { _args = a; }
	Symbol *classname() { return _classname; }
	void classname(Symbol *c) { _classname = c; }
};

inline boolean
Type::ismemfunc()
{
	return _typeval == T_FUNCTION &&
			((Function_type*)this)->classname() != NULL;
}

// never stored in type-pool as there is no way to access it without
// first finding a Symbol, hence no virtuals are needed here
class Overload_type : public Type
{
protected:
	Symbol *_sym;

	Overload_type(Overload_type &o) : Type(o) { _sym = o._sym; }

public:
	Overload_type(Symbol *sym) : Type(T_OVERLOAD) { _sym = sym; }
	//virtual void free();
	virtual void debug();
	virtual Type *dup();
	virtual int hashlen();
	//virtual unsigned long hash();
	virtual boolean operator==(Type &t);
	//virtual void size(Integer &s);
	//virtual int align();
	//virtual int getwidth();
	//virtual boolean complete();
	Symbol *sym() { return _sym; }
	void sym(Symbol *s) { _sym = s; }
};

extern void init_types();
extern Type *pool_type(Type *t);
extern boolean equivalent(Type *t1, Type *t2);
extern boolean sub_type(Type *from, Type *to);
extern Type *base_type(Type *t);
extern boolean valid_cast(Type *from, Type *to, boolean ptr = FALSE);
extern Type *arith_conv(Type *t1, Type *t2);
inline boolean compare_args(Param_arr *a1, Param_arr *a2)
		{ return *a1 == *a2; }

#endif // __CCTYPES_H_
