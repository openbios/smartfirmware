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

// Copyright (c) 1992,1998,2000-2001 by Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/bin/cc/error.cc,v 1.74 2001/01/21 18:29:30 parag Exp $
#include "cppdefs.h"
#include "cpp.h"
#include <stdarg.h>
#include <stdiox.h>
#include <backtrace.h>

extern Cpp cpp;

// location of error messages here MUST match that of the corresponding enums
static const char *errors[NUM_ERRORS] =
{
	// preprocessor warnings and errors:
	"cannot open file \"%s\" - either nonexistent or unreadable",
	"open failed - cannot get length of file \"%s\"",
	"open failed - cannot read file \"%s\"",
	"file %s does not end with a newline",
	"unterminated C \"/* */\" comment",
	"unterminated BCPL \"//\" comment",
	"BCPL \"//\" comment is not legal ISO/ANSI C",

	"#%s is not a legal preprocessing directive",
	"missing an #endif directive at end of file",
	"missing %ld #endif directives at end of file",
	"tokens \"#\" and \"##\" are only legal in macros",
	"illegal \"..\" in source - perhaps \"...\" was intended",
	"unterminated string or character constant",
	"character \"$\" is not part of a legal ISO/ANSI C identifier",
	"old-style line directive here is not ISO/ANSI C",

	"macro(s) in #include did not expand to proper form",
	"cannot find or open file \"%s\" for #include",
	"macro name instead of \"%s\" must follow #define",
	"bad argument list for #define \"%s\"",
	"expected comma separator in #define \"%s\"",
	"expected space after macro name \"%s\"",
	"cannot #undef builtin macro \"%s\"",
	"macro \"%s\" defined on line %ld is redefined here",
	"macro \"%s\" defined in file %s line %ld is redefined here",

	"cannot #undef \"%s\" - not an identifier",
	"extra stuff after #undef directive",
	"badly formed #line directive",
	"#undef of builtin macro \"%s\"",
	"%s",		// #error - looks just like a normal error message

	"attempt to divide by zero in #if directive",
	"attempt to modula by zero in #if directive",
	"missing close parenthesis in #if",
	"unexpected \"long long\" constant in #if is treated as \"long\"",
	"unexpected item \"%s\" within #if - something missing?",
	"expected \":\" to match \"?\" in #if",
	"missing close-paren of \"defined\" in #if",
	"macro name must follow \"defined\" in #if",
	"incorrect use of \"defined\" in #if",
	"unexpected stuff in #if at and after \"%s\" - badly formed",
	"too many #else directives in #if*",
	"#elif directive after #else",
	"missing #endif for #if*",
	"macro name must follow #ifdef - not \"%s\"",
	"unmatched #elif without an #if* directive",
	"unmatched #else without an #if* directive",
	"unmatched #endif without an #if* directive",

	"expected a close-paren or comma here - not EOF",
	"expected a close-paren here - not EOF",
	"\"##\" in macro \"%s\" should expand into only a single token - not %ld",
	"argument count mismatch for macro \"%s\"",
	"\"##\" must not occur at the beginning a line",
	"\"##\" must not occur at the end of a line",
	"stringify operator \"#\" must have an argument",
	"token following \"#\" must be a macro parameter and not \"%s\"",

	"\"long long\" is not standard ISO/ANSI C",
	"\"short double\" type is not standard ISO/ANSI C",
	"badly formed number here with illegal suffix character '%c'",

	// compiler warnings and errors:
	"expected \"%s\" token here",
	"badly formed %s here",

	"must have a constant expression here",
	"symbol \"%s\" is not defined or declared",
	"array expression is backwards here - n[a] should be a[n]",
	"array reference of non-array or non-pointer object",
	"array reference expression must be of an integer type",
	"redundant escaped character \'\\%c\' in string",
	"redundant escaped character \'\\%c\' in character constant",
	"extra stuff in character constant",
	"illegal stuff in Struct_type::size() - bug?!?",

	"attempt to call an object that is not a function",
	"not enough arguments for call to function \"%s\"",
	"type mismatch for argument %d in call to function \"%s\"",
	"too many arguments for call to function \"%s\"",

	"structure member reference \"%s\" of non-struct object",
	"no such member \"%s\" in union \"%s\"",
	"no such member \"%s\" in struct \"%s\"",
	"cannot take address of unaddressable or \"const\" object",
	"dereference of non-pointer or non-array object",

	"cannot \"++\" or \"--\" a \"const\" object",
	"should not \"++\" or \"--\" \"volatile\" object",
	"expected integral or floating point argument for unary operator",
	"expected integral argument for unary operator",
	"expected arithmetic expressions for operator",
	"expected arithmetic or pointer type expressions for operator",
	"expected integral expression for operator",
	"incompatible pointer types",
	"illegal types in assignment expression",
	"mismatched expressions in ?:",

	"duplicate type qualifiers \"const\" or \"volatile\"",
	"duplicate type specifiers \"short\", \"long\", \"unsigned\", or \"signed\"",
	"duplicate type specifiers \"inline\", \"virtual\", or \"friend\"",
	"duplicate storage classes \"extern\", \"typedef\", \"static\", \"auto\", or \"register\"",
	"struct/union/enum \"%s\" is already defined on line %d",
	"negative value for bitfield - must have positive or zero integral value",
	"illegal value for bitfield - must be positive integral value",
	"symbol \"%s\" is already defined on line %d",
	"label \"%s\" is already defined on line %d",
	"label \"%s\" is used on line %d but was not defined",
	"label \"%s\" on line %d is not used",

	"\"case\" label is outside of a \"switch\" statement",
	"\"default\" label is outside of a \"switch\" statement",
	"ranged \"case\" statement is not ISO/ANSI C standard",
	"\"continue\" statement is outside a loop statement",
	"\"break\" statement is outside a loop or \"switch\" statement",
	"\"break\" or \"return\" should be last statement in \"switch\"",

	"arrays must have a positive size",
	"incompatible mixture of type elements in declaration",
	"too many storage classes here - only one is allowed",
	"symbol must be of function type",
	"cannot assign to \"const\" object",

	"\"short\" or \"long\" are meaningless for type \"char\"",
	"these type specifiers are meaningless for \"double\" or \"float\"",
	"any type specifier is meaningless for type \"void\"",
	"cannot declare anything of type \"void\" - symbol \"%s\"",
	"cannot declare an array of \"void\"s",
	"type for \"%s\" not compatible with definition on line %d file %s",

	"ISO/ANSI C bitfield may only be \"int\" or \"unsigned int\"",
	"bitfield must be of an integral type",
	"bitfields are only allowed in \"struct\"s",
	"bitfield value %d is too large to store in declared type",
	"bitfield can only be of some integer type",

	"field %s is previously defined on line %d in struct/union",
	"only \"register\" may be specified for parameters",
	"parameter \"%s\" is redeclared here",
	"comma at the end of \"enum\" list is not ISO/ANSI C standard",
	"function \"%s\" already defined on line %d file %s",
	"function parameter \"%s\" may not be of type \"void\"",
	"function parameter \"%s\" is already defined",
	"missing name for parameter %d of func \"%s\" is not ISO/ANSI C standard",

	"illegal type for a \"sizeof\" expression",
	"illegal type for cast - expected arithmetic, pointer, or void exprs",
	"duplicate \"default\" label is in a \"switch\" on line %d",
	"\"case\" expression on line %d must be a constant",
	"duplicate \"case\" statement with value %s on line %d",
	"overlapping ranged \"case\" statement for value %s on line %d",
	"range error in \"case\" on line %d - 1st value must be less than 2nd",
	"no \"case\" statements within \"switch\" statement",

	"obsolescent ISO/ANSI C (old-style) function declarator here",
	"old-style argument declaration \"%s\" is not declared as parameter",

	"type mismatch in initializer here",
	"attempt to initialize incomplete struct %s",
	"attempt to initialize incomplete union %s",
	"too many initializers for array",
	"too many initializers for struct %s",
	"too many initializers for union %s",
	"not enough initializers for array",
	"too many initializers for object",
	"expected constant expression(s) in initializer(s) for \"%s\"",

	"cannot assign to this object - \"const\" or not a legal l-value",
	"symbol \"%s\" on line %d is never used within \"%s\"",
	"symbol \"%s\" on line %d is never used within its scope",
	"symbol \"%s\" is used before it is initialized",

	"\"return\" has expression for void function \"%s\"",
	"expected \"return\" expression for non-void function \"%s\"",
	"expected \"return\" expression for int function \"%s\"",
	"type mismatch for \"return\" expression in function \"%s\"",
	"redundant \"return\" at end of function \"%s\"",
	"possibly missing \"return\" at end of function \"%s\"",

	"referenced symbol \"%s\" cannot be put into a register",
	"cannot put symbol \"%s\" into a register - wrong type",
	"storage class for function \"%s\" can only be \"extern\" or \"static\"",
	"\"%s\" has incompatible storage with definition on line %d file %s",
	"symbol \"%s\" is declared \"extern\" but is also initialized",
	"no storage class is allowed for struct/union members - field \"%s\"",
	"illegal type for struct/union members - field \"%s\"",

	"\"const\" symbol \"%s\" should be initialized here",
	"function \"%s\" ought to be declared before it is called",
	"illegal expression or type mismatch in initializer here",
	"expected integer expression for \"switch\"",
	"cannot cast \"case\" expr on line %d to type of it's \"switch\" expr",
	"enum symbol \"%s\" is already defined on line %d",

	"symbol \"%s\" is already initialized on line %d",
	"cannot declare symbol \"%s\" as \"auto\" - not in a function",
	"cannot get \"sizeof\" an incomplete type",
	"structure member reference \"%s\" of incomplete struct/union/enum",
	"cannot declare symbol \"%s\" as incomplete struct/union/enum",

	"declaration within statements here is not standard ISO/ANSI C",
	"unnamed substructures are not standard ISO/ANSI C",
	"only unnamed subunions and substructs are allowed",
	"cannot make incomplete unnamed struct/union",
	"struct display syntax is not standard ISO/ANSI C",
	"initialisation indexes are not standard ISO/ANSI C",
	"illegal initialisation index value \"%d\"",
	"duplicate initialisation index value \"%d\"",
	"missing initialisation expression for index %d",
	"missing init expressions for indices %d through %d",

	"Pascal-string is not standard ISO/ANSI C",
	"hexadecimal character value is too big to fit in %d byte(s)",
	"Pascal-string here is longer than %d bytes",

	"expected a variable name declaration here",
	"expected a struct bitfield or member declaration here",
	"function definition with both ANSI and K&R style parameter(s)",
	"expected a named struct member declaration here",
	"missing type for a declaration here",
	"function prototype or function-pointer type mismatch here",

	"ISO/ANSI C requires a comma \",\" before \"...\" declaration",
	"mixing declarations and statements is not ISO/ANSI C",
	"no match for call to overloaded function \"%s\"",

	"reference variable declaration must be initialized",
	"\"friend\" may only be used within a class declaration",
	"\"friend\" may only be used for function or class declarations",
	"should declare inheritance as \"public\" or \"private\"",
	"can only inherit a new class from a \"class\" or \"struct\"",
	"reference declaration is not ISO/ANSI C",
	"member function declarations are not ISO/ANSI C",
	"default function parameters are not ISO/ANSI C",
	"declaration within a \"for\" statement is not ISO/ANSI C",
	"multiple inheritance is not supported",

	"non-class \"%s\" initialized using constructor style",
	"no constructors defined for class \"%s\"",
	"no match for any constructor of class \"%s\"",
	"no destructors defined for class \"%s\"",
	"expected a \"class\" type here",
	"expected a pointer type here",
	"illegal expression in constructor here",
	"constructor initialization in non-member here",
	"non-base class initialization for \"%s\" here",
	"no member \"%s\" in class \"%s\"",
	"no match for overloaded member \"%s\" in class \"%s\"",

	// extras to be sorted above later
	"unexpected label here - perhaps this is not ANSI C code",
	"\"%s\" on line %d is not declared",
	"illegal/unsupported type in cast operator here",
	"comparing pointer constants, which is pointless",
	"badly formatted #pragma directive here",
	"struct fstr must be defined before Forth strings are used",

	""				// last entry without comma
};

