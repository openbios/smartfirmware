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

// Copyright (c) 1991-2002 by Parag Patel.  All Rights Reserved.

// Code generator for C/C++ functions (original back-end).

#include "defs.h"
#include "toks.h"
#include "gen.h"
#include "genc.h"
#include <stdarg.h>

GenC::GenC()
{
	currfile = NULL;
	inputfile = NULL;
	loop_label = 1;
	voidstr = strget("void");
}

GenC::~GenC()
{
}

const char *
GenC::mkname(Symbol *sym)
{
	if (*sym->name != '"')
		return sym->name;

	return strbldf("W_S%d/*%s*/", sym->id, sym->name);
}

const char *
GenC::mkerrname(Symbol *sym)
{
	if (sym->type == NONTERMINAL && sym->realname != NULL)
		return sym->realname;

	return mkname(sym);
}


void
GenC::printset(const char *name, Bitset &set)
{
	Symbol *s;
	int id = 0;
	const char *pre = "";
	putf("static int %s[] = { ", name);

	for (; id < set.size(); id++)
	{
		if (!set.has(id))
			continue;

		if (id == EMPTY)
			putf("%sW_EMPTY", pre);
		else if (id == END)
			putf("%sW_EOI", pre);
		else
		{
			s = getsymbol(id);
			putf("%s%s", pre, mkname(s));
		}

		pre = ", ";
	}

	putf("%s-1 };\n", pre);
}

void
GenC::printcase(int indent, Bitset &set)
{
	Symbol *s;
	int id = 0;

	for (; id < set.size(); id++)
	{
		if (!set.has(id))
			continue;

		if (id == EMPTY)
			continue;

		if (id == END)
			putf(indent, "case W_EOI:\n");
		else
		{
			s = getsymbol(id);
			putf(indent, "case %s:\n", mkname(s));
		}
	}
}

const char *
GenC::mktype(Symbol *sym, const char *pre, const char *post)
{
	if (!dotype(sym))
		return "";

	const char *name;

	if (sym->rettype == NULL)
		name = "int";
	else
	{
		name = sym->rettype;

		if (sym->mkstruct)
		{
			if (sym->realname != NULL && sym->realname != sym->name
					&& sym->rettype == findsymbol(sym->realname)->rettype)
				return strbldf("%sstruct W_T%s%s", pre, sym->realname, post);

			return strbldf("%sstruct W_T%s%s", pre, sym->name, post);
		}
	}

	return strbldf("%s%s%s", pre, name, post);
}

const char *
GenC::mkvoidtype(Symbol *sym, const char *pre, const char *post)
{
	const char *type = mktype(sym, pre, post);

	if (*type == '\0')
		type = "void";

	return type;
}

void
GenC::mkstruct(Symbol *sym)
{
	if (!dotype(sym) || !sym->mkstruct)
		return;

	const char *name = sym->name;

	if (sym->realname != NULL && sym->realname != sym->name)
	{
		Symbol *real = findsymbol(sym->realname);

		if (real->rettype == sym->rettype)
		{
			if (dotype(real))
				return;

			name = sym->realname;
		}
	}

	putf("struct W_T%s\n{\n", name);
	putf(1, "%s;\n};\n\n", sym->rettype);
}

void
GenC::mkcode(int indent, Symbol *code)
{
	if (!dumpcode || code == NULL || code->code == NULL)
		return;

	if (genlineinfo)
		putf("#line %d \"%s\"\n", code->line, inputfile);
	else
		putf("/* #line %d */\n", code->line);

	// output code directly to avoid overflowing putf buffers
	putindent(indent);
	fputs(code->code, fp);

	for (const char *s = code->code; *s != '\0'; s++)
		if (*s == '\n')
			currline++;

	putf("\n");

	if (genlineinfo)
		putf("#line %d \"%s\"\n", currline + 1, currfile);
}

const char *
GenC::mkvarname(Symnode *start, Symnode *node)
{
	if (node->alias != NULL)
		return node->alias;

	int count = 0;
	boolean addnum = FALSE;
	const char *name = node->sym->name;

	if (node->sym->realname != NULL && node->sym->name != node->sym->realname)
	{
		name = "_";
		Symnode *n;

		for (n = start; n != node; n = n->next)
			if (n->sym->type == NONTERMINAL && n->sym->realname != NULL
					&& n->sym->name != n->sym->realname)
				count++;

		for (n = n->next; n != NULL; n = n->next)
			if (n->sym->type == NONTERMINAL && n->sym->realname != NULL
					&& n->sym->name != n->sym->realname)
				addnum = TRUE;
	}
	else
	{
		Symnode *n;

		for (n = start; n != node; n = n->next)
			if (n->sym == node->sym)
				count++;

		for (n = n->next; n != NULL; n = n->next)
			if (n->sym == node->sym)
				addnum = TRUE;
	}

	if (addnum || count > 0)
		return strbldf("%s%d", name, count + 1);

	return name;
}

