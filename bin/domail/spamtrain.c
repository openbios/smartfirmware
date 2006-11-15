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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include "spamlib.h"

char *prog;
int errs = 0;

char *
filesize(char *filename)
{
    struct stat statbuf;
    static char buf[10];
    off_t l;

    if (stat(filename, &statbuf) < 0)
	return "?";

    l = statbuf.st_size;

    if (l < 10000)
	sprintf(buf, "%lld", l);
    else if (l < 10*1024)
	sprintf(buf, "%lld.%lldK", l / 1024, ((l * 10) / 1024) % 10);
    else if (l < 1000*1024)
	sprintf(buf, "%lldK", l / 1024);
    else if (l < 10*1024*1024)
	sprintf(buf, "%lld.%lldK", l / (1024*1024),
		(((l / 1024) * 10) / 1024) % 10);
    else if (l < 1000*1024*1024)
	sprintf(buf, "%lldM", l / (1024*1024));
    else
	sprintf(buf, "%lldG", l / (1024*1024*1024));

    return buf;
}

void
train(SPAMDB *db, char *filelist, int spam)
{
    FILE *fp = fopen(filelist, "r");
    char buf[1024];

    if (fp == NULL)
    {
	errs++;
	fprintf(stderr, "%s: %s: cannot open file\n", prog, filelist);
	return;
    }

    while (fgets(buf, sizeof buf, fp) != NULL)
    {
	int len = strlen(buf);

	while (len > 0 && (buf[len - 1] == '\r' || buf[len - 1] == '\n'))
		len--;

	buf[len] = '\0';

	if (!spamaddfile(db, buf, spam))
	{
	    errs++;
	    fprintf(stderr, "%s: %s: cannot open file\n", prog, buf);
	    continue;
	}

	fprintf(stderr, "file: %s %s\n", buf, filesize(buf));
    }
}

int 
retrain(SPAMDB *olddb, SPAMDB *newdb)
{
    char filename[1024];
    int spam;
    int ok;

    ok = spamfirstfilename(olddb, filename, &spam);

    while (ok)
    {
	if (!spamaddfile(newdb, filename, spam))
	{
	    errs++;
	    fprintf(stderr, "%s: %s: cannot open file\n", prog, filename);
	    continue;
	}

	fprintf(stderr, "file: %s [%sspam]\n", filename, spam ? "" : "not ");
	ok = spamnextfilename(olddb, filename, &spam);
    }

    return 1;
}

void
outputprobs(SPAMDB *db)
{
    char word1[1024];
    char word2[1024];
    float prob;
    int ok;

    ok = spamfirstpair(db, SPAM_LOOKUP_GRAHAM, word1, word2, NULL, NULL, NULL, NULL, &prob);

    while (ok)
    {
	printf("%s %s %7.4f\n", word1, word2, prob);
	ok = spamnextpair(db, SPAM_LOOKUP_GRAHAM, word1, word2, NULL, NULL, NULL, NULL, &prob);
    }
}

void
usage()
{
    fprintf(stderr, "Usage: spamtrain [-B dbname] [-v] goodmsgs badmsgs\n");
    fprintf(stderr, "\tspamtrain [-v] -r\n");
    exit(2);
}

/* 
 * Usage: spamtrain goodmsgs badmsgs
 */
int
main(int argc, char **argv)
{
    extern int optind;
    int verbose = 0;
    int rebuild = 0;
    int ch;
    int n;
    int dbtype;
    char *dbname = NULL;
    SPAMDB *db;

    prog = argv[0];
    g_spamlibprog = prog;

    while ((ch = getopt(argc, argv, "B:vr")) != EOF)
	switch (ch)
	{
	case 'B':
	    dbname = optarg;
	    break;
	case 'v':
	    verbose = 1;
	    break;
	case 'r':
	    rebuild = 1;
	    break;
	default:
	    usage();
	    break;
	}

    n = argc - optind;

    if ((rebuild && n) || (!rebuild && n != 2))
	usage();

    db = spamopendb(dbname);

    if (db == NULL)
    {
	fprintf(stderr, "%s: unable to open spam database\n", prog);
	exit(1);
    }

    dbtype = spamgetdbtype(db);

    if (rebuild)
    {
	SPAMDB *newdb = spaminitdb(".mail.spamprobs.tmp", dbtype);

	if (newdb == NULL)
	{
	    fprintf(stderr, "%s: unable to open temporary spam database\n", prog);
	    exit(1);
	}

	retrain(db, newdb);
	spamclosedb(db);
	db = newdb;
    }
    else
    {
	train(db, argv[optind], 0);
	train(db, argv[optind + 1], 1);
    }

    if (verbose)
	outputprobs(db);

    spamclosedb(db);

    if (rebuild && errs == 0)
	spamrenamedb(".mail.spamprobs.tmp", NULL, dbtype);

    exit(errs ? 1 : 0);
}
