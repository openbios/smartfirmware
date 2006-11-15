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

#include "spamlib.h"

char *prog;
int errs = 0;

void
outputdb(SPAMDB *db)
{
    char word1[1024];
    char word2[1024];
    int good;
    int bad;
    int goodfilecnt;
    int badfilecnt;
    float prob;
    int ok;

    ok = spamfirstpair(db, SPAM_LOOKUP_GRAHAM, word1, word2, &good, &bad, &goodfilecnt, &badfilecnt,
	    &prob);

    while (ok)
    {
	printf("%s %s %d %d %d %d %7.4f\n", word1, word2, good, bad,
		goodfilecnt, badfilecnt, prob);
	ok = spamnextpair(db, SPAM_LOOKUP_GRAHAM, word1, word2, &good, &bad,
		&goodfilecnt, &badfilecnt, &prob);
    }

    ok = spamfirstword(db, SPAM_LOOKUP_GRAHAM, word1, &good, &bad, &goodfilecnt, &badfilecnt,
	    &prob);

    while (ok)
    {
	printf("%s %d %d %d %d %7.4f\n", word1, good, bad,
		goodfilecnt, badfilecnt, prob);
	ok = spamnextword(db, SPAM_LOOKUP_GRAHAM, word1, &good, &bad,
		&goodfilecnt, &badfilecnt, &prob);
    }
}

void
outputfilelist(SPAMDB *db)
{
    char filename[1024];
    int spam;
    int goodfilecnt;
    int badfilecnt;
    int ok;

    spamgetfiles(db, &goodfilecnt, &badfilecnt);

    printf("%d Non Spam file%s:\n", goodfilecnt, goodfilecnt == 1 ? "" : "s");
    ok = spamfirstfilename(db, filename, &spam);

    while (ok)
    {
	if (!spam)
	    printf("\t%s\n", filename);

	ok = spamnextfilename(db, filename, &spam);
    }

    printf("%d Spam file%s:\n", badfilecnt, badfilecnt == 1 ? "" : "s");
    ok = spamfirstfilename(db, filename, &spam);

    while (ok)
    {
	if (spam)
	    printf("\t%s\n", filename);

	ok = spamnextfilename(db, filename, &spam);
    }
}

void
outputoverview(SPAMDB *db)
{
    char word1[1024];
    char word2[1024];
    int goodfilecnt;
    int badfilecnt;
    int pairs = 0;
    int words = 0;
    int ok;

    ok = spamfirstpair(db, SPAM_LOOKUP_GRAHAM, word1, word2, NULL, NULL, NULL, NULL,
	    NULL);

    while (ok)
    {
	pairs++;
	ok = spamnextpair(db, SPAM_LOOKUP_GRAHAM, word1, word2, NULL, NULL, NULL,
		NULL, NULL);
    }

    ok = spamfirstword(db, SPAM_LOOKUP_GRAHAM, word1, NULL, NULL, NULL, NULL, NULL);

    while (ok)
    {
	words++;
	ok = spamnextword(db, SPAM_LOOKUP_GRAHAM, word1, NULL, NULL, NULL, NULL,
		NULL);
    }

    spamgetfiles(db, &goodfilecnt, &badfilecnt);

    printf("good files %d, bad files %d, words %d, pairs %d\n", goodfilecnt,
	    badfilecnt, words, pairs);
}

void
usage()
{
    fprintf(stderr, "Usage: %s [-f] [-l]\n", prog);
    exit(2);
}

/* 
 * Usage: spamtrain goodmsgs badmsgs
 */
int
main(int argc, char **argv)
{
    int full = 0;
    int list = 0;
    int ch;
    char *dbname = NULL;
    SPAMDB *db;

    prog = argv[0];
    g_spamlibprog = prog;
    g_spamliberrs = 0;

    while ((ch = getopt(argc, argv, "B:fl")) != EOF)
	switch (ch)
	{
	case 'B':
	    dbname = optarg;
	    break;
	case 'l':
	    list++;
	    break;
	case 'f':
	    full++;
	    break;
	default:
	    usage();
	    break;
	}

    db = spamopendb(dbname);

    if (g_spamliberrs > 0)
	errs++;

    if (full)
	outputdb(db);
    else if (list)
	outputfilelist(db);
    else
	outputoverview(db);

    spamclosedb(db);
    exit(errs ? 1 : 0);
}
