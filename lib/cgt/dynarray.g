
//
//	Copyright (c) 1991,1993 by Thomas J. Merritt and Parag Patel.
//	All Rights Reserved.
//

#if defined(SARRAY) || defined(ARRAY) || defined(BIGARRAY) || !defined(__DYNARRAY_H_)

#ifndef __DYNARRAY_H_
#define __DYNARRAY_H_
#endif

#ifndef __STDLIBX_H_
#include <stdlib.h>
#include <stdlibx.h>
#endif

//$begincomment
#ifdef SARRAY
#ifndef IMPLEMENT
//$endcomment

//$begin declare_sarray(SARRAY, TYPE)

class SARRAY
{
    TYPE *_array;
    int _size;
public:
    SARRAY();
    SARRAY(int length);
    SARRAY(SARRAY const &);
    ~SARRAY();
    void assign(SARRAY const &);
    void swap(SARRAY &);
    boolean compare(SARRAY const &) const;
    int size() const { return _size; }
    void reset(int newlen = 0);
    void resize(int newlen = 0) { reset(newlen); }
    TYPE &elem(int e)
    {
	if (e < 0) crash("negative sarray reference");
	else if (e >= _size) reset(e + 1);
	return _array[e];
    }
    TYPE &elt(int e) const { return _array[e]; }
    TYPE &end() { int sz = _size; reset(sz + 1); return _array[sz]; }
    TYPE *getarr() const { return _array; }
    SARRAY &operator=(SARRAY const &a) { assign(a); return *this; }
    boolean operator==(SARRAY const &a) const { return compare(a); }
    boolean operator!=(SARRAY const &a) const { return !compare(a); }
    TYPE &operator[](int e) { return elem(e); }
    TYPE *operator()() const { return getarr(); }
};

//$end

//$begincomment
#else
//$endcomment

//$begin implement_sarray(SARRAY, TYPE)

void
SARRAY::reset(int newlen)
{
    if (newlen == _size)
	return;

    if (newlen == 0)
    {
	if (_array != NULL)
	    delete[/*_size*/] _array;

	_array = NULL;
	_size = 0;
	return;
    }

    TYPE *narray = new TYPE[newlen];

    if (_array != NULL)
    {
	for (int i = 0; i < _size && i < newlen; i++)
	    narray[i] = _array[i];

	delete[/*_size*/] _array;
    }

    _array = narray;
    _size = newlen;
}

SARRAY::SARRAY()
{
    _array = NULL;
    _size = 0;
}

SARRAY::SARRAY(int length)
{
    _array = NULL;
    _size = 0;
    reset(length);
}

SARRAY::SARRAY(SARRAY const &a)
{
    _array = NULL;
    _size = 0;
    int sz = a.size();
    reset(sz);

    for (int i = 0; i < sz; i++)
    	_array[i] = a._array[i];
}

SARRAY::~SARRAY()
{
    if (_array)
	delete[/*_size*/] _array;
}

void
SARRAY::assign(SARRAY const &a)
{
    reset(a.size());

    for (int i = 0; i < _size; i++)
    	_array[i] = a._array[i];
}

void
SARRAY::swap(SARRAY &x)
{
    int s = _size;
    TYPE *arr = _array;
    _size = x._size;
    _array = x._array;
    x._size = s;
    x._array = arr;
}

boolean
SARRAY::compare(SARRAY const &a) const
{
    if (_size != a._size)
	return FALSE;

    for (int i = 0; i < _size; i++)
    	if (!(_array[i] == a._array[i]))
	    return FALSE;

    return TRUE;
}
//$end

//$begincomment
#endif
#undef SARRAY
#undef TYPE
#endif

#ifdef ARRAY
#ifndef IMPLEMENT
//$endcomment

#ifndef NULL
#define NULL 0
#endif

//$begin declare_array(ARRAY, TYPE, BUMP)

class ARRAY
{
    TYPE *_array;
    int _arrlen;
    int _size;
    void doexpand(int leng);
    void expand(int leng)
	{ if (leng > _arrlen) doexpand(leng);
	  else if (leng > _size) _size = leng; }
public:
    ARRAY();
    ARRAY(int length);
    ARRAY(ARRAY const &);
    ~ARRAY();
    void assign(ARRAY const &);
    void swap(ARRAY &);
    boolean compare(ARRAY const &) const;
    int size() const { return _size; }
    void reset(int newlen = 0);
    void resize(int newlen = 0);
    TYPE &elem(int e)
    {
	if (e < 0) crash("negative dynarray reference");
	else if (e >= _size) expand(e + 1);
	return _array[e];
    }
    TYPE &elt(int e) const { return _array[e]; }
    TYPE &end() { int sz = _size; expand(sz + 1); return _array[sz]; }
    TYPE *getarr() const { return _array; }
    ARRAY &operator=(ARRAY const &a) { assign(a); return *this; }
    boolean operator==(ARRAY const &a) const { return compare(a); }
    boolean operator!=(ARRAY const &a) const { return !compare(a); }
    TYPE &operator[](int e) { return elem(e); }
    TYPE *operator()() const { return getarr(); }
};

