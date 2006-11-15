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
// $Header: /u/cgt/cvs/bin/cc/ccsym.h,v 1.30 2003/04/29 21:56:09 parag Exp $
#ifndef __CCSYM_H_
#define __CCSYM_H_

#if defined(__GNUC__) && !defined(__APPLE__)
#pragma interface
#endif

#include "cppdefs.h"
//#include "cctype.h"

class Type;
class Function_type;
class Symbol;
class Symbol_tab;
class Symbol_arr;
class Node;
class Node_arr;
class Expr_node;
class Expr_arr;
class Scope;
class TreeNode;
class StorageAllocator;

// used for both symbols and struct/union/enum members
class Symbol
{
	const char *_name;
	const char *_alias;		// to rename symbols on output, if needed
	Type *_type;
	Scope *_scope;
	union
	{
		Node *_node;				// init expr or func or memfunc
		Symbol *_member;		// enum member
		Symbol *_unnamed;		// unnamed struct/union entry
		Symbol_arr *_overloads;		// overloaded symbols
		Expr_arr *_init;		// for constructor initializations
	};
	int _line;
	short _storage;		// static, extern, register, typedef, enum, virtual, &
	union
	{
		short _access;				// private, protected, public
		short _regnum;				// register number for register vars
	};
	int _read, _wrote;
	union
	{
		Symbol_tab *_labels;		// for functions only, ptr to label table
		Integer *_enumval;		// else value of enum entry or
		Integer *_offset;		// offset of symbol as appropriate
	};
	union
	{
		const char *_file;		// file of symbol definition/declaration
		Integer *_pad;				// else padding of struct member
	};
	int _referenced : 1;
	int _parameter : 1;
	int _treedone : 1;
	int _isinline : 1;
	int _overloaded : 1;
	int _virtual : 1;
	int _unused : 1;
	int _isasm : 1;

public:
	void debug();
	Symbol();
	Symbol(Symbol &s);
	Symbol(const char *n);
	~Symbol();
	boolean operator==(Symbol &key) { return _name == key._name; }
	boolean operator==(const char *key) { return _name == key; }
	operator const char*() { return _name; }
	const char *name() { return _name; }

	void alias(const char *a) { _alias = a; }
	const char *alias() { return _alias; }
	Type *type() { return _type; }
	//void type(Type *t) { _type = t; }
	Scope *scope() { return _scope; }
	void scope(Scope *s) { _scope = s; }
	Node *node() { return _node; }
	void node(Node *n) { _node = n; }
	Symbol *member() { return _member; }
	void member(Symbol *m) { _member = m; }
	Symbol *unnamed() { return _unnamed; }
	void unnamed(Symbol *u) { _unnamed = u; }
	Expr_arr *init() { return _init; }
	void init(Expr_arr *i) { _init = i; }
	const char *file() { return _file; }
	void file(const char *f) { _file = f; }
	int line() { return _line; }
	void line(int l) { _line = l; }
	int storage() { return _storage; }
	void storage(int st) { _storage = st; }
	int access() { return _access; }
	void access(int a) { _access = a; }
	int regnum() { return _regnum; }
	void regnum(int r) { _regnum = r; }
	boolean referenced() { return _referenced; }
	void referenced(boolean r) { _referenced = r; }
	int read() { return _read; }
	void read(int r) { _read = r; }
	int wrote() { return _wrote; }
	void wrote(int w) { _wrote = w; }
	boolean parameter() { return _parameter; }
	void parameter(boolean p) { _parameter = p; }
	boolean treedone() { return _treedone; }
	void treedone(boolean i) { _treedone = i; }

	TreeNode *mktree();
	Integer *offset() { return _offset; }
	void offset(Integer *a) { _offset = a; }
	Integer *enumval() { return _enumval; }
	void enumval(Integer *e) { _enumval = e; }
	Integer *pad() { return _pad; }
	void pad(Integer *e) { _pad = e; }

	boolean is_inline() { return _isinline; }
	void is_inline(boolean i) { _isinline = i; }
	boolean overloaded() { return _overloaded; }
	void overloaded(boolean o) { _overloaded = o; }
	boolean is_virtual() { return _virtual; }
	void is_virtual(boolean v) { _virtual = v; }
	boolean unused() { return _unused; }
	void unused(boolean u) { _unused = u; }
	boolean is_asm() { return _isasm; }
	void is_asm(boolean a) { _isasm = a; }
	Symbol *exact_match(Type *type);
	long cost_overload_match(Expr_arr *a);
	Symbol *overload_match(Expr_arr *args);
	Symbol *add_overload(Type *t);
	void type(Type *t)
		{ if (!g_cplusplus || _type == NULL || !add_overload(t)) _type = t; }

	Symbol_tab &labels();
	boolean has_labels() { return _labels != NULL; }
	void new_labels();
	void cleanup_labels();
};

declare_hashtable(Symbol_tab, Symbol_iter, Symbol, const char*);
declare_array(Symbol_arr, Symbol*, 32);

inline Symbol_tab &Symbol::labels() { return *_labels; }
inline void Symbol::new_labels()
		{ if (_labels == NULL) _labels = new Symbol_tab; }

class Scope;
declare_array(Scope_list, Scope*, 64);

class Scope
{
	Symbol_tab *_symtab;
	Symbol_tab *_suetypes;
	const char *_fname;
	int _fline;
	Scope *_parent;
	Scope_list *_children;
	Symbol_arr *_init;			// all local declarations in decl order
	Symbol_arr *_fini;			// all local declarations to be destroyed
	Symbol *_symbol;			// points to function or struct symbol
	Node *_node;				// for function body or compound statement
	Symbol_arr *_ctor;			// any constructor initializations
	int _id;					// unique scope id for back-end name-generation

public:
	void debug();

	Scope(const char *fn, int fl, Scope *p);
	~Scope();

	const char *file() { return _fname; }
	int line() { return _fline; }
	Scope *parent() { return _parent; }

	boolean has_symtab() { return _symtab != NULL; }
	boolean has_suetypes() { return _suetypes != NULL; }
	Symbol_tab &symtab() { return *_symtab; }
	Symbol_tab &suetypes() { return *_suetypes; }
	Symbol *lookup_sue(const char *name);
	Symbol *lookup_sym(const char *name);
	Symbol *lookup_member(const char *name);

	Symbol_arr *init() { return _init; }
	void new_init() { if (_init == NULL) _init = new Symbol_arr; }
	Symbol_arr *fini() { return _fini; }
	void new_fini() { if (_fini == NULL) _fini = new Symbol_arr; }
	Symbol_arr *ctor() { return _ctor; }
	void new_ctor() { if (_ctor == NULL) _ctor = new Symbol_arr; }
	Scope_list *children() { return _children; }
	void new_children() { if (_children == NULL) _children = new Scope_list; }
	void cleanup();

	Node *node() { return _node; }
	void node(Node *n) { _node = n; }
	Symbol *symbol() { return _symbol; }
	void symbol(Symbol *s) { _symbol = s; }
	int id() { return _id; }
	void id(int i) { _id = i; }

	TreeNode *mktree();
	void calclocals(StorageAllocator &stkalloc);
};

#endif // __CCSYM_H_
