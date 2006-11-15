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
#include <stdarg.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <errno.h>
#include <pwd.h>
#include <regex.h>

#include "spamlib.h"

#define MAIL_LOCKFILE ".mail.lock"
#define LOCKEXT ".lock"
#define DEFAULT_MBOX "inbox"
#define SPAM_MBOX "inbox.spam"
#define MAX_FORKATTEMPTS 60

char *prog;
char *user;
char *home;

void
dprintf(char *fmt, ...)
{
    static FILE *debug = NULL;
    va_list args;

    if (debug == NULL)
    {
	pid_t pid = getpid();
	int omask = umask(077);
	char name[BUFSIZ];

	sprintf(name, "/tmp/%s.%d", prog, pid);
	debug = fopen(name, "a");
	umask(omask);

	if (debug == NULL)
	    return;
    }

    va_start(args, fmt);
    vfprintf(debug, fmt, args);
    va_end(args);
    fflush(debug);
}

void
dprintfrm()
{
    pid_t pid = getpid();
    char name[BUFSIZ];

    sprintf(name, "/tmp/%s.%d", prog, pid);
    dprintf("Removing dprint output from %s\n", name);
    unlink(name);
}

int
setstdfd()
{
    char path[BUFSIZ];
    int fdout;
    int fderr;
    int fd;

    sprintf(path, "%s/.mail.output", home);
    fdout = open(path, O_WRONLY|O_CREAT|O_APPEND, 0644);
    dprintf("fdout = %d\n", fdout);

    if (fdout >= 0 && fdout != 1)
    {
	close(1);
	fd = fcntl(fdout, F_DUPFD, 1);

	if (fd >= 0)
	{
	    dprintf("fd = %d\n", fd);

	    if (fd != 1)
		close(fd);
	}

	close(fdout);
    }

    sprintf(path, "%s/.mail.errors", home);
    fderr = open(path, O_WRONLY|O_CREAT|O_APPEND, 0644);
    dprintf("fderr = %d\n", fderr);

    if (fderr >= 0 && fderr != 2)
    {
	close(2);
	fd = fcntl(fderr, F_DUPFD, 2);

	if (fd >= 0)
	{
	    dprintf("fd = %d\n", fd);

	    if (fd != 2)
		close(fd);
	}

	close(fderr);
    }

    return 1;
}

int
savemsg(FILE *input, char **nameptr, char **header)
{
    pid_t pid = getpid();
    static char name[BUFSIZ];
    FILE *fp;
    char *hbuf = NULL;
    int hbuflen = 0;
    int hlen = 0;
    int inheader = 1;
    int nl = 1;
    int ch;

    sprintf(name, "%s/.mail.%d", home, pid);
    fp = fopen(name, "w");

    if (fp == NULL)
    {
	dprintf("savemsg: could not open file %s\n", name);
	return 0;
    }

    while ((ch = getc(input)) != EOF)
    {
	if (inheader)
	{
	    if (hlen + 1 >= hbuflen)
	    {
		char *nbuf;

		while (hlen + 1 >= hbuflen)
		    hbuflen += BUFSIZ;

		nbuf = malloc(hbuflen);

		if (nbuf == NULL)
		{
		    dprintf("savemsg: could not get header in %s\n", name);
		    inheader = 0;
		}
		else
		{
		    if (hbuf)
		    {
			strncpy(nbuf, hbuf, hlen);
			free(hbuf);
		    }

		    hbuf = nbuf;
		}
	    }

	    if (inheader)
	    {
		hbuf[hlen] = ch;
		hlen++;
	    }
	}

	if (nl && ch == '\n')
	    inheader = 0;

	if (ch == '\n')
	    nl = 1;
	else if (ch != ' ' && ch != '\r')
	    nl = 0;

	putc(ch, fp);
    }

    if (hbuf)
	hbuf[hlen] = '\0';

    fclose(fp);

    if (nameptr)
	*nameptr = name;

    dprintf("Header = '%s'\n", hbuf ? hbuf : "<null>");

    if (header)
	*header = hbuf;
    else
	free(hbuf);

    return 1;
}

