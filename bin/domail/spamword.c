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
#include <math.h>
#include <unistd.h>

#include "spamlib.h"

char *prog;
int g_type = SPAM_LOOKUP_GRAHAM;

int
wordinfo(SPAMDB *db, char *word)
{
    int goodcnt;
    int badcnt;
    int goodfilecnt;
    int badfilecnt;
    float prob;

    if (!spamlookupword(db, word, g_type, &goodcnt, &badcnt, &goodfilecnt,
	    &badfilecnt, &prob))
    {
	fprintf(stderr, "%s: %s: word not found\n", prog, word);
	return 0;
    }

    printf("%s: ham %d spam %d ham files %d spam files %d prob %f\n", word,
	    goodcnt, badcnt, goodfilecnt, badfilecnt, prob);
    return 1;
}

int
wordpairinfo(SPAMDB *db, char *word1, char *word2)
{
    int goodcnt;
    int badcnt;
    int goodfilecnt;
    int badfilecnt;
    float prob;

    if (!spamlookuppair(db, word1, word2, g_type, &goodcnt, &badcnt,
	    &goodfilecnt, &badfilecnt, &prob))
    {
	fprintf(stderr, "%s: %s %s: word pair not found\n", prog, word1, word2);
	return 0;
    }

    printf("%s %s: ham %d spam %d ham files %d spam files %d prob %f\n", word1, word2,
	    goodcnt, badcnt, goodfilecnt, badfilecnt, prob);
    return 1;
}

void
usage()
{
    fprintf(stderr, "Usage: %s word [word2]\n", prog);
    exit(2);
}

/* 
 * Usage: spamwords word1 [word2]
 */
int
main(int argc, char **argv)
{
    int ch;
    extern int optind;
    char *dbname = NULL;
    int errs = 0;
    SPAMDB *db;

    prog = argv[0];

    while ((ch = getopt(argc, argv, "B:gr")) != EOF)
	switch (ch)
	{
	case 'B':
	    dbname = optarg;
	    break;
	case 'g':
	    g_type = SPAM_LOOKUP_GRAHAM;
	    break;
	case 'r':
	    g_type = SPAM_LOOKUP_ROBINSON;
	    break;
	default:
	    usage();
	    break;
	}

    db = spamopendb(dbname);

    if (argc - optind >= 1)
    {
	if (argc - optind > 2)
	    usage();
	else if (argc - optind == 2)
	{
	    if (!wordpairinfo(db, argv[optind], argv[optind + 1]))
		errs++;
	}
	else
	{
	    if (!wordinfo(db, argv[optind]))
		errs++;
	}
    }
    else
	usage();

    spamclosedb(db);
    exit(errs ? 1 : 0);
}
