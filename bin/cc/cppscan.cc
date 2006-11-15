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
// $Header: /u/cgt/cvs/bin/cc/cppscan.cc,v 1.57 1999/03/15 21:48:57 parag Exp $
#include "cpp.h"
#include "fileuse.h"
#include <limits.h>
#include <ctype.h>
#include <pool.h>
#include <stdiox.h>

#define C_NULL 1
#define C_QUESTION 2
#define C_BACKSLASH 3
#define C_CR 4
#define C_NL 5
#define C_DQUOTE 6
#define C_SQUOTE 7
#define C_SLASH 8

// get rid of trigraphs, backslash-newlines, carriage-return-newline, and
// comments from the current buffer - backslash-newlines are moved to just
// before the following regular newlines so the line count comes out right
void
Cpp::cleanupbuf()
{
	static unsigned char lookup[UCHAR_MAX + 1];		// inits everything to zero

	// this is so the following "switch" has a better chance to run fast
	// - sequential switch elements should let the compiler optimize better
	if (!lookup['\0'])
	{
		lookup['\0'] = C_NULL;
		lookup['?'] = C_QUESTION;
		lookup['\\'] = C_BACKSLASH;
		lookup[CR] = C_CR;
		lookup[NL] = C_NL;
		lookup['"'] = C_DQUOTE;
		lookup['\''] = C_SQUOTE;
		lookup['/'] = C_SLASH;
	}

	char *from = ptr;
	char *to = from;		// to <= from as buffer is overwritten
	int bsnl = 0;		// back-slash-new-line count
	int instr = 0;		// currently within a string - don't strip comments
	int linecount = 0;

	for (;;)
	{
		char *copy = from;

		while (lookup[(unsigned char)*from] == 0)
			from++;

		int len = from - copy;

		if (len)
		{
			if (to != copy)
				memcpy(to, copy, len);

			to += len;
		}

		switch (lookup[(unsigned char)*from])
		{
		case C_NULL:
			*to = '\0';

			if (g_verbose)
				new_file_use(files[currfile].fname,
						currfile > 0 ? files[currfile - 1].fname : (const char *)NULL,
						linecount, from - ptr);

			return;

		case C_QUESTION:
			if (from[1] == '?')				// possible trigraph
			{
				int c;

				switch (from[2])
				{
				case '=': c = '#'; break;
				case '(': c = '['; break;
				case '/': c = '\\'; break;
				case ')': c = ']'; break;
				case '\'': c = '^'; break;
				case '<': c = '{'; break;
				case '!': c = '|'; break;
				case '>': c = '}'; break;
				case '-': c = '~'; break;
				default:
					*to++ = *from++;		// nope - keep looping
					continue;
				}

				from += 2;
				*from = c;		// check real character in the next iteration
			}
			else
				*to++ = *from++;

			break;
		
		case C_BACKSLASH:		// possible newline-continuation
			if (from[1] == NL)
			{
				from += 2;
				bsnl++;
			}
			else if (from[1] == CR && from[2] == NL)
			{
				from += 3;
				bsnl++;
			}
			else		// escape-sequence - copy the whole thing
			{
				*to++ = *from++;
				*to++ = *from++;
			}

			break;

		case C_CR:		// skip carriage-returns
			from++;
			break;

		case C_NL:		// insert newlines deleted earlier so line count is OK
			linecount += bsnl + 1;

			for (; bsnl; bsnl--)
				*to++ = NL;

			*to++ = *from++;
			break;

		case C_DQUOTE:
			if (!instr)
				instr = '"';
			else if (instr == '"')
				instr = 0;

			*to++ = *from++;
			break;

		case C_SQUOTE:
			if (!instr)
				instr = '\'';
			else if (instr == '\'')
				instr = 0;

			*to++ = *from++;
			break;

		case C_SLASH:		// possible comment, but not in a string
			if (instr)
				*to++ = *from++;
			// C-style comment, dealing with backslash-nl and trigraph
			else if (from[1] == '*' ||
					(from[1] == '\\' && from[2] == CR && from[3] == NL &&
						from[4] == '*') ||
					(from[1] == '?' && from[2] == '?' && from[3] == '/' &&
						from[4] == CR && from[5] == NL && from[6] == '*') ||
					(from[1] == '\\' && from[2] == NL && from[3] == '*') ||
					(from[1] == '?' && from[2] == '?' && from[3] == '/' &&
							from[4] == NL && from[5] == '*'))
			{
				if (from[1] == '*')
					from += 2;
				else if (from[2] == CR || from[4] == CR)
				{
					from += (from[1] == '?') ? 7 : 5;
					bsnl++;
				}
				else
				{
					from += (from[1] == '?') ? 6 : 4;
					bsnl++;
				}

				for (; *from != '\0'; from++)
					if (*from == '*' && (from[1] == '/' ||
						(from[1] == '\\' && from[2] == CR &&
								from[3] == NL && from[3] == '/') ||
						(from[1] == '?' && from[2] == '?' && from[3] == '/' &&
								from[4] == CR && from[5] == NL &&
								from[6] == '/') ||
						(from[1] == '\\' && from[2] == NL &&
								from[3] == '/') ||
						(from[1] == '?' && from[2] == '?' && from[3] == '/' &&
								from[4] == NL && from[5] == '/')))
						break;
					else if (*from == NL)
						bsnl++;

				if (*from == '\0')
					warn(UNTERMINATED_C_COMMENT);
				else if (from[1] == '/')
					from += 2;
				else if (from[2] == CR || from[4] == CR)
				{
					from += (from[1] == '?') ? 7 : 5;
					bsnl++;
				}
				else
				{
					from += (from[1] == '?') ? 6 : 4;
					bsnl++;
				}

				*to++ = ' ';		// convert comment to one space
			}
			// BCPL comment
			else if (from[1] == '/' ||
				(from[1] == '\\' && from[2] == CR &&
					from[3] == NL && from[4] == '/') ||
				(from[1] == '?' && from[2] == '?' && from[3] == '/' &&
						from[4] == CR && from[5] == NL &&
						from[6] == '/') ||
				(from[1] == '\\' && from[2] == NL && from[3] == '/') ||
				(from[1] == '?' && from[2] == '?' && from[3] == '/' &&
						from[4] == NL && from[5] == '/'))
			{
				if (g_strict_iso && !g_cplusplus)
					warn(NON_ISO_BCPL_COMMENT);

				if (from[1] == '/')
					from += 2;
				else if (from[2] == CR || from[4] == CR)
				{
					from += (from[1] == '?') ? 7 : 5;
					bsnl++;
				}
				else
				{
					from += (from[1] == '?') ? 6 : 4;
					bsnl++;
				}

				// why anyone would want to continue one of these comments
				// over multiple lines is beyond me, but do it anyway
				while (*from != '\0' && *from != NL)
					if (from[0] == '\\' && from[1] == NL)
					{
						from += 2;
						bsnl++;
					}
					else if (from[0] == '?' && from[1] == '?' &&
							from[2] == '/' && from[3] == NL)
					{
						from += 4;
						bsnl++;
					}
					else if (from[0] == '\\' && from[1] == CR &&
							from[2] == NL)
					{
						from += 3;
						bsnl++;
					}
					else if (from[0] == '?' && from[1] == '?' &&
							from[2] == '/' && from[3] == CR &&
							from[4] == NL)
					{
						from += 5;
						bsnl++;
					}
					else
						from++;

				if (*from == '\0')
					warn(UNTERMINATED_BCPL_COMMENT);

				*to++ = ' ';		// convert comment to one space
			}
			else
				*to++ = *from++;

			break;
		}
	}
}