int
matchre(char *re, char *headers)
{
    regex_t exp;
    char errbuf[BUFSIZ];
    int err;

    err = regcomp(&exp, re, REG_EXTENDED|REG_ICASE|REG_NOSUB|REG_NEWLINE);

    if (err != 0)
    {
	regerror(err, &exp, errbuf, BUFSIZ);
	dprintf("RE compile failed, (%d) %s\n", err, errbuf);
	return 0;
    }

    err = regexec(&exp, headers, 0, NULL, REG_NOTEOL);

    if (err != 0 && err != REG_NOMATCH)
    {
	regerror(err, &exp, errbuf, BUFSIZ);
	regfree(&exp);
	dprintf("RE match failed, (%d) %s\n", err, errbuf);
	return 0;
    }

    regfree(&exp);
    return err == 0;
}

int
lookupmbox(char *header, char **boxname, int *checkforspam)
{
    char path[BUFSIZ];
    FILE *fp;
    char buf[BUFSIZ];
    char *box;
    char *re;

    sprintf(path, "%s/.mail.lists", home);
    fp = fopen(path, "r");

    if (fp == NULL)
    {
	if (boxname)
	    *boxname = DEFAULT_MBOX;

	if (checkforspam)
	    *checkforspam = 1;

	return 0;
    }

    while (fgets(buf, BUFSIZ, fp) != NULL)
    {
	int len = strlen(buf);
	box = buf;

	if (len < 1)
	    break;

	box = &buf[len - 1];

	while (box > buf && (*box == '\n' || *box == ' ' || *box == '\t'))
	    box--;

	box[1] = '\0';
	box = buf;

	if (*box == '#' || *box == ';' || *box == '\0')
	    continue;

	re = box + 1;

	while (*re != '\0' && *re != ' ' && *re != '\t')
	    re++;

	if (*re != '\0')
	    *re++ = '\0';

	while (*re == ' ' || *re == '\t')
	    re++;

	dprintf("Box = '%s', Re = '%s'\n", box, re);

	if (matchre(re, header))
	{
	    int cfs = 0;
	    dprintf("Matched\n");

	    if (box[0] == '*')
	    {
		box++;
		cfs = 1;
	    }


	    if (boxname)
		*boxname = strdup(box);

	    if (checkforspam)
		*checkforspam = cfs;

	    return 1;
	}
    }

    if (boxname)
	*boxname = DEFAULT_MBOX;

    if (checkforspam)
	*checkforspam = 1;

    return 0;
}

static struct pid {
	struct pid *next;
	FILE *fp;
	pid_t pid;
} *pidlist;

FILE *
xpopen(char **args, char *type)
{
    struct pid *cur;
    FILE *iop;
    int pdes[2], pid;

    if ((*type != 'r' && *type != 'w') || type[1])
	return (NULL);

    if ((cur = malloc(sizeof (struct pid))) == NULL)
	return (NULL);

    if (pipe(pdes) < 0)
    {
	free(cur);
	return (NULL);
    }

    switch (pid = vfork())
    {
    case -1:			/* Error. */
	(void)close(pdes[0]);
	(void)close(pdes[1]);
	free(cur);
	return (NULL);
	/* NOTREACHED */
    case 0:			/* Child. */
	if (*type == 'r')
	{
	    if (pdes[1] != STDOUT_FILENO)
	    {
		(void)dup2(pdes[1], STDOUT_FILENO);
		(void)close(pdes[1]);
	    }
	    (void)close(pdes[0]);
	}
	else
	{
	    if (pdes[0] != STDIN_FILENO)
	    {
		(void)dup2(pdes[0], STDIN_FILENO);
		(void)close(pdes[0]);
	    }
	    (void)close(pdes[1]);
	}
	setuid(geteuid());
	execvp(args[0], args);
	_exit(127);
	/* NOTREACHED */
    }

    /* Parent; assume fdopen can't fail. */
    if (*type == 'r')
    {
	iop = fdopen(pdes[0], type);
	(void)close(pdes[1]);
    }
    else
    {
	iop = fdopen(pdes[1], type);
	(void)close(pdes[0]);
    }

    /* Link into list of file descriptors. */
    cur->fp = iop;
    cur->pid = pid;
    cur->next = pidlist;
    pidlist = cur;

    return (iop);
}

