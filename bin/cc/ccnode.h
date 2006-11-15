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
// $Header: /u/cgt/cvs/bin/cc/ccnode.h,v 1.56 2003/05/01 06:48:52 parag Exp $

#ifndef __CCNODE_H_
#define __CCNODE_H_

#if defined(__GNUC__) && !defined(__APPLE__)
#pragma interface
#endif

#include "cppdefs.h"
#include "cctype.h"

class Symbol;
class Scope;
class TreeNode;
class LValueTreeNode;

enum Node_type
{
	N_UNKNOWN = 0,
	N_NODE,
	N_EXPR,
	N_INTEGER,
	N_SIZEOF,
	N_FLOAT,
	N_STRING,
	N_SYMBOL,
	N_ARRAY_REF,
	N_FUNC_CALL,
	N_STRUCT_REF,
	N_OPERATOR,
	N_OPERATOR2,
	N_OPERATOR3,
	N_CAST,
	N_INITIALIZER,
	N_LABEL,
	N_CASE,
	N_SWITCH,
	N_COMPOUND,
	N_IF,
	N_WHILE,
	N_DOWHILE,
	N_FOR,
	N_GOTO,
	N_LOOP,
	N_CONTINUE,
	N_BREAK,
	N_RETURN,
	N_ASM,
	N_NEW,		// C++
	N_DELETE,		// C++
	N_NUM_NODES
};

class Node
{
protected:
	Scope *_scope;

	Node() { _scope = NULL; }
	Node(Scope *s) { _scope = s; }
	void scope(Scope *s) { _scope = s; }

public:
	virtual ~Node();

	virtual Node_type nodetype();
	Scope *scope() { return _scope; }
	virtual const char *name();
	virtual Node_type check_return();
	virtual boolean check_break();
	virtual TreeNode *mktree();
	virtual TreeNode *mkinit();
	virtual void debug();
	virtual const char *brklabel();
	virtual const char *contlabel();
};
declare_array(Node_arr, Node*, 256);

class Const_expr
{
public:
	union
	{
		Integer *ival;
		Float *fval;
	};
	boolean isint;		// is result in "i" or "f"?
	Symbol *sym;		// for calculating addr-of-sym +- offset

	Const_expr()
			{ ival = NULL; isint = TRUE; sym = NULL; }
	void free() { if (isint) delete ival; else delete fval; }
	~Const_expr() { free(); }
	void set_ival(Integer &i)
		{ if (ival) *ival = i; else ival = new Integer(i); isint = TRUE; }
	void new_ival(IntegerClass &ic)
		{ free(); ival = new Integer(ic); isint = TRUE; }
	void set_fval(Float &f)
		{ if (fval) *fval = f; else fval = new Float(f); isint = FALSE; }
};

class Expr_node : public Node
{
protected:
	Type *_type;

	Expr_node() : Node() { _type = g_unknown_type; }
	Expr_node(Scope *s) : Node(s) { _type = g_unknown_type; }
	Expr_node(Type *t, Scope *s) : Node(s)
			{ _type = (t == NULL) ? g_unknown_type : t; }

	friend void init_nodes();

public:
	void type(Type *t) { _type = (t == NULL) ? g_unknown_type : t; }
	Type *type() { return _type; }
	virtual Node_type nodetype();
	virtual void eval_const_expr(Const_expr &val);
	virtual boolean is_int_expr();
	virtual boolean is_static_init(int *ref = NULL);
	virtual boolean check_init_type(Type *t, Integer &newlen);
	virtual boolean is_lvalue();
	virtual void mark_symbol(boolean read, boolean wrote, boolean ref);
	virtual TreeNode *mktree();
	virtual TreeNode *mkinit();
	virtual LValueTreeNode *mklval();
	virtual void debug();
};
declare_array(Expr_arr, Expr_node*, 256);

