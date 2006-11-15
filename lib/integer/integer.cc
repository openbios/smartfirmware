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


#include <stdlib.h>
#include <stdlibx.h>
#include <ctype.h>
#include "integer.h"

IntegerClass icchar(sizeof (char) * 8);
IntegerClass icshort(sizeof (short) * 8);
IntegerClass icint(sizeof (int) * 8);
IntegerClass iclong(sizeof (long) * 8);
#ifdef __LONGLONG
IntegerClass iclonglong(sizeof (long long) * 8);
#else
IntegerClass iclonglong(sizeof (long) * 8 * 2);
#endif

void
IntegerClass::Init(int bits)
{
    _bits = bits;
    int i = (bits - 1) % BITSPERLONG;
    _hshift = (BITSPERLONG - i) % BITSPERLONG;
    _hind = (bits - 2) / BITSPERLONG;
    _hmask = (1UL << i) - 1;

    if (_hmask == 0 && bits != 1)
    	_hmask = ~0UL;

    if (_hind)
	_lmask = ~0UL;
    else
	_lmask = _hmask;

    _sshift = i;
    _sind = (bits - 1) / BITSPERLONG;
    _smask = 1UL << i;
    _sextmask = -(_smask << 1);
}

IntegerClass::IntegerClass(int bits)
{
    _size = (bits + BITSPERLONG - 1) / BITSPERLONG;
    _freelist = NULL;
    _temps = NULL;
    _tempcnt = 0;
    Init(bits);
    _zero = new Integer(*this);
    _one = new Integer(*this, 1);
    _minusone = new Integer(*this, -1);
    _ten = new Integer(*this, 10);
    _min = new Integer(*this);
    _max = new Integer(*this);
    _min->makemin();
    _max->makemax();
}

IntegerClass::~IntegerClass()
{
    delete _zero;
    delete _one;
    delete _minusone;
    delete _ten;

    while (_freelist != NULL)
    {
	uLong *t = _freelist;
	_freelist = *(uLong **)_freelist;
	delete[/*_size*/] t;
    }

    if (_temps)
	delete[/*_tempcnt*/] _temps;
}

uLong *
IntegerClass::get()
{
    if (_freelist == NULL)
	return new uLong[_size];

    uLong *t = _freelist;
    _freelist = *(uLong **)_freelist;
    return t;
}

void
IntegerClass::put(uLong *p)
{
    *(uLong **)p = _freelist;
    _freelist = p;
}

static IntegerClass *gic = NULL;

class _Integer : public Integer
{
public:
    _Integer() : Integer(*gic) { }
};

Integer &
IntegerClass::tmp(int i)
{
    if (i < 0)
	return *(Integer *)NULL;

    if (i >= _tempcnt)
    {
	gic = this;
	Integer *ntemps = new _Integer[i + 1];

	if (_temps != NULL)
	{
	    for (int i = 0; i < _tempcnt; i++)
	    	ntemps[i] = _temps[i];

	    delete[/*_tempcnt*/] _temps;
	}

	_temps = ntemps;
	_tempcnt = i + 1;
    }

    return _temps[i];
}

void
Integer::Init(IntegerClass &ic)
{
    _ic = &ic;

    if (ic.alloc())
	_vec = ic.get();
    else
	_vec = _buf;
}

Integer::Integer(IntegerClass &ic, uLong *v)
{
    Init(ic);

    for (int i = 0; i < ic.size(); i++)
	_vec[i] = v[i];
}

Integer::Integer(IntegerClass &ic)
{
    Init(ic);

    for (int i = 0; i < ic.size(); i++)
	_vec[i] = 0UL;
}

Integer::Integer(IntegerClass &ic, long x)
{
    Init(ic);

    for (int i = 0; i < ic.size(); i++)
    	_vec[i] = 0UL;

    if (x == 0)
	return;

    if (x < 0)
	_vec[0] = (uLong)-x;
    else
	_vec[0] = (uLong)x;

    if (ic.hind() == 0)
    	_vec[0] &= ic.hmask();

    if (x < 0)
	neg();
}