/*
 * pclose --
 *	Pclose returns -1 if stream is not associated with a `popened' command,
 *	if already `pclosed', or waitpid returns an error.
 */
int
xpclose(FILE *iop)
{
    register struct pid *cur, *last;
    int omask;
    union wait pstat;
    pid_t pid;

    (void)fclose(iop);

    /* Find the appropriate file pointer. */
    for (last = NULL, cur = pidlist; cur; last = cur, cur = cur->next)
	if (cur->fp == iop)
	    break;
    if (cur == NULL)
	return (-1);

    /* Get the status of the process. */
    omask = sigblock(sigmask(SIGINT) | sigmask(SIGQUIT) | sigmask(SIGHUP));
    do
    {
	pid = waitpid(cur->pid, (int *)&pstat, 0);
    } while (pid == -1 && errno == EINTR);
    (void)sigsetmask(omask);

    /* Remove the entry from the linked list. */
    if (last == NULL)
	pidlist = cur->next;
    else
	last->next = cur->next;
    free(cur);

    return (pid == -1 ? -1 : pstat.w_status);
}

int
currentfolder(char *name, int len)
{
    char *args[3];
    FILE *fp;
    char buf[BUFSIZ];
    int l;
    int stat;

    args[0] = "folder";
    args[1] = "-fast";
    args[2] = NULL;
    fp = xpopen(args, "r");

    if (fp == NULL)
    {
	strcpy(name, "");
	dprintf("popen: failed\n");
	return 0;
    }

    if (fgets(buf, BUFSIZ, fp) == NULL)
    {
	strcpy(name, "");
	dprintf("fgets: failed\n");
	return 0;
    }

    stat = xpclose(fp);
    l = strlen(buf);

    if (buf[l - 1] == '\n')
	buf[l -1] = '\0';

    strncpy(name, buf, len);
    dprintf("Current folder = \"%s\"\n", buf);
    return stat == 0;
}

void
getpath(char *path, char *mailbox)
{
    int nobox = 0;
    struct stat statbuf;

    if (mailbox == NULL || *mailbox == '\0')
	nobox = 1;
    else
    {
	sprintf(path, "%s/Mail/%s", home, mailbox);

	if (stat(path, &statbuf) < 0)
	    nobox = 1;
	else if ((statbuf.st_mode & S_IFMT) != S_IFDIR)
	    nobox = 1;
    }

    if (nobox)
	sprintf(path, "%s/%s", home, MAIL_LOCKFILE);
    else
	sprintf(path, "%s/Mail/%s/%s", home, mailbox, LOCKEXT);
}

int
getlock(char *mailbox)
{
    char path[BUFSIZ];
    int fd;
    pid_t pid = getpid();
    int i;

    getpath(path, mailbox);

    for (i = 0; i < 3600; i++)
    {
	struct stat statbuf;
	char pidstr[20];

	if (stat(path, &statbuf) >= 0)
	{
	    dprintf("%s: lock file exists\n", path);

	    /* check to see if the process is still alive */ 
	    fd = open(path, O_RDONLY, 0444);

	    if (fd >= 0)
	    {
		int n = read(fd, pidstr, 20);
		close(fd);

		dprintf("%s: file opened, %d bytes read\n", path, n);

		if (n > 0)
		{
		    pid_t owner;

		    pidstr[n] = '\0';
		    owner = strtoul(pidstr, NULL, 10);

		    dprintf("%s: lock file pid = %d (%s)\n", path, owner,
			    pidstr);

		    if (owner > 0)
		    {
			if (kill(owner, 0) < 0)
			{
			    dprintf("%s: kill failed, errno = %d\n", path,
				    errno);

			    if (errno == ESRCH)
			    {
				/* process no longer exists */
				/* there is a race here, but what can we do */
				dprintf("%s: lock file abandoned by pid %d\n", path, owner);
				unlink(path);
			    }
			    else
				sleep(1);
			}
			else
			    sleep(1);
		    }
		    else
			sleep(1);
		}
		else
		    sleep(1);
	    }
	}

	fd = open(path, O_RDWR|O_CREAT|O_EXCL, 0444);

	if (fd >= 0)
	{
	    /* we got the lock, write our pid into the file */
	    char pidstr[20];
	    sprintf(pidstr, "%d", pid);
	    write(fd, pidstr, strlen(pidstr));
	    close(fd);
	    return 1;
	}
	else
	{
	    dprintf("%s: lock file open failed, errno = %d\n", path, errno);
	    sleep(1);
	}
    }

    return 0;
}