class Integer_node : public Expr_node
{
	Integer *_val;

public:
	Integer_node(Int_type *t, Scope *s, const char *text);
	Integer_node(Int_type *t, Scope *s, long val);
	Integer_node(Type *t, Scope *s, Integer *val);
	Integer &value() { return *_val; }
	virtual Node_type nodetype();
	virtual void eval_const_expr(Const_expr &val);
	virtual boolean is_int_expr();
	virtual boolean is_static_init(int *ref = NULL);
	virtual boolean check_init_type(Type *t, Integer &newlen);
	virtual TreeNode *mktree();
	virtual TreeNode *mkinit();
	//virtual LValueTreeNode *mklval();
	virtual void debug();
};

class Sizeof_node : public Expr_node
{
	Integer _size;

public:
	Sizeof_node(Scope *s, Type *type);
	Sizeof_node(Scope *s, Expr_node *obj);
	virtual Node_type nodetype();
	virtual void eval_const_expr(Const_expr &val);
	virtual boolean is_int_expr();
	virtual boolean is_static_init(int *ref = NULL);
	virtual TreeNode *mktree();
	virtual TreeNode *mkinit();
	//virtual LValueTreeNode *mklval();
	virtual void debug();
};

class Float_node : public Expr_node
{
protected:
	Float *_val;

public:
	Float_node(Float_type *t, Scope *s, const char *text);
	Float &value() { return *_val; }
	virtual Node_type nodetype();
	virtual void eval_const_expr(Const_expr &val);
	virtual boolean is_static_init(int *ref = NULL);
	virtual TreeNode *mktree();
	virtual TreeNode *mkinit();
	//virtual LValueTreeNode *mklval();
	virtual void debug();
};

class String_node : public Expr_node
{
protected:
	const char *_string;
	size_t _len;
	ShortBool _dup, _pascal, _wide, _forth;
	const char *_label;

public:
	String_node(Scope *s, boolean wide = FALSE);
	String_node(String_node &str);
	virtual ~String_node();
	void append(const char *str);
	void finish();
	const char *string() { return _string; }
	size_t len() { return _len; }
	boolean is_wide() { return _wide; }
	boolean is_pascal() { return _pascal; }
	boolean is_forth() { return _forth; }
	const char *label() { return _label; }
	void label(const char *l) { _label = l; }
	virtual Node_type nodetype();
	virtual boolean is_int_expr();
	virtual boolean is_static_init(int *ref = NULL);
	virtual boolean check_init_type(Type *t, Integer &newlen);
	virtual TreeNode *mktree();
	virtual TreeNode *mkinit();
	virtual LValueTreeNode *mklval();
	virtual void debug();
};

class Symbol_node : public Expr_node
{
protected:
	Symbol *_sym;
	const char *_file;
	int _line;

public:
	Symbol_node(Scope *scope, const char *sym, const char *file, int line);
	Symbol_node(Scope *scope, Symbol *sym, const char *file, int line);
	Symbol *sym() { return _sym; }
	void sym(Symbol *s) { _sym = s; }
	const char *file() { return _file; }
	int line() { return _line; }
	virtual Node_type nodetype();
	virtual const char *name();
	virtual boolean is_int_expr();
	virtual void eval_const_expr(Const_expr &val);
	virtual boolean is_static_init(int *ref = NULL);
	virtual boolean check_init_type(Type *t, Integer &newlen);
	virtual boolean is_lvalue();
	virtual void mark_symbol(boolean read, boolean wrote, boolean ref);
	virtual TreeNode *mktree();
	virtual TreeNode *mkinit();
	virtual LValueTreeNode *mklval();
	virtual void debug();
};