void
GenC::printtestexpr(int indent, Bitset &set, const char *post)
{
	boolean first = TRUE;
	int id = 0;

	for (; id < set.size(); id++)
	{
		if (!set.has(id) || id == EMPTY)
			continue;

		if (first)
		{
			putf("w_nexttoken() == ");
			first = FALSE;
		}
		else
		{
			putf(" ||\n");

			if (dumponlyparser)
				putf(indent + 2, "w_nexttoken() == ");
			else
				putf(indent + 2, "w_currtoken == ");
		}

		if (id == END)
			putf("W_EOI");
		else
			putf("%s", mkname(getsymbol(id)));
	}

	if (first)
		putf("1");

	putf(")%s\n", post);
}

void
GenC::printwhile(int indent, Bitset &set, const char *post)
{
	putf("while (");
	printtestexpr(indent, set, post);
}

void
GenC::printif(int indent, Bitset &set, const char *post)
{
	putf("if (");
	printtestexpr(indent, set, post);
}

void
GenC::loopbegin(int &indent, int doloop, Symbol *sym)
{
	if (doloop == 0)
		return;

	if (sym->type == TERMINAL || sym->first->count() < 4)
	{
		switch (doloop)
		{
		case ZEROPLUS:
			putindent(indent);
			printwhile(indent, *sym->first, "");
			break;

		case ONEPLUS:
			putf(indent, "do\n");
			break;

		case ONEZERO:
			putindent(indent);
			printif(indent, *sym->first, "");
			break;

		default:
			error("Unexpected doloop value %d", doloop);
			break;
		}

		putf(indent++, "{\n");
	}
	else
	{
		if (doloop == ONEPLUS)
			putf(indent, "goto w_oneplus_%d;\n\n", loop_label);

		if (doloop == ZEROPLUS || doloop == ONEPLUS)
		{
			putf(indent, "for (;;)\n");
			putf(indent++, "{\n");
		}

		putf(indent, "switch (w_nexttoken())\n");
		putf(indent, "{\n");

		if (doloop == ONEPLUS)
			putf(indent, "w_oneplus_%d:\n", loop_label++);

		for (int id = 0; id < sym->first->size(); id++)
		{
			if (!sym->first->has(id) || id == EMPTY)
				continue;

			if (id == END)
				putf(indent, "case W_EOI:\n");
			else
				putf(indent, "case %s:\n", mkname(getsymbol(id)));
		}

		indent++;
	}
}

void
GenC::loopend(int &indent, int doloop, Symbol *sym)
{
	if (doloop == 0)
		return;

	if (sym->type == TERMINAL || sym->first->count() < 4)
	{
		putindent(--indent);

		if (doloop == ONEPLUS)
		{
			putf("} ");
			printwhile(indent, *sym->first, ";");
		}
		else
			putf("}\n");
	}
	else
	{
		if (doloop == ONEZERO)
			putf(indent, "break;\n\n");
		else
			putf(indent, "continue;\n\n");

		putf(indent - 1, "default:\n");
		putf(indent, "break;\n");
		putf(--indent, "}\n\n");

		if (doloop != ONEZERO)
		{
			putf(indent, "break; /*for(;;)*/\n");
			putf(--indent, "}\n\n");
		}
	}
}

