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
// $Header: /u/cgt/cvs/bin/cc/ccstor.cc,v 1.20 1999/03/15 21:48:51 parag Exp $

#include "cpp.h"
#include "cc.h"
#include "cctype.h"
#include "ccsym.h"
#include "ccnode.h"
#include <ctype.h>
#include <stdiox.h>

void
merge_flags(unsigned short &flags, int f)
{
	if (flags == S_UNKNOWN || f == S_UNKNOWN)
		flags = S_UNKNOWN;

	// handle "long long" type
	if ((flags & S_LONG) && (f & S_LONG))
	{
		if (g_strict_iso)
			warn(LONGLONG_IS_NOT_ISO);

		flags &= ~S_LONG;
		f &= ~S_LONG;

		if (flags & S_LONGLONG)
			f |= S_LONGLONG;
		else
			flags |= S_LONGLONG;
	}

	if (flags & f & S_QUALMASK)
		warn(DUPLICATE_TYPE_QUALIFIERS);

	if (flags & f & S_SPECMASK)
		warn(DUPLICATE_TYPE_SPECIFIERS);

	if (flags & f & S_FUNCMASK)
		warn(DUPLICATE_FUNC_TYPE_SPECIFIERS);

	flags |= f;

	if (((flags & S_SHORT) && (flags & S_LONG)) ||
			((flags & S_SHORT) && (flags & S_LONGLONG)) ||
			((flags & S_LONG) && (flags & S_LONGLONG)) ||
			((flags & S_SIGNED) && (flags & S_UNSIGNED)))
	{
		error(INCOMPATIBLE_TYPE_ELEMENTS);
		flags = S_UNKNOWN;
	}

/*****
	if ((flags & S_FRIEND) && (g_curr_scope->symbol() == NULL ||
			!g_curr_scope->symbol()->type()->issu()))
	{
		error(FRIEND_OUTSIDE_OF_CLASS);
		flags = S_UNKNOWN;
	}
*****/
}

void
Storage_thing::merge_basic(int b)
{
	if (basic == UNKNOWN || b == UNKNOWN)
		basic = UNKNOWN;
	else if (basic && b)
		error(INCOMPATIBLE_TYPE_ELEMENTS);
	else
		basic = b;
}

void
Storage_thing::merge_storage(int s)
{
	if (storage == UNKNOWN || s == UNKNOWN)
		storage = UNKNOWN;
	else if (storage == NONE)
		storage = s;
	else if (storage == s)
		warn(DUPLICATE_STORAGE_CLASSES);
	else
	{
		error(TOO_MANY_STORAGE_CLASSES);
		storage = UNKNOWN;
	}
}

