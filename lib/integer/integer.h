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


#ifndef __INTEGER_H_
#define __INTEGER_H_

#ifndef __STDLIBX_H_
#include <stdlibx.h>
#endif

// Constant Map
//
//
//	+---------------------------------+
// hind:|             |                   |
//	+---------------------------------+
//
//	 000000000000001111111111111111111	hmask
//       |             | 			hshift
//
//	+---------------------------------+
// lind:|                  |              |
//	+---------------------------------+
//
//	 111111111111111111100000000000000	lmask
//                         |             | 	lshift
//
//	+---------------------------------+
// sind:|                | |              |
//	+---------------------------------+
//
//	 000000000000000001000000000000000	smask
//                        |              | 	sshift
//
//	 111111111111111110000000000000000	sextmask
//

#ifndef _INTEGER_MINLONGS
#define _INTEGER_MINLONGS 2
#endif // _INTEGER_MINLONGS

class Integer;

class IntegerClass
{
    int _bits;
    int _size;
    Integer *_one;
    Integer *_zero;
    Integer *_minusone;
    Integer *_ten;
    Integer *_min;
    Integer *_max;
    Integer *_temps;
    int _tempcnt;
    int _hind;
    int _hshift;
    uLong _hmask;
    uLong _lmask;
    int _sind;
    int _sshift;
    uLong _smask;
    uLong _sextmask;
    uLong *_freelist;
    void Init(int bits);
protected:
    friend class Integer;
    uLong *get();
    void put(uLong *p);
    Integer &tmp(int i = 0);
public:
    IntegerClass(int bits);
    ~IntegerClass();
    int bits() { return _bits; }
    int size() { return _size; }	// size in longs
    boolean alloc() { return _size > _INTEGER_MINLONGS; }

    // constant properties of integers for this class
    int hind() { return _hind; }
    int hshift() { return _hshift; }
    uLong hmask() { return _hmask; }
    int lind() { return 0; }
    int lshift() { return 0; }
    uLong lmask() { return _lmask; }

    int sind() { return _sind; }
    int sshift() { return _sshift; }
    uLong smask() { return _smask; }
    uLong sextmask() { return _sextmask; }

    // constant integer values for this class
    Integer &zero() { return *_zero; }
    Integer &one() { return *_one; }
    Integer &minusone() { return *_minusone; }
    Integer &ten() { return *_ten; }
    Integer &min() { return *_min; }
    Integer &max() { return *_max; }
};

class Integer
{
    IntegerClass *_ic;
    uLong *_vec;
    uLong _buf[_INTEGER_MINLONGS];

    friend class IntegerClass;
    void Init(IntegerClass &ic);
    void convert(Integer const &i);
    void divide(Integer const &i, Integer &q, Integer &r);
public:
    // constructors and destructors
    Integer(IntegerClass &ic);		// Initialize to zero;
    Integer(IntegerClass &ic, long x);	// Initialize from integer
    Integer(IntegerClass &ic, char const *s, char const **end = NULL,
	    int base = 10);		// Initialize from string
    Integer(Integer const &i);				// sameary
    Integer(IntegerClass &ic, uLong *v);		// from vector
    Integer(IntegerClass &ic, Integer const &i);	// Type convertion
    ~Integer();

    // member functions
    void makezero();
    void makeone();
    void makemin();
    void makemax();
    IntegerClass &integerclass() const { return *_ic; }
    void changetype(IntegerClass &ic);
    int size() const { return _ic->size(); }
    uLong get(int i = 0) const
	    { return (i < 0 || i >= _ic->size()) ? 0UL : _vec[i]; }
    boolean isPos() const;
    boolean isNeg() const;
    boolean isZero() const;
    void complement();
    void neg();
    void assign(Integer const &i);
    void add(Integer const &i, boolean *carry = NULL);
    void sub(Integer const &i, boolean *borrow = NULL);
    void mult(Integer const &i);
    void umult(Integer const &i);
    void div(Integer const &i);
    void udiv(Integer const &i);
    void rem(Integer const &i);
    void urem(Integer const &i);
    void b_and(Integer const &i);
    void b_or(Integer const &i);
    void b_xor(Integer const &i);
    uLong lshift(int i);
    uLong rshift(int i);
    uLong urshift(int i);
    int compare(Integer const &i) const;
    int ucompare(Integer const &i) const;

