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
#include <ctype.h>
#include <limits.h>

#include <stdlibx.h>
#include <integer.h>
#include "floatc.h"

// these FloatClasses are independent of any floating point capabilities
// of the host machine.  The constants below are always correct by definition.
FloatClass fcsingle(23, 8);
FloatClass fcdouble(52, 11);
FloatClass fcquad(112, 15);
FloatClass fcocta(236, 19);

#define ALLONES (~0UL)

void
FloatClass::InitParameters()
{
    _msize = (_mantissa + BITSPERLONG - 1) / BITSPERLONG;
    _esize = (_exponent + BITSPERLONG - 1) / BITSPERLONG;

    int i = _mantissa % BITSPERLONG;
    _mhshift = (BITSPERLONG - i) % BITSPERLONG;
    _mhind = (_mantissa - 1) / BITSPERLONG;

    if (i)
	_mhmask = (1UL << i) - 1;
    else
    	_mhmask = ALLONES;

    if (_mhind)
	_mlmask = ALLONES;
    else
	_mlmask = _mhmask;

    int j = (_exponent + i) % BITSPERLONG;	

    _ehshift = (BITSPERLONG - j) % BITSPERLONG;
    _ehind = (_exponent + _mantissa - 1) / BITSPERLONG;

    if (j)
	_ehmask = (1UL << j) - 1;
    else
	_ehmask = ALLONES;

    int k = _exponent % BITSPERLONG;

    if (k)
    	_efmask = (1UL << k) - 1;
    else
	_efmask = ALLONES;

    _elshift = i;
    _elind = _mantissa / BITSPERLONG;

    if (_mhshift)
	_elmask = ((1UL << _mhshift) - 1) << _elshift;
    else
	_elmask = ALLONES;

    if (_ehind == _elind)
    {
	_ehmask &= _elmask;
	_elmask = _ehmask;
    }

    _sshift = j;
    _sind = (_exponent + _mantissa) / BITSPERLONG;
    _smask = 1UL << j;
}

void
FloatClass::InitConstants()
{
    constant(FloatConstQuietNaN, Float(*this));	    // expand it all at once
    constant(FloatZero, Float(*this));
    constant(FloatOne, Float(*this, 1));
    constant(FloatTwo, Float(*this, 2));
    constant(FloatFour, Float(*this, 4));
    constant(FloatMinusOne, Float(*this, -1));
    constant(FloatTen, Float(*this, 10));
    Float x(*this, 1);
    x.decexp();
    constant(FloatHalf, x);
    x.decexp();
    constant(FloatFourth, x);
    x.decexp(_mantissa - 2);
    constant(FloatEpsilon, x);
    x.makeinf();
    constant(FloatConstPosInf, x);
    x.neg();
    constant(FloatConstNegInf, x);
    x.makenan();
    constant(FloatConstNaN, x);
    x.makequietnan();
    constant(FloatConstQuietNaN, x);
    constant(FloatTenth, constant(FloatOne) / constant(FloatTen));
}

FloatClass::FloatClass(int mantissa, int exponent)
{
    _size = ((mantissa + exponent) / BITSPERLONG) + 1;
    _bits = mantissa + exponent + 1;
    _mantissa = mantissa;
    _exponent = exponent;
    _freelist = NULL;
    _tempcnt = 0;
    _temps = NULL;
    _constcnt = 0;
    _consts = NULL;
    _constset = NULL;
    _roundmode = FloatRoundToNearest;
    _trapmask = 0;
    _status = 0;

    for (int t = FloatFormatMismatch; t < FloatTrapTypes; t++)
	_traphandlers[t] = (FloatTrapHandlerPtr)NULL;

    InitParameters();
    InitConstants();
}

FloatClass::~FloatClass()
{
    while (_freelist != NULL)
    {
	uLong *t = _freelist;
	_freelist = *(uLong **)_freelist;
	delete[/*_size*/] t;
    }

    if (_tempcnt)
	delete[/*_tempcnt*/] _temps;

    if (_constcnt)
    {
    	delete[/*_constcnt*/] _consts;
    	delete[/*_constcnt*/] _constset;
    }
}

