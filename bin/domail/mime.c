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
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "spamlib.h"
#include "mimelib.h"

typedef int (*mime_decodefunc)(MIME *m);
typedef void (*mime_cfreefunc)(void *m);

/* non-mime decoding functions */
int mime_basicbody(MIME *m);

/* header decoding functions */
int mime_mimeversion_decode(MIME *m);
int mime_contenttype_decode(MIME *m);
int mime_contenttransferencoding_decode(MIME *m);

/* message part decoding functions */
int mime_basicheader_decode(MIME *m);
int mime_mimeheader_decode(MIME *m);
int mime_preamble_decode(MIME *m);
int mime_boundary_decode(MIME *m);
int mime_contentheader_decode(MIME *m);
int mime_content_decode(MIME *m);

/* encoded content decoding functions */
int mime_8bit_decode(MIME *m);
int mime_base64_decode(MIME *m);
int mime_quotedprintable_decode(MIME *m);

/* content specific decoding functions */
void mime_htmlcontent_free(void *cdata);
int mime_htmlcontent_decode(MIME *m);

int mime_igetch(MIME *m);

struct html_content
{
    int type;
    short buf[2];
    int bufsz;
    int tagbuflen;
    int tagbufsz;
    int tagbufpos;
    short *tagbuf;
    int numattribs;
    int attribslen;
    char *tag;
    char **attribnames;
    char **attribvalues;
};

typedef struct html_content Html_content;

#define MIME_CONTENT_HTML	1

struct mime_content_param
{
    struct mime_content_param *next;
    char *header;
    char *name;
    char *value;
};

typedef struct mime_content_param Mime_content_param;

struct mime_content
{
    struct mime_content *parent;
    int flags;
    mime_decodefunc transdecodefunc;		/* Transfer-Type decoder */
    mime_decodefunc decodefunc;			/* Contenet-Type decode */
    int boc;
    char *type;
    char *subtype;
    char *boundary;
    int boundarylen;
    char *encoding;

    Mime_content_param *params;
    void *cdata;		/* Content specific parser data */
    mime_cfreefunc cfree;

#if 0
    /* From Content-Type */
    char *type;
    char *subtype;
    char *boundary;
    int boundarylen;

    /* From Content-Transfer-Encoding */
    char *encoding;

    /* From Content-ID */
    char *id;

    /* From Content-Description */
    char *desc;

    /* From Content-Disposition RFC1806 */
    char *filename;
    char *createdate;
    char *moddate;
    char *accessdate;
    char *size;

    /* From Content-Base */
    char *base;
#endif
};

typedef struct mime_content Mime_content;

#define MIME_CONTENT_MULTIPART	0x0001
#define MIME_CONTENT_PREAMBLE	0x0002
#define MIME_CONTENT_EPILOG	0x0004
#define MIME_CONTENT_INLINE	0x0008
#define MIME_CONTENT_ATTACHMENT	0x0010

struct mime
{
    FILE *fp;			/* File pointer for source data */
    int flags;			/* enables returning specific sections */
    short *ungetbuf;		/* used to implement mime_ungetch */
    int ungetlen;		/* must be short since some chars are > 255 */
    int ungetsz;
    mime_decodefunc decodefunc;
    mime_decodefunc headerdecodefunc;
    char lbuf[128];	/* line buffer */
    int lsz;
    int check;		/* boundary check index into line buffer */
    char *header;	/* copy of current header name */
    char *hbuf;		/* header buffer */
    int hbuflen;
    int hsz;
    Mime_content *content;
    Mime_content *nextcontent;
    Mime_content *boundary;
    unsigned short base64state;
    unsigned char base64bits;
};

#define MIME_EOF			0x1000

static char *
strndup(char *str, int len)
{
    char *s = (char *)malloc(len + 1);

    if (s == NULL)
	return NULL;

    memcpy(s, str, len);
    s[len] = '\0';
    return s;
}

#ifdef TEST
void
debugf(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
//    vfprintf(stderr, fmt, args);
    printf("===== ");
    vprintf(fmt, args);
    va_end(args);
}
#else
inline void debugf(char *fmt, ...) { }
#endif

MIME *
mime_open(FILE *fp, int flags)
{
    MIME *m = (MIME *)malloc(sizeof (MIME));

    if (m == NULL)
	return NULL;

    memset(m, '\0', sizeof (MIME));
    m->fp = fp;
    m->flags = flags;

    if (flags == MIME_NOCRACK)
    {
	m->decodefunc = mime_basicbody;
	m->headerdecodefunc = mime_basicbody;
    }
    else
    {
	mime_ungetch(m, MIME_BOH);
//	m->decodefunc = mime_basicheader_decode;
//	m->headerdecodefunc = mime_basicheader_decode;
	m->decodefunc = mime_mimeheader_decode;
	m->headerdecodefunc = mime_mimeheader_decode;
    }

    return m;
}

void
mime_new_content(MIME *m)
{
    Mime_content *c;

    if (m->nextcontent != NULL)
	return;

    c = (Mime_content *)malloc(sizeof (Mime_content));

    if (c == NULL)
	return;

    memset(c, '\0', sizeof (Mime_content));
    c->boc = MIME_BOC;
    m->nextcontent = c;
    c->transdecodefunc = mime_8bit_decode;
}

void
mime_push_content(MIME *m)
{
    Mime_content *c;

    mime_new_content(m);
    c = m->nextcontent;

    if (c == NULL)
	return;

    c->parent = m->content;
    m->content = c;
    m->nextcontent = NULL;
    m->decodefunc = c->decodefunc;
}

void
mime_pop_content(MIME *m)
{
    Mime_content *c = m->content;

    if (c == NULL)
	return;

    m->content = c->parent;

    if (c->type)
	free(c->type);

    if (c->encoding)
	free(c->encoding);

    if (c->boundary)
	free(c->boundary);

    if (c->cdata && c->cfree)
	(*c->cfree)(c->cdata);
}