Integer::Integer(IntegerClass &ic, char const *s, char const **end, int base)
{
    boolean neg = FALSE;
    Integer &t = ic.tmp(1);

    while (isspace(*s))
    	s++;

    if ((*s == '+' || *s == '-') && isdigit(s[1]))
    {
	if (*s == '-')
	    neg = TRUE;

	s++;
    }

    t = ic.zero();
    uLong tmp = 0UL;
    uLong limit = ~0UL / (unsigned long)base;
    uLong f = 1;
    int lb = findlowestbit(base);
    int shift = (((1UL << lb) == (unsigned)base) ? lb : -1);

    for (; isxdigit(*s); s++)
    {
    	if (f > limit)
	{
	    t *= Integer(ic, f);
	    t += Integer(ic, tmp);
	    tmp = 0UL;
	    f = 1;
	}

	int dig;

	if (isdigit(*s))
	    dig = *s - '0';
	else if (islower(*s))
	    dig = *s - 'a' + 10;
	else if (isupper(*s))
	    dig = *s - 'A' + 10;
	else
	    break;

	if (dig >= base)
	    break;

	if (base == 10)
	{
	    tmp = (tmp << 3) + (tmp << 1);
	    f = (f << 3) + (f << 1);
	}
	else if (shift != -1)
	{
	    tmp <<= shift;
	    f <<= shift;
	}
	else
	{
	    tmp *= base;
	    f *= base;
	}

	tmp += dig;
    }

    if (f > 1)
    {
    	if (t.isZero())
	    t = Integer(ic, tmp);
	else
	{
	    t *= Integer(ic, f);
	    t += Integer(ic, tmp);
	}
    }

    if (neg)
    	t.neg();

    if (end != NULL)
	*end = s;

    Init(ic);
    *this = t;
}

Integer::Integer(Integer const &i)
{
    Init(*i._ic);

    for (int j = 0; j < _ic->size(); j++)
	_vec[j] = i._vec[j];
};

Integer::Integer(IntegerClass &ic, Integer const &i)
{
    Init(ic);

    if (&ic == i._ic || ic.bits() == i._ic->bits())
    {
	for (int j= 0; j < ic.size(); j++)
	    _vec[j] = i._vec[j];

	return;
    }

    convert(i);
}

Integer::~Integer()
{
    if (_ic->alloc())
	_ic->put(_vec);
}

// What do we do now?  Have to type convert!
// truncate if ic is smaller, sign extend if ic is larger

void
Integer::convert(Integer const &i)
{
    if (_ic->bits() <= i._ic->bits())
    {
	for (int j = 0; j < _ic->size(); j++)
	    _vec[j] = i._vec[j];

	// truncate
	_vec[_ic->sind()] &= ~_ic->sextmask();
	return;
    }
    
    int j;

    for (j = 0; j < i._ic->size(); j++)
	_vec[j] = i._vec[j];

    if (_vec[i._ic->sind()] & i._ic->smask())
    {
	_vec[i._ic->sind()] |= i._ic->sextmask();

	for (; j < _ic->size(); j++)
	    _vec[j] = ~0UL;

	return;
    }

    for (; j < _ic->size(); j++)
	_vec[j] = 0UL;
}

void
Integer::assign(Integer const &i)
{
    if (_ic != i._ic)
    {
	convert(i);
	return;
    }

    int sz = _ic->size();

    for (int j = 0; j < sz; j++)
	_vec[j] = i._vec[j];
}

void
Integer::makezero()
{
    int sz = _ic->size();

    for (int j = 0; j < sz; j++)
    	_vec[j] = 0UL;
}

void
Integer::makeone()
{
    int sz = _ic->size();
    _vec[0] = 1UL;

    for (int j = 1; j < sz; j++)
    	_vec[j] = 0UL;
}

void
Integer::makemin()
{
    int hind = _ic->hind();

    for (int j = 0; j <= hind; j++)
    	_vec[j] = 0UL;

    _vec[_ic->sind()] = _ic->smask();
}

void
Integer::makemax()
{
    int hind = _ic->hind();

    for (int j = 0; j < hind; j++)
    	_vec[j] = ~0UL;

    _vec[_ic->sind()] = 0UL;
    _vec[hind] = _ic->hmask();
}