void
GenC::genstmt(int indent, Symnode *node, boolean saveret)
{
	Symbol *s;
	Symnode *n;
	putf(indent++, "{\n");

	for (n = node; n != NULL; n = n->next)
	{
		if (n->sym->type != NONTERMINAL || n->sym->id == EMPTY
				|| !dotype(n->sym))
			continue;

		putf(indent, "%s w_rv%s;\n", mktype(n->sym), mkvarname(node, n));
	}

	for (n = node; n != NULL; n = n->next)
	{
		s = n->sym;
		int doloop = 0;

		if (s->type == CODE)
		{
			mkcode(indent, s);
			continue;
		}

		if (s->type == ZEROPLUS || s->type == ONEPLUS || s->type == ONEZERO)
		{
			doloop = s->type;
			n = n->next;
			s = n->sym;
		}

		if (s->id == EMPTY)
			continue;

		if (s->type == TERMINAL)
		{
			loopbegin(indent, doloop, s);

			if (doresync && n->resync)
				putf(indent, "%sw_scantoken(%s, &w_link, w_resync%d);\n",
						mkretcode(saveret), mkname(s), n->resync->id);
			else
				putf(indent, "%sw_scantoken(%s);\n",
						mkretcode(saveret), mkname(s));

			loopend(indent, doloop, s);
		}
		else if (optimize && (s->usecount == 1 || s->doinline) && !s->exportsym)
		{
			Symbol *sym = s;
			putf("\n");
			putf(indent++, "{/*%s*/\n", sym->name);

			if (dotype(sym))
				putf(indent, "%s = &w_rv%s;\n", mktype(sym, "", " *w_rr"),
						mkvarname(node, n));

			if (doresync && n->resync)
			{
				if (sym->doresync)
					putf(indent, "struct w_resynclink *w_lnk = &w_link;\n");

				putf(indent, "struct w_resynclink w_link;\n");
			}

			boolean dosave = (sym->usedret & RET_CODE) ? TRUE : FALSE;

			if (dosave)
				putf(indent, "int w_rc = W_RETOK;\n");

			mkcode(indent, sym->symcode);
			putf("\n");

			if (doresync && n->resync)
			{
				putf(indent, "w_link.prev = %s;\n",
						sym->doresync ? "w_lnk" : "NULL");
				putf(indent, "w_link.resync = w_resync%d;\n\n", n->resync->id);

				if (sym->resync != NULL)
					putf(indent, "%sw_scantoken(-1, &w_link, w_resync%d);\n\n",
							mkretcode(saveret), sym->resync->id);
			}

			if (sym->node != NULL && sym->node->orelse == NULL)
			{
				loopbegin(indent, doloop, sym);
				genstmt(indent, sym->node, dosave);
				loopend(indent, doloop, sym);
				mkcode(indent, sym->endcode);

				if (saveret)
					putf(indent, "w_rc = W_RETOK;\n");

				putf(--indent, "}/*%s*/\n\n", sym->name);
				continue;
			}

			loopbegin(indent, doloop, sym);
			boolean donedefault = FALSE;
			putf(indent, "switch (w_nexttoken())\n");
			putf(indent++, "{");
			Bitset set(numsymbols());

			for (Symnode * nn = sym->node; nn != NULL; nn = nn->orelse)
			{
				set.clear();

				for (Symnode * ss = nn; ss != NULL; ss = ss->next)
				{
					if (ss->sym->type >= CODE)
						continue;

					set |= *ss->sym->first;

					if (!ss->sym->first->has(EMPTY))
						break;
				}

				if (set.count() == 0 || (set.count() == 1 && set.has(EMPTY)))
				{
					donedefault = TRUE;
					putf("\n");
					putf(indent, "default:\n");
					genstmt(indent + 1, nn, dosave);

					if (!sym->first->has(EMPTY))
					{
						putf(indent + 1, "w_err_illegal(\"%s\");\n",
								mkerrname(sym));

						if (saveret)
							putf(indent + 1, "w_rc = W_RETERR;\n");
					}
				}
				else
				{
					putf("\n");
					printcase(indent, set);
					genstmt(indent + 1, nn, dosave);
				}

				putf(indent + 1, "break;\n");
			}

			if (!donedefault && !sym->first->has(EMPTY))
			{
				putf("\n");
				putf(indent, "default:\n");
				putf(indent + 1, "w_err_illegal(\"%s\");\n", mkerrname(sym));

				if (saveret)
					putf(indent + 1, "w_rc = W_RETERR;\n");

				putf(indent + 1, "break;\n");
			}

			putf(--indent, "}\n");
			loopend(indent, doloop, sym);
			mkcode(indent, sym->endcode);

			if (saveret)
				putf(indent, "w_rc = W_RETOK;\n");

			putf(--indent, "}/*%s*/\n\n", sym->name);
		}
		else
		{
			loopbegin(indent, doloop, s);

			if (dotype(s))
			{
				if (doresync && s->doresync && n->resync)
					putf(indent, "%sw_f%s(&w_link, w_resync%d, &w_rv%s);\n",
							mkretcode(saveret), mkname(s),
							n->resync->id, mkvarname(node, n));
				else
					putf(indent, "%sw_f%s(&w_rv%s);\n", mkretcode(saveret),
							mkname(s), mkvarname(node, n));
			}
			else
			{
				if (doresync && s->doresync && n->resync)
					putf(indent, "%sw_f%s(&w_link, w_resync%d);\n",
							mkretcode(saveret), mkname(s), n->resync->id);
				else
					putf(indent, "%sw_f%s();\n", mkretcode(saveret), mkname(s));
			}

			loopend(indent, doloop, s);
		}
	}

	putf(--indent, "}\n\n");
}

