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
// $Header: /u/cgt/cvs/bin/cc/cppif.cc,v 1.33 1999/03/15 21:48:56 parag Exp $
#include "cpp.h"
#include <pool.h>
#include <ctype.h>

// to calculate return values from #if sub-expressions
struct Ret
{
	union
	{
		Long val;
		uLong uval;
	};
	boolean is_signed;
};

static void conditional(Cpp_node *&buf, struct Ret &ret);

static const char *s_defined = NULL;
static const char *s_if = NULL;
static const char *s_ifdef = NULL;
static const char *s_ifndef = NULL;
static const char *s_else = NULL;
static const char *s_elif = NULL;
static const char *s_endif = NULL;

static void
init_strs()
{
	s_defined = strget("defined");
	s_if = strget("if");
	s_ifdef = strget("ifdef");
	s_ifndef = strget("ifndef");
	s_else = strget("else");
	s_elif = strget("elif");
	s_endif = strget("endif");
}

static inline void
bufskipwhsp(Cpp_node *&buf)
{
	while (buf->token == WHITESPACE)
		buf++;
}

static void
primary(Cpp_node *&buf, struct Ret &ret)
{
	bufskipwhsp(buf);

	switch (buf->token)
	{
	// (expr)
	case '('/*)*/:
		buf++;
		conditional(buf, ret);
		bufskipwhsp(buf);

		if (buf->token != /*(*/')')
		{
			error(MISSING_PAREN_IN_IFDEF);
			return;
		}
		else
			buf++;

		break;

	// integer value
	case LONGLONGVAL:
	case ULONGLONGVAL:
		warn(UNEXPECTED_LONGLONG_IN_IFDEF);
		// fall through to rest of integer code

	case INTVAL:
	case UINTVAL:
	case LONGVAL:
	case ULONGVAL:
		char *s;
		ret.val = strtol(buf->text, &s, 0);

		if (tolower(*s) == 'u' || tolower(s[1]) == 'u')
			ret.is_signed = FALSE;
		else
			ret.is_signed = TRUE;

		buf++;
		break;

	case CHARACTER:
		ret.val = buf->text[1];
		ret.is_signed = TRUE;
		buf++;
		break;

	// any name seen here expands to zero
	case IDENTIFIER:
	case EXPANDED_IDENT:
		ret.val = 0;
		buf++;
		break;
	
	default:
		error(UNEXPECTED_ITEM_IN_IFDEF, buf->tok2str());
		break;
	}
}

static void
unary(Cpp_node *&buf, struct Ret &ret)
{
	bufskipwhsp(buf);
	ret.is_signed = TRUE;

	switch (buf->token)
	{
	case '-':
		buf++;
		unary(buf, ret);
		ret.val = -ret.val;
		break;

	case '!':
		buf++;
		unary(buf, ret);
		ret.val = !ret.val;
		break;

	case '~':
		buf++;
		unary(buf, ret);
		ret.uval = ~ret.uval;
		break;

	default:
		primary(buf, ret);
		break;
	}
}

static void
multiplicative(Cpp_node *&buf, struct Ret &ret)
{
	bufskipwhsp(buf);
	unary(buf, ret);
	struct Ret e2;

	while (1)
	{
		bufskipwhsp(buf);

		switch (buf->token)
		{
		case '*':
			buf++;
			unary(buf, e2);

			if (ret.is_signed && e2.is_signed)
				ret.val *= e2.val;
			else if (ret.val != 0)
				ret.uval *= e2.uval;

			break;

		case '/':
			buf++;
			unary(buf, e2);

			if (e2.val == 0)
				warn(DIVIDE_BY_ZERO_IN_IFDEF);
			else if (ret.val != 0)
			{
				if (ret.is_signed && e2.is_signed)
					ret.val /= e2.val;
				else
					ret.uval /= e2.uval;
			}

			break;

		case '%':
			buf++;
			unary(buf, e2);

			if (e2.val == 0)
				warn(MODULO_BY_ZERO_IN_IFDEF);
			else if (ret.val != 0)
			{
				if (ret.is_signed && e2.is_signed)
					ret.val %= e2.val;
				else
					ret.uval %= e2.uval;
			}

			break;

		default:
			return;
		}

		ret.is_signed = ret.is_signed && e2.is_signed;
	}
}

