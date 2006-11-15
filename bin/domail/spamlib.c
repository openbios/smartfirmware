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


#define DEBUG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <fcntl.h>
#include <db.h>
#include <unistd.h>

#include "spamlib.h"
#include "mimelib.h"

#define DEFAULT_SPAMDB_NAME ".mail.spamprobs"

char *g_spamlibprog = "<unknown>";
int g_spamliberrs = 0;

#define KEYTYPE_FILECOUNT	1
#define KEYTYPE_FILETYPE	2		/* obsolete */
#define KEYTYPE_WORDCOUNT	3
#define KEYTYPE_WORDPAIRCOUNT	4
#define KEYTYPE_MAXID		5
#define KEYTYPE_FILEWORDCOUNT	6
#define KEYTYPE_MSGTYPE		7

struct filecountrec
{
    int goodfiles;
    int badfiles;
};

typedef struct filecountrec Filecountrec;

struct idrec
{
    int id;
};

typedef struct idrec Idrec;

struct msgtyperec
{
    int spam;
    char filename[1];
};

typedef struct msgtyperec Msgtyperec;

struct wordcountrec
{
    int id;
    int good;
    int bad;
    int goodfiles;
    int badfiles;
};

typedef struct wordcountrec Wordcountrec;

struct wordpaircountrec
{
    int good;
    int bad;
    int goodfiles;
    int badfiles;
};

typedef struct wordpaircountrec Wordpaircountrec;

SPAMDB *
spaminitdb(char *name, int key)
{
    char path[1024];
    DB *db;
    char *home = getenv("HOME");
    SPAMDB *sdb;

    if (home == NULL)
    {
	g_spamliberrs++;
	fprintf(stderr, "%s: environment variable HOME not set\n",
		g_spamlibprog);
	return (SPAMDB *)0;
    }

    if (name == NULL)
	name = DEFAULT_SPAMDB_NAME;

    switch (key)
    {
    case SPAM_INITDB_KEY:
	sprintf(path, "%s/%s.btree", home, name);
	db = xdb_open_btree(path, O_RDWR|O_CREAT|O_EXCL, 0600);
	break;
    case SPAM_INITDB_KEY_HASH:
	sprintf(path, "%s/%s.hash", home, name);
	db = xdb_open_hash(path, O_RDWR|O_CREAT|O_EXCL, 0600);
	break;
    default:
	sprintf(path, "%s/%s.???", home, name);
	db = NULL;
    }

    if (db == NULL)
    {
	g_spamliberrs++;
	fprintf(stderr, "%s: %s: unable to create database\n", g_spamlibprog,
		path);
	return (SPAMDB *)0;
    }

    sdb = (SPAMDB *)malloc(sizeof *sdb);

    if (sdb == NULL)
    {
	g_spamliberrs++;
	xdb_close(db);
	unlink(path);
	return (SPAMDB *)0;
    }

    memset(sdb, '\0', sizeof *sdb);
    sdb->d = db;
    sdb->dbtype = key;
    sdb->wordidtable = NULL;
    return sdb;
}

SPAMDB *
spamopendb(char *name)
{
    char path[1024];
    DB *db;
    char *home = getenv("HOME");
    SPAMDB *sdb;
    int key;

    if (home == NULL)
    {
	g_spamliberrs++;
	fprintf(stderr, "%s: environment variable HOME not set\n",
		g_spamlibprog);
	return (SPAMDB *)0;
    }

    if (name == NULL)
	name = DEFAULT_SPAMDB_NAME;

    sprintf(path, "%s/%s.btree", home, name);
    key = SPAM_INITDB_KEY;
    db = xdb_open_btree(path, O_RDWR, 0600);

    if (db == NULL)
    {
	sprintf(path, "%s/%s.hash", home, name);
	key = SPAM_INITDB_KEY_HASH;
	db = xdb_open_hash(path, O_RDWR, 0600);
    }

    if (db == NULL)
    {
	g_spamliberrs++;
	fprintf(stderr, "%s: %s: unable to open database\n", g_spamlibprog,
		path);
	return (SPAMDB *)0;
    }

    sdb = (SPAMDB *)malloc(sizeof *sdb);

    if (sdb == NULL)
    {
	g_spamliberrs++;
	xdb_close(db);
	return (SPAMDB *)0;
    }

    memset(sdb, '\0', sizeof *sdb);
    sdb->d = db;
    sdb->dbtype = key;
    sdb->wordidtable = NULL;
    return sdb;
}

void
spamclosedb(SPAMDB *db)
{
    if (db == NULL)
	return;

    if (db->d != NULL)
	xdb_close(db->d);

    free(db);
}

int
spamrenamedb(char *oldname, char *newname, int key)
{
    char oldpath[1024];
    char newpath[1024];
    char *home = getenv("HOME");

    if (home == NULL)
    {
	g_spamliberrs++;
	fprintf(stderr, "%s: environment variable HOME not set\n",
		g_spamlibprog);
	return 0;
    }

    if (oldname == NULL && newname == NULL)
    {
	g_spamliberrs++;
	fprintf(stderr, "%s: spamrenamedb: must specify either source or destination\n", 
		g_spamlibprog);
	return 0;
    }

    if (oldname == NULL)
	oldname = DEFAULT_SPAMDB_NAME;

    if (newname == NULL)
	newname = DEFAULT_SPAMDB_NAME;

    switch (key)
    {
    case SPAM_INITDB_KEY:
	sprintf(oldpath, "%s/%s.btree", home, oldname);
	sprintf(newpath, "%s/%s.btree", home, newname);
	break;
    case SPAM_INITDB_KEY_HASH:
	sprintf(oldpath, "%s/%s.hash", home, oldname);
	sprintf(newpath, "%s/%s.hash", home, newname);
	break;
    default:
	sprintf(oldpath, "%s/%s.???", home, oldname);
	fprintf(stderr, "%s: %s: unable to rename database\n", g_spamlibprog,
		oldpath);
	return 0;
    }

    if (rename(oldpath, newpath) != 0)
    {
	fprintf(stderr, "%s: %s: unable to rename database\n", g_spamlibprog,
		oldpath);
	return 0;
    }

    return 1;
}

int
spamgetdbtype(SPAMDB *db)
{
    if (db == NULL)
	return 0;

    return db->dbtype;
}

int
spamgetfiles(SPAMDB *db, int *goodcnt, int *badcnt)
{
    char keybuf[1024];
    DBT k;
    DBT d;
    int g;
    int b;
    int ret;

    if (db == NULL && db->d == NULL)
    {
	g_spamliberrs++;
	return 0;
    }

    keybuf[0] = KEYTYPE_FILECOUNT;
    k.data = keybuf;
    k.size = 1;

    d = xdb_fetch(db->d, k);

    if (d.data == NULL)
    {
	g = 0;
	b = 0;
	ret = 0;
    }
    else
    {
	Filecountrec *rec = (Filecountrec *)d.data;
	g = rec->goodfiles;
	b = rec->badfiles;
	ret = 1;
    }

    if (goodcnt)
	*goodcnt = g;

    if (badcnt)
	*badcnt = b;

    return ret;
}

void
spamadjustfiles(SPAMDB *db, int goodfilesinc, int badfilesinc)
{
    char keybuf[1024];
    DBT k;
    DBT d;
    Filecountrec rec;
    Filecountrec *recp;

    if (db == NULL || db->d == NULL)
    {
	g_spamliberrs++;
	return;
    }

    keybuf[0] = KEYTYPE_FILECOUNT;
    k.data = keybuf;
    k.size = 1;

    d = xdb_fetch(db->d, k);

//    fprintf(stderr, "spamadjustfiles: %d %d: %p %d\n", goodfilesinc, badfilesinc,
//	    d.data, d.size);

    if (d.data == NULL)
    {
	rec.goodfiles = 0;
	rec.badfiles = 0;
    }
    else
    {
	recp = (Filecountrec *)d.data;
	rec.goodfiles = recp->goodfiles;
	rec.badfiles = recp->badfiles;
//	fprintf(stderr, "\tgood %d bad %d: new good %d new bad %d\n",
//		rec.goodfiles, rec.badfiles,
//		rec.goodfiles + goodfilesinc, rec.badfiles + badfilesinc);
    }

    rec.goodfiles += goodfilesinc;
    rec.badfiles += badfilesinc;

    d.data = (char *)&rec;
    d.size = sizeof rec;
    xdb_store(db->d, k, d, DB_REPLACE);
}