inline void
check(IntegerClass *ic1, IntegerClass *ic2)
{
    if (ic1 != ic2 && ic1->bits() != ic2->bits())
	crash("Integer type mismatch");
}

void
Integer::add(Integer const &i, boolean *carryflag)
{
    Integer const *t = &i;
    
    if (_ic != i._ic)
    {
    	Integer *t2 = &_ic->tmp(2);
	*t2 = i;		// convert i to our size
	t = t2;
    }

    int carryin = 0;

    for (int j = 0; j <= _ic->sind(); j++)
    {
    	uLong sum = _vec[j] + t->_vec[j];
	boolean carry = carryf(sum, _vec[j], t->_vec[j]);
    	uLong sum2 = sum + carryin;
	carryin = carry || carryf(sum2, sum, carryin);
	_vec[j] = sum2;
    }

    carryin = carryin || (_vec[_ic->sind()] & _ic->sextmask());
    _vec[_ic->sind()] &= ~_ic->sextmask();

    if (carryflag != NULL)
    	*carryflag = carryin;
}

void
Integer::neg()
{
    int l = _ic->sind();
    uLong m = _ic->smask();

    if (l == _ic->hind())
    	m |= _ic->hmask();

    int j;
    
    for (j = 0; j <= l; j++)
    {
	uLong sum = ~_vec[j] + 1;

	if (!carryf(sum, ~_vec[j], 1))
	{
	    _vec[j] = sum;
	    break;
	}

	_vec[j] = sum;
    }

    for (j++; j <= l; j++)
	_vec[j] = ~_vec[j];

    _vec[l] &= m;
}

void
Integer::sub(Integer const &i, boolean *borrowflag)
{
    Integer const *t = &i;
    
    if (_ic != i._ic)
    {
    	Integer *t2 = &_ic->tmp(3);
	*t2 = i;		// convert i to our size
	t = t2;
    }

    int borrowin = 0;

    for (int j = 0; j <= _ic->sind(); j++)
    {
    	uLong diff = _vec[j] - t->_vec[j];
	boolean borrow = borrowf(diff, _vec[j], t->_vec[j]);
    	uLong diff2 = diff - borrowin;
	borrowin = borrow || borrowf(diff2, diff, borrowin);
	_vec[j] = diff2;
    }

    borrowin = borrowin || (_vec[_ic->sind()] & _ic->sextmask());
    _vec[_ic->sind()] &= ~_ic->sextmask();

    if (borrowflag != NULL)
    	*borrowflag = borrowin;
}

void
Integer::mult(Integer const &i)
{
    boolean neg = FALSE;

    if (_vec[_ic->sind()] & _ic->smask())
    {
	this->neg();
    	neg = TRUE;
    }

    Integer const *t = &i;

    if (i._vec[i._ic->sind()] & i._ic->smask())
    {
    	Integer *t2 = &i._ic->tmp(4);
	*t2 = i;
	t2->neg();
	neg = !neg;
	t = t2;
    }

    Integer &r = _ic->tmp(5);
    r = _ic->zero();
    int bits = i._ic->bits();

    for (int j = 0; j < bits; j++)
    {
    	if (i._vec[j / BITSPERLONG] & (1 << (j % BITSPERLONG)))
	    r.add(*this);

    	this->lshift(1);
    }

    if (neg)
    	r.neg();

    *this = r;
}

void
Integer::umult(Integer const &i)
{
    Integer r = _ic->zero();
    Integer p = *this;
    int bits = i._ic->bits();

    for (int j = 0; j < bits; j++)
    {
    	if (i._vec[j / BITSPERLONG] & (1 << (j % BITSPERLONG)))
	    r += p;

    	p <<= 1;
    }

    *this = r;
}


void
Integer::divide(Integer const &i, Integer &q, Integer &r)
{
    check(_ic, i._ic);
    check(_ic, q._ic);
    check(_ic, r._ic);

    if (i.isZero())
	crash("Integer division by zero");

    q = _ic->zero();
    r = *this;

    Integer d = i;
    int count = 1;

    while (d.isPos() && r.ucompare(d) > 0)
    {
    	d <<= 1;
	count++;
    }

    while (count--)
    {
	q <<= 1;

	if (r.ucompare(d) >= 0)
	{
	    r -= d;
	    q |= _ic->one();
	}

	d.urshift(1);
    }
}