void
GenC::genheader(void)
{
	Symbol *sym;
	int i;

	putf("#ifndef __TOKENS_H_\n");
	putf("#define __TOKENS_H_\n\n");
	putf("#ifndef EOF\n");
	putf("#include <stdio.h>\n");
	putf("#endif\n");
	putf("#ifndef NULL\n");
	putf("#include <stddef.h>\n");
	putf("#endif\n\n");
	putf("#define W_RETOK 1\n");
	putf("#define W_RETERR 0\n\n");

	putf(0, "#define W_EOI 0\n");
	putf(0, "#define W_EMPTY 256\n");
	int tokid = 257;

	for (i = START; i < numsymbols(); i++)
	{
		sym = getsymbol(i);

		if (sym->type != TERMINAL || *sym->name == '\'')
			continue;

		putf(0, "#define %s %d\n", mkname(sym), tokid++);
	}

	putf(0, "#define W_LAST_TOKEN %d\n", tokid);
	putf("\n");

	if (gotlexstr)
	{
		putf("#ifdef __cplusplus\n");
		putf("extern \"C\" {\n");
		putf("#endif\n");
		putf("#if defined(__cplusplus) || defined(__STDC__)\n");
		putf("extern int yylex(void);\n");
		putf("#else\n");
		putf("extern int yylex();\n");
		putf("#endif\n");
		putf("#ifdef __cplusplus\n");
		putf("}\n");
		putf("#endif\n\n");

		putf("#ifdef FLEX_SCANNER\n");
		putf("#   undef YY_INPUT\n");
		putf("#   define YY_INPUT(buf,result,max_size) \\\n");
		putf("        result = (buf[0] = w_input()) == W_EOI ? YY_NULL : 1;\n");
		putf("#else\n");
		putf("#   undef input\n");
		putf("#   define input w_input\n");
		putf("#   undef unput\n");
		putf("#   define unput w_unput\n");
		putf("#endif\n\n");
	}

	putf("#ifdef __cplusplus\n");
	putf("extern \"C\" {\n");
	putf("#endif\n");
	putf("#if defined(__cplusplus) || defined(__STDC__)\n");
	putf("extern void w_closefile(void);\n");
	putf("extern int w_openfile(char *);\n");
	putf("extern FILE *w_getfile(void);\n");
	putf("extern void w_setfile(FILE *);\n");
	putf("extern int w_currcol(void);\n");
	putf("extern int w_currline(void);\n");
	putf("extern char *w_getcurrline(void);\n");
	putf("extern int w_input(void);\n");
	putf("extern int w_unput(int);\n");
	putf("extern void w_output(int);\n");
	putf("extern void w_scanerr(const char *fmt, ...);\n");
	putf("#else /* K&R C */\n");
	putf("extern w_closefile();\n");
	putf("extern int w_openfile();\n");
	putf("extern FILE *w_getfile();\n");
	putf("extern w_setfile();\n");
	putf("extern int w_currcol();\n");
	putf("extern int w_currline();\n");
	putf("extern char *w_getcurrline();\n");
	putf("extern int w_input();\n");
	putf("extern int w_unput();\n");
	putf("extern w_output();\n");
	putf("extern w_scanerr();\n");
	putf("#endif /* K&R C */\n");
	putf("#ifdef __cplusplus\n");
	putf("}\n");
	putf("#endif\n\n");

	putf("#ifdef __cplusplus\n");
	putf("extern \"C\" {\n");
	putf("#endif\n");
	putf("#if defined(__cplusplus) || defined(__STDC__)\n");
	putf("extern void w_err_expected(const char *token);\n");
	putf("extern void w_err_illegal(const char *rule);\n");
	putf("extern void w_err_skip(void);\n");
	putf("extern void w_flusherrs(void);\n");
	putf("extern int w_gettoken(void);\n");
	putf("extern int w_nexttoken(void);\n");
	putf("extern void w_skiptoken(void);\n");
	putf("extern const char *w_tokenname(int);\n");
	putf("#else /* K&R C */\n");
	putf("extern w_err_expected();\n");
	putf("extern w_err_illegal();\n");
	putf("extern w_err_skip();\n");
	putf("extern w_flusherrs();\n");
	putf("extern int w_gettoken();\n");
	putf("extern int w_nexttoken();\n");
	putf("extern w_skiptoken();\n");
	putf("extern char *w_tokenname();\n");
	putf("#endif /* K&R C */\n");
	putf("#ifdef __cplusplus\n");
	putf("}\n");
	putf("#endif\n\n");

	putf("extern int w_numerrors;\n");
	putf("extern int w_currtoken;\n\n");

	putf("#ifdef __cplusplus\n");
	putf("extern \"C\" {\n");
	putf("#endif\n");

	if (exportedname)
	{
		for (i = 0; i < numnonterms(); i++)
		{
			sym = getnonterm(i);

			if (sym->type != NONTERMINAL || !sym->exportsym)
				continue;

			mkstruct(sym);
			putf("#if defined(__cplusplus) || defined(__STDC__)\n");
			putf("extern int %s(%s);\n", mkname(sym),
					mkvoidtype(sym, "", " *"));
			putf("#else\n");
			putf("extern int %s();\n", mkname(sym));
			putf("#endif\n");
		}
	}
	else
	{
		mkstruct(startsymbol);
		putf("#if defined(__cplusplus) || defined(__STDC__)\n");
		putf("extern int %s(%s);\n", mkname(startsymbol),
				mkvoidtype(startsymbol, "", " *"));
		putf("#else\n");
		putf("extern int %s();\n", mkname(startsymbol));
		putf("#endif\n");
	}

	putf("#ifdef __cplusplus\n");
	putf("}\n");
	putf("#endif\n");

	putf("\n\n#endif /* __TOKENS_H_ */\n");
}

