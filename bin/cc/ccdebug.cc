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
// $Header: /u/cgt/cvs/bin/cc/ccdebug.cc,v 1.43 2003/04/29 21:56:09 parag Exp $

#include "cctype.h"
#include "ccsym.h"
#include "ccnode.h"
#include "cc.h"


extern void dumpcexpr(Const_expr *ce);

void
dumpcexpr(Const_expr *ce)
{
	fprintf(stderr, "const-expr: ");

	if (ce->sym)
		fprintf(stderr, "sym=%s ", ce->sym->name());

	if (ce->isint)
		fprintf(stderr, "int=%s ",
				ce->ival ? integertostr(*ce->ival) : "<none>");
	else
		fprintf(stderr, "float=%s ",
				ce->fval ? floattostr(*ce->fval) : "<none>");

	fprintf(stderr, "\n");
}

static void
dbg(Type *t)
{
	fprintf(stderr, "<%p:%d>", t, t->typeval());
	fprintf(stderr, "[%s] ", integertostr(t->getsize()));

	if (!t->inpool())
		fprintf(stderr, "@");

	if (t->isconst())
		fprintf(stderr, "const ");

	if (t->isvolatile())
		fprintf(stderr, "volatile ");
}

void
Type::debug()
{
	dbg(this);
	fprintf(stderr, "<no-type>\n");
}

void
Void_type::debug()
{
	dbg(this);
	fprintf(stderr, "void\n");
}

void
Int_type::debug()
{
	dbg(this);
	fprintf(stderr, "%s:%d\n", _sign ? "int" : "uint", _ic->bits());
}

void
Float_type::debug()
{
	dbg(this);
	fprintf(stderr, "float:%d\n", _fc->bits());
}

void
Array_type::debug()
{
	dbg(this);

	if (_complete)
		fprintf(stderr, "complete ");

	fprintf(stderr, "array[%s] of ", integertostr(_len));

	if (_type != NULL)
		_type->debug();
}

void
Pointer_type::debug()
{
	dbg(this);
	fprintf(stderr, "pointer to ");

	if (_type != NULL)
		_type->debug();
}

void
Bitfield_type::debug()
{
	dbg(this);
	fprintf(stderr, "%s:%d<%d>\n", _sign ? "bitfield" : "ubitfield",
			_width, _ic->bits());
}

void
Function_type::debug()
{
	dbg(this);
	fprintf(stderr, "function ");

	if (_retval != NULL)
	{
		fprintf(stderr, "returning ");
		_retval->debug();
	}

	if (_args != NULL)
	{
		fprintf(stderr, "  _args is <%p>\n", _args);
		Param_arr &args = *_args;

		for (int i = 0; i < args.size(); i++)
		{
			fprintf(stderr, "	arg %d: \"%s\" ", i + 1,
					args[i].symbol() ? args[i].symbol() : "<null>");

			if (args[i].storage() == REGISTER)
				fprintf(stderr, "register ");
			else if (args[i].storage() == '&')
				fprintf(stderr, "& ");
			else if (args[i].storage() != 0)
				fprintf(stderr, "illegal-storage:%d ", args[i].storage());

			if (args[i].type() != NULL)
				args[i].type()->debug();
			else
				fprintf(stderr, "bug:NULL-type\n");
		}

		if (_varargs)
			fprintf(stderr, "...\n");
	}
	else
		fprintf(stderr, "	taking unspecified arguments\n");
}

void
Overload_type::debug()
{
	dbg(this);
	fprintf(stderr, "overload type for %s\n", _sym->name());
}

void
Struct_type::debug()
{
	dbg(this);

	if (complete())
		fprintf(stderr, "complete ");

	fprintf(stderr, "%s %s\n", is_class() ? "class" : "struct", name());
}

void
Union_type::debug()
{
	dbg(this);

	if (complete())
		fprintf(stderr, "complete ");

	fprintf(stderr, "union %s\n", name());
}

void
Enum_type::debug()
{
	dbg(this);

	if (complete())
		fprintf(stderr, "complete ");

	fprintf(stderr, "enum %s\n", name());
}

static void
dumptype(Type *t)
{
	if (t == NULL)
		fprintf(stderr, "type==NULL\n");
	else
		t->debug();
}

