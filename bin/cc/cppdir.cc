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
// $Header: /u/cgt/cvs/bin/cc/cppdir.cc,v 1.44 1999/03/15 21:48:55 parag Exp $
#include "cpp.h"
#include "fileuse.h"
#include <stdiox.h>
#include <pool.h>
#include <ctype.h>

#define boolean bogus_bool
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#undef boolean

#ifdef macintosh
#  define STRGET strgeti
#else
#  define STRGET strget
#endif

boolean
Cpp_file::open(const char *fn)
{
#ifdef _DJGPP_SOURCE
	int fd = ::open(fn, O_RDONLY | O_BINARY);
#else
	int fd = ::open(fn, O_RDONLY);
#endif

	if (fd < 0)
		return FALSE;

	if (!filelength(fn, len))
	{
		::close(fd);
		return FALSE;
	}

	strfree(buf);
	// 1 for leading '\0' + 1 for final '\n' at end + 1 for '\0' = 3
	// - leading '\0' is to easily search backwards in buffer
	buf = strnew(len + 3);

	if (buf == NULL || (len = read(fd, buf + 1, len)) <= 0)
	{
		::close(fd);
		return FALSE;
	}

	::close(fd);
	*buf = '\0';		// inc past leading nul byte

	// add final newline at end of file if one is not already there
	if (buf[len] != NL)
	{
		if (g_strict_iso)
			warn(MISSING_NEWLINE_AT_EOF, base_name(fn));

		buf[++len] = NL;
	}

	buf[len + 1] = '\0';
	ptr = buf + 1;
	ifcount = 0;
	seen_else[0] = FALSE;
	fname = STRGET(fn);
	line = 0;
	control = NULL;

	if (g_debug)
		fprintf(stderr, "#include %s\n", fname);

	return TRUE;
}

void
Cpp_file::rescan(const char *str)
{
	strfree(buf);
	ptr = buf = strdup(str);
	ifcount = 0;
	seen_else[0] = FALSE;
	fname = NULL;
	line = 0;
}

void
Cpp_file::close()
{
	strfree(buf);
	buf = NULL;
}

