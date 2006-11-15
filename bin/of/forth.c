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

/* (c) Copyright 1996-2005 by CodeGen, Inc.  All Rights Reserved. */

#include "defs.h"


/* externs to control.c Forth/C funcs */
EC(f_bcase);
EC(f_bendcase);
EC(f_bltmark);
EC(f_bleave);
EC(f_bconstant);
EC(f_bvalue);
EC(f_bvariable);
EC(f_bbuffercolon);
EC(f_bcolon);
EC(f_bsemicolon);
EC(f_bdefer);
EC(f_bfield);
EC(f_bcreate);


/* 7.3  Forth language command group */

/* all Forth words that are not also FCodes are defined here */


/* 3dup (x1 x2 x3 -- x1 x2 x3 x1 x2 x3) */

C(f_clear)			/* clear (... --) */
{
	e->sp = e->stack;
	return NO_ERROR;
}

/* 3drop (x1 x2 x3 --) */

C(f_utimes)			/* u* (u1 u2 -- uprod) */
{
	IFCKSP(e, 2, 1);
	e->sp[-1] = (uCell)e->sp[-1] * (uCell)e->sp[0];
	DROP(e);
	return NO_ERROR;
}

/* use "star-slash" for the two characters which delimint end-comment in C: */

C(f_multdiv)		/* star-slash (n1 n2 n3 -- quot) */
{
	Int n1, n2, n3;
	
	IFCKSP(e, 3, 1);
	POP(e, n3);
	POP(e, n2);
	POP(e, n1);
	PUSH(e, n1 * n2 / n3);
	return NO_ERROR;
}

C(f_multdivmod)		/* star-slash-mod (n1 n2 n3 -- rem quot) */
{
	Int n1, n2, n3;
	
	IFCKSP(e, 3, 2);
	POP(e, n3);
	POP(e, n2);
	POP(e, n1);

	if (n3 == 0)
		return E_DIVIDE_BY_ZERO;

	PUSH(e, n1 * n2 % n3);
	PUSH(e, n1 * n2 / n3);
	return NO_ERROR;
}

/* 1+ (nu1 -- nu2) */

/* 1- (nu1 -- nu2) */

/* 2+ (nu1 -- nu2) */

/* 2- (nu1 -- nu2) */

C(f_even)			/* even (n -- n | n+1) */
{
	IFCKSP(e, 1, 0);
	
	if (e->sp[0] & 0x01)
		e->sp[0] += 1;
	
	return NO_ERROR;
}

C(f_stod)			/* s>d (n1 -- d1) */
{
	Cell c;
	
	IFCKSP(e, 1, 2);
	c = TOP(e);
	
	/* push sign extension */
	if (c < 0)
		PUSH(e, -1);
	else
		PUSH(e, 0);

	return NO_ERROR;
}

C(f_dnegate)		/* dnegate (dn -- -dn) [non-standard] */
{
	EC(f_dminus);

	IFCKSP(e, 2, 4);
	e->sp[2] = e->sp[0];
	e->sp[1] = e->sp[-1];
	e->sp[0] = e->sp[-1] = 0;
	e->sp += 2;
	return f_dminus(e);
}

C(f_mtimes)			/* m* (n1 n2 -- d) */
{
	Bool neg = FALSE;
	Retcode ret;
	EC(f_umtimes);

	if (TOP(e) < 0)
	{
		e->sp[0] = -e->sp[0];
		neg = TRUE;
	}

	if (STACK(e, 1) < 0)
	{
		e->sp[-1] = -e->sp[-1];
		neg = !neg;
	}

	ret = f_umtimes(e);

	if (ret != NO_ERROR)
		return ret;

	if (neg)
		ret = f_dnegate(e);

	return ret;
}

C(f_fmdivmod)		/* fm/mod (d n -- rem quot) */
{
	Bool negq = FALSE;
	Bool negr = FALSE;
	Cell d, rh, rl, qh, ql;
	Retcode ret;
	EC(f_umddivmod);
	EC(f_dplus);
	EC(f_dminus);

	IFCKSP(e, 3, 2);
	POP(e, d);

	if (STACK(e, 1) < 0)
	{
		negq = TRUE;
		ret = f_dnegate(e);

		if (ret != NO_ERROR)
			return ret;
	}

	if (d < 0)
	{
		d = -d;
		negq = !negq;
		negr = TRUE;
	}

	PUSH(e, d);
	PUSH(e, 0);
	ret = f_umddivmod(e);

	if (ret != NO_ERROR)
		return ret;

	POP(e, qh);
	POP(e, ql);
	POP(e, rh);
	POP(e, rl);

	if (negq)
	{
		if (rh || rl)
		{
			/* q += 1 */
			PUSH(e, ql);
			PUSH(e, qh);
			PUSH(e, 1);
			PUSH(e, 0);
			ret = f_dplus(e);

			if (ret != NO_ERROR)
				return ret;

			POP(e, qh);
			POP(e, ql);

			/* m = den - m; */
			PUSH(e, d);
			PUSH(e, 0);
			PUSH(e, rl);
			PUSH(e, rh);
			ret = f_dminus(e);

			if (ret != NO_ERROR)
				return ret;

			POP(e, rh);
			POP(e, rl);
		}

		/* q = -q */
		PUSH(e, ql);
		PUSH(e, qh);
		ret = f_dnegate(e);

		if (ret != NO_ERROR)
			return ret;

		POP(e, qh);
		POP(e, ql);
	}

	if (negr)
	{
		PUSH(e, rl);
		PUSH(e, rh);
		ret = f_dnegate(e);

		if (ret != NO_ERROR)
			return ret;

		POP(e, rh);
		POP(e, rl);
	}

	/* finally push least-significant words of both values */
	PUSH(e, rl);
	PUSH(e, ql);
	return ret;
}