static void
dumpsym(Symbol *sym)
{
	if (sym)
	{
		fprintf(stderr, "sym \"%s\" is ", sym->name());

		if (sym->storage() == ENUM)
			fprintf(stderr, "enumval=%s",
					integertostr(*sym->member()->enumval()));
		else
		{
			if (sym->storage() == '&')
				fprintf(stderr, "& ");
			else
				dumptype(sym->type());
		}
	}
	else
		fprintf(stderr, "sym==NULL\n");
}

static void
dumpsymtab(const char *str, Symbol_tab &tab, Scope *scope)
{
	fprintf(stderr, "\ntable %s for scope %p...\n", str, scope);
	int count = 1;

	for (Symbol_iter i = tab; i; ++i)
	{
		fprintf(stderr, "\nsymbol %d of scope %p is chain[%ld]...\n",
				count++, scope, i.chain());

		if (scope && i().scope() && scope != i().scope() &&
				(i().type()->isfunc() || i().type()->isclass()))
		{
			fprintf(stderr, "	symbol \"%s\" has scope %p...\n",
					i().name(), i().scope());
			i().scope()->debug();
		}

		i().debug();
	}

}

void
Symbol::debug()
{
	fprintf(stderr, "symbol \"%s\" line=%d storage=%d read=%d wrote=%d ref=%d\n"
			, _name, _line, _storage, _read, _wrote, _referenced);
	fprintf(stderr, "	scope=%p ", _scope);
	dumptype(_type);

	if (_storage == ENUM)
	{
		fprintf(stderr, "	enumval=%s\n", integertostr(*_member->enumval()));
		return;
	}

	if (_type->issu())
	{
		Struct_type *ts = (Struct_type*)_type;

		for (Struct_type *s = ts->base(); s; s = s->base())
		{
			fprintf(stderr, "	derived from ");
			dumptype(s);
		}

		if (ts->memlist() == NULL)
			fprintf(stderr, "	<no members>\n");
		else
			for (int i = 0; i < ts->memlist()->size(); i++)
			{
				Symbol &mem = *ts->memlist()->elt(i);
				fprintf(stderr, "   [%d] %s (line %d) is ", i,
						mem.name(), mem.line());

				if (mem.offset() != NULL)
					fprintf(stderr, "offset %s ", integertostr(*mem.offset()));

				if (mem.pad() != NULL)
					fprintf(stderr, "pad %s ", integertostr(*mem.pad()));

				dumptype(mem.type());
			}
	}
	else if (_type->isenum())
	{
		Enum_type *te = (Enum_type*)_type;

		if (te->members() == NULL)
			fprintf(stderr, "	<no members>\n");
		else
		{
			int i = 1;

			for (Symbol_iter m = *te->members(); m; ++m, i++)
				fprintf(stderr, "   [%d] member %s (line %d) is %s\n", i,
						m().name(), m().line(), integertostr(*m().enumval()));
		}
	}

	if (_type->isfunc() && _labels)
		dumpsymtab("labels", *_labels, _scope);

	if (_overloaded)
	{
		for (int i = 0; i < _overloads->size(); i++)
		{
			fprintf(stderr, "	overload[%d] = ", i + 1);
			dumpsym(_overloads->elt(i));
		}
	}
	else if (_type->isenum())
	{
		;
	}
	else if (_node)
	{
		fprintf(stderr, "	");
		_node->debug();
	}
}

void
Scope::debug()
{
	fprintf(stderr, "scope %p file=%s line=%d parent=%p node=%p\n",
			this, _fname, _fline, _parent, _node);

	if (_symtab)
		dumpsymtab("symtab", *_symtab, this);

	if (_suetypes)
		dumpsymtab("suetypes", *_suetypes, this);

	if (_init)
	{
		fprintf(stderr, "\ninit array for scope %p...\n", this);

		for (int i = 0; i < _init->size(); i++)
			if (_init->elt(i))
			{
				fprintf(stderr, "  [%d] ", i);
				dumpsym(_init->elt(i));
			}
	}

	if (_fini)
	{
		fprintf(stderr, "\nfini array for scope %p...\n", this);

		for (int i = 0; i < _fini->size(); i++)
			if (_fini->elt(i))
			{
				fprintf(stderr, "  [%d] ", i);
				dumpsym(_fini->elt(i));
			}
	}

	if (_children)
	{
		fprintf(stderr, "scope %p children...\n", this);

		for (int i = 0; i < _children->size(); i++)
			//_children->elt(i)->debug();
			fprintf(stderr, "	child[%d] = %p\n", i, _children->elt(i));
	}
	else
		fprintf(stderr, "scope %p has no children\n", this);

	fprintf(stderr, "\n");
}