int
Cpp::scan(Cpp_node_list *buf, boolean retwhsp)
{
	int c, c2, ret;
	char *p = ptr;
	char *s;
	const char *ss;

	for (;;)
	{
		// scan tokens from buffers unless a file is pushed on top of stack
		while (top >= 0 && stack.elt(top).file == currfile)
		{
			node.token = WHITESPACE;
			Cpp_stack_elt &elt = stack.elt(top);

			if (elt.loc < elt.buf.size())
				node = elt.buf.elt(elt.loc++);
			else
			{
				top--;
				continue;
			}

			if (in_cpp)
			{
				if (!retwhsp && node.token == WHITESPACE)
					continue;
			}
			else
			{
				if (node.token == NEWLINE)
				{
					if (buf != NULL && buf->size() > 0)
					{
						ptr = p;
						return node.token;
					}

					continue;
				}

				if (node.token == WHITESPACE || node.token == END)
					continue;

				if (node.token == IDENTIFIER && macros.find(node.text))
				{
					// we have a macro we may need to expand
					Cpp_node n = node;
					Cpp_node_list exp(16);		// expansion buffer
					Cpp_macro &macro = macros();
					in_cpp = TRUE;
					ptr = p;
					boolean e = expand_macro(macro, exp);
					in_cpp = FALSE;
					p = ptr;

					if (e)
					{
						// expanded - push it on the top and continue
						top++;
						stack[top].buf = exp;
						stack[top].macro = &macro;
						stack[top].loc = 0;
						stack[top].file = currfile;
						continue;
					}
					else		// could not expand it, so just return it
						node = n;
				}
				else
				{
					if (node.token == EXPANDED_IDENT)
						node.token = IDENTIFIER;
				}
			}

			if (buf == NULL || node.token == END)
			{
				ptr = p;
				return node.token;
			}

			if (node.token == NEWLINE)
			{
				if (buf->size() == 0 && !in_cpp)
					continue;

				ptr = p;
				return node.token;
			}

			buf->end() = node;
		}

		// scan tokens from file - uses goto to avoid function-call overhead

		// look for newlines here to check for following macro directives
		while (!in_cpp && node.token == NEWLINE)
		{
			line++;
			s = p;

			for (c = *p++; iswhsp(c); c = *p++)
				;

			// if this is not a preprocessor directive, treat it as whsp
			if (c != '#')
				goto whitespace;

			// otherwise, get the directive's name
			for (c = *p++; iswhsp(c); c = *p++)
				;

			s = p - 1;

			for (; isalpha(c); c = *p++)
				;

			c = *--p;
			*p = '\0';
			ss = strget(s);		// put the directive name in the pool for now
			*p++ = c;

			for (; iswhsp(c); c = *p++)
				;

			ptr = --p;
			in_cpp = TRUE;

			switch (*ss)
			{
			case 'd':
				if (streq(ss, "define"))
					define_directive();
				else
					goto no_match;

				break;

			case 'e':
				c2 = ss[2];

				if (c2 == 'd' && streq(ss, "endif"))
					endif_directive();
				else if (c2 == 's' && streq(ss, "else"))
					else_directive();
				else if (c2 == 'i' && streq(ss, "elif"))
					elif_directive();
				else if (streq(ss, "error"))
					error_directive();
				else
					goto no_match;

				break;

			case 'i':
				c2 = ss[2];

				if (c2 == 'c' && streq(ss, "include"))
					include_directive();
				else if (c2 == 'd' && streq(ss, "ifdef"))
					ifdef_directive(TRUE);
				else if (c2 == 'n' && streq(ss, "ifndef"))
					ifdef_directive(FALSE);
				else if (streq(ss, "if"))
					if_directive();
				else
					goto no_match;

				break;

			case 'l':
				if (streq(ss, "line"))
					line_directive();
				else
					goto no_match;

				break;

			case 'p':
				if (streq(ss, "pragma"))
					pragma_directive();
				else
					goto no_match;

				break;

			case 'u':
				if (streq(ss, "undef"))
					undef_directive();
				else
					goto no_match;

				break;

			default:
			no_match:
				if (isdigit(*s))		// perhaps an old-style line directive
				{
					if (g_strict_iso)
						warn(OLD_STYLE_LINE_DIRECTIVE);

					ptr = p = s;		// reset to start of line number
					line_directive();
				}
				else if (*ss)
				{
					error(ILLEGAL_DIRECTIVE, ss);

					// skip to the next line
					while (c != NL && c != '\0')
						c = *p++;

					node.token = (*p == NL) ? NEWLINE : END;
				}

				break;
			}

			in_cpp = FALSE;
			p = ptr;
		}

	doswitch:
		switch (c = *p++)		// not a newline - scan the next token
		{
		// end of this file
		case '\0':
			// if no more files, then we are finished
			if (currfile < 0)
			{
				node.token = END;
				goto addbuf;
			}

			c = files[currfile].ifcount;

			// check counts of preprocessor #if* directives
			if (c == 1)
				error(MISSING_ENDIF);
			else if (c > 1)
				error(MISSING_ENDIFS, c);

			// if no more files, then we are finished
			if (currfile <= 0)
			{
				node.token = END;
				goto addbuf;
			}
			else
				currfile--;

			// go to the previous file/buffer and continue scanning
			line = files[currfile].line;
			ptr = p = files[currfile].ptr;

			if (in_cpp)
			{
				node.token = END;
				goto addbuf;
			}

			// not in_cpp, so look for another token
			continue;

		// whitespace is normally skipped, unless we are expanding macros
		case ' ': case '\t': case '\v': case '\f':
			while (iswhsp(c))
				c = *p++;

		whitespace:
			p--;
			node.token = WHITESPACE;

			if (in_cpp && retwhsp)
				goto addbuf;

			goto doswitch;

		// newlines are normally skipped, unless we are expanding macros
		case NL:
			node.token = NEWLINE;

			if (in_cpp || buf != NULL)
				goto addbuf;

			continue;

		// # ## - only used within pre-processor, otherwise an error
		case '#':
			ret = c;

			if (*p++ == '#')
				ret = HASHHASH;
			else
				p--;

			if (in_cpp)
				node.token = ret;
			else
				error(ILLEGAL_HASH_TOKENS);

			break;

		// numeric token - this is really ugly
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			s = p - 1;

		decimal:
			{
				ret = INTVAL;		// assume integer unless proven otherwise
				boolean hex = FALSE;		// these are to print error messages
				boolean dot = FALSE;
				boolean exp = FALSE;

				// scan the next thing that looks kinda-sorta like a number
				for (; isdigit(c) || isalpha(c) || c == '_' || c == '.'; c = *p++)
				{
					if (c == 'e' || c == 'E')
					{
						if (!hex)		// exponent implies float unless hex
							ret = DOUBLEVAL;

						c = *p;

						if (c == '-' || c == '+')
						{
							if (hex)
								error(BADLY_FORMED_NUMBER, c);

							p++;
						}

						if (exp && !hex)
							error(BADLY_FORMED_NUMBER, c);
						else
							exp = TRUE;
					}
					else if (c == '.')
					{
						ret = DOUBLEVAL;		// decimal point implies float

						if (hex || dot || exp)
							error(BADLY_FORMED_NUMBER, c);
						else
							dot = TRUE;
					}
					else if (c == 'x' || c == 'X')
					{
						if (hex || dot || exp)
							error(BADLY_FORMED_NUMBER, c);
						else
							hex = TRUE;
					}
				}

				c = *--p;
				*p = '\0';
				ss = node.text = strget(s);
				*p = c;
				ss += strlen(ss) - 1;
				c = tolower(*ss);

				// check for type suffix at the end of this integer value
				if (ret == INTVAL)
				{
					// we ignore case here
					c2 = tolower(ss[-1]);
					int c3 = tolower(ss[-2]);

					if (c2 == 'l' && (c == 'l' || c3 == 'l')) // 123*LL 123LL*
					{
						if (g_strict_iso)
							warn(LONGLONG_IS_NOT_ISO);

						if (c3 == 'u' || c == 'u')		// 123ULL 123LLU
							ret = ULONGLONGVAL;
						else						// 123LL
							ret = LONGLONGVAL;
					}
					else if (c == 'l' || c2 == 'l')		// 123*L 123L*
					{
						if (c == 'u' || c2 == 'u')		// 123UL 123LU
							ret = ULONGVAL;
						else						// 123L
							ret = LONGVAL;
					}
					else if (c == 'u' || c2 == 'u')		// 123U
						ret = UINTVAL;
					else if (!isxdigit(c))
						error(BADLY_FORMED_NUMBER, c);
				}
				else
				{
					if (c == 'f')						// 1.23F
						ret = FLOATVAL;
					else if (c == 'l')				// 1.23L
						ret = LONGDOUBLEVAL;
					else if (c == 's')				// 1.23S
					{
						if (g_strict_iso)
							warn(SHORTDOUBLE_IS_NOT_ISO);

						ret = SHORTDOUBLEVAL;
					}
					else if (!isdigit(c) && c != '.')
						error(BADLY_FORMED_NUMBER, c);
				}

				node.token = ret;
			}

			break;

		// . ... .[0-9]
		case '.':
			s = p - 1;
			ret = '.';
			c = *p++;

			// oops - really a floating-point number
			if (isdigit(c))
			{
				c = '.';
				goto decimal;
			}

			// ... for param args
			if (c == '.')
			{
				ret = DOTDOTDOT;

				if (*p++ != '.')
				{
					error(ILLEGAL_DOTDOT_TOKEN);
					p--;
				}
			}
			else
				p--;

			node.token = ret;
			break;

		// / /=
		case '/':
			ret = c;
			c = *p++;

			if (c == '=')
				ret = DIVEQ;
			else
				p--;

			node.token = ret;
			break;

		// * *=
		case '*':
			ret = c;
			c = *p++;

			if (c == '=')
				ret = MULTEQ;
			else
				p--;

			node.token = ret;
			break;

		// % %=
		case '%':
			ret = c;
			c = *p++;

			if (c == '=')
				ret = MODEQ;
			else
				p--;

			node.token = ret;
			break;

		// = ==
		case '=':
			ret = c;
			c = *p++;

			if (c == '=')
				ret = EQUAL;
			else
				p--;

			node.token = ret;
			break;

		// ^ ^=
		case '^':
			ret = c;
			c = *p++;

			if (c == '=')
				ret = XOREQ;
			else
				p--;

			node.token = ret;
			break;

		// ! !=
		case '!':
			ret = c;
			c = *p++;

			if (c == '=')
				ret = NOTEQ;
			else
				p--;

			node.token = ret;
			break;

		// + += ++
		case '+':
			ret = c;
			c = *p++;

			if (c == '=')
				ret = PLUSEQ;
			else if (c == '+')
				ret = INCR;
			else
				p--;

			node.token = ret;
			break;

		// & &= &&
		case '&':
			ret = c;
			c = *p++;

			if (c == '=')
				ret = ANDEQ;
			else if (c == '&')
				ret = ANDAND;
			else
				p--;

			node.token = ret;
			break;

		// | |= ||
		case '|':
			ret = c;
			c = *p++;

			if (c == '=')
				ret = OREQ;
			else if (c == '|')
				ret = OROR;
			else
				p--;

			node.token = ret;
			break;

		// - -= -- ->
		case '-':
			ret = c;
			c = *p++;

			if (c == '=')
				ret = MINUSEQ;
			else if (c == '-')
				ret = DECR;
			else if (c == '>')
				ret = ARROW;
			else
				p--;

			node.token = ret;
			break;

		// < <= << <<=
		case '<':
			ret = c;
			c = *p++;

			if (c == '<')
			{
				ret = LSHIFT;
				c = *p++;

				if (c == '=')
					ret = LSHIFTEQ;
				else
					p--;
			}
			else if (c == '=')
				ret = LESSEQ;
			else
				p--;

			node.token = ret;
			break;

		// > >= >> >>=
		case '>':
			ret = c;
			c = *p++;

			if (c == '>')
			{
				ret = RSHIFT;
				c = *p++;

				if (c == '=')
					ret = RSHIFTEQ;
				else
					p--;
			}
			else if (c == '=')
				ret = GREATEQ;
			else
				p--;

			node.token = ret;
			break;

		// character constant
		case '\'':
			ret = CHARACTER;
			goto scanstr;

		// string constant
		case '"':
			ret = STRING;

		scanstr:
			s = p - 1;
			c2 = c;

			// handle embedded back-slashes - do NOT wraparound lines
			for (c = *p++; c != '\0' && c != NL && c != c2; c = *p++)
				if (c == '\\')
					c = *p++;

			if (c != c2)
			{
				error(UNTERMINATED_STRING_OR_CHAR_CONST);
				line++;
			}

			c = *p;
			*p = '\0';
			node.text = strget(s); // pool it
			*p = c;
			node.token = ret;
			break;

		// L"str"  L'c'  L[ident]
		case 'L':
			s = p - 1;
			c = *p++;

			if (c == '\'')
			{
				ret = WCHARACTER;
				goto scanstr;
			}
			else if (c == '"')
			{
				ret = WSTRING;
				goto scanstr;
			}
			else
			{
				c = 'L';
				p--;
			}
			//fall through to identifier below

		// identifier - note that 'L' is handled above
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		case 'G': case 'H': case 'I': case 'J': case 'K': //case 'L':
		case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
		case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
		case 'Y': case 'Z':
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
		case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
		case 's': case 't': case 'u': case 'v': case 'w': case 'x':
		case 'y': case 'z':
		case '_': case '$':
			{
				boolean had_dollar = FALSE;
				s = p - 1;

				for (; isidchar2(c); c = *p++)
					if (c == '$')
						had_dollar = TRUE;

				if (g_strict_iso && had_dollar)
					warn(ILLEGAL_DOLLAR_CHAR_IN_IDENT);

				c = *--p;
				*p = '\0';
				node.text = strget(s); // pool it
				*p = c;

				// if this identifier is a macro, expand it
				if (!in_cpp && macros.find(node.text))
				{
					Cpp_node n = node;
					Cpp_node_list exp(16);		// expansion buffer
					Cpp_macro &macro = macros();

					in_cpp = TRUE;
					ptr = p;
					boolean e = expand_macro(macro, exp);
					in_cpp = FALSE;
					p = ptr;

					if (e)
					{
						// expanded - push it on the top of our stack and continue
						top++;
						stack[top].buf = exp;
						stack[top].macro = &macro;
						stack[top].loc = 0;
						stack[top].file = currfile;
						continue;
					}
					else		// could not expand it, so just return it
					{
						node = n;
						node.token = IDENTIFIER;
					}
				}
				else
					node.token = IDENTIFIER;
			}

			break;

		// : ::
		case ':':
			if (!g_cplusplus)
			{
				node.token = c;
				break;
			}

			ret = c;
			c = *p++;

			if (c == ':')
				ret = COLONCOLON;
			else
				p--;

			node.token = ret;
			break;

		// all other characters return as themselves, so macro expansion
		// works - the parser must deal with illegal tokens

		//case '[': case ']': case '{': case '}': case '(': case ')':
		//case '?': case ',': case ';': case '~': //case ':':
		//case 1: case 2: case 3: case 4: case 5: case 6: case 7:
		//case '\b': case 016: case 017:
		//case 020: case 021: case 022: case 023: case 024: case 025:
		//case 026: case 027:
		//case 030: case 031: case 032: case 033: case 034: case 035:
		//case 036: case 037:
		default:
			// scan for various single-char operators, and anything else
			node.token = c;
			break;
		}

	addbuf:
		if (buf == NULL || node.token == END)
		{
			ptr = p;
			return node.token;
		}

		if (node.token == NEWLINE)
		{
			if (buf->size() == 0 && !in_cpp)
				continue;

			ptr = p;
			return node.token;
		}

		buf->end() = node;

		// we can skip pushback/newline checks since the explicit
		// "continue"s above force control around the for (;;) loop
		// where necessary
		goto doswitch;
	}

	bug("Cpp::scan(): should never ever get here - ever");
	return -1;
}
