# Copyright (c) 1991 by Parag Patel.  All Rights Reserved.
# $Header: /u/cgt/cvs/lib/sym/Makefile,v 1.3 2001/01/22 21:18:57 tjm Exp $

CFLAGS = -O -g
CC=gcc

SRCS =	addsym.c findsym.c freesym.c freesymtab.c hashsym.c linksym.c \
	nextsym.c nsyms.c rmsym.c symalloc.c symcmp.c symfns.c \
	symtaball.c allochash.c _growtable.c unlinksym.c
OBJS = $(SRCS:.c=.o)
LIB = sym.a

all: $(LIB)

lib: $(LIB)

$(LIB): $(OBJS)
	ar ru $(LIB) $(OBJS)
	-[ -x /usr/bin/ranlib ] && ranlib $(LIB)

$(OBJS):  defs.h

clean:
	rm -f *.a *.o
