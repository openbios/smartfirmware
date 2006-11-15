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

// Copyright (c) 2001-2002 by Parag Patel.  All Rights Reserved.

// Code generator for Modula-3.

#include "defs.h"
#include "toks.h"
#include "gen.h"
#include "genm.h"
#include <stdarg.h>

GenModula3::GenModula3()
{
	currfile = NULL;
	inputfile = NULL;
}

GenModula3::~GenModula3()
{
}

const char *
GenModula3::mkiname(Symbol *sym)
{
	if (*sym->name == '\'')
		return strbldf("ORD(%s)", sym->name);

	if (*sym->name != '"')
		return sym->name;

	return strbldf("W_S%d(*%s*)", sym->id, sym->name);
}

const char *
GenModula3::mkname(Symbol *sym)
{
	if (*sym->name == '\'')
		return strbldf("ORD(%s)", sym->name);

	if (*sym->name != '"')
		return strbldf("%s", sym->name);

	return strbldf("W_S%d(*%s*)", sym->id, sym->name);
}

const char *
GenModula3::mkerrname(Symbol *sym)
{
	if (sym->type == NONTERMINAL && sym->realname != NULL)
		return sym->realname;

	return mkiname(sym);
}


void
GenModula3::printset(const char *name, Bitset &set)
{
	Symbol *s;
	int id;
	const char *pre = "";

	putf(1, "%s = W_resyncset { ", name);

	for (id = 0; id < set.size(); id++)
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

	putf(" };\n");
}

void
GenModula3::printcase(int indent, Bitset &set)
{
	Symbol *s;
	int id = 0;
	boolean first = TRUE;

	if (!set.size())
		return;

	putf(indent, "");

	for (; id < set.size(); id++)
	{
		if (!set.has(id))
			continue;

		if (id == EMPTY)
			continue;

		if (!first)
			putf(", ");

		if (id == END)
			putf("W_EOI");
		else
		{
			s = getsymbol(id);
			putf("%s", mkname(s));
		}

		first = FALSE;
	}

	putf(" =>\n");
}

const char *
GenModula3::mktype(Symbol *sym, const char *pre, const char *post)
{
	if (!dotype(sym))
		return "";

	const char *name;

	if (sym->rettype == NULL)
		name = "INTEGER";
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
GenModula3::mkvoidtype(Symbol *sym, const char *pre, const char *post)
{
	const char *type = mktype(sym, pre, post);

	return type;
}

void
GenModula3::mkstruct(Symbol *sym)
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

	putf(1, "W_T%s = RECORD\n", name);
	putf(2, "%s;\n", sym->rettype);
	putf(1, "END;\n");
}

void
GenModula3::mkcode(int indent, Symbol *code)
{
	if (!dumpcode || code == NULL || code->code == NULL)
		return;

	if (genlineinfo)
		putf("<*LINE %d \"%s\" *>\n", code->line, inputfile);

	// output code directly to avoid overflowing putf buffers
	putindent(indent);
	fputs(code->code, fp);

	for (const char *s = code->code; *s != '\0'; s++)
		if (*s == '\n')
			currline++;

	putf("\n");

	if (genlineinfo)
		putf("<*LINE %d \"%s\" *>\n\n", currline + 1, currfile);
}

const char *
GenModula3::mkvarname(Symnode *start, Symnode *node)
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
GenModula3::printtestexpr(int indent, Bitset &set, const char *post)
{
	boolean first = TRUE;
	int id = 0;

	for (; id < set.size(); id++)
	{
		if (!set.has(id) || id == EMPTY)
			continue;

		if (first)
		{
			putf("w_nexttoken() = ");
			first = FALSE;
		}
		else
		{
			putf(" OR\n");

			if (dumponlyparser)
				putf(indent + 2, "w_nexttoken() = ");
			else
				putf(indent + 2, "w_currtoken = ");
		}

		if (id == END)
			putf("W_EOI");
		else
			putf("%s", mkname(getsymbol(id)));
	}

	if (first)
		putf("1");
}

