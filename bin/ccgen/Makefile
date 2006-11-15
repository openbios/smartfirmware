CC = gcc

.SUFFIXES:	.cc .o

.cc.o:
	$(CC) $(CFLAGS) -c $<

ccgen:	ccgen.o
	$(CC) $(CFLAGS) -o ccgen ccgen.o

export:	ccgen
	cp ccgen /cgt/bin

clean:
	rm -f *.o ccgen