void
mime_close(MIME *m)
{
    if (m == (MIME *)0)
	return;

    if (m->ungetbuf)
	free(m->ungetbuf);

    if (m->hbuf)
	free(m->hbuf);

    while (m->content)
	mime_pop_content(m);

    free(m);
}

char *
mime_find_content_param(Mime_content *c, char *header, char *param)
{
    Mime_content_param *p;

    if (c == NULL)
	return NULL;

    if (strcasecmp(header, "Content-Type") == 0)
    {
	if (strcasecmp(param, "type") == 0)
	    return c->type;

	if (strcasecmp(param, "subtype") == 0)
	    return c->subtype;

	if (strcasecmp(param, "boundary") == 0)
	    return c->boundary;
    }
    else if (strcasecmp(header, "Content-Transfer-Encoding") == 0)
    {
	if (strcasecmp(param, "encoding") == 0)
	    return c->encoding;
    }

    p = c->params;

    for (p = c->params; p; p = p->next)
	if (strcasecmp(p->header, header) == 0 &&
		strcasecmp(p->name, param) == 0)
	    return p->value;

    return NULL;
}

char *
mime_get_content_param(MIME *m, char *header, char *param)
{
    return mime_find_content_param(m->content, header, param);
}

void
mime_add_content_param(MIME *m, char *header, char *param, char *value)
{
    Mime_content *c;
    Mime_content_param *p;

    if (m->nextcontent == NULL)
	mime_new_content(m);
    else if (mime_find_content_param(m->nextcontent, header, param) != NULL)
	return;

    c = m->nextcontent;

    if (c == NULL)
	return;

    if (strcasecmp(header, "Content-Type") == 0 &&
	    strcasecmp(param, "boundary") == 0)
    {
	c->boundary = strdup(value);
	c->boundarylen = strlen(value);
	return;
    }

    p = (Mime_content_param *)malloc(sizeof (Mime_content_param));
    p->next = c->params;
    p->header = header;
    p->name = strdup(param);
    p->value = strdup(value);
    c->params = p;
}

int
mime_basicbody(MIME *m)
{
    return fgetc(m->fp);
}

int
mime_check_boundary(MIME *m)
{
    int ch;
    int i;
    int numboundries;
    int boundries;
    Mime_content *c;
    int lsz;

    /* check if we failed to find a boundary but haven't yet eaten up all of */
    /* the line buffer */
    m->boundary = NULL;

    if (m->check < m->lsz)
    {
	ch = m->lbuf[m->check++];

	if (ch == '\n')
	    m->lsz = 0;

	return ch;
    }

    if (m->lsz != 0)
    {
	/* we are just processing a line that is not a boundary. */
	ch = fgetc(m->fp);

	if (ch == EOF)
	    m->flags |= MIME_EOF;

	if (ch == '\n')
	    m->lsz = 0;

	return ch;
    }

    /* we are at the begining of the line, check for a boundary */
    ch = fgetc(m->fp);
    m->lbuf[0] = ch;
    lsz = 1;
    m->lsz = 1;
    m->check = 1;

    if (ch != '-')
    {
	/* we don't have a boundary */
	if (ch == EOF)
	    m->flags |= MIME_EOF;

	if (ch == '\n')
	    m->lsz = 0;

	return ch;
    }

    ch = fgetc(m->fp);

    if (ch == EOF)
	return '-';

    m->lbuf[1] = ch;
    lsz = 2;
    m->lsz = 2;

    if (ch != '-')
    {
	/* we don't have a boundary */
	return '-';
    }

    /* build the boundary list and keep checking characters against it. */
    boundries = 0;
    numboundries = 0;

    for (c = m->content; c; c = c->parent)
    {
	if (c->boundary && c->boundarylen)
	    boundries |= 1 << numboundries;

	numboundries++;
    }

    while (boundries)
    {
	ch = fgetc(m->fp);

	if (ch == EOF || ch == '\n')
	{
	    if (ch == '\n')
		ungetc(ch, m->fp);

	    /* look for a boundary match */
	    for (i = 0, c = m->content; c; c = c->parent, i++)
		if ((boundries & (1 << i)) &&
			(lsz == c->boundarylen + 2 || lsz == c->boundarylen + 4))
		{
		    /* we matched a boundary */
		    m->boundary = c;
		    m->check = 0;

		    if (lsz == c->boundarylen + 4)
			c->flags |= MIME_CONTENT_EPILOG;

		    /* eat the newline */
		    if (ch == '\n')
		    {
			m->lbuf[lsz++] = ch;
			m->lsz = lsz;
			fgetc(m->fp);
		    }

		    return EOF;
		}

	    return '-';
	}

	/* buffer the character */
	m->lbuf[lsz++] = ch;
	m->lsz = lsz;

	/* eliminate unmatched boundries */
	for (i = 0, c = m->content; c; c = c->parent, i++)
	    if (boundries & (1 << i))
	    {
		int match = 0;

		/* lbuf should be match either --boundary or --boundary-- */
		if (lsz <= c->boundarylen + 2)
		{
		    if (m->lbuf[lsz - 1] == c->boundary[lsz - 3])
			match = 1;
		}
		else if (lsz <= c->boundarylen + 4)
		{
		    if (m->lbuf[lsz - 1] == '-')
			match = 1;
		}

		if (!match)
		    boundries &= ~(1 << i);
	    }
    }

    /* we have eliminated the possibility of a boundary seperator */
    return '-';
}

