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

// Copyright (c) 2000-2002 by Parag Patel.  All Rights Reserved.

// Code generator for Java.

#include "defs.h"
#include "toks.h"
#include "gen.h"
#include "genj.h"
#include <stdarg.h>

GenJava::GenJava()
{
	currfile = NULL;
	inputfile = NULL;
	voidstr = strget("void");
}

GenJava::~GenJava()
{
}

const char *
GenJava::mkiname(Symbol *sym)
{
	if (*sym->name == '\'')
		return sym->name;

	if (*sym->name != '"')
		return sym->name;

	return strbldf("W_S%d/*%s*/", sym->id, sym->name);
}

const char *
GenJava::mkname(Symbol *sym)
{
	if (*sym->name == '\'')
		return sym->name;

	if (*sym->name != '"')
		return strbldf("%s.%s", tokenclass, sym->name);

	return strbldf("%s.W_S%d/*%s*/", tokenclass, sym->id, sym->name);
}

const char *
GenJava::mkerrname(Symbol *sym)
{
	if (sym->type == NONTERMINAL && sym->realname != NULL)
		return sym->realname;

	return mkiname(sym);
}


void
GenJava::printset(const char *name, Bitset &set)
{
	Symbol *s;
	int id = 0;
	const char *pre = "";
	putf("final int[] %s = { ", name);

	for (; id < set.size(); id++)
	{
		if (!set.has(id))
			continue;

		if (id == EMPTY)
			putf("%s%s.W_EMPTY", pre, tokenclass);
		else if (id == END)
			putf("%s%s.W_EOI", pre, tokenclass);
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
GenJava::printcase(int indent, Bitset &set)
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
			putf(indent, "case %s.W_EOI:\n", tokenclass);
		else
		{
			s = getsymbol(id);
			putf(indent, "case %s:\n", mkname(s));
		}
	}
}

const char *
GenJava::mktype(Symbol *sym, const char *pre, const char *post)
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
				return strbldf("%sW_T%s%s", pre, sym->realname, post);

			return strbldf("%sW_T%s%s", pre, sym->name, post);
		}
	}

	return strbldf("%s%s%s", pre, name, post);
}

const char *
GenJava::mkvoidtype(Symbol *sym, const char *pre, const char *post)
{
	const char *type = mktype(sym, pre, post);

	if (*type == '\0')
		type = "void";

	return type;
}

void
GenJava::mkstruct(Symbol *sym)
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

	putf("class W_T%s\n{\n", name);
	putf(1, "%s;\n};\n\n", sym->rettype);
}

void
GenJava::mkcode(int indent, Symbol *code)
{
	if (!dumpcode || code == NULL || code->code == NULL)
		return;

	if (genlineinfo)
		putf("// @line %d %s\n", code->line, inputfile);

	// output code directly to avoid overflowing putf buffers
	putindent(indent);
	fputs(code->code, fp);

	for (const char *s = code->code; *s != '\0'; s++)
		if (*s == '\n')
			currline++;

	putf("\n");

	if (genlineinfo)
		putf("// @line %d %s\n", currline + 1, currfile);
}

const char *
GenJava::mkvarname(Symnode *start, Symnode *node)
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
GenJava::printtestexpr(int indent, Bitset &set, const char *post)
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
GenJava::printwhile(int indent, Bitset &set, const char *post)
{
	putf("while (");
	printtestexpr(indent, set, post);
}

void
GenJava::printif(int indent, Bitset &set, const char *post)
{
	putf("if (");
	printtestexpr(indent, set, post);
}

void
GenJava::loopbegin(int &indent, int doloop, Symbol *sym)
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
		if (doloop == ZEROPLUS || doloop == ONEPLUS)
		{
			putf(indent, "for (;;)\n");
			putf(indent++, "{\n");
		}

		if (doloop != ONEPLUS)
		{
			putf(indent, "switch (w_nexttoken())\n");
			putf(indent, "{\n");

			for (int id = 0; id < sym->first->size(); id++)
			{
				if (!sym->first->has(id) || id == EMPTY)
					continue;

				if (id == END)
					putf(indent, "case %s.W_EOI:\n", tokenclass);
				else
					putf(indent, "case %s:\n", mkname(getsymbol(id)));
			}

			indent++;
		}
	}
}

