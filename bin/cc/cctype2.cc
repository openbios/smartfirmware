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
// $Header: /u/cgt/cvs/bin/cc/cctype2.cc,v 1.26 1999/03/15 21:48:54 parag Exp $

#include "cpp.h"
#include "ccsym.h"
#include "cctype.h"
#include "cc.h"

// to maintain a pool of types, and thus minimize memory usage
class Typeptr
{
public:
	Type *_type;

	Typeptr(Type *t) { _type = t; }
	Typeptr(const Typeptr &t) { _type = t._type; }
	boolean operator==(const Typeptr &t)
			{ return _type == t._type || *_type == *t._type; }
};

static inline unsigned long typehash(Typeptr const &t)
{ return t._type->hash(); }

declare_pool(Type_pool, Typeptr);
implement_pool(Type_pool, Typeptr, typehash);

static Type_pool type_pool;

// put a type into the global type pool - gets really weird for some types
Type *
pool_type(Type *t)
{
	if (t->inpool())
		return t;

	t->inpool(TRUE);		// to make sure the hash comes out right
	Type_val tv = t->typeval();

	switch (tv)
	{
	case T_ARRAY:
		{
			Array_type *at = (Array_type*)t;

			// put the array type into the pool
			if (!at->type()->inpool())
				at->type(pool_type(at->type()));
		}

		break;

	case T_POINTER:
		{
			Pointer_type *pt = (Pointer_type*)t;

			// put the pointer type into the pool
			if (!pt->type()->inpool())
				pt->type(pool_type(pt->type()));
		}

		break;

	case T_FUNCTION:
		{
			Function_type *ft = (Function_type*)t;

			// put the function return type into the pool
			if (!ft->retval()->inpool())
				ft->retval(pool_type(ft->retval()));
		}

		break;

	default:
		break;
	}

	// now we can stick this type into our pool
	Typeptr tp(t);
	Typeptr &tt = type_pool.get(tp);

	if (tt._type != t)				// did we find a type already in the pool?
	{
		t->inpool(FALSE);		// so we can delete the temp type later

		if (tv == T_FUNCTION)
		{
			// we need to copy the latest arg names into the pool arg list,
			// in case the new names happen to be different - note that the
			// Function_type hash carefully avoids hashing the names of args

			Function_type *to = (Function_type*)tt._type;
			Function_type *from = (Function_type*)t;

			if (from->args() && to->args() &&
					from->args()->size() == to->args()->size())
				for (int i = 0; i < from->args()->size(); i++)
				{
					to->args()->elt(i).symbol(from->args()->elt(i).symbol());
					to->args()->elt(i).storage(from->args()->elt(i).storage());

					if (from->args()->elt(i).init())
						to->args()->elt(i).init(from->args()->elt(i).init());
				}
		}
	}

	return tt._type;
}