int
spamgetnextid(SPAMDB *db)
{
    char keybuf[1024];
    DBT k;
    DBT d;
    Idrec rec;
    Idrec *recp;
    int id;

    if (db == NULL || db->d == NULL)
    {
	g_spamliberrs++;
	return 0;
    }

    keybuf[0] = KEYTYPE_MAXID;
    k.data = keybuf;
    k.size = 1;

    d = xdb_fetch(db->d, k);

//    fprintf(stderr, "spamadjustfiles: %d %d: %p %d\n", goodfilesinc, badfilesinc,
//	    d.data, d.size);

    if (d.data == NULL)
	rec.id = 1;
    else
    {
	recp = (Idrec *)d.data;
	rec.id = recp->id;
//	fprintf(stderr, "\tgood %d bad %d: new good %d new bad %d\n",
//		rec.goodfiles, rec.badfiles,
//		rec.goodfiles + goodfilesinc, rec.badfiles + badfilesinc);
    }

    id = rec.id++;

    d.data = (char *)&rec;
    d.size = sizeof rec;
    xdb_store(db->d, k, d, DB_REPLACE);
    return id;
}

void
spamadjustword(SPAMDB *db, char *word, int goodinc, int badinc, int goodfileinc,
	int badfileinc)
{
    char keybuf[1024];
    DBT k;
    DBT d;
    Wordcountrec rec;
    Wordcountrec *recp;
    int len = strlen(word);

    if (db == NULL || db->d == NULL)
    {
	g_spamliberrs++;
	return;
    }

    keybuf[0] = KEYTYPE_WORDCOUNT;
    strcpy(keybuf + 1, word);
    k.data = keybuf;
    k.size = len + 2;

    d = xdb_fetch(db->d, k);

//    fprintf(stderr, "spamadjust: \"%s\" \"%s\" %d %d: %p %d\n", word1, word2,
//	    goodinc, badinc, d.data, d.size);

    if (d.data == NULL)
    {
	rec.id = spamgetnextid(db);
	rec.good = 0;
	rec.bad = 0;
	rec.goodfiles = 0;
	rec.badfiles = 0;
    }
    else
    {
	recp = (Wordcountrec *)d.data;
	rec.id = recp->id;
	rec.good = recp->good;
	rec.bad = recp->bad;
	rec.goodfiles = recp->goodfiles;
	rec.badfiles = recp->badfiles;
//	fprintf(stderr, "\tgood %d bad %d: new good %d new bad %d\n", rec.good,
//		rec.bad, rec.good + goodinc, rec.bad + badinc);
    }

    rec.good += goodinc;
    rec.bad += badinc;
    rec.goodfiles += goodfileinc;
    rec.badfiles += badfileinc;

    if (rec.good || rec.bad || rec.goodfiles || rec.badfiles ||
	    !(goodinc || badinc || goodfileinc || badfileinc))
    {
	d.data = (char *)&rec;
	d.size = sizeof rec;
	xdb_store(db->d, k, d, DB_REPLACE);
    }
    else
	xdb_delete(db->d, k);
}

struct wordident
{
    char *word;
    int id;
};

typedef struct wordident Wordident;

static unsigned int
strhash(char const *str)
{
    unsigned int hash = 0;

    while (*str != '\0')
	hash = (hash << 2) + *str++;

    return hash ^ (hash >> 16);
}

static int
wordidcmp(void *p1, void *p2)
{
    Wordident *wp1 = (Wordident *)p1;
    Wordident *wp2 = (Wordident *)p2;
    return strcmp(wp1->word, wp2->word);
}

static void
wordiddestroy(void *p)
{
    Wordident *wp = (Wordident *)p;
    free(wp->word);
}

static int
wordidlookup(Hashtable *ht, char *word)
{
    unsigned int hash = strhash(word);
    Wordident ent;
    Wordident *entp;

    if (ht == NULL)
	return 0;

    ent.word = word;
    entp = (Wordident *)hashtablelookup(ht, &ent, hash);

    if (entp == NULL)
	return 0;

    return entp->id;
}

static void
wordidadd(Hashtable **htp, char *word, int id)
{
    unsigned int hash = strhash(word);
    Wordident ent;
    Wordident *entp;

    if (*htp == NULL)
	*htp = hashtablecreate(sizeof (Wordident), wordidcmp, wordiddestroy);

    if (*htp == NULL)
	return;

    ent.word = strdup(word);
    entp = (Wordident *)hashtableadd(*htp, &ent, hash);

    if (entp != NULL)
	entp->id = id;
    else
	free(ent.word);
}

void
wordidfree(Hashtable *ht)
{
    if (ht != NULL)
	hashtableclear(ht);
}

int
spamgetwordid(SPAMDB *db, char *word)
{
    char keybuf[1024];
    DBT k;
    DBT d;
    int len = strlen(word);
    int id;

    if (db == NULL || db->d == NULL)
    {
	g_spamliberrs++;
	return 0;
    }

    id = wordidlookup(db->wordidtable, word);

    if (id > 0)
	return id;

    keybuf[0] = KEYTYPE_WORDCOUNT;
    strcpy(keybuf + 1, word);
    k.data = keybuf;
    k.size = len + 2;

    d = xdb_fetch(db->d, k);

//    fprintf(stderr, "spamadjust: \"%s\" \"%s\" %d %d: %p %d\n", word1, word2,
//	    goodinc, badinc, d.data, d.size);

    if (d.data == NULL)
	return 0;

    id = ((Wordcountrec *)d.data)->id;
    wordidadd(&db->wordidtable, word, id);
    return id;
}

void
spamadjustpair(SPAMDB *db, char *word1, char *word2, int goodinc, int badinc,
	int goodfileinc, int badfileinc)
{
    char keybuf[1024];
    DBT k;
    DBT d;
    Wordpaircountrec rec;
    Wordpaircountrec *recp;
    int id1 = spamgetwordid(db, word1);
    int id2 = spamgetwordid(db, word2);

    if (db == NULL || db->d == NULL)
    {
	printf("spamadjustpair: %s %s: no db\n", word1, word2);
	g_spamliberrs++;
	return;
    }

    if (id1 == 0)
    {
	spamadjustword(db, word1, 0, 0, 0, 0);
	id1 = spamgetwordid(db, word1);
    }

    if (id2 == 0)
    {
	spamadjustword(db, word2, 0, 0, 0, 0);
	id2 = spamgetwordid(db, word2);
    }

    if (id1 == 0 || id2 == 0)
    {
	printf("spamadjustpair: %s %s: %d %d: no id\n", word1, word2, id1, id2);
	g_spamliberrs++;
	return;
    }

    keybuf[0] = KEYTYPE_WORDPAIRCOUNT;
    keybuf[1] = id1 & 0xFF;
    keybuf[2] = (id1 >> 8) & 0xFF;
    keybuf[3] = (id1 >> 16) & 0xFF;
    keybuf[4] = (id1 >> 24) & 0xFF;
    keybuf[5] = id2 & 0xFF;
    keybuf[6] = (id2 >> 8) & 0xFF;
    keybuf[7] = (id2 >> 16) & 0xFF;
    keybuf[8] = (id2 >> 24) & 0xFF;
    k.data = keybuf;
    k.size = 9;

    d = xdb_fetch(db->d, k);

//    fprintf(stderr, "spamadjust: \"%s\" \"%s\" %d %d: %p %d\n", word1, word2,
//	    goodinc, badinc, d.data, d.size);

    if (d.data == NULL)
    {
	rec.good = 0;
	rec.bad = 0;
	rec.goodfiles = 0;
	rec.badfiles = 0;
    }
    else
    {
	recp = (Wordpaircountrec *)d.data;
	rec.good = recp->good;
	rec.bad = recp->bad;
	rec.goodfiles = recp->goodfiles;
	rec.badfiles = recp->badfiles;
//	fprintf(stderr, "\tgood %d bad %d: new good %d new bad %d\n", rec.good,
//		rec.bad, rec.good + goodinc, rec.bad + badinc);
    }

    rec.good += goodinc;
    rec.bad += badinc;
    rec.goodfiles += goodfileinc;
    rec.badfiles += badfileinc;

    if (rec.good || rec.bad || rec.goodfiles || rec.badfiles)
    {
	d.data = (char *)&rec;
	d.size = sizeof rec;
	xdb_store(db->d, k, d, DB_REPLACE);
    }
    else
	xdb_delete(db->d, k);
}

