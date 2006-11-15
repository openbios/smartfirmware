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

#if defined __MWERKS__ && defined macintosh
#define macintosh
#include <console.h>
#endif

#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

extern void *memalign(size_t align, size_t sz);
#if defined macintosh || defined BeOS
extern void *sbrk(size_t sz);
#endif

#define LT_UNUSED 0
#define LT_MALLOC 1
#define LT_FREE 2
#define LT_REALLOC 3
#define LT_MEMALIGN 4

struct logentry
{
    long type;
    long id;
    long sz;
    long align;
};

#ifndef MAXENTRIES
#define MAXENTRIES 800000
#define INFOENTRIES 100000
#endif

struct logentry runlog[MAXENTRIES];
int runlogsz = 0;
void *ptrs[MAXENTRIES];

int failures = 0;
long failedbytes = 0;

#ifdef macintosh
#define BUFSIZE 8192
#else
#define BUFSIZE 65536
#endif

char *prog = NULL;

static int
readlog(char const *name)
{
    static char buf[BUFSIZE];
    char *bp;
    char *bufend;
    int fd = open(name, O_RDONLY);

    if (fd < 0)
	return 0;

    bp = buf;
    bufend = buf;

    while (1)
    {
	static char line[BUFSIZE];
	char *lp = line;
	char *arg1;
	char *arg2;
	char *arg3;
	int id;
	int sz;

	while (1)
	{
	    if (bp >= bufend)
	    {
		/* attempt to reload the buffer */
		int bytes = read(fd, buf, BUFSIZE);

		if (bytes < 0)
		{
		    close(fd);
		    return 0;
		}

		if (bytes == 0)
		{
		    close(fd);
		    return 1;
		}

		bp = buf;
		bufend = &buf[bytes];
	    }

	    if (*bp == '\n' || *bp == '\r' || *bp == '\0')
		break;

	    *lp++ = *bp++;
	}

	*lp = '\0';
	bp++;

	if (line[0] == '\0' || line[0] == '#')
	    continue;

	arg1 = line + 1;

	while (*arg1 != '\0' && *arg1 != ' ' && *arg1 != '\t')
	    arg1++;

	while (*arg1 == ' ' || *arg1 == '\t')
	    arg1++;

	arg2 = arg1;

	while (*arg2 != '\0' && *arg2 != ' ' && *arg2 != '\t')
	    arg2++;

	while (*arg2 == ' ' || *arg2 == '\t')
	    arg2++;

	arg3 = arg2;

	while (*arg3 != '\0' && *arg3 != ' ' && *arg3 != '\t')
	    arg3++;

	while (*arg3 == ' ' || *arg3 == '\t')
	    arg3++;

	id = atoi(arg1);
	sz = atoi(arg2);

	/* process line */
	switch (line[0])
	{
	case 'm':
	    runlog[runlogsz].type = LT_MALLOC;
	    runlog[runlogsz].id = id;
	    runlog[runlogsz].sz = sz;
#ifdef DEBUG
	    printf("malloc %d %d (\"%s\" \"%s\")\n", id, sz, arg1, arg2);
#endif
	    runlogsz++;
	    break;
	case 'f':
	    runlog[runlogsz].type = LT_FREE;
	    runlog[runlogsz].id = id;
#ifdef DEBUG
	    printf("free %d (\"%s\")\n", id, arg1);
#endif
	    runlogsz++;
	    break;
	case 'r':
	    runlog[runlogsz].type = LT_REALLOC;
	    runlog[runlogsz].id = id;
	    runlog[runlogsz].sz = sz;
#ifdef DEBUG
	    printf("realloc %d %d (\"%s\" \"%s\")\n", id, sz, arg1, arg2);
#endif
	    runlogsz++;
	    break;
	case 'a':
	    runlog[runlogsz].type = LT_MEMALIGN;
	    runlog[runlogsz].id = id;
	    runlog[runlogsz].sz = atoi(arg3);
	    runlog[runlogsz].align = sz;
#ifdef DEBUG
	    printf("memalign %d %d %d (\"%s\" \"%s\" \"%s\")\n", id, align, align, arg1, arg3, arg2);
#endif
	    runlogsz++;
	    break;
	default:
	    fprintf(stderr, "%s: bogus lines %d, \"%s\"\n", prog, runlogsz,
		    line);
	    close(fd);
	    return 0;
	}

	if ((runlogsz % INFOENTRIES) == 0)
	    fprintf(stderr, "%s: log size is %d entries\n", name, runlogsz);

	if (runlogsz >= MAXENTRIES)
	{
	    fprintf(stderr, "%s: %s: log is too large, max is %d lines\n", prog,
		    name, MAXENTRIES);
	    close(fd);
	    return 1;    /* continue running with what we have */
	}
    }
}