//$end

//$begincomment
#else
//$endcomment

//$begin implement_array(ARRAY, TYPE, BUMP)

void
ARRAY::doexpand(int size)
{
    if (size > _size)
    {
	if (size > _arrlen)
	{
	    int nlen = _arrlen > 0 ? _arrlen : 1;

	    while (nlen < size)
		if (nlen < BUMP)
		    nlen <<= 1;
		else
		    nlen += BUMP;

	    TYPE *narray = new TYPE[nlen];

	    if (_array != NULL)
	    {
		for (int i = 0; i < _arrlen; i++)
		    narray[i] = _array[i];

		delete[/*_arrlen*/] _array;
	    }

	    _array = narray;
	    _arrlen = nlen;
	}

	_size = size;
    }
}

ARRAY::ARRAY()
{
    _array = NULL;
    _arrlen = 0;
    _size = 0;
}

ARRAY::ARRAY(int length)
{
    _array = NULL;
    _arrlen = 0;
    _size = 0;
    expand(length);
}

ARRAY::ARRAY(ARRAY const &a)
{
    _array = NULL;
    _arrlen = 0;
    _size = 0;
    int sz = a.size();
    expand(sz);

    for (int i = 0; i < sz; i++)
    	_array[i] = a._array[i];
}

ARRAY::~ARRAY()
{
    if (_array)
	delete[/*_arrlen*/] _array;
}

void
ARRAY::assign(ARRAY const &a)
{
    reset(a.size());

    for (int i = 0; i < _size; i++)
    	_array[i] = a._array[i];
}

void
ARRAY::swap(ARRAY &x)
{
    int s = _size;
    int l = _arrlen;
    TYPE *arr = _array;
    _size = x._size;
    _arrlen = x._arrlen;
    _array = x._array;
    x._size = s;
    x._arrlen = l;
    x._array = arr;
}

boolean
ARRAY::compare(ARRAY const &a) const
{
    if (_size != a._size)
	return FALSE;

    for (int i = 0; i < _size; i++)
    	if (!(_array[i] == (TYPE)a._array[i]))
	    return FALSE;

    return TRUE;
}

void
ARRAY::reset(int newlen)
{
    int len = newlen > 0 ? newlen : 0;

    if (len > 0)
	expand(len);
    else
    {
    	if (_array != NULL)
	    delete[/*_arrlen*/] _array;

	_array = NULL;
	_arrlen = 0;
    }

    _size = len;
}

void
ARRAY::resize(int newlen)
{
    int len = newlen > 0 ? newlen : 0;
    expand(len);
    _size = len;
}
//$end

//$begincomment
#endif
#undef ARRAY
#undef TYPE
#undef BUMP
#endif

#ifdef BIGARRAY
#ifndef IMPLEMENT
//$endcomment

//$begin declare_bigarray(BIGARRAY, TYPE, CHUNK, BUMP)

class BIGARRAY
{
    TYPE **_array;
    long _arrlen;
    long _size;
    void doexpand(long leng);
    void expand(long leng)
	{ if (leng > _arrlen) doexpand(leng);
	  else if (leng > _size) _size = leng; }
public:
    BIGARRAY();
    BIGARRAY(long length);
    BIGARRAY(BIGARRAY const &);
    ~BIGARRAY();
    void assign(BIGARRAY const &);
    void swap(BIGARRAY &);
    boolean compare(BIGARRAY const &) const;
    long size() const { return _size; }
    void reset(long newlen = 0);
    void resize(long newlen = 0) { reset(newlen); }
    TYPE &elem(long e)
    {
	if (e < 0) crash("negative bigarray reference");
	else if (e >= _size) expand(e + 1);
	return _array[e / CHUNK][e % CHUNK];
    }
    TYPE &elt(long e) const { return _array[e / CHUNK][e % CHUNK]; }
    TYPE &end()
    {
	long sz = _size;
	expand(sz + 1);
	return _array[sz / CHUNK][sz % CHUNK];
    }
    BIGARRAY &operator=(BIGARRAY const &a) { assign(a); return *this; }
    boolean operator==(BIGARRAY const &a) const { return compare(a); }
    boolean operator!=(BIGARRAY const &a) const { return !compare(a); }
    TYPE &operator[](long e) { return elem(e); }
};