static void
additive(Cpp_node *&buf, struct Ret &ret)
{
	bufskipwhsp(buf);
	multiplicative(buf, ret);
	struct Ret e2;

	while (1)
	{
		bufskipwhsp(buf);

		switch (buf->token)
		{
		case '+':
			buf++;
			multiplicative(buf, e2);
			ret.val = ret.val + e2.val;
			break;

		case '-':
			buf++;
			multiplicative(buf, e2);
			ret.val = ret.val - e2.val;
			break;

		default:
			return;
		}

		ret.is_signed = ret.is_signed && e2.is_signed;
	}
}

static void
shift(Cpp_node *&buf, struct Ret &ret)
{
	bufskipwhsp(buf);
	additive(buf, ret);
	struct Ret e2;

	while (1)
	{
		bufskipwhsp(buf);

		switch (buf->token)
		{
		case LSHIFT:
			buf++;
			additive(buf, e2);

			if (ret.is_signed)
				ret.val = ret.val << e2.val;
			else
				ret.uval = ret.uval << e2.val;

			break;

		case RSHIFT:
			buf++;
			additive(buf, e2);

			if (ret.is_signed)
				ret.val = ret.val >> e2.val;
			else
				ret.uval = ret.uval >> e2.val;

			break;

		default:
			return;
		}
	}
}

static void
relational(Cpp_node *&buf, struct Ret &ret)
{
	bufskipwhsp(buf);
	shift(buf, ret);
	struct Ret e2;

	while (1)
	{
		bufskipwhsp(buf);

		switch (buf->token)
		{
		case '<':
			buf++;
			shift(buf, e2);
			ret.is_signed = ret.is_signed && e2.is_signed;

			if (ret.is_signed)
				ret.val = ret.val < e2.val;
			else
				ret.uval = ret.uval < e2.uval;

			break;

		case LESSEQ:
			buf++;
			shift(buf, e2);
			ret.is_signed = ret.is_signed && e2.is_signed;

			if (ret.is_signed)
				ret.val = ret.val <= e2.val;
			else
				ret.uval = ret.uval <= e2.uval;

			break;

		case '>':
			buf++;
			shift(buf, e2);
			ret.is_signed = ret.is_signed && e2.is_signed;

			if (ret.is_signed)
				ret.val = ret.val > e2.val;
			else
				ret.uval = ret.uval > e2.uval;

			break;

		case GREATEQ:
			buf++;
			shift(buf, e2);
			ret.is_signed = ret.is_signed && e2.is_signed;

			if (ret.is_signed)
				ret.val = ret.val >= e2.val;
			else
				ret.uval = ret.uval >= e2.uval;

			break;

		default:
			return;
		}

		ret.is_signed = TRUE;
	}
}

static void
equality(Cpp_node *&buf, struct Ret &ret)
{
	bufskipwhsp(buf);
	relational(buf, ret);
	struct Ret e2;

	while (1)
	{
		bufskipwhsp(buf);

		switch (buf->token)
		{
		case EQUAL:
			buf++;
			relational(buf, e2);
			ret.val = ret.val == e2.val;
			break;

		case NOTEQ:
			buf++;
			relational(buf, e2);
			ret.val = ret.val != e2.val;
			break;

		default:
			return;
		}

		ret.is_signed = TRUE;
	}
}

static void
bitwise_and(Cpp_node *&buf, struct Ret &ret)
{
	bufskipwhsp(buf);
	equality(buf, ret);
	struct Ret e2;

	while (1)
	{
		bufskipwhsp(buf);

		if (buf->token != '&')
			return;

		buf++;
		equality(buf, e2);
		ret.is_signed = ret.is_signed && e2.is_signed;
		ret.val = ret.val & e2.val;
	}
}

