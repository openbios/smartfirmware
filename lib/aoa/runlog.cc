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
#include <stdio.h>
#include <fcntl.h>

#ifdef STOPWATCH
#include "stopwch.h"
#endif

#ifdef GETTIMEOFDAY
#include <sys/time.h>
#include <unistd.h>
#ifndef bsdi
extern "C" void gettimeofday(struct timeval *, struct tzval *);
#endif
#endif

#define LT_UNUSED 0
#define LT_MALLOC 1
#define LT_FREE 2
#define LT_REALLOC 3

struct logentry
{
    int type : 3;
    int id : 30;
    long sz;
};

#define MAXENTRIES 100000

struct logentry runlog[MAXENTRIES];
int runlogsz = 0;
void *ptrs[MAXENTRIES];

#define BUFSIZE 65536

char *prog = NULL;

int
readlog(char const *name)
{
    int fd = open(name, O_RDONLY);

    if (fd < 0)
    	return 0;

    char buf[BUFSIZE];
    char *bp;
    char *bufend;

    bp = buf;
    bufend = buf;

    while (1)
    {
    	char line[BUFSIZE];
	char *lp = line;

	while (1)
	{
	    if (bp >= bufend)
	    {
	    	// attempt to reload the buffer
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

	    if (*bp == '\n')
		break;

	    *lp++ = *bp++;
	}

	*lp = '\0';
	bp++;

	if (line[0] == '\0' || line[0] == '#')
	    continue;

	char *arg1 = line + 1;

	while (*arg1 != '\0' && *arg1 != ' ' && *arg1 != '\t')
	    arg1++;

	while (*arg1 == ' ' || *arg1 == '\t')
	    arg1++;

	char *arg2 = arg1 + 1;

	while (*arg2 != '\0' && *arg2 != ' ' && *arg2 != '\t')
	    arg2++;

	while (*arg2 == ' ' || *arg2 == '\t')
	    arg2++;

	int id = atoi(arg1);
	int sz = atoi(arg2);

	// process line
	switch (line[0])
	{
	case 'm':
	    runlog[runlogsz].type = LT_MALLOC;
	    runlog[runlogsz].id = id;
	    runlog[runlogsz].sz = sz;
//	    printf("malloc %d %d (\"%s\" \"%s\")\n", id, sz, arg1, arg2);
	    runlogsz++;
	    break;
	case 'f':
	    runlog[runlogsz].type = LT_FREE;
	    runlog[runlogsz].id = id;
	    runlogsz++;
	    break;
	case 'r':
	    runlog[runlogsz].type = LT_REALLOC;
	    runlog[runlogsz].id = id;
	    runlog[runlogsz].sz = sz;
	    runlogsz++;
	    break;
	default:
	    fprintf(stderr, "%s: bogus lines %d, \"%s\"\n", prog, runlogsz,
		    line);
	    close(fd);
	    return 0;
	}
    }
}

void
usage()
{
    fprintf(stderr, "Usage: %s logfile\n", prog);
    exit(2);
}

int
main(int argc, char **argv)
{
    prog = argv[0];

    if (argc < 2)
    	usage();

    if (!readlog(argv[1]))
    {
    	fprintf(stderr, "%s: cannot read log file, %s\n", prog, argv[1]);
	exit(1);
    }

#ifdef STOPWATCH
    stopwatch_start();
#else
#ifdef GETTIMEOFDAY
    struct timeval ts;
    gettimeofday(&ts, 0);
#else
    long ts = time(NULL);
#endif
#endif
    void *ps = sbrk(0);

    for (int i = 0; i < runlogsz; i++)
    {
    	int id = runlog[i].id;

    	switch (runlog[i].type)
	{
	case LT_MALLOC:
	    ptrs[id] = malloc(runlog[i].sz);

	    if (ptrs[id] != NULL)
		*(int*)ptrs[id] = 0;

//	    fprintf(stderr, "m %d %d 0x%X\n", id, runlog[i].sz, ptrs[id]);
	    break;
	case LT_FREE:
//	    fprintf(stderr, "f %d 0x%X\n", id, ptrs[id]);
	    free(ptrs[id]);
	    break;
	case LT_REALLOC:
//	    fprintf(stderr, "r %d %d 0x%X ", id, runlog[i].sz, ptrs[id]);
	    ptrs[id] = realloc(ptrs[id], runlog[i].sz);

	    if (ptrs[id] != NULL)
		*(int*)ptrs[id] = 0;

//	    fprintf(stderr, "0x%X\n", ptrs[id]);
	    break;
	default:
	    fprintf(stderr, "corrupt log entry %d, run aborted\n", i);
	    fprintf(stderr, "\ttype = %d\n", runlog[i].type);
	    fprintf(stderr, "\tid = %d\n", runlog[i].id);
	    fprintf(stderr, "\tsz = %d\n", runlog[i].sz);
	    exit(1);
	}
    }

#ifdef STOPWATCH
    double tm = stopwatch_stop();
#else
#ifdef GETTIMEOFDAY
    struct timeval te;
    struct timeval rt;
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
#else
    long te = time(NULL);
    long rt = te - ts;
#endif
#endif

    void *pe = sbrk(0);
    long rm = pe - ps;

#ifdef STOPWATCH
    printf("%.2f seconds, %dK memory\n", tm, rm / 1024);
#else
#ifdef GETTIMEOFDAY
    printf("%d.%02d seconds, %dK memory\n", rt.tv_sec, rt.tv_usec / 10000,
	    rm / 1024);
#else
    printf("%d seconds, %dK memory\n", rt, rm / 1024);
#endif
#endif

    exit(0);
}
