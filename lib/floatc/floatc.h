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


#ifndef __FLOATC_H_
#define __FLOATC_H_

#include <stdio.h>

#ifndef __STDLIBX_H_
#include <stdlibx.h>
#endif

#define _FLOAT_MINLONGS 2

class Float;

enum FloatType { FloatNaN, FloatQuietNaN, FloatNegInf, FloatNegative,
	FloatNegDenorm, FloatNegZero, FloatPosZero, FloatPosDenorm,
	FloatPositive, FloatPosInf };

enum FloatRoundingMode { FloatRoundToNearest, FloatRoundTowardPosInf,
	FloatRoundTowardNegInf, FloatRoundTowardZero };

enum FloatTrapType { FloatFormatMismatch, FloatInvalidOperation,
	FloatDivideByZero, FloatOverflow, FloatUnderflow, FloatInexact,
	FloatTrapTypes };

enum FloatOperationType { FloatNone, FloatSignalingNaN, FloatAddition,
	FloatSubtraction, FloatReverseSubtraction, FloatMultiplication,
	FloatDivision, FloatModulus, FloatSquareRoot, FloatConvertion,
	FloatComparison, FloatNextAfter };

typedef boolean (*FloatTrapHandlerPtr)(enum FloatTrapType,
	enum FloatOperationType, Float const &operand1, Float const &operand2,
	Float &result);

enum FloatConstants { FloatZero, FloatOne, FloatTwo, FloatFour, FloatMinusOne,
	FloatTen, FloatHalf, FloatFourth, FloatTenth, FloatEpsilon,
	FloatConstPosInf, FloatConstNegInf, FloatConstNaN, FloatConstQuietNaN,
	FloatPi, FloatE, FloatSqrtTwo, FloatLnTwo, FloatLnTen };

class FloatClass
{
    int _size;
    int _msize;
    int _esize;
    int _bits;
    int _mantissa;
    int _exponent;
    int _tempcnt;
    Float *_temps;
    int _constcnt;
    boolean *_constset;
    Float *_consts;
    int _mhind;
    int _mhshift;
    uLong _mhmask;
    uLong _mlmask;
    int _ehind;
    int _ehshift;
    uLong _ehmask;
    int _elind;
    int _elshift;
    uLong _elmask;
    uLong _efmask;
    int _sind;
    int _sshift;
    uLong _smask;
    uLong *_freelist;
    enum FloatRoundingMode _roundmode;
    FloatTrapHandlerPtr _traphandlers[FloatTrapTypes];
    uLong _trapmask;
    uLong _status;
    void InitParameters();
    void InitConstants();
protected:
    friend class Float;
    uLong *get();
    void put(uLong *p);
    Float &tmp(int i);
    Float const &constant(enum FloatConstants i) const;
    Float const &mkconstant(enum FloatConstants i);
    boolean constantset(enum FloatConstants i) const
	    { return i >= 0 && i < _constcnt && _constset[i]; }
    void constant(enum FloatConstants i, Float const &);
    boolean trap(enum FloatTrapType, enum FloatOperationType,
	Float const &operand1, Float const &operand2, Float &result);
public:
    FloatClass(int mantissa, int exponent);
    ~FloatClass();
    int bits() const { return _bits; }
    int mantissa() const { return _mantissa; }
    int exponent() const { return _exponent; }
    int size() const { return _size; }		// size in longs
    int msize() const { return _msize; }	// mantissa size in longs
    int esize() const { return _esize; }	// exponent size in longs
    boolean alloc() const { return _size > _FLOAT_MINLONGS; }

    // constant properties of floats for this class
    int mhind() const { return _mhind; }
    int mhshift() const { return _mhshift; }
    uLong mhmask() const { return _mhmask; }
    int mlind() const { return 0; }		// redundant
    int mlshift() const { return 0; }		// redundant
    uLong mlmask() const { return _mlmask; }
    uLong mfmask() const { return _mhmask; }	// redundant
    int ehind() const { return _ehind; }
    int ehshift() const { return _ehshift; }
    uLong ehmask() const { return _ehmask; }
    int elind() const { return _elind; }
    int elshift() const { return _elshift; }
    uLong elmask() const { return _elmask; }
    uLong efmask() const { return _efmask; }
    int sind() const { return _sind; }
    int sshift() const { return _sshift; }
    uLong smask() const { return _smask; }