int
putlock(char *mailbox)
{
    char path[BUFSIZ];

    getpath(path, mailbox);
    unlink(path);
    return 1;
}

int
runcmd(char **argv)
{
    pid_t child;
    int stat;
    char buf[BUFSIZ];
    int i;

    sprintf(buf, "runcmd: '%s'", argv[0] ? argv[0] : "<null>");

    for (i = 1; argv[i]; i++)
	sprintf(&buf[strlen(buf)], ", '%s'", argv[i]);

    dprintf("%s\n", buf);
    child = vfork();

    while (child == -1)
    {
	if (errno != EAGAIN)
	    return 0;

	sleep(1);
	child = vfork();
    }

    if (child == 0)
    {
	setuid(geteuid());
	execvp(argv[0], argv);
	_exit(1);
    }

    dprintf("Child pid = %d\n", child);
    waitpid(child, &stat, NULL);
    return stat == 0;
}

#ifdef INC_MAIL
int
filemsg(char *filename, char *mailbox)
{
    char curfolder[BUFSIZ];
    char curfolder2[BUFSIZ];
    char cmd[BUFSIZ];
    char folder[BUFSIZ];
    char *args[20];

    /* grab $HOME/.mail.lock */
    if (!getlock(mailbox))
	return 0;

    /* get the current folder */
    currentfolder(curfolder, BUFSIZ);

    sprintf(cmd, "inc +%s -file %s -truncate -nochangecur", mailbox, filename);
    sprintf(folder, "+%s", mailbox);

    args[0] = "inc";
    args[1] = folder;
    args[2] = "-file";
    args[3] = filename;
    args[4] = "-truncate";
    args[5] = "-nochangecur";
    args[6] = NULL;
    runcmd(args);
/*    system(cmd);	*/
#if 0
    {
	FILE *fp = popen(cmd, "r");

	if (fp != NULL)
	{
	    char buf[BUFSIZ];

	    while (fgets(buf, BUFSIZ, fp) != NULL)
	    {
		buf[strlen(buf) - 1] = '\0';
		dprintf("cmd: --> %s\n", buf);
	    }

	    pclose(fp);
	}
    }
#endif

    currentfolder(curfolder2, BUFSIZ);

    if (strcmp(curfolder, curfolder2) != 0 && curfolder[0])
    {
	sprintf(cmd, "folder +%s", curfolder);
	sprintf(folder, "+%s", curfolder);
	args[0] = "folder";
	args[1] = folder;
	args[2] = NULL;
	runcmd(args);
/*	system(cmd);	*/
	dprintf("cmd: %s\n", cmd);
    }

    putlock(mailbox);
    return 1;
}
#else
int
filemsg(char *filename, char *mailbox)
{
    char folder[BUFSIZ];
    char *args[20];
    FILE *sink = NULL;
    FILE *src;
    int ch;
    int stat;
    int fa;

    /* grab $HOME/.mail.lock */
    if (!getlock(mailbox))
	return 0;

    src = fopen(filename, "r");

    if (src == NULL)
    {
	dprintf("Could not open up message file, %s\n", filename);
	putlock(mailbox);
	return 0;
    }

    sprintf(folder, "+%s", mailbox);

    args[0] = "rcvstore";
    args[1] = folder;
    args[2] = "-create";
    args[3] = NULL;

    for (fa = 0; fa < MAX_FORKATTEMPTS; fa++)
    {
	sink = xpopen(args, "w");

	if (sink != NULL)
	    break;

	sleep(1);
    }

    if (sink == NULL)
    {
	dprintf("Could not open rcvstore command, folder %s\n", folder);
	putlock(mailbox);
	fclose(src);
	return 0;
    }

    /* copy the file to the command */
    while ((ch = getc(src)) != EOF)
	putc(ch, sink);

    fclose(src);
    stat = xpclose(sink);

    putlock(mailbox);
    return stat == 0;
}
#endif