/* return Message-ID header from file, or original filename
   if no such header is found
 */
char *
spamgetmessageid(char *filename)
{
    #define ID_LEN 255
    #define HDR_LEN 255
    static char id[ID_LEN + 1];
    FILE *fp;
    char ch;
    char hdr[HDR_LEN + 1];
    int i;

    fp = fopen(filename, "r");

    if (fp == NULL)
	return filename;

    id[0] = '\0';

    while ((ch = fgetc(fp)) != EOF)
    {
	/* continued from previous header? - skip it */
	if (ch == ' ' || ch == '\t')
	{
	    while (ch == ' ' || ch == '\t')
		ch = fgetc(fp);

	    /* hit end-of-line? - end of headers */
	    if (ch == '\r' || ch == '\n')
		goto done;    

	    while (ch != '\r' && ch != '\n' && ch != EOF)
		ch = fgetc(fp);

	    continue;
	}

	/* get the header name */
	for (i = 0; ch != ':' && ch != '\r' && ch != '\n' && ch != EOF &&
		i < HDR_LEN; i++)
	{
	    hdr[i] = isupper(ch) ? tolower(ch) : ch;
	    ch = fgetc(fp);
	}

	hdr[i] = '\0';
	//fprintf(stderr, "HDR='%s'\n", hdr);

	/* did not get a header name, it was too long, or not message-id */
	if (ch != ':' || strcmp(hdr, "message-id") != 0)
	{
	    /* skip this one */
	    while (ch != '\r' && ch != '\n' && ch != EOF)
		ch = fgetc(fp);

	    continue;
	}

	/* message-id header - first skip ':' */
	ch = fgetc(fp);

	/* skip white space and start of ID */
	while (ch == ' ' || ch == '\t' || ch == '<')
	    ch = fgetc(fp);

	/* get the ID */
	for (i = 0; ch != '>' && ch != '\r' && ch != '\n' && ch != EOF &&
		i < ID_LEN; i++)
	{
	    id[i] = ch;
	    ch = fgetc(fp);
	}

	id[i] = '\0';

	/* and we are done */
	break;
    }

done:
    fclose(fp);
    return id[0] == '\0' ? filename : id;
}

/* return 0 if unknown, 1 is not spam, 2 if is spam */
int
spamgetmsgtype(SPAMDB *db, char *msgid, char *lastfilename, int len)
{
    char keybuf[1024];
    DBT k;
    DBT d;
    Msgtyperec *recp;

    if (db == NULL || db->d == NULL)
    {
	g_spamliberrs++;
	return MSGTYPE_UNKNOWN;
    }

    if (msgid == NULL)
	return MSGTYPE_UNKNOWN;

    keybuf[0] = KEYTYPE_MSGTYPE;
    strcpy(keybuf + 1, msgid);
    k.data = keybuf;
    k.size = strlen(msgid) + 2;
    d = xdb_fetch(db->d, k);

    if (d.data == NULL)
	return MSGTYPE_UNKNOWN;

    recp = (Msgtyperec *)d.data;

    if (lastfilename && len)
    {
	strncpy(lastfilename, recp->filename, len - 1);
	lastfilename[len - 1] = '\0';
    }

    if (recp->spam)
	return MSGTYPE_SPAM;

    return MSGTYPE_NOT_SPAM;
}

void
spamchangemsgtype(SPAMDB *db, char *msgid, int spam, char *newfilename)
{
    char keybuf[1024];
    Msgtyperec *recp;
    int l;
    DBT k;
    DBT d;

    if (db == NULL || db->d == NULL)
    {
	g_spamliberrs++;
	return;
    }

    keybuf[0] = KEYTYPE_MSGTYPE;
    strcpy(keybuf + 1, msgid);
    k.data = keybuf;
    k.size = strlen(msgid) + 2;
    l = sizeof (Msgtyperec) + (newfilename ? strlen(newfilename) : 0);
    recp = (Msgtyperec *)malloc(l);

    if (recp == NULL)
    {
	g_spamliberrs++;
	return;
    }

    recp->spam = spam;

    if (newfilename)
	strcpy(recp->filename, newfilename);
    else
	recp->filename[0] = '\0';

    d.data = (char *)recp;
    d.size = l;
    xdb_store(db->d, k, d, DB_REPLACE);
}

void
spamremovemsgtype(SPAMDB *db, char *msgid)
{
    char keybuf[1024];
    DBT k;

    if (db == NULL || db->d == NULL)
    {
	g_spamliberrs++;
	return;
    }

    keybuf[0] = KEYTYPE_MSGTYPE;
    strcpy(keybuf + 1, msgid);
    k.data = keybuf;
    k.size = strlen(msgid) + 2;
    xdb_delete(db->d, k);
}

static int
spamgetwordcount(SPAMDB *db, char *word, int *goodcnt, int *badcnt,
	int *goodfilecnt, int *badfilecnt)
{
    char keybuf[1024];
    DBT k;
    DBT d;
    int len = strlen(word);
    int g;
    int b;
    int gfc;
    int bfc;
    int ret;

    if (db == NULL || db->d == NULL)
    {
	g_spamliberrs++;
	return 0;
    }

    keybuf[0] = KEYTYPE_WORDCOUNT;
    strcpy(keybuf + 1, word);
    k.data = keybuf;
    k.size = len + 2;

    d = xdb_fetch(db->d, k);

    if (d.data == NULL)
    {
	g = 0;
	b = 0;
	gfc = 0;
	bfc = 0;
	ret = 0;
    }
    else
    {
	Wordcountrec *rec = (Wordcountrec *)d.data;
	g = rec->good;
	b = rec->bad;
	gfc = rec->goodfiles;
	bfc = rec->badfiles;
	ret = 1;
    }

    if (goodcnt)
	*goodcnt = g;

    if (badcnt)
	*badcnt = b;

    if (goodfilecnt)
	*goodfilecnt = gfc;

    if (badfilecnt)
	*badfilecnt = bfc;

    return ret;
}

static int
spamgetpaircount(SPAMDB *db, char *word1, char *word2, int *goodcnt, int *badcnt,
	int *goodfilecnt, int *badfilecnt)
{
    char keybuf[1024];
    DBT k;
    DBT d;
    int id1 = spamgetwordid(db, word1);
    int id2 = spamgetwordid(db, word2);
    int g;
    int b;
    int gfc;
    int bfc;
    int ret;

    if (db == NULL || db->d == NULL)
    {
	g_spamliberrs++;
	return 0;
    }

    if (id1 == 0 || id2 == 0)
    {
	g = 0;
	b = 0;
	gfc = 0;
	bfc = 0;
	ret = 0;
    }
    else
    {
	keybuf[0] = KEYTYPE_WORDPAIRCOUNT;
	keybuf[1] = id1 & 0xFF;
	keybuf[2] = (id1 >> 8) & 0xFF;
	keybuf[3] = (id1 >> 16) & 0xFF;
	keybuf[4] = (id1 >> 24) & 0xFF;
	keybuf[5] = id2 & 0xFF;
	keybuf[6] = (id2 >> 8) & 0xFF;
	keybuf[7] = (id2 >> 16) & 0xFF;
	keybuf[8] = (id2 >> 24) & 0xFF;
	k.data = keybuf;
	k.size = 9;

	d = xdb_fetch(db->d, k);

	if (d.data == NULL)
	{
	    g = 0;
	    b = 0;
	    gfc = 0;
	    bfc = 0;
	    ret = 0;
	}
	else
	{
	    Wordpaircountrec *rec = (Wordpaircountrec *)d.data;
	    g = rec->good;
	    b = rec->bad;
	    gfc = rec->goodfiles;
	    bfc = rec->badfiles;
	    ret = 1;
	}
    }

    if (goodcnt)
	*goodcnt = g;

    if (badcnt)
	*badcnt = b;

    if (goodfilecnt)
	*goodfilecnt = gfc;

    if (badfilecnt)
	*badfilecnt = bfc;

    return ret;
}

