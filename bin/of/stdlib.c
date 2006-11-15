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

/* (c) Copyright 1996-2003 by CodeGen, Inc.  All Rights Reserved. */

/* very simple malloc/free/realloc for embedded systems
	- logging facility for standalone unix malloc to trace memory usage
	- also has a simple vbprintf/bprintf for formatted printing
	- bunch of misc standard routines like memset, strcpy, rand, etc
 */

#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "defs.h"

#ifdef LOG_MALLOC
#	ifndef STANDALONE
#		error LOG_MALLOC is only valid in STANDALONE builds
#	endif

#	include <stdio.h>
#	define malloc base_malloc
#	define memalign base_memalign
#	define free base_free
#	define realloc base_realloc

FILE *g_malloclog = NULL;
Bool g_logging = FALSE;
#endif /* LOG_MALLOC */

#ifdef DEBUG_MALLOC
typedef size_t guard_t;
#	define HEAD_GUARD	0xDEADBEEF
#	define TAIL_GUARD	0xFEEDFACE
#	define FREE_GUARD	0x69
#endif /* DEBUG_MALLOC */

struct block
{
	size_t len;
	struct block *next;
};
typedef struct block Block;

#define BLOCK_SIZE	(sizeof (Block))

/* uninitialized to zero so they will go into the BSS segment */
static Block *g_free;		/* maintained sorted by address */
static void *g_memory;		/* for statistics */
static size_t g_max;		/* for statistics */
static size_t g_allocblks;	/* for statistics */

Retcode
init_malloc(void *memory, size_t max)
{
#ifdef DEBUG_MALLOC
	memset(memory, FREE_GUARD, max);
#endif
	g_memory = memory;
	g_max = max;
	g_allocblks = 0;
	g_free = (Block*)memory;
	g_free->len = max - max % BLOCK_SIZE;
	g_free->next = NULL;

#ifdef LOG_MALLOC
	{
		char *e = getenv("MALLOC_LOG_FILE");

		if (e != NULL && *e != '\0')
			g_malloclog = fopen(e, "w");
	}
#endif
	return NO_ERROR;
}

/* return statistics for free and used memory */
void
meminfo(size_t *allocspc, size_t *freespc, size_t *allocblks,
	size_t *freeblks, void **endarena, size_t *arenasz)
{
	Block *p;

	*freespc = 0;
	*freeblks = 0;

	for (p = g_free; p; p = p->next)
	{
		*freespc += p->len;
		*freeblks += 1;
	}

	*allocspc = g_max - *freespc;
	*allocblks = g_allocblks;
	*endarena = (Byte*)g_memory + g_max;
	*arenasz = g_max;
}

#ifdef DEBUG_MALLOC
static void
check_free(char *str)
{
	Block *b;
	char *p;

	for (b = g_free; b; b = b->next)
	{
		if ((size_t)b < (size_t)g_memory || (size_t)b >= (size_t)g_memory + g_max)
		{
			dprintf("check %s: block pointer %p is out of range!\n", b);
			break;
		}

		/* preceding block should end with a tail guard */
		if (b != g_free && 
				*(guard_t*)((char*)b - sizeof (guard_t)) != TAIL_GUARD)
			dprintf("check %s: tail guard of prev %p len %d trashed!\n",
					str, b, b->len);
		
		/* next block should begin with a head guard */
		if (b->next && 
				*(guard_t*)((char*)b + b->len + sizeof (guard_t)) !=
						HEAD_GUARD)
			dprintf("check %s: head guard of next %p len %d trashed!\n",
					str, b, b->len);

		/* check only smaller blocks because this is expensive */
		if (b->len < 256)
			for (p = (char*)b + sizeof *b; p < (char*)b + b->len; p++)
				if (*p != FREE_GUARD)
				{
					dprintf("check %s: free area at %p trashed - %p (%d)\n",
							str, p, b, b->len);
					break;
				}
	}
}
#endif

