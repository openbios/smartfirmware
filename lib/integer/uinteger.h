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


#ifndef __UINTEGER_H_
#define __UINTEGER_H_

#ifndef __INTEGER_H_
#include <integer.h>
#endif

class UnsignedInteger : private Integer
{
public:
    // constructors and destructors
    UnsignedInteger(IntegerClass &ic);			// Initialize to zero;
    UnsignedInteger(IntegerClass &ic, unsigned long x);	// Initialize from long
    UnsignedInteger(IntegerClass &ic, char const *s, char const **end = NULL,
	    int base = 10);		// Initialize from string
    UnsignedInteger(UnsignedInteger const &i);		// sameary
    UnsignedInteger(IntegerClass &ic, uLong *v);	// from vector
    UnsignedInteger(IntegerClass &ic, UnsignedInteger const &i); // convertion
    ~UnsignedInteger() { }

    // member functions
    IntegerClass &integerclass() const { return Integer::integerclass(); }
    void changetype(IntegerClass &ic) { Integer::changetype(ic); }
    int size() const { return Integer::size(); }
    uLong get(int i = 0) const { return Integer::get(i); }
    boolean isPos() const { return Integer::isPos(); }
    boolean isNeg() const { return Integer::isNeg(); }
    boolean isZero() const { return Integer::isZero(); }
    void complement() { Integer::complement(); }
    void neg() { Integer::neg(); }
    void assign(UnsignedInteger const &i) { Integer::assign(i); }
    void add(UnsignedInteger const &i, boolean *carry = NULL)
	    { Integer::add(i, carry); }
    void sub(UnsignedInteger const &i, boolean *borrow = NULL)
	    { Integer::sub(i, borrow); }
    void mult(UnsignedInteger const &i) { Integer::mult(i); }
    void umult(UnsignedInteger const &i) { Integer::umult(i); }
    void div(UnsignedInteger const &i) { Integer::div(i); }
    void udiv(UnsignedInteger const &i) { Integer::udiv(i); }
    void rem(UnsignedInteger const &i) { Integer::rem(i); }
    void urem(UnsignedInteger const &i) { Integer::urem(i); }
    void b_and(UnsignedInteger const &i) { Integer::b_and(i); }
    void b_or(UnsignedInteger const &i) { Integer::b_or(i); }
    void b_xor(UnsignedInteger const &i) { Integer::b_xor(i); }
    uLong lshift(int i) { return Integer::lshift(i); }
    uLong rshift(int i) { return Integer::rshift(i); }
    uLong urshift(int i) { return Integer::urshift(i); }
    int compare(UnsignedInteger const &i) const { return Integer::compare(i); }
    int ucompare(UnsignedInteger const &i) const
	    { return Integer::ucompare(i); }

    // assignment operators
    UnsignedInteger &operator=(UnsignedInteger const &i)
	    { assign(i); return *this; }
    UnsignedInteger &operator+=(UnsignedInteger const &i)
    	    { add(i); return *this; }
    UnsignedInteger &operator-=(UnsignedInteger const &i)
    	    { sub(i); return *this; }
    UnsignedInteger &operator*=(UnsignedInteger const &i)
    	    { umult(i); return *this; }
    UnsignedInteger &operator/=(UnsignedInteger const &i)
    	    { udiv(i); return *this; }
    UnsignedInteger &operator%=(UnsignedInteger const &i)
    	    { urem(i); return *this; }
    UnsignedInteger &operator&=(UnsignedInteger const &i)
    	    { b_and(i); return *this; }
    UnsignedInteger &operator|=(UnsignedInteger const &i)
    	    { b_or(i); return *this; }
    UnsignedInteger &operator^=(UnsignedInteger const &i)
    	    { b_xor(i); return *this; }
    UnsignedInteger &operator<<=(int i)
    	    { lshift(i); return *this; }
    UnsignedInteger &operator>>=(int i)
    	    { urshift(i); return *this; }

    // unary operators
    UnsignedInteger operator~() const
    	    { UnsignedInteger t = *this; t.complement(); return t; }
    UnsignedInteger operator-() const
    	    { UnsignedInteger t = *this; t.neg(); return t; }
    boolean operator!() const
    	    { return isZero(); }
    UnsignedInteger operator++()
    	    { UnsignedInteger t = *this; add(integerclass().one()); return t; }
    UnsignedInteger &operator--()
    	    { add(integerclass().minusone()); return *this; }

    // binary operators
    UnsignedInteger operator+(UnsignedInteger const &i) const
	    { UnsignedInteger t = *this; t.add(i); return t; }
    UnsignedInteger operator-(UnsignedInteger const &i) const
	    { UnsignedInteger t = *this; t.sub(i); return t; }
    UnsignedInteger operator*(UnsignedInteger const &i) const
	    { UnsignedInteger t = *this; t.umult(i); return t; }
    UnsignedInteger operator/(UnsignedInteger const &i) const
	    { UnsignedInteger t = *this; t.udiv(i); return t; }
    UnsignedInteger operator%(UnsignedInteger const &i) const
	    { UnsignedInteger t = *this; t.urem(i); return t; }
    UnsignedInteger operator&(UnsignedInteger const &i) const
	    { UnsignedInteger t = *this; t.b_and(i); return t; }
    UnsignedInteger operator|(UnsignedInteger const &i) const
	    { UnsignedInteger t = *this; t.b_or(i); return t; }
    UnsignedInteger operator^(UnsignedInteger const &i) const
	    { UnsignedInteger t = *this; t.b_xor(i); return t; }
    UnsignedInteger operator<<(int i) const
	    { UnsignedInteger t = *this; t.lshift(i); return t; }
    UnsignedInteger operator>>(int i) const
	    { UnsignedInteger t = *this; t.urshift(i); return t; }

    // comparison operators
    boolean operator==(UnsignedInteger const &i) const
    	    { return ucompare(i) == 0; }
    boolean operator!=(UnsignedInteger const &i) const
    	    { return ucompare(i) != 0; }
    boolean operator<(UnsignedInteger const &i) const
    	    { return ucompare(i) < 0; }
    boolean operator<=(UnsignedInteger const &i) const
    	    { return ucompare(i) <= 0; }
    boolean operator>(UnsignedInteger const &i) const
    	    { return ucompare(i) > 0; }
    boolean operator>=(UnsignedInteger const &i) const
    	    { return ucompare(i) >= 0; }
};

extern char const *uintegertostr(Integer const &, int base = 10);
extern boolean uintegertol(UnsignedInteger const &i, long &res);
extern boolean uintegertoul(UnsignedInteger const &i, unsigned long &res);
extern boolean ltouinteger(long v, UnsignedInteger &res);
extern boolean ultouinteger(unsigned long v, UnsignedInteger &res);

#ifdef __LONGLONG
extern boolean uintegertoll(UnsignedInteger const &i, long long &res);
extern boolean uintegertoull(UnsignedInteger const &i, unsigned long long &res);
extern boolean lltouinteger(long long ll, UnsignedInteger &res);
extern boolean ulltouinteger(unsigned long long ll, UnsignedInteger &res);
#endif

#endif // __UINTEGER_H_