void
GenC::genscanner(const char *header)
{
	Symbol *sym;
	int i, c;
	boolean nl = TRUE;

	// scan past the definition section
	while ((c = w_input()) != EOI)
	{
		if (nl && c == '%')
		{
			int t = w_input();

			if (t == '%')
				break;

			put(c);
			c = t;
		}

		put(c);
		nl = c == '\n';
	}

	// dump out our header stuff here
	putf("%%{\n");
	putf("#include \"%s\"\n", header);
	putf("%%}\n");

	putf("%%%%\n");

	// print the lexical values from the grammer
	for (i = START; i < numsymbols(); i++)
	{
		Symbol *sym = getsymbol(i);

		if (sym->type != TERMINAL || sym->lexdef || sym->lexstr == NULL)
			continue;

		if (sym->name[0] == '"')
		{
			for (const char *s = sym->lexstr + 1; *s != '\0' &&
					!(*s == '"' && *(s + 1) == '\0'); s++)
				if (isalpha(*s))
				{
					if (casesensitive)
						put(*s);
					else
						putf("[%c%c]",
								islower(*s) ? toupper(*s) : *s,
								isupper(*s) ? tolower(*s) : *s);
				}
				else if (isdigit(*s))
					put(*s);
				else
				{
					if (*s == '\\')
						put(*s++);
					else
						put('\\');

					put(*s);
				}
		}
		else if (sym->name[0] == '\'')
		{
			const char *s = sym->lexstr + 1;

			if (*s != '\\' && !isalnum(*s))
				put('\\');

			put(*s);

			if (*s == '\\')
				put(s[1]);
		}
		else
			putf("%s", sym->lexstr);

		putf("\t{ return (int)%s; }\n", mkname(sym));
		sym->lexdef = TRUE;
	}

	// now add the rules at the bottom of the source
	if (c != EOI)
		while ((c = w_input()) != EOI)
		{
			if (nl && c == '%')
			{
				int t = w_input();

				if (t == '%')
					break;

				put(c);
				c = t;
			}
			else if (nl && c == '$')
			{
				const char *name = getword();

				if (name == NULL)
					quit("Unexpected end-of-w_input");

				sym = findsymbol(name);

				if (sym == NULL)
					error("Symbol %s not in grammer", name);

				if (sym->lexstr != NULL)
					error("Symbol %s lexical string redefined in lex section",
							name);

				while ((c = w_input()) == ' ' || c == '\t')
					;

				if (c == '\n' || c == EOI)
					putf("\n");
				else
				{
					for (; c != EOI && c != '\n'; c = w_input())
						put(c);

					putf("\t{ return (int)%s; }", name);
				}

				sym->lexdef = TRUE;
			}

			put(c);
			nl = c == '\n';
		}

	// check for missing tokens
	for (i = START; i < numsymbols(); i++)
	{
		sym = getsymbol(i);

		if (sym->type != TERMINAL || sym->lexdef)
			continue;

		if (sym->lexstr == NULL)
			error("Lexical value for Symbol %s is not defined", mkname(sym));
	}

	putf("%%%%\n");

	// now copy the rest (user code)
	if (c != EOI)
		while ((c = w_input()) != EOI)
			put(c);
}