int
mime_boundary_decode(MIME *m)
{
    Mime_content *c = m->content;

//    if (c != m->boundary)
//    {
//	m->content = c->parent;
//	return MIME_EOC;
//    }

//    if (m->check < 0)
//    {
//	m->check++;
//	return MIME_EOC;
//    }

    if (m->check < m->lsz)
	return m->lbuf[m->check++];

    m->lsz = 0;

    if (c->flags & MIME_CONTENT_EPILOG)
    {
	m->decodefunc = mime_content_decode;
	mime_ungetch(m, MIME_BOE);
    }
    else
    {
	m->decodefunc = mime_contentheader_decode;
	m->headerdecodefunc = mime_contentheader_decode;
	mime_ungetch(m, MIME_BOCH);
    }

    return MIME_EOB;
}

int
mime_end_content(MIME *m)
{
    Mime_content *c = m->content;
    Mime_content *p = NULL;
    Mime_content *t;

    /* we are either at EOF or we are at a MIME boundary */

    if (c)
	p = c->parent;

    if (m->boundary != c && m->boundary != p)
	debugf("Unexpected boundary\n");

    if ((m->flags & MIME_EOF) == 0)
    {
	m->decodefunc = mime_boundary_decode;
	mime_ungetch(m, MIME_BOB);
    }

    p = NULL;

    while (c && c != m->boundary)
    {
	/* remove c */
	t = c->parent;

	/* add c to removed list */
	c->parent = p;
	p = c;
	c = t;
    }

    while (p)
    {
	mime_ungetch(m, MIME_EOC);

	if (p->flags & MIME_CONTENT_PREAMBLE)
	    mime_ungetch(m, MIME_EOP);
	else if (p->flags & MIME_CONTENT_EPILOG)
	    mime_ungetch(m, MIME_EOE);

	/* place back on the list */
	t = p->parent;
	p->parent = c;
	c = p;
	p = t;
    }

    while (c && c != m->boundary)
    {
	mime_pop_content(m);
	c = m->content;
    }

    if (c == NULL)
    {
	/* we got an EOF, send out the last queued bytes before returning */
	/* EOF. */
	m->decodefunc = mime_basicbody;
	return mime_igetch(m);
    }

    if ((c->flags & MIME_CONTENT_PREAMBLE))
    {
	c->flags &= ~MIME_CONTENT_PREAMBLE;
	mime_ungetch(m, MIME_EOP);
    }

    if ((c->flags & MIME_CONTENT_EPILOG) == 0)
	mime_new_content(m);

    return mime_igetch(m);
}

int
mime_content_decode(MIME *m)
{
    int ch = (*m->content->transdecodefunc)(m);

    /* return the next character in the body, but also check for a boundary */
    /* and if one is found switch back to normal processing. */
    if (ch != EOF)
	return ch;

    /* we are at the start of a mime boundry, with the boundary line buffered */
    /* in lbuf. */
    return mime_end_content(m);
}

int
mime_preamble_decode(MIME *m)
{
    int ch;
    Mime_content *c = m->content;

    if (c && (c->flags & MIME_CONTENT_PREAMBLE) == 0)
    {
	c->flags |= MIME_CONTENT_PREAMBLE;
	return MIME_BOP;
    }

    ch = mime_check_boundary(m);

    /* return the next character in the body, but also check for a boundary */
    /* and if one is found switch back to normal processing. */
    if (ch != EOF)
	return ch;

    /* we are at the start of a mime boundry, with the boundary line buffered */
    /* in lbuf. */

    return mime_end_content(m);
}

void
mime_begin_header(MIME *m)
{
    if (m->hbuflen < m->lsz)
    {
	/* expand the hbuf to hold the lbuf */
	m->hbuf = (char *)realloc((void *)m->hbuf, m->lsz + 64);

	if (m->hbuf)
	    m->hbuflen = m->lsz + 64;
	else
	{
	    m->hbuflen = 0;
	    m->hsz = 0;
	    return;
	}
    }

    memcpy(m->hbuf, m->lbuf, m->lsz);
    m->hsz = m->lsz;
//    m->lbuf[m->lsz] = '\0';
//    debugf("Hbuf set to \"%s\"\n", m->lbuf);
    m->lsz = 0;
}

int
mime_buffer_header(MIME *m)
{
    int ch = fgetc(m->fp);

//    debugf("mime_buffer_header: hsz %d, last char '%c'\n", m->hsz,
//	    m->hbuf[m->hsz - 1]);

    if (m->hsz && m->hbuf[m->hsz - 1] == '\n')
    {
	/* this is the start of a line */
	if (!isspace(ch) || ch == '\n')
	{
	    ungetc(ch, m->fp);

	    if (m->header)
	    {
		free(m->header);
		m->header = NULL;
	    }

	    return EOF;
	}
    }

    if (m->hsz >= m->hbuflen)
    {
	/* expand the header buffer */
	m->hbuf = (char *)realloc((void *)m->hbuf, m->hbuflen + 64);

	if (m->hbuf)
	    m->hbuflen += 64;
	else
	{
	    m->hsz = 0;
	    m->hbuflen = 0;
	    return ch;
	}
    }

    m->hbuf[m->hsz++] = ch;
    return ch;
}