/* sm/rem (d n -- rem quot) [alias to fm/mod] */
/* symmetric divison apparently means both d and n have the same sign
   - we use fm/mod for this which handles different signs as well
 */

C(f_dump)			/* dump (addr len --) */
{
	Byte *addr;
	Int len;
	
	IFCKSP(e, 2, 0);
	POP(e, len);
	POPT(e, addr, Byte*);
	display_memory(e, addr, len);
	return NO_ERROR;
}

/* blank (addr len --) */

/* erase (addr len --) */

C(f_comment)		/* ( ([text<)>] --) */
{
	Int c;
	
	for (c = read_char(e); c && c != ')'; c = read_char(e))
		;
	
	return NO_ERROR;
}

C(f_linecomment)	/* \\ ([rest-of-line<cr>] --) */
{
	(void)parse_line(e);	/* skip the rest of the line */
	return NO_ERROR;
}

C(f_gtin)           /* >in: T_ADDR to e->in.loc */
{
	Input *i = &e->in;

	IFCKSP(e, 0, 1);

	/* search for the most recent text input stream */
	while (i->type == I_FCODE)
		i = i->link;

	if (i == NULL || i->type == I_NONE)
		return E_INPUT_ERROR;

	PUSHP(e, &i->loc);
	return NO_ERROR;
}

C(f_parse)			/* parse (delim "text<delim>" -- str len) */
{
	Byte *str;
	Int slen;
	Int delim;
	Retcode ret;
	
	IFCKSP(e, 1, 2);
	POP(e, delim);
	ret = parse(e, delim, FALSE, &str, &slen);

	if (ret != NO_ERROR)
		return ret;
	
	if (str && slen)
	{
		PUSHP(e, str);
		PUSH(e, slen);
	}
	else
	{
		PUSHP(e, "");
		PUSH(e, 0);
	}
	
	return NO_ERROR;
}

C(f_parse_word)		/* parse-word ("< >text< >" -- str len) */
{
	Byte *str;
	Int slen;
	Retcode ret;
	
	IFCKSP(e, 0, 2);
	ret = parse_word_len(e, &str, &slen);

	if (ret != NO_ERROR)
		return ret;

	PUSHP(e, str);
	PUSH(e, slen);
	return NO_ERROR;
}

C(f_source)			/* source (-- addr len) */
{
	Input *i = &e->in;

	IFCKSP(e, 0, 2);

	/* search for the most recent text input stream */
	while (i->type == I_FCODE)
		i = i->link;

	if (i == NULL || i->type == I_NONE)
		return E_INPUT_ERROR;

	PUSHP(e, i->buf);
	PUSH(e, i->len);
	return NO_ERROR;
}

C(f_word)			/* word (delim "<delims>text<delim>" -- pstr) */
{
	Byte *str;
	Int slen, delim;
	Retcode ret;
	static Byte buf[STR_SIZE];
	
	IFCKSP(e, 1, 1);
	POP(e, delim);
	ret = parse(e, delim, TRUE, &str, &slen);

	if (ret != NO_ERROR)
		return ret;
	
	if (str && slen)
	{
		memcpy(buf + 1, str, slen);
		*(uByte*)buf = slen;
	}
	else
	{
		buf[0] = 0;
		buf[1] = '\0';
	}
	
	PUSH(e, buf);
	return NO_ERROR;
}

/* accept (addr len1 -- len2) */

static Retcode
do_ascii(Environ *e, Int a)
{
	Retcode ret = NO_ERROR;
	
	if (e->iscompiling)
	{
		compile_fcode(e, 0x10);		/* b(lit) */
		ret = compile_num32(e, a);
	}
	else
		PUSH(e, a);
		
	return ret;
}

C(f_carret)			/* carret (-- 0x0D) */
{
	return do_ascii(e, 0x0D);
}

C(f_linefeed)		/* linefeed (-- 0x0A) */
{
	return do_ascii(e, 0x0A);
}

C(f_ascii)			/* ascii ([text< >] -- char) */
{
	Byte *word = parse_word(e);
	
	IFCKSP(e, 0, 1);
	return do_ascii(e, *word ? word[1] : 0);
}

C(f_char)			/* char ("text< >" -- char) */
{
	if (e->iscompiling)
		return E_COMPILATION_ERROR;

	return f_ascii(e);
}

C(f_bcharb)			/* [char] (C: [text< >] --) (-- char) */
{
	if (!e->iscompiling)
		return E_COMPILATION_ERROR;

	return f_ascii(e);
}

C(f_control)		/* control ([text< >] -- char) */
{
	Byte *word = parse_word(e);
	
	IFCKSP(e, 0, 1);
	/* mask out all but the last 5 bits to make it a control-char */
	return do_ascii(e, (uByte)word[1] & 0x1F);
}

static Retcode
do_quote(Environ *e, Int delim)
{
	Byte *word;
	Int wlen;
	Retcode ret;
	
	ret = parse(e, delim, FALSE, &word, &wlen);

	if (ret != NO_ERROR)
		return ret;
	
	if (e->istokenizing)
	{
		compile_fcode(e, 0x12);			/* b(") */
		ret = compile_str(e, word, wlen);

		if (ret == NO_ERROR)
			ret = compile_fcode(e, 0x90);		/* type */
	}
	else if (e->iscompiling)
	{
		compile_fcode(e, 0x712);		/* b(string) */
		ret = compile_str(e, word, wlen);

		if (ret == NO_ERROR)
			ret = compile_byte(e, '\0');		/* NULL-terminate */

		if (ret == NO_ERROR)
			ret = compile_fcode(e, 0x90);		/* type */
	}
	else
		display_text(e, word, wlen);
	
	return ret;
}