void
Node::debug()
{
	fprintf(stderr, "Node %p: scope=%p\n", this, _scope);
}

void
Expr_node::debug()
{
	fprintf(stderr, "Expr_node %p: scope=%p ", this, _scope);
	dumptype(_type);
}

void
Integer_node::debug()
{
	fprintf(stderr, "Integer_node %p: scope=%p val=%s ",
			this, _scope, _val ? integertostr(*_val) : "<null>");
	dumptype(_type);
}

void
Sizeof_node::debug()
{
	fprintf(stderr, "Sizeof_node %p: scope=%p size=%s ",
			this, _scope, integertostr(_size));
	dumptype(_type);
}

void
Float_node::debug()
{
	fprintf(stderr, "Float_node %p: scope=%p double=%s ",
			this, _scope, floattostr(*_val));
	dumptype(_type);
}

void
String_node::debug()
{
	fprintf(stderr, "String_node %p: scope=%p str=\"%s\" len=%d "
			"wide=%c pascal=%c ",
			this, _scope, _string, (int)_len, _wide ? 'T' : 'F',
			_pascal ? 'T' : 'F');
	dumptype(_type);
}

void
Symbol_node::debug()
{
	fprintf(stderr, "Symbol_node %p: scope=%p line=%d ",
			this, _scope, _line);
	dumptype(_type);
	dumpsym(_sym);
}

void
dumpnode(Node *n)
{
	n->debug();
}

void
dumpstrnode(const char *s, Node *n)
{
	if (n == NULL)
	{
		fprintf(stderr, "%s==NULL\n", s);
		return;
	}

	fprintf(stderr, "%s is ", s);
	n->debug();
}

void
Array_ref_node::debug()
{
	fprintf(stderr, "Array_ref_node %p: scope=%p ",
			this, _scope);
	dumptype(_type);
	dumpstrnode("arr", _arr);
	dumpstrnode("expr", _expr);
}

static void
dumpexprarr(const char *str, Expr_arr *arr)
{
	if (arr == NULL)
	{
		fprintf(stderr, "Expr_arr %s is NULL\n", str);
		return;
	}

	fprintf(stderr, "Expr_arr %s...\n", str);

	for (int i = 0; i < arr->size(); i++)
		if (arr->elt(i))
		{
			fprintf(stderr, "  expr[%d] ", i);
			arr->elt(i)->debug();
		}
}

void
Func_call_node::debug()
{
	fprintf(stderr, "Func_call_node %p: scope=%p func=%p ",
			this, _scope, _func);
	dumptype(_type);
	dumpstrnode("func", _func);
	dumpexprarr("args", _args);
}

void
Struct_ref_node::debug()
{
	fprintf(stderr, "Struct_ref_node %p: scope=%p line=%d ",
			this, _scope, _line);
	dumptype(_type);
	dumpstrnode("structref", _structref);

	if (_elem)
	{
		fprintf(stderr, "	elem=%s line=%d ", _elem->name(), _elem->line());
		dumptype(_elem->type());
	}
	else
		fprintf(stderr, "	elem is NULL\n");
}

void
Operator_node::debug()
{
	fprintf(stderr,
			"Operator_node %p: scope=%p token=%d(%c) prefix=%c ",
			this, _scope, _token, _token, _prefix ? 'T' : 'F');
	dumptype(_type);
	dumpstrnode("op1 arg", _arg);
}

void
Operator2_node::debug()
{
	fprintf(stderr, "Operator2_node %p: scope=%p token=%d(%c) ",
			this, _scope, _token, _token);
	dumptype(_type);
	dumpstrnode("op2 arg1", _arg1);
	dumpstrnode("op2 arg2", _arg2);
}

void
Operator3_node::debug()
{
	fprintf(stderr, "Operator3_node %p: scope=%p token=%d(%c) ",
			this, _scope, _token, _token);
	dumptype(_type);
	dumpstrnode("op3 arg1", _arg1);
	dumpstrnode("op3 arg2", _arg2);
	dumpstrnode("op3 arg3", _arg3);
}

void
Cast_node::debug()
{
	fprintf(stderr, "Cast_node %p: scope=%p ", this, _scope);
	dumptype(_type);
	dumpstrnode("cast", _cast);
}

void
Initializer_node::debug()
{
	fprintf(stderr, "Initializer_node %p: scope=%p ", this, _scope);
	dumptype(_type);
	dumpexprarr("init", &_init);
}

