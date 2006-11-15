
//
//	Copyright (C) 1992  Thomas J. Merritt
//

#if defined(TABLE) || !defined(__CACHETABLE_H_)

#ifndef __CACHETABLE_H_
#define __CACHETABLE_H_
#endif

//$begincomment
#ifndef IMPLEMENT
//$endcomment

#include <stdlibx.h>

long _hashtable_bumpfunc(long);

#define __cacheelem(TABLE)  name2(__cachetable_elem, TABLE)

//$begin declare_cachetable(TABLE, ELEM, KEY)

struct __cacheelem(TABLE) : public ELEM
{
    __cacheelem(TABLE) *_link;
    __cacheelem(TABLE) *_lruf;
    __cacheelem(TABLE) *_lrub;
    boolean _locked;
    __cacheelem(TABLE)(KEY const &k) : ELEM(k) { }
};

class TABLE
{
    __cacheelem(TABLE) **_chains;
    long _numchains;
    long _size;
    __cacheelem(TABLE) *_cur;
    long _maxsize;
    __cacheelem(TABLE) *_mru;
    __cacheelem(TABLE) *_lru;

    // private member functions
    void init(long maxsize);
    void bump(long chains = 0);
    __cacheelem(TABLE) *newelem(KEY const &k);
    void deleteelem(__cacheelem(TABLE) *);
    __cacheelem(TABLE) *lookup(KEY const &k) const;
    void use(__cacheelem(TABLE) *elem);
    boolean remove(__cacheelem(TABLE) *elem);
    boolean lock(__cacheelem(TABLE) *elem) const;
    boolean unlock(__cacheelem(TABLE) *elem) const;
    boolean locked(__cacheelem(TABLE) *elem) const;
public:
    TABLE();
    TABLE(long maxsize);
    TABLE(TABLE const &);
    ~TABLE();
    void clear();
    void assign(TABLE const &);
    boolean compare(TABLE const &) const;
    long maxsize() const { return _maxsize; }
    void maxsize(long sz);
    long size() const { return _size; }
    boolean insert(KEY const &k);
    boolean find(KEY const &k);
    ELEM &cur() const { return *_cur; }
    boolean remove() { return remove(_cur); }
    boolean remove(KEY const &k) { return remove(lookup(k)); }
    boolean lock() const { return lock(_cur); }
    boolean lock(KEY const &k) const { return lock(lookup(k)); }
    boolean unlock() const { return unlock(_cur); }
    boolean unlock(KEY const &k) const { return unlock(lookup(k)); }
    boolean locked() const { return locked(_cur); }
    boolean locked(KEY const &k) const { return locked(lookup(k)); }
    void flush() { clear(); }
    ELEM &elt(KEY const &k);
    ELEM &elem(long i) const;
    boolean operator+=(KEY const &k) { return insert(k); }
    boolean operator-=(KEY const &k) { return remove(k); }
    TABLE &operator=(TABLE const &t) { assign(t); return *this; }
    boolean operator==(TABLE const &t) const { return compare(t); }
    boolean operator!=(TABLE const &t) const { return compare(t); }
    ELEM &operator[](KEY const &k) { return elt(k); }
    ELEM &operator()() const { return cur(); }
    ELEM &operator*() const { return cur(); }
    ELEM *operator->() const { return &cur(); }
};
//$end

//$begincomment
#else
//$endcomment

//$begin implement_cachetable(TABLE, ELEM, KEY, HASHFUNC)
void
TABLE::init(long maxsize)
{
    _maxsize = maxsize;
    _size = 0;
    _numchains = 0;
    _chains = NULL;
    _mru = NULL;
    _lru = NULL;
    _cur = NULL;
    bump();
}

void
TABLE::bump(long chains)
{
    if (chains == 0)
    {
	chains = _hashtable_bumpfunc(_size);

	if (chains == 0)
	    chains = 1;
    }

    if (_numchains >= chains)
	return;

    __cacheelem(TABLE) **nchains = new __cacheelem(TABLE) *[chains];
	long i;

    for (i = 0; i < chains; i++)
    	nchains[i] = NULL;

    __cacheelem(TABLE) *list = NULL;
    __cacheelem(TABLE) *last = NULL;

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
    	unsigned long hv = HASHFUNC((KEY)*list) % chains;
	list->_link = nchains[hv];
	nchains[hv] = list;
    }

    if (_chains != NULL)
	delete[/*_numchains*/] _chains;

    _chains = nchains;
    _numchains = chains;
}

__cacheelem(TABLE) *
TABLE::newelem(KEY const &k)
{
    if (_size >= _maxsize)
    {
    	// find an element to kill
	__cacheelem(TABLE) *elem;

	for (elem = _lru; elem != NULL; elem = elem->_lrub)
	    if (!elem->_locked)
	    	break;

	if (elem != NULL)
	    remove(elem);
    }

    __cacheelem(TABLE) *elem = new __cacheelem(TABLE)(k);
    elem->_lruf = _mru;
    elem->_lrub = NULL;
    elem->_locked = FALSE;

    if (_mru != NULL)
    	_mru->_lrub = elem;
    else
    {
	_lru = elem;
	_lru->_lruf = NULL;
    }

    _mru = elem;
    return elem;
}