char *
findnextheader(char *header)
{
    char *hp = header;

    /* skip to end of line */
    while (*hp != '\0' && *hp != '\n')
	hp++;

    /* start of next line */
    if (*hp == '\n')
	hp++;

    /* while next line starts with white space */
    while (*hp == ' ' || *hp == '\t')
    {
	/* skip to end of line */
	while (*hp != '\0' && *hp != '\n')
	    hp++;

	/* start of next line */
	if (*hp == '\n')
	    hp++;
    }

    return hp;
}

char *
getheader(char *headers, char *tag)
{
    char *hp = headers;
    int tlen = strlen(tag);

    /* find a line in headers that starts with tag followed by a colon */
    /* suck in the first non blank after the colon until we get to a */
    /* new line followed by a blank */
    /* strip out leading and trailing white space and convert new lines */
    /* into a single space. */

    while (*hp != '\0')
    {
	if (hp[tlen] == ':' && strncasecmp(hp, tag, tlen) == 0)
	    break;

	/* skip to start of next header */
	hp = findnextheader(hp);
    }

    if (*hp == '\0')
	return NULL;

    {
	char *nhp = findnextheader(hp);
	char *buf;
	int len;

	hp = &hp[tlen + 1];

	while (*hp == ' ' || *hp == '\t')
	    hp++;

	while (nhp > hp && nhp[-1] == '\n')
	    nhp--;

	len = nhp - hp;
	
	/* convert bogus chars to spaces */
	for (buf = hp; buf < nhp; buf++)
	    if (*buf == '\r' || *buf == '\n' || *buf == '\f' || *buf == '\b')
		*buf = ' ';

	buf = malloc(len + 1);

	if (buf == NULL)
	    return NULL;

	strncpy(buf, hp, len);
	buf[len] = '\0';
	dprintf("Header '%s' is '%s'\n", tag, buf);
	return buf;
    }
}

char *
getsender(char *header)
{
    char *sender = getheader(header, "Sender");
    char *replyto = getheader(header, "Reply-To");
    char *from = getheader(header, "From");
    char *returnpath = getheader(header, "Return-Path");
    char *str;
    int len, i, e;
    char *sp;

    if (sender != NULL)
	str = sender;
    else if (replyto != NULL)
	str = replyto;
    else if (from != NULL)
	str = from;
    else 
	str = returnpath;

    if (str == NULL)
	return NULL;

    len = strlen(str);
    sp = &str[len - 1];

    /* erase old-style paren comments */
    e = 0;
    for (i = 0; i < len; i++)
    {
	if (str[i] == '(')
	    e++;
	else if (str[i] == ')')
	{
	    e--;
	    str[i] = ' ';
	}

	if (e)
	    str[i] = ' ';
    }

    dprintf("Str = '%s'\n", str);

    while (sp >= str && (*sp == ' ' || *sp == '\t' || *sp == '>'))
	sp--;

    dprintf("Angle search leaves 0x%X of 0x%X, '%s'\n", sp, str, sp);
    sp[1] = '\0';

    while (sp >= str && *sp != ' ' && *sp != '\t' && *sp != '<' && *sp != '>')
	sp--;

    sp++;
    dprintf("Address search leaves 0x%X of 0x%X, '%s'\n", sp, str, sp);

    dprintf("From = %s\n", sp);
    return sp;
}

int
flagmail(char *header, char *mailbox)
{
    char *sender = getsender(header);
    char *date = getheader(header, "Date");
    char path[BUFSIZ];
    FILE *fp;

    if (sender == NULL)
	sender = "Unknown@Unknown.dom";

    if (date == NULL)
	date = "";

    dprintf("Date = %s\n", date);

    sprintf(path, "%s/.mail.flag", home);
    fp = fopen(path, "a");

    if (fp)
    {
	fprintf(fp, "\nFrom (+%s) %s sent %s\n", mailbox, sender, date);
	dprintf("From (+%s) %s sent %s\n", mailbox, sender, date);
	fclose(fp);
    }

    return 1;
}

