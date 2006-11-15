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
// $Header: /u/cgt/cvs/bin/cc/cpp.h,v 1.37 1999/03/15 21:48:55 parag Exp $
#ifndef __CPP_H_
#define __CPP_H_

#include "cppdefs.h"
#include <hashtable.h>

class Cpp_node
{
public:
	union
	{
		int token;
		int argnum;				// position of arg in arg list
	};
	union
	{
		const char *text;
		Cpp_node *arg;				// for pointers to arguments in macros
	};

	Cpp_node() { }
	Cpp_node(const Cpp_node &n) { token = n.token; text = n.text; }
	boolean operator==(const Cpp_node &n)
			{ return token == n.token && text == n.text; }
	boolean operator!=(const Cpp_node &n) { return !(*this == n); }
	const char *tok2str();
};
declare_array(Cpp_node_list, Cpp_node, 1024);
declare_array(Cpp_node_list_vec, Cpp_node_list, 64);

enum Cpp_macro_type
{
	NORMAL_MACRO = 0,				// every normal macro
	PREDEFINED_MACRO,				// __STDC__, anything on command line
								// only used as a parameter to a function

	// all magic-macro values must be less than zero
	FILE_MACRO = -1,				// __FILE__
	LINE_MACRO = -2,				// __LINE__
	FUNC_MACRO = -3,				// __FUNC__ (not ISO)

	LAST_MACRO_TYPE_WITHOUT_COMMA
};

class Cpp_macro
{
public:
	const char *name;
	Cpp_node_list *args;		// NULL=none, zero=function-type args
	Cpp_node_list *body;
	boolean exp;
	const char *file;
	Cpp_macro_type type;
	int line;

	Cpp_macro() { name = NULL; args = NULL; body = NULL; exp = TRUE; }
	Cpp_macro(const char *key)
			{ name = key; args = NULL; body = NULL; exp = TRUE; }
	boolean operator==(Cpp_macro &key) { return name == key.name; }
	boolean operator==(const char *key) { return name == key; }
	operator const char*() { return name; }

	boolean expand() { return exp; }
	boolean expand(boolean e) { return exp = e; }
	Cpp_node_list *getbody() { return body; }
	Cpp_node_list *getargs() { return args; }
	const char *getname() { return name; }
	int getline() { return line; }
	int gettype() { return type; }
	const char *getfile() { return file; }

	void define_macro(Cpp_node_list *list, Cpp_macro_type mtype);
};
declare_hashtable(Cpp_macro_tab, Cpp_macro_iter, Cpp_macro, const char*);

class Cpp_control
{
	const char *_name;
	Cpp_node_list *_control;

public:
	Cpp_control() { _name = NULL; _control = NULL; }
	~Cpp_control() { strfree(_name); }
	Cpp_control(const char *key) { _name = strdup(key); _control = NULL; }
	boolean operator==(Cpp_control &key) { return streq(_name, key._name); }
	boolean operator==(const char *key) { return streq(_name, key); }
	operator const char*() { return _name; }
	const char *name() { return _name; }
	Cpp_node_list *control() { return _control; }
	void control(Cpp_node_list *c) { delete _control; _control = c; }
};
declare_hashtable(Cpp_control_tab, Cpp_control_iter, Cpp_control, const char*);

class Cpp_file
{
public:
	char *buf, *ptr;
	size_t len;
	int ifcount;		// used for counting #if directives
	Charbuf seen_else;		// used as array of booleans to count #else directives
	const char *fname;
	int line;
	Cpp_node_list *control;

	Cpp_file() { buf = NULL; fname = NULL; control = NULL; }
	boolean operator==(const Cpp_file &f) { return buf == f.buf; }
	boolean open(const char *fn);
	void close();
	void rescan(const char *str);
};
declare_array(Cpp_file_list, Cpp_file, 64);

class Cpp_stack_elt
{
public:
	Cpp_node_list buf;
	Cpp_macro *macro;
	int file;
	int loc;

	boolean operator==(const Cpp_stack_elt &e)
			{ return buf == e.buf && macro == e.macro
					&& file == e.file && loc == e.loc; }
};
declare_array(Cpp_stack, Cpp_stack_elt, 64);

class Cpp
{
	Cpp_file_list files;		// stack of files for scanning - #include
	int currfile;
	char *ptr;
	Charvec includes;
	Cpp_macro_tab macros;
	Cpp_control_tab controls;
	Cpp_stack stack;
	int top;
	int line, col;
	Cpp_node node;
	boolean in_cpp;
	const char *s_one, *s_zero;

	void cleanupbuf();
	boolean check_control(const char *fn);
	inline int get() { return *ptr++; }

	boolean expand_macro(Cpp_macro &macro, Cpp_node_list &buf);
	boolean expand_macro_body(Cpp_macro &macro, Cpp_node_list_vec &params,
			Cpp_node_list &buf);
	void expand_all_macros(Cpp_node_list &buf);

	void do_defined(Cpp_node_list &buf, Cpp_node *arr);
	boolean test_if_expr(Cpp_node_list *buf);
	void if_directive();
	void skip_if(boolean stop_at_else = TRUE);
	void ifdef_directive(boolean expected);
	void elif_directive();
	void else_directive();
	void endif_directive();

	void include_directive();
	void define_directive();
	void undef_directive();
	void line_directive();
	void error_directive();
	void pragma_directive();

public:
	Cpp();
	boolean open(const char *fname);
	void add_include_dir(const char *dir);
	void clear_includes() { includes.reset(); }

	int scan(Cpp_node_list *buf = NULL, boolean retwhsp = TRUE);
	void skipwhsp() { scan(NULL, FALSE); }
	void getline(Cpp_node_list &buf) { scan(&buf); }

	Cpp_node &getnode() { return node; }
	int linenum() { return line; }
	const char *filename()
			{ return currfile < 0 ? (const char*)NULL : files[currfile].fname; }

	boolean define_macro(const char *macro, const char *string,
			Cpp_macro_type mtype = PREDEFINED_MACRO);
	boolean undef_macro(const char *macro);
};

// compiler hooks for special preprocessor functions:
extern const char *get_current_func();						// for __FUNC__
extern void do_pragma_directive(Cpp_node_list &toks);		// for #pragma

#endif // __CPP_H_