class Array_ref_node : public Expr_node
{
protected:
	Expr_node *_arr;
	Expr_node *_expr;

public:
	Array_ref_node(Scope *s, Expr_node *arr, Expr_node *expr);
	virtual ~Array_ref_node();
	Expr_node *expr() { return _expr; }
	Expr_node *arr() { return _arr; }
	virtual Node_type nodetype();
	virtual const char *name();
	virtual boolean is_lvalue();
	virtual void eval_const_expr(Const_expr &val);
	virtual void mark_symbol(boolean read, boolean wrote, boolean ref);
	virtual boolean is_static_init(int *ref = NULL);
	virtual TreeNode *mktree();
	//virtual TreeNode *mkinit();
	virtual LValueTreeNode *mklval();
	virtual void debug();
};

class Func_call_node : public Expr_node
{
protected:
	Expr_node *_func;
	Expr_arr *_args;
	Function_type *_overload;		// real type if _type is overload

public:
	Func_call_node(Scope *s, Expr_node *f, Expr_arr *a);
	virtual ~Func_call_node();
	Expr_node *func() { return _func; }
	Expr_arr &args() { return *_args; }
	Function_type *realtype() { return _overload; }
	virtual Node_type nodetype();
	virtual const char *name();
	virtual boolean is_lvalue();
	virtual void mark_symbol(boolean read, boolean wrote, boolean ref);
	virtual TreeNode *mktree();
	virtual LValueTreeNode *mklval();
	virtual void debug();
};

class Struct_ref_node : public Expr_node
{
protected:
	Expr_node *_structref;
	Symbol *_elem;
	const char *_file;
	int _line;

public:
	Struct_ref_node(Scope *s, Expr_node *sref, const char *tag,
			const char *file, int line, boolean pointer);
	virtual ~Struct_ref_node();
	Expr_node *structref() { return _structref; }
	Symbol *elem() { return _elem; }
	const char *file() { return _file; }
	int line() { return _line; }
	virtual Node_type nodetype();
	virtual const char *name();
	virtual void eval_const_expr(Const_expr &val);
	virtual boolean is_lvalue();
	virtual void mark_symbol(boolean read, boolean wrote, boolean ref);
	virtual boolean is_static_init(int *ref = NULL);
	virtual TreeNode *mktree();
	virtual TreeNode *mkinit();
	virtual LValueTreeNode *mklval();
	virtual void debug();
};

class Operator_node : public Expr_node
{
protected:
	int _token;
	Expr_node *_arg;
	boolean _prefix;

public:
	Operator_node(Scope *s, int tok, Expr_node *a, boolean p = TRUE);
	virtual ~Operator_node();
	int token() { return _token; }
	boolean prefix() { return _prefix; }
	Expr_node *arg() { return _arg; }
	void arg(Expr_node *a) { _arg = a; }
	virtual Node_type nodetype();
	virtual void eval_const_expr(Const_expr &val);
	virtual boolean is_int_expr();
	virtual boolean is_static_init(int *ref = NULL);
	virtual boolean check_init_type(Type *t, Integer &newlen);
	virtual boolean is_lvalue();
	virtual void mark_symbol(boolean read, boolean wrote, boolean ref);
	virtual const char *name();
	virtual TreeNode *mktree();
	virtual TreeNode *mkinit();
	virtual LValueTreeNode *mklval();
	virtual void debug();
};

class Operator2_node : public Expr_node
{
protected:
	int _token;
	Expr_node *_arg1;
	Expr_node *_arg2;

public:
	Operator2_node(Scope *s, int tok, Expr_node *a1, Expr_node *a2);
	virtual ~Operator2_node();
	int token() { return _token; }
	Expr_node *arg1() { return _arg1; }
	Expr_node *arg2() { return _arg2; }
	virtual Node_type nodetype();
	virtual void eval_const_expr(Const_expr &val);
	virtual boolean is_int_expr();
	virtual boolean is_static_init(int *ref = NULL);
	virtual boolean check_init_type(Type *t, Integer &newlen);
	virtual void mark_symbol(boolean read, boolean wrote, boolean ref);
	virtual TreeNode *mktree();
	virtual TreeNode *mkinit();
	virtual LValueTreeNode *mklval();
	virtual void debug();
};