void *
malloc(size_t sz)
{
	Block *p = NULL;
	Block *b;
	Block *n;

	DPRINTF(("malloc: %#x\n", sz));

	if (sz == 0)		/* not ANSI behavior, but fine for us */
		return NULL;

	g_allocblks++;
	sz += sizeof (size_t);		/* add room to store the block size */

#ifdef DEBUG_MALLOC
	sz += 2 * sizeof (guard_t);	/* add room for head and tail guards */
	check_free("malloc");
#endif

	/* round it up to next multiple of BLOCK_SIZE */
	if (sz % BLOCK_SIZE)
		sz += BLOCK_SIZE - sz % BLOCK_SIZE;
	
	for (b = g_free; b; b = b->next)
	{
		if (b->len >= sz)
		{
			if (b->len == sz || b->len - sz < BLOCK_SIZE)
			{
				n = b->next; /* exact fit or nothing useful left over? */
				sz = b->len;
			}
			else
			{
				n = (Block *)((char *)b + sz);
				n->len = b->len - sz;
				n->next = b->next;
				b->len = sz;
			}

			if (p == NULL)
				g_free = n;
			else
				p->next = n;
	
			break;
		}
			
		p = b;
	}

	if (b == NULL)
	{
		g_allocblks--;
		return NULL;
	}

#ifdef DEBUG_MALLOC
	*(guard_t*)((char*)b + sizeof (size_t)) = HEAD_GUARD;
	*(guard_t*)((char*)b + sz - sizeof (guard_t)) = TAIL_GUARD;
	check_free("malloc2");
	DPRINTF(("malloc: return %p\n",
			(char*)b + sizeof (size_t) + sizeof (guard_t)));
	return (char*)b + sizeof (size_t) + sizeof (guard_t);
#else
	DPRINTF(("malloc: return %p\n", (char*)&b->next));
	return &b->next;
#endif
}

void *
memalign(size_t align, size_t sz)
{
	Block *p = NULL;
	Block *b;
	Block *n;
	size_t offset;

	DPRINTF(("memalign: %#x, %#x\n", align, sz));

	if (sz == 0)		/* not ANSI behavior, but fine for us */
		return NULL;

	if (align < BLOCK_SIZE)
	{
		if (BLOCK_SIZE % align)
			return NULL;

		align = BLOCK_SIZE;
	}

	if (align % BLOCK_SIZE)		/* alignment must be a multiple of block size */
		return NULL;

	g_allocblks++;
	offset = sizeof (size_t);		/* add room to store the block size */
	sz += sizeof (size_t);		/* add room to store the block size */

#ifdef DEBUG_MALLOC
	offset += sizeof (guard_t);	/* add room for head guard */
	sz += 2 * sizeof (guard_t);	/* add room for head and tail guards */
	check_free("memalign");
#endif

	/* round it up to next multiple of BLOCK_SIZE */
	if (sz % BLOCK_SIZE)
		sz += BLOCK_SIZE - sz % BLOCK_SIZE;

	for (b = g_free; b; b = b->next)
	{
		size_t alignsz = align - (((size_t)b + offset) % align);

		if (b->len >= alignsz + sz)
		{
			DPRINTF(("memalign: alignsz %#x\n", alignsz));

			/* carve off free space before the allocated block */
			if (alignsz)
			{
				p = b;
				b = (Block *)((char *)p + alignsz);
				b->len = p->len - alignsz;
				b->next = p->next;
				p->next = b;
				p->len = alignsz;
			}

			/* exact fit or nothing useful left over? */
			if (b->len == sz || b->len - sz < BLOCK_SIZE)
			{
				if (p == NULL)
					g_free = b->next;
				else
					p->next = b->next;
		
				sz = b->len;
				break;
			}

			/* carve off free space after the block */
			n = (Block *)((char *)b + sz);
			n->next = b->next;
			n->len = b->len - sz;
			b->len = sz;

			if (p == NULL)
				g_free = n;
			else
				p->next = n;

			DPRINTF(("memalign: p %#x, b %#x, n %#x\n", p, b, n));
			DPRINTF(("memalign: len %d, alignsz %d, plen %d, nlen %d\n", b->len,
					alignsz, p ? p->len : 0, n->len));
			break;
		}
		
		p = b;
	}

	if (b == NULL)
		return NULL;

#ifdef DEBUG_MALLOC
	*(guard_t*)((char*)b + sizeof (size_t)) = HEAD_GUARD;
	*(guard_t*)((char*)b + sz - sizeof (guard_t)) = TAIL_GUARD;
	check_free("memalign2");
	DPRINTF(("memalign: return %p\n",
			(char*)b + sizeof (size_t) + sizeof (guard_t)));
	return (char*)b + sizeof (size_t) + sizeof (guard_t);
#else
	DPRINTF(("memalign: return %p\n", (char*)&b->next));
	return &b->next;
#endif
}