void
GenJava::loopend(int &indent, int doloop, Symbol *sym)
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
		if (doloop == ONEPLUS)
		{
			putf(indent, "switch (w_nexttoken())\n");
			putf(indent, "{\n");

			for (int id = 0; id < sym->first->size(); id++)
			{
				if (!sym->first->has(id) || id == EMPTY)
					continue;

				if (id == END)
					putf(indent, "case %s.W_EOI:\n", tokenclass);
				else
					putf(indent, "case %s:\n", mkname(getsymbol(id)));
			}
		}
		else if (doloop == ONEZERO)
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
GenJava::genstmt(int indent, Symnode *node, boolean /*saveret*/)
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
				putf(indent, "w_scantoken(%s, w_link, w_resync%d);\n",
						mkname(s), n->resync->id);
			else
				putf(indent, "w_scantoken(%s);\n", mkname(s));

			loopend(indent, doloop, s);
		}
		else if (optimize && (s->usecount == 1 || s->doinline) && !s->exportsym)
		{
			Symbol *sym = s;
			putf("\n");
			putf(indent++, "{/*%s*/\n", sym->name);

			if (dotype(sym))
				putf(indent, "%s = w_rv%s;\n", mktype(sym, "", " w_rr"),
						mkiname(sym));

			if (doresync && n->resync)
			{
				if (sym->doresync)
					putf(indent, "w_resynclink w_lnk%s = w_link;\n",
							mkiname(sym));

				putf(indent, "w_link = new w_resynclink();\n");
			}

			//if (sym->usedret & RET_CODE)
				//quit("cannot use $? syntax with Java for %s", mkerrname(sym));

			mkcode(indent, sym->symcode);
			putf("\n");

			if (doresync && n->resync)
			{
				if (sym->doresync)
					putf(indent, "w_link.prev = w_lnk%s;\n", mkiname(sym));

				putf(indent, "w_link.resync = w_resync%d;\n\n", n->resync->id);

				if (sym->resync != NULL)
					putf(indent, "w_scantoken(-1, w_link, w_resync%d);\n\n",
							sym->resync->id);
			}

			if (sym->node != NULL && sym->node->orelse == NULL)
			{
				loopbegin(indent, doloop, sym);
				genstmt(indent, sym->node);
				loopend(indent, doloop, sym);
				mkcode(indent, sym->endcode);

				if (doresync && sym->doresync)
				{
					putf("\n");
					putf(indent, "w_link = w_lnk%s;\n", mkiname(sym));
				}

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
					genstmt(indent + 1, nn);

					if (!sym->first->has(EMPTY))
					{
						putf(indent + 1, "w_err_illegal(\"%s\");\n",
								mkerrname(sym));
					}
				}
				else
				{
					putf("\n");
					printcase(indent, set);
					genstmt(indent + 1, nn);
				}

				putf(indent + 1, "break;\n");
			}

			if (!donedefault && !sym->first->has(EMPTY))
			{
				putf("\n");
				putf(indent, "default:\n");
				putf(indent + 1, "w_err_illegal(\"%s\");\n", mkerrname(sym));
				putf(indent + 1, "break;\n");
			}

			putf(--indent, "}\n");
			loopend(indent, doloop, sym);
			mkcode(indent, sym->endcode);

			if (doresync && sym->doresync)
			{
				putf("\n");
				putf(indent, "w_link = w_lnk%s;\n", mkiname(sym));
			}

			putf(--indent, "}/*%s*/\n\n", sym->name);
		}
		else
		{
			loopbegin(indent, doloop, s);

			if (dotype(s))
			{
				if (doresync && s->doresync && n->resync)
					putf(indent,
							"w_rv%s = w_f%s(w_link, w_resync%d, w_rv%s);\n",
							mkvarname(node, n), mkiname(s),
							n->resync->id, mkvarname(node, n));
				else
					putf(indent,
							"w_rv%s = w_f%s(w_rv%s);\n",
							mkvarname(node, n),
							mkiname(s), mkvarname(node, n));
			}
			else
			{
				if (doresync && s->doresync && n->resync)
					putf(indent, "w_f%s(w_link, w_resync%d);\n",
							mkiname(s), n->resync->id);
				else
					putf(indent, "w_f%s();\n", mkiname(s));
			}

			loopend(indent, doloop, s);
		}
	}

	putf(--indent, "}\n\n");
}

void
GenJava::genheader(void)
{
	Symbol *sym;
	int i;
	int tokid = 65536;

	putf("public class %s\n", tokenclass);
	putf("{\n");

	putf(1, "static final int W_EOI = 0;\n");
	putf(1, "static final int W_EMPTY = %d;\n\n", tokid++);

	for (i = START; i < numsymbols(); i++)
	{
		sym = getsymbol(i);

		if (sym->type != TERMINAL || *sym->name == '\'')
			continue;

		putf(1, "static final int %s = %d;\n", mkiname(sym), tokid++);
	}

	putf("\n");
	putf(1, "static final int W_LAST_TOKEN = %d;\n", tokid);
	putf("}\n");
}