// From Paul Graham 
static float
grahamcalcprob(int good, int bad, int goodfilecnt, int badfilecnt,
	int totalgoodfiles, int totalbadfiles)
{
    float fg;
    float fb;
    float p;

//    fprintf(stderr, "calcprob: %d %d %d %d\n", good, bad, totalgoodfiles, totalbadfiles);

    if ((good * 2) + bad < 5)
	return 0.4;

    if (totalgoodfiles != 0)
	fg = (float)(good * 2) / totalgoodfiles;
    else if (good == 0)
	fg = 0.0;
    else
	fg = 1.0;

    if (totalbadfiles != 0)
	fb = (float)bad / totalbadfiles;
    else if (bad == 0)
	fb = 0.0;
    else
	fb = 1.0;

    if (fg > 1.0)
	fg = 1.0;

    if (fb > 1.0)
	fb = 1.0;

    p = fb / (fg + fb);

    if (p > 0.99)
	p = 0.99;

    if (p < 0.01)
	p = 0.01;

    return p;
}

// From Gary Robinson
// http://radio.weblogs.com/0101454/stories/2002/09/16/spamDetection.html
static float
robinsoncalcprob(int good, int bad, int goodfilecnt, int badfilecnt,
	int totalgoodfiles, int totalbadfiles)
{
    float a = 1.0;
    float b = 2.0;
    float n = a + badfilecnt;
    float d = (a * b) + (goodfilecnt + badfilecnt);
    float f = n / d;

    if (f > 0.99)
	f = 0.99;

    if (f < 0.01)
	f = 0.01;

    return f;
}

static float
calcprob(int type, int good, int bad, int goodfilecnt, int badfilecnt,
	int totalgoodfiles, int totalbadfiles)
{
    switch (type)
    {
    case SPAM_LOOKUP_GRAHAM:
	return grahamcalcprob(good, bad, goodfilecnt, badfilecnt,
		totalgoodfiles, totalbadfiles);
    case SPAM_LOOKUP_ROBINSON:
	return robinsoncalcprob(good, bad, goodfilecnt, badfilecnt,
		totalgoodfiles, totalbadfiles);
    }

    return grahamcalcprob(good, bad, goodfilecnt, badfilecnt,
	    totalgoodfiles, totalbadfiles);
}

int
spamlookupword(SPAMDB *db, char *word, int type, int *goodcnt, int *badcnt,
	int *goodfilecnt, int *badfilecnt, float *prob)
{
    /* return 0 on error */
    /* return 1 if found in db and we can get the file counts to compute */
    /* 	the spam probability */
    /* return 2 if not found in db and were making up a probabilty */
    int ok;
    int ok2;
    int g;
    int b;
    int gfc;
    int bfc;
    int gf;
    int bf;
    float p;

    if (db == NULL || db->d == NULL)
    {
	g_spamliberrs++;
	return 0;
    }

    ok = spamgetwordcount(db, word, &g, &b, &gfc, &bfc);

    if (!ok)
    {
	g = 0;
	b = 0;
	gfc = 0;
	bfc = 0;
    }
	
    ok2 = spamgetfiles(db, &gf, &bf);

    if (!ok2)
    {
	gf = 0;
	bf = 0;
	ok = ok2;
    }

    p = calcprob(type, g, b, gfc, bfc, gf, bf);

    if (goodcnt)
	*goodcnt = g;

    if (badcnt)
	*badcnt = b;

    if (goodfilecnt)
	*goodfilecnt = gfc;

    if (badfilecnt)
	*badfilecnt = bfc;

    if (prob)
	*prob = p;

    return ok ? 1 : 2;
}

int
spamlookuppair(SPAMDB *db, char *word1, char *word2, int type, int *goodcnt, int *badcnt,
	int *goodfilecnt, int *badfilecnt, float *prob)
{
    /* return 0 on error */
    /* return 1 if found in db and we can get the file counts to compute */
    /* 	the spam probability */
    /* return 2 if not found in db and were making up a probabilty */
    int ok;
    int ok2;
    int g;
    int b;
    int gfc;
    int bfc;
    int gf;
    int bf;
    float p;

    if (db == NULL || db->d == NULL)
    {
	g_spamliberrs++;
	return 0;
    }

    ok = spamgetpaircount(db, word1, word2, &g, &b, &gfc, &bfc);

    if (!ok)
    {
	g = 0;
	b = 0;
	gfc = 0;
	bfc = 0;
    }
	
    ok2 = spamgetfiles(db, &gf, &bf);

    if (!ok2)
    {
	gf = 0;
	bf = 0;
	ok = ok2;
    }

    p = calcprob(type, g, b, gfc, bfc, gf, bf);

    if (goodcnt)
	*goodcnt = g;

    if (badcnt)
	*badcnt = b;

    if (goodfilecnt)
	*goodfilecnt = gfc;

    if (badfilecnt)
	*badfilecnt = bfc;

    if (prob)
	*prob = p;

    return ok ? 1 : 2;
}

static void
makeidtable(SPAMDB *db)
{
    char keybuf[10];
    DBT k;
    DBT d;
    Wordcountrec *recp;
    int i;
    int id;

    if (db == NULL || db->d == NULL)
    {
	g_spamliberrs++;
	return;
    }

    keybuf[0] = KEYTYPE_MAXID;
    k.data = keybuf;
    k.size = 1;

    d = xdb_fetch(db->d, k);

    if (d.data == NULL)
	return;

    /* free the old table */
    for (i = 0; i < db->idtablelen; i++)
	if (db->idtable[i])
	    free(db->idtable[i]);

    free(db->idtable);

    /* allocate the idtabl */
    db->idtablelen = ((Idrec *)d.data)->id + 1;
    db->idtable = (char **)malloc(sizeof (char *) * db->idtablelen);

    if (db->idtable == NULL)
    {
	db->idtablelen = 0;
	return;
    }

    for (i = 0; i < db->idtablelen; i++)
	db->idtable[i] = NULL;

    for (k = xdb_firstkey(db->d); k.data != NULL; k = xdb_nextkey(db->d))
    {
	char *keyptr = (char *)k.data;

	if (keyptr[0] != KEYTYPE_WORDCOUNT)
	    continue;

	d = xdb_fetch(db->d, k);

	if (d.data == NULL)
	    continue;

	recp = (Wordcountrec *)d.data;
	id = recp->id;

	if (db->idtable[id] != NULL)
	    fprintf(stderr, "%s and %s have the same id (%d)\n", db->idtable[id],
		    &keyptr[1], id);
	else
	    db->idtable[id] = strdup(&keyptr[1]);
    }
}

static char *
id2word(SPAMDB *db, int id)
{
    static char word[20];

    if (db != NULL && db->idtable == NULL)
	makeidtable(db);

    if (db != NULL && db->idtable != NULL && id >= 0 && id < db->idtablelen &&
	    db->idtable[id] != NULL)
	return db->idtable[id];

    sprintf(word, "???id%d", id);
    return word;
}

static int goodfiles;
static int badfiles;