void
Storage_thing::make_type(boolean bitfieldok)
{
	if (basic == UNKNOWN || flags == S_UNKNOWN || storage == UNKNOWN)
		return;

	if (type == NULL)
	{
		// C allows stuff such as "long foo;" or "unsigned bar;"
		if (basic == NONE)
			basic = INT;

		switch (basic)
		{
		// signed/unsigned short/long/longlong int
		case INT:
			if (flags & S_SHORT)
				type = flags & S_UNSIGNED ? g_ushort_type : g_short_type;
			else if (flags & S_LONG)
				type = flags & S_UNSIGNED ? g_ulong_type : g_long_type;
			else if (flags & S_LONGLONG)
				type = flags & S_UNSIGNED ? g_ulonglong_type : g_longlong_type;
			else
				type = flags & S_UNSIGNED ? g_uint_type : g_int_type;

			break;

		// signed/unsigned char
		case CHAR:
			if (flags & (S_SHORT | S_LONG | S_LONGLONG))
				error(SHORT_LONG_MEANINGLESS_FOR_CHAR);
			else
				type = flags & S_UNSIGNED ? g_uchar_type : g_char_type;

			break;

		// no specifiers are allowed for float
		case FLOAT:
			if (flags & S_SPECMASK)
			{
				error(SPECIFIERS_MEANINGLESS_FOR_FLOAT);
				return;
			}
			else
				type = g_float_type;

			break;

		// only double, short double, or long double is legal
		case DOUBLE:
			if (flags & S_LONG)
				type = g_longdouble_type;
			else if (flags & S_SHORT)
			{
				if (g_strict_iso)
					warn(SHORTDOUBLE_IS_NOT_ISO);

				type = g_shortdouble_type;
			}
			else if (flags & S_SPECMASK)
			{
				error(SPECIFIERS_MEANINGLESS_FOR_FLOAT);
				return;
			}
			else
				type = g_double_type;

			break;

		// nothing is valid for void
		case VOID:
			if (flags & S_SPECMASK)
			{
				error(SPECIFIERS_MEANINGLESS_FOR_VOID);
				return;
			}
			else
				type = g_void_type;

			break;

		default:
			bug("Storage_thing::make_type(): illegal basic type %d", basic);
			return;
		}
	}

	// set const/volatile flags for this type
	if (type->inpool())
	{
		// this type is already in the pool, so we need create a new one
		// with the same basic type, but different const/volatile-ness
		boolean c = (flags & S_CONST) ? TRUE : FALSE;
		boolean v = (flags & S_VOLATILE) ? TRUE : FALSE;

		if ((c && !type->isconst()) || (v && !type->isvolatile()))
		{
			Type *t = type->dup();
			t->isconst(c);
			t->isvolatile(v);
			type = t;
		}
	}
	else
	{
		type->isconst(flags & S_CONST);
		type->isvolatile(flags & S_VOLATILE);
	}

	if (type->isfunc() &&
			((flags & (S_PASCAL | S_FORTRAN)) || g_default_linkage))
	{
		if (type->inpool())
			type = type->dup();

		Function_type *ft = (Function_type*)type;

		if (flags & S_PASCAL)
		{
			if (ft->functype() != NONE && ft->functype() != PASCAL)
				error(INCOMPATIBLE_TYPE_ELEMENTS);

			ft->functype(PASCAL);
		}
		else if (flags & S_FORTRAN)
		{
			if (ft->functype() != NONE && ft->functype() != FORTRAN)
				error(INCOMPATIBLE_TYPE_ELEMENTS);

			ft->functype(FORTRAN);
		}
		else if (g_default_linkage)
		{
			if (ft->functype() != NONE && ft->functype() != g_default_linkage)
				error(INCOMPATIBLE_TYPE_ELEMENTS);

			ft->functype(g_default_linkage);
		}
	}

	// handle bitfields here
	if (bitfield >= 0)
	{
		if (!bitfieldok)
		{
			error(BITFIELD_ALLOWED_ONLY_IN_STRUCTS);
			return;
		}

		if (type->typeval() == T_INT)
		{
			if (g_strict_iso && !g_cplusplus &&
					(type == g_int_type || type == g_uint_type))
				warn(ISO_BITFIELD_MUST_BE_INT);

			if (bitfield > ((Int_type*)type)->width())
			{
				error(BITFIELD_TOO_LARGE, bitfield);
				return;
			}

			Type *t = new Bitfield_type(bitfield, (Int_type*)type);
			type = pool_type(t);
			t->free();
		}
		else
		{
			error(BITFIELD_MUST_BE_INTEGRAL);
			return;
		}
	}

	// This frees any types in the original "type" that are not in the pool
	// and sets us up to return a proper type for chained declarations.
	Type *t = pool_type(type);
	type->free();
	type = t;
	basic = NONE;
	flags = S_NONE | (flags & (S_REFERENCE | S_INLINE));

	if (scope == NULL || !g_cplusplus)
		scope = g_curr_scope;
}