// see if "from" may be automatically casted into "to" - "ptr" is to
// handle checks for casts of pointers to const objects
boolean
valid_cast(Type *from, Type *to, boolean ptr)
{
	if (from == NULL || to == NULL)
		return FALSE;

	Type_val f = from->typeval();
	Type_val t = to->typeval();

	if (from == to)
		return TRUE;

	if (ptr)
	{
		if (g_strict_iso)
		{
			// ISO makes const/volatile almost completely worthless
			if (from->isconst() != to->isconst() ||
					from->isvolatile() != to->isvolatile())
				return FALSE;
		}
		else if ((from->isconst() && !to->isconst()) ||
				(from->isvolatile() && !to->isvolatile()))
			// ptr to qualified cannot be cast into ptr to non-qualified
			return FALSE;
	}

	switch (f)
	{
	case T_INT:
	case T_BITFIELD:
	case T_ENUM:
	case T_FLOAT:
		switch (t)
		{
		case T_INT:
		case T_BITFIELD:
		case T_ENUM:
		case T_FLOAT:

			// pointers to things better be the same things
			if (ptr)
			{
				Type *bf = base_type(from);
				Type *bt = base_type(to);

				if (bf != bt)
					return FALSE;
			}

			return TRUE;

		// zero int constants may be cast to pointers, but we cannot
		// handle this case here
		//case T_POINTER:
		default:
			break;
		}
		break;

	case T_POINTER:
		{
			Pointer_type *fp = (Pointer_type*)from;

			switch (t)
			{
			case T_POINTER:
				{
					Pointer_type *tp = (Pointer_type*)to;

					if (fp->type()->typeval() == T_VOID
							|| tp->type()->typeval() == T_VOID)
						return TRUE;

					return valid_cast(fp->type(), tp->type(), TRUE);
				}

			case T_ARRAY:
				{
					Array_type *ta = (Array_type*)to;

					// auto-cast of void* to anything is probably illegal ISO C
					if (fp->type()->typeval() == T_VOID)
						return TRUE;

					return valid_cast(fp->type(), ta->type(), TRUE);
				}

			case T_FUNCTION:
				if (fp->type()->typeval() == T_VOID)
					return TRUE;

				if (fp->type()->typeval() != T_FUNCTION)
					return FALSE;

				return equivalent(fp->type(), to);

			default:
				break;
			}
		}

		break;

	case T_ARRAY:
		{
			Array_type *fa = (Array_type*)from;

			switch (t)
			{
			case T_POINTER:
				{
					Pointer_type *tp = (Pointer_type*)to;

					if (tp->type()->typeval() == T_VOID)
						return TRUE;

					return valid_cast(fa->type(), tp->type(), TRUE);
				}

			case T_ARRAY:
				{
					Array_type *ta = (Array_type*)to;
					return valid_cast(fa->type(), ta->type(), TRUE);
				}

			default:
				break;
			}
		}

		break;

	case T_FUNCTION:
		if (t == T_POINTER)
		{
			to = ((Pointer_type*)to)->type();
			t = to->typeval();
		}

		if (t != T_FUNCTION)
			return FALSE;

		// just compare these function types directly
		return equivalent(from, to);

	// !ISO - struct*s may be auto-casted into unnamed substruct*s
	case T_STRUCT:
		if (t == T_STRUCT)
		{
			if (g_cplusplus)
			{
				// look for inheritance typecasts
				for (Struct_type *fs = (Struct_type*)from;
						fs != NULL; fs = fs->base())
					if (fs == to || equivalent(fs, to) ||
							base_type(fs) == base_type(to))
						return TRUE;
			}

			if (ptr)
			{
				Struct_type *fs = (Struct_type*)from;

				// if un-qualified types are the same, assume a match
				if (fs->info() == ((Struct_type*)to)->info())
					return TRUE;

				// look for "from" in the member list of "to"
				if (fs->memlist() != NULL && fs->members() != NULL)
					for (int i = 0; i < fs->memlist()->size(); i++)
						if (fs->memlist()->elt(i)->type() == to &&
							fs->memlist()->elt(i)->name()[0] == UNNAMED_CHAR)
								return TRUE;

				return FALSE;
			}
			else
				return base_type(from) == base_type(to);
		}

		break;

	default:
		break;
	}

	return FALSE;
}

// this sees if two types are the same, but also compares two function
// argument lists in the correct C way, unlike operator== which must
// compare them exactly to properly pool the elements
//
boolean
equivalent(Type *t1, Type *t2)
{
	if (t1 == t2)
		return TRUE;

	// both must have same const, volatile, and type values
	if (t1->isconst() != t2->isconst()
			|| t1->isvolatile() != t2->isvolatile()
			|| t1->typeval() != t2->typeval())
		return FALSE;

	if (t1->typeval() != T_FUNCTION)
		return *t1 == *t2;

	Function_type *f1 = (Function_type*)t1;
	Function_type *f2 = (Function_type*)t2;

	if (f1->varargs() != f2->varargs() || f1->functype() != f2->functype() ||
			!equivalent(f1->retval(), f2->retval()))
		return FALSE;

	if (f1->args() == NULL && f2->args() == NULL)
		return TRUE;

	// note that this compares parameter types - not their names
	if (f1->args() != NULL && f2->args() != NULL)
		return compare_args(f1->args(), f2->args());

	// must explicitly ask to do sloppy argument checks
	if (g_sloppy_args)
		return TRUE;

	if (!g_cplusplus && (f1->args() == NULL || f2->args() == NULL))
	{
		warn(FUNCTION_PROTOTYPE_MISMATCH);
		return TRUE;
	}

	// I like this check to behave like C++, ie very picky
	if (!g_strict_iso || f1->varargs() || f2->varargs())
		return FALSE;

	Param_arr *a = f1->args() == NULL ? f2->args() : f1->args();

	// args must auto-cast to default arg types (int/float) to be valid
	for (int i = 0; i < a->size(); i++)
		if (!valid_cast(a->elt(i).type(), g_int_type) &&
				!valid_cast(a->elt(i).type(), g_double_type))
			return FALSE;

	return TRUE;
}