int
mime_mimeversion_decode(MIME *m)
{
    int i;
    int ch = mime_buffer_header(m);

    if (ch != EOF)
	return ch;

//    m->hbuf[m->hsz] = '\0';
//    debugf("MIME-Version: is \"%s\"\n", m->hbuf);

    /* the header is fully buffered so parse it */
    for (i = 14; i < m->hsz && isspace(m->hbuf[i]); i++)
	;

//    debugf("i is %d, rest \"%s\"\n", i, &m->hbuf[i]);
//    debugf("m->hsz >= i + 3 is %d\n", m->hsz >= i + 3);
//    debugf("strncasecmp(&m->hbuf[i], \"1.0\", 3) == 0 is %d\n",
//	    strncasecmp(&m->hbuf[i], "1.0", 3) == 0);
//    debugf("i + 3 == m->hsz is %d\n", i + 3 == m->hsz);
//    debugf("isspace(m->hbuf[i + 3]) is %d\n", isspace(m->hbuf[i + 3]));

    if (m->hsz >= i + 3 && strncasecmp(&m->hbuf[i], "1.0", 3) == 0 &&
	    (i + 3 == m->hsz || isspace(m->hbuf[i + 3])))
    {
//	debugf("This is a MIME message\n");
	m->headerdecodefunc = mime_mimeheader_decode;
    }

    m->decodefunc = m->headerdecodefunc;
    mime_new_content(m);
    return mime_igetch(m);
}

static int
mime_parse_token(char *buf, int buflen, char **token, int *toklen,
	char **rest, int *restlen)
{
    char *t;
    int tl;

    while (buflen)
    {
	if (!isspace(*buf))
	    break;

	buf++;
	buflen--;
    }

    t = buf;

    while (buflen)
    {
	int ch = *buf;
	int done = 0;

	switch (ch)
	{
	case 0x00: case 0x01: case 0x02: case 0x03:	/* control characters */
	case 0x04: case 0x05: case 0x06: case 0x07:
	case 0x08:                       case 0x0B:
	case 0x0C: case 0x0D: case 0x0E: case 0x0F:
	case 0x10: case 0x11: case 0x12: case 0x13:
	case 0x14: case 0x15: case 0x16: case 0x17:
	case 0x18: case 0x19: case 0x1A: case 0x1B:
	case 0x1C: case 0x1D: case 0x1E: case 0x1F:
	case ' ': case '\t': case '\n':			/* space chars */
	case '(': case ')': case '<': case '>': case '@':	/* specials */
	case ',': case ';': case ':': case '\\': case '\"':
	case '/': case '[': case ']': case '?': case '=':
	    done = 1;
	    break;
	}

	if (done)
	    break;

	buf++;
	buflen--;
    }

    tl = buf - t;

    while (buflen)
    {
	if (!isspace(*buf))
	    break;

	buf++;
	buflen--;
    }

    if (token)
	*token = t;

    if (toklen)
	*toklen = tl;

    if (tl == 0)
	return 0;

    if (rest)
	*rest = buf;

    if (restlen)
	*restlen = buflen;

    return 1;
}

static int
mime_parse_quotedstring(char *buf, int buflen, char **str,
	char **rest, int *restlen)
{
    char *s;

//    buf[buflen] = '\0';
//    debugf("parse quoted-string in \"%s\"\n", buf);

    while (buflen)
    {
	if (!isspace(*buf))
	    break;

	buf++;
	buflen--;
    }

    if (buflen == 0 || *buf != '\"')
	return 0;

    s = buf++;
    buflen--;

    if (str)
	*str = s;


    while (buflen)
    {
	int ch = *buf;

	if (ch == '\"')
	    break;

	if (ch == '\\')
	{
	    if (buflen <= 1)
		return 0;

	    buf++;
	    buflen -= 2;
	    *s++ = *buf++;
	    continue;
	}

	*s++ = *buf++;
	buflen--;
    }

    *s++ = '\0';

    if (buflen < 1)
	return 0;

    buf++;
    buflen--;		/* eat the quote */

    while (buflen)
    {
	if (!isspace(*buf))
	    break;

	buf++;
	buflen--;
    }

    if (rest)
	*rest = buf;

    if (restlen)
	*restlen = buflen;

//    debugf("quoted-string returns \"%s\"\n", *str);
    return 1;
}

int
mime_contenttype_decode(MIME *m)
{
    int ch = mime_buffer_header(m);
    char *buf;
    int buflen;
    char *type;
    int typelen;
    Mime_content *c;
    mime_decodefunc func;

    if (ch != EOF)
	return ch;

    /* the header is fully buffered so parse it */
    m->decodefunc = m->headerdecodefunc;
//    m->hbuf[m->hsz] = '\0';
//    debugf("Content-Type: is \"%s\"\n", m->hbuf);

    mime_new_content(m);
    c = m->nextcontent;

    /* we need to parse out the mime type and boundary if it is multipart */
    if (!mime_parse_token(m->hbuf + 14, m->hsz - 14, &type, &typelen,
	    &buf, &buflen))
    {
	/* no type */
//	debugf("Content-Type: no type\n");
	buf = m->hbuf + 14;
	buflen = m->hsz - 14;
    }

    c->type = strndup(type, typelen);

    if (buflen == 0 || buf[0] != '/')
    {
	/* no slash */
//	debugf("Content-Type: no slash, buf %p,  *buf '%c', buflen %d\n", buf, *buf, buflen);
    }

    if (!mime_parse_token(buf + 1, buflen - 1, &type, &typelen, &buf, &buflen))
    {
	/* no sub-type */
//	debugf("Content-Type: no subtype\n");
    }

    c->subtype = strndup(type, typelen);

    while (buflen > 0 && buf[0] == ';')
    {
	char *attrib;
	int attriblen;
	char *value;
	int valuelen;

//	debugf("Content-Type: params left %s\n", buf);

	/* parse the parameters */
	if (!mime_parse_token(buf + 1, buflen - 1, &attrib, &attriblen,
		&buf, &buflen))
	{
	    /* no attribute */
//	    debugf("Content-Type: no attribute\n");

	    /* skip semicolon */
	    buflen--;
	    buf++;

	    while (buflen)
	    {
		if (!isspace(*buf))
		    break;

		buf++;
		buflen--;
	    }

	    break;
	}

	if (buflen == 0 || buf[0] != '=')
	{
	    /* no equals */
//	    debugf("Content-Type: no equals\n");
	}

	attrib[attriblen] = '\0';
//	debugf("Content-Type: value is in %s\n", buf + 1);

	if (!mime_parse_token(buf + 1, buflen - 1, &value, &valuelen,
		&buf, &buflen))
	{
//	    debugf("Content-Type: no value token, left %s\n", buf);

	    if (!mime_parse_quotedstring(buf + 1, buflen - 1, &value, 
		    &buf, &buflen))
	    {
		/* no value */
//		debugf("Content-Type: no value string, left %s\n", buf);
//		debugf("Content-Type: no value, buf %p, *buf '%c', buflen %d\n", buf, *buf, buflen);
	    }
	    else
	    {
		if (strcasecmp(attrib, "boundary") == 0)
		    c->boundary = strdup(value);
		else
		    mime_add_content_param(m, "Content-Type", attrib, value);
	    }
	}
	else
	{
	    if (strcasecmp(attrib, "boundary") == 0)
		c->boundary = strndup(value, valuelen);
	    else
		mime_add_content_param(m, "Content-Type", attrib, value);
	}
    }

    if (buflen != 0)
    {
	/* junk after parameters */
//	debugf("Content-Type: junk after parameters \"%s\" %d\n", buf, buflen);
    }
    else
    {
	/* parsed everything */
//	debugf("Content-Type: is %s/%s ;boundary=\"%s\"\n", c->type ? c->type : "<null>", c->subtype ? c->subtype : "<null>", c->boundary ? c->boundary : "<none>");

	if (c->type && strcasecmp(c->type, "multipart") == 0)
	{
	    c->flags = MIME_CONTENT_MULTIPART;
	    func = mime_preamble_decode;
	}
	else if ((m->flags & MIME_CRACKHTML) && c->type && c->subtype &&
		strcasecmp(c->type, "text") == 0 &&
		strcasecmp(c->subtype, "html") == 0)
	    func = mime_htmlcontent_decode;
	else
	    func = mime_content_decode;

	c->decodefunc = func;
    }

    if (c->boundary)
	c->boundarylen = strlen(c->boundary);

    return mime_igetch(m);
}

