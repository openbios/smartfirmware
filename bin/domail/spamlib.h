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


#ifndef __SPAMLIB_H__
#define __SPAMLIB_H__

#include <stdio.h>
#include <db.h>

/* return values from most routines */
#define OK	1
#define NOT_OK	0

/* hash table functions */
typedef int (*Hashtablecmpfunc)(void *p1, void *p2);
typedef void (*Hashtabledestfunc)(void *p);
struct hashtable;
typedef struct hashtable Hashtable;

Hashtable *hashtablecreate(int entrysize, Hashtablecmpfunc cmp,
	Hashtabledestfunc dest);
void *hashtablelookup(Hashtable *ht, void *key, unsigned int hash);
void *hashtableadd(Hashtable *ht, void *key, unsigned int hash);
void hashtableclear(Hashtable *ht);
void hashtablefree(Hashtable *ht);

/* XDB functions */
#define DB_REPLACE	0
#define DB_INSERT	1

DB *xdb_open_btree(char *filename, int flags, int mode);
DB *xdb_open_hash(char *filename, int flags, int mode);
void xdb_close(DB *db);
DBT xdb_fetch(DB *db, DBT key);
DBT xdb_firstkey(DB *db);
DBT xdb_nextkey(DB *db);
int xdb_delete(DB *db, DBT key);
int xdb_store(DB *db, DBT key, DBT data, int flags);

/* Spam DB functions */
struct spamdb
{
    DB *d;
    int dbtype;
    Hashtable *wordidtable;
    char **idtable;
    int idtablelen;
};

typedef struct spamdb SPAMDB;

/* returned by spamgetmsgtype() */
#define MSGTYPE_UNKNOWN		0
#define MSGTYPE_NOT_SPAM	1
#define MSGTYPE_SPAM		2

int g_spamliberrs;
char *g_spamlibprog;
SPAMDB *spaminitdb(char *name, int key);
SPAMDB *spamopendb(char *name);
void spamclosedb(SPAMDB *db);
int spamrenamedb(char *oldname, char *newname, int key);
int spamgetdbtype(SPAMDB *db);
int spamgetfiles(SPAMDB *db, int *goodcnt, int *badcnt);
void spamadjustfiles(SPAMDB *db, int goodfilesinc, int badfilesinc);
void spamadjust(SPAMDB *db, char *word1, char *word2, int goodinc, int badinc,
	int goodfileinc, int badfileinc);
int spamgetmsgtype(SPAMDB *db, char *msgid, char *filename, int len);
void spamchangemsgtype(SPAMDB *db, char *msgid, int spam, char *filename);
void spamremovemsgtype(SPAMDB *db, char *msgid);
int spamlookupword(SPAMDB *db, char *word, int type, int *goodcnt, int *badcnt,
	int *goodfilecnt, int *badfilecnt, float *prob);
int spamlookuppair(SPAMDB *db, char *word1, char *word2, int type, int *goodcnt,
	int *badcnt, int *goodfilecnt, int *badfilecnt, float *prob);
int spamfirstfilename(SPAMDB *db, char *filename, int *spam);
int spamnextfilename(SPAMDB *db, char *filename, int *spam);
int spamfirstword(SPAMDB *db, int type, char *word, int *good, int *bad, 
	int *goodfilecnt, int *badfilecnt, float *prob);
int spamnextword(SPAMDB *db, int type, char *word, int *good, int *bad,
	int *goodfilecnt, int *badfilecnt, float *prob);
int spamfirstpair(SPAMDB *db, int type, char *word1, char *word2, int *good, int *bad, 
	int *goodfilecnt, int *badfilecnt, float *prob);
int spamnextpair(SPAMDB *db, int type, char *word1, char *word2, int *good, int *bad,
	int *goodfilecnt, int *badfilecnt, float *prob);

typedef void (*spamwordfunctype)(char *words[], int num, int first);

int spamparsefile(FILE *fp, int crackmime, int crackhtml,
	spamwordfunctype wordseqfunc, int max);
int spamparsefilename(char *filename, int crackmime, int crackhtml,
	spamwordfunctype wordseqfunc, int max);
int spamaddfile(SPAMDB *db, char *filename, int spam);
int spamupdatefile(SPAMDB *db, char *filename, int spam);
int spamremovefile(SPAMDB *db, char *filename, int spam);
int spamrenamefile(SPAMDB *db, char *oldname, char *newname, int spam);
int spamcheckfile(SPAMDB *db, FILE *fp, int type, int maxinterest, int verbose,
	float *prob);
int spamcheckfilename(SPAMDB *db, char *filename, int type, int maxinterest,
	int verbose, float *prob);
char *spamgetmessageid(char *filename);

#define SPAM_INITDB_KEY		0x031417DB
#define SPAM_INITDB_KEY_HASH	0x931417DB

#define SPAM_LOOKUP_GRAHAM	0x0001
#define SPAM_LOOKUP_ROBINSON	0x0002
#define SPAM_LOOKUP_CALCMASK	0x000F

#define SPAM_LOOKUP_WORDS	0x0010
#define SPAM_LOOKUP_WORDPAIRS	0x0020
#define SPAM_LOOKUP_SETMASK	0x00F0

#define SPAM_LOOKUP_MIME	0x0100
#define SPAM_CRACK_HTML		0x0200

#endif /* __SPAMLIB_H__ */
