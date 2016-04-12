PROGNAME=uma
OBJECTS=parsing.o virtual-address-space.o polyhedral-slice.o parameters.o concurrent.o config.o support.o 

all : program

program: main parsing virtual-address-space polyhedral-slice parameters concurrent support config
	gcc $(PROGNAME).o $(OBJECTS) -l pet -l isl -o $(PROGNAME)

main: $(PROGNAME).c support.h partitioning.h config.h
	gcc $(CFLAGS) -c $(PROGNAME).c -o $(PROGNAME).o

parsing: parsing.c partitioning.h support.h
	gcc $(CFLAGS) -c parsing.c -o parsing.o

virtual-address-space: virtual-address-space.c partitioning.h support.h
	gcc $(CFLAGS) -c virtual-address-space.c -o virtual-address-space.o

polyhedral-slice: polyhedral-slice.c partitioning.h config.h support.h
	gcc $(CFLAGS) -c polyhedral-slice.c -o polyhedral-slice.o

parameters: parameters.c partitioning.h config.h support.h
	gcc $(CFLAGS) -c parameters.c -o parameters.o

concurrent: concurrent.c partitioning.h support.h
	gcc $(CFLAGS) -c concurrent.c -o concurrent.o

config: config.c config.h
	gcc $(CFLAGS) -c config.c -o config.o

support: support.c support.h colours.h
	gcc $(CFLAGS) -c support.c -o support.o

clean:
	rm -f *.o
	rm -f *.~
