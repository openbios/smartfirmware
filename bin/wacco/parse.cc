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

#include "toks.h"
#define YYTEXT_DECL char *yytext

#line 1 "wacco.w"

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

#line 31 "parse.cc"

extern YYTEXT_DECL;
int w_numerrors = 0;
int w_currtoken = -1;

struct w_resynclink
{
	struct w_resynclink *prev;
	int *resync;
};

struct W_T__P6
{
	int v; const char *n;
};

#if defined(__cplusplus) || defined(__STDC__)
#ifdef __cplusplus
extern "C" {
#endif
static int w_fprogram(struct w_resynclink *, int *);
static int w_fidtype(struct w_resynclink *, int *, const char* *);
static int w_fconcatlist(struct w_resynclink *, int *, Nodedat *);
static int w_forlist(struct w_resynclink *, int *, Nodedat *);
static int w_fexprlist(struct w_resynclink *, int *, Nodedat *);
static int w_fresyncset(struct w_resynclink *, int *, Resynclist* *);
static int w_fresyncitem(struct w_resynclink *, int *, Resynclist* *);
static int w_fsettype(struct w_resynclink *, int *, int *);
static int w_fcount(struct w_resynclink *, int *, int *);
static int w_falias(struct w_resynclink *, int *, const char* *);
static int w_fcode(struct w_resynclink *, int *, Symbol* *);
#ifdef __cplusplus
}
#endif
#else /* K&R */
static int w_fprogram();
static int w_fidtype();
static int w_fconcatlist();
static int w_forlist();
static int w_fexprlist();
static int w_fresyncset();
static int w_fresyncitem();
static int w_fsettype();
static int w_fcount();
static int w_falias();
static int w_fcode();
#endif /* K&R */

static char *w_toknams[] =
{
	"[] <empty>",
	"DIRECTIVE",
	"ID",
	"EXPORT",
	"NORESYNC",
	"INLINE",
	"NOINLINE",
	"EXITCODE",
	"NULLSYM",
	"STRING",
	"CHARACTER",
	"INT",
	"BLOCKCODE",
};

#if defined(__cplusplus) || defined(__STDC__)
const char *
w_tokenname(int tok)
#else
char *
w_tokenname(tok)
int tok;
#endif
{
	static char buf[2];

	if (tok >= W_EMPTY)
		return w_toknams[tok - W_EMPTY];

	if (tok == W_EOI)
		return "EOI <end-of-input>";

	buf[0] = tok;
	buf[1] = '\0';
	return buf;
}

int
#if defined(__cplusplus) || defined(__STDC__)
w_nexttoken(void)
#else
w_nexttoken()
#endif
{
	return (w_currtoken < 0) ? (w_currtoken = w_gettoken()) : w_currtoken;
}

#define w_nexttoken() ((w_currtoken < 0) ? (w_currtoken = w_gettoken()) : w_currtoken)

#if defined(__cplusplus) || defined(__STDC__)
void
w_skiptoken(void)
#else
w_skiptoken()
#endif
{
	w_currtoken = -1;
}

#define w_skiptoken() (w_currtoken = -1)

static int
#if defined(__cplusplus) || defined(__STDC__)
w_scantoken(int expect, struct w_resynclink *lnk,int *resync)
#else
w_scantoken(expect, lnk, resync)
int expect;
struct w_resynclink *lnk;
int *resync;
#endif
{
	struct w_resynclink rlink;
	struct w_resynclink *link;
	int level = 1;

	rlink.prev = lnk;
	rlink.resync = resync;

	if (w_currtoken < 0)
		w_currtoken = w_gettoken();

	if (expect >= 0 && w_currtoken != expect)
		w_err_expected(w_tokenname(expect));

	while (w_currtoken != expect)
	{
		int i;
		int l = level;

		for (link = &rlink; link != NULL && l-- > 0; link = link->prev)
			for (i = 0; link->resync && link->resync[i] >= 0; i++)
				if (w_currtoken == link->resync[i])
					return -1;

		w_err_skip();
		w_currtoken = w_gettoken();

		if (w_currtoken == expect)
			break;

		level++;
	}
	
	w_currtoken = -1;
	return expect;
}

int
#if defined(__cplusplus) || defined(__STDC__)
program(void)
#else
program()
#endif
{
	int w_rval;
	int w_savnum = w_numerrors;
	struct w_resynclink w_link;
	static int w_follow[] = { W_EOI, -1 };

	w_link.prev = NULL;
	w_link.resync = NULL;
	w_numerrors = 0;
	w_rval = w_fprogram(&w_link, w_follow);

	switch (w_nexttoken())
	{
	case W_EOI:
		break;

	default:
		w_err_illegal("program");
		w_rval = W_RETERR;
		break;
	}

	if (w_numerrors > 0)
		w_rval = W_RETERR;

	w_numerrors += w_savnum;
	w_flusherrs();
	return w_rval;
}


