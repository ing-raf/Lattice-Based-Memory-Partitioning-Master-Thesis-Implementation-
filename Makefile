PROGNAME=uma

all : program

program: main support
	gcc $(PROGNAME).o support.o -l pet -l isl -o $(PROGNAME)

main: $(PROGNAME).c support.h
	gcc $(CFLAGS) -c $(PROGNAME).c -o $(PROGNAME).o

support: support.c support.h colours.h
	gcc -c support.c -o support.o

clean:
	rm -f *.o
	rm -f *.~