static void
exclusive_or(Cpp_node *&buf, struct Ret &ret)
{
	bufskipwhsp(buf);
	bitwise_and(buf, ret);
	struct Ret e2;

	while (1)
	{
		bufskipwhsp(buf);

		if (buf->token != '^')
			return;

		buf++;
		bitwise_and(buf, e2);
		ret.is_signed = ret.is_signed && e2.is_signed;
		ret.val = ret.val ^ e2.val;
	}
}

static void
inclusive_or(Cpp_node *&buf, struct Ret &ret)
{
	bufskipwhsp(buf);
	exclusive_or(buf, ret);
	struct Ret e2;

	while (1)
	{

		bufskipwhsp(buf);

		if (buf->token != '|')
			return;

		buf++;
		exclusive_or(buf, e2);
		ret.is_signed = ret.is_signed && e2.is_signed;
		ret.val = ret.val | e2.val;
	}
}

static void
logical_and(Cpp_node *&buf, struct Ret &ret)
{
	bufskipwhsp(buf);
	inclusive_or(buf, ret);
	struct Ret e2;

	while (1)
	{
		bufskipwhsp(buf);

		if (buf->token != ANDAND)
			return;

		buf++;
		inclusive_or(buf, e2);
		ret.is_signed = ret.is_signed && e2.is_signed;
		ret.val = ret.val && e2.val;
	}
}

static void
logical_or(Cpp_node *&buf, struct Ret &ret)
{
	bufskipwhsp(buf);
	logical_and(buf, ret);
	struct Ret e2;

	while (1)
	{
		bufskipwhsp(buf);

		if (buf->token != OROR)
			return;

		buf++;
		logical_and(buf, e2);
		ret.is_signed = ret.is_signed && e2.is_signed;
		ret.val = ret.val || e2.val;
	}
}

static void
conditional(Cpp_node *&buf, struct Ret &ret)
{
	bufskipwhsp(buf);
	logical_or(buf, ret);
	bufskipwhsp(buf);

	if (buf->token != '?')
		return;

	struct Ret e1;
	buf++;
	conditional(buf, e1);
	bufskipwhsp(buf);

	if (buf->token != ':')
	{
		error(MISSING_COLON_IN_IFDEF);
		return;
	}

	struct Ret e2;
	buf++;
	conditional(buf, e2);
	ret = ret.val ? e1 : e2;
}

// is this #if, #ifdef, #ifndef the first in this file?
static boolean
is_first(const char *p)
{
	// first skip backwards over "if", "ifdef", or "ifndef" to the '#'
	while (*p != '\0' && *p != '#')
		--p;

	// only whitespace and newlines may precede this if it is the first
	for (--p; *p != '\0' && (iswhsp(*p) || *p == NL); --p)
		;

	// nul byte means beginning of buffer, so this is indeed the first
	return *p == '\0';
}

void
Cpp::do_defined(Cpp_node_list &buf, Cpp_node *arr)
{
	buf.reset(0);

	// first handle all occurrences of "defined" in the expression
	for (; arr->token != NEWLINE && arr->token != END; arr++)
	{
		// is this identifier the string "defined"?
		if ((arr->token == IDENTIFIER || arr->token == EXPANDED_IDENT) &&
				arr->text == s_defined)
		{
			arr++;
			const char *macro = NULL;
			bufskipwhsp(arr);

			// the following token must be identifier or open-paren
			if (arr->token == IDENTIFIER || arr->token == EXPANDED_IDENT)
				macro = arr->text;
			else if (arr->token == '('/*)*/)
			{
				arr++;
				bufskipwhsp(arr);

				// identifier and close-paren must follow open-paren
				if (arr->token == IDENTIFIER || arr->token == EXPANDED_IDENT)
				{
					macro = arr->text;
					arr++;
					bufskipwhsp(arr);

					if (arr->token != /*(*/')')
						error(MISSING_DEFINED_PAREN_IN_IFDEF);
				}
				else
					error(MISSING_MACRO_NAME_IN_IFDEF);
			}
			else
				error(BAD_DEFINED_USAGE_IN_IFDEF);

			// if we found a macro identifier, see if is currently defined
			if (macro != NULL)
			{
				Cpp_node &node = buf.end();
				node.text = macros.find(macro) ? s_one : s_zero;
				node.token = INTVAL;
				continue;
			}
		}

		// otherwise append this node to the end of our expansion buffer
		buf.end() = *arr;
	}
}