static int
splitpair(SPAMDB *db, DBT k, int type, char *word1, char *word2, int *good, int *bad,
	int *goodfilecnt, int *badfilecnt, float *prob)
{
    DBT d;
    char *key;
    int id1;
    int id2;
    float p;
    int g = 0;
    int b = 0;
    int gfc = 0;
    int bfc = 0;

    if (k.data == NULL)
	return 0;

    key = k.data;

    while (key[0] != KEYTYPE_WORDPAIRCOUNT)
    {
	k = xdb_nextkey(db->d);

	if (k.data == NULL)
	    return 0;

	key = k.data;
    }

    id1 = (key[1] & 0xFF) | ((key[2] & 0xFF) << 8) |
	    ((key[3] & 0xFF) << 16) | ((key[4] & 0xFF) << 24);
    id2 = (key[5] & 0xFF) | ((key[6] & 0xFF) << 8) |
	    ((key[7] & 0xFF) << 16) | ((key[8] & 0xFF) << 24);

    d = xdb_fetch(db->d, k);

    if (word1)
	strcpy(word1, id2word(db, id1));

    if (word2)
	strcpy(word2, id2word(db, id2));

    if (d.data == NULL)
	p = 0.4;
    else
    {
	Wordpaircountrec *rec = (Wordpaircountrec *)d.data;
	g = rec->good;
	b = rec->bad;
	gfc = rec->goodfiles;
	bfc = rec->badfiles;
	p = calcprob(type, g, b, gfc, bfc, goodfiles, badfiles);
    }

    if (good)
	*good = g;

    if (bad)
	*bad = b;

    if (goodfilecnt)
	*goodfilecnt = gfc;

    if (badfilecnt)
	*badfilecnt = bfc;

    if (prob)
	*prob = p;

    return 1;
}

int
spamfirstpair(SPAMDB *db, int type, char *word1, char *word2, int *good, int *bad,
	int *goodfilecnt, int *badfilecnt, float *prob)
{
    int ok = spamgetfiles(db, &goodfiles, &badfiles);

    if (!ok)
    {
	goodfiles = 0;
	badfiles = 0;
    }

    if (db == NULL || db->d == NULL)
	return 0;

    makeidtable(db);

    return splitpair(db, xdb_firstkey(db->d), type, word1, word2, good, bad,
	    goodfilecnt, badfilecnt, prob);
}

int
spamnextpair(SPAMDB *db, int type, char *word1, char *word2, int *good, int *bad,
	int *goodfilecnt, int *badfilecnt, float *prob)
{
    if (db == NULL || db->d == NULL)
	return 0;

    return splitpair(db, xdb_nextkey(db->d), type, word1, word2, good, bad,
	    goodfilecnt, badfilecnt, prob);
}

static int
splitword(SPAMDB *db, DBT k, int type, char *word, int *good, int *bad,
	int *goodfilecnt, int *badfilecnt, float *prob)
{
    DBT d;
    char *key;
    float p;
    int g = 0;
    int b = 0;
    int gfc = 0;
    int bfc = 0;

    if (k.data == NULL)
	return 0;

    key = k.data;

    while (key[0] != KEYTYPE_WORDCOUNT)
    {
	k = xdb_nextkey(db->d);

	if (k.data == NULL)
	    return 0;

	key = k.data;
    }

    if (word)
	strcpy(word, &key[1]);

    d = xdb_fetch(db->d, k);

    if (d.data == NULL)
	p = 0.4;
    else
    {
	Wordcountrec *rec = (Wordcountrec *)d.data;
	g = rec->good;
	b = rec->bad;
	gfc = rec->goodfiles;
	bfc = rec->badfiles;
	p = calcprob(type, g, b, gfc, bfc, goodfiles, badfiles);
    }

    if (good)
	*good = g;

    if (bad)
	*bad = b;

    if (goodfilecnt)
	*goodfilecnt = gfc;

    if (badfilecnt)
	*badfilecnt = bfc;

    if (prob)
	*prob = p;

    return 1;
}

int
spamfirstword(SPAMDB *db, int type, char *word, int *good, int *bad,
	int *goodfilecnt, int *badfilecnt, float *prob)
{
    int ok = spamgetfiles(db, &goodfiles, &badfiles);

    if (!ok)
    {
	goodfiles = 0;
	badfiles = 0;
    }

    if (db == NULL || db->d == NULL)
	return 0;

    return splitword(db, xdb_firstkey(db->d), type, word, good, bad,
	    goodfilecnt, badfilecnt, prob);
}

int
spamnextword(SPAMDB *db, int type, char *word, int *good, int *bad,
	int *goodfilecnt, int *badfilecnt, float *prob)
{
    if (db == NULL || db->d == NULL)
	return 0;

    return splitword(db, xdb_nextkey(db->d), type, word, good, bad,
	    goodfilecnt, badfilecnt, prob);
}

static int
splitfilename(SPAMDB *db, DBT k, char *filename, int *spam)
{
    DBT d;
    char *key;
    int s = 0;

    if (k.data == NULL)
	return 0;

    key = k.data;

    while (key[0] != KEYTYPE_MSGTYPE)
    {
	k = xdb_nextkey(db->d);

	if (k.data == NULL)
	    return 0;

	key = k.data;
    }

    d = xdb_fetch(db->d, k);

    if (d.data != NULL)
    {
	Msgtyperec *rec = (Msgtyperec *)d.data;
	s = rec->spam;

	if (filename)
	    strcpy(filename, rec->filename);
    }
    else
	if (filename)
	    strcpy(filename, &key[1]);

    if (spam)
	*spam = s;

    return 1;
}

int
spamfirstfilename(SPAMDB *db, char *filename, int *spam)
{
    if (db == NULL || db->d == NULL)
	return 0;

    return splitfilename(db, xdb_firstkey(db->d), filename, spam);
}

int
spamnextfilename(SPAMDB *db, char *filename, int *spam)
{
    if (db == NULL || db->d == NULL)
	return 0;

    return splitfilename(db, xdb_nextkey(db->d), filename, spam);
}

struct headerprefix
{
    char *name;
    char *prefix;
};

struct headerprefix headerprefixtable[] = 
{
    { "From", "F*" },
    { "Subject", "S*" },
    { "To", "T*" },
    { NULL, NULL },
};

char *
headerprefixlookup(char *headername)
{
    int i;

    for (i = 0; headerprefixtable[i].name; i++)
	if (strcasecmp(headername, headerprefixtable[i].name) == 0)
	    return headerprefixtable[i].prefix;

    return NULL;
}

static char nullword[] = "<nullword>";

static char *
spamparsemimeword(MIME *mime, char **headername)
{
    static char buf1[1024];
    static char buf2[1024];
    static char *buf = buf1;
    char *bp = buf;
    int ch;
    int letter = 0;
    int len;
    char *h;

    while ((ch = mime_getch(mime)) != EOF)
    {
	if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || 
		(ch >= '0' && ch <= '9') || ch == '-' || ch == '\'' ||
		ch == '$')
	    break;

	if (ch == MIME_BOC)
	{
	    char *type = mime_contenttype(mime);

	    // ignore content other than text/*, message/* or multipart/*
	    if (type && strcasecmp(type, "text") != 0 &&
		    strcasecmp(type, "message") != 0 &&
		    strcasecmp(type, "multipart") != 0)
	    {
		while (ch != MIME_EOC && ch != EOF)
		    ch = mime_getch(mime);
	    }
	}
    }

    if (ch == EOF)
	return (char *)NULL;

    h = mime_headername(mime);

    if (headername)
	*headername = h;

    if (h)
    {
	char *p;

	/* lookup header */
	p = headerprefixlookup(h);

	if (p)
	{
//	    printf("Header = %s, Prefix = %s\n", h, p);
	    strcpy(bp, p);
	    bp += strlen(p);
	}
    }

    while ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || 
	    (ch >= '0' && ch <= '9') || ch == '-' || ch == '\'' ||
	    ch == '$' || ch == MIME_TAG)
    {
	if (ch != MIME_TAG)
	{
	    if (ch >= 'A' && ch <= 'Z')
	    {
		ch = ch - 'A' + 'a';
		letter = 1;
	    }
	    else if (ch >= 'a' && ch <= 'z')
		letter = 1;

	    if (bp - buf < 1023)
		*bp++ = ch;
	}

	ch = mime_getch(mime);
    }

    if (ch != EOF)
	mime_ungetch(mime, ch);

    len = bp - buf;

    if (!letter || len < 3 || len > 30)
	return nullword;

    *bp = '\0';