int
mime_contenttransferencoding_decode(MIME *m)
{
    int ch = mime_buffer_header(m);
    Mime_content *c;
    char *type;
    int typelen;
    char *buf;
    int buflen;

    if (ch != EOF)
	return ch;

    /* the header is fully buffered so parse it */
    mime_new_content(m);
    c = m->nextcontent;

    /* we need to parse out the content transfer encoding */
    if (!mime_parse_token(m->hbuf + 27, m->hsz - 27, &type, &typelen,
	    &buf, &buflen))
    {
	/* no type */
//	debugf("Content-Transfer-Encoding: no type\n");
    }

    c->encoding = strndup(type, typelen);

    if (c->type && strcmp(c->type, "multipart") == 0)
    {
	if (strcmp(c->encoding, "7bit") == 0 ||
		strcmp(c->encoding, "8bit") == 0 ||
		strcmp(c->encoding, "binary") == 0)
	    ; /* ok */
	else
	    ; /* err */
    }
    else
    {
	if (strcmp(c->encoding, "base64") == 0)
	{
	    c->transdecodefunc = mime_base64_decode;
	    m->base64state = 0;
	    m->base64bits = 0;
	}
	else if (strcmp(c->encoding, "quoted-printable") == 0)
	    c->transdecodefunc = mime_quotedprintable_decode;
    }

//    m->hbuf[m->hsz] = '\0';
//    debugf("Content-Transfer-Encoding: is \"%s\"\n", m->hbuf);
    m->decodefunc = m->headerdecodefunc;
    return mime_igetch(m);
}

struct header_info
{
    char *name;
    mime_decodefunc decodefunc;
};

typedef struct header_info Header_info;

Header_info basicheaders[] =
{
    { "MIME-Version", mime_mimeversion_decode },
    { NULL, NULL },
};

Header_info mimeheaders[] =
{
//    { "Content-Base", mime_contentbase_decode },
//    { "Content-Classes", mime_contentclasses_decode },
//    { "Content-Description", mime_contentdescription_decode },
//    { "Content-Disposition", mime_contentdisposition_decode },
//    { "Content-ID", mime_contentid_decode },
//    { "Content-Length", mime_contentlength_decode },

    { "Content-Type", mime_contenttype_decode },
    { "Content-Transfer-Encoding", mime_contenttransferencoding_decode },
    { NULL, NULL },
};

void
mime_check_headers(MIME *m, Header_info *headers)
{
    int i;

//    m->lbuf[m->lsz] = '\0';
//    debugf("Header is \"%s\"\n", m->lbuf);

    for (i = 0; headers[i].name; i++)
    {
	char *n = headers[i].name;
	int l = strlen(n);

	if (m->lsz > l && strncasecmp(m->lbuf, n, l) == 0 && m->lbuf[l] == ':')
	{
	    /* we have a match */
//	    debugf("Header matched %s:\n", n);
	    mime_begin_header(m);
	    m->decodefunc = headers[i].decodefunc;
	    break;
	}
    }
}

struct header_desc
{
    mime_decodefunc bodydecode;
    int eoh;
    Header_info *headers;
};

typedef struct header_desc Header_desc;

