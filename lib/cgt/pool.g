

//
//	Copyright (C) 1991  Thomas J. Merritt
//

#if defined(SPOOL) || defined(POOL) || !defined(__POOL_H_)

#ifndef __POOL_H_
#define __POOL_H_
#endif

//$begincomment
#ifndef IMPLEMENT
//$endcomment

#include <stdlibx.h>

long _hashtable_bumpfunc(long);

//$begincomment
#endif
//$endcomment

//$begincomment
#ifdef SPOOL
#ifndef IMPLEMENT
//$endcomment

#define __spoolelemtype(SPOOL)  name2(__spool_elem, SPOOL)

//$begin declare_spool(SPOOL, ELEM)

class __spoolelemtype(SPOOL) : public ELEM
{
public:
     __spoolelemtype(SPOOL) *_link;
     __spoolelemtype(SPOOL)(ELEM const &k) : ELEM(k) { _link = NULL; }
};

class SPOOL	// check comment striping
{		// make sure "qutoes" don't cause a problem \\
    // keep gcc happy
    __spoolelemtype(SPOOL) **_chains;
    long _numchains;
    long _size;
    __spoolelemtype(SPOOL) *_cur;
    void bump(long chains = 0);
    __spoolelemtype(SPOOL) *lookup(ELEM const &) const;
    boolean find(ELEM const &e) { _cur = lookup(e); return _cur != NULL; }
    boolean insert(ELEM const &);
public:
    SPOOL();
    SPOOL(SPOOL const &);
    ~SPOOL();
    void clear();
    void assign(SPOOL const &);
    boolean compare(SPOOL const &) const;
    void add(SPOOL const &);
    long size() const { return _size; }
    ELEM &get(ELEM const &k) { insert(k); return *_cur; }
    ELEM &get(ELEM const &k, boolean &x) { x = !insert(k); return *_cur; }
    SPOOL &operator=(SPOOL const &p) { assign(p); return *this; }
    SPOOL &operator+=(SPOOL const &p) { add(p); return *this; }
    boolean operator==(SPOOL const &p) const { return compare(p); }
    boolean operator!=(SPOOL const &p) const { return !compare(p); }
};
//$end

//$begincomment
#else
//$endcomment

//$begin implement_spool(SPOOL, ELEM, HASHFUNC)

void
SPOOL::bump(long chains)
{
    if (chains == 0)
    {
	chains = _hashtable_bumpfunc(_size);

	if (chains == 0)
	    chains = 1;
    }

    if (_numchains >= chains)
	return;

    __spoolelemtype(SPOOL) **nchains = new __spoolelemtype(SPOOL) *[chains];
    long i;

    for (i = 0; i < chains; i++)
    	nchains[i] = NULL;

    __spoolelemtype(SPOOL) *list = NULL;
    __spoolelemtype(SPOOL) *last = NULL;

    for (i = 0; i < _numchains; i++)
    	if (_chains[i] != NULL)
	{
	    if (last != NULL)
		last->_link = _chains[i];
	    else
		list = _chains[i];

	    for (last = _chains[i]; last->_link != NULL; last = last->_link)
	    	;
	}

    for (last = list; last != NULL; )
    {
    	list = last;
	last = last->_link;
    	unsigned long hv = HASHFUNC(*list) % chains;
	list->_link = nchains[hv];
	nchains[hv] = list;
    }

    if (_chains != NULL)
	delete[/*_numchains*/] _chains;

    _chains = nchains;
    _numchains = chains;
}

SPOOL::SPOOL()
{
    _chains = NULL;
    _numchains = 0;
    _size = 0;
    _cur = NULL;
    bump();
}

SPOOL::SPOOL(SPOOL const &p)
{
    _chains = NULL;
    _numchains = 0;
    _size = 0;
    _cur = NULL;
    bump();
    add(p);
}

SPOOL::~SPOOL()
{
    clear();
    delete[/*_numchains*/] _chains;
}

void
SPOOL::clear()
{
    for (long i = 0; i < _numchains; i++)
    {
    	for (__spoolelemtype(SPOOL) *p = _chains[i], *q = NULL; p != NULL;
		p = q)
	{
	    q = p->_link;
	    delete p;
	}

	_chains[i] = NULL;
    }

    _size = 0;
    _cur = NULL;
}

void
SPOOL::assign(SPOOL const &p)
{
    clear();
    add(p);
}