int
checkaliases(char *aliasfile, char *headers)
{
    char *ap;
    char *aliases;
    int aliaslen;
    FILE *fp = NULL;
    int ret = 0;
    char *user = getsender(headers);
    struct stat statbuf;

    if (user == NULL)
	return 0;

    /* get length of alias file and malloc that much space */
    if (stat(aliasfile, &statbuf) != 0)
	return 0;
	
    aliaslen = statbuf.st_size;
    aliases = malloc(aliaslen + 2);

    if (aliases == NULL)
	return 0;

    /* open and read in the entire alias file */
    fp = fopen(aliasfile, "r");

    if (fp == NULL)
    {
	free(aliases);
	return 0;
    }

    if (fread(aliases, sizeof aliases[0], aliaslen, fp) !=
	    aliaslen * sizeof aliases[0])
	aliaslen = 0;

    aliases[aliaslen] = '\0';
    aliases[aliaslen + 1] = '\0';
    fclose(fp);

    /* search the file for a matching alias */
    for (ap = aliases; *ap != '\0'; ap++)
    {
	char *c = NULL;
	char *e;

	/* find the beginning of the alias */
	while (*ap == ' ' || *ap == '\t')
	    ap++;
	
	/* comment? skip to next line */
	if (*ap == '#')
	{
	    while (*ap != '\n' && *ap != '\0')
		ap++;
	    
	    continue;
	}

	/* find the end of this alias */
	for (e = ap; *e != '\n' && *e != '\0'; e++)
	{
	    if (*e == ':' && c == NULL)
	    {
		c = e;
		*c++ = '\0';
	    }
	    else if (e[0] == '\\' && e[1] == '\n')
		e++;
	}

	while (*e == '\n' || *e == ' ' || *e == '\t')
	    *e-- = '\0';

	ap = e + 1;

	/* found a colon - skip whitespace to compare strings */
	if (c != NULL)
	{
	    while (*c == ' ' || *c == '\t')
		c++;

	    /* erase trailing comments */
	    for (e = c; *e != '\n' && *e != '\0'; e++)
		if (*e == '#')
		{
		    /* erase space before comment */
		    for (e--; *e == ' ' || *e == '\t'; e--)
			;

		    *++e = '\0';
		    break;
		}

	    dprintf("checkaliases: name='%s' alias='%s'\n", ap, c);

	    if (*c != '\0' && strcasecmp(c, user) == 0)
	    {
		dprintf("checkaliases: match user='%s' alias='%s'\n", user, c);
		ret = 1;
		break;
	    }
	}
    }

    free(aliases);
    return ret;
}

void
usage()
{
    fprintf(stderr, "Usage: %s [-u username]\n", prog);
}