//$end

//$begincomment
#else
//$endcomment

//$begin implement_bigarray(BIGARRAY, TYPE, CHUNK, BUMP)

void
BIGARRAY::doexpand(long size)
{
    if (size > _size)
    {
	if (size > _arrlen)
	{
	    long nlen = _arrlen > 0 ? _arrlen : 1;

	    while (nlen < size)
		if (nlen < BUMP)
		    nlen <<= 1;
		else
		    nlen += BUMP;

	    long _lastchunklen = _arrlen % CHUNK;
	    long _lastchunkindex = _arrlen / CHUNK;
	    long nlastchunklen = nlen % CHUNK;
	    long nlastchunkindex = nlen / CHUNK;
	    long nchunklen = nlastchunklen;

	    if (nlastchunkindex != _lastchunkindex || _array == NULL)
	    {
	    	TYPE **narray = new TYPE *[nlastchunkindex + 1];

		if (_array != NULL)
		{
		    for (long i = 0; i <= _lastchunkindex; i++)
			narray[i] = _array[i];

		    delete[/*_lastchunkindex + 1*/] _array;
		    _array = narray;
		}

	    	nchunklen = CHUNK;
	    }

	    TYPE *nchunk = new TYPE[nchunklen];

	    if (_array[_lastchunkindex] != NULL)
	    {
		for (long i = 0; i < _lastchunklen; i++)
		    nchunk[i] = _array[_lastchunkindex][i];

		delete[/*_lastchunklen*/] _array[_lastchunkindex];
	    }

	    _array[_lastchunkindex] = nchunk;

	    for (long i = _lastchunkindex + 1; i <= nlastchunkindex; i++)
		if (i != nlastchunkindex)
		    _array[i] = new TYPE[CHUNK];
		else if ((size % CHUNK) != 0)
		    _array[i] = new TYPE[size % CHUNK];
		else
		    _array[i] = NULL;

	    _arrlen = nlen;
	}

	_size = size;
    }
}

BIGARRAY::BIGARRAY()
{
    _array = NULL;
    _arrlen = 0;
    _size = 0;
}

BIGARRAY::BIGARRAY(long length)
{
    _array = NULL;
    _arrlen = 0;
    _size = 0;
    expand(length);
}

BIGARRAY::BIGARRAY(BIGARRAY const &a)
{
    _array = NULL;
    _arrlen = 0;
    _size = 0;
    long sz = a.size();
    expand(sz);

    for (long i = 0; i < sz; i++)
    	_array[i / CHUNK][i % CHUNK] = a._array[i / CHUNK][i % CHUNK];
}

BIGARRAY::~BIGARRAY()
{
    if (_array != NULL)
    {
    	// We really want an int here instead of a long so that the array
	// indexing is efficient on machines where int and long are not the
	// same type.
	int arrlen = (int)(_arrlen / CHUNK);

    	for (int i = 0; i <= arrlen; i++)
	    if (_array[i] != NULL)
	    	if (i != arrlen)
		    delete[/*CHUNK*/] _array[i];
		else
		    delete[/*_arrlen % CHUNK*/] _array[i];

	delete[/*arrlen + 1*/] _array;
    }
}

void
BIGARRAY::assign(BIGARRAY const &a)
{
    reset(a.size());

    for (long i = 0; i < _size; i++)
    	_array[i / CHUNK][i % CHUNK] = a._array[i / CHUNK][i % CHUNK];
}

void
BIGARRAY::swap(BIGARRAY &x)
{
    long s = _size;
    long l = _arrlen;
    TYPE **arr = _array;
    _size = x._size;
    _arrlen = x._arrlen;
    _array = x._array;
    x._size = s;
    x._arrlen = l;
    x._array = arr;
}

boolean
BIGARRAY::compare(BIGARRAY const &a) const
{
    if (_size != a._size)
	return FALSE;

    for (long i = 0; i < _size; i++)
    	if (!(_array[i / CHUNK][i % CHUNK] == a._array[i / CHUNK][i % CHUNK]))
	    return FALSE;

    return TRUE;
}

void
BIGARRAY::reset(long newlen)
{
    long len = newlen > 0 ? newlen : 0;
    expand(len);
    _size = len;
}

//$end

//$begincomment
#endif
#undef BIGARRAY
#undef TYPE
#undef CHUNK
#undef BUMP
#endif
//$endcomment

//$begincomment
#if 0
//$endcomment

declare_array(Charbuf, char, 1024);
declare_bigarray(bCharbuf, char, 16384, 1024);

//$begincomment
#endif
//$endcomment

#endif // __DYNARRAY_H_