void
New_node::debug()
{
	fprintf(stderr, "New_node %p: scope=%p ", this, _scope);
	dumptype(_type);
	dumpexprarr("args", _args);
	dumpstrnode("arrlen", _arrlen);

	if (_ctor)
		dumpsym(_ctor);
}

void
Delete_node::debug()
{
	fprintf(stderr, "Delete_node %p: scope=%p array=%c",
			this, _scope, _array ? 'T' : 'F');
	dumptype(_type);
	dumpstrnode("expr", _expr);
	dumpstrnode("arrexpr", _arrexpr);

	if (_dtor)
		dumpsym(_dtor);
}

void
Label_node::debug()
{
	fprintf(stderr, "Label_node %p: scope=%p line=%d\n",
			this, _scope, _line);
	dumpstrnode("label stmt", _stmt);
	dumpsym(_sym);
}

void
Case_node::debug()
{
	fprintf(stderr, "Case_node %p: scope=%p line=%d val1=%s\n",
			this, _scope, _line,
			_val1 ? integertostr(*_val1) : "<null>");
	fprintf(stderr, " val2=%s\n", _val2 ? integertostr(*_val2) : "<null>");
	dumpstrnode("case expr1", _expr1);
	dumpstrnode("case expr2", _expr2);
	dumpstrnode("case stmt", _stmt);
	fprintf(stderr, "  switch=%p nextcase=%p\n", _switch, _nextcase);
}

static void
dumpstrnodearr(const char *s, Node_arr *arr)
{
	if (arr == NULL)
	{
		fprintf(stderr, "Node_arr %s is NULL\n", s);
		return;
	}

	fprintf(stderr, "Node_arr %s...\n", s);

	for (int i = 0; i < arr->size(); i++)
		if (arr->elt(i))
		{
			fprintf(stderr, "  [%d] ", i);
			arr->elt(i)->debug();
		}
}

void
Switch_node::debug()
{
	fprintf(stderr, "Switch_node %p: scope=%p firstcase=%p\n",
			this, _scope, _firstcase);
	dumpstrnode("switch expr", _expr);
	dumpstrnode("switch stmt", _stmt);
	fprintf(stderr, "switch cases %p...\n", &_cases);

	for (int i = 0; i < _cases.size(); i++)
		if (_cases.elt(i))
			fprintf(stderr, " [%d] %p", i, _cases.elt(i));

	fprintf(stderr, "\n");
}

void
Compound_node::debug()
{
	fprintf(stderr, "Compound_node %p: scope=%p\n",
			this, _scope);
	dumpstrnodearr("compound stmts", _stmts);
}

void
If_node::debug()
{
	fprintf(stderr, "If_node %p: scope=%p\n",
			this, _scope);
	dumpstrnode("if expr", _expr);
	dumpstrnode("if then", _then);
	dumpstrnode("if else", _else);
}

void
While_node::debug()
{
	fprintf(stderr, "While_node %p: scope=%p\n",
			this, _scope);
	dumpstrnode("while expr", _expr);
	dumpstrnode("while stmt", _stmt);
}

void
Dowhile_node::debug()
{
	fprintf(stderr, "Dowhile_node %p: scope=%p\n",
			this, _scope);
	dumpstrnode("dowhile stmt", _stmt);
	dumpstrnode("dowhile expr", _expr);
}

void
For_node::debug()
{
	fprintf(stderr, "For_node %p: scope=%p\n",
			this, _scope);
	dumpstrnode("for expr1", _expr1);
	dumpstrnode("for expr2", _expr2);
	dumpstrnode("for expr3", _expr3);
	dumpstrnode("for stmt", _stmt);
}

void
Goto_node::debug()
{
	fprintf(stderr, "Goto_node %p: scope=%p line=%d ",
			this, _scope, _line);
	dumpsym(_sym);
}

void
Continue_node::debug()
{
	fprintf(stderr, "Continue_node %p: scope=%p loop=%p\n",
			this, _scope, _loop);
}

void
Break_node::debug()
{
	fprintf(stderr, "Break_node %p: scope=%p loop=%p\n",
			this, _scope, _stmt);
}

void
Return_node::debug()
{
	fprintf(stderr, "Return_node %p: scope=%p\n",
			this, _scope);
	dumpstrnode("return expr", _expr);
}

void
Asm_node::debug()
{
	fprintf(stderr, "Asm_node %p: scope=%p line=%d asmstr=",
			this, _scope, _line);
	_asmstr->debug();
}
