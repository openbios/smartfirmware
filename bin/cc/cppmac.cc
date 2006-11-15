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

// Copyright (c) 1992-1995 by Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/bin/cc/cppmac.cc,v 1.36 1999/09/26 18:51:23 parag Exp $
#include "cpp.h"
#include "fileuse.h"
#include <pool.h>
#include <stdiox.h>

void 
Cpp_macro::define_macro(Cpp_node_list *list, Cpp_macro_type mtype)
{
	if (mtype >= 0 && list)
		body = list;

	type = mtype;
	file = NULL;
	line = 0;
}

boolean
Cpp::define_macro(const char *macro, const char *def, Cpp_macro_type mtype)
{
	macro = strget(macro);

	if (macros.find(macro))		// macro already defined
		return FALSE;

	Cpp_node_list *body = NULL;

	if (def)
	{
		// push a fake file on the file stack to scan the definition
		files[currfile].line = line;
		files[currfile].ptr = ptr;
		files[++currfile].rescan(def);
		ptr = files[currfile].ptr;

		body = new Cpp_node_list;
		in_cpp = TRUE;

		// scan the string back in as tokens to build the macro body
		while (scan() != END)
			body->end() = node;

		in_cpp = FALSE;
	}

	macros[macro].define_macro(body, mtype);
	return TRUE;
}

boolean
Cpp::undef_macro(const char *macro)
{
	macro = strget(macro);

	if (!macros.find(macro))
		return TRUE;

	if (macros().gettype() != NORMAL_MACRO && g_strict_iso)
		return FALSE;

	delete macros().body;
	delete macros().args;
	macros.remove(macro);
	return TRUE;
}

boolean
Cpp::expand_macro(Cpp_macro &macro, Cpp_node_list &buf)
{
	if (!macro.expand())
		return FALSE;

	if (g_verbose)
		bump_file_use(macro.getfile(), files[currfile].fname);

	buf.resize();
	Cpp_node_list_vec params(8);
	params.resize(0);

	if (macro.getargs() != NULL)		// grab 0 or more arg params
	{
		Cpp_node mac = node;
		scan();

		while (node.token == WHITESPACE || node.token == NEWLINE)
		{
			if (node.token == NEWLINE)
				line++;

			scan();
		}

		// function-style macro - pick up comma-separated args
		if (node.token == '('/*)*/)
		{
			scan();

			while (node.token != END && node.token != /*(*/')')
			{
				int parens = 1;
				Cpp_node_list &arg = params.end();

				for (; node.token != END; scan())
				{
					// anything within nested parens counts as one arg
					if (node.token == '(')
						parens++;
					else if (node.token == ')')
					{
						parens--;
						if (parens == 0)
							break;
					}
					else if (node.token == ',' && parens == 1)
						// comma separtor seen at the the correct paren level
						break;

					// newlines are considered white-space here
					if (node.token == NEWLINE)
					{
						line++;
						node.token = WHITESPACE;
					}

					arg.end() = node;
				}

				if (node.token == END)
					error(EXPECTED_CLOSE_PAREN_OR_COMMA);
				else if (node.token != /*(*/')')
					scan();
			}

			if (node.token == END)
				error(EXPECTED_CLOSE_PAREN);

			// at this point all the supplied args+whsp are in "params"

			Cpp_node_list &args = *macro.getargs();

			if (args.size() != params.size())
			{
				error(MACRO_ARGUMENT_COUNT_MISMATCH, macro.getname());
				return FALSE;
			}
		}
		else
		{
			// function-style macro but object-style use, so we ignore it,
			// but we need to push back "node" since it wasn't a paren
			top++;
			stack[top].macro = NULL;
			stack[top].buf.resize();
			stack[top].buf.end() = node;
			stack[top].buf.end().token = END;
			stack[top].loc = 0;
			stack[top].file = currfile;
			node = mac;
			return FALSE;
		}
	}

	// The following 3 sections implement the core of the ANSI preprocessor
	// macro substitution rules *exactly* as the spec reads and *exactly*
	// as it is supposed to behave.

	// perform argument substitution after macro-expanding each parameter
	expand_macro_body(macro, params, buf);

	// rescan the result and expand macros but do not expand ourselves
	macro.expand(FALSE);
	expand_all_macros(buf);
	macro.expand(TRUE);

	// prevent further re-expansion of ourselves in awkward contexts
	for (int i = 0; i < buf.size(); i++)
		if (buf.elt(i).token == IDENTIFIER
				&& buf.elt(i).text == macro.getname())
			buf.elt(i).token = EXPANDED_IDENT;

	return TRUE;
}