class Operator3_node : public Expr_node
{
protected:
	int _token;
	Expr_node *_arg1;
	Expr_node *_arg2;
	Expr_node *_arg3;

public:
	Operator3_node(Scope *s, int tok, Expr_node *a1, Expr_node *a2,
			Expr_node *a3);
	virtual ~Operator3_node();
	int token() { return _token; }
	Expr_node *arg1() { return _arg1; }
	Expr_node *arg2() { return _arg2; }
	Expr_node *arg3() { return _arg3; }
	virtual Node_type nodetype();
	virtual void eval_const_expr(Const_expr &val);
	virtual boolean is_int_expr();
	virtual boolean is_static_init(int *ref = NULL);
	virtual void mark_symbol(boolean read, boolean wrote, boolean ref);
	virtual TreeNode *mktree();
	virtual TreeNode *mkinit();
	virtual LValueTreeNode *mklval();
	virtual void debug();
};

class Cast_node : public Expr_node
{
protected:
	Expr_node *_cast;

public:
	Cast_node(Scope *s, Type *type, Expr_node *cast);
	virtual ~Cast_node();
	Expr_node *cast() { return _cast; }
	virtual Node_type nodetype();
	virtual void eval_const_expr(Const_expr &val);
	virtual boolean is_int_expr();
	virtual boolean is_static_init(int *ref = NULL);
	virtual void mark_symbol(boolean read, boolean wrote, boolean ref);
	virtual const char *name();
	virtual TreeNode *mktree();
	virtual TreeNode *mkinit();
	virtual LValueTreeNode *mklval();
	virtual void debug();
};

class Initializer_node : public Expr_node
{
protected:
	Expr_arr _init;
	int _next;

public:
	Initializer_node(Scope *s);
	virtual ~Initializer_node();
	Expr_arr &init() { return _init; }
	void append(Expr_node *elt, Expr_node *indx = NULL);
	virtual Node_type nodetype();
	virtual boolean is_static_init(int *ref = NULL);
	virtual boolean check_init_type(Type *t, Integer &newlen);
	virtual TreeNode *mktree();
	virtual TreeNode *mkinit();
	//virtual LValueTreeNode *mklval();
	virtual void debug();
};

class New_node : public Expr_node
{
protected:
	Expr_arr *_args;
	Expr_node *_arrlen;
	Symbol *_ctor;

public:
	New_node(Scope *s, Type *t, Expr_arr *args, Expr_node *arrlen);
	virtual ~New_node();
	Expr_arr *args() { return _args; }
	void args(Expr_arr *i) { _args = i; }
	Expr_node *arrlen() { return _arrlen; }
	void arrlen(Expr_node *a) { _arrlen = a; }
	Symbol *ctor() { return _ctor; }
	void ctor(Symbol *c) { _ctor = c; }
	virtual Node_type nodetype();
	virtual void mark_symbol(boolean read, boolean wrote, boolean ref);
	//virtual boolean is_static_init(int *ref = NULL);
	//virtual boolean check_init_type(Type *t, Integer &newlen);
	virtual TreeNode *mktree();
	//virtual TreeNode *mkinit();
	//virtual LValueTreeNode *mklval();
	virtual void debug();
};

class Delete_node : public Expr_node
{
protected:
	Expr_node *_expr;
	Expr_node *_arrexpr;
	Symbol *_dtor;
	boolean _array;

public:
	Delete_node(Scope *s, Expr_node *e, Expr_node *ae, boolean a);
	virtual ~Delete_node();
	void expr(Expr_node *e) { _expr = e; }
	Expr_node *expr() { return _expr; }
	void arrexpr(Expr_node *ae) { _arrexpr = ae; }
	Expr_node *arrexpr() { return _arrexpr; }
	Symbol *dtor() { return _dtor; }
	void dtor(Symbol *d) { _dtor = d; }
	boolean array() { return _array; }
	void array(boolean a) { _array = a; }
	virtual Node_type nodetype();
	virtual void mark_symbol(boolean read, boolean wrote, boolean ref);
	//virtual boolean is_static_init(int *ref = NULL);
	//virtual boolean check_init_type(Type *t, Integer &newlen);
	virtual TreeNode *mktree();
	//virtual TreeNode *mkinit();
	//virtual LValueTreeNode *mklval();
	virtual void debug();
};