void *
calloc(size_t nmemb, size_t size)
{
	void *mem;
	
	size *= nmemb;
	mem = malloc(size);

	if (mem)
		memset(mem, 0, size);

	return mem;
}

void
free(void *ptr)
{
	Block *f = (Block*)((char*)ptr - sizeof (size_t));
	Block *p = NULL;
	Block *b;

	DPRINTF(("free: %p\n", ptr));
#ifdef DEBUG_MALLOC
	check_free("free");
	f = (Block*)((char*)f - sizeof (guard_t));
#endif

	/* lame sanity checks */
	if (ptr == NULL || f == NULL)
		return;

	g_allocblks--;

#ifdef DEBUG_MALLOC
	if (*(guard_t*)((char*)f + sizeof (size_t)) != HEAD_GUARD)
		dprintf("free: head guard of block %p len %d trashed!\n",
				ptr, f->len);
	
	if (*(guard_t*)((char*)f + f->len - sizeof (guard_t)) != TAIL_GUARD)
		dprintf("free: tail guard of block %p len %d trashed!\n",
				ptr, f->len);
#endif
		
	if (g_free == NULL)		/* completely empty free list */
	{
		g_free = f;
		return;
	}
	
	/* does this block go before the head of the free list? */
	if (f < g_free)
	{
		if ((char*)f + f->len == (char*)g_free)
		{
			f->len += g_free->len;
			f->next = g_free->next;
		}
		else
			f->next = g_free;
		
		g_free = f;
#ifdef DEBUG_MALLOC
		memset((char*)f + sizeof *f, FREE_GUARD, f->len - sizeof (*f));
#endif
		return;
	}
	
	for (b = g_free; b; b = b->next)
	{
		/* find the place this block should go in the freelist */
		if (f > b && (b->next == NULL || f < b->next))
		{
			/* first see if we can merge it with this block */
			if ((char*)b + b->len == (char*)f)
			{
				b->len += f->len;
				f = b;
			}
			else
			{
				/* nope - link it to the end of the block */
				f->next = b->next;
				b->next = f;
			}
			
			/* now see if we can merge it with the following block */
			if (f->next && (char*)f + f->len == (char*)f->next)
			{
				f->len += f->next->len;
				f->next = f->next->next;
			}

#ifdef DEBUG_MALLOC
		memset((char*)f + sizeof *f, FREE_GUARD, f->len - sizeof (*f));
#endif
			return;
		}
		
		p = b;
	}
}

void *
realloc(void *ptr, size_t size)
{
	char *p;
	Block *o;
	size_t sz = size;
	size_t osz;
	
	/* size of zero means to just free it */
	if (sz == 0)
	{
		free(ptr);
		return NULL;
	}
	
	/* just malloc if ptr is NULL */
	if (ptr == NULL)
		return malloc(sz);
	
	o = (Block*)((char*)ptr - sizeof (size_t));
	sz += sizeof (size_t);				/* add room for the length */

#ifdef DEBUG_MALLOC
	/* add room for head and tail guards */
	o = (Block*)((char*)o - sizeof (guard_t));
	sz += 2 * sizeof (guard_t);			/* add room for the guard bytes */
	osz = o->len - sizeof (size_t) - 2 * sizeof (guard_t);
	check_free("realloc");
#else
	osz = o->len - sizeof (size_t);		/* number of data bytes in the block */
#endif
	
	/* round the requested size up to next multiple of BLOCK_SIZE */
	if (sz % BLOCK_SIZE)
		sz += BLOCK_SIZE - sz % BLOCK_SIZE;

#ifdef DEBUG_MALLOC
	if (*(guard_t*)((char*)o + sizeof (size_t)) != HEAD_GUARD)
		dprintf("realloc - head guard of block %p len %d trashed!\n",
				ptr, osz);
	
	if (*(guard_t*)((char*)o + o->len - sizeof (guard_t)) != TAIL_GUARD)
		dprintf("realloc - tail guard of block %p len %d trashed!\n",
				ptr, osz);
#endif
	
	/* don't do anything if the requested size is smaller than the old size */
	if (sz <= o->len)
		return ptr;
	
	/* we have to allocate another block and copy the original into it */
	p = (char*)malloc(size);		/* use the originally requested size */
	
	if (p == NULL)
		return NULL;
	
	/* copy the original, destroy the original pointer, return the new block */
	memcpy(p, ptr, osz);			/* copy all the original data bytes */
	free(ptr);
	return p;
}