boolean
Cpp::expand_macro_body(Cpp_macro &macro, Cpp_node_list_vec &params,
		Cpp_node_list &buf)
{
	Cpp_node *body = NULL;
	int bodylen = 0;

	if (macro.getbody() != NULL)
	{
		body = macro.getbody()->getarr();
		bodylen = macro.getbody()->size();
	}

	// handle all magic macros here
	switch (macro.gettype())
	{
	case FILE_MACRO:		// __FILE__
		buf[0].text = strget(xsprintf("\"%s\"", files[currfile].fname));
		buf[0].token = STRING;
		return TRUE;

	case LINE_MACRO:		// __LINE__
		buf[0].text = strget(xsprintf("%d", line));
		buf[0].token = INTVAL;
		return TRUE;

	case FUNC_MACRO:		// __FUNC__ (not ISO, but nonetheless cool)
		buf[0].text = strget(xsprintf("\"%s\"", get_current_func()));
		buf[0].token = STRING;
		return TRUE;

	default:
		break;
	}

	int i = 0;

	while (i < bodylen && body[i].token == WHITESPACE)
		i++;

	if (i < bodylen && body[i].token == HASHHASH)
		error(HASHHASH_AT_BOL);

	i = 0;

	while (i < bodylen)
	{
		int nx = i + 1;

		while (nx < bodylen && body[nx].token == WHITESPACE)
			nx++;

		// if the next tok is ##, we have to paste tokens - ugly
		if (nx < bodylen && body[nx].token == HASHHASH)
		{
			// start and end tokens to be pasted together
			int s = i;
			int e = nx + 1;

			while (e < bodylen && body[e].token == WHITESPACE)
				e++;

			if (e >= bodylen)
			{
				error(HASHHASH_AT_EOL);
				break;
			}

			i = e + 1;

			Cpp_node n1, n2;		// holds the 2 tokens to be pasted
			int a = 0;
			int ae = 0;
			int j;

			// if toks before/after ## are args, then plug in their params
			if (body[s].token == ARGUMENT)
			{
				Cpp_node_list &arg = params[body[s].arg->argnum];
				a = 0;

				while (a < arg.size() && arg.elt(a).token == WHITESPACE)
					a++;

				if (a >= arg.size())
					a--;

				ae = arg.size() - 1;

				while (ae >= 0 && arg.elt(ae).token == WHITESPACE)
					ae--;

				if (ae < 0)
					ae = 0;

				// copy everything in 1st parameter except last tok
				for (j = a; j < ae; j++)
					buf.end() = arg.elt(j);

				n1 = arg.elt(j);		// this gets pasted
			}
			else
				n1 = body[s];

			if (body[e].token == ARGUMENT)
			{
				Cpp_node_list &arg = params[body[e].arg->argnum];
				a = 0;

				while (a < arg.size() && arg.elt(a).token == WHITESPACE)
					a++;

				if (a >= arg.size())
					a--;

				ae = arg.size() - 1;

				while (ae >= 0 && arg.elt(ae).token == WHITESPACE)
					ae--;

				if (ae < 0)
					ae = 0;

				n2 = arg.elt(a++);		// 1st tok in 2nd argument is pasted
			}
			else
				n2 = body[e];

			// now make a string catenating the 2 tokens
			int len = strlen(n1.tok2str());
			len += strlen(n2.tok2str());
			char *str = strnew(len);
			strcpy(str, n1.tok2str());
			strcat(str, n2.tok2str());

			// push a fake file on the file stack for rescanning
			files[currfile].line = line;
			files[currfile].ptr = ptr;
			files[++currfile].rescan(str);
			ptr = files[currfile].ptr;
			int c = 0;

			// scan and buffer up the tokens within the string
			while (scan() != END)
			{
				buf.end() = node;
				c++;
			}

			strfree(str);

			if (c > 1)
				warn(HASHHASH_EXPANDS_TO_ONE_TOKEN, macro.getname(), c);

			// append anything left in the second parameter
			if (body[e].token == ARGUMENT)
			{
				Cpp_node_list &arg = params[body[e].arg->argnum];

				for (j = a; j <= ae; j++)
					buf.end() = arg.elt(j);
			}
		}
		else		// relatively simple expansion of macro body
		{
			Cpp_node &n = body[i];

			switch (n.token)
			{
			// copy expanded argument parameter - this could be buffered
			// for future expansions of the same parameter, but hardly
			// seems worth the effort after looking at the profiler data
			case ARGUMENT:
				{
					Cpp_node_list arg = params[n.arg->argnum];
					expand_all_macros(arg);
					int k = arg.size();

					for (int j = 0; j < k; j++)
						buf.end() = arg.elt(j);
				}

				break;

			// stringify next argument - add quotes and escape magic chars
			case '#':
				{
					if (nx >= bodylen)
					{
						error(STRINGIFY_MISSING_ARG);
						break;
					}

					if (body[nx].token != ARGUMENT)
					{
						error(PARAM_MUST_FOLLOW_STRINGIFY, body[nx].text);
						break;
					}

					// select starting "a" and ending "ae" points in param
					// - note that param is NOT macro-expanded for #op
					Cpp_node_list &arg = params[body[nx].arg->argnum];
					i = nx;
					int a = 0;

					while (a < arg.size() && arg.elt(a).token == WHITESPACE)
						a++;

					if (a >= arg.size())		// empty token
						a--;

					int ae = arg.size() - 1;

					while (ae >= 0 && arg.elt(ae).token == WHITESPACE)
						ae--;

					if (ae < 0)
						ae = 0;

					int j, len = 0;

					for (j = a; j <= ae; j++)
						len += strlen(arg.elt(j).tok2str());

					// assume we have to esc every char + 2 for quotes
					char *str = strnew(len * 2 + 2);

					char *ss = str;
					*ss++ = '"';

					for (j = a; j <= ae; j++)
					{
						// these need to be escaped properly
						if (arg.elt(j).token == STRING
								|| arg.elt(j).token == WSTRING
								|| arg.elt(j).token == CHARACTER
								|| arg.elt(j).token == WCHARACTER)
						{
							for (const char *s = arg.elt(j).tok2str();
									*s != '\0'; *ss++ = *s++)
								if (*s == '"' || *s == '\\')
									*ss++ = '\\';
						}
						else		// these can just be copied
						{
							strcpy(ss, arg.elt(j).tok2str());
							ss += strlen(ss);
						}

						// skip whitespace in param
						if (arg.elt(j).token == WHITESPACE)
							while (arg.elt(j + 1).token == WHITESPACE)
								j++;
					}

					*ss++ = '"';
					*ss = '\0';

					Cpp_node n;
					n.text = strget(str);
					n.token = STRING;
					buf.end() = n;
					strfree(str);
				}

				break;

			case NEWLINE:
			case END:
				break;

			// just a plain old normal token
			default:
				buf.end() = n;
				break;
			}

			i++;

			// copy anything else, such as whsp, if we did not do stringify
			while (i < nx)
				buf.end() = body[i++];
		}
	}

	return TRUE;
}

