PROGNAME=uma

all : program

program: main support allocation
	gcc $(PROGNAME).o support.o allocation.o -l pet -l isl -o $(PROGNAME)

main: $(PROGNAME).c support.h partitioning.h
	gcc $(CFLAGS) -c $(PROGNAME).c -o $(PROGNAME).o

allocation: allocation.c partitioning.h
	gcc $(CFLAGS) -c allocation.c -o allocation.o
	
support: support.c support.h colours.h
	gcc $(CFLAGS) -c support.c -o support.o

clean:
	rm -f *.o
	rm -f *.~