void
GenC::dumpexport(Symbol *sym)
{
	putf("int\n");
	putf("#if defined(__cplusplus) || defined(__STDC__)\n");

	const char *exptype = mktype(sym, "", " *ret");
	putf("%s(%s)\n", mkname(sym), *exptype != '\0' ? exptype : "void");
	putf("#else\n");

	if (*exptype != '\0')
		putf("%s(ret)\n%s;\n", mkname(sym), exptype);
	else
		putf("%s()\n", mkname(sym));

	putf("#endif\n");
	putf("{\n");
	putf(1, "int w_rval;\n");
	putf(1, "int w_savnum = w_numerrors;\n");

	if (doresync && sym->doresync)
	{
		putf(1, "struct w_resynclink w_link;\n");
		putindent(1);
		printset("w_follow", *sym->follow);
		putf("\n");
		putf(1, "w_link.prev = NULL;\n");
		putf(1, "w_link.resync = NULL;\n");
	}

	putf(1, "w_numerrors = 0;\n");

	if (doresync && sym->doresync)
		putf(1, "w_rval = w_f%s(&w_link, w_follow%s);\n",
				mkname(sym), dotype(sym) ? ", ret" : "");
	else
		putf(1, "w_rval = w_f%s(%s);\n",
				mkname(sym), dotype(sym) ? "ret" : "");

	putf("\n");
	putf(1, "switch (w_nexttoken())\n");
	putf(1, "{\n");
	printcase(1, *sym->follow);
	putf(2, "break;\n\n");
	putf(1, "default:\n");
	putf(2, "w_err_illegal(\"%s\");\n", mkname(sym));
	putf(2, "w_rval = W_RETERR;\n");
	putf(2, "break;\n");
	putf(1, "}\n\n");
	putf(1, "if (w_numerrors > 0)\n");
	putf(2, "w_rval = W_RETERR;\n\n");
	putf(1, "w_numerrors += w_savnum;\n");
	putf(1, "w_flusherrs();\n");
	putf(1, "return w_rval;\n");
	putf("}\n\n");
}