boolean 
SPOOL::compare(SPOOL const &t) const
{
    if (_size != t._size)
	return FALSE;

    for (long i = 0; i < _numchains; i++)
    	for (__spoolelemtype(SPOOL) *p = _chains[i]; p != NULL; p = p->_link)
	{
	    __spoolelemtype(SPOOL) *e = t.lookup(*p);

	    if (e == NULL || !(*e == *p))
		return FALSE;
	}

    return TRUE;
}

void
SPOOL::add(SPOOL const &t)
{
    for (long i = 0; i < t._numchains; i++)
    	for (__spoolelemtype(SPOOL) *p = t._chains[i]; p != NULL; p = p->_link)
	    if (insert(*p))
		*_cur = *p;
}

__spoolelemtype(SPOOL) *
SPOOL::lookup(ELEM const &k) const
{
    unsigned long hv = HASHFUNC(k) % _numchains;

    for (__spoolelemtype(SPOOL) *p = _chains[hv]; p != NULL; p = p->_link)
    	if (*p == k)
	    return p;

    return NULL;
}

boolean
SPOOL::insert(ELEM const &k)
{
    unsigned long hv = HASHFUNC(k) % _numchains;
    __spoolelemtype(SPOOL) *p;

    for (p = _chains[hv]; p != NULL; p = p->_link)
    	if (*p == k)
	{
	    _cur = p;
	    return FALSE;
	}

    p = new __spoolelemtype(SPOOL)(k);
    p->_link = _chains[hv];
    _chains[hv] = p;
    _cur = p;
    _size++;

    if (_size << 1 > (_numchains << 1) + _numchains)
    	bump();

    return TRUE;
}
//$end

//$begincomment
#undef HASHFUNC
#endif
#undef SPOOL
#undef ELEM
#endif
//$endcomment

//$begincomment
#ifdef POOL
#ifndef IMPLEMENT
//$endcomment

#define __poolelemtype(POOL)  name2(__pool_elem, POOL)

//$begin declare_pool(POOL, ELEM)

class __poolelemtype(POOL) : public ELEM
{
public:
     int _refs;
     __poolelemtype(POOL) *_link;
     __poolelemtype(POOL)(ELEM const &k) : ELEM(k) { _link = NULL; _refs = 0; }
};

class POOL	
{
    __poolelemtype(POOL) **_chains;
    long _numchains;
    long _size;
    __poolelemtype(POOL) *_cur;
    void bump(long chains = 0);
    __poolelemtype(POOL) *lookup(ELEM const &) const;
    boolean find(ELEM const &e) { _cur = lookup(e); return _cur != NULL; }
    boolean insert(ELEM const &);
    boolean remove(__poolelemtype(POOL) *);
public:
    POOL();
    POOL(POOL const &);
    ~POOL();
    void clear();
    void assign(POOL const &);
    boolean compare(POOL const &) const;
    void add(POOL const &);
    long size() const { return _size; }
    ELEM &get(ELEM const &k) { insert(k); _cur->_refs++; return *_cur; }
    ELEM &get(ELEM const &k, boolean &x)
	   { x = !insert(k); _cur->_refs++; return *_cur; }
    void put(ELEM const &);
    int refs(ELEM const &k) { return ((__poolelemtype(POOL) *)&k)->_refs; }
    void ref(ELEM const &k) { ((__poolelemtype(POOL) *)&k)->_refs++; }
    void unref(ELEM const &k);
    POOL &operator=(POOL const &p) { assign(p); return *this; }
    POOL &operator+=(POOL const &p) { add(p); return *this; }
    boolean operator==(POOL const &p) const { return compare(p); }
    boolean operator!=(POOL const &p) const { return !compare(p); }
};
//$end

//$begincomment
#else
//$endcomment

//$begin implement_pool(POOL, ELEM, HASHFUNC)

void
POOL::bump(long chains)
{
    if (chains == 0)
    {
	chains = _hashtable_bumpfunc(_size);

	if (chains == 0)
	    chains = 1;
    }

    if (_numchains >= chains)
	return;

    __poolelemtype(POOL) **nchains = new __poolelemtype(POOL) *[chains];
    long i;

    for (i = 0; i < chains; i++)
    	nchains[i] = NULL;

    __poolelemtype(POOL) *list = NULL;
    __poolelemtype(POOL) *last = NULL;

    for (i = 0; i < _numchains; i++)
    	if (_chains[i] != NULL)
	{
	    if (last != NULL)
		last->_link = _chains[i];
	    else
		list = _chains[i];

	    for (last = _chains[i]; last->_link != NULL; last = last->_link)
	    	;
	}

    for (last = list; last != NULL; )
    {
    	list = last;
	last = last->_link;
    	unsigned long hv = HASHFUNC(*list) % chains;
	list->_link = nchains[hv];
	nchains[hv] = list;
    }

    if (_chains != NULL)
	delete[/*_numchains*/] _chains;

    _chains = nchains;
    _numchains = chains;
}