void
GenModula3::printwhile(int indent, Bitset &set, const char *post)
{
	putf("\n");
	putf(indent, "WHILE ");
	printtestexpr(indent, set, post);
	putf(" DO%s\n", post);
}

void
GenModula3::printif(int indent, Bitset &set, const char *post)
{
	putf("\n");
	putf(indent, "IF ");
	printtestexpr(indent, set, post);
	putf(" THEN%s\n", post);
}

void
GenModula3::loopbegin(int &indent, int doloop, Symbol *sym)
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
			putf(indent, "REPEAT\n");
			break;

		case ONEZERO:
			putindent(indent);
			printif(indent, *sym->first, "");
			break;

		default:
			error("Unexpected doloop value %d", doloop);
			break;
		}

		indent++;
	}
	else
	{
		if (doloop == ZEROPLUS || doloop == ONEPLUS)
		{
			putf(indent++, "LOOP\n");
		}

		if (doloop != ONEPLUS)
		{
			putf(indent, "CASE w_nexttoken() OF\n");

			for (int id = 0; id < sym->first->size(); id++)
			{
				if (!sym->first->has(id) || id == EMPTY)
					continue;

				if (id == END)
					putf(indent, "| W_EOI =>n", tokenclass);
				else
					putf(indent, "| %s =>\n", mkname(getsymbol(id)));
			}

			indent++;
		}
	}
}

void
GenModula3::loopend(int &indent, int doloop, Symbol *sym)
{
	if (doloop == 0)
		return;

	if (sym->type == TERMINAL || sym->first->count() < 4)
	{
		putindent(--indent);

		if (doloop == ONEPLUS)
		{
			putf("UNTIL ");
			printwhile(indent, *sym->first, ";");
			putf(";\n");
		}
		else
			putf("END;\n");
	}
	else
	{
		if (doloop == ONEPLUS)
		{
			putf(indent, "CASE w_nexttoken() OF\n");

			for (int id = 0; id < sym->first->size(); id++)
			{
				if (!sym->first->has(id) || id == EMPTY)
					continue;

				if (id == END)
					putf(indent, "| W_EOI =>\n", tokenclass);
				else
					putf(indent, "| %s =>\n", mkname(getsymbol(id)));
			}
		}
		else if (doloop == ONEZERO)
			putf(indent, "EXIT;\n\n");
		else
			putf(indent, "continue;\n\n");

		putf(indent - 1, "ELSE\n");
		putf(indent, "EXIT;\n");
		putf(--indent, "END;\n\n");

		if (doloop != ONEZERO)
		{
			putf(indent, "EXIT; (* LOOP *)\n");
			putf(--indent, "END;\n\n");
		}
	}
}

void
GenModula3::genstmt(int indent, Symnode *node, boolean /*saveret*/)
{
	Symbol *s;
	Symnode *n;
	boolean dovar = FALSE;

	for (n = node; n != NULL; n = n->next)
	{
		if (n->sym->type != NONTERMINAL || n->sym->id == EMPTY
				|| !dotype(n->sym))
			continue;

		if (!dovar)
		{
			putf(indent++, "VAR\n");
			dovar = TRUE;
		}

		putf(indent, "W_rv%s: %s;\n", mkvarname(node, n), mktype(n->sym));
	}

	if (dovar)
		putf(indent - 1, "BEGIN\n");

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
				putf(indent, "W_scantoken(%s, W_link, W_resync%d);\n",
						mkname(s), n->resync->id);
			else
				putf(indent, "W_scantoken(%s);\n", mkname(s));

			loopend(indent, doloop, s);
		}
