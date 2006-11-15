
#CFLAGS=		-DINC_MAIL
#CFLAGS=		-g -pg
CFLAGS=		-pipe -Wall -O -g

PROGS= spamtrain spaminit spamdumpdb spamcheck spamword spamwords spamrefile \
	spamremove domail notspam spam mime

all:		$(PROGS)

x:	x.o spamlib.a
	$(CC) $(CFLAGS) -o x x.o spamlib.a -lm

domail:		domail.o spamlib.a spamlib.h
	$(CC) $(CFLAGS) -o domail domail.o spamlib.a -lm
#	chown root domail
#	chmod 4511 domail

spamtrain:	spamtrain.o spamlib.a spamlib.h
	$(CC) $(CFLAGS) -o spamtrain spamtrain.o spamlib.a -lm

spaminit:	spaminit.o spamlib.a spamlib.h
	$(CC) $(CFLAGS) -o spaminit spaminit.o spamlib.a -lm

spamdumpdb:	spamdumpdb.o spamlib.a spamlib.h
	$(CC) $(CFLAGS) -o spamdumpdb spamdumpdb.o spamlib.a -lm

spamcheck:	spamcheck.o spamlib.a spamlib.h
	$(CC) $(CFLAGS) -o spamcheck spamcheck.o spamlib.a -lm

spamword:	spamword.o spamlib.a spamlib.h
	$(CC) $(CFLAGS) -o spamword spamword.o spamlib.a -lm

spamwords:	spamwords.o spamlib.a spamlib.h
	$(CC) $(CFLAGS) -o spamwords spamwords.o spamlib.a -lm

spamrefile:	spamrefile.o spamlib.a spamlib.h
	$(CC) $(CFLAGS) -o spamrefile spamrefile.o spamlib.a -lm

spamremove:	spamremove.o spamlib.a spamlib.h
	$(CC) $(CFLAGS) -o spamremove spamremove.o spamlib.a -lm

notspam:	notspam.o spamlib.a spamlib.h
	$(CC) $(CFLAGS) -o notspam notspam.o spamlib.a -lm

spam:		notspam
	cp notspam spam

mime:	mimet.o spamlib.a spamlib.h
	$(CC) $(CFLAGS) -o mime mimet.o spamlib.a -lm

spamlib.o:	spamlib.h
hashtable.o:	spamlib.h
xdb.o:		spamlib.h
spaminit.o:	spamlib.h
spamcheck.o:	spamlib.h
spamword.o:	spamlib.h
spamwords.o:	spamlib.h
spamrefile.o:	spamlib.h
spamtrain.o:	spamlib.h
spamdumpdb.o:	spamlib.h
notspam.o:	spamlib.h
mime.o:		spamlib.h

mimet.o:		mime.c
	cp mime.c mimet.c
	$(CC) $(CFLAGS) -c -DTEST mimet.c

spamlib.a:	spamlib.o hashtable.o xdb.o mime.o
	ar rv spamlib.a spamlib.o hashtable.o xdb.o mime.o

install:	$(PROGS)
	cp $(PROGS) /usr/local/bin

files: $(PROGS)
	@echo $(PROGS)

list:
	for p in $(PROGS); do ls -l /usr/local/bin/$$p $$p; echo; done

clean:
	rm -f $(PROGS) *.[oa]

dir:
	@pwd
	@ls -l $(PROGS)
	@echo "/usr/local/bin"
	@(cd /usr/local/bin; ls -l $(PROGS) )