C(f_dotquote)		/* ." ([text<">] --) */
{
	return do_quote(e, '"');
}

C(f_dotparen)		/* .( ([text<)>] --) */
{
	return do_quote(e, ')');
}

/* space (--) */

/* spaces (count --) */

CC(f_exitq)			/* exit? (-- done?) */
{
	IFCKSP(e, 0, 1);

	if (ask_exit(e))
		PUSH(e, FTRUE);
	else
		PUSH(e, FFALSE);

	return NO_ERROR;
}

static Retcode
do_string(Environ *e, Byte *str, Int slen)
{
	Retcode ret = NO_ERROR;

	if (e->istokenizing)
	{
		compile_fcode(e, 0x12);		/* b(") */
		ret = compile_str(e, str, slen);
	}
	else if (e->iscompiling)
	{
		compile_fcode(e, 0x712);	/* b(string) */
		ret = compile_str(e, str, slen);

		if (ret == NO_ERROR)
			ret = compile_byte(e, '\0');		/* NULL-terminate */
	}
	else
	{
		PUSHP(e, str);
		PUSH(e, slen);
	}
	
	return ret;
}

#ifndef NUM_STRING_BUFS
#	define NUM_STRING_BUFS	2	/* OF spec says at least two bufs */
#endif

C(f_dquote)			/* " ([text<">< >] -- text-str text-len) */
{
	static Byte wbuf[NUM_STRING_BUFS][STR_SIZE];
	static unsigned int wnum = 0;
	Byte *wordbuf = wbuf[wnum++ % NUM_STRING_BUFS];
	Byte *w = wordbuf;
	Int c, num;
	
	IFCKSP(e, 0, 2);

	/* parse the string up to the next quote char */
	for (c = read_char(e); c; c = read_char(e))
	{
		if (c == '"')
		{
			/* got a quote - see if it is the end of string or start of
			   a hex sequence */
			c = read_char(e);
			
			if (c == '(')
			{
				/* parse pairs of hex digits, ignoring anything between them */
				for (c = read_char(e); c != ')'; c = read_char(e))
				{
					if (isxdigit(c))
					{
						if (isupper(c))
							c = tolower(c);
					
						num = (c <= '9') ? (c - '0') : (c - 'a' + 10);
						c = read_char(e);
						
						if (!isxdigit(c))
						{
							unparse_char(e);
							continue;
						}
						
						num = (num << 4) |
							(((c <= '9') ? (c - '0') : (c - 'a' + 10)) & 0xF);
						*w++ = num;
					}
				}
				
				continue;
			}
			else if (c != '"')	/* allow "" to mean a single " - not standard */
			{
				/* push it back, if it is not white-space */
				if (!isspace(c))
					unparse_char(e);
					
				break;
			}
		}
		
		*w++ = c;
	}
	
	*w = '\0';
	return do_string(e, wordbuf, strlen(wordbuf));
}

C(f_sdquote)		/* s" ([text<">] -- text-str text-len) */
{
	static Byte wbuf[NUM_STRING_BUFS][STR_SIZE];
	static unsigned int wnum = 0;
	Byte *wordbuf = wbuf[wnum++ % NUM_STRING_BUFS];
	Byte *word;
	Int wlen;
	Retcode ret;
	
	IFCKSP(e, 0, 2);
	ret = parse(e, '"', FALSE, &word, &wlen);

	if (ret != NO_ERROR)
		return ret;

	memcpy(wordbuf, word, wlen);
	wordbuf[wlen] = '\0';
	return do_string(e, wordbuf, wlen);
}

C(f_mtrailing)		/* -trailing (str len1 -- str len2) */
{
	Byte *str;
	Int len;
	
	IFCKSP(e, 2, 2);
	POP(e, len);
	str = (Byte*)(uPtr)TOP(e);
	
	while (len > 0 && isspace(str[len - 1]))
		len--;
	
	PUSH(e, len);
	return NO_ERROR;
}

/* decimal (--) */

/* hex (--) */

/* octal (--) */

C(f_tonumber)		/* >number (d1 str1 len1 -- d2 str2 len2) */
{
	Byte *str;
	Int len;
	Cell num, val, err;
	
	IFCKSP(e, 3, 3);
	POP(e, len);
	POPT(e, str, Byte*);
	POP(e, num);
	parse_number(e->radix, &str, &len, &val, &err, FALSE);
	num += val;
	PUSH(e, num);
	PUSHP(e, str);
	PUSH(e, len);
	return NO_ERROR;
}

static Retcode
do_num(Environ *e, Int radix)
{
	Byte *str;
	Int len;
	Cell val, err;
	Retcode ret;
	
	IFCKSP(e, 0, 1);
	ret = parse_word_len(e, &str, &len);

	if (ret != NO_ERROR)
		return ret;

	parse_number(radix, &str, &len, &val, &err, TRUE);
	
	if (e->iscompiling)
	{
#ifdef SF_64BIT
		ret = compile_num64(e, val);
#else
		compile_fcode(e, 0x10);			/* b(lit) */
		ret = compile_num32(e, val);
#endif
	}
	else
		PUSH(e, val);
	
	return ret;
}

C(f_dnum)			/* d# ([number< >] -- n) */
{
	return do_num(e, 10);
}

C(f_hnum)			/* h# ([number< >] -- n) */
{
	return do_num(e, 16);
}

C(f_onum)			/* o# ([number< >] -- n) */
{
	return do_num(e, 8);
}

/* s. (n --) */

/* .d (n --) */

/* .h (n --) */

/* ? (a-addr --) */

C(f_if)				/* if (do-next? --) */
{
	begin_temp_compile(e);

	/* b?branch - need to back-patch offset */
	return compile_branch(e, e->istokenizing ? 0x14 : 0x714, 1);
}