    // assignment operators
    Integer &operator=(Integer const &i)
	    { assign(i); return *this; }
    Integer &operator+=(Integer const &i)
    	    { add(i); return *this; }
    Integer &operator-=(Integer const &i)
    	    { sub(i); return *this; }
    Integer &operator*=(Integer const &i)
    	    { mult(i); return *this; }
    Integer &operator/=(Integer const &i)
    	    { div(i); return *this; }
    Integer &operator%=(Integer const &i)
    	    { rem(i); return *this; }
    Integer &operator&=(Integer const &i)
    	    { b_and(i); return *this; }
    Integer &operator|=(Integer const &i)
    	    { b_or(i); return *this; }
    Integer &operator^=(Integer const &i)
    	    { b_xor(i); return *this; }
    Integer &operator<<=(int i)
    	    { lshift(i); return *this; }
    Integer &operator>>=(int i)
    	    { rshift(i); return *this; }

    // unary operators
    Integer operator~() const
    	    { Integer t = *this; t.complement(); return t; }
    Integer operator-() const
    	    { Integer t = *this; t.neg(); return t; }
    boolean operator!() const
    	    { return isZero(); }
    Integer &operator++()	// pre-inc
    	    { add(_ic->one()); return *this; }
    Integer operator++(int)	// post-inc
    	    { Integer t = *this; add(_ic->one()); return t; }
    Integer &operator--()	// pre-dec
    	    { add(_ic->minusone()); return *this; }
    Integer operator--(int)	// post dec
    	    { Integer t = *this; add(_ic->minusone()); return t; }

    // binary operators
    Integer operator+(Integer const &i) const
	    { Integer t = *this; t.add(i); return t; }
    Integer operator-(Integer const &i) const
	    { Integer t = *this; t.sub(i); return t; }
    Integer operator*(Integer const &i) const
	    { Integer t = *this; t.mult(i); return t; }
    Integer operator/(Integer const &i) const
	    { Integer t = *this; t.div(i); return t; }
    Integer operator%(Integer const &i) const
	    { Integer t = *this; t.rem(i); return t; }
    Integer operator&(Integer const &i) const
	    { Integer t = *this; t.b_and(i); return t; }
    Integer operator|(Integer const &i) const
	    { Integer t = *this; t.b_or(i); return t; }
    Integer operator^(Integer const &i) const
	    { Integer t = *this; t.b_xor(i); return t; }
    Integer operator<<(int i) const
	    { Integer t = *this; t.lshift(i); return t; }
    Integer operator>>(int i) const
	    { Integer t = *this; t.rshift(i); return t; }

    // comparison operators
    boolean operator==(Integer const &i) const
    	    { return compare(i) == 0; }
    boolean operator!=(Integer const &i) const
    	    { return compare(i) != 0; }
    boolean operator<(Integer const &i) const
    	    { return compare(i) < 0; }
    boolean operator<=(Integer const &i) const
    	    { return compare(i) <= 0; }
    boolean operator>(Integer const &i) const
    	    { return compare(i) > 0; }
    boolean operator>=(Integer const &i) const
    	    { return compare(i) >= 0; }
};

// standard integer classes
extern IntegerClass icchar;
extern IntegerClass icshort;
extern IntegerClass icint;
extern IntegerClass iclong;
extern IntegerClass iclonglong;

// integer exceptions
#ifdef __EXCEPTION_H_
extern Exception integer_type_mismatch;
extern Exception integer_division_by_zero;
#endif

/* negative base means to return a signed quantity if i is negative */
extern char const *integertostr(Integer const &i, int base = -10);
extern int bitcount(Integer const &i);
extern int bits(Integer const &i);
extern int findhighestbit(Integer const &i);
extern int findlowestbit(Integer const &i);

extern boolean integertol(Integer const &i, long &res);
extern boolean integertoul(Integer const &i, unsigned long &res);
extern boolean ltointeger(long v, Integer &res);
extern boolean ultointeger(unsigned long v, Integer &res);

#ifdef __LONGLONG
extern boolean integertoll(Integer const &i, long long &res);
extern boolean integertoull(Integer const &i, unsigned long long &res);
extern boolean lltointeger(long long ll, Integer &res);
extern boolean ulltointeger(unsigned long long ll,
	Integer &res);
#endif

#endif // __INTEGER_H_