//    printf("token = \"%s\"\n", buf);

    bp = buf;

    if (buf == buf1)
	buf = buf2;
    else
	buf = buf1;

//    printf("Word = \"%s\"\n", bp);
    return bp;
}

#define MAX_WORDS		5

struct wordseq
{
    int len;
    char *words[MAX_WORDS];
};

typedef struct wordseq Wordseq;

static unsigned int
wordseqhash(char *words[], int num)
{
    unsigned int hash = 0;
    int i;

    for (i = 0; i < num; i++)
    	hash ^= strhash(words[i]);

    return hash;
}

static int
wordseqcmp(void *p1, void *p2)
{
    Wordseq *ws1 = (Wordseq *)p1;
    Wordseq *ws2 = (Wordseq *)p2;
    int i;
    int m = ws1->len;

    if (m > ws2->len)
	m = ws2->len;

    for (i = 0; i < m; i++)
    {
	int x = strcmp(ws1->words[i], ws2->words[i]);

	if (x != 0)
		return x;
    }

    return ws1->len - ws2->len;
}

static void
wordseqdestroy(void *p)
{
    Wordseq *ws = (Wordseq *)p;
    int i;

    for (i = 0; i < ws->len; i++)
	free(ws->words[i]);
}

static Wordseq *
wordseqlookup(Hashtable *ht, char *words[], int num)
{
    unsigned int hash = wordseqhash(words, num);
    Wordseq seq;
    int i;

    if (ht == NULL)
	return (Wordseq *)NULL;

    for (i = 0; i < num; i++)
	seq.words[i] = words[i];

    seq.len = num;
    return (Wordseq *)hashtablelookup(ht, &seq, hash);
}

static Wordseq *
wordseqadd(Hashtable **htp, char *words[], int num)
{
    unsigned int hash = wordseqhash(words, num);
    Wordseq seq;
    Wordseq *ent;
    int i;

    if (*htp == NULL)
	*htp = hashtablecreate(sizeof (Wordseq), wordseqcmp, wordseqdestroy);

    if (*htp == NULL)
	return (Wordseq *)NULL;

    for (i = 0; i < num; i++)
	seq.words[i] = strdup(words[i]);

    seq.len = num;
    ent = (Wordseq *)hashtableadd(*htp, &seq, hash);

    if (ent != NULL)
	return ent;

    for (i = 0; i < num; i++)
	free(seq.words[i]);

    return NULL;
}

static void
wordseqfree(Hashtable *ht)
{
    if (ht != NULL)
	hashtableclear(ht);
}

int
spamparsefile(FILE *fp, int crackmime, int crackhtml, spamwordfunctype wordseqfunc, int max)
{
    char *word;
    Hashtable *wordseqtable = NULL;
    MIME *mime = NULL;
    char *wordseq[MAX_WORDS + 1];
    int flags;
    int seqlen = 0;
    int i;

    if (max < 1)
	max = 1;

    if (max > MAX_WORDS)
	max = MAX_WORDS;

    if (crackmime)
    {
	flags = MIME_HEADERS|MIME_CONTENTHEADERS|MIME_CONTENT;

	if (crackhtml)
	    flags |= MIME_CRACKHTML;
    }
    else
	flags = MIME_NOCRACK;

    mime = mime_open(fp, flags);

    if (mime == NULL)
	return 0;

    word = spamparsemimeword(mime, NULL);

    while (word != NULL)
    {
	if (word == nullword)
	{
	    seqlen = 0;
	    word = spamparsemimeword(mime, NULL);
	    continue;
	}

	if (seqlen == max)
	{
	    for (i = 0; i < max - 1; i++)
		wordseq[i] = wordseq[i + 1];
	}
	else
	    seqlen++;

	wordseq[seqlen - 1] = word;

	if (wordseqfunc != NULL)
	{
	    for (i = 0; i < seqlen; i++)
	    {
		int first = wordseqlookup(wordseqtable, &wordseq[i],
			seqlen - i) == NULL;

		if (first)
		    wordseqadd(&wordseqtable, &wordseq[i], seqlen - i);

		(*wordseqfunc)(&wordseq[i], seqlen - i, first);
	    }
	}

	word = spamparsemimeword(mime, NULL);
    }

    wordseqfree(wordseqtable);
    mime_close(mime);
    return 1;
}

int
spamparsefilename(char *filename, int crackmime, int crackhtml, spamwordfunctype wordfunc, int max)
{
    FILE *fp;
    int ok;

    fp = fopen(filename, "r");

    if (fp == NULL)
	return 0;

    ok = spamparsefile(fp, crackmime, crackhtml, wordfunc, max);
    fclose(fp);
    return ok;
}

static SPAMDB *pdb;
static int gi;
static int bi;

static void
adjustseqcount(char *words[], int num, int first)
{
    if (num == 1)
    {
	if (first)
	    spamadjustword(pdb, words[0], gi, bi, gi, bi);
	else
	    spamadjustword(pdb, words[0], gi, bi, 0, 0);
    }
    else if (num == 2)
    {
	if (first)
	    spamadjustpair(pdb, words[0], words[1], gi, bi, gi, bi);
	else
	    spamadjustpair(pdb, words[0], words[1], gi, bi, 0, 0);
    }
}

/* Combine spamaddfile() and spamupdatefile(). By using message-ids, we
 * determine if the message has already been scanned and correct it.
 */
int
spamupdatefile(SPAMDB *db, char *filename, int spam)
{
    int ok;
    char *msgid = spamgetmessageid(filename);
    int type = spamgetmsgtype(db, msgid, NULL, 0);

    if (type == MSGTYPE_UNKNOWN)
    {
	/* new message */
	if (spam)
	{
	    gi = 0;
	    bi = 1;
	}
	else
	{
	    gi = 1;
	    bi = 0;
	}
    }
    else if (type == MSGTYPE_NOT_SPAM && spam)
    {
	/* incorrectly filed as not-spam */
	gi = -1;
	bi = 1;
    }
    else if (type == MSGTYPE_SPAM && !spam)
    {
	/* incorrectly filed as spam */
	gi = 1;
	bi = -1;
    }
    else
    {
	/* already filed correctly - nothing to do */
	return OK;
    }

    pdb = db;
    ok = spamparsefilename(filename, 1, 1, adjustseqcount, 2);

    if (ok)
    {
	if (type == MSGTYPE_UNKNOWN)
	{
	    if (spam)
		spamadjustfiles(db, 0, 1);
	    else
		spamadjustfiles(db, 1, 0);
	}
	else if (type == MSGTYPE_NOT_SPAM && spam)
	{
	    spamadjustfiles(db, -1, 1);
	}
	else if (type == MSGTYPE_SPAM && !spam)
	{
	    spamadjustfiles(db, 1, -1);
	}

	spamchangemsgtype(db, msgid, spam, filename);
    }

    return ok;
}

int
spamaddfile(SPAMDB *db, char *filename, int spam)
{
    return spamupdatefile(db, filename, spam);
}

int
spamremovefile(SPAMDB *db, char *filename, int spam)
{
    int ok;

    if (spam)
    {
	gi = 0;
	bi = -1;
    }
    else
    {
	gi = -1;
	bi = 0;
    }

    pdb = db;
    ok = spamparsefilename(filename, 1, 1, adjustseqcount, 2);

    if (ok)
    {
	if (spam)
	    spamadjustfiles(db, 0, -1);
	else
	    spamadjustfiles(db, -1, 0);
    }

    spamremovemsgtype(db, spamgetmessageid(filename));
    return ok;
}

