C = gcc
CFLAGS = -Wall -g -fpic

LDFLAGS = -Wall -g

LD = gcc

intel-all: lib/liblwp.so

lib/liblwp.so: lib lwp.o rr.o magic64.o
	$(CC) $(CFLAGS) -shared -o $@ lwp.o rr.o magic64.o

lib:
	mkdir lib

lwp.o: lwp.c
	$(CC) $(CFLAGS) -m64 -c -o lwp.o lwp.c

rr.o: rr.c
	$(CC) $(CFLAGS) -m64 -c -o rr.o rr.c

magic64.o: magic64.S
	$(CC) -o magic64.o -c magic64.S

nums: numbersmain.o liblwp.so numbersmain.c
	$(LD) $(LDFLAGS) -c -o nums numbersmain.o numbersmain.c -L. -llwp

numbermain.o: lwp.h

clean:
	rm lwp.o rr.o magic64.o numbersmain.o