int
main(int argc, char **argv)
{
    char *filename;
    char *header;
    char *mailbox;
    char *unknownbox = DEFAULT_MBOX;
    char *aliasfile = "Mail/aliases";
    char buf[200];
    struct passwd *pwent;
    int ch;
    uid_t uid = getuid();
    int errs = 0;
    int keep = 0;
    int cfs;
    int spam = 0;
    int add = 0;
    SPAMDB *db = NULL;
    int crackhtml = SPAM_CRACK_HTML;
    int maxinterest = 25;
    int trunc = 0;

    prog = strrchr(argv[0], '/');

    if (prog)
	prog++;
    else
	prog = argv[0];

    dprintf("Hello, World!\n");
    dprintf("UID=%d\n", getuid());
    dprintf("EUID=%d\n", geteuid());
    dprintf("PID=%d\n", getpid());

    user = NULL;

    while ((ch = getopt(argc, argv, "ahi:kl:t:u:x:")) != EOF)
	switch (ch)
	{
	case 'a':
	    add = 1;
	    break;
	case 'h':
	    crackhtml = 0;
	    break;
	case 'i':
	    maxinterest = atoi(optarg);
	    break;
	case 'k':
	    keep = 1;
	    break;
	case 'l':
	    aliasfile = optarg;
	    break;
	case 'u':
	    user = optarg;
	    break;
	case 't':
	    trunc = atoi(optarg);
	    break;
	case 'x':
	    unknownbox = optarg;
	    break;
	default:
	    usage();
	    break;
	}

    if (user == NULL)
    {
	pwent = getpwuid(getuid());

	if (pwent == NULL)
	{
	    dprintf("No pwent for uid %d\n", uid);
	    exit(1);
	}

	user = pwent->pw_name;
    }

    dprintf("USER=%s\n", user);

    pwent = getpwnam(user);

    if (pwent == NULL)
    {
	dprintf("No pwent for user name %s\n", user);
	exit(1);
    }

    uid = pwent->pw_uid;
    dprintf("NEWUID=%d\n", uid);
    home = strdup(pwent->pw_dir);
    dprintf("HOME=%s\n", home);

#define N(x) ((x) ? (x) : "<null>")

    dprintf("PATH=%s\n", N(getenv("PATH")));
    dprintf("USER=%s\n", N(getenv("USER")));
    dprintf("LOGNAME=%s\n", N(getenv("LOGNAME")));
    dprintf("SHELL=%s\n", N(getenv("SHELL")));
    setenv("HOME", home, 1);
    setenv("PATH", "/bin:/usr/bin:/usr/local/bin:/usr/local/libexec/nmh", 1);
    setenv("USER", user, 1);
    setenv("LOGNAME", user, 1);
    setenv("SHELL", "/bin/sh", 1);

    if (geteuid() == 0)
	seteuid(uid);

    setstdfd();

    /* save message in $HOME/.mail.$$ */
    if (!savemsg(stdin, &filename, &header))
    {
	dprintf("Failure in savemsg(stdin, &%s, &%s)\n", filename, header);
	errs++;
    }

    /* match header against $HOME/.mail.lists to get mailbox name
       - if there is no match, check aliases to see if this message
       is from anyone we know - if not either, assume it is SPAM
     */
    if (lookupmbox(header, &mailbox, &cfs))
    {
	;		/* use the mailbox we just looked up */
    }
    else if (checkaliases(aliasfile, header))
    {
	mailbox = DEFAULT_MBOX;	/* if in aliases list use default */
	cfs = 0;
    }
    else
    {
	mailbox = unknownbox;
	cfs = 1;
    }

    if (cfs)
    {
	float prob;
	db = spamopendb(NULL);

	/* may or may not be spam, so use the filter to check */
	spam = (spamcheckfilename(db, filename,
		SPAM_LOOKUP_GRAHAM|SPAM_LOOKUP_MIME|crackhtml, maxinterest,
		0, &prob) == 1);
	dprintf("Spam probability %f\n", prob);

	if (spam)
	    mailbox = SPAM_MBOX;	/* likely spam */

	/* we now index by message-id so this should work */
	if (add)
	    spamaddfile(db, filename, spam);

	/* option to limit length of spam files */
	if (spam && trunc > 0)
	{
	    struct stat statbuf;

	    if (stat(filename, &statbuf) == 0 && statbuf.st_size > trunc)
		truncate(filename, trunc);
	}
    }

    /* file the message away in the right folder */
    if (!filemsg(filename, mailbox))
    {
	dprintf("Failure in filemsg(%s, %s)\n", filename, mailbox);
	sprintf(buf, "inbox-for-%s", mailbox);
	mailbox = buf;

	if (!filemsg(filename, "inbox"))
	{
	    dprintf("Failure in filemsg(%s, %s)\n", filename, mailbox);
	    errs++;
	}
    }

    if (cfs)
	spamclosedb(db);

    if (!errs && !keep)
    {
	/* remove the temp file */
	dprintf("Removing %s\n", filename);
	unlink(filename);
    }

    /* flag incomming mail */
    if (!flagmail(header, mailbox))
    {
	dprintf("Failure in flagmail(%s, %s)\n", header, mailbox);
	errs++;
    }

    if (seteuid(0) < 0)
	dprintf("seteuid(0), errno = %d\n", errno);

    dprintf("errs = %d, keep = %d\n", errs, keep);

    if (!errs && !keep)
	dprintfrm();

    exit(0);
}