Symbol*
Storage_thing::do_declaration()
{
	if (sym == NULL || type == NULL)
	{
		error(MUST_DECLARE_A_VARIABLE_NAME);
		return NULL;
	}

	boolean isinline = (flags & S_INLINE) ? TRUE : FALSE;

	if (flags & S_REFERENCE)
		merge_storage('&');

	make_type();

	if (type == g_void_type && storage != TYPEDEF)
	{
		error(CANNOT_DECLARE_VOID_TYPE, sym);
		return NULL;
	}

	Symbol *s;

	if (scope->symtab().find(sym))
	{
		// the symbol already exists in the scope - check this declaration
		// to see if it is a subtype of its earlier type or it is an error
		s = &scope->symtab()();

		if (scope != g_file_scope)
		{
			error(SYMBOL_ALREADY_DEFINED, sym, s->line());
			return NULL;
		}

		if (s->type() != type)
			// s->type will be NULL only for parameter declarations
			if (s->type() == NULL || sub_type(s->type(), type))
				s->type(type);
			else if (!sub_type(type, s->type()))
			{
				if (g_cplusplus)
				{
					if (!s->type()->isanyfunc() ||
							!type->isfunc() || s->add_overload(type) == NULL)
					{
						error(INCOMPATIBLE_TYPES_FOR_IDENT, sym,
								s->line(), s->file());
					}
				}
				else
				{
					if (s->type()->isfunc() && type->isfunc() &&
							(((Function_type*)s->type())->args() == NULL ||
								((Function_type*)type)->args() == NULL))
						warn(INCOMPATIBLE_TYPES_FOR_IDENT, sym,
								s->line(), s->file());
					else
						error(INCOMPATIBLE_TYPES_FOR_IDENT, sym,
								s->line(), s->file());
				}
			}

		if (s->storage() == EXTERN && storage != EXTERN && storage != '&')
			s->storage(storage);				// not an extern any longer
	}
	else
	{
		// the symbol is not in the scope, so add it
		scope->symtab().insert(sym);
		s = &scope->symtab()();
		s->type(type);
		s->line(g_cpp.linenum());
		s->file(g_cpp.filename());
		// determines if this name is a typedef or a reference
		s->storage(storage);
		s->scope(scope);
	}

	if (isinline)
		s->is_inline(TRUE);

	Type_val tv = type->typeval();

	// now do some sanity checks depending on the declared type
	if (tv == T_FUNCTION)
	{
		if (storage != EXTERN && storage != STATIC &&
				storage != TYPEDEF && storage != NONE)
		{
			error(BAD_STORAGE_FOR_FUNCTION, sym);
			return NULL;
		}
	}
	else if ((tv == T_STRUCT || tv == T_UNION || tv == T_ENUM) &&
			storage != EXTERN && storage != TYPEDEF && storage &&
			!type->complete())
	{
		error(CANNOT_DECLARE_INCOMPLETE_VAR, sym);
		return NULL;
	}
	else if (storage == REGISTER)
	{
		if (type->typeval() == T_ARRAY || s->scope() == g_file_scope)
		{
			warn(CANNOT_MAKE_REGISTER_SYMBOL, sym);
			s->storage(NONE);
		}
	}
	else if (storage == AUTO && s->scope() == g_file_scope)
	{
		error(CANNOT_AUTO_A_GLOBAL, sym);
		return NULL;
	}

	return s;
}

void
Storage_thing::do_member_decl(Struct_type *stype, int access)
{
	if (flags & S_FRIEND)
	{
		if (sym != NULL)
		{
			Symbol *s = do_declaration();

			if (s != NULL && !s->type()->isanyfunc())
				error(BAD_FRIEND_FOR_CLASS, stype->name());
		}
		else
		{
			symbol = stype->scope()->symbol();

			if (!symbol->type()->issu())
				error(BAD_FRIEND_FOR_CLASS, symbol->name());
		}

		stype->add_friend(symbol);
		return;
	}

	if (sym == NULL && bitfield < 0)
	{
		error(EXPECTED_BITFIELD_OR_DECL);
		return;
	}
	else if (sym != NULL && bitfield == 0)
	{
		error(ILLEGAL_VALUE_FOR_BITFIELD);
		return;
	}

	boolean isinline = (flags & S_INLINE) ? TRUE : FALSE;

	if (flags & S_REFERENCE)
		merge_storage('&');

	make_type(TRUE);

	if (type == NULL)
		return;

	if (type == g_void_type && storage != TYPEDEF)
	{
		error(CANNOT_DECLARE_VOID_TYPE, sym);
		return;
	}
	else if (g_cplusplus)
	{
		if ((storage != NONE && storage != STATIC && storage != '&') ||
				(!type->isanyfunc() && isinline))
		{
			error(BOGUS_STORAGE_FOR_MEMBER, sym);
			return;
		}

		// mark this as a member function
		if (type->isfunc())
			((Function_type*)type)->classname(stype->symbol());
	}
	else if (type->isfunc())
	{
		error(BOGUS_TYPE_FOR_MEMBER, sym);
		return;
	}
	else if (storage != NONE)
	{
		error(BOGUS_STORAGE_FOR_MEMBER, sym);
		return;
	}

	if (sym == NULL)		// everything should have a name
		sym = make_unnamed_name();

	// assume the top-most element is indeed a struct/union
	stype->add(sym, type, g_cpp.linenum(), storage, access, isinline);
}