boolean
Cpp::test_if_expr(Cpp_node_list *expr)
{
	Cpp_node_list buf(16);
	buf.resize(0);
	Cpp_node *arr = expr->getarr();

	if (s_defined == NULL)
		init_strs();

	// first handle all "defined"s in the expression, then expand macros
	do_defined(buf, arr);
	expand_all_macros(buf);
	buf.end().token = END;

	// then see if any macros expanded to "defined", which is undefined
	// behaviour according to the ISO spec, but people seem to do it anyway
	//Cpp_node_list buf2 = buf;
	//do_defined(buf, buf2.getarr());
	//buf.end().token = END;

	// evaluate the resulting conditional expression
	struct Ret ret;
	arr = buf.getarr();
	conditional(arr, ret);
	bufskipwhsp(arr);

	if (arr->token != END)
		error(UNEXPECTED_STUFF_IN_IFDEF, arr->tok2str());

	return ret.val != 0;
}

void
Cpp::if_directive()
{
	Cpp_file &file = files[currfile];
	boolean docontrol = !file.control && file.ifcount == 0 && is_first(ptr);
	file.ifcount++;
	file.seen_else[file.ifcount] = FALSE;
	Cpp_node_list buf(16);
	buf.resize(0);
	skipwhsp();
	buf.end() = node;
	getline(buf);
	buf.end().token = END;

	if (docontrol)
		file.control = new Cpp_node_list(buf);
	else if (file.ifcount == 1)
		file.control = NULL;

	// skip this #if (to #else, #elif, or #endif) if value is zero
	if (!test_if_expr(&buf))
	{
		// an #if* directive could follow this #if, so make skip_if() behave
		*--ptr = NL;
		skip_if();
	}
	// otherwise, we continue scanning the tokens within this #if block
}

void
Cpp::skip_if(boolean stop_at_else)		// default is TRUE
{
	Cpp_file &file = files[currfile];
	int ifcount = file.ifcount;
	int count = ifcount;
	int c, e;
	char *p = ptr;

	if (s_if == NULL)
		init_strs();

	// scan the raw text here rather than in Cpp::scan for speed
	// - look for directives, strings, and char constants
	for (c = *p++; c != '\0'; c = *p++)
		switch (c)
		{
		case NL:
			line++;

			for (c = *p++; iswhsp(c); c = *p++)
				;

			// start of some sort of preprocessing directive?
			if (c == '#')
			{
				ptr = p;
				skipwhsp();

				// bump counts of nested ifdefs
				if (node.text == s_if || node.text == s_ifdef
						|| node.text == s_ifndef)
					file.seen_else[++count] = FALSE;
				else if (node.text == s_else)
				{
					if (file.seen_else[count])
						error(TOO_MANY_ELSES_IN_IFDEF);
					else
						file.seen_else[count] = TRUE;

					// if this is our matching #else, then return
					if (stop_at_else && count == ifcount)
					{
						if (file.control && ifcount == 1)
						{
							delete file.control;
							file.control = NULL;
						}

						skipwhsp();
						return;
					}
				}
				else if (node.text == s_elif)
				{
					if (file.seen_else[count])
						error(ELIF_AFTER_ELSE_IN_IFDEF);

					// if this is a matching #elif, try evaluating it (which
					// will probably call skip_if recursively) and return
					if (stop_at_else && count == ifcount)
					{
						if (file.control && ifcount == 1)
						{
							delete file.control;
							file.control = NULL;
						}

						file.ifcount--;
						if_directive();		// this increments the if count again
						return;
					}
				}
				else if (node.text == s_endif)
				{
					// if this is our endif then return, else decr counts
					if (count == ifcount)
					{
						endif_directive();
						return;
					}
					else
						count--;
				}

				p = ptr;
			}
			else
				p--;		// not a directive - put it back and keep scanning

			break;

		// scan strings and character constants
		case '"':
		case '\'':
			e = c;

			for (c = *p++; c != '\0' && c != NL && c != e; c = *p++)
				if (c == '\\')
					c = *p++;

			if (c != e)
			{
				error(UNTERMINATED_STRING_OR_CHAR_CONST);
				line++;
			}

			break;

		default:
			break;
		}

	if (node.token == END && count != ifcount)
		error(MISSING_ENDIF_FOR_IFDEF);

	ptr = p;
}