// see if "from" is a sub-type of "to", especially for incomplete types
boolean
sub_type(Type *from, Type *to)
{
	if (from == to)
		return TRUE;

	Type_val fv = from->typeval();

	// both must have same const, volatile, and type values
	if (from->isconst() != to->isconst()
			|| from->isvolatile() != to->isvolatile()
			|| fv != to->typeval())
		return FALSE;

	switch (fv)
	{
	case T_ARRAY:
		{
			Array_type *fa = (Array_type*)from;
			Array_type *ta = (Array_type*)to;

			if (fa->complete() == ta->complete())
				return sub_type(fa->type(), ta->type());
		}

		break;

	case T_STRUCT:
		return !((Struct_type*)from)->complete()
				&& ((Struct_type*)to)->complete();

	case T_ENUM:
		return !((Enum_type*)from)->complete() && ((Enum_type*)to)->complete();

	case T_FUNCTION:
		return equivalent(from, to);

	case T_POINTER:
		return sub_type(((Pointer_type*)from)->type(),
				((Pointer_type*)to)->type());

	default:
		break;
	}

	return FALSE;
}

// return the base type of a type, with no type qualifiers
Type *
base_type(Type *t)
{
	if (t->isconst() || t->isvolatile())
	{
		Type *tmp = t->dup();
		tmp->isconst(FALSE);
		tmp->isvolatile(FALSE);
		t = pool_type(tmp);
		tmp->free();
	}

	return t;
}

// return the correct arithmetic conversion type of performing any
// arithmetic operation upon arithmetic types "t1" and "t2"
Type *
arith_conv(Type *t1, Type *t2)
{
	Type_val v1 = t1->typeval();
	Type_val v2 = t2->typeval();

	// zeros, enums, and bitfields are treated as ints

	if (v1 == T_ENUM)
	{
		t1 = g_int_type;
		v1 = T_INT;
	}
	else if (v1 == T_BITFIELD)
	{
		t1 = ((Bitfield_type*)t1)->sign() ? g_int_type : g_uint_type;
		v1 = T_INT;
	}
	else if (v1 != T_INT && v1 != T_FLOAT)
		bug("arith_conv(): expected integral or floating value");

	if (v2 == T_ENUM)
	{
		t2 = g_int_type;
		v2 = T_INT;
	}
	else if (v2 == T_BITFIELD)
	{
		t2 = ((Bitfield_type*)t2)->sign() ? g_int_type : g_uint_type;
		v2 = T_INT;
	}
	else if (v2 != T_INT && v2 != T_FLOAT)
		bug("arith_conv(): expected integral or floating value");

	// if either is a float, the result must be float
	if (v1 == T_FLOAT)
	{
		if (v2 == T_FLOAT)
		{
			// both are float, so result is the largest float
			int fw1 = ((Float_type*)t1)->width();
			int fw2 = ((Float_type*)t2)->width();

			return base_type((fw1 > fw2) ? t1 : t2);
		}

		return base_type(t1);
	}
	else if (v2 == T_FLOAT)
		return base_type(t2);

	// both types are ints
	int w1 = ((Int_type*)t1)->width();
	int w2 = ((Int_type*)t2)->width();
	boolean s1 = ((Int_type*)t1)->sign();
	boolean s2 = ((Int_type*)t2)->sign();

	// anything smaller than an "int" must be bumped up to "int"
	if (w1 < g_int_type->width())
	{
		t1 = g_int_type;
		w1 = g_int_type->width();
		s1 = TRUE;
	}

	if (w2 < g_int_type->width())
	{
		t2 = g_int_type;
		w2 = g_int_type->width();
		s1 = TRUE;
	}

	// if unsigned is equal to or wider than signed, return unsigned
	if (!s1 && s2 && (w1 >= w2))
		return base_type(t1);
	else if (s1 && !s2 && (w2 >= w1))
		return base_type(t2);

	// if both signed or both unsigned, or unsigned is smaller than signed,
	// return the widest
	return base_type((w1 > w2) ? t1 : t2);
}