    // constant float values for this class
    Float const &zero() const { return constant(FloatZero); }
    Float const &one() const { return constant(FloatOne); }
    Float const &two() const { return constant(FloatTwo); }
    Float const &four() const { return constant(FloatFour); }
    Float const &minusone() const { return constant(FloatMinusOne); }
    Float const &ten() const { return constant(FloatTen); }
    Float const &half() const { return constant(FloatHalf); }
    Float const &fourth() const { return constant(FloatFourth); }
    Float const &tenth() const { return constant(FloatTenth); }
    Float const &epsilon() const { return constant(FloatEpsilon); }
    Float const &posinf() const { return constant(FloatConstPosInf); }
    Float const &neginf() const { return constant(FloatConstNegInf); }
    Float const &nan() const { return constant(FloatConstNaN); }
    Float const &qnan() const { return constant(FloatConstQuietNaN); }
    Float const &pi() { return mkconstant(FloatPi); }
    Float const &e() { return mkconstant(FloatE); }
    Float const &sqrttwo() { return mkconstant(FloatSqrtTwo); }
    Float const &ln2() { return mkconstant(FloatLnTwo); }
    Float const &ln10() { return mkconstant(FloatLnTen); }
    
    // rounding mode controls
    void roundmode(enum FloatRoundingMode rm) { _roundmode = rm; }
    enum FloatRoundingMode roundmode() const { return _roundmode; }

    // float trap handling
    void trap(enum FloatTrapType, FloatTrapHandlerPtr);
    FloatTrapHandlerPtr trap(enum FloatTrapType) const;
    boolean trapenable(enum FloatTrapType, boolean);
    boolean trapenable(enum FloatTrapType) const;
    void trapmask(uLong mask) { _trapmask = mask; }
    uLong trapmask() const { return _trapmask; }
    void status(enum FloatTrapType, boolean);
    boolean status(enum FloatTrapType) const;
    void status(uLong stat) { _status = stat; };
    uLong status() const { return _status; }
};

class Float
{
    FloatClass *_fc;
    uLong *_vec;
    uLong _buf[_FLOAT_MINLONGS];