void 
TABLE::deleteelem(__cacheelem(TABLE) *elem)
{
    // remove from list
    if (elem->_lrub == NULL)
    {
    	_mru = elem->_lruf;

	if (_mru != NULL)
	    _mru->_lrub = NULL;
    }
    else
	elem->_lrub->_lruf = elem->_lruf;

    if (elem->_lruf == NULL)
    {
	_lru = elem->_lrub;

	if (_lru != NULL)
	    _lru->_lruf = NULL;
    }
    else
	elem->_lruf->_lrub = elem->_lrub;

    delete elem;
}

__cacheelem(TABLE) *
TABLE::lookup(KEY const &k) const
{
    unsigned long hashval = HASHFUNC(k) % _numchains;

    for (__cacheelem(TABLE) *elem = _chains[hashval]; elem != NULL;
	    elem = elem->_link)
	if (*elem == k)
	    return elem;
	
    return NULL;
}

void 
TABLE::use(__cacheelem(TABLE) *elem)
{
    if (elem == NULL)
	return;

    // remove from list
    if (elem->_lrub == NULL)
    	return;

    elem->_lrub->_lruf = elem->_lruf;

    if (elem->_lruf == NULL)
    {
    	_lru = elem->_lrub;
	_lru->_lruf = NULL;
    }
    else
    	elem->_lruf->_lrub = elem->_lrub;

    // add back to list at mru spot
    elem->_lruf = _mru;
    _mru->_lrub = elem;
    elem->_lrub = NULL;
    _mru = elem;
}

boolean 
TABLE::remove(__cacheelem(TABLE) *item)
{
    if (item == NULL)
	return FALSE;

    // remove from hash chains
    unsigned long hashval = HASHFUNC((KEY &)*item) % _numchains;
    __cacheelem(TABLE) *prev = NULL;
    __cacheelem(TABLE) *elem = NULL;

    for (elem = _chains[hashval]; elem != NULL ;
	    elem = elem->_link)
	if (elem == item)
	    break;
	else
	    prev = elem;

    if (elem == NULL)
	return FALSE;

    if (prev != NULL)
    	prev->_link = elem->_link;
    else
    	_chains[hashval] = elem->_link;

    deleteelem(elem);
    _size--;

    if (_cur == elem)
    	_cur = NULL;

    return TRUE;
}

boolean
TABLE::lock(__cacheelem(TABLE) *item) const
{
    if (item == NULL)
    	return FALSE;

    if (item->_locked)
	return FALSE;

    item->_locked = TRUE;
    return TRUE;
}

boolean
TABLE::unlock(__cacheelem(TABLE) *item) const
{
    if (item == NULL)
    	return FALSE;

    if (!item->_locked)
	return FALSE;

    item->_locked = FALSE;
    return TRUE;
}

boolean
TABLE::locked(__cacheelem(TABLE) *item) const
{
    if (item == NULL)
    	return FALSE;

    return item->_locked;
}

TABLE::TABLE()
{
    init(64);
}

TABLE::TABLE(long maxsize)
{
    init(maxsize);
}

TABLE::TABLE(TABLE const &t)
{
    init(t.maxsize());
    assign(t);
}

TABLE::~TABLE()
{
    clear();
    delete[/*_numchains*/] _chains;
}

void
TABLE::clear()
{
    for (__cacheelem(TABLE) *elem = _lru; elem != NULL; )
    {
    	__cacheelem(TABLE) *next = elem->_lrub;
	remove(elem);
	elem = next;
    }
}

void
TABLE::assign(TABLE const &t)
{
    clear();
    long i = 0;

    while (1)
    {
	ELEM &e = t.elem(i++);

	if (&e == NULL)
	    break;

	insert((KEY)e);
	*(ELEM *)_cur = e;
    }
}

boolean
TABLE::compare(TABLE const &t) const
{
    if (_size != t._size)
	return FALSE;

    for (long i = 0; i < _numchains; i++)
    	for (__cacheelem(TABLE) *p = _chains[i]; p != NULL; p = p->_link)
	{
	    ELEM *e = t.lookup((KEY)*p);

	    if (e == NULL || !(*e == *p))
		return FALSE;
	}

    return TRUE;
}

void 
TABLE::maxsize(long sz)
{
    for (__cacheelem(TABLE) *elem = _lru; elem != NULL && sz < _size;
	    elem = elem->_lrub)
	if (!elem->_locked)
	    remove(elem);

    _maxsize = sz;
}

boolean
TABLE::insert(KEY const &k)
{
    unsigned long hashval = HASHFUNC(k) % _numchains;

    _cur = NULL;
    __cacheelem(TABLE) *elem;

    for (elem = _chains[hashval]; elem != NULL;
	    elem = elem->_link)
	if (*elem == k)
	    return FALSE;

    elem = newelem(k);
    elem->_link = _chains[hashval];
    _chains[hashval] = elem;
    _cur = elem;
    _size++;

    if (_size << 1 > (_numchains << 1) + _numchains)
    	bump();

    return TRUE;
}

boolean
TABLE::find(KEY const &k)
{
    _cur = lookup(k);
    use(_cur);
    return _cur != NULL;
}

ELEM &
TABLE::elt(KEY const &k)
{
    if (!find(k))
	insert(k);

    return cur();
}

ELEM &
TABLE::elem(long i) const
{
    for (__cacheelem(TABLE) *elem = _mru; elem != NULL; elem = elem->_lruf)
    	if (i-- == 0)
	    return *elem;

    return *(ELEM *)NULL;
}
//$end

//$begincomment
#undef HASHFUNC
#endif
#undef TABLE
#undef ELEM
#undef KEY
//$endcomment

#endif // __CACHETABLE_H_