C(f_else)			/* else (--) */
{
	Byte *b;
	Int code;
	Retcode ret;

	if (!e->iscompiling)
		return E_IF_ELSE_THEN_ERR;

	IFCKSP(e, 1, 1);
	POP(e, code);

	POPT(e, b, Byte*);			/* get pointer to "if" offset */
	compile_fcode(e, 0x13);		/* bbranch */
	PUSHP(e, e->fbrk);	/* now need to back-patch "else" offset at "then" */
	PUSH(e, code);
	ret = compile_offset(e, 0);

	/* back-patch "if" to come here */
	compile_fcode(e, 0xB2);		/* b(>resolve) */
	patch_offset(b, (Byte*)e->fbrk - (Byte*)b);
	return ret;
}

C(f_then)			/* then (--) */
{
	if (!e->iscompiling)
		return E_IF_ELSE_THEN_ERR;
	
	/* back-patch "else" or "then", then finish temp compile */
	return resolve_branch(e);
}

C(f_case)			/* case (sel -- sel) */
{
	return f_bcase(e);
}

C(f_of)				/* of (sel of-val -- sel|<nothing>) */
{
	if (!e->iscompiling)
		return E_COMPILATION_ERROR;
	
	return compile_of(e);
}

C(f_endof)			/* endof (--) */
{
	if (!e->iscompiling)
		return E_COMPILATION_ERROR;
	
	return compile_endof(e);
}

C(f_endcase)		/* endcase (sel|<nothing> --) */
{
	if (!e->iscompiling)
		return E_COMPILATION_ERROR;
	
	return f_bendcase(e);
}

C(f_begin)			/* begin (--) */
{
	return f_bltmark(e);
}

C(f_until)			/* until (done? --) */
{
	/* b?branch back to "begin" & resolve */
	return compile_branch(e, e->istokenizing ? 0x14 : 0x714, -1);
}

C(f_again)			/* again (--) */
{
	/* bbranch back to "begin" & resolve */
	return compile_branch(e, 0x13, -1);
}

C(f_while)			/* while (continue? --) */
{
	/* b?branch forward to "repeat" */
	return compile_branch(e, e->istokenizing ? 0x14 : 0x714, 1);
}

C(f_repeat)			/* repeat (--) */
{
	Byte *w, *b;
	Int code;
	Retcode ret;
	
	IFCKSP(e, 1, 0);
	POP(e, code);
	
	POPT(e, w, Byte*);		/* pointer to previous "while" */
	POP(e, code);
	
	if (code != 0xB1)		/* b(<mark) */
		return E_COMPILATION_ERROR;
		
	POPT(e, b, Byte*);		/* pointer to previous "begin" */
	compile_fcode(e, 0x13);	/* bbranch to "begin" */
	ret = compile_offset(e, (Byte*)b - (Byte*)e->fbrk);	/* -offset */
	
	if (ret != NO_ERROR)
		return ret;
	
	/* patch "while" to come here */
	compile_fcode(e, 0xB2);		/* b(>resolve) */
	patch_offset(w, (Byte*)e->fbrk - (Byte*)w);
	return resolve_temp_compile(e);
}

C(f_do)				/* do (limit start --) */
{
	return compile_do(e, e->istokenizing ? 0x17 : 0x717);		/* b(do) */
}

C(f_qdo)			/* ?do (limit start --) */
{
	return compile_do(e, e->istokenizing ? 0x18 : 0x718);		/* b(?do) */
}

C(f_loop)			/* loop (--) */
{
	return compile_loop(e, 0x15);	/* b(loop) */
}

C(f_ploop)			/* +loop (delta --) */
{
	return compile_loop(e, 0x16);	/* b(+loop) */
}

C(f_leave)			/* leave (--) */
{
	if (e->iscompiling)
		return compile_fcode(e, 0x1B);		/* b(leave) */
	
	return f_bleave(e);
}

/* ?leave (exit? --) */

C(f_quit)			/* quit (--) */
{
	return E_QUIT;
}

C(f_do_abortquote)	/* do abort" (abort? str len -- | nothing) */
{
	Byte *str;
	Int len;
	Int flag;

	IFCKSP(e, 3, 0);
	POP(e, len);
	POPT(e, str, Byte*);
	POP(e, flag);

	if (flag)
	{
		if (str == NULL || len == 0)
			str = "<null-string>";

		display_text(e, str, len);
		display_text(e, "\n", CSTR);
	}

	return flag ? E_ABORTQUOTE : NO_ERROR;
}

C(f_abortquote)		/* abort" (abort? -- | nothing) */
{
	Byte *str;
	Int slen;
	Retcode ret;
	Entry *ent;

	ret = parse(e, '"', FALSE, &str, &slen);

	if (ret != NO_ERROR)
		return ret;

	ret = do_string(e, str, slen);

	if (ret != NO_ERROR)
		return ret;

	if (!e->iscompiling)
		return f_do_abortquote(e);

	ent = find_table(e->names, "do abort\"", CSTR);

	if (ent == NULL)
	{
		cprintf(e, "cannot find do abort\" word?!?\n");
		ret = E_BAD_SYMBOL;
	}

	if (ret == NO_ERROR)
		ret = compile_fcode(e, ent->xtok);

	return ret;
}


static Retcode
set_defname(Environ *e, Int fcode)
{
	Byte *name = parse_word(e);
	
	e->newdef = new_entry();
	e->newdef->name = pstrdup(name);
	set_xtok(e, e->newdef);		/* init the fcode value */
	
	if (e->istokenizing)
	{
		Retcode ret;
		
		e->newdef->type = T_TEMP;
		ret = create_newdef(e);
		
		if (ret != NO_ERROR)
			return ret;
	}
	
	if (e->iscompiling)
	{
		/* named-token, external-token or new-token */
		compile_fcode(e, e->headertok);

		if (e->headertok != 0xB5 /* new-token */)
			compile_str(e, name, PSTR);

		compile_fcode(e, e->newdef->xtok);
		return compile_fcode(e, fcode);
	}

	return NO_ERROR;
}