void
Integer::div(Integer const &i)
{
    check(_ic, i._ic);
    boolean n = FALSE;

    if (isNeg())
    {
    	n = TRUE;
	neg();
    }

    Integer q(*_ic);
    Integer r(*_ic);

    if (i.isNeg())
    {
    	n = !n;
	divide(-i, q, r);
    }
    else
	divide(i, q, r);

    if (n)
	*this = -q;
    else
	*this = q;
}

void
Integer::udiv(Integer const &i)
{
    check(_ic, i._ic);
    Integer q(*_ic);
    Integer r(*_ic);
    divide(i, q, r);
    *this = q;
}

void
Integer::rem(Integer const &i)
{
    check(_ic, i._ic);

    if (isNeg())
	neg();

    Integer q(*_ic);
    Integer r(*_ic);

    if (i.isNeg())
    {
	divide(-i, q, r);
	*this = -r;
    }
    else
    {
	divide(i, q, r);
	*this = r;
    }
}

void
Integer::urem(Integer const &i)
{
    check(_ic, i._ic);
    Integer q(*_ic);
    Integer r(*_ic);
    divide(i, q, r);
    *this = r;
}

// check that this is less than to f
int
Integer::compare(Integer const &i) const
{
    if (&i == this)
	return 0;

    check(_ic, i._ic);
    int l = _ic->sind();
    uLong m = _ic->smask();

    if (_vec[l] & m)
    {
    	if (!(i._vec[l] & m))
	    return -1;
    }
    else
    	if (i._vec[l] & m)
	    return 1;

    l = _ic->hind();
    m = _ic->hmask();

    if ((_vec[l] & m) < (i._vec[l] & m))
    	return -1;

    if ((_vec[l] & m) > (i._vec[l] & m))
    	return 1;

    for (int j = l - 1; j >= 0; j--)
    {
	if (_vec[j] < i._vec[j])
	    return -1;

	if (_vec[j] > i._vec[j])
	    return 1;
    }

    return 0;
}

// check that this is less than to f
int
Integer::ucompare(Integer const &i) const
{
    if (&i == this)
	return 0;

    check(_ic, i._ic);

    int l = _ic->sind();
    uLong m = _ic->smask();

    if (l == _ic->hind())
    	m |= _ic->hmask();

    if ((_vec[l] & m) < (i._vec[l] & m))
    	return -1;

    if ((_vec[l] & m) > (i._vec[l] & m))
    	return 1;

    for (int j = l - 1; j >= 0; j--)
    {
	if (_vec[j] < i._vec[j])
	    return -1;

	if (_vec[j] > i._vec[j])
	    return 1;
    }

    return 0;
}

boolean
Integer::isZero() const
{
    int l = _ic->sind();
    uLong m = _ic->smask();

    if (l == _ic->hind())
	m |= _ic->hmask();

    if ((_vec[l] & m) != 0UL)
    	return FALSE;

    for (int j = 0; j < l; j++)
    	if (_vec[j] != 0UL)
	    return FALSE;

    return TRUE;
}

boolean
Integer::isPos() const
{
    return !(_vec[_ic->sind()] & _ic->smask());
}

boolean
Integer::isNeg() const
{
    return !!(_vec[_ic->sind()] & _ic->smask());
}

#if 0
void
Integer::lshift(int b)
{
    if (b < 1)
	return;

    if (b > 1)
    {
    	for (int i = 0; i < b; i++)
	    lshift(1);

	return;
    }

    int l = _ic->sind();
    uLong m = _ic->smask();

    if (l == _ic->hind())
    	m |= _ic->hmask();

    uLong carry = 0;

    for (int i = 0; i < l; i++)
    {
    	uLong v = (_vec[i] >> 1) | carry;

    	if (_vec[i] & LONGMSB)
	    carry = 1;
	else
	    carry = 0;

    	_vec[i] = v;
    }

    _vec[l] &= m;
}