Cpp::Cpp()
{
	currfile = 0;
	top = -1;
	node.token = NEWLINE;
	in_cpp = FALSE;
	s_zero = strget("0");
	s_one = strget("1");
	add_include_dir("/usr/include");

	time_t now;
	struct tm *tbuf;
	now = time((time_t*)NULL);
	tbuf = localtime(&now);

	static const char *month[] = { "Jan", "Feb", "Mar", "Apr", "May",
			"Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	char buf[100];

	sprintf(buf, "\"%2d:%.2d:%.2d\"", tbuf->tm_hour,
			tbuf->tm_min, tbuf->tm_sec);
	define_macro("__TIME__", buf);
	sprintf(buf, "\"%3s %2d %4d\"", month[tbuf->tm_mon],
			tbuf->tm_mday, tbuf->tm_year + 1900);
	define_macro("__DATE__", buf);
	define_macro("__STDC__", g_strict_iso ? s_one : s_zero);

	define_macro("__FILE__", NULL, FILE_MACRO);
	define_macro("__LINE__", NULL, LINE_MACRO);

	if (!g_strict_iso)
		define_macro("__FUNC__", NULL, FUNC_MACRO);
}

boolean
Cpp::check_control(const char *fn)
{
	// if this file is wrapped by a controlling #if* expression, and this
	// time the expr does not have the same value as the previous time,
	// then this entire file will end up being skipped, so do not read it

	if (controls.find(fn) && controls().control() != NULL &&
			!test_if_expr(controls().control()))
	{
		if (g_debug)
			fprintf(stderr, "skipping #include %s\n", fn);

		if (g_verbose)
			new_file_use(fn,
					currfile ? files[currfile - 1].fname : (const char*)NULL,
					-1, 0);

		return TRUE;
	}

	return FALSE;
}

boolean
Cpp::open(const char *fname)
{
	currfile = 0;
	top = -1;
	in_cpp = FALSE;
	node.token = NEWLINE;

	if (check_control(fname))
		return TRUE;

	if (files[currfile].open(fname))
	{
		ptr = files[currfile].ptr;
		line = files[currfile].line;
		cleanupbuf();
		return TRUE;
	}

	return FALSE;
}

void
Cpp::add_include_dir(const char *dir)
{
	includes.end() = STRGET(dir);
}

void 
Cpp::include_directive()
{
	const char *fn = NULL;
	int c = get();

	// if not one of the normal "f" <f> forms, then try expanding macros
	if (c != '"' && c != '<')
	{
		Cpp_node_list buf(16);
		buf.resize(0);
		ptr--;
		getline(buf);
		expand_all_macros(buf);
		int len = 0;
		int i;

		for (i = 0; i < buf.size(); i++)
			len += strlen(buf[i].text);

		char *str = strnew(len);
		str[0] = '\0';

		for (i = 0; i < buf.size(); i++)
			strcat(str, buf[i].text);

		len--;

		// must have a normal form here
		if ((str[0] != '"' || str[len] != '"') &&
				(str[0] != '<' || str[len] != '>'))
		{
			error(ILLEGAL_FORM_FOR_INCLUDE_MACRO);
			return;
		}

		str[len] = '\0';
		fn = STRGET(str + 1);
		strfree(str);
	}
	else
	{
		// normal form - get the file name without any macro expansion
		fn = ptr - 1;
		int end = c;

		if (c == '<')
			end = '>';

		for (c = get(); c != end && c != NL && c != '\0'; c = get())
			;

		ptr[-1] = '\0';
	}

	files[currfile].line = line;
	files[currfile].ptr = ptr;
	currfile++;
	const char *fname = fn + 1;
	node.token = NEWLINE;

	// search here for a file, wherever here happens to be
	if (*fn == '"')
	{
		if (*fname != PATH_SEP)
		{
			// search for this file in directory of the enclosing file
			const char *dir = dir_name(files[currfile - 1].fname);
			
			if (streq(dir, CURRENT_DIR))
					fn = fname;
			else
				fn = xsprintf("%s%c%s", dir, PATH_SEP, fname);
		}
		else
			fn = fname;

		if (check_control(fn))
		{
			--currfile;
			node.token = WHITESPACE;		// make sure line count is correct
			return;
		}

		if (files[currfile].open(fn))
		{
			// got a file
			line = files[currfile].line;
			ptr = files[currfile].ptr;
			cleanupbuf();
			return;
		}
	}

	// look through the search-path directories for this file
	for (int i = 0; i < includes.size(); i++)
	{
		fn = xsprintf("%s%c%s", (char*)includes[i], PATH_SEP, fname);

		if (check_control(fn))
		{
			--currfile;
			node.token = WHITESPACE;		// make sure line count is correct
			return;
		}

		if (files[currfile].open(fn))
		{
			// got a file
			line = files[currfile].line;
			ptr = files[currfile].ptr;
			cleanupbuf();
			return;
		}
	}

	currfile--;
	node.token = WHITESPACE;		// make sure line count is correct
	error(CANNOT_OPEN_INCLUDE_FILE, fname);
}

void 
Cpp::define_directive()
{
	Cpp_node_list *argp = NULL;
	scan();		// get the macro-name to be defined

	if (node.token != IDENTIFIER)
	{
		error(MISSING_NAME_AFTER_DEFINE_MACRO, node.text);
		Cpp_node_list buf(16);
		buf.resize(0);
		getline(buf);		// skip the rest of this line
		return;
	}

	int macline = line;
	Cpp_node macro = node;
	scan();

	// token immediately following macro-name must be open-paren or whsp
	if (node.token == '('/*)*/)
	{
		// parse off our argument parameter names
		argp = new Cpp_node_list;
		Cpp_node_list &args = *argp;
		skipwhsp();

		while (node.token != /*(*/')' && node.token != NEWLINE)
		{
			if (node.token == IDENTIFIER)
			{
				// save parameter name on end of param list
				Cpp_node &n = args.end();
				n.text = node.text;
				n.argnum = args.size() - 1;
				skipwhsp();
			}
			else
				error(BAD_ARG_LIST_FOR_DEFINE_MACRO, macro.text);

			// param may be followed only by a comma or close paren
			if (node.token == ',')
				skipwhsp();
			else if (node.token != /*(*/')')
				error(EXPECTED_COMMA_IN_DEFINE_MACRO, macro.text);
		}
	}
	else if (node.token != WHITESPACE && node.token != NEWLINE)
		error(EXPECTED_SPACE_AFTER_MACRO_NAME, macro.text);

	Cpp_node_list *bodyp = new Cpp_node_list;
	Cpp_node_list &body = *bodyp;

	// scan in the macro body if there is one
	if (node.token != NEWLINE)
	{
		skipwhsp();

		if (node.token != NEWLINE)
		{
			body.end() = node;
			getline(body);
		}

		int last = body.size() - 1;

		while (last >= 0 && body[last].token == WHITESPACE)
			body.reset(last--);
	}

	// point arguments in the body back to parameter args
	if (argp != NULL)
	{
		Cpp_node *args = argp->getarr();
		Cpp_node *bod = body.getarr();

		for (int i = 0; i < body.size(); i++)
			if (bod[i].token == IDENTIFIER)
			{
				int j;
				
				// linear search for now - unlikely to have too many args
				for (j = 0; j < argp->size(); j++)
					if (args[j].text == bod[i].text) // strings are in pool
						break;

				if (j < argp->size())
				{
					bod[i].token = ARGUMENT;
					bod[i].arg = &args[j];
				}
			}
	}

	// if the macro is already defined, this definition must match it
	if (macros.find(macro.text))
	{
		// see if this is an attempt to redefine a so-called builtin macro
		if (macros().gettype() != NORMAL_MACRO)
		{
			if (g_strict_iso)
			{
				error(CANNOT_UNDEF_BUILTIN_MACRO, macros().getname());
				return;
			}
			else
				warn(UNDEF_OF_BUILTIN_MACRO, macros().getname());
		}

		Cpp_node_list &macbody = *macros().getbody();
		boolean err = FALSE;

		// make sure that the macro parameter args are identical
		if (argp)
		{
			if (macros().getargs() == NULL ||
					argp->size() != macros().getargs()->size())
				err = TRUE;
		}
		else
			if (macros().getargs() != NULL)
				err = TRUE;

		// make sure that the macro bodies are identical
		if (body.size() != macbody.size())
			err = TRUE;
		else if (!err)
		{
			Cpp_node *bod = body.getarr();
			Cpp_node *mbod = macbody.getarr();

			for (int i = 0; i < body.size(); i++)
				if (body[i] != macbody[i] && (bod[i].token != ARGUMENT
						|| bod[i].arg->text != mbod[i].arg->text))
				{
					err = TRUE;
					break;
				}
		}

		// if macros are not the same, print the appropriate error/warning
		if (err)
		{
			if (macros().getfile() == files[currfile].fname)
			{
				if (g_strict_iso)
					error(MACRO_REDEFINED, macro.text, macros().getline());
				else
					warn(MACRO_REDEFINED, macro.text, macros().getline());
			}
			else
			{
				if (g_strict_iso)
					error(MACRO_REDEFINED_IN_FILE, macro.text,
							macros().getfile(), macros().getline());
				else
					warn(MACRO_REDEFINED_IN_FILE, macro.text,
							macros().getfile(), macros().getline());
			}
		}

		// delete the old macro definition
		delete macros().body;
		delete macros().args;
	}
	else
		macros.insert(macro.text);

	// now define the new macro
	Cpp_macro &mac = macros();
	mac.body = bodyp;
	mac.args = argp;
	mac.file = files[currfile].fname;
	mac.line = macline;
	mac.type = NORMAL_MACRO;
}

void 
Cpp::undef_directive()
{
	skipwhsp();
	const char *macro = NULL;

	// get the macro name to be undefined
	if (node.token != IDENTIFIER)
		error(CANNOT_UNDEF_NON_IDENT, node.text);
	else
		macro = node.text;		// save the macro name

	skipwhsp();

	if (node.token != NEWLINE)
		error(EXTRA_STUFF_AFTER_UNDEF);

	if (macros.find(node.text))
	{
		if (macros().gettype() != NORMAL_MACRO)
		{
			if (g_strict_iso)
				error(CANNOT_UNDEF_BUILTIN_MACRO, macros().getname());
			else
				warn(UNDEF_OF_BUILTIN_MACRO, macros().getname());
		}

		// undefine macro name
		delete macros().body;
		delete macros().args;
		macros.remove(node.text);
	}
}

void 
Cpp::line_directive()
{
	Cpp_node_list buffer(16);
	buffer.resize(0);
	getline(buffer);
	expand_all_macros(buffer);		// could have macros expanding to file/line
	Cpp_node n;
	n.token = END;
	buffer.end() = n;
	Cpp_node *buf = buffer.getarr();

	while (buf->token == WHITESPACE)
		buf++;

	// 1st token must be a line-number composed solely of digits
	if (buf->token == INTVAL)
	{
		// line gets bumped by Cpp::scan() for the newline still in "node"
		files[currfile].line = atoi(buf->text) - 1;

		for (buf++; buf->token == WHITESPACE; buf++)
			;

		// file-name may follow line-number
		if (buf->token == STRING)
		{
			// make sure this is a file-name and not a string
			if (buf->text[0] == '"')
			{
				char *s = strdup(buf->text + 1);
				s[strlen(s) - 1] = '\0';
				buf->text = STRGET(s);
				strfree(s);
			}

			// mark this new file as referenced from the current file
			bump_file_use(buf->text, files[currfile].fname);

			// fake the current file on the stack to be this filename
			files[currfile].fname = buf->text;

			for (buf++; buf->token == WHITESPACE; buf++)
				;
		}

		if (buf->token != NEWLINE && buf->token != END)
			error(BADLY_FORMED_LINE_DIRECTIVE);
		
		// set "line" here so error message above has the proper line number
		line = files[currfile].line;
	}
	else
		error(BADLY_FORMED_LINE_DIRECTIVE);
}

void 
Cpp::error_directive()
{
	Cpp_node_list buffer(16);
	buffer.resize(0);
	getline(buffer);
	Cpp_node n;
	n.token = END;
	buffer.end() = n;
	Cpp_node *buf = buffer.getarr();

	while (buf->token == WHITESPACE)
		buf++;

	Cpp_node *p = buf;
	int len = 2;

	// count the number of chars we need for our message
	for (; p->token != END; p++)
		len += strlen(p->tok2str());

	// allocate buffer for said message
	char *str = strnew(len);
	str[0] = '\0';

	// copy the message tokens into the buffer
	for (p = buf; p->token != END; p++)
		strcat(str, p->tok2str());

	// finally, print the error
	error(ERROR_DIRECTIVE, str);
	strfree(str);
}

void 
Cpp::pragma_directive()
{
	Cpp_node_list buf(16);
	buf.resize(0);
	getline(buf);
	do_pragma_directive(buf);
}

extern void dumpcppnode(Cpp_node &n);

void
dumpcppnode(Cpp_node &n)
{
	fprintf(stderr, "node.text=\"%s\"  node.token=%d (%c)\n", n.text,
			n.token, n.token < 128 ? n.token : '?');
}