void
GenJava::genscanner(const char * /*header*/)
{
	Symbol *sym;
	int i, c;
	boolean nl = TRUE;

	// dump out our header stuff here
	putf("//import %s;\n", tokenclass);
	putf("\n%%%%\n");

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

		putf("\t{ return new %s(%s); }\n", tokenclass, mkname(sym));
		sym->lexdef = TRUE;
	}

	// now add the rules at the bottom of the source
	if (c != EOI)
	{
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

					putf("\t{ return new %s(%s.%s); }", tokenclass, tokenclass, name);
				}

				sym->lexdef = TRUE;
			}

			put(c);
			nl = c == '\n';
		}
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

	// now copy the rest (user code)
	if (c != EOI)
		while ((c = w_input()) != EOI)
			put(c);
}

void
GenJava::dumpexport(Symbol *sym)
{
	putf("public static %s\n", mkvoidtype(sym, "", ""));

	const char *exptype = mktype(sym, "", "");
	putf("%s(%s%s)\n", mkiname(sym), exptype, *exptype != '\0' ? " ret" : "");

	putf("{\n");

	if (dotype(sym))
		putf(1, "%s w_rval;\n", mktype(sym, "", ""));

	putf(1, "int w_savnum = w_numerrors;\n");

	if (doresync && sym->doresync)
	{
		putf(1, "w_resynclink w_link = new w_resynclink();\n");
		putindent(1);
		printset("w_follow", *sym->follow);
		putf("\n");
		putf(1, "w_link.prev = null;\n");
		putf(1, "w_link.resync = null;\n");
	}

	putf(1, "w_numerrors = 0;\n");

	if (doresync && sym->doresync)
	{
		if (dotype(sym))
			putf(1, "w_rval = w_f%s(w_link, w_follow%s, ret);\n", mkiname(sym));
		else
			putf(1, "w_f%s(w_link, w_follow);\n", mkiname(sym));
	}
	else
	{
		if (dotype(sym))
			putf(1, "w_rval = w_f%s(ret);\n", mkiname(sym));
		else
			putf(1, "w_f%s();\n", mkiname(sym));
	}

	putf("\n");
	putf(1, "switch (w_nexttoken())\n");
	putf(1, "{\n");
	printcase(1, *sym->follow);
	putf(2, "break;\n\n");
	putf(1, "default:\n");
	putf(2, "w_err_illegal(\"%s\");\n", mkerrname(sym));
	putf(2, "break;\n");
	putf(1, "}\n\n");
	putf(1, "w_numerrors += w_savnum;\n");
	putf(1, "w_flusherrs();\n");

	if (dotype(sym))
		putf(1, "return w_rval;\n");

	putf("}\n\n");
}