    friend class FloatClass;
    friend long floattol(Float const &);
    friend Float sqrt(Float const &);
    void Init(FloatClass &fc)
	    { _fc = &fc; _vec = fc.alloc() ? fc.get() : _buf; }
    int normalize();
    void convert(Float const &);
    boolean checkexceptions(enum FloatType thistype, enum FloatType ftype,
	    enum FloatOperationType optype, Float const &f);
    boolean formatcheck(Float const &f, enum FloatOperationType optype);
    void handleinexact(boolean overflow, boolean underflow, Float const &f,
	    enum FloatOperationType optype);
protected:
    void copyexp(Float const &);
    void copymant(Float const &);
    boolean next();
    boolean prev();
    boolean incexp(int amount = 1);
    boolean decexp(int amount = 1);
    boolean lowerexp(int amount = 1);	// handle denormals
    int expdelta(Float const &f) const;
    void set(int i, uLong v);
    void setm(int i, uLong v);
    void sete(int i, uLong v);
    void setbit(int i, boolean v = TRUE);
    void setmbit(int i, boolean v = TRUE);
    void setebit(int i, boolean v = TRUE);
    int ebits() const;			// width of bits set in exponent
    int mbits() const;			// width of bits set in mantissa
    uLong rshift(int b);
    uLong lshift(int b);
    uLong mrshift(int b);
    uLong mlshift(int b);
    uLong ershift(int b);
    uLong elshift(int b);
    void band(Float const &f);
    void mand(Float const &f);
    void eand(Float const &f);
    void bor(Float const &f);
    void mor(Float const &f);
    void eor(Float const &f);
    void bcompl();
    void mcompl();
    void ecompl();
    boolean madd(Float const &f, uLong carryin = 0UL);
    boolean eadd(Float const &f, uLong carryin = 0UL);
    boolean msub(Float const &f, uLong borrowin = 0UL);
    boolean esub(Float const &f, uLong borrowin = 0UL);
    boolean roundup(uLong lowbits);
    boolean rounddown(uLong lowbits);
    void uadd(Float const &f);
    void usub(Float const &f);
    void usubr(Float const &f);
public:
#ifdef __GNUC__
    void makezero() { for (int i = 0; i < _fc->size(); i++) _vec[i] = 0UL; }
#else
    void makezero();
#endif
    Float(FloatClass &fc) { Init(fc); makezero(); } // Initialize to zero
    Float(FloatClass &fc, long x);		// Initialize from integer
    Float(FloatClass &fc, char const *s, char const **end = NULL); // from string
    Float(Float const &f);			// sameary
    Float(FloatClass &fc, Float const &f);	// Type convertion
    Float(FloatClass &fc, uLong *v);		// Initialize from vector
    ~Float();
    void makeone();
    void makeinf();
    void makenan() { makeinf(); _vec[0] |= 2; }
    void makequietnan() { makeinf(); _vec[0] |= 1; }
    void changetype(FloatClass &fc);
    Float &operator=(Float const &f);
    void add(Float const &f);
    void neg() { _vec[_fc->sind()] ^= 1UL << _fc->sshift(); }	// (A.2)
    void sub(Float const &f);
    void subr(Float const &f);
    void mult(Float const &f);
    void mult2n(int n);
    void div(Float const &f);
    void rem(Float const &f);
    Float &operator+=(Float const &f) { add(f); return *this; }
    Float &operator-=(Float const &f) { sub(f); return *this; }
    Float &operator*=(Float const &f) { mult(f); return *this; }
    Float &operator/=(Float const &f) { div(f); return *this; }
    Float &operator%=(Float const &f) { rem(f); return *this; }
    Float operator+(Float const &f) const
	    { Float t = *this; t.add(f); return t; }
    Float operator-() const { Float t = *this; t.neg(); return t; }
    Float operator-(Float const &f) const
	    { Float t = f; t.subr(*this); return t; }
    Float operator*(Float const &f) const
	    { Float t = *this; t.mult(f); return t; }
    Float operator/(Float const &f) const 
	    { Float t = *this; t.div(f); return t; }
    Float operator%(Float const &f) const 
	    { Float t = *this; t.rem(f); return t; }
    Float &operator++() { add(_fc->one()); return *this; }
    Float operator++(int) { Float t = *this; add(_fc->one()); return t; }
    Float &operator--() { add(_fc->minusone()); return *this; }
    Float operator--(int) { Float t = *this; add(_fc->minusone()); return t; }
    int ucompare(Float const &f) const;		// magnitude comparison
    int compare(Float const &f) const;
    boolean equal(Float const &f) const { return compare(f) == 0; }
    boolean lessthan(Float const &f) const { return compare(f) == -1; }
    boolean greaterthan(Float const &f) const { return compare(f) == 1; }
    boolean unordered(Float const &f) const { return compare(f) == -2; } //(A.9)
    boolean operator==(Float const &f) const { return compare(f) == 0; }
    boolean operator!=(Float const &f) const			// (A.8)
	    { int c = compare(f); return c == -1 || c == 1; }
    boolean operator<(Float const &f) const { return compare(f) == -1; }
    boolean operator<=(Float const &f) const
	    { int c = compare(f); return c == -1 || c == 0; }
    boolean operator>(Float const &f) const { return compare(f) == 1; }
    boolean operator>=(Float const &f) const
	    { int c = compare(f); return c == 0 || c == 1; }
    enum FloatType floattype() const;				// (A.10)
    boolean isPos() const
    {
    	enum FloatType t= floattype();
	return t == FloatPosZero || t == FloatPosDenorm ||
		t == FloatPositive || t == FloatPosInf;
    }
    boolean isNeg() const
    {
    	enum FloatType t= floattype();
	return t == FloatNegZero || t == FloatNegDenorm ||
		t == FloatNegative || t == FloatNegInf;
    }
    boolean isZero() const
    {
    	enum FloatType t= floattype();
	return t == FloatNegZero || t == FloatPosZero;
    }
    boolean isPZero() const { return floattype() == FloatPosZero; }
    boolean isNZero() const { return floattype() == FloatNegZero; }
    boolean isInf() const
    {
    	enum FloatType t= floattype();
	return t == FloatNegInf || t == FloatPosInf;
    }
    boolean isPInf() const { return floattype() == FloatPosInf; }
    boolean isNInf() const { return floattype() == FloatNegInf; }
    boolean isNan() const					// (A.7)
    {
    	enum FloatType t= floattype();
	return t == FloatNaN || t == FloatQuietNaN;
    }
    boolean isQNan() const { return floattype() == FloatQuietNaN; }
    boolean isSNan() const { return floattype() == FloatNaN; }
#ifdef finite
#undef finite
#endif
    boolean finite() const					// (A.6)
    {
    	enum FloatType t = floattype();
	return t == FloatNegative || t == FloatNegDenorm || t == FloatNegZero ||
		t == FloatPosZero || t == FloatPosDenorm || t == FloatPositive;
    }
    boolean operator!() const { return isZero(); }
    boolean sign() const { return !!(_vec[_fc->sind()] & _fc->smask()); }
    void sign(boolean v)
    {
    	if (v)
	    _vec[_fc->sind()] |= 1UL << _fc->sshift();
	else
	    _vec[_fc->sind()] &= ~(1UL << _fc->sshift());
    }
#ifdef copysign
#undef copysign
#endif
    void copysign(Float const &y)				// (A.1)
    {
    	if (y._vec[y._fc->sind()] & y._fc->smask())
	    _vec[_fc->sind()] |= 1UL << _fc->sshift();
	else
	    _vec[_fc->sind()] &= ~(1UL << _fc->sshift());
    }
    void scalb(Float const &n);					// (A.3)
    void logb();						// (A.4)
    void nextafter(Float const &y);				// (A.5)
    int fint(int);
    void frexp(Float *, Float *) const;
    FloatClass &floatclass() const { return *_fc; }
    int size() const { return _fc->size(); }
    uLong get(int i) const
	    { return (i < 0 || i >= _fc->size()) ? 0UL : _vec[i]; } 
    uLong getm(int i) const;
    uLong gete(int i) const;
};