/****
// compare two function parameter lists to see if their types match
boolean
compare_args(Param_arr *a1, Param_arr *a2)
{
	if (a1->size() != a2->size())
		return FALSE;

	for (int i = 0; i < a1->size(); i++)
		if (a1->elt(i)->storage() != a2->elt(i)->storage() ||
					a1->elt(i)->type() != a2->elt(i)->type() ||
					*a1->elt(i)->type() != *a2->elt(i)->type())
			return FALSE;
	
	return TRUE;
}
****/

// build up our type pool for builtin types
void
init_types()
{
	if (g_unknown_type != NULL)		// already initialized
		return;

	// unknown
	Type *t = new Type;
	g_unknown_type = pool_type(t);
	t->free();

	// void
	t = new Void_type;
	g_void_type = pool_type(t);
	t->free();

	// void*
	t = new Pointer_type(g_void_type);
	g_voidptr_type = pool_type(t);
	t->free();

	// do all the various integer classes

	IntegerClass *ic = new IntegerClass(g_size_char);

	// char
	t = new Int_type(ic, TRUE);
	g_char_type = (Int_type*)pool_type(t);
	t->free();

	// uchar
	t = new Int_type(ic, FALSE);
	g_uchar_type = (Int_type*)pool_type(t);
	t->free();

	// uchar
	t = new Int_type(ic, FALSE);
	g_uchar_type = (Int_type*)pool_type(t);
	t->free();

	if (g_size_short > g_size_char)
	{
		ic = new IntegerClass(g_size_short);

		// short
		t = new Int_type(ic, TRUE);
		g_short_type = (Int_type*)pool_type(t);
		t->free();

		// unsigned short
		t = new Int_type(ic, FALSE);
		g_ushort_type = (Int_type*)pool_type(t);
		t->free();
	}
	else
	{
		g_size_short = g_size_char;
		g_align_short = g_align_char;
		g_short_type = g_char_type;
		g_ushort_type = g_uchar_type;
	}

	if (g_size_int > g_size_short)
	{
		ic = new IntegerClass(g_size_int);

		// int
		t = new Int_type(ic, TRUE);
		g_int_type = (Int_type*)pool_type(t);
		t->free();

		//unsigned int
		t = new Int_type(ic, FALSE);
		g_uint_type = (Int_type*)pool_type(t);
		t->free();
	}
	else
	{
		g_size_int = g_size_short;
		g_align_int = g_align_short;
		g_int_type = g_short_type;
		g_uint_type = g_ushort_type;
	}

	if (g_size_long > g_size_int)
	{
		ic = new IntegerClass(g_size_long);

		// long
		t = new Int_type(ic, TRUE);
		g_long_type = (Int_type*)pool_type(t);
		t->free();

		// unsigned long
		t = new Int_type(ic, FALSE);
		g_ulong_type = (Int_type*)pool_type(t);
		t->free();
	}
	else
	{
		g_size_long = g_size_int;
		g_align_long = g_align_int;
		g_long_type = g_int_type;
		g_ulong_type = g_uint_type;
	}

	if (g_size_longlong > g_size_long)
	{
		ic = new IntegerClass(g_size_longlong);

		// long long
		t = new Int_type(ic, TRUE);
		g_longlong_type = (Int_type*)pool_type(t);
		t->free();

		// unsigned long long
		t = new Int_type(ic, FALSE);
		g_ulonglong_type = (Int_type*)pool_type(t);
		t->free();
	}
	else
	{
		g_size_longlong = g_size_long;
		g_align_longlong = g_align_long;
		g_longlong_type = g_long_type;
		g_ulonglong_type = g_ulong_type;
	}

	// now do all the float classes

	FloatClass *fc = new FloatClass(g_size_float_mantissa - 1,
			g_size_float_exponent);

	// float
	t = new Float_type(fc);
	g_float_type = (Float_type*)pool_type(t);
	t->free();

	if (g_size_shortdouble_mantissa > g_size_float_mantissa ||
			g_size_shortdouble_exponent > g_size_float_exponent)
	{
		fc = new FloatClass(g_size_shortdouble_mantissa - 1,
				g_size_shortdouble_exponent);

		// short double
		t = new Float_type(fc);
		g_shortdouble_type = (Float_type*)pool_type(t);
		t->free();
	}
	else
	{
		g_size_shortdouble_mantissa = g_size_float_mantissa;
		g_size_shortdouble_exponent = g_size_float_exponent;
		g_align_shortdouble = g_align_float;
		g_shortdouble_type = g_float_type;
	}

	if (g_size_double_mantissa > g_size_shortdouble_mantissa ||
			g_size_double_exponent > g_size_shortdouble_exponent)
	{
		fc = new FloatClass(g_size_double_mantissa - 1,
				g_size_double_exponent);

		// double
		t = new Float_type(fc);
		g_double_type = (Float_type*)pool_type(t);
		t->free();
	}
	else
	{
		g_size_double_mantissa = g_size_shortdouble_mantissa;
		g_size_double_exponent = g_size_shortdouble_exponent;
		g_align_double = g_align_shortdouble;
		g_double_type = g_shortdouble_type;
	}

	if (g_size_longdouble_mantissa > g_size_double_mantissa ||
			g_size_longdouble_exponent > g_size_double_exponent)
	{
		fc = new FloatClass(g_size_longdouble_mantissa - 1,
				g_size_longdouble_exponent);

		// long double
		t = new Float_type(fc);
		g_longdouble_type = (Float_type*)pool_type(t);
		t->free();
	}
	else
	{
		g_size_longdouble_mantissa = g_size_double_mantissa;
		g_size_longdouble_exponent = g_size_double_exponent;
		g_align_longdouble = g_align_double;
		g_longdouble_type = g_double_type;
	}

	// all types derived from the above go here

	// wchar_t for wide character representations
	if (g_size_wchar_t <= g_size_char)
		g_wchar_t_type = g_char_type;
	else if (g_size_wchar_t <= g_size_short)
		g_wchar_t_type = g_short_type;
	else if (g_size_wchar_t <= g_size_int)
		g_wchar_t_type = g_int_type;
	else if (g_size_wchar_t <= g_size_long)
		g_wchar_t_type = g_long_type;
	else if (g_size_wchar_t <= g_size_longlong)
		g_wchar_t_type = g_longlong_type;
	else
		bug("init_types(): wchar_t (%d) is too big", g_size_wchar_t);

	if (g_const_strings)
	{
		// string
		t = new Int_type(&g_char_type->ic(), TRUE);
		t->isconst(TRUE);
		g_string_type = (Int_type*)pool_type(t);
		t->free();

		// wstring
		t = new Int_type(&g_wchar_t_type->ic(), TRUE);
		t->isconst(TRUE);
		g_wstring_type = (Int_type*)pool_type(t);
		t->free();

		// Pascal string
		t = new Int_type(&g_char_type->ic(), FALSE);
		t->isconst(TRUE);
		g_pascal_string_type = (Int_type*)pool_type(t);
		t->free();
	}
	else
	{
		g_string_type = g_char_type;
		g_wstring_type = g_wchar_t_type;
		g_pascal_string_type = g_uchar_type;
	}

	// size_t for sizeof, array sizes, etc
	if (g_size_size_t <= g_size_char)
		g_size_t_type = g_uchar_type;
	else if (g_size_size_t <= g_size_short)
		g_size_t_type = g_ushort_type;
	else if (g_size_size_t <= g_size_int)
		g_size_t_type = g_uint_type;
	else if (g_size_size_t <= g_size_long)
		g_size_t_type = g_ulong_type;
	else if (g_size_size_t <= g_size_longlong)
		g_size_t_type = g_ulonglong_type;
	else
		bug("init_types(): size_t (%d) is too big", BITSPERLONG);

	// ptrdiff_t for subtracting two pointers
	if (g_size_pointer <= g_size_char)
		g_ptrdiff_t_type = g_char_type;
	else if (g_size_pointer <= g_size_short)
		g_ptrdiff_t_type = g_short_type;
	else if (g_size_pointer <= g_size_int)
		g_ptrdiff_t_type = g_int_type;
	else if (g_size_pointer <= g_size_long)
		g_ptrdiff_t_type = g_long_type;
	else if (g_size_pointer <= g_size_longlong)
		g_ptrdiff_t_type = g_longlong_type;
	else
		bug("init_types(): ptrdiff_t (%d) is too big", g_size_pointer);
	
	// minimum addressable size, as Integer for division
	g_min_addr_size = new Integer(g_size_t_type->ic(), g_min_addressable);
}
