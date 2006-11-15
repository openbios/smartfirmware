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
int verbose = 0;
int showprob = 0;

void
usage()
{
    fprintf(stderr, "Usage: %s [-grvp] [msgs ...]\n", prog);
    exit(2);
}

/* 
 * Usage: spamcheck [-v] [msgs ...]
 */
int
main(int argc, char **argv)
{
    float prob;
    int i;
    int ch;
    int calctype = SPAM_LOOKUP_GRAHAM;
    int settype = 0;
    int type;
    extern int optind;
    char *dbname = NULL;
    int crackmime = 0;
    int crackhtml = 0;
    int maxinterest = 25;
    SPAMDB *db;

    prog = argv[0];

    while ((ch = getopt(argc, argv, "B:ghi:mrvp12")) != EOF)
	switch (ch)
	{
	case 'B':
	    dbname = optarg;
	    break;
	case '1':
	    settype = SPAM_LOOKUP_WORDS;
	    break;
	case '2':
	    settype = SPAM_LOOKUP_WORDPAIRS;
	    break;
	case 'i':
	    maxinterest = atoi(optarg);
	    break;
	case 'm':
	    crackmime = 1;
	    break;
	case 'g':
	    calctype = SPAM_LOOKUP_GRAHAM;
	    break;
	case 'h':
	    crackhtml = 1;
	    break;
	case 'r':
	    calctype = SPAM_LOOKUP_ROBINSON;
	    break;
	case 'v':
	    verbose++;
	    break;
	case 'p':
	    showprob = 1;
	    break;
	default:
	    usage();
	    break;
	}

    type = settype | calctype;

    if (crackmime)
	type |= SPAM_LOOKUP_MIME;

    if (crackhtml)
	type |= SPAM_CRACK_HTML;

    db = spamopendb(dbname);

    if (argc - optind >= 1)
    {
	for (i = optind; i < argc; i++)
	{
	    if (spamcheckfilename(db, argv[i], type, maxinterest, verbose, &prob) < 0)
	    {
		fprintf(stderr, "%s: %s: cannot open file\n", prog, argv[i]);
		prob = 0.0;
	    }
	    else if (showprob)
	    {
		printf("%s spam probability %7.4f\n", argv[i], prob);
		fflush(stdout);
	    }
	}
    }
    else
    {
	if (!spamcheckfile(db, stdin, type, maxinterest, verbose, &prob))
	    prob = 0.0;
	else if (showprob)
	    printf("<stdin> spam probability %7.4f\n", prob);
    }

    spamclosedb(db);
    return (prob < 0.7) ? 1 /*false*/ : 0 /*true*/;
}