C(f_constant)		/* constant (x "new-name< >" --) */
{
	if (set_defname(e, 0xBA) != NO_ERROR)		/* b(constant) */
		return E_NEWDEF_ERROR;
	
	return (e->iscompiling) ? NO_ERROR : f_bconstant(e);
}

C(f_2constant)		/* 2constant (x1 x2 "new-name< >" --) */
{
	if (set_defname(e, 0) != NO_ERROR)
		return E_NEWDEF_ERROR;
	
	IFCKSP(e, 2, 0);
	e->newdef->type = T_2CONST;
	POP(e, e->newdef->len);
	POP(e, e->newdef->v.val);
	return create_newdef(e);
}

C(f_value)			/* value (x "new-name< >" --) */
{
	if (set_defname(e, 0xB8) != NO_ERROR)		/* b(value) */
		return E_NEWDEF_ERROR;
	
	return (e->iscompiling) ? NO_ERROR : f_bvalue(e);
}

C(f_variable)		/* variable ("new-name< >" --) */
{
	if (set_defname(e, 0xB9) != NO_ERROR)		/* b(variable) */
		return E_NEWDEF_ERROR;
	
	return (e->iscompiling) ? NO_ERROR : f_bvariable(e);
}

C(f_buffercolon)	/* buffer: (len "new-name< >" --) */
{
	if (set_defname(e, 0xBD) != NO_ERROR)		/* b(buffer:) */
		return E_NEWDEF_ERROR;
	
	return (e->iscompiling) ? NO_ERROR : f_bbuffercolon(e);
}

C(f_colon)			/* : ("new-name< >" -- colon-sys) */
{
	if (set_defname(e, 0xB7) != NO_ERROR)		/* b(:) */
		return E_NEWDEF_ERROR;
	
	e->comp_state = FFALSE;	/* set to FTRUE by f_bcolon */
	e->iscompiling = FALSE;	/* set to TRUE by f_bcolon */
	return f_bcolon(e);
}

C(f_semicolon)		/* ; (colon-sys --) */
{
	if (e->istokenizing)
		return compile_fcode(e, 0xC2);		/* b(;) */
	
	return f_bsemicolon(e);
}

C(f_alias)			/* alias ("new-name< >old-name< >" --) */
{
	if (set_defname(e, 0) != NO_ERROR)
		return E_NEWDEF_ERROR;
	
	e->newdef->type = T_ALIAS;
	e->newdef->v.alias = lookup_sym(e, parse_word(e), PSTR);
	
	if (e->newdef->v.alias == NULL)
		return E_BAD_SYMBOL;
		
	return create_newdef(e);	
}

C(f_defer)			/* defer ("new-name< >" --) */
{
	if (set_defname(e, 0xBC) != NO_ERROR)		/* b(defer) */
		return E_NEWDEF_ERROR;
	
	return f_bdefer(e);
}

C(f_struct)			/* struct (-- 0) */
{
	Retcode ret = NO_ERROR;
	
	if (e->iscompiling)
		ret = compile_fcode(e, 0xA5);		/* compile 0 (zero) */
	else
	{
		IFCKSP(e, 0, 1);
		PUSH(e, 0);
	}
	
	return ret;
}

C(f_field)			/* field (offset size "new-name< >" -- offset+size) */
{
	if (set_defname(e, 0xBE) != NO_ERROR)		/* b(field) */
		return E_NEWDEF_ERROR;
	
	return (e->iscompiling) ? NO_ERROR : f_bfield(e);
}

C(f_create)			/* create ("new-name< >" --) (E:-- a-addr) */
{
	/* this word behaves differently in Forth vs Fcode */
	if (e->iscompiling && !e->istokenizing)
	{
		Entry *ent = find_table(e->names, "create", CSTR);
		
		if (ent == NULL)
			return E_NEWDEF_ERROR;
		
		return compile_fcode(e, ent->xtok);
	}
	
	if (set_defname(e, 0xBB) != NO_ERROR)		/* b(create) */
		return E_NEWDEF_ERROR;
	
	if (e->istokenizing)
		return NO_ERROR;

	return f_bcreate(e);
}

C(f_doesgt)			/* does> [...] */
{
	Byte *mem;

	if (e->newdef->type != T_CREATE || e->in.type != I_FCODE)
		return E_NEWDEF_ERROR;

	/* allocate space for new word */
	mem = falloc(e, e->in.len - (Int)e->in.loc);

	if (mem == NULL)
		return E_OUT_OF_MEMORY;

	/* patch create word with actual start of fcode */
	*(Byte**)e->newdef->v.fcode = mem;

	/* copy everything after ourself into the new word */
	memcpy(mem, e->in.buf + (Int)e->in.loc, e->in.len - (Int)e->in.loc);

	e->newdef->len = e->in.len - (Int)e->in.loc;	/* actual length of fcode */
	e->in.loc = e->in.len;				/* nothing after ourself is executed */
	return NO_ERROR;
}

C(f_dollarcreate)	/* $create (name-str name-len --) */
{
	Byte *str;
	Int len;
	
	IFCKSP(e, 2, 0);
	POP(e, len);
	POPT(e, str, Byte*);
	e->newdef = new_entry();
	e->newdef->name = lstrdup(str, len);	
	set_xtok(e, e->newdef);		/* init the fcode value */
	return f_bcreate(e);
}

C(f_forget)			/* forget ("old-name< >" --) */
{
	return forget_sym(e, parse_word(e), PSTR, TRUE);
}

/* allot (len --) */

