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
// $Header: /u/cgt/cvs/bin/cc/cc.h,v 1.47 1999/03/15 21:48:45 parag Exp $
#ifndef __CC_H_
#define __CC_H_

#include <stddef.h>
#include <stdlib.h>
#include <hashtable.h>
#include <stdlibx.h>
#include <stringx.h>

#define UNNAMED_CHAR '!'

class C_keyword
{
	const char *name;
	int tok;
public:

	C_keyword() { name = NULL; }
	C_keyword(const char *key) { name = key; }
	boolean operator==(C_keyword &key) { return name == key.name; }
	boolean operator==(const char *key) { return name == key; }
	void operator=(C_keyword &n) { name = n.name; tok = n.tok; }
	operator const char*() { return name; }

	int token() { return tok; }
	void token(int t) { tok = t; }
	void operator=(int t) { tok = t; }
};
declare_hashtable(C_keyword_tab, C_keyword_iter, C_keyword, const char*);

class Node;
class Expr_node;
class String_node;
class Switch_node;
class Initializer_node;
class Node_arr;
class Expr_arr;
class Type;
class Struct_type;
class Enum_type;
class Type_arr;
class Symbol;
class Charvec;
class Cpp;
class Cpp_node;
class Scope;

struct Token
{
	const char *text;
	const char *file;
	int line;
	Symbol *sym;
};

// for flags in Storage_thing below
#define S_NONE	  0x0000
#define S_SHORT	 0x0001
#define S_LONG	  0x0002
#define S_LONGLONG  0x0004
#define S_SIGNED	0x0008
#define S_UNSIGNED  0x0010
#define S_PASCAL	0x0020
#define S_FORTRAN   0x0040
#define S_INLINE	0x0080
#define S_FRIEND	0x0100
#define S_VIRTUAL   0x0200
//#define S_UNUSED  0x0400
#define S_REFERENCE 0x0800
#define S_VOLATILE  0x1000
#define S_CONST	 0x2000
#define S_NEAR	  0x4000
#define S_FAR	   0x8000
#define S_SPECMASK  0x001F
#define S_FUNCMASK  0x03E0
#define S_QUALMASK  0xF800
#define S_UNKNOWN   0xFFFF

class Storage_thing
{
public:
	Type *type;
	union
	{
		const char *sym;
		Symbol *symbol;
		Expr_node *expr;
	};
	Scope *scope;
	short storage;
	short basic;
	unsigned short flags;
	short bitfield;

	Storage_thing()
	{
		type = NULL;
		sym = NULL;
		scope = NULL;
		storage = 0;		// NONE
		basic = 0;		// NONE
		flags = S_NONE;
		bitfield = -1;
	}
	Storage_thing(Storage_thing &s)
	{
		type = s.type;
		sym = s.sym;
		scope = s.scope;
		storage = s.storage;
		basic = s.basic;
		flags = s.flags;
		bitfield = s.bitfield;
	}
	void merge_basic(int b);
	void merge_storage(int s);
	void make_type(boolean bitfieldok = FALSE);
	Symbol *do_declaration();
	void do_member_decl(Struct_type *stype, int access = 0/*NONE*/);
};

extern Cpp g_cpp;
extern C_keyword_tab g_keywords;
extern Scope *g_curr_scope;
extern Scope *g_file_scope;
extern Scope *g_func_scope;
extern Node_arr *g_curr_statlist;
extern int g_loop_count;
extern int g_default_linkage;

extern void init_keywords();
extern long strtochar(const char *s, boolean wide = FALSE);
extern const char *make_unnamed_name();
extern const char *make_destructor_name(const char *classname);
extern const char *make_constructor_name(const char *classname);
extern const char *make_operator_name(int optok);
extern const char *make_memfunc_name(const char *classname,
		const char *memfunc);
const char *make_opcast_name(Type *type);

extern void merge_flags(unsigned short &flags, int f);
extern Struct_type *do_sue_declaration(int stok, const char *id);
extern Struct_type *do_sue_definition(int stok, const char *id);
Type *do_array(Expr_node *expr, Type *type, boolean isparam);
void fini_function_def(Symbol *func, Node_arr *body);

extern Symbol *do_function_def(Storage_thing &func);
extern void do_old_function_def(Storage_thing &func);
extern Symbol *check_old_function_def(Storage_thing &func);

extern void check_unused_vars(Scope *scope, Symbol *func = NULL);
extern void check_for_return(Symbol *func);
void check_labels(Symbol *func);
extern void do_initializer(Symbol *sym, Expr_node *init,
		boolean isparam = FALSE);
extern void do_ctor_init(Symbol *sym, Expr_arr *init);
extern void do_member_init(const char *sym, Expr_arr *init);

extern void do_unnamed_substruct(Type *stype, Type *parent);
extern Expr_node *do_struct_display(Type *cast, Initializer_node *init);

extern void init_tree();
extern void do_tree(Symbol *sym);
extern void do_tree(Node *node);
extern class TreeNode *fini_tree();

extern void init_parser();

#endif // __CC_H_