static const char *warnstr = "warning: ";
static const char *errorstr = "error: ";
static const char *fatalstr = "fatal: ";

static int err = 0;

boolean
num_errors()
{
	return err;
}

void
reset_errors()
{
	err = 0;
}

static void
printfile()
{
	fflush(stdout);

	if (g_cpp.filename() != NULL)
#ifdef macintosh
		fprintf(stderr, "File \"%s\"; line %d # ", g_cpp.filename(), g_cpp.linenum());
#else
		fprintf(stderr, "%s:%d: ", base_name(g_cpp.filename()),
				g_cpp.linenum());
#endif
}

void
warn(Error e, ...)
{
	if (g_nowarnings)
		return;

	va_list ap;
	va_start(ap, e);

	printfile();

	if (g_warn2error)
	{
		fprintf(stderr, errorstr);
		err++;
	}
	else
		fprintf(stderr, warnstr);

	vfprintf(stderr, errors[e], ap);
	fprintf(stderr, ".\n");

	va_end(ap);
}

void
error(Error e, ...)
{
	printfile();
	fprintf(stderr, errorstr);
	err++;

	va_list ap;
	va_start(ap, e);
	vfprintf(stderr, errors[e], ap);
	va_end(ap);

	fprintf(stderr, ".\n");
}

