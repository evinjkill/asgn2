C = gcc
CFLAGS = -Wall -g -fpic

intel-all: lib/liblwp.so

lib/liblwp.so: lib lwp.o
	$(CC) $(CFLAGS) -shared -o $@ lwp.o

lib:
	mkdir lib

lwp.o: lwp.c
	$(CC) $(CFLAGS) -m64 -c -o lwp.o lwp.c

clean:
	rm lwp.o