void
GenC::genparser(const char *header)
{
	Symbol *sym;
	Symnode *n;
	int i;

	putf("#include \"%s\"\n", header);

	if (dumponlyparser)
	{
		mkcode(0, startcode);
		putf("\n");
	}
	else
	{
		putf("#define YYTEXT_DECL char *yytext\n\n");
		mkcode(0, startcode);
		putf("\n");
		putf("extern YYTEXT_DECL;\n");
		putf("int w_numerrors = 0;\n");
		putf("int w_currtoken = -1;\n");
	}

	putf("\n");

	if (doresync)
	{
		putf("struct w_resynclink\n{\n");
		putf(1, "struct w_resynclink *prev;\n");
		putf(1, "int *resync;\n};\n\n");
	}

	// first generate all auto-defined structs
	for (i = 0; i < numnonterms(); i++)
	{
		sym = getnonterm(i);

		if (sym->type != NONTERMINAL)
			continue;

		if (!sym->exportsym)
			mkstruct(sym);
	}

	// next generate prototyped function declarations
	putf("#if defined(__cplusplus) || defined(__STDC__)\n");

	putf("#ifdef __cplusplus\n");
	putf("extern \"C\" {\n");
	putf("#endif\n");

	for (i = 0; i < numnonterms(); i++)
	{
		sym = getnonterm(i);

		if (sym->type != NONTERMINAL)
			continue;

		if (optimize && (sym->usecount == 1 || sym->doinline) &&
				!sym->exportsym)
			continue;

		if (doresync && sym->doresync)
			putf("static int w_f%s(struct w_resynclink *, int *%s);\n",
					mkname(sym), mktype(sym, ", ", " *"));
		else
			putf("static int w_f%s(%s);\n", mkname(sym),
					mkvoidtype(sym, "", " *"));
	}

	putf("#ifdef __cplusplus\n");
	putf("}\n");
	putf("#endif\n");

	putf("#else /* K&R */\n");

	// finally generate all old-style C function declarations
	for (i = 0; i < numnonterms(); i++)
	{
		sym = getnonterm(i);

		if (sym->type != NONTERMINAL)
			continue;

		if (optimize && (sym->usecount == 1 || sym->doinline) &&
				!sym->exportsym)
			continue;

		putf("static int w_f%s();\n", mkname(sym));
	}

	putf("#endif /* K&R */\n");
	putf("\n");

	if (!dumponlyparser)
	{
		putf("static char *w_toknams[] =\n{\n");
		putf(1, "\"[] <empty>\",\n");

		for (i = START; i < numsymbols(); i++)
		{
			sym = getsymbol(i);

			if (sym->type != TERMINAL || *sym->name == '\'')
				continue;

			const char *str = sym->lexstr;

			if (str == NULL)
				putf(1, "\"%s\",\n", mkname(sym));
			else if (isalpha(*str))
				putf(1, "\"%s\",\n", str);
			else if (*str == '"' && str[strlen(str) - 1] == '"')
				putf(1, "%s,\n", sym->lexstr);
			else
				putf(1, "\"%s\",\n", mkname(sym));
		}

		putf("};\n\n");

		putf("#if defined(__cplusplus) || defined(__STDC__)\n");
		putf("const char *\n");
		putf("w_tokenname(int tok)\n");
		putf("#else\n");
		putf("char *\n");
		putf("w_tokenname(tok)\nint tok;\n");
		putf("#endif\n");
		putf("{\n");
		putf(1, "static char buf[2];\n\n");
		putf(1, "if (tok >= W_EMPTY)\n");
		putf(2, "return w_toknams[tok - W_EMPTY];\n\n");
		putf(1, "if (tok == W_EOI)\n");
		putf(2, "return \"EOI <end-of-input>\";\n\n");
		putf(1, "buf[0] = tok;\n");
		putf(1, "buf[1] = '\\0';\n");
		putf(1, "return buf;\n");
		putf("}\n\n");

		putf("int\n");
		putf("#if defined(__cplusplus) || defined(__STDC__)\n");
		putf("w_nexttoken(void)\n");
		putf("#else\n");
		putf("w_nexttoken()\n");
		putf("#endif\n");
		putf("{\n");
		putf(1, "return (w_currtoken < 0) ? (w_currtoken = w_gettoken()) : w_currtoken;\n");
		putf("}\n\n");
		putf("#define w_nexttoken() ((w_currtoken < 0) ? (w_currtoken = w_gettoken()) : w_currtoken)\n\n");

		putf("#if defined(__cplusplus) || defined(__STDC__)\n");
		putf("void\n");
		putf("w_skiptoken(void)\n");
		putf("#else\n");
		putf("w_skiptoken()\n");
		putf("#endif\n");
		putf("{\n");
		putf(1, "w_currtoken = -1;\n");
		putf("}\n\n");
		putf("#define w_skiptoken() (w_currtoken = -1)\n\n");

		putf("static int\n");
		putf("#if defined(__cplusplus) || defined(__STDC__)\n");

		if (doresync)
		{
			putf("w_scantoken(int expect, struct w_resynclink *lnk,");
			putf("int *resync)\n");
			putf("#else\n");
			putf("w_scantoken(expect, lnk, resync)\n");
			putf("int expect;\n");
			putf("struct w_resynclink *lnk;\n");
			putf("int *resync;\n");
			putf("#endif\n");
			putf("{\n");
			putf(1, "struct w_resynclink rlink;\n");
			putf(1, "struct w_resynclink *link;\n");
			putf(1, "int level = 1;\n\n");
			putf(1, "rlink.prev = lnk;\n");
			putf(1, "rlink.resync = resync;\n\n");
		}
		else
		{
			putf("w_scantoken(int expect)\n");
			putf("#else\n");
			putf("w_scantoken(expect)\n");
			putf("int expect;\n");
			putf("#endif\n");
			putf("{\n");
		}

		putf(1, "if (w_currtoken < 0)\n");
		putf(2, "w_currtoken = w_gettoken();\n\n");
		putf(1, "if (expect >= 0 && w_currtoken != expect)\n");
		putf(2, "w_err_expected(w_tokenname(expect));\n\n");

		if (doresync)
		{
			putf(1, "while (w_currtoken != expect)\n");
			putf(1, "{\n");
			putf(2, "int i;\n");
			putf(2, "int l = level;\n\n");
			putf(2, "for (link = &rlink; link != NULL && l-- > 0;");
			putf(" link = link->prev)\n");
			putf(3, "for (i = 0; link->resync && link->resync[i] >= 0; i++)\n");
			putf(4, "if (w_currtoken == link->resync[i])\n");
			putf(5, "return -1;\n\n");
		}
		else
		{
			putf(1, "while (w_currtoken != expect && w_currtoken != W_EOI)\n");
			putf(1, "{\n");
		}

		putf(2, "w_err_skip();\n");
		putf(2, "w_currtoken = w_gettoken();\n\n");
		putf(2, "if (w_currtoken == expect)\n");
		putf(3, "break;\n");

		if (doresync)
		{
			putf("\n");
			putf(2, "level++;\n");
		}

		putf(1, "}\n");
		putf(1, "\n");
		putf(1, "w_currtoken = -1;\n");
		putf(1, "return expect;\n");
		putf("}\n\n");

		if (exportedname)
			for (i = 0; i < numnonterms(); i++)
			{
				sym = getnonterm(i);

				if (sym->type != NONTERMINAL || !sym->exportsym)
					continue;

				dumpexport(sym);
			}
		else
			dumpexport(startsymbol);
	}

	putf("\n");
	i = 1;

	if (doresync)
	{
		for (Setiter setiter(settab); setiter; ++setiter)
		{
			Set & set = setiter();
			printset(strbldf("w_resync%d", set.id = i++), *set.set);
		}

		putf("\n");
	}

	Bitset set(numsymbols());

	for (i = 0; i < numnonterms(); i++)
	{
		sym = getnonterm(i);

		if (sym->type == CODE)
		{
			mkcode(0, sym);
			continue;
		}

		if (optimize && (sym->usecount < 2 || sym->doinline) && !sym->exportsym)
		{
			if (sym->usecount == 0)
				error("Warning - non-terminal %s is never used", sym->name);

			continue;
		}

		putf("static int\n");
		putf("#if defined(__cplusplus) || defined(__STDC__)\n");

		if (doresync && sym->doresync)
		{
			putf("w_f%s(struct w_resynclink *w_lnk, int *w_resync%s)\n",
					mkname(sym), mktype(sym, ", ", " *w_rr"));
			putf("#else\n");
			putf("w_f%s(w_lnk, w_resync%s)\n", mkname(sym),
					dotype(sym) ? ", w_rr" : "");
			putf("struct w_resynclink *w_lnk;\n");
			putf("int *w_resync;\n");

			if (dotype(sym))
				putf("%s *w_rr;\n", mktype(sym));
		}
		else
		{
			putf("w_f%s(%s)\n", mkname(sym), mkvoidtype(sym, "", " *w_rr"));
			putf("#else\n");
			putf("w_f%s", mkname(sym));

			if (dotype(sym))
			{
				putf("(w_rr)\n");
				putf("%s *w_rr;\n", mktype(sym));
			}
			else
				putf("()\n");
		}

		putf("#endif\n");
		putf("{\n");

		if (doresync)
			putf(1, "struct w_resynclink w_link;\n");

		boolean dosave = (sym->usedret & RET_CODE) ? TRUE : FALSE;

		if (dosave)
			putf(1, "int w_rc = W_RETOK;\n");

		mkcode(1, sym->symcode);
		putf("\n");

		if (doresync)
		{
			if (sym->doresync)
			{
				putf(1, "w_link.prev = w_lnk;\n");
				putf(1, "w_link.resync = w_resync;\n\n");

				if (sym->resync != NULL)
					putf(1, "%sw_scantoken(-1, &w_link, w_resync%d);\n",
							mkretcode(dosave), sym->resync->id);
			}
			else
			{
				putf(1, "w_link.prev = NULL;\n");
				putf(1, "w_link.resync = w_resync%d;\n\n", sym->resync->id);
			}
		}

		if (sym->node != NULL && sym->node->orelse == NULL)
		{
			genstmt(1, sym->node, dosave);
			mkcode(1, sym->endcode);

			if (dosave)
				putf(1, "return w_rc;\n");
			else
				putf(1, "return W_RETOK;\n");

			putf("}\n\n");
			continue;
		}

		boolean donedefault = FALSE;
		putf(1, "switch (w_nexttoken())\n");
		putf(1, "{");

		for (n = sym->node; n != NULL; n = n->orelse)
		{
			set.clear();

			for (Symnode * s = n; s != NULL; s = s->next)
			{
				if (s->sym->type >= CODE)
					continue;

				set |= *s->sym->first;

				if (!s->sym->first->has(EMPTY))
					break;
			}

			if (set.count() == 0 || (set.count() == 1 && set.has(EMPTY)))
			{
				donedefault = TRUE;
				putf("\n");
				putf(1, "default:\n");
				genstmt(2, n, dosave);

				if (!sym->first->has(EMPTY))
				{
					putf(2, "w_err_illegal(\"%s\");\n", mkerrname(sym));

					if (dosave)
					{
						putf(2, "w_rc = W_RETERR;\n");
						putf(2, "break;\n");
					}
					else
					{
						mkcode(2, sym->endcode);
						putf(2, "return W_RETERR;\n");
					}
				}
				else
					putf(2, "break;\n");
			}
			else
			{
				putf("\n");
				printcase(1, set);
				genstmt(2, n, dosave);
				putf(2, "break;\n");
			}
		}

		if (!donedefault && !sym->first->has(EMPTY))
		{
			putf("\n");
			putf(1, "default:\n");
			putf(2, "w_err_illegal(\"%s\");\n", mkerrname(sym));

			if (dosave)
			{
				putf(2, "w_rc = W_RETERR;\n");
				putf(2, "break;\n");
			}
			else
			{
				mkcode(2, sym->endcode);
				putf(2, "return W_RETERR;\n");
			}
		}

		putf(1, "}\n\n");
		mkcode(1, sym->endcode);

		if (dosave)
			putf(1, "return w_rc;\n");
		else
			putf(1, "return W_RETOK;\n");

		putf("}\n\n");
	}
}

void
GenC::gencode(const char *infile, const char *headername,
			const char *parsername, const char *scannername)
{
	inputfile = infile;

	if (headername == NULL)
		headername = "tokens.h";

	if (scannername == NULL)
		scannername = "scanner.l";

	if (parsername == NULL)
		parsername = "parser.cc";

	openfile(headername);
	genheader();
	cmpandmv(headername);

	openfile(parsername);
	genparser(headername);
	cmpandmv(parsername);

	if (gotlexstr)
	{
		openfile(scannername);
		genscanner(headername);
		cmpandmv(scannername);
	}

	closefile();
}