int
mime_header_decode(MIME *m, Header_desc *desc)
{
    int ch;
    int blank = 0;
    int i;
    Mime_content *c;

    ch = fgetc(m->fp);

    if (m->lsz == 0 && m->header && !isspace(ch))
    {
	free(m->header);
	m->header = NULL;
    }

    if (m->lsz < sizeof m->lbuf)
	m->lbuf[m->lsz++] = ch; 

    if (ch == ':' && !isspace(m->lbuf[0]))
    {
	/* the header name is sitting in lbuf, so parse it out and save it */
	for (i = 0; i < m->lsz; i++)
	    if (isspace(m->lbuf[i]) || m->lbuf[i] == ':')
		break;

	if (i && i < m->lsz && m->lbuf[i] == ':')
	    m->header = strndup(m->lbuf, i);
	else
	    m->header = NULL;
    }

    if (ch != '\n')
	return ch;

    for (i = 0; i < m->lsz; i++)
	if (!isspace(m->lbuf[i]))
	    break;

    if (i == m->lsz)
	blank = 1;		/* we have a blank line */

    if (blank)
    {
	mime_push_content(m);
	c = m->content;

	if (m->header)
	{
	    free(m->header);
	    m->header = NULL;
	}

	if (m->decodefunc == NULL)
	    m->decodefunc = desc->bodydecode;

	if (c && c->boc)
	    mime_ungetch(m, c->boc);

	mime_ungetch(m, desc->eoh);
	m->lsz = 0;
	return ch;
    }

    mime_check_headers(m, desc->headers);
    m->lsz = 0;
    return ch;
}

Header_desc basicheaderdesc =
{
    mime_basicbody,
    MIME_EOH,
    basicheaders,
};

int
mime_basicheader_decode(MIME *m)
{
    return mime_header_decode(m, &basicheaderdesc);
}

Header_desc mimeheaderdesc =
{
    mime_content_decode,
    MIME_EOH,
    mimeheaders,
};

int
mime_mimeheader_decode(MIME *m)
{
    return mime_header_decode(m, &mimeheaderdesc);
}

Header_desc contentheaderdesc =
{
    mime_content_decode,
    MIME_EOCH,
    mimeheaders,
};

int
mime_contentheader_decode(MIME *m)
{
//    debugf("In contentheader_decode\n");
    return mime_header_decode(m, &contentheaderdesc);
}

int
mime_8bit_decode(MIME *m)
{
    return mime_check_boundary(m);
}

int
mime_base64_decode(MIME *m)
{
    int ch;
    int state = 0;
    int bits = m->base64bits;

    if (bits)
	state = m->base64state;

/*
    A B C D
    aaaaaa bbbbbb cccccc dddddd
    XXXXXX XXYYYY YYYYZZ ZZZZZZ
*/

    while (1)
    {
	int dig;

	ch = mime_check_boundary(m);

	if (ch == EOF)
	    return EOF;

//	debugf("char = %d\n", ch);

	if (ch >= 'A' && ch <= 'Z')
	    dig = ch - 'A';
	else if (ch >= 'a' && ch <= 'z')
	    dig = ch - 'a' + 26;
	else if (ch >= '0' && ch <= 'z')
	    dig = ch - '0' + 52;
	else if (ch == '+')
	    dig = 62;
	else if (ch == '/')
	    dig = 63;
	else if (isspace(ch) || ch == '=')
	    continue;
	else
	{
	    debugf("Unrecognized character %d\n", ch);
	    continue;
	}

//	debugf("dig = %d\n", dig);

	state = (state << 6) | dig;
	bits += 6;
//	debugf("bits = %d\n", bits);

	if (bits < 8)
	    continue;

	if (bits == 8)
	{
	    m->base64bits = 0;
	    return state & 0xFF;
	}

	if (bits > 8)
	{
	    dig = state >> (bits - 8);
	    bits -= 8;
	    m->base64state = state;
	    m->base64bits = bits;
	    return dig & 0xFF;
	}
    }
}

int
mime_quotedprintable_decode(MIME *m)
{
    int ch = mime_check_boundary(m);
    int ch2;
    int dig;

    if (ch == EOF)
	return EOF;

    if (ch != '=')
	return ch;

    ch = mime_check_boundary(m);

    if (ch == EOF)
	return EOF;

    if (ch == '\n')
	return mime_quotedprintable_decode(m);

    ch2 = mime_check_boundary(m);

    if (ch2 == EOF)
	return EOF;

    if (ch >= '0' && ch <= '9')
	dig = ch - '0';
    else if (ch >= 'A' && ch <= 'F')
	dig = ch - 'A' + 10;
    else if (ch >= 'a' && ch <= 'f')
	dig = ch - 'a' + 10;
    else
	return mime_quotedprintable_decode(m);

    dig <<= 4;

    if (ch2 >= '0' && ch2 <= '9')
	dig |= ch2 - '0';
    else if (ch2 >= 'A' && ch2 <= 'F')
	dig |= ch2 - 'A' + 10;
    else if (ch2 >= 'a' && ch2 <= 'f')
	dig |= ch2 - 'a' + 10;
    else
	return mime_quotedprintable_decode(m);

    return dig;
}

void
mime_htmlcontent_free(void *cdata)
{
    Html_content *h = (Html_content *)cdata;

    if (h == NULL)
	return;

    if (h->tagbuf)
	free(h->tagbuf);

    free(h);
}

void
mime_html_addtagbuf(Html_content *h, int ch)
{
    if (h == NULL)
	return;

    if (h->tagbufpos == h->tagbufsz)
    {
	/* reset buffer */
	h->tagbufpos = 0;
	h->tagbufsz = 0;
    }

    if (h->tagbufsz >= h->tagbuflen)
    {
	/* expand buffer */
	short *b = (short *)malloc(sizeof (short) * (h->tagbuflen + 64));

	if (b == NULL)
	    return;

	memcpy(b, h->tagbuf, sizeof (short) * h->tagbuflen);
	free(h->tagbuf);
	h->tagbuf = b;
	h->tagbuflen += 64;
    }

    h->tagbuf[h->tagbufsz++] = ch;
}

void
html_ungetch(MIME *m, Html_content *h, int ch)
{
    if (h)
	h->buf[h->bufsz++] = ch;
}

