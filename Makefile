C = gcc
CFLAGS = -Wall -g -pedantic -I.

all: lwp.c

lwp: lwp.o
	$(CC) $(CFLAGS) -o lwp lwp.o

lwp.o: lwp.c
	$(CC) $(CFLAGS) -c lwp.c

matadd: matadd.o
	$(CC) $(CFLAGS) -o matadd matadd.o -lpthread

matadd.o: matadd.c
	$(CC) $(CFLAGS) -c matadd.c

clean:
	rm lwp.o