void
do_unnamed_substruct(Type *stype, Type *ptype)
{
	// just a declaration for a class so ignore it
	if (g_cplusplus && stype->isclass())
		return;

	if (g_strict_iso)
		warn(UNNAMED_SUBSTRUCT_IS_NOT_ISO);

	if (!ptype->issu() || !stype->issu())
	{
		error(ONLY_UNNAMED_SUBSTRUCTS_ALLOWED);
		return;
	}

	Struct_type *substruct = (Struct_type*)stype;
	Struct_type *parent = (Struct_type*)ptype;

	if (!substruct->complete())
	{
		error(CANNOT_MAKE_INCOMPLETE_UNNAMED);
		return;
	}

	// we add this unnamed substruct into its parent with a bogus name, which
	// should also add in all its subelements into the parent
	const char *id = make_unnamed_name();
	parent->add(id, substruct, g_cpp.linenum());
}

static Type*
new_struct_type(int stok, Symbol *sym)
{
	Struct_type *stype;

	switch (stok)
	{
	case STRUCT:
		stype = new Struct_type();
		break;

	case UNION:
		stype = new Union_type();
		break;

	case ENUM:
		stype = new Enum_type();
		break;

#ifdef CLASS
	case CLASS:
		stype = new Struct_type();
		stype->is_class(TRUE);
		stype->scope()->symbol(sym);
		break;
#endif

	default:
		bug("do_sue_declaration: unexpected token %s", w_tokenname(stok));
		return NULL;
	}

	//stype->name(id);
	stype->symbol(sym);
	Type *ret = pool_type(stype);
	stype->free();
	return ret;
}

static boolean
check_struct_type(Type *st, int stok)
{
	switch (st->typeval())
	{
	case T_STRUCT:
#ifdef CLASS
		return stok == STRUCT || stok == CLASS;
#else
		return stok == STRUCT;
#endif

	case T_UNION:
		return stok == UNION;

	case T_ENUM:
		return stok == ENUM;

	default:
		break;
	}

	return FALSE;
}

static void
do_cxx_typedef(Scope *scope, const char *id, Type *type)
{
	if (!g_cplusplus)
		return;

	if (!scope->symtab().find(id))
	{
		// this typedef is not in the scope, so add it
		scope->symtab().insert(id);
		Symbol *s = &scope->symtab()();
		s->type(type);
		s->line(g_cpp.linenum());
		s->file(g_cpp.filename());
		s->storage(TYPEDEF);
		s->scope(scope);
	}
}

Struct_type *
do_sue_definition(int stok, const char *id)
{
	if (g_curr_scope->suetypes().find(id) &&
			((Struct_type*)g_curr_scope->suetypes()().type())->complete())
	{
		error(STRUCT_ALREADY_DEFINED, id, g_curr_scope->suetypes()().line());
		return (Struct_type*)g_curr_scope->suetypes()().type();
	}

	Symbol &s = g_curr_scope->suetypes()[id];
	Struct_type *stype;

	if (s.type() == NULL)
		s.type(new_struct_type(stok, &s));

	stype = (Struct_type*)s.type();

	if (!check_struct_type(stype, stok))
		error(STRUCT_ALREADY_DEFINED, s.name(), s.line());
	else
	{
		stype->complete(TRUE);
		s.scope(stype->scope());
		s.file(g_cpp.filename());
		s.line(g_cpp.linenum());
	}

	if (g_cplusplus && (stype->is_class() || stype->isenum()))
		do_cxx_typedef(g_curr_scope, id, stype);

	return stype;
}

Struct_type *
do_sue_declaration(int stok, const char *id)
{
	Symbol *sym = g_curr_scope->lookup_sue(id);

	if (sym == NULL || !check_struct_type(sym->type(), stok))
	{
		Symbol &s = sym ? *sym : g_curr_scope->suetypes()[id];

		if (s.type() == NULL)
		{
			s.type(new_struct_type(stok, &s));
			s.scope(((Struct_type*)s.type())->scope());
			s.file(g_cpp.filename());
			s.line(g_cpp.linenum());

			//if (g_cplusplus && (s.type()->isclass() || s.type()->isenum()))
			if (g_cplusplus)
				do_cxx_typedef(g_curr_scope, id, s.type());
		}
		else if (!check_struct_type(s.type(), stok))
			error(STRUCT_ALREADY_DEFINED, s.name(), s.line());

		return (Struct_type*)s.type();
	}
	else
		return (Struct_type*)sym->type();
}