uLong
Integer::rshift(int b)
{
    if (b < 1)
	return 0UL;

    if (b > 1)
    {
    	for (int i = 0; i < b; i++)
	    rshift(1);

	return 0UL;
    }

    uLong x = 0;
    
    if (_vec[_ic->sind()] & _ic->smask())
    	x = _ic->sextmask();

    int l = _ic->sind();
    uLong m = _ic->smask();
    uLong carry = _vec[l] & m;

    if (l == _ic->hind())
    	m |= _ic->hmask();

    for (int i = l; i >= 0; i--)
    {
    	uLong v = (_vec[i] >> 1) | carry;

    	if (_vec[i] & 1)
	    carry = LONGMSB;
	else
	    carry = 0;

    	_vec[i] = v;
    }

    return 0UL;
}

uLong
Integer::urshift(int b)
{
    if (b < 1)
	return 0UL;

    if (b > 1)
    {
    	for (int i = 0; i < b; i++)
	    urshift(1);

	return 0UL;
    }

    int l = _ic->sind();
    uLong m = _ic->smask();

    if (l == _ic->hind())
    	m |= _ic->hmask();

    uLong carry = 0;

    for (int i = l; i >= 0; i--)
    {
    	uLong v = (_vec[i] >> 1) | carry;

    	if (_vec[i] & 1)
	    carry = LONGMSB;
	else
	    carry = 0;

    	_vec[i] = v;
    }

    return 0UL;
}
#endif

uLong
Integer::lshift(int b)
{
    if (b < 1)
	return 0UL;

    IntegerClass &ic = *_ic;
    int x = b / BITSPERLONG;
    b %= BITSPERLONG;
    int br = BITSPERLONG - b;
    int hind = ic.hind();
    int hshift = ic.hshift();
    uLong hmask = ic.hmask();

    if (ic.sind() == hind)
    {
    	hmask |= ic.smask();
	hshift--;
    }
    else
    {
    	hind = ic.sind();
	hmask = ic.smask();
	hshift = BITSPERLONG - 1;
    }

    uLong hbits = _vec[hind] & hmask;
    uLong out;
    uLong v;

    if (x == 0)
    {
	out = hbits >> br;

	if (hind)
	    v = (hbits << b) | (_vec[hind - 1] >> br);
	else
	    v = hbits << b;

	_vec[hind] &= ~hmask;
	_vec[hind] |= v & hmask;

	if (hshift)
	{
	    v = (out << hshift) | (v >> (BITSPERLONG - hshift));

	    if (out >> (BITSPERLONG - hshift))
		out = v | LONGMSB;
	    else
		out = v;
	}

	int i;
	
	// this is the main shift loop
	for ( i = hind - 1; i > 0; i--)
	    _vec[i] = (_vec[i] << b) | (_vec[i - 1] >> br);

	if (i == 0)
	    _vec[0] <<= b;

	return out;
    }

    if (b == 0)
    {
	if (hind - x < -1)
	    out = 0UL;
	else if (hind - x == -1)
	{
	    if (hind)
		out = _vec[0];
	    else
		out = hbits;
	}
	else if (x == 1)
	    out = hbits;
	else
	    out = _vec[hind - x + 1];
	
	int i;

	for (i = hind - x + 2; i < hind; i++)
	    if (_vec[i])
	    {
		out |= LONGMSB;
		break;
	    }

	if (i == hind && hbits)
	    out |= LONGMSB;

	if (hind - x < 0)
	    v = 0UL;
	else if (hind - x == 0)
	{
	    if (hind)
		v = _vec[0];
	    else
		v = hbits;
	}
	else 
	    v = _vec[hind - x];

	_vec[hind] &= ~hmask;
	_vec[hind] |= v & hmask;

	if (hshift)
	{
	    v = (out << hshift) | (v >> (BITSPERLONG - hshift));

	    if (out >> (BITSPERLONG - hshift))
		out = v | LONGMSB;
	    else
		out = v;
	}

	// this is the main shift loop
	for (i = hind - 1; i >= x; i--)
	    _vec[i] = _vec[i - x];

	for (; i >= 0; i--)
	    _vec[i] = 0UL;

	return out;
    }

    if (hind - x < -1)
	out = 0UL;
    else if (hind - x == -1)
    {
    	if (hind)
	    out = _vec[0] << b;
	else
	    out = hbits << b;
    }
    else if (x == 1)
	out = (hbits << b) | (_vec[hind - 1] >> br);
    else
	out = (_vec[hind - x + 1] << b) | (_vec[hind - x] >> br);

    if (x > 1)
    {
	if (_vec[hind - x + 1] >> br)
	    out |= LONGMSB;
    }
    if (x == 1)
    {
	if (hbits >> br)
	    out |= LONGMSB;
    }
    
    int i;

    for (i = hind - x + 2; i < hind; i++)
    	if (_vec[i])
	{
	    out |= LONGMSB;
	    break;
	}

    if (i == hind && hbits)
    	out |= LONGMSB;

    if (hind - x < 0)
	v = 0UL;
    else if (hind - x == 0)
    {
    	if (hind)
	    v = _vec[0] << b;
	else
	    v = hbits << b;
    }
    else 
	v = (_vec[hind - x] << b) | (_vec[hind - x - 1] >> br);

    _vec[hind] &= ~hmask;
    _vec[hind] |= v & hmask;

    if (hshift)
    {
	v = (out << hshift) | (v >> (BITSPERLONG - hshift));

    	if (out >> (BITSPERLONG - hshift))
	    out = v | LONGMSB;
	else
	    out = v;
    }

    // this is the main shift loop
    for (i = hind - 1; i > x; i--)
	_vec[i] = (_vec[i - x] << b) | (_vec[i - x - 1] >> br);

    if (i == x)
	_vec[i--] = _vec[0] << b;

    for (; i >= 0; i--)
	_vec[i] = 0UL;

    return out;
}