#ifdef DEBUG_MALLOC
void
dump_heap()
{
	Block *b;
	int i;

	for (b = g_free; b; b = b->next)
	{
		Block *a = (Block *)((char *)b + b->len);
		size_t end = (size_t)b->next;

		if (!end)
			end = (size_t)g_memory + g_max;

		dprintf("%p: size %#x (%d) free, next %p\n", b, b->len, b->len, b->next);

		for (i = 0; i < 10 && (size_t)a < end; i++)
		{
			dprintf("%p: size %#x (%d) allocated\n", a, a->len, a->len);
			a = (Block *)((char *)a + a->len);
		}

		if ((size_t)a < end)
		{
			dprintf("%p: ", a);

			for (i = 0; (size_t)a < end; i++)
				a = (Block *)((char *)a + a->len);

			dprintf("%d more allocated blocks\n", i);
		}
	}
}
#endif

#ifdef LOG_MALLOC
#  undef malloc
#  undef memalign
#  undef free
#  undef realloc

int
stacksnapshot(int size, unsigned long *stack, int skip)
{
	int count = 0;
	int i;

#ifdef __i386__
	unsigned long *bp = &((unsigned long *)&size)[-2];
	extern char _start[];

	/* while (count < size && bp[1] >= 0) */
	while (count < size && bp != NULL &&
			(unsigned long)bp[1] >= (unsigned long)&_start)
	{
		if (!skip)
			stack[count++] = (unsigned long)bp[1];
		else
			skip--;

		if (bp[0] == 0)
			break;

		if ((long)bp[0] <= (long)bp)
		{
			fprintf(stderr, "bad frame pointer 0x%lX\r\n", bp[0]);
			break;
		}

		bp = (unsigned long*)(bp[0]);
	}
#else
	#warning Need architecture-specific stack backtrace code here
#endif

	for (i = count; i < size; i++)
		stack[i] = 0UL;

	return count;
}

static void
do_stk(void)
{
	int i, n;
	#define STK 10
	unsigned long stk[STK];

	n = stacksnapshot(STK, stk, 2);
	
	if (n)
	{
		fprintf(g_malloclog, " %#lx", stk[0]);

		for (i = 1; i < n; i++)
			fprintf(g_malloclog, ":%#lx", stk[i]);
	}
	
	fprintf(g_malloclog, "\n");
}

void *
malloc(size_t sz)
{
	static Bool inmalloc = FALSE;
	void *p = base_malloc(sz);

	if (!inmalloc)
	{
		inmalloc = TRUE;

		if (g_malloclog != NULL)
		{
			fprintf(g_malloclog, "m %u %#x", sz, (uInt)p);
			do_stk();
		}

		inmalloc = FALSE;
	}

	return p;
}

void *
memalign(size_t align, size_t sz)
{
	void *p = base_memalign(align, sz);

	if (g_malloclog != NULL)
	{
		fprintf(g_malloclog, "a %u %u %#x", align, sz, (uInt)p);
		do_stk();
	}

	return p;
}

void
free(void *ptr)
{
    if (g_malloclog != NULL && ptr)
	{
		fprintf(g_malloclog, "f %#x", (uInt)ptr);
		do_stk();
	}

    base_free(ptr);
}

void *
realloc(void *p, size_t nsz)
{
    void *np = base_realloc(p, nsz);

    if (g_malloclog != NULL)
	{
		fprintf(g_malloclog, "r %#x %u %#x", (uInt)p, nsz, (uInt)np);
		do_stk();
	}

    return np;
}
#endif /* LOG_MALLOC */


