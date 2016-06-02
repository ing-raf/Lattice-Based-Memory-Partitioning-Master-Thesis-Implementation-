PROGNAME=numa
OBJECTS=parsing.o virtual-address-space.o polyhedral-slice.o parameters.o concurrent.o mlp.o config.o support.o model.o
LIBS=-lpet -lisl -lbarvinok -lglpk
SYSLIBS=-lntl -lpolylibgmp -lgmp -lstdc++ -lm

all: program

program: main parsing virtual-address-space polyhedral-slice parameters concurrent mlp support config model
	gcc $(PROGNAME).o $(OBJECTS) $(LIBS) $(SYSLIBS) -o $(PROGNAME)

main: $(PROGNAME).c support.h partitioning.h config.h model.h
	gcc $(CFLAGS) -c $(PROGNAME).c -o $(PROGNAME).o

mlp: mlp.c partitioning.h config.h model.h support.h
	gcc $(CFLAGS) -c mlp.c -o mlp.o

parsing: parsing.c partitioning.h config.h support.h model.h
	gcc $(CFLAGS) -c parsing.c -o parsing.o

virtual-address-space: virtual-address-space.c partitioning.h config.h support.h model.h
	gcc $(CFLAGS) -c virtual-address-space.c -o virtual-address-space.o

polyhedral-slice: polyhedral-slice.c partitioning.h config.h support.h model.h
	gcc $(CFLAGS) -c polyhedral-slice.c -o polyhedral-slice.o

parameters: parameters.c partitioning.h config.h support.h model.h
	gcc $(CFLAGS) -c parameters.c -o parameters.o

concurrent: concurrent.c partitioning.h config.h support.h model.h
	gcc $(CFLAGS) -c concurrent.c -o concurrent.o

model: model.c model.h config.h
	gcc $(CFLAGS) -c model.c -o model.o

config: config.c config.h
	gcc $(CFLAGS) -c config.c -o config.o

support: support.c support.h colours.h
	gcc $(CFLAGS) -c support.c -o support.o

clean:
	rm -f *.o
	rm -f *.~