uLong
Integer::urshift(int b)
{
    if (b <= 0)
	return 0UL;

    IntegerClass &ic = *_ic;
    int x = b / BITSPERLONG;
    b %= BITSPERLONG;
    int br = BITSPERLONG - b;
    int hind = ic.hind();
    int hshift = ic.hshift();
    uLong hmask = ic.hmask();

    if (ic.sind() == hind)
    {
    	hmask |= ic.smask();
	hshift--;
    }
    else
    {
    	hind = ic.sind();
	hmask = ic.smask();
	hshift = BITSPERLONG - 1;
    }

    uLong hbits = _vec[hind] & hmask;
    uLong out;

    if (x == 0)
    {
	if (hind)
	    out = _vec[0] << br;
	else
	    out = hbits << br;
	
	int i;

	// this is the main shift loop
	for (i = 0; i + 1 < hind; i++)
	    _vec[i] = (_vec[i] >> b) | (_vec[i + 1] << br);

	if (i + 1 == hind)
	    _vec[hind - 1] = (_vec[hind - 1] >> b) | (hbits << br);

	_vec[hind] >>= b;
	return out;
    }

    if (b == 0)
    {
	if (x <= hind)
	    out = _vec[x - 1];
	else if (x == hind + 1)
	    out = hbits;
	else 
	    out = 0UL;
	
	int i;

	for (i = 0; i < x - 1; i++)
	    if (_vec[i])
	    {
		out |= 1;
		break;
	    }

	// this is the main shift loop
	for (i = 0; i + x < hind; i++)
	    _vec[i] = _vec[i + x];

	if (i + x == hind)
	    _vec[i++] = hbits >> b;

	for (; i <= hind; i++)
	    _vec[i] = 0UL;

	return out;
    }

    if (x < hind)
    {
	if (x == 0)
	    out = _vec[x] << br;
	else
	    out = (_vec[x - 1] >> b) | (_vec[x] << br);
    }
    else if (x == hind)
    {
    	if (hind)
	    out = (_vec[hind - 1] >> b) | (hbits << br);
	else
	    out = hbits << br;
    }
    else if (x == hind + 1)
	out = hbits >> b;
    else 
    	out = 0UL;
    
    int i;

    for (i = 0; i + 1 < x; i++)
    	if (_vec[i])
	{
	    out |= 1;
	    break;
	}

    if (x && _vec[x - 1] << br)
    	out |= 1;

    // this is the main shift loop
    for (i = 0; i + x + 1 < hind; i++)
	_vec[i] = (_vec[i + x] >> b) | (_vec[i + x + 1] << br);

    if (i + x + 1 == hind)
	_vec[i++] = (_vec[hind - 1] >> b) | (hbits << br);

    if (i + x == hind)
	_vec[i++] = hbits >> b;

    for (; i <= hind; i++)
    	_vec[i] = 0UL;

    return out;
}