#if TODO
		else if (optimize && (s->usecount == 1 || s->doinline) && !s->exportsym)
		{
			Symbol *sym = s;
			putf("\n");
			putf(indent++, "VAR (* %s *)\n", sym->name);

			if (dotype(sym))
				putf(indent, "W_rv%s: %s;\n", mkiname(sym),
						mktype(sym, "", " W_rr"));

			if (doresync && n->resync)
			{
				if (sym->doresync)
					putf(indent, "W_lnk%s : W_resynclink := W_link;\n",
							mkiname(sym));

				putf(indent, "W_link := NEW(W_resynclink);\n");
			}

			/*
			if (sym->usedret & RET_CODE)
				quit("cannot use $? syntax with Modula-3 for %s",
						mkerrname(sym));
			*/

			mkcode(indent, sym->symcode);
			putf("\n");

			if (doresync && n->resync)
			{
				if (sym->doresync)
					putf(indent, "W_link.prev = W_lnk%s;\n", mkiname(sym));

				putf(indent, "W_link.resync = W_resync%d;\n\n", n->resync->id);

				if (sym->resync != NULL)
					putf(indent, "W_scantoken(-1, W_link, W_resync%d);\n\n",
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
					putf(indent, "W_link = W_lnk%s;\n", mkiname(sym));
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
				putf(indent, "W_link = W_lnk%s;\n", mkiname(sym));
			}

			putf(--indent, "}/*%s*/\n\n", sym->name);
		}
#endif
		else
		{
			loopbegin(indent, doloop, s);

			if (dotype(s))
			{
				if (doresync && s->doresync && n->resync)
					putf(indent,
							"W_rv%s := W_f%s(W_link, W_resync%d, W_rv%s);\n",
							mkvarname(node, n), mkiname(s),
							n->resync->id, mkvarname(node, n));
				else
					putf(indent,
							"W_rv%s := W_f%s(W_rv%s);\n",
							mkvarname(node, n),
							mkiname(s), mkvarname(node, n));
			}
			else
			{
				if (doresync && s->doresync && n->resync)
					putf(indent, "W_f%s(W_link, W_resync%d);\n",
							mkiname(s), n->resync->id);
				else
					putf(indent, "W_f%s();\n", mkiname(s));
			}

			loopend(indent, doloop, s);
		}
	}

	if (dovar)
		putf(--indent, "END;\n\n");
}

void
GenModula3::genheader(void)
{
	Symbol *sym;
	int i;
	int tokid = 256;
	const char *exptype;

	putf("INTERFACE %s;\n", tokenclass);
	putf("\n");
	putf("TYPE T = INTEGER;\n");
	putf("\n");

	if (exportedname)
	{
		for (i = 0; i < numnonterms(); i++)
		{
			sym = getnonterm(i);

			if (sym->type != NONTERMINAL || !sym->exportsym)
				continue;

			mkstruct(sym);
		}
	}
	else
	{
		mkstruct(startsymbol);
	}

	putf("\n");
	putf("CONST\n");

	putf(1, "W_EOI : INTEGER = 0;\n");
	putf(1, "W_EMPTY : INTEGER = %d;\n\n", tokid++);

	for (i = START; i < numsymbols(); i++)
	{
		sym = getsymbol(i);

		if (sym->type != TERMINAL || *sym->name == '\'')
			continue;

		putf(1, "%s : INTEGER = %d;\n", mkiname(sym), tokid++);
	}

	putf(1, "W_LAST_TOKEN : INTEGER = %d;\n", tokid);
	putf("\n");

	putf("TYPE W_resyncset = SET OF [0..%d];\n", tokid);
	putf("\n");

	if (exportedname)
	{
		for (i = 0; i < numnonterms(); i++)
		{
			sym = getnonterm(i);

			if (sym->type != NONTERMINAL || !sym->exportsym)
				continue;

			exptype = mktype(sym, "", "");
			putf("PROCEDURE %s(%s%s)", mkiname(sym),
					*exptype != '\0' ? " ret : REF" : "", exptype);
			putf("%s;\n", mkvoidtype(sym, ": ", ""));
		}
	}
	else
	{
		exptype = mktype(startsymbol, "", "");
		putf("PROCEDURE %s(%s%s)", mkiname(startsymbol),
				*exptype != '\0' ? " ret : REF" : "", exptype);
		putf("%s;\n", mkvoidtype(startsymbol, ": ", ""));
	}

	putf("END %s.\n", tokenclass);
}