class Label_node : public Node
{
protected:
	Symbol *_sym;
	Node *_stmt;
	int _line;

public:
	Label_node(Scope *s, Symbol *sy, Node *stmt)
			: Node(s) { _sym = sy; _stmt = stmt; }
	virtual ~Label_node();
	Symbol *sym() { return _sym; }
	Node *stmt() { return _stmt; }
	int line() { return _line; }
	virtual Node_type nodetype();
	virtual Node_type check_return();
	virtual boolean check_break();
	virtual TreeNode *mktree();
	virtual void debug();
};

class Case_node : public Node
{
protected:
	Expr_node *_expr1;
	Expr_node *_expr2;		// for GNU-style ranges
	int _line;
	Integer *_val1, *_val2;
	Node *_stmt;
	Node *_switch;
	Case_node *_nextcase;
	const char *_label;

public:
	Case_node(Scope *s, Expr_node *e1, Expr_node *e2, Node *stmt,
			Node *sw, int l) : Node(s)
		{ _expr1 = e1; _expr2 = e2; _stmt = stmt; _switch = sw; _line = l;
				_nextcase = NULL; _val1 = _val2 = NULL; _label = NULL; }
	virtual ~Case_node();
	void expr1(Expr_node *e) { _expr1 = e; }
	void expr2(Expr_node *e) { _expr2 = e; }
	Expr_node *expr1() { return _expr1; }
	Expr_node *expr2() { return _expr2; }
	Node *stmt() { return _stmt; }
	Node *switchstmt() { return _switch; }
	Case_node *nextcase() { return _nextcase; }
	void nextcase(Case_node *n) { _nextcase = n; }
	Integer *val1() { return _val1; }
	Integer *val2() { return _val2; }
	void val1(Integer *v) { _val1 = v; }
	void val2(Integer *v) { _val2 = v; }
	int line() { return _line; }
	const char *label() { return _label; }
	void label(const char *l) { _label = l; }
	virtual Node_type nodetype();
	virtual Node_type check_return();
	virtual boolean check_break();
	virtual TreeNode *mktree();
	virtual void debug();
};

class Switch_node : public Node
{
protected:
	Expr_node *_expr;
	Node *_stmt;
	Node_arr _cases;
	int _default;
	Case_node *_firstcase;
	const char *_brklabel;

public:
	Switch_node(Scope *s) : Node(s)
		{ _expr = NULL; _stmt = NULL; _default = -1; _firstcase = NULL;
				_brklabel = NULL; }
	Switch_node(Scope *s, Expr_node *expr) : Node(s)
		{ _expr = expr; _stmt = NULL; _default = -1; _firstcase = NULL;
					_brklabel = NULL; }
	virtual ~Switch_node();
	Expr_node *expr() { return _expr; }
	void expr(Expr_node *e) { _expr = e; }
	Node *stmt() { return _stmt; }
	void stmt(Node *s) { _stmt = s; }
	Node_arr &cases() { return _cases; }
	int default_case() { return _default; }
	Case_node *firstcase() { return _firstcase; }
	boolean check_cases();
	virtual Node_type check_return();
	virtual Node_type nodetype();
	virtual TreeNode *mktree();
	virtual const char *brklabel();
	virtual void debug();
};