POOL::POOL()
{
    _chains = NULL;
    _numchains = 0;
    _size = 0;
    _cur = NULL;
    bump();
}

POOL::POOL(POOL const &t)
{
    _chains = NULL;
    _numchains = 0;
    _size = 0;
    _cur = NULL;
    bump();
    add(t);
}

POOL::~POOL()
{
    clear();
    delete[/*_numchains*/] _chains;
}

__poolelemtype(POOL) *
POOL::lookup(ELEM const &k) const
{
    unsigned long hv = HASHFUNC(k) % _numchains;

    for (__poolelemtype(POOL) *p = _chains[hv]; p != NULL; p = p->_link)
    	if (*p == k)
	    return p;

    return NULL;
}

boolean
POOL::insert(ELEM const &k)
{
    unsigned long hv = HASHFUNC(k) % _numchains;
    __poolelemtype(POOL) *p;

    for (p = _chains[hv]; p != NULL; p = p->_link)
    	if (*p == k)
	{
	    _cur = p;
	    return FALSE;
	}

    p = new __poolelemtype(POOL)(k);
    p->_link = _chains[hv];
    _chains[hv] = p;
    _cur = p;
    _size++;

    if (_size << 1 > (_numchains << 1) + _numchains)
    	bump();

    return TRUE;
}

boolean
POOL::remove(__poolelemtype(POOL) *e)
{
    unsigned long hv = HASHFUNC(*e) % _numchains;

    for (__poolelemtype(POOL) *p = _chains[hv], *q = NULL; p != NULL;
	    p = p->_link)
    	if (p == e)
	{
	    if (_cur == p)
		_cur = NULL;

	    if (q != NULL)
		q->_link = p->_link;
	    else
		_chains[hv] = p->_link;

	    delete p;
	    _size--;
	    return TRUE;
	}
	else
	    q = p;

    return FALSE;
}

void
POOL::clear()
{
    for (long i = 0; i < _numchains; i++)
    {
    	for (__poolelemtype(POOL) *p = _chains[i], *q = NULL; p != NULL; p = q)
	{
	    q = p->_link;
	    delete p;
	}

	_chains[i] = NULL;
    }

    _size = 0;
    _cur = NULL;
}

void
POOL::assign(POOL const &t)
{
    clear();
    add(t);
}

boolean 
POOL::compare(POOL const &t) const
{
    if (_size != t._size)
	return FALSE;

    for (long i = 0; i < _numchains; i++)
    	for (__poolelemtype(POOL) *p = _chains[i]; p != NULL; p = p->_link)
	{
	    __poolelemtype(POOL) *e = t.lookup(*p);

	    if (e == NULL || !(*e == *p))
		return FALSE;
	}

    return TRUE;
}

void
POOL::add(POOL const &t)
{
    for (long i = 0; i < t._numchains; i++)
    	for (__poolelemtype(POOL) *p = t._chains[i]; p != NULL; p = p->_link)
	    if (insert(*p))
		*_cur = *p;
}

void
POOL::put(ELEM const &k)
{
    if (!find(k))
	return;

    if (--(_cur->_refs) == 0)
    	remove(_cur);
}

// ref and unref are like get and put except they assume that the object
// is in the pool to start with.  We optimize this case by not doing the
// object search.  refs returns the number of references for folks that
// might need this info.
void
POOL::unref(ELEM const &k)
{
    __poolelemtype(POOL) *elem = (__poolelemtype(POOL) *)&k;

    if (--(elem->_refs) == 0)
	remove(elem);
}
//$end

//$begincomment
#undef HASHFUNC
#endif
#undef POOL
#undef ELEM
#endif
//$endcomment

extern char const *strget(char const *);
extern char const *strgeti(char const *);
extern void strput(char const *);
extern void strputi(char const *);
extern void clearstringpool();

#endif // __POOL_H_