CC(f_align)			/* align (--) */
{
	if (((Ptr)e->fbrk % ALIGNMENT) &&
			falloc(e, ALIGNMENT - ((Ptr)e->fbrk % ALIGNMENT)) == NULL)
		return E_OUT_OF_MEMORY;
	
	return NO_ERROR;
}

C(f_immediate)		/* immediate (--) */
{
	e->newdef->flags |= F_IMMEDIATE;
	return NO_ERROR;
}

C(f_lbracket)		/* [ (--) */
{
	if (e->istemp)
	{
		Byte *brk = e->fbrk;
		e->fbrk = e->tempbrk;
		e->tempbrk = brk;
	}
	
	e->comp_state = FFALSE;
	e->iscompiling = FALSE;
	return NO_ERROR;
}

C(f_rbracket)		/* ] (--) */
{
	if (e->istemp)
	{
		Byte *brk = e->fbrk;
		e->fbrk = e->tempbrk;
		e->tempbrk = brk;
	}
	
	e->comp_state = FTRUE;
	e->iscompiling = TRUE;
	return NO_ERROR;
}

C(f_compile)		/* compile (--) */
{
	/* get the following xtok and compile it into the current definition */
	Int fcode = next_fcode_num(e);
	
	return compile_fcode(e, fcode);
}

C(f_bcompileb)		/* [compile] ([old-name< >] --) */
{
	Entry *ent;

	/* parse the next word and compile its xtok */
	ent = lookup_sym(e, parse_word(e), PSTR);
	
	if (ent == NULL)
		return E_BAD_SYMBOL;
	
	return compile_fcode(e, ent->xtok);
}

C(f_literal)		/* literal (C: x1 --) (-- x1) */
{
	Int val;
	
	if (!e->iscompiling)
		return E_COMPILATION_ERROR;
	
	IFCKSP(e, 1, 0);
	POP(e, val);

#ifdef SF_64BIT
	return compile_num64(e, val);
#else
	compile_fcode(e, 0x10);		/* b(lit) */
	return compile_num32(e, val);
#endif
}

C(f_bsquoteb)		/* ['] ([old-name< >] -- xt) */
{
	Entry *ent;
	Retcode ret = NO_ERROR;

	ent = lookup_sym(e, parse_word(e), PSTR);
	
	if (ent == NULL)
		return E_COMPILATION_ERROR;

	if (e->iscompiling)
	{
		compile_fcode(e, 0x11);		/* b(') */
		ret = compile_fcode(e, ent->xtok);
	}
	else
	{
		IFCKSP(e, 0, 1);
		PUSH(e, ent->xtok);
	}
	
	return ret;
}

C(f_squote)			/* ' ("old-name< >" -- xt) */
{
	return f_bsquoteb(e);
}

C(f_find)			/* find (pstr -- xt n | pstr false) */
{
	Entry *ent;
	Byte *str;
	
	IFCKSP(e, 1, 2);
	POPT(e, str, Byte*);
	
	ent = lookup_sym(e, str, PSTR);
	
	if (ent == NULL)
	{
		PUSHP(e, str);
		PUSH(e, FFALSE);
	}
	else
	{
		PUSH(e, ent->xtok);
		
		if (ent->flags & F_IMMEDIATE)
			PUSH(e, +1);
		else
			PUSH(e, -1);
	}
	
	return NO_ERROR;
}

C(f_to)				/* to (param [old-name< >] --) */
{
	Entry *ent;
		
	ent = lookup_sym(e, parse_word(e), PSTR);
	
	if (ent == NULL)
		return E_DEFER_ERROR;
	
	if (e->iscompiling)
	{
		compile_fcode(e, 0xC3);		/* b(to) */
		return compile_fcode(e, ent->xtok);
	}
	
	return do_to(e, ent);
}

C(f_recursive)		/* recursive (--) */
{
	if (!e->iscompiling)
		return E_COMPILATION_ERROR;
		
	return create_newdef(e);
}

C(f_recurse)		/* recurse (... -- ?) */
{
	Retcode ret;
	
	if (!e->iscompiling)
		return E_COMPILATION_ERROR;
		
	if ((ret = create_newdef(e)) != NO_ERROR)
		return ret;
	
	return compile_fcode(e, e->newdef->xtok);
}

C(f_forth)			/* forth (--) */
{
	e->currpkg = NULL;
	return NO_ERROR;
}

C(f_environmentq)	/* environment? (str len -- false | value true) */
{
	Byte *param, *str;
	Int plen, slen;
	Retcode ret;

	IFCKSP(e, 2, 2);
	POP(e, plen);
	POPT(e, param, Byte*);
	ret = get_config_val(e, param, plen, &str, &slen);

	if (ret == NO_ERROR && str && slen)
	{
		PUSHP(e, str);			/* push the C string result */
		PUSH(e, FTRUE);
	}
	else
		PUSH(e, FFALSE);

	return NO_ERROR;
}


