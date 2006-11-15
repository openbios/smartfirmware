
//
//	Copyright (C) 1991,1992  Thomas J. Merritt   All Rights Reserved.
//

#if defined(DUMBPTR) || defined(CONSTDUMBPTR) || defined(SMARTPTR) || !defined(__POINTER_H_)

#ifndef __POINTER_H_
#define __POINTER_H_
#endif

//$begincomment
#ifndef IMPLEMENT
//$endcomment

#include <stdlibx.h>

//$begincomment
#endif
//$endcomment

//$begincomment
#ifdef DUMBPTR
#ifndef IMPLEMENT
//$endcomment

//$begin declare_pointer(DUMBPTR, TYPE)

class DUMBPTR
{
    TYPE *_ptr;
public:
    DUMBPTR();
    DUMBPTR(TYPE *ptr);
    TYPE *operator=(TYPE *ptr) { _ptr = ptr; return ptr; }
    boolean operator==(DUMBPTR const &p) const { return _ptr == p._ptr; }
    TYPE *pointer() const { return _ptr; }
    operator TYPE *() const { return _ptr; }
    TYPE &operator()() const { return *_ptr; }
    TYPE &operator*() const { return *_ptr; }
    TYPE *operator->() const { return _ptr; }
};

//$end

//$begincomment
#else
//$endcomment

//$begin implement_pointer(DUMBPTR, TYPE)

DUMBPTR::DUMBPTR()
{
    _ptr = NULL;
}

DUMBPTR::DUMBPTR(TYPE *ptr)
{
    _ptr = ptr;
}
//$end

//$begincomment
#endif
#undef DUMBPTR
#undef TYPE
#endif
//$endcomment

//$begincomment
#ifdef CONSTDUMBPTR
#ifndef IMPLEMENT
//$endcomment

//$begin declare_constpointer(CONSTDUMBPTR, TYPE)

class CONSTDUMBPTR
{
    TYPE const *_ptr;
public:
    CONSTDUMBPTR();
    CONSTDUMBPTR(TYPE const *ptr);
    TYPE const *operator=(TYPE const *ptr) { _ptr = ptr; return ptr; }
    boolean operator==(CONSTDUMBPTR const &p) const { return _ptr == p._ptr; }
    TYPE const *pointer() const { return _ptr; }
    operator TYPE const *() const { return _ptr; }
    TYPE const &operator()() const { return *_ptr; }
    TYPE const &operator*() const { return *_ptr; }
    TYPE const *operator->() const { return _ptr; }
};

//$end

//$begincomment
#else
//$endcomment

//$begin implement_constpointer(CONSTDUMBPTR, TYPE)

CONSTDUMBPTR::CONSTDUMBPTR()
{
    _ptr = NULL;
}

CONSTDUMBPTR::CONSTDUMBPTR(TYPE const *ptr)
{
    _ptr = ptr;
}
//$end

//$begincomment
#endif
#undef CONSTDUMBPTR
#undef TYPE
#endif
//$endcomment

#define __objtype(PTRTYPE)  name2(__smartptr, PTRTYPE)

//$begincomment
#ifdef SMARTPTR
#ifndef IMPLEMENT
//$endcomment

//$begin declare_smartpointer(SMARTPTR, TYPE)

class __pointer(SMARTPTR) : public TYPE
{
public:
     int refs;
     __objtype(SMARTPTR)() : TYPE() { refs = 0; }
     void unref() { refs--; if (refs < 0) delete this; }
};

class SMARTPTR
{
    __objtype(SMARTPTR) *_ptr;
public:
    SMARTPTR();
    SMARTPTR(TYPE &ptr);
    ~SMARTPTR();
    TYPE *operator=(TYPE *) { _ptr = ptr; return ptr; }
    boolean operator==(SMARTPTR const &p) const { return _ptr == p._ptr; }
    TYPE *pointer() const { return _ptr; }
    operator TYPE *() const { return _ptr; }
    TYPE &operator()() const { return *_ptr; }
    TYPE *operator*() const { return _ptr; }
    TYPE &operator->() const { return *_ptr; }
};

//$end

//$begincomment
#else
//$endcomment

//$begin implement_smartpointer(SMARTPTR, TYPE)

SMARTPTR::SMARTPTR()
{
    _ptr = NULL;
}

SMARTPTR::SMARTPTR(TYPE *ptr)
{
    _ptr = ptr;
}
//$end

//$begincomment
#endif 
#undef SMARTPTR
#undef TYPE
#endif
//$endcomment

#endif // __POINTER_H_