static void
execlog()
{
    struct logentry *end = &runlog[runlogsz];
    struct logentry *e = runlog;
    void *p;

    for (; e < end; e++)
    {
	int id = e->id;
	int t = e->type;

	if (t == LT_MALLOC)
	{
#ifdef __cplusplus
	    p = new char[e->sz];
#else
	    p = malloc(e->sz);
#endif
	    ptrs[id] = p;

	    if (p != NULL)
		*((int *)p) = 0;
	    else
	    {
		failures++;
		failedbytes += e->sz;
	    }

#ifdef DEBUG
	    fprintf(stderr, "m %d %d 0x%X\n", id, e->sz, ptrs[id]);
#endif
	}
	else if (t == LT_FREE)
	{
#ifdef DEBUG
	    fprintf(stderr, "f %d 0x%X\n", id, ptrs[id]);
#endif
#ifdef __cplusplus
	    delete ptrs[id];
#else
	    free(ptrs[id]);
#endif
	}
	else if (t == LT_REALLOC)
	{
#ifdef DEBUG
	    fprintf(stderr, "r %d %d 0x%X ", id, e->sz, ptrs[id]);
#endif
#ifdef __cplusplus
	    p = new char[e->sz];
	    delete ptrs[id];
#else
	    p = realloc(ptrs[id], e->sz);
#endif
	    ptrs[id] = p;

	    if (p != NULL)
		*((int *)p) = 0;
	    else
	    {
		failures++;
		failedbytes += e->sz;
	    }

#ifdef DEBUG
	    fprintf(stderr, "0x%X\n", p);
#endif
	}
	else if (t == LT_MEMALIGN)
	{
	    p = memalign(e->align, e->sz);
	    ptrs[id] = p;

	    if (p != NULL)
		*((int *)p) = 0;
	    else
	    {
		failures++;
		failedbytes += e->sz;
	    }

#ifdef DEBUG
	    fprintf(stderr, "a %d %d %d, 0x%X\n", id, e->align, e->sz,
		    ptrs[id]);
#endif
	}
	else
	{
	    fprintf(stderr, "corrupt log entry %d, run aborted\n", e - runlog);
	    fprintf(stderr, "\ttype = %ld\n", e->type);
	    fprintf(stderr, "\tid = %ld\n", e->id);
	    fprintf(stderr, "\tsz = %ld\n", e->sz);
	    fprintf(stderr, "\talign = %ld\n", e->align);
	    exit(1);
	}
    }
}

static void
usage()
{
    fprintf(stderr, "Usage: %s logfile\n", prog);
    exit(2);
}

int
main(int argc, char **argv)
{
    void *ps;
    struct timeval ts;
    struct timeval te;
    struct timeval rt;
    void *pe;
    long rm;
    int count;

#if defined __MWERKS__ && defined macintosh
    argc = ccommand(&argv);
    printf("Starting...\n");
#endif

    prog = argv[0];

    if (argc < 2)
	usage();

    if (!readlog(argv[1]))
    {
	fprintf(stderr, "%s: cannot read log file, %s\n", prog, argv[1]);
	exit(1);
    }

    if (argc > 2)
	count = atoi(argv[2]);
    else
	count = 1;

    ps = sbrk(0);

    while (count-- > 0)
    {
	gettimeofday(&ts, 0);
	execlog();
	gettimeofday(&te, 0);

	if (te.tv_usec < ts.tv_usec)
	{
	    rt.tv_sec = te.tv_sec - ts.tv_sec - 1;
	    rt.tv_usec = te.tv_usec - ts.tv_usec + 1000000;
	}
	else
	{
	    rt.tv_sec = te.tv_sec - ts.tv_sec;
	    rt.tv_usec = te.tv_usec - ts.tv_usec;
	}

	pe = sbrk(0);
	rm = (char *)pe - (char *)ps;

	printf("%ld.%04ld seconds, %ldK memory\n", rt.tv_sec, rt.tv_usec / 100,
		rm / 1024);
    }

    if (failures)
	printf("%d allocations totaling %ld bytes were unsuccessful\n",
		failures, failedbytes);

    return 0;
}