/* vbprintf is like vsprintf but only handles strings and ints,
   with just a few formats and field options plus some custom extensions:
   #	alternate formatting - for hex, prepend 0x to output otherwise ignore
   -	right-pad output with blanks - only for strings right now
   0	left-pad numbers with zero

   0-9+	numeric field width - left-pad with blanks unless leading 0 specified
   *	numeric field width taken from next argument

   lqL	display following integer specification as a Long instead of an Int
   C	display following integer specification as a Cell - may be Int or Long

   d	display integer argument in decimal
   u	display unsigned integer argument in decimal
   x	display unsigned integer argument in hexadecimal - case HEX_A
   X	display unsigned integer argument in upper-case hexadecimal
   p	display pointer argument as hex with leading 0x - like %#x or %#lx
   c	display integer as a single character
   s	display C string (null-byte terminated)
   P	display Pascal string (1st byte is length)
   S	display Forth string (2 args - string & length)
 */
extern int vbprintf(char *buf, const char *fmt, va_list args);

int
vbprintf(char *buf, const char *fmt, va_list args)
{
	char *sarg, *s;
	uLong iarg;
	int field, hex;
	Int len;
	Bool rpad, lzero, sign, larg, alt;
	char nbuf[STR_SIZE];
	
	if (fmt == NULL || buf == NULL)
		return 0;
		
	while (*fmt)
	{
		if (*fmt != '%')
		{
			*buf++ = *fmt++;
			continue;
		}

		if (*++fmt == '%')
		{
			*buf++ = *fmt++;
			continue;
		}
		
		field = 0;
		rpad = FALSE;
		lzero = FALSE;
		sign = FALSE;
		hex = 0;
		larg = FALSE;
		alt = FALSE;
	
		if (*fmt == '#')
		{
			fmt++;
			alt = TRUE;
		}
	
		if (*fmt == '-')
		{
			fmt++;
			rpad = TRUE;
		}

		if (*fmt == '0')
		{
			fmt++;
			lzero = TRUE;
		}

		if (*fmt == '*')
		{
			field = va_arg(args, Int);
			fmt++;
		}
		else
			for (; isdigit(*fmt); fmt++)
				field = field * 10 + *fmt - '0';
		
		/* following arg is long instead of int */
		if (*fmt == 'l' || *fmt == 'L' || *fmt == 'q' || *fmt == 'C')
		{
			if (*fmt != 'C' || sizeof (Cell) > sizeof (Int))
				larg = TRUE;

			fmt++;

			/* skip "%ll*" style formats */
			if (*fmt == 'l')
				fmt++;
		}

		/* used to be switch (*fmt++) but some gcc revs generate bad code */
		iarg = *fmt++;

		if (iarg == 'c')			/* character */
		{
			iarg = va_arg(args, Int);
			*buf++ = iarg;
		}
		else if (iarg == 'x' || iarg == 'X' || iarg == 'd' || iarg == 'u' ||
				iarg == 'p')
		{
			if (iarg == 'x')
				hex = HEX_A;
			else if (iarg == 'X')
				hex = 'A';
			else if (iarg == 'p')
			{
				hex = HEX_A;
				alt = TRUE;
			}

			if (larg || (iarg == 'p' && sizeof (Ptr) > sizeof (Int)))
				iarg = va_arg(args, uLong);
			else if (hex || iarg == 'u')
			{
				iarg = va_arg(args, uInt);

				/* work around for gcc ARM problem */
				iarg &= 0xFFFFFFFF;
			}
			else
				iarg = va_arg(args, Int);

			s = nbuf + STR_SIZE - 1;
			*s = '\0';
			
			if (!hex && (Long)iarg < 0)
			{
				sign = TRUE;
				iarg = -(Long)iarg;
				field--;
			}
			else if (iarg == 0)
			{
				*--s = '0';
				field--;
			}
			
			for (; iarg; field--)
			{
				if (hex)
				{
					int d = iarg & 0xF;
					*--s = (d < 10) ? d + '0' : d - 10 + hex;
					iarg >>= 4;
				}
				else
				{
					*--s = iarg % 10 + '0';
					iarg /= 10;
				}
			}
			
			while (field-- > 0)
				*--s = (lzero) ? '0' : ' ';
			
			if (hex && alt)
			{
				*--s = 'x';
				*--s = '0';
			}

			if (sign)
				*--s = '-';
			
			while (*s)
				*buf++ = *s++;
		}
		else if (iarg == 's' || iarg == 'S' || iarg == 'P')		/* string */
		{
			sarg = va_arg(args, char*);

			if (iarg == 'S')			/* Forth-style string */
				len = va_arg(args, Int);
			else if (sarg == NULL)		/* no string */
				len = 0;
			else if (iarg == 'P')		/* Pascal-style string */
				len = *(uByte*)sarg++;
			else						/* C string */
				len = strlen(sarg);
			
			if (!rpad)
			{
				while (field - len > 0)
				{
					*buf++ = ' ';
					field--;
				}
			}

			for (; len-- > 0; field--)
				*buf++ = *sarg++;
			
			if (rpad)
				while (field-- > 0)
					*buf++ = ' ';
		}
	}
	
	*buf = '\0';
	return strlen(buf);
}
 