void
GenModula3::genscanner(const char * /*header*/)
{
#ifdef TODO
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
#endif
}

void
GenModula3::dumpexport(Symbol *sym)
{
	const char *exptype = mktype(sym, "", "");
	putf("PROCEDURE %s(%s%s)",
			mkiname(sym), *exptype != '\0' ? " ret : REF" : "", exptype);
	putf("%s =\n", mkvoidtype(sym, ": ", ""));

	if (doresync && sym->doresync)
	{
		putf("CONST\n");
		printset("W_follow", *sym->follow);
	}

	putf("VAR\n");

	if (dotype(sym))
		putf(1, "W_rval : %s;\n", mktype(sym, "", ""));

	putf(1, "W_savnum : INTEGER := w_numerrors;\n");

	if (doresync && sym->doresync)
	{
		putf(1, "W_link : W_resynclink := NEW(W_resynclink);\n");
		putf("BEGIN\n");
		putf(1, "W_link.prev := NIL;\n");
		putf(1, "W_link.resync := W_resyncset {};\n");
	}
	else
		putf("BEGIN\n");

	putf(1, "w_numerrors := 0;\n");

	if (doresync && sym->doresync)
	{
		if (dotype(sym))
			putf(1, "W_rval := W_f%s(W_link, W_follow%s, ret);\n",
					mkiname(sym));
		else
			putf(1, "W_f%s(W_link, W_follow);\n", mkiname(sym));
	}
	else
	{
		if (dotype(sym))
			putf(1, "W_rval := W_f%s(ret);\n", mkiname(sym));
		else
			putf(1, "W_f%s();\n", mkiname(sym));
	}

	putf("\n");
	putf(1, "CASE w_nexttoken() OF\n");
	printcase(1, *sym->follow);
	putf(1, "ELSE\n");
	putf(2, "w_err_illegal(\"%s\");\n", mkerrname(sym));
	putf(1, "END;\n\n");
	putf(1, "w_numerrors := w_numerrors + W_savnum;\n");
	putf(1, "w_flusherrs();\n");

	if (dotype(sym))
		putf(1, "RETURN W_rval;\n");

	putf("END %s;\n\n", mkiname(sym));
}