uLong *
FloatClass::get()
{
    if (_freelist == NULL)
	return new uLong[_size];

    uLong *t = _freelist;
    _freelist = *(uLong **)_freelist;
    return t;
}

void
FloatClass::put(uLong *p)
{
    *(uLong **)p = _freelist;
    _freelist = p;
}

static FloatClass *gfc = NULL;

class _Float : public Float
{
public:
    _Float() : Float(*gfc) { }
};

Float &
FloatClass::tmp(int i)
{
    if (i < 0)
	return *(Float *)NULL;

    if (i >= _tempcnt)
    {
    	gfc = this;
    	Float *ntemps = new _Float[i + 1];

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

Float const &
FloatClass::constant(enum FloatConstants i) const
{
    if (i < 0 || i >= _constcnt || !_constset[i])
	return *(Float *)NULL;

    return _consts[i];
}

void
FloatClass::constant(enum FloatConstants n, Float const &f)
{
    if (n < 0)
    	return;

    if (n >= _constcnt)
    {
    	gfc = this;
    	Float *nconsts = new _Float[n + 1];
    	boolean *nconstset = new boolean[n + 1];

	for (int i = 0; i < n; i++)
	    nconstset[i] = FALSE;

	if (_consts != NULL)
	{
	    for (int i = 0; i < _constcnt; i++)
	    {
	    	nconsts[i] = _consts[i];
		nconstset[i] = _constset[i];
	    }

	    delete[/*_constcnt*/] _consts;
	    delete[/*_constcnt*/] _constset;
	}

	_consts = nconsts;
	_constset = nconstset;
	_constcnt = n + 1;
    }

    _consts[n] = f;
    _constset[n] = TRUE;
}

extern Float pi(FloatClass &fc);
extern Float e(FloatClass &fc);

Float const &
FloatClass::mkconstant(enum FloatConstants i)
{
    if (constantset(i))
    	return constant(i);

    switch (i)
    {
    case FloatPi:
    	constant(FloatPi, ::pi(*this));
	break;
    case FloatE:
    	constant(FloatE, ::e(*this));
	break;
    case FloatSqrtTwo:
    	constant(FloatSqrtTwo, sqrt(two()));
	break;
    case FloatLnTwo:
    	constant(FloatLnTwo, ln(two()));
	break;
    case FloatLnTen:
    	constant(FloatLnTen, ln(ten()));
	break;
    default:
    	break;
    }

    if (constantset(i))
    	return constant(i);

    return *(Float const *)NULL;
}

boolean 
FloatClass::trap(enum FloatTrapType t, enum FloatOperationType op,
	Float const &operand1, Float const &operand2, Float &result)
{
    status(t, TRUE);
    Float temp = result;

    if (t >= 0 && t < FloatTrapTypes && (_trapmask & (1 << t)) &&
	    _traphandlers[t] != (FloatTrapHandlerPtr)NULL &&
	    (*_traphandlers[t])(t, op, operand1, operand2, temp))
    {
	result = temp;
	return TRUE;
    }

    return FALSE;
}

void
FloatClass::trap(enum FloatTrapType t, FloatTrapHandlerPtr v)
{
    if (t >= 0 && t < FloatTrapTypes)
	_traphandlers[t] = v;
}

FloatTrapHandlerPtr
FloatClass::trap(enum FloatTrapType t) const
{
    if (t < 0 || t >= FloatTrapTypes)
    	return (FloatTrapHandlerPtr)NULL;

    return _traphandlers[t];
}

boolean
FloatClass::trapenable(enum FloatTrapType t, boolean v)
{
    if (t < 0 || t >= FloatTrapTypes)
    	return FALSE;

    boolean prev = !!(_trapmask & (1 << t));

    if (v)
	_trapmask |= 1 << t;
    else
	_trapmask &= ~(1 << t);

    return prev;
}

boolean
FloatClass::trapenable(enum FloatTrapType t) const
{
    if (t < 0 || t >= FloatTrapTypes)
    	return FALSE;

    return !!(_trapmask & (1 << t));
}

void
FloatClass::status(enum FloatTrapType t, boolean v)
{
    if (t >= 0 && t < FloatTrapTypes)
    	if (v)
	    _status |= 1 << t;
	else
	    _status &= ~(1 << t);
}

boolean
FloatClass::status(enum FloatTrapType t) const
{
    if (t < 0 || t >= FloatTrapTypes)
    	return FALSE;

    return !!(_status & (1 << t));
}

Float::Float(FloatClass &fc, uLong *v)
{
    Init(fc);

    for (int i = 0; i < fc.size(); i++)
	_vec[i] = v[i];
}

Float::Float(FloatClass &fc, long x)
{
    Init(fc);

    if (x == 0)
    {
    	makezero();
	return;
    }

    int neg = 0;

    if (x < 0)
    {
    	x = -x;
	neg = 1;
    }

    if (x == 1)
    {
	makeone();

	if (neg)
	    _vec[fc.sind()] |= fc.smask();

	return;
    }

    makezero();
    _vec[0] = (uLong)x;
    int n = fc.mantissa() - bits(x) + 1;

    if (!fc.mhind())
    {
    	if (n < 0)
	    _vec[0] >>= -n;
	else
	    _vec[0] <<= n;

	_vec[0] &= fc.mhmask();
    }
    else
	mlshift(n);

    copyexp(fc.constant(FloatOne));
    incexp(fc.mantissa() - n);

    if (neg)
	_vec[fc.sind()] |= fc.smask();
}

Float::Float(Float const &f)
{
    Init(*f._fc);

    for (int i = 0; i < _fc->size(); i++)
	_vec[i] = f._vec[i];
};

Float::Float(FloatClass &fc, Float const &f)
{
    Init(fc);

    if (&fc == f._fc)
	for (int i= 0; i < fc.size(); i++)
	    _vec[i] = f._vec[i];
    else
	convert(f);
}

Float::~Float()
{
    if (_fc->alloc())
	_fc->put(_vec);
}

Float &
Float::operator=(Float const &f)
{
    if (_fc == f._fc)
	for (int i = 0; i < _fc->size(); i++)
	    _vec[i] = f._vec[i];
    else
    	convert(f);

    return *this;
}

void
Float::convert(Float const &f)
{
    if (_fc->_mantissa == f._fc->_mantissa && 
	    _fc->_exponent == f._fc->_exponent &&
	    _fc->_size == f._fc->_size)
    {
    	for (int i = 0; i < _fc->_size; i++)
	    _vec[i] = f._vec[i];

	return;
    }

    enum FloatType ftype = f.floattype();

    switch (ftype)
    {
    case FloatNaN:
    	makenan();
	return;
    case FloatQuietNaN:
    	makequietnan();
	return;
    case FloatPosInf:
    	makeinf();
	return;
    case FloatPosZero:
    default:
    	makezero();
	return;
    case FloatNegZero:
	makezero();
	neg();
	return;
    case FloatNegInf:
	makeinf();
	neg();
	return;
    case FloatPositive:
    case FloatPosDenorm:
    case FloatNegDenorm:
    case FloatNegative:
    	break;
    }

    // start with zero
    makezero();

    // copy over the exponent adjusting the bias
    if (_fc->_exponent != f._fc->_exponent)
    {
    	// convert the exponent
	Float t = f;
	boolean borrow = t.esub(f._fc->one());

	if (borrow)
	{
	    t.ecompl();		// negate 
	    t.incexp();
	}

	int bits = t.ebits();

	if (bits > _fc->_exponent)
	{
	    if (borrow)
		makezero();		// underflow
	    else
		makeinf();		// overflow

	    sign(f.sign());
	    return;
	}

	int words = (bits + BITSPERLONG - 1) / BITSPERLONG;

	for (int i = 0; i < words; i++)
	    sete(i, t.gete(i));

	if (borrow)
	{
	    ecompl();		// negate
	    incexp();
	}

	boolean carry = eadd(_fc->one());

	if (borrow ^ carry)
	{
	    if (borrow)
		makezero();		// underflow
	    else
		makeinf();		// overflow

	    sign(f.sign());
	    return;
	}
    }
    else
    	for (int i = 0; i < _fc->esize(); i++)
	    sete(i, f.gete(i));

    uLong lowbits = 0UL;

    // copy over the mantissa shifting to align it
    if (_fc->_mantissa >= f._fc->_mantissa)
    {
    	for (int i = 0; i < f._fc->msize(); i++)
	    setm(i, f.getm(i));

	mlshift(_fc->_mantissa - f._fc->_mantissa);
    }
    else
    {
	Float t = f;
	lowbits = t.mrshift(f._fc->_mantissa - _fc->_mantissa);

	for (int i = 0; i < _fc->msize(); i++)
	    setm(i, t.getm(i));
    }

    int headroom, bias;

    sign(f.sign());
    enum FloatType thistype = floattype();

    switch (thistype)
    {
    case FloatNaN:
    case FloatQuietNaN:
    	makeinf();
	sign(f.sign());
	return;
    case FloatPosInf:
    case FloatNegInf:
    	return;
    case FloatPositive:
    case FloatNegative:
    	if (ftype == FloatPositive || ftype == FloatNegative)
	    return;

	// denormal to normal, adjust mantissa
	headroom = expdelta(_fc->zero());
	bias = normalize();

	if (bias < headroom)
	    decexp(bias);
	else
	{
	    int sf = bias - headroom;
	    copyexp(_fc->zero());
	    mrshift(sf + 1);
	    setmbit(_fc->mantissa() - sf);
	}

	return;
    case FloatPosDenorm:
    case FloatPosZero:
    case FloatNegZero:
    case FloatNegDenorm:
    	if (ftype == FloatPosDenorm || ftype == FloatNegDenorm)
	    return;

	// normal to denormal, adjust mantissa
	mrshift(1);
	setmbit(_fc->_mantissa - 1);
	return;
    }

    return;
}

enum FloatType
Float::floattype() const
{
    boolean sign = (boolean)((_vec[_fc->sind()] & _fc->smask()) >>
	    _fc->sshift());
    boolean expmin = TRUE;
    boolean expmax = TRUE;

    int i = _fc->elind();

    if ((_vec[i] & _fc->elmask()) != 0UL)
    {
    	expmin = FALSE;

	if ((_vec[i] & _fc->elmask()) != (ALLONES & _fc->elmask()))
	    expmax = FALSE;
    }
    else
    	expmax = FALSE;

    for (i++; (expmin || expmax) && i < _fc->ehind(); i++)
    	if (_vec[i] != 0UL)
	{
	    expmin = FALSE;

	    if (_vec[i] != ALLONES)
		expmax = FALSE;
	}
	else
	    expmax = FALSE;

    if ((expmin || expmax) && i == _fc->ehind())
    {
	if ((_vec[i] & _fc->ehmask()) != 0UL)
	{
	    expmin = FALSE;

	    if ((_vec[i] & _fc->ehmask()) != _fc->ehmask())
		expmax = FALSE;
	}
	else
	    expmax = FALSE;
    }

    if (!expmin && !expmax)
    	return sign ? FloatNegative : FloatPositive;

    boolean mantzero = TRUE;
    i = _fc->mlind();

    if ((_vec[i] & _fc->mlmask()) != 0UL)
    	mantzero = FALSE;

    for (i++; mantzero && i < _fc->mhind(); i++)
	if (_vec[i] != 0UL)
	    mantzero = FALSE;

    if (mantzero && i == _fc->mhind() && (_vec[i] & _fc->mhmask()) != 0UL)
	mantzero = FALSE;

    switch ((expmax << 3) | (sign << 2) | (expmin << 1) | mantzero)
    {
    case 2:
    	return FloatPosDenorm;
    case 3:
    	return FloatPosZero;
    case 6:
    	return FloatNegDenorm;
    case 7:
    	return FloatNegZero;
    case 9:
    	return FloatPosInf;
    case 13:
    	return FloatNegInf;
    }

    if (_vec[0] & 1)
	return FloatQuietNaN;

    return FloatNaN;
}

void
Float::makeone()
{
    int i;
    
    for (i = 0; i <= _fc->mhind(); i++)
    	_vec[i] = 0UL;

    _vec[_fc->sind()] = 0UL;

    i = _fc->ehind();
    _vec[i] = (_fc->ehmask() >> 1) & _fc->ehmask();

    for (i--; i > _fc->elind(); i--)
	_vec[i] = ALLONES;

    if (i == _fc->elind())
	_vec[i] = _fc->elmask();
}

void
Float::makeinf()
{
    int i;
    
    for (i = 0; i <= _fc->mhind(); i++)
    	_vec[i] = 0UL;

    _vec[_fc->sind()] = 0UL;

    i = _fc->elind();
    _vec[i] = _fc->elmask();

    for (i++; i < _fc->ehind(); i++)
	_vec[i] = ALLONES;

    if (i == _fc->ehind())
	_vec[i] = _fc->ehmask();
}

void
Float::changetype(FloatClass &fc)
{
    if (_fc != &fc)
    {
	if (_fc->alloc())
	    _fc->put(_vec);

	_fc = &fc;

	if (_fc->alloc())
	    _vec = _fc->get();
	else
	    _vec = _buf;

	makezero();
    }
}

inline boolean
sameformat(Float &f1, Float &f2)
{
    FloatClass &fc1 = f1.floatclass();
    FloatClass &fc2 = f2.floatclass();
    return fc1.mantissa() == fc2.mantissa() &&
	    fc1.exponent() == fc2.exponent();
}

inline boolean
betterformat(Float &f1, Float &f2)
{
    FloatClass &fc1 = f1.floatclass();
    FloatClass &fc2 = f2.floatclass();
    return fc1.mantissa() >= fc2.mantissa() &&
	    fc1.exponent() >= fc2.exponent();
}

struct FloatClassKey
{
    int mant;
    int exp;
};

struct FloatClassElem
{
    struct FloatClassKey key;
    FloatClass *fcp;
    FloatClassElem(FloatClassKey const &k) { key = k; }
    operator FloatClassKey() { return key; }
    boolean operator==(FloatClassKey const &k)
	    { return key.mant == k.mant && key.exp == k.exp; }
    boolean operator==(FloatClassElem const &e)
	    { return key.mant == e.key.mant && key.exp == e.key.exp; }
};

unsigned
floatclasskeyhash(FloatClassKey const &k)
{
    return (k.mant >> 3) ^ (k.exp >> 2) ^ k.mant ^ k.exp;
}

#define TABLE FloatClassTable
#define ITER FloatClassIter
#define ELEM FloatClassElem
#define KEY FloatClassKey
#define HASHFUNC floatclasskeyhash
#include <hashtable.g>
#define TABLE FloatClassTable
#define ITER FloatClassIter
#define ELEM FloatClassElem
#define KEY FloatClassKey
#define HASHFUNC floatclasskeyhash
#define IMPLEMENT
#include <hashtable.g>

//declare_hashtable(FloatClassTable, FloatClassIter, FloatClassElem, FloatClassKey)
//implement_hashtable(FloatClassTable, FloatClassIter, FloatClassElem, FloatClassKey, floatclasskeyhash)

FloatClassTable floatclasstable;

FloatClass &
findformat(int mant, int exp)
{
    FloatClassKey key;
    key.mant = mant;
    key.exp = exp;

    if (floatclasstable.find(key))
    	return *floatclasstable().fcp;

    floatclasstable.insert(key);
    FloatClass *fcp = new FloatClass(mant, exp);
    floatclasstable().fcp = fcp;
    return *fcp;
}

FloatClass &
commonformat(Float &f1, Float &f2)
{
    FloatClass &fc1 = f1.floatclass();
    FloatClass &fc2 = f2.floatclass();
    int m1 = fc1.mantissa();
    int m2 = fc2.mantissa();
    int e1 = fc1.exponent();
    int e2 = fc2.exponent();
    int mant;
    int exp;

    if (m1 > m2)
	mant = m1;
    else
    	mant = m2;

    if (e1 > e2)
    	exp = e1;
    else
	exp = e2;

    return findformat(mant, exp);
}

void
makecompatible(Float &a, Float &b, Float *&pa, Float *&pb)
{
    static Float *t1 = NULL;
    static Float *t2 = NULL;
    pa = &a;
    pb = &b;

    if (sameformat(a, b))
	return;

    if (betterformat(a, b))
    {
    	if (t1 == NULL)
	    t1 = new Float(a.floatclass(), b);
	else
	{
	    t1->changetype(a.floatclass());
	    *t1 = b;
	}

	pb = t1;
	return;
    }

    if (betterformat(b, a))
    {
    	if (t1 == NULL)
	    t1 = new Float(b.floatclass(), a);
	else
	{
	    t1->changetype(b.floatclass());
	    *t1 = a;
	}

	pa = t1;
	return;
    }

    FloatClass &fc = commonformat(a, b);

    if (t1 == NULL)
	t1 = new Float(fc, a);
    else
    {
	t1->changetype(fc);
	*t1 = a;
    }

    if (t2 == NULL)
	t2 = new Float(fc, b);
    else
    {
	t2->changetype(fc);
	*t2 = b;
    }

    pa = t1;
    pb = t2;
    return;
} 

long
floattol(Float const &f)
{
    enum FloatType ft = f.floattype();

    switch (ft)
    {
    case FloatNaN:
    case FloatQuietNaN:
    case FloatPosInf:
	return LONG_MAX;
    case FloatNegInf:
	return LONG_MIN;
    case FloatPosDenorm:
    case FloatPosZero:
    case FloatNegZero:
    case FloatNegDenorm:
    	return 0;
    default:
	break;
    }

    int delta = f.expdelta(f.floatclass().half());

    if (delta < 0)
    	return 0;

    if (delta > BITSPERLONG)
	return LONG_MAX;

    int sf = f.floatclass().mantissa() - delta + 1;

    uLong lowbits = 0UL;
    uLong bits = 0UL;

    if (sf > 0)
    {
    	Float x = f;
    	lowbits = x.mrshift(sf);
	bits = x.getm(0) | (1UL << (delta - 1));
    }
    else
	bits = f.getm(0) | (1UL << f.floatclass().mantissa());

    if (sf < 0)
    	bits <<= -sf;

    switch (f.floatclass().roundmode())
    {
    case FloatRoundToNearest:
    	if (lowbits > LONGMSB)
	    bits++;
    	else if (lowbits == LONGMSB && (bits & 1))
	    bits++;

	break;
    case FloatRoundTowardZero:
    	break;
    case FloatRoundTowardPosInf:
    	if (ft == FloatPositive)
	    bits++;

	break;
    case FloatRoundTowardNegInf:
    	if (ft == FloatNegative)
	    bits++;

	break;
    }

    if (ft == FloatNegative)
    {
	if (bits > LONGMSB)
	    return LONG_MIN;

	return -(long)bits;
    }

    if (bits >= LONGMSB)
    	return LONG_MAX;

    return (long)bits;
}

int
floattoi(Float const &f)
{
    long l = floattol(f);

    if (l >= INT_MAX)
    	return INT_MAX;

    if (l <= INT_MIN)
	return INT_MIN;

    return (int)l;
}
