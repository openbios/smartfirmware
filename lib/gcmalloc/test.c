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

/* An incomplete test for the garbage collector.  		*/
/* Some more obscure entry points are not tested at all.	*/
# include <stdlib.h>
# include <stdio.h>
/*# include "gc.h"*/

#define GC_MALLOC malloc
#define GC_MALLOC_ATOMIC malloc
#define GC_REALLOC realloc
#define GC_FREE free
#define DCL_LOCK_STATE 0
#define LOCK() { }
#define UNLOCK() { }

struct SEXPR {
    struct SEXPR * sexpr_car;
    struct SEXPR * sexpr_cdr;
};

# ifdef __STDC__
    typedef void * void_star;
# else
    typedef char * void_star;
# endif

typedef struct SEXPR * sexpr;

extern sexpr cons();

# define nil ((sexpr) 0)
# define car(x) ((x) -> sexpr_car)
# define cdr(x) ((x) -> sexpr_cdr)
# define is_nil(x) ((x) == nil)


int extra_count = 0;        /* Amount of space wasted in cons node */

/* Silly implementation of Lisp cons. Intentionally wastes lots of space */
/* to test collector.                                                    */
sexpr cons (x, y)
sexpr x;
sexpr y;
{
    register sexpr r;
    register int *p;
    register my_extra = extra_count;
    
    r = (sexpr) GC_MALLOC(8 + my_extra);
    if (r == 0) {
        (void)printf("Out of memory\n");
        exit(1);
    }
    for (p = (int *)r; ((char *)p) < ((char *)r) + my_extra + 8; p++) {
	if (*p) {
	    (void)printf("Found nonzero at %p - allocator is broken\n", p);
	    exit(1);
        }
        *p = 13;
    }
    r -> sexpr_car = x;
    r -> sexpr_cdr = y;
    extra_count = (my_extra + 1) % 5000;
    return(r);
}

/* Return reverse(x) concatenated with y */
sexpr reverse1(x, y)
sexpr x, y;
{
    if (is_nil(x)) {
        return (y);
    } else {
        return ( reverse1(cdr(x), cons(car(x), y)) );
    }
}

sexpr reverse(x)
sexpr x;
{
    sexpr ret;
    ret = ( reverse1(x, nil) );
/*
printf("reverse(x=%p) ret=%p ret->car=%d ret->cdr=%p\n",
x, ret, car(ret), cdr(ret));
*/
    return ret;
}

sexpr ints(low, up)
int low, up;
{
    sexpr ret;
    if (low > up) {
	ret = (nil);
    } else {
        ret = (cons((sexpr)low, ints(low+1, up)));
    }
/*
printf("ints(low=%d, up=%d) ret=%p ret->car=%d ret->cdr=%p\n",
low, up, ret, car(ret), cdr(ret));
*/
    return ret;
}

void check_ints(list, low, up)
sexpr list;
int low, up;
{
/*
printf("check_ints(list=%p, low=%d, up=%d) list->car=%d list->cdr=%p\n",
list, low, up, car(list), cdr(list));
*/
    if ((int)(car(list)) != low) {
        (void)printf(
           "List reversal produced incorrect list - collector is broken\n");
        exit(1);
    }
    if (low == up) {
        if (cdr(list) != nil) {
           (void)printf("List too long - collector is broken\n");
           exit(1);
        }
    } else {
        check_ints(cdr(list), low+1, up);
    }
}

/* Not used, but useful for debugging: */
void print_int_list(x)
sexpr x;
{
    if (is_nil(x)) {
        (void)printf("NIL\n");
    } else {
        (void)printf("%p", car(x));
        if (!is_nil(cdr(x))) {
            (void)printf(", ");
            (void)print_int_list(cdr(x));
        } else {
            (void)printf("\n");
        }
    }
}

/* Try to force a to be strangely aligned */
struct {
  sexpr aa;
  char dummy;
} A;
#define a A.aa

/*
 * Repeatedly reverse lists built out of very different sized cons cells.
 * Check that we didn't lose anything.
 */
void
reverse_test()
{
    int i;
    sexpr b;

    a = ints(1, 100);
    b = ints(1, 50);
    for (i = 0; i < 50; i++) {
        b = reverse(reverse(b));
    }
    for (i = 0; i < 10; i++) {
        a = reverse(reverse(a));
        if (i & 1) {
            a = (sexpr)GC_REALLOC((void_star)a, 500);
        } else {
            a = (sexpr)GC_REALLOC((void_star)a, 4200);
        }
    }
    check_ints(a,1,100);
    check_ints(b,1,50);
    a = b = 0;
}

/*
 * The rest of this builds balanced binary trees, checks that they don't
 * disappear, and tests finalization.
 */
typedef struct treenode {
    int level;
    struct treenode * lchild;
    struct treenode * rchild;
} tn;