class Compound_node : public Node
{
protected:
	Node_arr *_stmts;
	//boolean newscope;

public:
	Compound_node(Scope *s, Node_arr *stmts) : Node(s)
			{ _stmts = stmts; }
	virtual ~Compound_node();
	Node_arr *stmts() { return _stmts; }
	virtual Node_type nodetype();
	virtual Node_type check_return();
	virtual boolean check_break();
	virtual TreeNode *mktree();
	virtual void debug();
};

class If_node : public Node
{
protected:
	Expr_node *_expr;
	Node *_then;
	Node *_else;

public:
	If_node(Scope *s, Expr_node *expr, Node *thenstmt, Node *elsestmt)
			: Node(s)
			{ _expr = expr; _then = thenstmt; _else = elsestmt; }
	virtual ~If_node();
	Expr_node *expr() { return _expr; }
	Node *thenstmt() { return _then; }
	Node *elsestmt() { return _else; }
	virtual Node_type nodetype();
	virtual Node_type check_return();
	virtual boolean check_break();
	virtual TreeNode *mktree();
	virtual void debug();
};

class While_node : public Node
{
protected:
	Expr_node *_expr;
	Node *_stmt;
	const char *_brklabel, *_contlabel;

public:
	While_node(Scope *s) : Node(s)
		{ _expr = NULL; _stmt = NULL; _brklabel = NULL; _contlabel = NULL; }
	While_node(Scope *s, Expr_node *expr) : Node(s)
		{ _expr = expr; _stmt = NULL; _brklabel = NULL; _contlabel = NULL; }
	virtual ~While_node();
	Expr_node *expr() { return _expr; }
	void expr(Expr_node *e) { _expr = e; }
	Node *stmt() { return _stmt; }
	void stmt(Node *s) { _stmt = s; }
	virtual Node_type nodetype();
	virtual Node_type check_return();
	virtual TreeNode *mktree();
	virtual void debug();
	virtual const char *brklabel();
	virtual const char *contlabel();
};

class Dowhile_node : public Node
{
protected:
	Node *_stmt;
	Expr_node *_expr;
	const char *_brklabel, *_contlabel;

public:
	Dowhile_node(Scope *s) : Node(s)
		{ _expr = NULL; _stmt = NULL; _brklabel = NULL; _contlabel = NULL; }
	Dowhile_node(Scope *s, Node *stmt, Expr_node *expr) : Node(s)
		{ _expr = expr; _stmt = stmt; _brklabel = NULL; _contlabel = NULL; }
	virtual ~Dowhile_node();
	Expr_node *expr() { return _expr; }
	void expr(Expr_node *e) { _expr = e; }
	Node *stmt() { return _stmt; }
	void stmt(Node *s) { _stmt = s; }
	virtual Node_type nodetype();
	virtual Node_type check_return();
	virtual TreeNode *mktree();
	virtual const char *brklabel();
	virtual const char *contlabel();
	virtual void debug();
};

class For_node : public Node
{
protected:
	Expr_node *_expr1;
	Expr_node *_expr2;
	Expr_node *_expr3;
	Node *_stmt;
	const char *_brklabel, *_contlabel;

public:
	For_node(Scope *s) : Node(s)
			{ _expr1 = _expr2 = _expr3 = NULL; _stmt = NULL;
					_brklabel = NULL; _contlabel = NULL; }
	For_node(Scope *s, Expr_node *e1, Expr_node *e2, Expr_node *e3)
			: Node(s)
			{ _expr1 = e1; _expr2 = e2; _expr3 = e3; _stmt = NULL;
					_brklabel = NULL; _contlabel = NULL; }
	virtual ~For_node();
	Expr_node *expr1() { return _expr1; }
	void expr1(Expr_node *e) { _expr1 = e; }
	Expr_node *expr2() { return _expr2; }
	void expr2(Expr_node *e) { _expr2 = e; }
	Expr_node *expr3() { return _expr3; }
	void expr3(Expr_node *e) { _expr3 = e; }
	Node *stmt() { return _stmt; }
	void stmt(Node *s) { _stmt = s; }
	virtual Node_type nodetype();
	virtual Node_type check_return();
	virtual TreeNode *mktree();
	virtual const char *brklabel();
	virtual const char *contlabel();
	virtual void debug();
};