int
html_getch(MIME *m, Html_content *h)
{
    int ch;
    int ch2;

    if (h && h->bufsz)
	return h->buf[--h->bufsz];

    /* get a character, convert %XX to a character code */
    ch = (*m->content->transdecodefunc)(m);

    if (ch != '%')
	return ch;

    ch = (*m->content->transdecodefunc)(m);

    if (!isxdigit(ch))
    {
	html_ungetch(m, h, ch);
	return '%';
    }

    ch2 = (*m->content->transdecodefunc)(m);

    if (!isxdigit(ch2))
    {
	html_ungetch(m, h, ch2);
	html_ungetch(m, h, ch);
	return '%';
    }

    if (ch >= '0' && ch <= '9')
	ch -= '0';
    else if (ch >= 'A' && ch <= 'F')
	ch -= 'A' + 10;
    else if (ch >= 'a' && ch <= 'f')
	ch -= 'a' + 10;

    if (ch2 >= '0' && ch2 <= '9')
	ch2 -= '0';
    else if (ch2 >= 'A' && ch2 <= 'F')
	ch2 -= 'A' + 10;
    else if (ch2 >= 'a' && ch2 <= 'f')
	ch2 -= 'a' + 10;

    return (ch << 4) | ch2;
}

int
html_parse_ident(short *buf, int len, char **ident, short **rest, int *restlen)
{
    return 0;
}

int
html_parse_string(short *buf, int len, char **ident, short **rest, int *restlen)
{
    return 0;
}

int
mime_htmlparsetag(MIME *m, Html_content *h)
{
    int ch;
    int inquote = 0;
    int startpos;
    short *buf;
    int buflen;

    if (h == NULL)
	return '<';

    startpos = h->tagbufsz;
    mime_html_addtagbuf(h, '<');

    /* Syntax to parse */
    /* tag: '<' ident <attributes> '>' */
    /* attributes: attribute attributes | [empty] */
    /* attribute: ident | ident = ident | ident = string */

    ch = html_getch(m, h);

    while (ch != EOF && (inquote || ch != '>'))
    {
	mime_html_addtagbuf(h, ch);
	ch = html_getch(m, h);

	if (ch == '\"')
	    inquote = !inquote;

	if (ch == '\n')
	    inquote = 0;
    }

    if (ch == EOF)
	mime_html_addtagbuf(h, MIME_EOT);

    mime_html_addtagbuf(h, ch);

    if (ch != EOF)
	mime_html_addtagbuf(h, MIME_EOT);

    /* ok, tagbuf now holds the HTML tag.  we should be able to parse out */
    /* the tag name and attributes from the buffer. */
    buf = &h->tagbuf[startpos];
    buflen = h->tagbufsz - startpos;

    /* XXX: parse the tag into attributes here */

    if (m->flags & MIME_HTMLTAGS)
	return MIME_BOT;

    h->tagbufpos = startpos;
    h->tagbufsz = startpos;

    if (ch == EOF)
	mime_html_addtagbuf(h, EOF);

    return MIME_TAG;
}

int
mime_htmlcontent_decode(MIME *m)
{
    int ch;
    Html_content *h = (Html_content *)m->content->cdata;

    /* construct our data structure if we don't have one. */
    if (h == NULL)
    {
	h = (Html_content *)malloc(sizeof (Html_content));

	if (h != NULL)
	{
	    m->content->cdata = h;
	    m->content->cfree = mime_htmlcontent_free;
	    memset(h, '\0', sizeof *h);
	    h->type = MIME_CONTENT_HTML;
	}
    }

    /* check for buffered tags that need to be returned */
    if (h->tagbufpos < h->tagbufsz)
    {
	ch = h->tagbuf[h->tagbufpos++];

	if (ch == EOF)
	{
//	    mime_htmlcontent_free(m->content->cdata);
//	    m->content->cdata = NULL;
//	    m->content->cfree = NULL;
	    return mime_end_content(m);
	}

	return ch;
    }

    ch = html_getch(m, h);

    /* return the next character in the body, but also check for a boundary */
    /* and if one is found switch back to normal processing. */
    if (ch == EOF)
	return mime_end_content(m);

    /* parse out mime tags, special charaters and encoded characters. */
    switch (ch)
    {
    case '<':
	/* parse <...> HTML tag.  save away the tag name and attributes. */
	/* return either MIME_BOT or MIME_TAG depending upon the flags. */
	return mime_htmlparsetag(m, h);
//    case '>':
//	mime_html_addtagbuf(h, MIME_EOT);
//	return '>';
    case '&':
	/* parse &xxx; construct and return an appropriate character code */
	/* XXX: need to implement */
    default:
	return ch;
    }

    return ch;
}

int
mime_igetch(MIME *m)
{
    int ch;

    if (m->ungetsz)
	return m->ungetbuf[--m->ungetsz];

    if (m->decodefunc)
	ch = (*m->decodefunc)(m);
    else
	ch = fgetc(m->fp);

//    printf("%c", ch);
    return ch;
}

static struct
{
    short bch;
    short ech;
    int mask;
} spectable[] =
{
    { MIME_BOH,  MIME_EOH,  MIME_HEADERS },
    { MIME_BOP,  MIME_EOP,  MIME_PREAMBLES },
    { MIME_BOB,  MIME_EOB,  MIME_BOUNDARIES },
    { MIME_BOCH, MIME_EOCH, MIME_CONTENTHEADERS },
    { MIME_BOC,  MIME_EOC,  MIME_CONTENT },
    { MIME_BOE,  MIME_EOE,  MIME_EPILOGS },
    { 0, 0, 0 },
};