/* this is like sprintf but renamed to avoid collision with standard libs */
int
bprintf(char *buf, const char *fmt, ...)
{
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = vbprintf(buf, fmt, args);
	va_end(args);
	return ret;
}
 
/* this is like bprintf only it uses and returns a static buffer */
char *
rbprintf(const char *fmt, ...)
{
	va_list args;
	static char buf[STR_SIZE];

	*buf = '\0';
	va_start(args, fmt);
	(void)vbprintf(buf, fmt, args);
	va_end(args);
	return buf;
}


/* hacks to speed up memset() and memmove() a little
   - we assume that the integer type is at least 32 bits */
#ifdef BROKEN_64BIT_INTS
	#define LSB_BITS	(sizeof (uInt) - 1)
	#define LONG_SIZE	(sizeof (uInt))
	#define LONG_TYPE	uInt
#else
	#define LSB_BITS	(sizeof (uLong) - 1)
	#define LONG_SIZE	(sizeof (uLong))
	#define LONG_TYPE	uLong
#endif

void *
memset(void *ptr, int val, size_t len)
{
    unsigned char *p = (unsigned char *)ptr;
    unsigned char b = val;

#if REALLY_SIMPLE

    while (len-- > 0)
	*p++ = b;

#else

    while (((Ptr)p & LSB_BITS) && len > 0)
    {
	*p++ = b;
	len--;
    }

    if (len)
    {
	LONG_TYPE *pl = (LONG_TYPE *)p;
	size_t l = len / LONG_SIZE;
	LONG_TYPE lval;

	val &= BYTE_MASK;
	lval = (val << BYTE_SIZE) | val;
	lval = (lval << (BYTE_SIZE * 2)) | lval;

#if defined __LONGLONG && !defined BROKEN_64BIT_INTS
	lval = (lval << (BYTE_SIZE * 4)) | lval;
#endif

	len -= l * LONG_SIZE;

	while (l--)
	    *pl++ = lval;

	if (len)
	{
	    p = (unsigned char *)pl;

	    while (len--)
		*p++ = b;
	}
    }
#endif				/* REALLY_SIMPLE */

    return ptr;
}

void *
memmove(void *dst, const void *src, size_t len)
{
    char *d = (char *)dst;
    char *s = (char *)src;

#if REALLY_SIMPLE

    if (d < s)			/* assume src overlaps end of dst */
    {
	while (len-- > 0)
	    *d++ = *s++;
    }
    else
    {
	s += len;
	d += len;

	while (len-- > 0)
	    *--d = *--s;
    }

#else
    if (d < s)			/* assume src overlaps end of dst */
    {
	if (len > LONG_SIZE * 3 && ((Ptr)d & LSB_BITS) == (((Ptr)s) & LSB_BITS))
	{
	    while (((Ptr)s & LSB_BITS) && len > 0)
	    {
		*d++ = *s++;
		len--;
	    }

	    if (len)
	    {
		LONG_TYPE *sl = (LONG_TYPE *)s;
		LONG_TYPE *dl = (LONG_TYPE *)d;
		size_t l = len / LONG_SIZE;

		len -= l * LONG_SIZE;

		while (l-- > 0)
		    *dl++ = *sl++;

		if (len)
		{
		    s = (char *)sl;
		    d = (char *)dl;

		    while (len-- > 0)
			*d++ = *s++;
		}
	    }
	}
	else
	{
	    while (len-- > 0)
		*d++ = *s++;
	}
    }
    else			/* assume dst overlaps end of src */
    {
	s += len;
	d += len;

	if (len > LONG_SIZE * 3 && ((Ptr)d & LSB_BITS) == (((Ptr)s) & LSB_BITS))
	{
	    while (((Ptr)s & LSB_BITS) && len > 0)
	    {
		*--d = *--s;
		len--;
	    }

	    if (len)
	    {
		LONG_TYPE *sl = (LONG_TYPE *)s;
		LONG_TYPE *dl = (LONG_TYPE *)d;
		size_t l = len / LONG_SIZE;

		len -= l * LONG_SIZE;

		while (l-- > 0)
		    *--dl = *--sl;

		if (len)
		{
		    s = (char *)sl;
		    d = (char *)dl;

		    while (len-- > 0)
			*--d = *--s;
		}
	    }
	}
	else
	{
	    while (len-- > 0)
		*--d = *--s;
	}
    }

#endif				/* REALLY_SIMPLE */

    return dst;
}