class Goto_node : public Node
{
protected:
	Symbol *_sym;
	int _line;

public:
	Goto_node(Scope *s, Symbol *sym, int line) : Node(s)
			{ _sym = sym; _line = line; }
	Symbol *sym() { return _sym; }
	int line() { return _line; }
	virtual Node_type nodetype();
	virtual Node_type check_return();
	virtual boolean check_break();
	virtual TreeNode *mktree();
	virtual void debug();
};

class Continue_node : public Node
{
protected:
	Node *_loop;

public:
	Continue_node(Scope *s, Node *l) : Node(s) { _loop = l; }
	Node *loop() { return _loop; }
	void loop(Node *l) { _loop = l; }
	virtual Node_type nodetype();
	virtual Node_type check_return();
	virtual boolean check_break();
	virtual TreeNode *mktree();
	virtual void debug();
};

class Break_node : public Node
{
protected:
	Node *_stmt;

public:
	Break_node(Scope *s, Node *l) : Node(s) { _stmt = l; }
	Node *stmt() { return _stmt; }
	void stmt(Node *l) { _stmt = l; }
	virtual Node_type nodetype();
	virtual boolean check_break();
	virtual TreeNode *mktree();
	virtual void debug();
};

class Return_node : public Node
{
protected:
	Expr_node *_expr;
	Symbol *_func;

public:
	Return_node(Scope *s, Expr_node *expr);
	virtual ~Return_node();
	Expr_node *expr() { return _expr; }
	Symbol *func() { return _func; }
	virtual Node_type check_return();
	virtual boolean check_break();
	virtual Node_type nodetype();
	virtual TreeNode *mktree();
	virtual void debug();
};

class Asm_node : public Node
{
protected:
	String_node *_asmstr;
	const char *_file;
	int _line;

public:
	Asm_node(Scope *s, String_node *asmstr, const char *file, int line)
			: Node(s) { _asmstr = asmstr; _file = file; _line = line; }
	virtual ~Asm_node();
	String_node *asmstr() { return _asmstr; }
	const char *file() { return _file; }
	int line() { return _line; }
	virtual Node_type nodetype();
	virtual TreeNode *mktree();
	virtual void debug();
};

extern Expr_node *do_implicit_cast(Expr_node *e, Type *type);

inline Expr_node *
implicit_cast(Expr_node *e, Type *type)
{
	return (e == NULL) ? g_null_expr :
			(e->type() == type || e->type() == g_unknown_type) ?
			e : do_implicit_cast(e, type);
}

extern void init_nodes();
extern boolean check_init_type(Expr_node *&expr, Type *t, Integer &newlen);
extern boolean valid_ecast(Expr_node *&efrom, Type *to);
extern boolean valid_opcast(Type *castfrom, Type *to);

extern const char *mksymbolname(Symbol &s);
extern const char *mklabelname(Symbol &s);

extern const char *op2str(int tok);
extern Expr_node *make_array_ref_node(Scope *s, Expr_node *arr,
		Expr_node *expr);
extern Expr_node *make_func_call_node(Scope *s, Expr_node *f, Expr_arr *a);
extern Expr_node *make_struct_ref_node(Scope *s, Expr_node *sref,
		const char *tag, const char *file, int line, boolean pointer);
extern Expr_node *make_operator_node(Scope *s, int tok, Expr_node *a,
		boolean p = TRUE);
extern Expr_node *make_operator2_node(Scope *s, int tok, Expr_node *a1,
		Expr_node *a2);


declare_array(Treevec, TreeNode*, 256);

#endif // __CCNODE_H_
