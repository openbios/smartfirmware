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

// This is an example of how to use SmartCollect inside a C++ program.
// Very little needs to be added to support garbage-collection (GC) within
// a class or a hierarchy of classes.  The additions made to the sample
// class "Object" below are simply applied to a base class for all derived
// objects to become GC safe.

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gcext.h"

class Object
{
private:
    // The two static class vars are to describe this Object to SmartCollect.
    // They are static class vars so that they have private access to the
    // two pointers _left and _right, or rather, access to their offsets.
    //
    static ptrdiff_t _offsets[];
    static struct gc_type_info _info;

public:
    // These must be overridden to call the SmartCollect routines.  They
    // will be inherited by any classes derived from Object.  Derived classes
    // which add addition pointers may need to define their own offset
    // lists as well as override these two member functions.
    //
    void *operator new(size_t len);
    void operator delete(void *obj);

private:
    // Our private variables go here or anywhere else we'd like.
    //
    char *_name;
    int _value;
    Object *_left, *_right;

public:
    ~Object();
    Object(char *name, int val);

    Object *left() { return _left; }
    Object *right() { return _right; }
    void left(Object *r) { _left = r; }
    void right(Object *r) { _right = r; }
};

// Since our Object contains memory not allocated by SC (the "_name"), we
// must free these ourselves whenever Objects are scavenged by SC.  To do
// this, we give it a function which will clean us up.  We can simply call
// the destructor, which could nicely handle derived objects.  We cannot
// just "delete (Object*)o" here as it will both destroy and attempt to
// free the object, which is a Bad Thing since SC is trying to free it.
// We could just clean up the object directly, but since we'll have to
// duplicate the code in the destructor anyway, we may as well call it.
//
static void
destroy_obj(void *o)
{
    ((Object*)o)->~Object();
}

// This is a list of offsets to the pointers in Object which may point
// to GC objects.  We cannot place it directly in "_info" because of the
// C/C++ language itself.  Note how the offsetof macro allows doing this
// both easily and portably.  Derived classes may need to define their
// own such offset lists if they add any vars which will point into GC
// memory.
//
ptrdiff_t Object::_offsets[] =
	{ offsetof(Object, _left), offsetof(Object, _right) };

// Finally we can create the gc_type_info that SC needs.  Again, a derived
// class may need to define its own _info struct if it adds any GC pointers.
// It may not temporarily modify some fields in this struct, as the address
// of "_info" is used by SC as a unique handle for all objects of this type.
// However, if we use UNKNOWN_ATOMIC instead of NOT_ATOMIC below, we don't
// have to supply a list of offsets, and SC will look inside each entire
// Object looking for anything that looks like a pointer.  This is slower,
// but makes handling lots of derived objects much much simpler.
//
struct gc_type_info Object::_info =
{
    "Object",
    gc_type_info::NOT_ATOMIC,
    gc_type_info::ZERO_FILL,
    destroy_obj,
    sizeof _offsets / sizeof _offsets[0],
    _offsets,
    NULL	/* no userdef */
};


// Allocate and return memory for an Object.
//
void *
Object::operator new(size_t len)
{
    // Simply call "gc_alloc()" instead of "malloc()" or the default
    // "operator new" to allocate memory for a new object in GC memory.
    // Since we pass in the "len" separately, we could handle derived
    // objects if they do not add additional pointers into GC space.
    //
    void *obj = gc_alloc(&_info, len);

    // Always check the return value for sanity.  An exception could be
    // raised here, or some other method used to handle this more gracefully.
    //
    if (!obj)
    {
	fprintf(stderr, "Object::operator new(): cannot allocate Object\n");
	exit(10);
    }

    // print a message just so we can watch it work.
    printf("Object::operator new(%d) = %p\n", len, obj);
    return obj;
}


// Free the memory used by an object. 
//
void
Object::operator delete(void *obj)
{
    // print a message just so we can watch it work.
    printf("Object::operator delete(%p)\n", obj);

    // Free GC memory properly.  Calling "delete obj" or "free()" will
    // not work since the memory was allocated with "gc_alloc()" above.
    //
    gc_free(obj);
}


// Object constructor.  This and any other constructors may be defined
// as desired.  There is nothing GC or SC-specific here.
//
Object::Object(char *name, int val)
{
    // Allocate space for a copy of "name" using the standard C++
    // allocator, which will usually end up calling "malloc()".
    //
    _name = new char[strlen(name) + 1];
    strcpy(_name, name);
    _value = val;	// copy the value

    // Initialize our other class variables.  We do not really need to
    // do this as our gc_type_info has told SC to zero-fill our object
    // when it is allocated.  Still, it is good style.
    //
    _left = NULL;
    _right = NULL;

    // print a message just so we can watch it work.
    printf("Object::Object(%s) = %p\n", _name, this);
}


// Object destructor.  This may be made virtual if needed.  There is
// again nothing GC or SC-specific here.
//
Object::~Object()
{
    // print a message just so we can watch it work.
    printf("Object::~Object(%s) = %p\n", _name, this);

    // We need to free up our copy of the name the usual way.
    delete _name;
}


// The main routine will simply allocate lots of Objects and fail to
// free them.
//
int
main(int argc, const char *argv[])
{
    int loops = 100;	// number of loops to execute
    int objects = 10;	// number of objects to allocate per loop

    if (argc > 1)
	loops = atoi(argv[1]);

    if (argc > 2)
	objects = atoi(argv[2]);

    // This is how to tell SmartCollect about our static global variable.
    // The function allows us to tell SC about larger chunks of static
    // data which may contain pointers into GC memory.  SC will pick up
    // pointers on the stack so we do not have to teach it about those.
    //
    static Object *o;
    gc_add_root((char*)&o, (char*)&o + sizeof o);

    // Allocate a bunch of Objects and just thrown them away.
    //
    for (int i = 0; i < loops; i++)
    {
	Object *oo;	// point to some object from the stack
	o = NULL;

	for (int j = 0; j < objects; j++)
	{
	    oo = new Object("nuts", i);
	    oo->right(o);
	    o = oo;
	}
    }

    gc_collect();	// force SC to collect unused memory
    return 0;
}