void
Cpp::expand_all_macros(Cpp_node_list &buf)
{
	// push buf on our stack for rescanning and macro expansion
	buf.end().token = END;
	int oldtop = top++;
	stack[top].macro = NULL;
	stack[top].buf = buf;		// copies buf
	stack[top].loc = 0;
	stack[top].file = currfile;
	Cpp_node savnode = node;

	buf.resize();		// return expanded buffer in buf

	for (scan(); node.token != END && top > oldtop; scan())
	{
		if (node.token == IDENTIFIER && macros.find(node.text))
		{
			Cpp_node sav = node;
			Cpp_node_list exp(16);
			exp.resize(0);

			// recursive expansion of macros
			if (expand_macro(macros(), exp))
				for (int i = 0; i < exp.size(); i++)
					buf.end() = exp.elt(i);
			else
				buf.end() = sav;
		}
		else
			buf.end() = node;
	}

	node = savnode;
	top = oldtop;
}

const char *
Cpp_node::tok2str()
{
	if (token < 256)
	{
		static char buf[2];

		buf[0] = token;
		buf[1] = '\0';
		return buf;
	}

	switch (token)
	{
	case EXPANDED_IDENT:
	case IDENTIFIER:
	case TYPENAME:
	case INTVAL:
	case UINTVAL:
	case LONGVAL:
	case ULONGVAL:
	case LONGLONGVAL:
	case ULONGLONGVAL:
	case DOUBLEVAL:
	case FLOATVAL:
	case LONGDOUBLEVAL:
	case SHORTDOUBLEVAL:
	case CHARACTER:
	case WCHARACTER:
	case STRING:
	case WSTRING:
		return text;

	case DOTDOTDOT:  return "...";
	case ARROW:  return "->";
	case INCR:  return "++";
	case DECR:  return "--";
	case LSHIFT:  return "<<";
	case RSHIFT:  return ">>";
	case LESSEQ:  return "<=";
	case GREATEQ:  return ">=";
	case EQUAL:  return "==";
	case NOTEQ:  return "!=";
	case ANDAND:  return "&&";
	case OROR:  return "||";
	case MULTEQ:  return "*=";
	case DIVEQ:  return "/=";
	case MODEQ:  return "%=";
	case PLUSEQ:  return "+=";
	case MINUSEQ:  return "-=";
	case LSHIFTEQ:  return "<<=";
	case RSHIFTEQ:  return ">>=";
	case ANDEQ:  return "&=";
	case XOREQ:  return "^=";
	case OREQ:  return "|=";
#ifdef COLONCOLON
	case COLONCOLON:  return "::";
#endif

	case WHITESPACE:  return " ";
	case NEWLINE:  return "\n";
	case HASHHASH:  return "##";

	default: return text; //"unknown";
	}
}