void
GenModula3::genparser(const char * /*header*/)
{
	Symbol *sym;
	Symnode *n;
	int i;

	putf("MODULE %s;\n\n", parserclass);
	putf("IMPORT Text, Stdio, Rd, Wr, Fmt;\n\n");

	putf("TYPE\n");

	if (doresync)
	{
		putf(1, "W_resynclink = REF RECORD\n");
		putf(2, "prev : W_resynclink;\n");
		putf(2, "resync : W_resyncset;\n");
		putf(1, "END;\n\n");
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

	// then generate all const items
	putf("CONST\n");

	if (!dumponlyparser)
	{
		putf(1, "W_tokenclasss = ARRAY OF TEXT {\n");
		putf(2, "\"[] <empty>\"");

		for (i = START; i < numsymbols(); i++)
		{
			sym = getsymbol(i);

			if (sym->type != TERMINAL || *sym->name == '\'')
				continue;

			const char *str = sym->lexstr;

			if (i > START)
				putf(",\n");

			if (str == NULL)
				putf(2, "\"%s\"", mkiname(sym));
			else if (isalpha(*str))
				putf(2, "\"%s\"", str);
			else if (*str == '"' && str[strlen(str) - 1] == '"')
				putf(2, "%sn", sym->lexstr);
			else
				putf(2, "\"%s\"", mkiname(sym));
		}

		putf(1, "\n");
		putf(1, "};\n");
	}

	putf(1, "\n");

	if (doresync)
	{
		i = 1;

		for (Setiter setiter(settab); setiter; ++setiter)
		{
			Set &s = setiter();
			printset(strbldf("W_resync%d", s.id = i++), *s.set);
		}

		putf("\n");
	}

	// now variables
	putf("VAR\n");

	if (!dumponlyparser)
	{
		putf(1, "w_numerrors : INTEGER := 0;\n");
		putf(1, "w_currtoken : INTEGER := -1;\n");
	}

	putf("\n");

	// user startup code
	mkcode(1, startcode);

	if (!dumponlyparser)
	{
		// our procedures
		putf("PROCEDURE w_tokenname(tok : INTEGER) : TEXT =\n");
		putf("BEGIN\n");
		putf(1, "IF tok >= W_EMPTY THEN\n");
		putf(2, "RETURN W_tokenclasss[tok - W_EMPTY];\n");
		putf(1, "END;\n\n");
		putf(1, "IF tok = W_EOI THEN\n");
		putf(2, "RETURN \"EOI <end-of-input>\";\n");
		putf(1, "END;\n\n");
		putf(1, "RETURN Fmt.Char(VAL(tok, CHAR));\n");
		putf("END w_tokenname;\n\n");

		putf("PROCEDURE w_nexttoken() : INTEGER =\n");
		putf("BEGIN\n");
		putf(1, "IF w_currtoken < 0 THEN\n");
		putf(2, "w_currtoken := w_gettoken();\n");
		putf(1, "END;\n\n");
		putf(1, "RETURN w_currtoken;\n");
		putf("END w_nexttoken;\n\n");

		putf("PROCEDURE w_skiptoken() =\n");
		putf("BEGIN\n");
		putf(1, "w_currtoken := -1;\n");
		putf("END w_skiptoken;\n\n");

		if (doresync)
		{
			putf("PROCEDURE W_scantoken(expect : INTEGER; lnk :"
					" W_resynclink;\n");
			putf(2, "READONLY resync : W_resyncset) =\n");

			putf("VAR\n");
			putf(1, "rlink : W_resynclink := NEW(W_resynclink);\n");
			putf(1, "link : W_resynclink;\n");
			putf(1, "level : INTEGER := 1;\n");
			putf(1, "l : INTEGER;\n");

			putf("BEGIN\n");
			putf(1, "rlink.prev := lnk;\n");
			putf(1, "rlink.resync := resync;\n\n");
		}
		else
		{
			putf("PROCEDURE W_scantoken(expect : INTEGER) =\n");
			putf("BEGIN\n");
		}

		putf(1, "IF w_currtoken < 0 THEN\n");
		putf(2, "w_currtoken := w_gettoken();\n");
		putf(1, "END;\n\n");
		putf(1, "IF expect >= 0 AND w_currtoken # expect THEN\n");
		putf(2, "w_err_expected(w_tokenname(expect));\n");
		putf(1, "END;\n\n");

		if (doresync)
		{
			putf(1, "WHILE w_currtoken # expect DO\n");
			putf(2, "l := level;\n");
			putf(2, "link := rlink;\n");
			putf(2, "\n");
			putf(2, "WHILE link # NIL AND l > 0 DO\n");
			putf(3, "IF w_currtoken IN link.resync THEN\n");
			putf(4, "RETURN;\n");
			putf(3, "END;\n");
			putf(2, "\n");
			putf(3, "l := l - 1;\n");
			putf(3, "link := link.prev;\n");
			putf(2, "END;\n");
			putf(2, "\n");
		}
		else
		{
			putf(1, "WHILE w_currtoken # expect AND w_currtoken # W_EOI DO\n");
		}

		putf(2, "w_err_skip();\n");
		putf(2, "w_currtoken := w_gettoken();\n\n");
		putf(2, "IF w_currtoken = expect THEN\n");
		putf(3, "EXIT;\n");
		putf(2, "END;\n");

		if (doresync)
		{
			putf("\n");
			putf(2, "level := level + 1;\n");
		}

		putf(1, "END;\n");
		putf(1, "\n");
		putf(1, "w_currtoken := -1;\n");
		putf("END W_scantoken;\n");
		putf("\n");

		// dump out all symbols
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

		if (doresync && sym->doresync)
			putf("PROCEDURE W_f%s(W_lnk : W_resynclink;"
					" READONLY W_resync : W_resyncset%s)",
					mkiname(sym), mktype(sym, "; W_rr : REF ", ""));
		else
			putf("PROCEDURE W_f%s(%s)",
					mkiname(sym), mktype(sym, "W_rr : REF ", ""));

		putf("%s =\n", mkvoidtype(sym, ": ", ""));
		putf("VAR\n");

		if (doresync)
			putf(1, "W_link : W_resynclink := NEW(W_resynclink);\n");

		//if (sym->usedret & RET_CODE)
			//quit("cannot use $? syntax with Modula3 for %s", mkerrname(sym));

		putf("BEGIN\n");
		mkcode(1, sym->symcode);
		putf("\n");

		if (doresync)
		{
			if (sym->doresync)
			{
				putf(1, "W_link.prev := W_lnk;\n");
				putf(1, "W_link.resync := W_resync;\n\n");

				if (sym->resync != NULL)
					putf(1, "W_scantoken(-1, W_link, W_resync%d);\n",
							sym->resync->id);
			}
			else
			{
				putf(1, "W_link.prev := NIL;\n");
				putf(1, "W_link.resync := W_resync%d;\n\n", sym->resync->id);
			}
		}

		if (sym->node != NULL && sym->node->orelse == NULL)
		{
			genstmt(1, sym->node);
			mkcode(1, sym->endcode);

			if (dotype(sym))
				putf(1, "RETURN W_rr;\n");
			else
				putf(1, "RETURN;\n");

			putf("END W_f%s;\n\n", mkiname(sym));
			continue;
		}

		boolean donedefault = FALSE;
		putf(1, "CASE w_nexttoken() OF\n");

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
				putf(1, "ELSE\n");
				genstmt(2, n);

				if (!sym->first->has(EMPTY))
				{
					putf(2, "w_err_illegal(\"%s\");\n", mkerrname(sym));
					mkcode(2, sym->endcode);

					if (dotype(sym))
						putf(2, "RETURN W_rr;\n");
					else
						putf(2, "RETURN;\n");
				}
			}
			else
			{
				putf("\n");

				if (n != sym->node)
				{
					putf(1, "| ");
					printcase(0, set);
				}
				else
					printcase(1, set);

				genstmt(2, n);
			}
		}

		if (!donedefault && !sym->first->has(EMPTY))
		{
			putf("\n");
			putf(1, "ELSE\n");
			putf(2, "w_err_illegal(\"%s\");\n", mkerrname(sym));
			mkcode(2, sym->endcode);

			if (dotype(sym))
				putf(2, "RETURN W_rr;\n");
			else
				putf(2, "RETURN;\n");
		}

		putf(1, "END;\n");
		mkcode(1, sym->endcode);

		if (dotype(sym))
		{
			putf("\n");
			putf(1, "RETURN W_rr;\n");
		}

		putf("END W_f%s;\n\n", mkiname(sym));
	}

	putf("BEGIN\n");
	/* TODO */
	putf("END %s.\n", parserclass);
}

void
GenModula3::gencode(const char *infile, const char *headername,
			const char *parsername, const char *scannername)
{
	char *s;

	inputfile = infile;

	if (headername == NULL)
		headername = "Wparse.i3";

	if (scannername == NULL)
		scannername = "Wscanner.l";

	if (parsername == NULL)
		parsername = "Wparse.m3";


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