uLong
Integer::rshift(int b)
{
    if (b <= 0)
	return 0UL;

    if (!isNeg())
	return urshift(b);

    // shifted right with 1 fill.
    IntegerClass &ic = *_ic;
    int x = b / BITSPERLONG;
    b %= BITSPERLONG;
    int br = BITSPERLONG - b;
    int hind = ic.hind();
    int hshift = ic.hshift();
    uLong hmask = ic.hmask();

    if (ic.sind() == hind)
    {
    	hmask |= ic.smask();
	hshift--;
    }
    else
    {
    	hind = ic.sind();
	hmask = ic.smask();
	hshift = BITSPERLONG - 1;
    }

    Long hbits = _vec[hind] | ~hmask;
    uLong out;

    if (x == 0)
    {
	if (hind)
	    out = _vec[0] << br;
	else
	    out = hbits << br;
	
	int i;

	// this is the main shift loop
	for (i = 0; i < hind - 1; i++)
	    _vec[i] = (_vec[i] >> b) | (_vec[i + 1] << br);

	if (i == hind - 1)
	    _vec[hind - 1] = (_vec[hind - 1] >> b) | (hbits << br);

	_vec[hind] = (hbits >> b) & hmask;
	return out;
    }

    if (b == 0)
    {
	if (x <= hind)
	    out = _vec[x - 1];
	else if (x == hind + 1)
	    out = hbits;
	else 
	    out = ~0UL;
	
	int i;

	for (i = 0; i < x - 1; i++)
	    if (_vec[i])
	    {
		out |= 1;
		break;
	    }

	// this is the main shift loop
	for (i = 0; i + x < hind; i++)
	    _vec[i] = _vec[i + x];

	if (i + x == hind)
	    _vec[i++] = hbits >> b;

	for (; i <= hind; i++)
	    _vec[i] = ~0UL;

	_vec[hind] &= hmask;
	return out;
    }

    if (x < hind)
	out = (_vec[x - 1] >> b) | (_vec[x] << br);
    else if (x == hind)
	out = (_vec[hind - 1] >> b) | (hbits << br);
    else if (x == hind + 1)
	out = hbits >> b;
    else 
    	out = ~0UL;
    
    int i;

    for (i = 0; i < x - 1; i++)
    	if (_vec[i])
	{
	    out |= 1;
	    break;
	}

    if (_vec[x - 1] << br)
    	out |= 1;

    // this is the main shift loop
    for (i = 0; i + x + 1 < hind; i++)
	_vec[i] = (_vec[i + x] >> b) | (_vec[i + x + 1] << br);

    if (i + x + 1 == hind)
	_vec[i++] = (_vec[hind - 1] >> b) | (hbits << br);

    if (i + x == hind)
	_vec[i++] = hbits >> b;

    for (; i <= hind; i++)
    	_vec[i] = ~0UL;

    _vec[hind] &= hmask;
    return out;
}

void
Integer::b_and(Integer const &i)
{
    int j;
    
    for ( j = 0; j <= _ic->sind() && j <= i._ic->sind(); j++)
    	_vec[j] &= i._vec[j];

    for (; j <= _ic->sind(); j++)
    	_vec[j] = 0UL;
}

void
Integer::b_or(Integer const &i)
{
    for (int j = 0; j <= _ic->sind() && j <= i._ic->sind(); j++)
    	_vec[j] |= i._vec[j];
}

void
Integer::b_xor(Integer const &i)
{
    for (int j = 0; j <= _ic->sind() && j <= i._ic->sind(); j++)
    	_vec[j] ^= i._vec[j];
}

void
Integer::complement()
{
    for (int j = 0; j <= _ic->sind(); j++)
    	_vec[j] = ~_vec[j];
}
