C = gcc
CFLAGS = -Wall -g -fpic

intel-all: liblwp.so

liblwp.so: lwp.o rr.o magic64.o
	$(CC) $(CFLAGS) -shared -o $@ lwp.o rr.o magic64.o

lwp.o: lwp.c
	$(CC) $(CFLAGS) -m64 -c -o lwp.o lwp.c

rr.o: rr.c
	$(CC) $(CFLAGS) -m64 -c -o rr.o rr.c

magic64.o: magic64.S
	$(CC) -o magic64.o -c magic64.S

clean:
	rm lwp.o rr.o magic64.o

