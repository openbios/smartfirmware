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

{
// Copyright (c) 1991-2002 by Parag Patel.  All Rights Reserved.

#undef YYTEXT_DECL
#define YYTEXT_DECL char yytext[]

#include "defs.h"
#include "toks.h"

class Nodedat
{
public:
	Symbol *sym;
	union
	{
		Symnode *node;
		const char *alias;
	};
	Nodedat *prev;

	Nodedat() { sym = NULL; node = NULL; prev = NULL; }
};

static Symbol *g_currsym = NULL;
}

program
	: directives { $code = NULL; } code { startcode = $code; } statlist
	;

directives
	: DIRECTIVE
		{
			int argc;
			const char **argv = strsplit(yytext, " \t", TRUE, &argc);
			getoptions(argc, argv);
		}
	| []
	;

statlist
	: ( statement { $code = NULL; } code { addnonterm($code); } )*
	;

statement<Symbol*>
	:	ID
		{
			g_currsym = $$ = addsymbol(yytext);

			if ($$->type == TERMINAL)
				(void)addnonterm($$);

			if (!exportedname && startsymbol == NULL)
			{
				startsymbol = $$;
				$$->usecount += 2;
			}
		}
		idtype
		{
			if ($$->rettype != NULL && $idtype != NULL &&
					strcmp($$->rettype, $idtype) != 0)
				w_scanerr("Two types defined for %s", $$->name);

			if ($$->rettype == NULL)
				$$->rettype = $idtype;
			
			$_1 = $$;
		}
		( EXPORT
			{
				exportedname = $$->exportsym = TRUE;
				$$->usecount += 2;

				if (startsymbol != NULL)
				{
				    startsymbol->usecount -= 2;
				    startsymbol = NULL;
				}
			}
		| NORESYNC { $$->doresync = FALSE; }
		| INLINE { $$->doinline = TRUE; }
		| NOINLINE
			{
			    $$->doinline = FALSE; 
			    $$->usecount++;
			}
		| []
		)
		resyncset
		{
			$$->list = $resyncset;
			Nodedat end;
			end.sym = $concatlist.sym = $orlist.sym = $$;
			$concatlist.prev = $orlist.prev = &end;
			$code = $$;
		}
		code=symcode (<void> ':' | '=')
		{
			$$->symcode = $symcode;
		}
		concatlist
		{
			Symnode *node = $$->node;

			if (node != NULL)
			{
				while (node->orelse != NULL)
					node = node->orelse;

				node->orelse = $concatlist.node;
			}
			else
				node = $$->node = $concatlist.node;
		}
		orlist
		{
			if (node == NULL)
				$$->node = $orlist.node;
			else if (node->orelse == NULL)
				node->orelse = $orlist.node;
			else
				node->orelse->orelse = $orlist.node;
		}
		( EXITCODE code { $$ = $code; } | [] { $$ = NULL; } )=endcode
		{
			$$->endcode = $endcode;
		}
		(<void> ';' | '.')
	;

idtype<const char*>
	: '<'	{ $$ = readtype(); }
	| []	{ $$ = NULL; }
	;

concatlist<Nodedat>
	:	{
			$code = $exprlist.sym = $$.sym;
			$exprlist.prev = $$.prev;
		} 
		code exprlist
		{
			if ($code == NULL)
				$$.node = $exprlist.node;
			else
			{
				$$.node = new Symnode;
				$$.node->sym = $code;
				$$.node->next = $exprlist.node;
			}
		}
	| []
	;

orlist<Nodedat>
	:	{
			$concatlist.sym = $orlist.sym = $$.sym;
			$concatlist.prev = $orlist.prev = $$.prev;
		}
		'|' concatlist orlist
		{
			$$.node = $concatlist.node;

			if ($$.node == NULL)
				$$.node = $orlist.node;
			else
				$$.node->orelse = $orlist.node;
		}
	| []
	;


exprlist<Nodedat>
	:	{
			$expression.sym = $code = $exprlist.sym = $$.sym;
			$expression.prev = $exprlist.prev = $$.prev;
		}
		expression
			(<int v; const char *n>
				[] { $$.v = 0; $$.n = NULL; }
				| '*' { $$.v = ZEROPLUS; $$.n = "<zero-plus>"; }
				| '+' { $$.v = ONEPLUS; $$.n = "<one-plus>"; }
				| '?' { $$.v = ONEZERO; $$.n = "<one-zero>"; }
			)
		resyncset code exprlist
		{
			$$.node = new Symnode;
			$$.node->sym = $expression.sym;
			$$.node->alias = $expression.alias;
			$$.node->next = $exprlist.node;
			$$.node->list = $resyncset;

			if ($code != NULL)
			{
				$$.node->next = new Symnode;
				$$.node->next->sym = $code;
				$$.node->next->next = $exprlist.node;
			}

			if ($_.v)
			{
			    Symnode *n = new Symnode;
			    n->sym = new Symbol($_.n);
			    n->sym->type = $_.v;
			    n->next = $$.node;
			    $$.node = n;
			}
		}
	| []
	;