int
spamrenamefile(SPAMDB *db, char *oldname, char *newname, int spam)
{
    char *msgid = spamgetmessageid(newname);
    int ret = spamgetmsgtype(db, msgid, NULL, 0);
    int curspam = ret == 2;
    int ok = 1;

    if (ret == 0)
	ok = spamaddfile(db, newname, spam);
    else if ((curspam && !spam) || (!curspam && spam))
	ok = spamupdatefile(db, newname, spam);

    return ok;
}

static int spamchecktype;

struct interest
{
    char *words[MAX_WORDS];
    int numwords;
    int good;
    int bad;
    int goodfilecnt;
    int badfilecnt;
    float prob;
};

typedef struct interest Interest;

#define MAXINTEREST	30

static Interest interest[MAXINTEREST];
static int numinterest;
static int pmaxinterest = MAXINTEREST;

static void
addtointerest(char *words[], int num, int good, int bad, int goodfilecnt,
	int badfilecnt, float prob)
{
    int i;
    int j;
    float off;
    int worsti = -1;
    float worstoff = 0.0;

    /* make sure we only enter it once */
    for (i = 0; i < numinterest; i++)
    {
	if (interest[i].numwords == num)
	{
	    for (j = 0; j < num; j++)
		if (strcmp(words[j], interest[i].words[j]) != 0)
		    break;

	    if (j == num)
		return;
	}
    }

    if (numinterest < pmaxinterest)
    {
	i = numinterest++;

	for (j = 0; j < num; j++)
	    interest[i].words[j] = strdup(words[j]);

	interest[i].numwords = num;
	interest[i].good = good;
	interest[i].bad = bad;
	interest[i].goodfilecnt = goodfilecnt;
	interest[i].badfilecnt = badfilecnt;
	interest[i].prob = prob;
//	printf("adding %s %s, %d %d %d %d %f\n", words[0], words[1], good, bad,
//		goodfilecnt, badfilecnt, prob);
	return;
    }

    if (prob >= 0.5)
	off = 1.0 - prob;
    else
	off = prob;

    for (i = 0; i < pmaxinterest; i++)
    {
	float eoff;

	if (interest[i].prob >= 0.5)
	    eoff = 1.0 - interest[i].prob;
	else
	    eoff = interest[i].prob;

	if (off < eoff)
	{
	    if (worsti < 0 || worstoff < eoff)
	    {
		    worstoff = eoff;
		    worsti = i;
	    }
	}
    }

    if (worsti >= 0)
    {
	i = worsti;
//	printf("replacing %s %s, %d %d %d %d %f\n", interest[i].words[0],
//		interest[i].words[1], interest[i].good, interest[i].bad,
//		interest[i].goodfilecnt, interest[i].badfilecnt,
//		interest[i].prob);
//	printf("\tby %s %s, %d %d %d %d %f\n", words[0], words[1], good, bad,
//		goodfilecnt, badfilecnt, prob);
	for (j = 0; j < num; j++)
	{
	    free(interest[i].words[j]);
	    interest[i].words[j] = strdup(words[j]);
	}

	interest[i].numwords = num;
	interest[i].good = good;
	interest[i].bad = bad;
	interest[i].goodfilecnt = goodfilecnt;
	interest[i].badfilecnt = badfilecnt;
	interest[i].prob = prob;
    }
}

static void
clearinterest()
{
    int i;
    int j;

    for (i = 0; i < numinterest; i++)
    {

//	printf("clearing %s %s, %d %d %d %d %f\n", interest[i].words[0],
//		interest[i].words[1], interest[i].good, interest[i].bad,
//		interest[i].goodfilecnt, interest[i].badfilecnt,
//		interest[i].prob);

	for (j = 0; j < interest[i].numwords; j++)
	{
	    free(interest[i].words[j]);
	    interest[i].words[j] = NULL;
	}

	interest[i].numwords = 0;
	interest[i].good = 0;
	interest[i].bad = 0;
	interest[i].goodfilecnt = 0;
	interest[i].badfilecnt = 0;
	interest[i].prob = 0.0;
    }

    numinterest = 0;
}

// From Paul Graham
static float
grahambayes(int verbose)
{
    int i;
    float s1 = 1.0;
    float s2 = 1.0;

    for (i = 0; i < numinterest; i++)
    {
	Interest *x = &interest[i];

	if (verbose)
	{
	    int j;
	    int l = 0;

	    for (j = 0; j < x->numwords; j++)
	    {
		printf("%s ", x->words[j]);
		l += strlen(x->words[j]) + 1;
	    }

	    if (l > 32)
		l = 32;

	    l = 32 - l;
	    printf("%*s%7d%7d%6d%6d %5.2f %7.4f %7.4f\n", l, "", x->good,
		    x->bad, x->goodfilecnt, x->badfilecnt, x->prob, s1, s2);
	}

	s1 *= x->prob;
	s2 *= 1.0 - x->prob;
    }

    return s1 / (s1 + s2);
}

// From Gary Robinson
static float
robinsonbayes(int verbose)
{
    int i;
    double s1 = 1.0;
    double s2 = 1.0;
    double p;
    double q;
    double s;

    for (i = 0; i < numinterest; i++)
    {
	Interest *x = &interest[i];

	if (verbose > 0)
	{
	    int j;

	    for (j = 0; j < x->numwords; j++)
		printf("%s ", x->words[j]);

	    printf("%d %d %d %d %7.4f\n", x->good, x->bad, x->goodfilecnt,
		    x->badfilecnt, x->prob);
	}

	s1 *= x->prob;
	s2 *= 1.0 - x->prob;

	if (verbose > 1)
	    printf("s1 %f, s2 %f\n", s1, s2);
    }

//    p = 1.0 - pow(s2, 1.0 / numinterest);		// spamminess
    p = 1.0 - exp((1.0 / numinterest) * log(s2));	// spamminess
//    q = 1.0 - pow(s1, 1.0 / numinterest);		// non-spamminess
    q = 1.0 - exp((1.0 / numinterest) * log(s1));	// non-spamminess
    s = (p - q) / (p + q);				// combined indicator

    if (verbose > 1)
	printf("s1 %f, s2 %f, p %f, q %f, s %f\n", s1, s2, p, q, s);

    return (1.0 + s) / 2.0;				// range 0..1
}

static float
bayes(int verbose)
{
    switch (spamchecktype)
    {
    case SPAM_LOOKUP_GRAHAM:
	return grahambayes(verbose);
    case SPAM_LOOKUP_ROBINSON:
	return robinsonbayes(verbose);
    }

    return grahambayes(verbose);
}

static int pverbose;
static int pcheckmask;