// standard float classes
extern FloatClass fcsingle;		// IEEE-754 single precision
extern FloatClass fcdouble;		// IEEE-754 double precision
extern FloatClass fcquad;		// quad precision
extern FloatClass fcocta;		// octa precision

// type conversion
extern int floattoi(Float const &);
extern long floattol(Float const &);
extern float floattof(Float const &);
extern double floattod(Float const &);
extern Float dtofloat(FloatClass &fc, double d);
extern char const *floattostr(Float const &, int width = 0, int prec = 0, boolean sci = FALSE);
extern Float strtofloat(FloatClass &fc, char const *s, char const **end = NULL);

// misc functions
extern Float ceil(Float const &);
inline Float fabs(Float const &x) { Float t = x; t.sign(FALSE); return t; }
extern Float floor(Float const &);
extern Float fmod(Float const &, Float const &);
extern Float frexp(Float const &, Float *);
extern Float ldexp(Float const &, Float const &);
extern Float modf(Float const &, Float const &);

// square and square root
extern Float sqr(Float const &);
extern Float sqrt(Float const &);

// logrithmic
extern Float pow(Float const &, Float const &);		// y^x
extern Float log(Float const &);			// log base e
extern Float log10(Float const &);			// log base 10
extern Float log2(Float const &);			// log base 2
inline Float ln(Float const &x) { return log(x); }	// log base e
extern Float exp(Float const &);			// e ^ x

// trigonometric
extern Float sin(Float const &);
extern Float cos(Float const &);
extern Float tan(Float const &);
extern Float asin(Float const &);
extern Float acos(Float const &);
extern Float atan(Float const &);
extern Float atan(Float const &, Float const &);
extern Float sinh(Float const &);
extern Float cosh(Float const &);
extern Float tanh(Float const &);
extern Float asinh(Float const &);
extern Float acosh(Float const &);
extern Float atanh(Float const &);

// debug utility
extern void dumpfloat(char const *label, Float const &f, FILE *file,
	boolean fp);
inline void dumpfloat(char const *label, Float const &f, FILE *file)
    { dumpfloat(label, f, file, TRUE); }
inline void dumpfloat(char const *label, Float const &f)
    { dumpfloat(label, f, stdout, TRUE); }

inline void dumpfloat(char const *label, Float const &f, boolean fp,
	FILE *file)
    { dumpfloat(label, f, file, fp); }
inline void dumpfloat(char const *label, Float const &f, boolean fp)
    { dumpfloat(label, f, stdout, fp); }

#endif // __FLOATC_H_