resyncset<Resynclist*>
	: '[' resynclist ']'	{ $$ = $resynclist; }
	| []			{ $$ = NULL; }
	;

resynclist<Resynclist*>
	: resyncitem { $$ = $_ = $resyncitem; }
		(',' resyncitem
			{
			    $$->next = $resyncitem;
			    $$ = $resyncitem;
			}
		)*
	;

resyncitem<Resynclist*>
	: settype=first resyncid settype=follow
		{
			$$ = $resyncid;
			$$->first = $first == 0 && $follow == 0 ? 1 : $first;
			$$->follow = $follow;
		}
	;

settype
	: '+'	{ $$ = 1; }
	| '-'	{ $$ = -1; }
	| []	{ $$ = 0; }
	;

resyncid<Resynclist*>
	: ID		{ $$ = new Resynclist(&yytext[0]); }
	| NULLSYM	{ $$ = new Resynclist(NULL); }
	| STRING	{ $$ = new Resynclist(&yytext[0]); }
	| CHARACTER	{ $$ = new Resynclist(&yytext[0]); }
	| '#' count	{ $$ = new Resynclist(NULL);
				$$->paren = $count == 0 ? 1 : $count; }
	;

count
	: INT	{ $$ = atoi(yytext); }
	| '*'	{ $$ = -1; }
	| []	{ $$ = 0; }
	;

alias<const char*>
	: '=' (ID { $$ = yytext; } | INT { $$ = yytext; })
			{ $$ = strget($_); }
	| []	{ $$ = NULL; }
	;

expression<Nodedat>
	: ID	
		{
			$$.sym = addsymbol(yytext);
			$$.sym->usecount++;

			if ($$.sym == g_currsym)
			    $$.sym->doinline = FALSE;
		}
		alias	{ $$.alias = $alias; }
	| NULLSYM	{ $$.sym = getsymbol(EMPTY); }
	| '#' count alias
		{
			Nodedat *p = $$.prev;
			int i;
			
			for (i = 0; p != NULL &&
				($count < 0 || i < $count); i++)
			{
				$$.sym = p->sym;
				p = p->prev;
			}

			if ($count > 0 && i < $count)
				w_scanerr("not that many parentheses");

			$$.sym->usecount++;
			$$.alias = $alias;

			if ($$.sym == g_currsym)
			    $$.sym->doinline = FALSE;
		}
	| STRING
		{
			$$.sym = addsymbol(yytext);
			$$.sym->lexstr = $$.sym->name;
			gotlexstr = TRUE;
		}
	| CHARACTER
		{
			$$.sym = addsymbol(yytext);

			if ($$.sym->lexstr == NULL)
			{
				yytext[0] = yytext[strlen(yytext) - 1] = '"';
				$$.sym->lexstr = strget(yytext);
			}
		}
	| '(' idtype
		{
			const char *type = $$.sym->rettype;
			const char *name = $$.sym->realname;
			Symbol *prevsym = g_currsym;
			static int num = 1;

			addnonterm($$.sym = addsymbol(strbldf("__P%d", num++)));
			$$.sym->type = NONTERMINAL;
			$$.sym->rettype = $idtype == NULL ? type : $idtype;
			$$.sym->realname = name;
			$$.sym->usecount++;

			$_ = g_currsym = $$.sym;
			$concatlist.sym = $orlist.sym = $$.sym;
			$concatlist.prev = $orlist.prev = &$$;
		}
		(<Symbol*>
			  NORESYNC { $$->doresync = FALSE; }
			| INLINE { $$->doinline = TRUE; }
			| NOINLINE
				{
				    $$->doinline = FALSE; 
				    $$->usecount++;
				}
			| []
		)
		concatlist orlist ')' alias
		{
			$$.sym->node = $concatlist.node;

			if ($$.sym->node == NULL)
				$$.sym->node = $orlist.node;
			else
				$$.sym->node->orelse = $orlist.node;

			$$.alias = $alias;
			g_currsym = prevsym;
		}
	;

code<Symbol*>
	:	{
			int rettype = 0;
			int line = w_currline();
		}
		(<const char*>
			'{'		{ $$ = readcode(rettype); }
			| BLOCKCODE	{ $$ = readblockcode(rettype); }
		)
		{
			if ($$ != NULL)
				$$->usedret |= rettype;

			$$ = new Symbol("<code>");
			$$->type = CODE;
			$$->code = $_;
			$$->line = line;
		}
	| []	{ $$ = NULL; }
	;