static void
checkwordseq(char *words[], int num, int first)
{
    int good;
    int bad;
    int goodfilecnt;
    int badfilecnt;
    float prob;
    static int sct = -1;
    static float missprob = 0.0;

    if (!first)
	return;

    if (sct != spamchecktype)
    {
	sct = spamchecktype;
	missprob = calcprob(spamchecktype, 0, 0, 0, 0, 0, 0);
    }

    if ((pcheckmask & 0x01) && num == 1)
    {
	char *word = words[0];

	if (spamlookupword(pdb, word, spamchecktype, &good, &bad, &goodfilecnt,
		&badfilecnt, &prob) == 1)
	{
	    if (pverbose == 2)
		printf("@1 %s %d %d %d %d %7.4f\n", word, good, bad, goodfilecnt, badfilecnt, prob);

	    addtointerest(words, num, good, bad, goodfilecnt, badfilecnt, prob);
	}
	else
	{
	    if (pverbose == 2)
		printf("@1x %s %d %d %d %d %7.4f\n", word, 0, 0, 0, 0, missprob);
	}
    }

    if ((pcheckmask & 0x02) && num == 2)
    {
	char *word1 = words[0];
	char *word2 = words[1];

	if (spamlookuppair(pdb, word1, word2, spamchecktype, &good, &bad, &goodfilecnt,
		&badfilecnt, &prob) == 1)
	{
	    if (pverbose == 2)
		printf("@2 %s %s %d %d %d %d %7.4f\n", word1, word2, good, bad, goodfilecnt, badfilecnt, prob);

	    addtointerest(words, num, good, bad, goodfilecnt, badfilecnt, prob);
	}
	else
	{
	    if (pverbose == 2)
		printf("@2x %s %s %d %d %d %d %7.4f\n", word1, word2, 0, 0, 0, 0, missprob);
	}
    }

    if ((pcheckmask & 0x04) && num == 3)
    {
	char *word1 = words[0];
	char *word2 = words[1];
	char *word3 = words[2];
	float prob2 = missprob;
	float s1;
	prob = missprob;

	if (spamlookuppair(pdb, word1, word2, spamchecktype, NULL, NULL, NULL, NULL, &prob) == 1 &&
		spamlookuppair(pdb, word2, word3, spamchecktype, NULL, NULL, NULL, NULL, &prob2) == 1)
	{
	    s1 = prob * prob2;
	    s1 = s1 / (s1 + (1.0 - prob) * (1.0 - prob2));

	    if (pverbose == 2)
		printf("@3 %s %s %s %7.4f %7.4f %7.4f\n", word1, word2, word3, prob, prob2, s1);
	    addtointerest(words, num, 0, 0, 0, 0, s1);
	}
	else
	{
	    if (pverbose == 2)
		printf("@3x %s %s %s, %7.4f %7.4f\n", word1, word2, word3, prob, prob2);
	}
    }
}

float
spamthreshhold(int type)
{
    switch (type)
    {
    case SPAM_LOOKUP_GRAHAM:
	return 0.9;
    case SPAM_LOOKUP_ROBINSON:
	return 0.7;
    }

    return 0.5;
}

int
interestcmp(const void *p1, const void *p2)
{
    const Interest *i1 = (const Interest *)p1;
    const Interest *i2 = (const Interest *)p2;

    if (i1->prob < i2->prob)
	return -1;

    if (i1->prob > i2->prob)
	return 1;

    return 0;
}

int
spamcheckfile(SPAMDB *db, FILE *fp, int type, int maxinterest, int verbose,
	float *prob)
{
    int ok;
    float p;
    int max = 2;

    clearinterest();
    spamchecktype = type & SPAM_LOOKUP_CALCMASK;
    pdb = db;
    pverbose = verbose;

    if (maxinterest > MAXINTEREST)
	maxinterest = MAXINTEREST;

    pmaxinterest = maxinterest;

    switch (type & SPAM_LOOKUP_SETMASK)
    {
    case SPAM_LOOKUP_WORDS:
	pcheckmask = 0x01;
	break;
    case SPAM_LOOKUP_WORDPAIRS:
	pcheckmask = 0x02;
	break;
    default:
	pcheckmask = 0x03;
	max = 2;
	break;
    }

    ok = spamparsefile(fp, type & SPAM_LOOKUP_MIME, type & SPAM_CRACK_HTML,
	    checkwordseq, max);

    if (!ok)
	return -1;

    /* sort interest list */
    qsort(interest, numinterest, sizeof (Interest), interestcmp);

    p = bayes(verbose);

    if (prob)
	*prob = p;

    return p > spamthreshhold(spamchecktype);
}

int
spamcheckfilename(SPAMDB *db, char *filename, int type, int maxinterest, 
	int verbose, float *prob)
{
    FILE *fp;
    int ok;

    fp = fopen(filename, "r");

    if (fp == NULL)
	return -1;

    ok = spamcheckfile(db, fp, type, maxinterest, verbose, prob);

    fclose(fp);
    return ok;
}

#if 0
/* File Word count records  */
/* Key Record
 *	00	Type
 *	01	filename
 *	n	NUL
 *
 * Data Record
 *	00	Word ID	#1
 *	a	Count	#1
 *	b	Word ID	#2
 *	c	Count	#2
 *	...
 *	y	Word ID	#n
 *	z	Count	#n
 */

/* Hash Record */
/* Key
 *	00	Type
 *	01	Hash Value % 4210751.
 * Data
 *	00	Word ID #1
 *	a	Word ID #2
 *	...
 *	z	Word ID #n
 */
/* Word Count Record */
/* Key
 *	00	Type
 *	01	Word ID
 * Data
 *	00	Word Str
 *	a	NUL
 *	b	good count
 *	c	bad count
 *	d	good file count
 *	e	bad file count
 */

static int
wordid(char *word)
{
    unsigned int hash = strhash(word);
    Wordident ent;
    Wordident *entp;
    uByte buf[5];
    uByte *bp;
    uInt len;
    DBT k;
    DBT d;

    if (wordidtable)
    {
	ent.word = word;

	entp = (Wordident *)hashtablelookup(wordidtable, &ent, hash);

	if (entp)
	    return entp->id;	
    }

    buf[0] = KEYTYPE_HASH;
    bp = &buf[1];
    len = 4;
    putint(&bp, &len, hash % 4210751);
    k.data = buf;
    k.size = 5 - len;
    d = xdb_fetch(db->d, k);

    if (d.data == NULL)
	return 0;

    /* search through ID's in data record and compare */
}

int
addword(char *word)
{
    int id = wordid(word);

    if (id)
	return id;

    id = spamgetnextid();
    /* add to hashtable */
    /* insert word count record */
    /* update hash record */
}

int
removeword(char *word)
{
    int id = wordid(word);

    if (!id)
	return 0;

    /* remove from hashtable */
    /* remove word count record */
    /* update hash record */
}

int
spamreleaseid(int id)
{
    int nextid;

    if (id + 1 == nextid)
    {
	/* update max id record */
	return;
    }

    /* update free id record */
}

/* Free ID Record */
/* Key
 *	00 Type
 *	01 ID / 256
 * Data
 *	00 Range Low ID #1
 *	a Range High ID #1
 *	b Range Low ID #2
 *	c Range High ID #2
 *	...
 *	y Range High ID #n
 *	z Range High ID #n
 */

int
mknewfreeidrec(int id)
{
    keybuf[0] = KEYTYPE_FREEID;
    putint(&kbp, &klen, id >> 8);

    putint(&dbp, &dlen, id);
    putint(&dbp, &dlen, id);
    /* insert */
}

int
addfreeidrec(int level, int id)
{
    /* get the record */
    int numranges;

    for (i = 0; i < numranges; i++)
    {
	if (id + 1 == range[i].low)
	{
	    range[i].low = id;
	    break;
	}

	if (range[i].high + 1 == id)
	{
	    range[i].high = id;
	    break;
	}

	if (id < range[i].low)
	{
	    for (j = numranges - 1; j >= i; j--)
		range[j + 1] = range[j];

	    range[i].low = id;
	    range[i].high = id;
	    break;
	}
    }

    if (i == numranges)
    {
	range[i].low = id;
	range[i].high = id;
    }
    /* update the record */
}

int
removefreeidrec(int id)
{
    /* get the record */
    int numranges;

    for (i = 0; i < numranges; i++)
    {
	if (id == range[i].low)
	{
	    range[i].low = id + 1;
	    break;
	}

	if (range[i].high == id)
	{
	    range[i].high = id - 1;
	    break;
	}

	if (id < range[i].low)
	    return 0;

	if (id < range[i].high)
	{
	    /* split into two ranges */
	    for (j = numranges - 1; j >= i; j--)
		range[j + 1] = range[j];

	    range[i].high = id - 1;
	    range[i + 1].low = id + 1;
	    numranges++;
	    break;
	}
    }

    if (i < numranges && range[i].low > range[i].high)
    {
	/* remove i from list */
	for (j = i + 1; j < numranges; j++)
	    range[j - 1] = range[j];

	numranges--;
    }

    if (numranges == 0)
	/* remove record */
    else
	/* update record */
}

/* 16 Million Level 0 records */
/* 64 K level 1 records */
/* 256 Level 2 records */
/* 1 level 3 records */
#endif /*0*/