const Initentry init_forth[] =
{
	{ "3dup", (Command)"2 pick 2 pick 2 pick",
			INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(x1 x2 x3 -- x1 x2 x3 x1 x2 x3) " \
					"duplicate top three stack items") },
	{ "clear", f_clear, INVALID_FCODE, F_NONE, T_FUNC
			HELP("clear (... --) empty the stack") },
	{ "3drop", (Command)"drop 2drop", INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(x1 x2 x3 --) remove top three items from stack") },
	
	{ "u*", f_utimes, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(u1 u2 -- uprod) multiply u1 by u2, unsigned") },
	{ "*/", f_multdiv, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(n1 n2 n3 -- quot) n1 times n2 divided by n3") },
	{ "*/mod", f_multdivmod, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(n1 n2 n3 -- rem quot) n1 times n2 divided by n3") },
	
	{ "1+", (Command)"1 +", INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(nu1 -- nu2) add one to nu1") },
	{ "1-", (Command)"1 -", INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(nu1 -- nu2) subtract one from nu1") },
	{ "2+", (Command)"2 +", INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(nu1 -- nu2) add two to nu1") },
	{ "2-", (Command)"2 -", INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(nu1 -- nu2) subtract two from nu1") },
	{ "even", f_even, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(n -- n | n+1) round to nearest even integer >=n") },
	
	{ "s>d", f_stod, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(n1 -- d1) convert number to a double number") },
	{ "m*", f_mtimes, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(n1 n2 -- d) signed multiply with double number product") },
	{ "sm/rem", f_fmdivmod, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(d n -- rem quot) divide d by n, symmetric division") },
			/* alias */
	{ "fm/mod", f_fmdivmod, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(d n -- rem quot) divide d by n") },
	
	{ "dump", f_dump, INVALID_FCODE, F_PAGINATE, T_FUNC
			HELP("(addr len --) " \
					"display len bytes of memory starting at addr") },
	{ "blank", (Command)"bl fill", INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(addr len --) set len bytes at addr to 0x20") },
	{ "erase", (Command)"0 fill", INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(addr len --) set len bytes at addr to zero") },

	{ "(", f_comment, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("([text<)>] --) comment - ignore intervening text") },
	{ "\\", f_linecomment, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("([rest-of-line<cr>] --) ignore text to end of line") },
	{ ">in", f_gtin, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- a-addr) variable containing " \
					"offset of next input buffer character") },
	{ "parse", f_parse, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(delim \"text<delim>\" -- str len)\n" \
					"\tparse text from input buffer, delimited by delim") },
	{ "parse-word", f_parse_word, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"< >text< >\" -- str len)\n" \
				"\tparse text from input buffer, delimited by white-space") },
	{ "source", f_source, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- addr len) return location and size of input buffer") },
	{ "word", f_word, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(delim \"<delims>text<delim>\" -- pstr)\n" \
					"\tparse text from the input buffer, delimted by delim") },
	
	{ "accept", (Command)"span @ -rot expect span @ swap span !",
			INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(addr len1 -- len2) get an edited input line at addr") },
	
	{ "carret", f_carret, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(-- 0x0D) ASCII code for carriage-return character") },
	{ "linefeed", f_linefeed, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(-- 0x0A) ASCII code for linefeed character") },
	{ "ascii", f_ascii, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("([text< >] -- char)\n" \
					"\tget ASCII code for immediately following character") },
	{ "char", f_char, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(\"text< >\" -- char)\n" \
					"\tget ASCII code for next character from input buffer") },
	{ "[char]", f_bcharb, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(C: [text< >] --) (-- char)\n" \
					"\tget ASCII code for next character from input buffer") },
	{ "control", f_control, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("([text< >] -- char)\n" \
					"\tget control code for immediately following character") },
	
	{ ".\"", f_dotquote, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("([text<\">] --) display immediately following text") },
	{ ".(", f_dotparen, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("([text<)>] --) display following text up to delimiting )") },

	{ "space", (Command)"bl emit", INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(--) display a singple space") },
	{ "spaces", (Command)"0 max 0 ?do bl emit loop",
			INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(cnt --) display cnt spaces") },
	
	{ "exit?", f_exitq, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- done?) return true when output should be terminated") },
	
	{ "\"", f_dquote, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("([text<\">< >] -- text-str text-len)\n" \
					"\tgather the immediately following string or hex data") },
	{ "s\"", f_sdquote, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("([text<\">] -- text-str text-len)\n" \
					"\tgather the immediately following string") },
	
	{ "-trailing", f_mtrailing, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(str len1 -- str len2) remove trailing spaces from string") },
	
	{ "decimal", (Command)"d# 10 base !", INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(--) set numeric conversion radix to ten") },
	{ "hex", (Command)"d# 16 base !", INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(--) set numeric conversion radix to sixteen") },
	{ "octal", (Command)"d# 8 base !", INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(--) set numeric conversion radix to eight") },
	
	{ ">number", f_tonumber, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(d1 str1 len1 -- d2 str2 len2)\n" \
					"\tconvert string to a number, add to d1") },
	{ "d#", f_dnum, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("([number< >] -- n)\n" \
					"\tinterpret following number as decimal (base ten)") },
	{ "h#", f_hnum, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("([number< >] -- n)\n" \
				"\tinterpret following number as hexadecimal (base sixteen)") },
	{ "o#", f_onum, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("([number< >] -- n)\n" \
					"\tinterpret following number as octal (base eight)") },
	
	{ "s.", (Command)"(.) type space", INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(n --) display signed number with trailing space") },
	{ ".d", (Command)"base @ swap d# 10 base ! . base !",
			INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(n --) display signed number in decimal (plus a space)") },
	{ ".h",
			#ifdef SUN_COMPATIBILITY
				(Command)"base @ swap d# 16 base ! u. base !",
			#else
				(Command)"base @ swap d# 16 base ! . base !",
			#endif
			INVALID_FCODE, F_NONE, T_FORTH
			HELP("(n --) display signed number in hex (plus a space)") },
	{ "?", (Command)"@ .", INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(a-addr --) display number at a-addr") },
	
	{ "(.)", (Command)"dup abs <# u#s swap sign u#>",
			INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(n -- str len) convert number to a text string") },
	{ "(u.)", (Command)"<# u#s u#>", INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(u -- str len) convert unsigned number to a text string") },

	{ "if", f_if, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(do-next? --) when flag is true, execute following code") },
	{ "else", f_else, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) when if flag is false, execute following code") },
	{ "then", f_then, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) terminate an if statement") },
	{ "endif", f_then, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) synonym for then to terminate if") },	/* synonym */

	{ "case", f_case, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(sel -- sel) begin a case statement") },
	{ "of", f_of, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(sel of-val -- sel|<nothing>)\n" \
				"\tbegin of clause; execute through endof if params match") },
	{ "endof", f_endof, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) mark end of cluase; jump to end of case if match") },
	{ "endcase", f_endcase, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(sel|<nothing> --) mark end of a case statement") },
	
	{ "begin", f_begin, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) begin a conditional loop") },
	{ "until", f_until, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(done? --) end a begin...until loop; exit if flag is true") },
	{ "again", f_again, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) end an (infinite) begin...again loop") },
	{ "while", f_while, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(continue? --) test within begin...while...repeat loop") },
	{ "repeat", f_repeat, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) end begin...while...repeat loop; jump to begin") },
	
	{ "do", f_do, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(limit start --) begin counted loop with index at start") },
	{ "?do", f_qdo, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(limit start --)" \
					"as \"do\" but do not execute if limit==start") },
	{ "loop", f_loop, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) add one to index; return to previous do or exit") },
	{ "+loop", f_ploop, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(delta --) add delta to index and continue loop or exit") },
	{ "leave", f_leave, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) exit this do or ?do loop") },
	{ "?leave", (Command)"if leave then",
			INVALID_FCODE, F_TOKENIZE | F_IMMEDIATE, T_FORTH
			HELP("(exit? --) exit this do or ?do loop if flag is true") },
	
	{ "quit", f_quit, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) abort program execution") },
	{ "do abort\"", f_do_abortquote, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(abort? str len --| nothing) " \
					"if flag is true, display string and execute -2 throw") },
	{ "abort\"", f_abortquote, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(abort? --| nothing) " \
					"if flag is true, display text and execute -2 throw") },
	
	{ "constant", f_constant, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(x \"new-name< >\" --) create named constant of value x") },
	{ "2constant", f_2constant, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(x1 x2 \"new-name< >\" --) " \
					"create named two-number constant") },
	{ "value", f_value, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(x \"new-name< >\" --)\n" \
					"\tcreate a named variable; change with to") },
	{ "variable", f_variable, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(\"new-name< >\" --)\n" \
					"\tcreate a named variable; new-name returns an address") },
	{ "buffer:", f_buffercolon, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(len \"new-name< >\" --)\n" \
					"\tcreate a named buffer; new-name returns address") },
	{ ":", f_colon, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(\"new-name< >\" -- colon-sys)\n" \
					"\tbegin creation of a colon definition") },
	{ ";", f_semicolon, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(colon-sys --) end creation of colon definition") },
	{ "alias", f_alias, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"new-name< >old-name< >\" --)\n" \
				"\tcreate a new command equivalent of an existing command") },
	{ "defer", f_defer, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(\"new-name< >\" --)\n" \
					"\tcreate a command with behavior alterable by to") },
	{ "struct", f_struct, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(-- 0) start a struct...field definition") },
	{ "field", f_field, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(offset size \"new-name< >\" -- offset+size)\n" \
					"\tcreate new field offset specifier named new-name") },
	{ "create", f_create, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(\"new-name< >\" --) (E:-- a-addr)\n" \
				"\tcreate a new command; behavior set by further commands") },
	{ "does>", f_doesgt, INVALID_FCODE, F_NONE, T_FUNC
			HELP("[...] specify run-time behavior of a create word") },
	{ "$create", f_dollarcreate, INVALID_FCODE, F_NONE, T_FUNC
		HELP("(name-str name-len --) create new word with specified name") },
	{ "forget", f_forget, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"old-name< >\" --) remove command old-name") },
	
	{ "allot", (Command)"0 max 0 ?do 0 c, loop",
			INVALID_FCODE, F_TOKENIZE, T_FORTH
			HELP("(len --) allocate len bytes in the dictionary") },
	{ "align", f_align, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) allocate bytes to var-align top of dictionary") },
	{ "immediate", f_immediate, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) declare previous definition as immediate") },
	{ "[", f_lbracket, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) enter interpret state") },
	{ "]", f_rbracket, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) enter compile state") },
	{ "compile", f_compile, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) compile following command at runtime") },
	{ "postpone", f_bcompileb, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(... -- \?\?\?) delay execution of " \
					"immediately following command") },	/* synonym? */
	{ "[compile]", f_bcompileb, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("([old-name< >] --) compile immediately following command") },
	{ "literal", f_literal, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(C: x1 --) (-- x1)\n"
					"\tcompile a number, later leave it on the stack") },
	
	{ "[']", f_bsquoteb, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("([old-name< >] -- xt)\n" \
					"\treturn execution token xt of a command") },
	{ "'", f_squote, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(\"old-name< >\" -- xt)\n" \
					"\treturn execution token xt of a command parsed later") },
	{ "find", f_find, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(pstr -- xt n | pstr false)\n" \
	"\tfind command, return -1 (found), +1 (immediate), or 0 (not found)") },
	{ "is", f_to, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(param [old-name< >] --)\n" \
					"\tsynonym for to") },					/* synonym */
	{ "to", f_to, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(param [old-name< >] --)\n" \
					"\tchange value or defer word contents") },
		
	{ "recursive", f_recursive, INVALID_FCODE, F_IMMEDIATE, T_FUNC
			HELP("(--) make current definition visible for recursive call") },
	{ "recurse", f_recurse, INVALID_FCODE, F_IMMEDIATE, T_FUNC
		HELP("(... -- ?) compile recursive call to command being compiled") },
	{ "forth", f_forth, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) make Forth the context vocabulary") },
	{ "environment?", f_environmentq, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(str len -- false | value true)\n" \
					"\treturn system information based on input keyword") },

	{ NULL, NULL }
};