void
Cpp::ifdef_directive(boolean expected)
{
	Cpp_file &file = files[currfile];
	boolean docontrol = !file.control && file.ifcount == 0 && is_first(ptr);
	file.ifcount++;
	file.seen_else[file.ifcount] = FALSE;
	skipwhsp();

	if (s_defined == NULL)
		init_strs();

	if (docontrol)
	{
		// build an equivalent "#if" controlling expression for this macro
		file.control = new Cpp_node_list;
		int i = 0;

		if (!expected)
			(*file.control)[i++].token = '!';

		if (node.token == IDENTIFIER)
		{
			(*file.control)[i].token = IDENTIFIER;
			(*file.control)[i++].text = s_defined;
		}

		(*file.control)[i++] = node;
		(*file.control)[i].token = END;
	}
	else if (file.ifcount == 1)
		file.control = NULL;

	// following token may be an identifier or an integer number
	if (node.token == IDENTIFIER)
	{
		// if this macro is expected, return, else skip to the end of #ifdef
		if (macros.find(node.text) == expected)
		{
			skipwhsp();
			return;
		}
		else
			skip_if();
	}
	else if (node.token == INTVAL || node.token == UINTVAL
			|| node.token == LONGVAL || node.token == ULONGVAL)
	{
		// if some value is expected, return, else skip to the end
		if (!!strtol(node.text, NULL, 0) == expected)
		{
			skipwhsp();
			return;
		}
		else
			skip_if();
	}
	else
	{
		if (docontrol)
		{
			delete file.control;
			file.control = NULL;
		}

		error(EXPECTED_MACRO_NAME_AFTER_IFDEF, node.text);
		skipwhsp();
	}
}

void 
Cpp::elif_directive()
{
	Cpp_file &file = files[currfile];
	int ifcount = file.ifcount;

	if (file.seen_else[ifcount])
		error(ELIF_AFTER_ELSE_IN_IFDEF);

	if (file.control && ifcount == 1)
	{
		delete file.control;
		file.control = NULL;
	}

	if (ifcount == 0)
	{
		error(UNMATCHED_ELIF_WITHOUT_IFDEF);
		skipwhsp();
	}
	else
		// skip this #if* to its #endif ignoring any #else/#elif in the way
		skip_if(FALSE);
}

void 
Cpp::else_directive()
{
	Cpp_file &file = files[currfile];
	int ifcount = file.ifcount;

	if (file.seen_else[ifcount])
		error(TOO_MANY_ELSES_IN_IFDEF);
	else
		file.seen_else[ifcount] = TRUE;

	if (file.control && ifcount == 1)
	{
		delete file.control;
		file.control = NULL;
	}

	if (ifcount == 0)
	{
		error(UNMATCHED_ELSE_WITHOUT_IFDEF);
		skipwhsp();
	}
	else
		// skip this #if* to its #endif ignoring any #else/#elif in the way
		skip_if(FALSE);
}

void 
Cpp::endif_directive()
{
	Cpp_file &file = files[currfile];

	if (file.ifcount == 0)
		error(UNMATCHED_ENDIF_WITHOUT_IFDEF);
	else
		file.ifcount--;

	skipwhsp();

	// does this end a nested #if, or is there no controlling macro?
	if (!file.control || file.ifcount)
		return;

	// otherwise we look to see if this is the final #endif for the file
	char *p = ptr;

	while (iswhsp(*p) || *p == NL)
		p++;

	// the end-of-file, so this must be the expression controlling #if*
	if (*p == '\0')
		controls[file.fname].control(file.control);
	else
		delete file.control;

	file.control = NULL;
}