static int w_resync1[] = { W_EMPTY, NORESYNC, INLINE, NOINLINE, '<', '(', -1 };
static int w_resync2[] = { W_EMPTY, ID, '*', '+', '?', '[', NULLSYM, STRING, CHARACTER, '#', '(', '{', BLOCKCODE, -1 };
static int w_resync3[] = { W_EMPTY, ID, '=', -1 };
static int w_resync4[] = { W_EMPTY, ID, '=', '|', NULLSYM, STRING, CHARACTER, '#', '(', ')', '{', BLOCKCODE, -1 };
static int w_resync5[] = { STRING, -1 };
static int w_resync6[] = { W_EMPTY, ID, '<', -1 };
static int w_resync7[] = { ID, -1 };
static int w_resync8[] = { '=', -1 };
static int w_resync9[] = { W_EMPTY, '*', '#', INT, -1 };
static int w_resync10[] = { W_EMPTY, ID, '+', '-', NULLSYM, STRING, CHARACTER, '#', -1 };
static int w_resync11[] = { '<', -1 };
static int w_resync12[] = { W_EMPTY, '=', -1 };
static int w_resync13[] = { W_EMPTY, ID, -1 };
static int w_resync14[] = { ';', '.', -1 };
static int w_resync15[] = { '.', -1 };
static int w_resync16[] = { ';', -1 };
static int w_resync17[] = { ',', -1 };
static int w_resync18[] = { '{', BLOCKCODE, -1 };
static int w_resync19[] = { '{', -1 };
static int w_resync20[] = { W_EMPTY, ID, NULLSYM, STRING, CHARACTER, '#', '(', -1 };
static int w_resync21[] = { '-', -1 };
static int w_resync22[] = { CHARACTER, -1 };
static int w_resync23[] = { EXPORT, -1 };
static int w_resync24[] = { W_EMPTY, '=', ')', -1 };
static int w_resync25[] = { W_EMPTY, '=', '|', ')', -1 };
static int w_resync26[] = { NOINLINE, -1 };
static int w_resync27[] = { ']', -1 };
static int w_resync28[] = { W_EMPTY, ID, NORESYNC, INLINE, NOINLINE, '|', NULLSYM, STRING, CHARACTER, '#', '(', ')', '{', BLOCKCODE, -1 };
static int w_resync29[] = { NORESYNC, -1 };
static int w_resync30[] = { W_EMPTY, EXITCODE, '{', BLOCKCODE, -1 };
static int w_resync31[] = { W_EMPTY, ':', '=', '[', '{', BLOCKCODE, -1 };
static int w_resync32[] = { INT, -1 };
static int w_resync33[] = { W_EMPTY, ID, '+', ',', '-', NULLSYM, STRING, CHARACTER, '#', -1 };
static int w_resync34[] = { W_EMPTY, ID, ':', '=', NULLSYM, STRING, CHARACTER, '#', '(', '{', BLOCKCODE, -1 };
static int w_resync35[] = { W_EMPTY, '{', BLOCKCODE, -1 };
static int w_resync36[] = { DIRECTIVE, -1 };
static int w_resync37[] = { ID, '=', INT, -1 };
static int w_resync38[] = { W_EMPTY, EXPORT, NORESYNC, INLINE, NOINLINE, '<', '[', -1 };
static int w_resync39[] = { W_EMPTY, EXITCODE, '|', -1 };
static int w_resync40[] = { W_EMPTY, DIRECTIVE, '{', BLOCKCODE, -1 };
static int w_resync41[] = { ID, INT, -1 };
static int w_resync42[] = { W_EMPTY, '+', '-', -1 };
static int w_resync43[] = { W_EMPTY, ID, '{', BLOCKCODE, -1 };
static int w_resync44[] = { BLOCKCODE, -1 };
static int w_resync45[] = { W_EMPTY, EXPORT, NORESYNC, INLINE, NOINLINE, '[', '{', BLOCKCODE, -1 };
static int w_resync46[] = { W_EMPTY, ID, '|', NULLSYM, STRING, CHARACTER, '#', '(', '{', BLOCKCODE, -1 };
static int w_resync47[] = { W_EMPTY, ID, NULLSYM, STRING, CHARACTER, '#', '(', '{', BLOCKCODE, -1 };
static int w_resync48[] = { '*', -1 };
static int w_resync49[] = { '+', -1 };
static int w_resync50[] = { W_EMPTY, '=', '*', INT, -1 };
static int w_resync51[] = { INLINE, -1 };
static int w_resync52[] = { W_EMPTY, ID, '+', ']', '-', NULLSYM, STRING, CHARACTER, '#', -1 };
static int w_resync53[] = { ':', -1 };
static int w_resync54[] = { '?', -1 };
static int w_resync55[] = { W_EMPTY, ID, NORESYNC, INLINE, NOINLINE, '<', NULLSYM, STRING, CHARACTER, '#', '(', '{', BLOCKCODE, -1 };
static int w_resync56[] = { NULLSYM, -1 };
static int w_resync57[] = { W_EMPTY, '|', -1 };
static int w_resync58[] = { W_EMPTY, '*', INT, -1 };
static int w_resync59[] = { W_EMPTY, EXITCODE, ';', '.', -1 };
static int w_resync60[] = { W_EMPTY, ID, '[', NULLSYM, STRING, CHARACTER, '#', '(', '{', BLOCKCODE, -1 };
static int w_resync61[] = { W_EMPTY, ID, '+', '[', ']', '-', NULLSYM, STRING, CHARACTER, '#', -1 };
static int w_resync62[] = { W_EMPTY, '=', '*', '#', INT, -1 };

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fprogram(struct w_resynclink *w_lnk, int *w_resync)
#else
w_fprogram(w_lnk, w_resync)
struct w_resynclink *w_lnk;
int *w_resync;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		Symbol* w_rvcode;

		{/*directives*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync40;

			switch (w_nexttoken())
			{
				case DIRECTIVE:
					{
						w_scantoken(DIRECTIVE, &w_link, w_resync36);
#line 33 "wacco.w"
						
			int argc;
			const char **argv = strsplit(yytext, " \t", TRUE, &argc);
			getoptions(argc, argv);
		
#line 324 "parse.cc"
					}

					break;

				default:
					{
					}

					break;
			}
		}/*directives*/

#line 28 "wacco.w"
		 w_rvcode = NULL; 
#line 339 "parse.cc"
		w_fcode(&w_link, w_resync43, &w_rvcode);
#line 28 "wacco.w"
		 startcode = w_rvcode; 
#line 343 "parse.cc"

		{/*statlist*/
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync13;

			{

				{/*__P1*/
					struct w_resynclink *w_lnk = &w_link;
					struct w_resynclink w_link;

					w_link.prev = w_lnk;
					w_link.resync = w_resync7;

					while (w_nexttoken() == ID)
					{
						{
							Symbol* w_rvstatement;
							Symbol* w_rvcode;

							{/*statement*/
								Symbol* *w_rr = &w_rvstatement;
								struct w_resynclink *w_lnk = &w_link;
								struct w_resynclink w_link;

								w_link.prev = w_lnk;
								w_link.resync = w_resync43;

								{
									const char* w_rvidtype;
									Symbol* w_rv_1;
									Resynclist* w_rvresyncset;
									Symbol* w_rvsymcode;
									Nodedat w_rvconcatlist;
									Nodedat w_rvorlist;
									Symbol* w_rvendcode;
									w_scantoken(ID, &w_link, w_resync6);
#line 47 "wacco.w"
									
			g_currsym = (*w_rr) = addsymbol(yytext);

			if ((*w_rr)->type == TERMINAL)
				(void)addnonterm((*w_rr));

			if (!exportedname && startsymbol == NULL)
			{
				startsymbol = (*w_rr);
				(*w_rr)->usecount += 2;
			}
		
#line 397 "parse.cc"
									w_fidtype(&w_link, w_resync38, &w_rvidtype);
#line 60 "wacco.w"
									
			if ((*w_rr)->rettype != NULL && w_rvidtype != NULL &&
					strcmp((*w_rr)->rettype, w_rvidtype) != 0)
				w_scanerr("Two types defined for %s", (*w_rr)->name);

			if ((*w_rr)->rettype == NULL)
				(*w_rr)->rettype = w_rvidtype;
			
			w_rv_1 = (*w_rr);
		
#line 410 "parse.cc"

									{/*__P2*/
										Symbol* *w_rr = &w_rv_1;
										struct w_resynclink *w_lnk = &w_link;
										struct w_resynclink w_link;

										w_link.prev = w_lnk;
										w_link.resync = w_resync45;

										switch (w_nexttoken())
										{
											case EXPORT:
												{
													w_scantoken(EXPORT, &w_link, w_resync23);
#line 71 "wacco.w"
													
				exportedname = (*w_rr)->exportsym = TRUE;
				(*w_rr)->usecount += 2;

				if (startsymbol != NULL)
				{
				    startsymbol->usecount -= 2;
				    startsymbol = NULL;
				}
			
#line 436 "parse.cc"
												}

												break;

											case NORESYNC:
												{
													w_scantoken(NORESYNC, &w_link, w_resync29);
#line 81 "wacco.w"
													 (*w_rr)->doresync = FALSE; 
#line 446 "parse.cc"
												}

												break;

											case INLINE:
												{
													w_scantoken(INLINE, &w_link, w_resync51);
#line 82 "wacco.w"
													 (*w_rr)->doinline = TRUE; 
#line 456 "parse.cc"
												}

												break;

											case NOINLINE:
												{
													w_scantoken(NOINLINE, &w_link, w_resync26);
#line 84 "wacco.w"
													
			    (*w_rr)->doinline = FALSE; 
			    (*w_rr)->usecount++;
			
#line 469 "parse.cc"
												}

												break;

											default:
												{
												}

												break;
										}
									}/*__P2*/

									w_fresyncset(&w_link, w_resync31, &w_rvresyncset);
#line 91 "wacco.w"
									
			(*w_rr)->list = w_rvresyncset;
			Nodedat end;
			end.sym = w_rvconcatlist.sym = w_rvorlist.sym = (*w_rr);
			w_rvconcatlist.prev = w_rvorlist.prev = &end;
			w_rvcode = (*w_rr);
		
#line 491 "parse.cc"
									w_fcode(&w_link, w_resync34, &w_rvsymcode);

									{/*__P3*/
										struct w_resynclink *w_lnk = &w_link;
										struct w_resynclink w_link;

										w_link.prev = w_lnk;
										w_link.resync = w_resync34;

										switch (w_nexttoken())
										{
											case ':':
												{
													w_scantoken(':', &w_link, w_resync53);
												}

												break;

											case '=':
												{
													w_scantoken('=', &w_link, w_resync8);
												}

												break;

											default:
												w_err_illegal("statement");
												break;
										}
									}/*__P3*/

#line 99 "wacco.w"
									
			(*w_rr)->symcode = w_rvsymcode;
		
#line 527 "parse.cc"
									w_fconcatlist(&w_link, w_resync46, &w_rvconcatlist);
#line 103 "wacco.w"
									
			Symnode *node = (*w_rr)->node;

			if (node != NULL)
			{
				while (node->orelse != NULL)
					node = node->orelse;

				node->orelse = w_rvconcatlist.node;
			}
			else
				node = (*w_rr)->node = w_rvconcatlist.node;
		
#line 543 "parse.cc"
									w_forlist(&w_link, w_resync39, &w_rvorlist);
#line 117 "wacco.w"
									
			if (node == NULL)
				(*w_rr)->node = w_rvorlist.node;
			else if (node->orelse == NULL)
				node->orelse = w_rvorlist.node;
			else
				node->orelse->orelse = w_rvorlist.node;
		
#line 554 "parse.cc"

									{/*__P4*/
										Symbol* *w_rr = &w_rvendcode;
										struct w_resynclink *w_lnk = &w_link;
										struct w_resynclink w_link;

										w_link.prev = w_lnk;
										w_link.resync = w_resync59;

										switch (w_nexttoken())
										{
											case EXITCODE:
												{
													Symbol* w_rvcode;
													w_scantoken(EXITCODE, &w_link, w_resync30);
													w_fcode(&w_link, w_resync35, &w_rvcode);
#line 125 "wacco.w"
													 (*w_rr) = w_rvcode; 
#line 573 "parse.cc"
												}

												break;

											default:
												{
#line 125 "wacco.w"
													 (*w_rr) = NULL; 
#line 582 "parse.cc"
												}

												break;
										}
									}/*__P4*/

#line 126 "wacco.w"
									
			(*w_rr)->endcode = w_rvendcode;
		
#line 593 "parse.cc"

									{/*__P5*/
										struct w_resynclink *w_lnk = &w_link;
										struct w_resynclink w_link;

										w_link.prev = w_lnk;
										w_link.resync = w_resync14;

										switch (w_nexttoken())
										{
											case ';':
												{
													w_scantoken(';', &w_link, w_resync16);
												}

												break;

											case '.':
												{
													w_scantoken('.', &w_link, w_resync15);
												}

												break;

											default:
												w_err_illegal("statement");
												break;
										}
									}/*__P5*/

								}

							}/*statement*/

#line 42 "wacco.w"
							 w_rvcode = NULL; 
#line 630 "parse.cc"
							w_fcode(&w_link, w_resync35, &w_rvcode);
#line 42 "wacco.w"
							 addnonterm(w_rvcode); 
#line 634 "parse.cc"
						}

					}
				}/*__P1*/

			}

		}/*statlist*/

	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fidtype(struct w_resynclink *w_lnk, int *w_resync, const char* *w_rr)
#else
w_fidtype(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
const char* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case '<':
		{
			w_scantoken('<', &w_link, w_resync11);
#line 133 "wacco.w"
			 (*w_rr) = readtype(); 
#line 671 "parse.cc"
		}

		break;

	default:
		{
#line 134 "wacco.w"
			 (*w_rr) = NULL; 
#line 680 "parse.cc"
		}

		break;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fconcatlist(struct w_resynclink *w_lnk, int *w_resync, Nodedat *w_rr)
#else
w_fconcatlist(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Nodedat *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case ID:
	case NULLSYM:
	case STRING:
	case CHARACTER:
	case '#':
	case '(':
	case '{':
	case BLOCKCODE:
		{
			Symbol* w_rvcode;
			Nodedat w_rvexprlist;
#line 138 "wacco.w"
			
			w_rvcode = w_rvexprlist.sym = (*w_rr).sym;
			w_rvexprlist.prev = (*w_rr).prev;
		
#line 722 "parse.cc"
			w_fcode(&w_link, w_resync47, &w_rvcode);
			w_fexprlist(&w_link, w_resync20, &w_rvexprlist);
#line 143 "wacco.w"
			
			if (w_rvcode == NULL)
				(*w_rr).node = w_rvexprlist.node;
			else
			{
				(*w_rr).node = new Symnode;
				(*w_rr).node->sym = w_rvcode;
				(*w_rr).node->next = w_rvexprlist.node;
			}
		
#line 736 "parse.cc"
		}

		break;

	default:
		{
		}

		break;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_forlist(struct w_resynclink *w_lnk, int *w_resync, Nodedat *w_rr)
#else
w_forlist(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Nodedat *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case '|':
		{
			Nodedat w_rvconcatlist;
			Nodedat w_rvorlist;
#line 157 "wacco.w"
			
			w_rvconcatlist.sym = w_rvorlist.sym = (*w_rr).sym;
			w_rvconcatlist.prev = w_rvorlist.prev = (*w_rr).prev;
		
#line 777 "parse.cc"
			w_scantoken('|', &w_link, w_resync46);
			w_fconcatlist(&w_link, w_resync46, &w_rvconcatlist);
			w_forlist(&w_link, w_resync57, &w_rvorlist);
#line 162 "wacco.w"
			
			(*w_rr).node = w_rvconcatlist.node;

			if ((*w_rr).node == NULL)
				(*w_rr).node = w_rvorlist.node;
			else
				(*w_rr).node->orelse = w_rvorlist.node;
		
#line 790 "parse.cc"
		}

		break;

	default:
		{
		}

		break;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fexprlist(struct w_resynclink *w_lnk, int *w_resync, Nodedat *w_rr)
#else
w_fexprlist(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Nodedat *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case ID:
	case NULLSYM:
	case STRING:
	case CHARACTER:
	case '#':
	case '(':
		{
			Nodedat w_rvexpression;
			struct W_T__P6 w_rv_;
			Resynclist* w_rvresyncset;
			Symbol* w_rvcode;
			Nodedat w_rvexprlist;
#line 175 "wacco.w"
			
			w_rvexpression.sym = w_rvcode = w_rvexprlist.sym = (*w_rr).sym;
			w_rvexpression.prev = w_rvexprlist.prev = (*w_rr).prev;
		
#line 839 "parse.cc"

			{/*expression*/
				Nodedat *w_rr = &w_rvexpression;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync2;

				switch (w_nexttoken())
				{
					case ID:
						{
							const char* w_rvalias;
							w_scantoken(ID, &w_link, w_resync3);
#line 266 "wacco.w"
							
			(*w_rr).sym = addsymbol(yytext);
			(*w_rr).sym->usecount++;

			if ((*w_rr).sym == g_currsym)
			    (*w_rr).sym->doinline = FALSE;
		
#line 863 "parse.cc"
							w_falias(&w_link, w_resync12, &w_rvalias);
#line 273 "wacco.w"
							 (*w_rr).alias = w_rvalias; 
#line 867 "parse.cc"
						}

						break;

					case NULLSYM:
						{
							w_scantoken(NULLSYM, &w_link, w_resync56);
#line 274 "wacco.w"
							 (*w_rr).sym = getsymbol(EMPTY); 
#line 877 "parse.cc"
						}

						break;

					case '#':
						{
							int w_rvcount;
							const char* w_rvalias;
							w_scantoken('#', &w_link, w_resync62);
							w_fcount(&w_link, w_resync50, &w_rvcount);
							w_falias(&w_link, w_resync12, &w_rvalias);
#line 276 "wacco.w"
							
			Nodedat *p = (*w_rr).prev;
			int i;
			
			for (i = 0; p != NULL &&
				(w_rvcount < 0 || i < w_rvcount); i++)
			{
				(*w_rr).sym = p->sym;
				p = p->prev;
			}

			if (w_rvcount > 0 && i < w_rvcount)
				w_scanerr("not that many parentheses");

			(*w_rr).sym->usecount++;
			(*w_rr).alias = w_rvalias;

			if ((*w_rr).sym == g_currsym)
			    (*w_rr).sym->doinline = FALSE;
		
#line 910 "parse.cc"
						}

						break;

					case STRING:
						{
							w_scantoken(STRING, &w_link, w_resync5);
#line 297 "wacco.w"
							
			(*w_rr).sym = addsymbol(yytext);
			(*w_rr).sym->lexstr = (*w_rr).sym->name;
			gotlexstr = TRUE;
		
#line 924 "parse.cc"
						}

						break;

					case CHARACTER:
						{
							w_scantoken(CHARACTER, &w_link, w_resync22);
#line 303 "wacco.w"
							
			(*w_rr).sym = addsymbol(yytext);

			if ((*w_rr).sym->lexstr == NULL)
			{
				yytext[0] = yytext[strlen(yytext) - 1] = '"';
				(*w_rr).sym->lexstr = strget(yytext);
			}
		
#line 942 "parse.cc"
						}

						break;

					case '(':
						{
							const char* w_rvidtype;
							Symbol* w_rv_;
							Nodedat w_rvconcatlist;
							Nodedat w_rvorlist;
							const char* w_rvalias;
							w_scantoken('(', &w_link, w_resync1);
							w_fidtype(&w_link, w_resync55, &w_rvidtype);
#line 313 "wacco.w"
							
			const char *type = (*w_rr).sym->rettype;
			const char *name = (*w_rr).sym->realname;
			Symbol *prevsym = g_currsym;
			static int num = 1;

			addnonterm((*w_rr).sym = addsymbol(strbldf("__P%d", num++)));
			(*w_rr).sym->type = NONTERMINAL;
			(*w_rr).sym->rettype = w_rvidtype == NULL ? type : w_rvidtype;
			(*w_rr).sym->realname = name;
			(*w_rr).sym->usecount++;

			w_rv_ = g_currsym = (*w_rr).sym;
			w_rvconcatlist.sym = w_rvorlist.sym = (*w_rr).sym;
			w_rvconcatlist.prev = w_rvorlist.prev = &(*w_rr);
		
#line 973 "parse.cc"

							{/*__P9*/
								Symbol* *w_rr = &w_rv_;
								struct w_resynclink *w_lnk = &w_link;
								struct w_resynclink w_link;

								w_link.prev = w_lnk;
								w_link.resync = w_resync28;

								switch (w_nexttoken())
								{
									case NORESYNC:
										{
											w_scantoken(NORESYNC, &w_link, w_resync29);
#line 330 "wacco.w"
											 (*w_rr)->doresync = FALSE; 
#line 990 "parse.cc"
										}

										break;

									case INLINE:
										{
											w_scantoken(INLINE, &w_link, w_resync51);
#line 331 "wacco.w"
											 (*w_rr)->doinline = TRUE; 
#line 1000 "parse.cc"
										}

										break;

									case NOINLINE:
										{
											w_scantoken(NOINLINE, &w_link, w_resync26);
#line 333 "wacco.w"
											
				    (*w_rr)->doinline = FALSE; 
				    (*w_rr)->usecount++;
				
#line 1013 "parse.cc"
										}

										break;

									default:
										{
										}

										break;
								}
							}/*__P9*/

							w_fconcatlist(&w_link, w_resync4, &w_rvconcatlist);
							w_forlist(&w_link, w_resync25, &w_rvorlist);
							w_scantoken(')', &w_link, w_resync24);
							w_falias(&w_link, w_resync12, &w_rvalias);
#line 340 "wacco.w"
							
			(*w_rr).sym->node = w_rvconcatlist.node;

			if ((*w_rr).sym->node == NULL)
				(*w_rr).sym->node = w_rvorlist.node;
			else
				(*w_rr).sym->node->orelse = w_rvorlist.node;

			(*w_rr).alias = w_rvalias;
			g_currsym = prevsym;
		
#line 1042 "parse.cc"
						}

						break;

					default:
						w_err_illegal("expression");
						break;
				}
			}/*expression*/


			{/*__P6*/
				struct W_T__P6 *w_rr = &w_rv_;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync2;

				switch (w_nexttoken())
				{
					default:
						{
#line 181 "wacco.w"
							 (*w_rr).v = 0; (*w_rr).n = NULL; 
#line 1068 "parse.cc"
						}

						break;

					case '*':
						{
							w_scantoken('*', &w_link, w_resync48);
#line 182 "wacco.w"
							 (*w_rr).v = ZEROPLUS; (*w_rr).n = "<zero-plus>"; 
#line 1078 "parse.cc"
						}

						break;

					case '+':
						{
							w_scantoken('+', &w_link, w_resync49);
#line 183 "wacco.w"
							 (*w_rr).v = ONEPLUS; (*w_rr).n = "<one-plus>"; 
#line 1088 "parse.cc"
						}

						break;

					case '?':
						{
							w_scantoken('?', &w_link, w_resync54);
#line 184 "wacco.w"
							 (*w_rr).v = ONEZERO; (*w_rr).n = "<one-zero>"; 
#line 1098 "parse.cc"
						}

						break;
				}
			}/*__P6*/

			w_fresyncset(&w_link, w_resync60, &w_rvresyncset);
			w_fcode(&w_link, w_resync47, &w_rvcode);
			w_fexprlist(&w_link, w_resync20, &w_rvexprlist);
#line 187 "wacco.w"
			
			(*w_rr).node = new Symnode;
			(*w_rr).node->sym = w_rvexpression.sym;
			(*w_rr).node->alias = w_rvexpression.alias;
			(*w_rr).node->next = w_rvexprlist.node;
			(*w_rr).node->list = w_rvresyncset;

			if (w_rvcode != NULL)
			{
				(*w_rr).node->next = new Symnode;
				(*w_rr).node->next->sym = w_rvcode;
				(*w_rr).node->next->next = w_rvexprlist.node;
			}

			if (w_rv_.v)
			{
			    Symnode *n = new Symnode;
			    n->sym = new Symbol(w_rv_.n);
			    n->sym->type = w_rv_.v;
			    n->next = (*w_rr).node;
			    (*w_rr).node = n;
			}
		
#line 1132 "parse.cc"
		}

		break;

	default:
		{
		}

		break;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fresyncset(struct w_resynclink *w_lnk, int *w_resync, Resynclist* *w_rr)
#else
w_fresyncset(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Resynclist* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case '[':
		{
			Resynclist* w_rvresynclist;
			w_scantoken('[', &w_link, w_resync61);

			{/*resynclist*/
				Resynclist* *w_rr = &w_rvresynclist;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync52;

				{
					Resynclist* w_rvresyncitem;
					Resynclist* w_rv_;
					w_fresyncitem(&w_link, w_resync33, &w_rvresyncitem);
#line 219 "wacco.w"
					 (*w_rr) = w_rv_ = w_rvresyncitem; 
#line 1183 "parse.cc"

					{/*__P7*/
						Resynclist* *w_rr = &w_rv_;
						struct w_resynclink *w_lnk = &w_link;
						struct w_resynclink w_link;

						w_link.prev = w_lnk;
						w_link.resync = w_resync17;

						while (w_nexttoken() == ',')
						{
							{
								Resynclist* w_rvresyncitem;
								w_scantoken(',', &w_link, w_resync33);
								w_fresyncitem(&w_link, w_resync10, &w_rvresyncitem);
#line 221 "wacco.w"
								
			    (*w_rr)->next = w_rvresyncitem;
			    (*w_rr) = w_rvresyncitem;
			
#line 1204 "parse.cc"
							}

						}
					}/*__P7*/

				}

			}/*resynclist*/

			w_scantoken(']', &w_link, w_resync27);
#line 214 "wacco.w"
			 (*w_rr) = w_rvresynclist; 
#line 1217 "parse.cc"
		}

		break;

	default:
		{
#line 215 "wacco.w"
			 (*w_rr) = NULL; 
#line 1226 "parse.cc"
		}

		break;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fresyncitem(struct w_resynclink *w_lnk, int *w_resync, Resynclist* *w_rr)
#else
w_fresyncitem(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Resynclist* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	{
		int w_rvfirst;
		Resynclist* w_rvresyncid;
		int w_rvfollow;
		w_fsettype(&w_link, w_resync10, &w_rvfirst);

		{/*resyncid*/
			Resynclist* *w_rr = &w_rvresyncid;
			struct w_resynclink *w_lnk = &w_link;
			struct w_resynclink w_link;

			w_link.prev = w_lnk;
			w_link.resync = w_resync10;

			switch (w_nexttoken())
			{
				case ID:
					{
						w_scantoken(ID, &w_link, w_resync7);
#line 244 "wacco.w"
						 (*w_rr) = new Resynclist(&yytext[0]); 
#line 1271 "parse.cc"
					}

					break;

				case NULLSYM:
					{
						w_scantoken(NULLSYM, &w_link, w_resync56);
#line 245 "wacco.w"
						 (*w_rr) = new Resynclist(NULL); 
#line 1281 "parse.cc"
					}

					break;

				case STRING:
					{
						w_scantoken(STRING, &w_link, w_resync5);
#line 246 "wacco.w"
						 (*w_rr) = new Resynclist(&yytext[0]); 
#line 1291 "parse.cc"
					}

					break;

				case CHARACTER:
					{
						w_scantoken(CHARACTER, &w_link, w_resync22);
#line 247 "wacco.w"
						 (*w_rr) = new Resynclist(&yytext[0]); 
#line 1301 "parse.cc"
					}

					break;

				case '#':
					{
						int w_rvcount;
						w_scantoken('#', &w_link, w_resync9);
						w_fcount(&w_link, w_resync58, &w_rvcount);
#line 248 "wacco.w"
						 (*w_rr) = new Resynclist(NULL);
				(*w_rr)->paren = w_rvcount == 0 ? 1 : w_rvcount; 
#line 1314 "parse.cc"
					}

					break;

				default:
					w_err_illegal("resyncid");
					break;
			}
		}/*resyncid*/

		w_fsettype(&w_link, w_resync42, &w_rvfollow);
#line 230 "wacco.w"
		
			(*w_rr) = w_rvresyncid;
			(*w_rr)->first = w_rvfirst == 0 && w_rvfollow == 0 ? 1 : w_rvfirst;
			(*w_rr)->follow = w_rvfollow;
		
#line 1332 "parse.cc"
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fsettype(struct w_resynclink *w_lnk, int *w_resync, int *w_rr)
#else
w_fsettype(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
int *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case '+':
		{
			w_scantoken('+', &w_link, w_resync49);
#line 238 "wacco.w"
			 (*w_rr) = 1; 
#line 1360 "parse.cc"
		}

		break;

	case '-':
		{
			w_scantoken('-', &w_link, w_resync21);
#line 239 "wacco.w"
			 (*w_rr) = -1; 
#line 1370 "parse.cc"
		}

		break;

	default:
		{
#line 240 "wacco.w"
			 (*w_rr) = 0; 
#line 1379 "parse.cc"
		}

		break;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fcount(struct w_resynclink *w_lnk, int *w_resync, int *w_rr)
#else
w_fcount(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
int *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case INT:
		{
			w_scantoken(INT, &w_link, w_resync32);
#line 253 "wacco.w"
			 (*w_rr) = atoi(yytext); 
#line 1410 "parse.cc"
		}

		break;

	case '*':
		{
			w_scantoken('*', &w_link, w_resync48);
#line 254 "wacco.w"
			 (*w_rr) = -1; 
#line 1420 "parse.cc"
		}

		break;

	default:
		{
#line 255 "wacco.w"
			 (*w_rr) = 0; 
#line 1429 "parse.cc"
		}

		break;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_falias(struct w_resynclink *w_lnk, int *w_resync, const char* *w_rr)
#else
w_falias(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
const char* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case '=':
		{
			const char* w_rv_;
			w_scantoken('=', &w_link, w_resync37);

			{/*__P8*/
				const char* *w_rr = &w_rv_;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync41;

				switch (w_nexttoken())
				{
					case ID:
						{
							w_scantoken(ID, &w_link, w_resync7);
#line 259 "wacco.w"
							 (*w_rr) = yytext; 
#line 1475 "parse.cc"
						}

						break;

					case INT:
						{
							w_scantoken(INT, &w_link, w_resync32);
#line 259 "wacco.w"
							 (*w_rr) = yytext; 
#line 1485 "parse.cc"
						}

						break;

					default:
						w_err_illegal("alias");
						break;
				}
			}/*__P8*/

#line 260 "wacco.w"
			 (*w_rr) = strget(w_rv_); 
#line 1498 "parse.cc"
		}

		break;

	default:
		{
#line 261 "wacco.w"
			 (*w_rr) = NULL; 
#line 1507 "parse.cc"
		}

		break;
	}

	return W_RETOK;
}

static int
#if defined(__cplusplus) || defined(__STDC__)
w_fcode(struct w_resynclink *w_lnk, int *w_resync, Symbol* *w_rr)
#else
w_fcode(w_lnk, w_resync, w_rr)
struct w_resynclink *w_lnk;
int *w_resync;
Symbol* *w_rr;
#endif
{
	struct w_resynclink w_link;

	w_link.prev = w_lnk;
	w_link.resync = w_resync;

	switch (w_nexttoken())
	{
	case '{':
	case BLOCKCODE:
		{
			const char* w_rv_;
#line 354 "wacco.w"
			
			int rettype = 0;
			int line = w_currline();
		
#line 1542 "parse.cc"

			{/*__P10*/
				const char* *w_rr = &w_rv_;
				struct w_resynclink *w_lnk = &w_link;
				struct w_resynclink w_link;

				w_link.prev = w_lnk;
				w_link.resync = w_resync18;

				switch (w_nexttoken())
				{
					case '{':
						{
							w_scantoken('{', &w_link, w_resync19);
#line 359 "wacco.w"
							 (*w_rr) = readcode(rettype); 
#line 1559 "parse.cc"
						}

						break;

					case BLOCKCODE:
						{
							w_scantoken(BLOCKCODE, &w_link, w_resync44);
#line 360 "wacco.w"
							 (*w_rr) = readblockcode(rettype); 
#line 1569 "parse.cc"
						}

						break;

					default:
						w_err_illegal("code");
						break;
				}
			}/*__P10*/

#line 362 "wacco.w"
			
			if ((*w_rr) != NULL)
				(*w_rr)->usedret |= rettype;

			(*w_rr) = new Symbol("<code>");
			(*w_rr)->type = CODE;
			(*w_rr)->code = w_rv_;
			(*w_rr)->line = line;
		
#line 1590 "parse.cc"
		}

		break;

	default:
		{
#line 371 "wacco.w"
			 (*w_rr) = NULL; 
#line 1599 "parse.cc"
		}

		break;
	}

	return W_RETOK;
}