int
mime_getch(MIME *m)
{
    int ch;

    if (m->ungetsz)
	return m->ungetbuf[--m->ungetsz];

    ch = mime_igetch(m);

    while (ch != EOF)
    {
	int i;

	if (ch < 512)
	    return ch;

	for (i = 0; spectable[i].bch; i++)
	    if (ch == spectable[i].bch)
		break;

	if (ch != spectable[i].bch)
	    return ch;

	if (m->flags & spectable[i].mask)
	    return ch;

	while (ch != EOF && ch != spectable[i].ech)
	    ch = mime_igetch(m);

	ch = mime_igetch(m);
    }

    return ch;
}

void 
mime_ungetch(MIME *m, int ch)
{
    /* create/expand unget buffer as necessary */
    if (m->ungetsz == m->ungetlen)
    {
	m->ungetbuf = (short *)realloc((void *)m->ungetbuf,
		sizeof (short) * (m->ungetlen + 64));

	if (m->ungetbuf)
	    m->ungetlen += 64;
	else
	{
	    m->ungetlen = 0;
	    return;
	}
    }

    m->ungetbuf[m->ungetsz++] = ch;
}

char *
mime_headername(MIME *m)
{
//    printf("Headername = %s\n", m->header ? m->header : "<none>");
    return m->header;
}

char *
mime_contenttype(MIME *m)
{
    Mime_content *c = m->content;

    if (c)
	return c->type;

    return NULL;
}

char *
mime_contentsubtype(MIME *m)
{
    Mime_content *c = m->content;

    if (c)
	return c->subtype;

    return NULL;
}

char *
mime_transferencoding(MIME *m)
{
    Mime_content *c = m->content;

    if (c && c->encoding)
	return c->encoding;

    return NULL;
}

char *
mime_contentparameter(MIME *m, char *header, char *param)
{
    Mime_content *c = m->content;

    if (c)
	return mime_find_content_param(c, header, param);

    return NULL;
}

char *
mime_charset(MIME *m)
{
    return mime_contentparameter(m, "Content-Type", "charset");
}

Html_content *
mime_html_findcontent(MIME *m)
{
    Mime_content *c = m->content;
    Html_content *h;

    if (c == NULL)
	return NULL;

    h = (Html_content *)c->cdata;

    if (h == NULL || h->type != MIME_CONTENT_HTML)
	return NULL;

    return h;
}

char *
mime_html_tagname(MIME *m)
{
    Html_content *h = mime_html_findcontent(m);
    return h ? h->tag : NULL;
}

int
mime_html_attributes(MIME *m)
{
    Html_content *h = mime_html_findcontent(m);
    return h ? h->numattribs : 0;
}

char *
mime_html_attributename(MIME *m, int n)
{
    Html_content *h = mime_html_findcontent(m);
    return (h && n >= 0 && n < h->numattribs) ? h->attribnames[n] : NULL;
}

char *
mime_html_attributevalue(MIME *m, int n)
{
    Html_content *h = mime_html_findcontent(m);
    return (h && n >= 0 && n < h->numattribs) ? h->attribvalues[n] : NULL;
}

#ifdef TEST

void
dofile(FILE *fp, int rmtags)
{
    MIME *m = mime_open(fp,
	    MIME_HEADERS|MIME_PREAMBLES|MIME_BOUNDARIES|MIME_CONTENTHEADERS|
	    MIME_CONTENT|MIME_EPILOGS|MIME_CRACKHTML|
	    (rmtags ? 0 : MIME_HTMLTAGS));
    int ch;
    int c = 0;
    int n = 0;

    while ((ch = mime_getch(m)) != EOF)
    {
	n++;

	if (ch & 512)
	    switch (ch)
	    {
	    case MIME_VERS:
		printf("<=VERS=>");
		break;
	    case MIME_MSGID:
		printf("<=MSGID=>");
		break;
	    case MIME_BOH:
		printf("<=BOH=>");
		break;
	    case MIME_EOH:
		printf("<=EOH=>");
		break;
	    case MIME_BOP:
		printf("<=BOP=>");
		break;
	    case MIME_EOP:
		printf("<=EOP=>");
		break;
	    case MIME_BOB:
		printf("<=BOB=>");
		break;
	    case MIME_EOB:
		printf("<=EOB=>");
		break;
	    case MIME_BOCH:
		printf("<=BOCH=>");
		break;
	    case MIME_EOCH:
		printf("<=EOCH=>");
		break;
	    case MIME_BOC:
		printf("<=BOC=>");
		c++;
		break;
	    case MIME_EOC:
		printf("<=EOC=>");

		if (c <= 0)
		    debugf("MIME End of Content, without matching BOC\n");

		c--;
		break;
	    case MIME_BOE:
		printf("<=BOE=>");
		break;
	    case MIME_EOE:
		printf("<=EOE=>");
		break;
	    case MIME_BOT:
		printf("<=BOT=>");
		break;
	    case MIME_EOT:
		printf("<=EOT=>");
		break;
	    case MIME_TAG:
		printf("<=TAG=>");
		break;
	    }
	else
	{
	    putchar(ch);

//	    if (ch == '\n')
//		printf("<=%d=>", n - 1);
	}
    }

    if (c > 0)
	debugf("MIME BOC without matching EOC\n");

    printf("<=EOF=>\n");
    mime_close(m);
}

int
main(int argc, char **argv)
{
    int i;
    int rmtags = 0;

    if (argc > 1)
    {
	if (strcmp(argv[1], "-t") == 0)
	{
	    rmtags = 1;
	    argv++;
	    argc--;
	}

	for (i = 1; i < argc; i++)
	{
	    FILE *fp = fopen(argv[i], "r");

	    if (fp == NULL)
		    continue;

	    dofile(fp, rmtags);
	    fclose(fp);
	}
    }
    else
	dofile(stdin, 0);

    exit(0);
}
#endif