void *
memcpy(void *dst, const void *src, size_t len)
{
	return memmove(dst, src, len);
}

#ifdef bcopy
#undef bcopy
#endif

void
bcopy(const void *src, void *dst, size_t len)
{
    memmove(dst, src, len);
}

#ifdef bzero
#undef bzero
#endif

void
bzero(void *ptr, size_t len)
{
    memset(ptr, '\0', len);
}

int
memcmp(const void *m1, const void *m2, size_t len)
{
	const unsigned char *s1 = (const unsigned char*)m1;
	const unsigned char *s2 = (const unsigned char*)m2;

	for (; len > 0; s1++, s2++, len--)
		if (*s1 != *s2)
			break;

	return (len == 0) ? 0 : *s2 - *s1;
}

size_t
strlen(const char *str)
{
	size_t len = 0;

	if (str)
		while (*str++)
			len++;

	return len;
}

char *
strcat(char *str, const char *append)
{
	char *ostr = str;

	if (str == NULL || append == NULL)
		return str;

	str += strlen(str);

	while (*append)
		*str++ = *append++;

	*str = '\0';
	return ostr;
}

char *
strncat(char *str, const char *append, size_t count)
{
	char *ostr = str;

	if (str == NULL || append == NULL || count == 0)
		return str;

	str += strlen(str);

	while (*append && count-- > 0)
		*str++ = *append++;

	*str = '\0';
	return ostr;
}

char *
strcpy(char *dst, const char *src)
{
	char *odst = dst;

	if (dst == NULL || src == NULL)
		return dst;

	while (*src)
		*dst++ = *src++;

	*dst = '\0';
	return odst;
}

char *
strncpy(char *dst, const char *src, size_t len)
{
	char *odst = dst;

	if (dst == NULL || src == NULL)
		return dst;

	while (*src && len-- > 0)
		*dst++ = *src++;

	if (len > 0)
		*dst = '\0';

	return odst;
}

char *
strchr(const char *str, int chr)
{
	if (str == NULL)
		return NULL;

	while (*str && *str != chr)
		str++;

	return (*str == chr) ? (char*)str : NULL;
}

char *
strrchr(const char *str, int chr)
{
	const char *ostr = str;

	if (str == NULL)
		return NULL;

	str += strlen(str) - 1;

	while (str >= ostr && *str != chr)
		str--;

	return (str >= ostr) ? (char*)str : NULL;
}

int
strcmp(const char *s1, const char *s2)
{
	if (s1 == NULL)
		s1 = "";

	if (s2 == NULL)
		s2 = "";

	for (; *s1 && *s2; s1++, s2++)
		if (*s1 != *s2)
			break;

	return (!*s1 && !*s2) ? 0 : *s2 - *s1;
}

int
strncmp(const char *s1, const char *s2, size_t len)
{
	if (s1 == NULL)
		s1 = "";

	if (s2 == NULL)
		s2 = "";

	for (; *s1 && *s2 && len--; s1++, s2++)
		if (*s1 != *s2)
			break;

	return (!len && !*s1 && !*s2) ? 0 : *s2 - *s1;
}

/* created using Knuth's Seminumerical Algorithms and tweaked a bit */
static unsigned long seed = 1262951413ul;

void
srand(unsigned s)
{
	seed = s;
}

unsigned long
urand(void)
{
    seed = seed * 3141592621ul + 1;
    seed ^= seed >> 13;
    return seed;
}

int
rand(void)
{
    return (int)(urand() & 0x7FFFFFFFul);
}