int finalizable_count = 0;
int finalized_count = 0;
int dropped_something = 0;

# ifdef __STDC__
  void finalizer(void * obj, void * client_data)
# else
  void finalizer(obj, client_data)
  char * obj;
  char * client_data;
# endif
{
  tn * t = (tn *)obj;
  if ((int)client_data != t -> level) {
     (void)printf("Wrong finalization data - collector is broken\n");
     exit(1);
  }
  finalized_count++;
}

size_t counter = 0;

tn * mktree(n)
int n;
{
    tn * result = (tn *)GC_MALLOC(sizeof(tn));
    
    if (n == 0) return(0);
    if (result == 0) {
        (void)printf("Out of memory\n");
        exit(1);
    }
    result -> level = n;
    result -> lchild = mktree(n-1);
    result -> rchild = mktree(n-1);
    if (counter++ % 119 == 0) {
        /*GC_REGISTER_FINALIZER((void_star)result, finalizer, (void_star)n,
        		      (GC_finalization_proc *)0, (void_star *)0);*/
        finalizable_count++;
    }
    return(result);
}

void chktree(t,n)
tn *t;
int n;
{
    if (n == 0 && t != 0) {
        (void)printf("Clobbered a leaf - collector is broken\n");
        exit(1);
    }
    if (n == 0) return;
    if (t -> level != n) {
        (void)printf("Lost a node at level %d - collector is broken\n", n);
        exit(1);
    }
    if (counter++ % 373 == 0) (void) GC_MALLOC(counter%5001);
    chktree(t -> lchild, n-1);
    if (counter++ % 73 == 0) (void) GC_MALLOC(counter%373);
    chktree(t -> rchild, n-1);
}

void alloc_small(n)
int n;
{
    register int i;
    
    for (i = 0; i < n; i += 8) {
        if (GC_MALLOC_ATOMIC(8) == 0) {
            (void)printf("Out of memory\n");
            exit(1);
        }
    }
}

void
tree_test()
{
    tn * root = mktree(16);
    register int i;
    
    alloc_small(5000000);
    chktree(root, 16);
    if (finalized_count && ! dropped_something) {
        (void)printf("Premature finalization - collector is broken\n");
        exit(1);
    }
    dropped_something = 1;
    root = mktree(16);
    chktree(root, 16);
    for (i = 16; i >= 0; i--) {
        root = mktree(i);
        chktree(root, i);
    }
    alloc_small(5000000);
}

/*# include "gc_private.h"*/

int n_tests = 0;

void
run_one_test()
{
    (void)DCL_LOCK_STATE;
    
    reverse_test();
    tree_test();
    LOCK();
    n_tests++;
    UNLOCK();
    
}

void
check_heap_stats()
{
    extern void *sbrk(int);

    (void)printf("Completed %d tests\n", n_tests);
    (void)printf("Finalized %d/%d objects - ",
    		 finalized_count, finalizable_count);
    (void)printf("Final brk value is %p bytes\n", sbrk(0));
/*****
    if (finalized_count > finalizable_count
        || finalized_count < finalizable_count/2) {
        (void)printf ("finalization is probably broken\n");
        exit(1);
    } else {
        (void)printf ("finalization is probably ok\n");
    }
    (void)printf("Total number of bytes allocated is %d\n",
    	         WORDS_TO_BYTES(GC_words_allocd + GC_words_allocd_before_gc));
    (void)printf("Final heap size is %d bytes\n", GC_heapsize);
    if (WORDS_TO_BYTES(GC_words_allocd + GC_words_allocd_before_gc)
        < 33500000*n_tests) {
        (void)printf("Incorrect execution - missed some allocations\n");
        exit(1);
    }
    if (GC_heapsize > 10000000*n_tests) {
        (void)printf("Unexpected heap growth - collector may be broken\n");
        exit(1);
    }
*****/
    (void)printf("Collector appears to work\n");
}

#ifndef PCR
int
main()
{
    n_tests = 0;
    run_one_test();
    check_heap_stats();
    (void)fflush(stdout);
    return 0;
}
# else
test()
{
    PCR_Th_T * th1;
    PCR_Th_T * th2;
    int code;

    n_tests = 0;
    th1 = PCR_Th_Fork(run_one_test, 0);
    th2 = PCR_Th_Fork(run_one_test, 0);
    run_one_test();
    if (PCR_Th_T_Join(th1, &code, NIL, PCR_allSigsBlocked, PCR_waitForever)
        != PCR_ERes_okay || code != 0) {
        (void)printf("Thread 1 failed\n");
    }
    if (PCR_Th_T_Join(th2, &code, NIL, PCR_allSigsBlocked, PCR_waitForever)
        != PCR_ERes_okay || code != 0) {
        (void)printf("Thread 2 failed\n");
    }
    check_heap_stats();
    (void)fflush(stdout);
    return(0);
}
#endif