void
fatal(Error e, ...)
{
	va_list ap;
	va_start(ap, e);

	printfile();
	fprintf(stderr, fatalstr);
	vfprintf(stderr, errors[e], ap);
	fprintf(stderr, "!\n");

	va_end(ap);
	exit(-1);
}

void
bug(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	printfile();
	fprintf(stderr, "BUG: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "?!?\n");

	va_end(ap);
	backtrace(TRUE);
	exit(-3);
}

void
dprintf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	printfile();
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void
eprintf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	printfile();
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	err++;
}


#if defined(__MWERKS__OFF) && !defined(DEBUG)

void *operator
new(size_t sz)
{
	void *p = malloc(sz);
	
	if (p == NULL)
		crash("operator new: Out of memory!");

	return p;
}

void operator
delete(void *p)
{
	if (p != NULL)
		free(p);
}

#endif


#ifdef DEBUG
extern "C" size_t blocksize(void *ptr);

#ifdef __MWERKS__
extern "C" void debugger(const char*);
#define crash debugger
#endif

void *
operator new(size_t sz)
{
	char *p = (char*)malloc(sz + 12);
	
	if (p == NULL)
		crash("operator new: Out of memory!");

	*(size_t*)p = sz;
	*(long*)(p + 4) = 0xFEEDFACE;
	*(long*)(p + 8 + sz) = 0xDEADBEEF;
	memset(p + 8, 0x86, sz);

	return p + 8;
}

void *
operator new(size_t sz, void *p)
{
	return p;
}

#if 0
void *
operator new[](size_t sz)
{
	return new char[sz];
}

void *
operator new[](size_t sz, void *p)
{
	return p;
}
#endif

void
operator delete(void *ptr)
{
	if (ptr == NULL)
		return;

	char *p = (char*)ptr - 8;
	
	if (p != NULL)
	{
		size_t sz = *(size_t*)p;

		if (*(long*)(p + 4) != 0xFEEDFACE)
			crash("operator delete: head guard trashed!");
		
		if (*(long*)(p + 8 + sz) != 0xDEADBEEF)
			crash("operator delete: tail guard trashed!");
		
		memset(p, 0x69, sz + 12);
		free(p);
	}
}

#if 0
void
operator delete[](void *ptr)
{
	delete (char*)ptr;
}
#endif

#endif