void
GenJava::genparser(const char * /*header*/)
{
	Symbol *sym;
	Symnode *n;
	int i;

	putf("import %s;\n\n", tokenclass);
	mkcode(1, startcode);
	putf("\n");

	putf("public class %s\n", parserclass);
	putf("{\n");

	if (!dumponlyparser)
	{
		putf("public static int w_numerrors = 0;\n");
		putf("public static int w_currtoken = -1;\n");
	}

	putf("\n");

	if (doresync)
	{
		putf("static class w_resynclink\n{\n");
		putf(1, "w_resynclink prev;\n");
		putf(1, "int[] resync;\n};\n\n");
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

	putf("\n");

	if (!dumponlyparser)
	{
		putf("static final String[] w_tokenclass =\n{\n");
		putf(1, "\"[] <empty>\",\n");

		for (i = START; i < numsymbols(); i++)
		{
			sym = getsymbol(i);

			if (sym->type != TERMINAL || *sym->name == '\'')
				continue;

			const char *str = sym->lexstr;

			if (str == NULL)
				putf(1, "\"%s\",\n", mkiname(sym));
			else if (isalpha(*str))
				putf(1, "\"%s\",\n", str);
			else if (*str == '"' && str[strlen(str) - 1] == '"')
				putf(1, "%s,\n", sym->lexstr);
			else
				putf(1, "\"%s\",\n", mkiname(sym));
		}

		putf("};\n\n");

		putf("public static String\n");
		putf("w_tokenname(int tok)\n");
		putf("{\n");
		putf(1, "if (tok >= %s.W_EMPTY)\n", tokenclass);
		putf(2, "return w_tokenclass[tok - %s.W_EMPTY];\n\n", tokenclass);
		putf(1, "if (tok == %s.W_EOI)\n", tokenclass);
		putf(2, "return \"EOI <end-of-input>\";\n\n");
		putf(1, "return String.valueOf((char)tok);\n");
		putf("}\n\n");

		putf("public static int\n");
		putf("w_nexttoken()\n");
		putf("{\n");
		putf(1, "if (w_currtoken < 0)\n");
		putf(2, "w_currtoken = w_gettoken();\n\n");
		putf(1, "return w_currtoken;\n");
		putf("}\n\n");

		putf("public static void\n");
		putf("w_skiptoken()\n");
		putf("{\n");
		putf(1, "w_currtoken = -1;\n");
		putf("}\n\n");

		putf("static int\n");

		if (doresync)
		{
			putf("w_scantoken(int expect, w_resynclink lnk, int[] resync)\n");
			putf("{\n");
			putf(1, "w_resynclink rlink = new w_resynclink();\n");
			putf(1, "w_resynclink link;\n");
			putf(1, "int level = 1;\n\n");
			putf(1, "rlink.prev = lnk;\n");
			putf(1, "rlink.resync = resync;\n\n");
		}
		else
		{
			putf("w_scantoken(int expect)\n");
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
			putf(2, "for (link = rlink; link != null && l-- > 0;");
			putf(" link = link.prev)\n");
			putf(3, "for (i = 0; link.resync != null && link.resync[i] >= 0; i++)\n");
			putf(4, "if (w_currtoken == link.resync[i])\n");
			putf(5, "return -1;\n\n");
		}
		else
		{
			putf(1, "while (w_currtoken != expect && w_currtoken != %s.W_EOI)\n",
					tokenclass);
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
			putf("static ");
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

		putf("static %s\n", mkvoidtype(sym, "", ""));

		if (doresync && sym->doresync)
			putf("w_f%s(w_resynclink w_lnk, int[] w_resync%s)\n",
					mkiname(sym), mktype(sym, ", ", " w_rr"));
		else
			putf("w_f%s(%s)\n", mkiname(sym), mktype(sym, "", " w_rr"));

		putf("{\n");

		if (doresync)
			putf(1, "w_resynclink w_link = new w_resynclink();\n");

		//if (sym->usedret & RET_CODE)
			//quit("cannot use $? syntax with Java for %s", mkerrname(sym));

		mkcode(1, sym->symcode);
		putf("\n");

		if (doresync)
		{
			if (sym->doresync)
			{
				putf(1, "w_link.prev = w_lnk;\n");
				putf(1, "w_link.resync = w_resync;\n\n");

				if (sym->resync != NULL)
					putf(1, "w_scantoken(-1, w_link, w_resync%d);\n",
							sym->resync->id);
			}
			else
			{
				putf(1, "w_link.prev = null;\n");
				putf(1, "w_link.resync = w_resync%d;\n\n", sym->resync->id);
			}
		}

		if (sym->node != NULL && sym->node->orelse == NULL)
		{
			genstmt(1, sym->node);
			mkcode(1, sym->endcode);

			if (dotype(sym))
				putf(1, "return w_rr;\n");
			else
				putf(1, "return;\n");

			putf("}\n\n");
			continue;
		}

		boolean donedefault = FALSE;
		putf(1, "switch (w_nexttoken())\n");
		putf(1, "{");

		for (n = sym->node; n != NULL; n = n->orelse)
		{
			set.clear();

			for (Symnode *s = n; s != NULL; s = s->next)
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
				genstmt(2, n);

				if (!sym->first->has(EMPTY))
				{
					putf(2, "w_err_illegal(\"%s\");\n", mkerrname(sym));
					mkcode(2, sym->endcode);

					if (dotype(sym))
						putf(2, "return w_rr;\n");
					else
						putf(2, "return;\n");
				}
				else
					putf(2, "break;\n");
			}
			else
			{
				putf("\n");
				printcase(1, set);
				genstmt(2, n);
				putf(2, "break;\n");
			}
		}

		if (!donedefault && !sym->first->has(EMPTY))
		{
			putf("\n");
			putf(1, "default:\n");
			putf(2, "w_err_illegal(\"%s\");\n", mkerrname(sym));
			mkcode(2, sym->endcode);

			if (dotype(sym))
				putf(2, "return w_rr;\n");
			else
				putf(2, "return;\n");
		}

		putf(1, "}\n");
		mkcode(1, sym->endcode);

		if (dotype(sym))
		{
			putf("\n");
			putf(1, "return w_rr;\n");
		}

		putf("}\n\n");
	}

	putf("}\n");
}

void
GenJava::gencode(const char *infile, const char *headername,
			const char *parsername, const char *scannername)
{
	char *s;

	inputfile = infile;

	if (headername == NULL)
		headername = "Wtoken.java";

	if (scannername == NULL)
		scannername = "Wscan.l";

	if (parsername == NULL)
		parsername = "Wparse.java";

	/* parse these out of the filenames */
	tokenclass = strdup(headername);
	parserclass = strdup(parsername);
	s = strchr(tokenclass, '.');

	if (s)
		*s = '\0';

	s = strchr(parserclass, '.');

	if (s)
		*s = '\0';

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
