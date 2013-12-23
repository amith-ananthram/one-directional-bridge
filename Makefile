# Query make file

CC=gcc
CFLAGS=-Wall -g -pedantic -std=c99 -ggdb -lpthread -m32
SOURCES=./bridge.c
CFILES=./bridge.c

bridge:		$(SOURCES) 
		$(CC) $(CFLAGS) -o bridge $(CFILES)

clean:
		@rm -f bridge
		@rm -f *~
		@rm -f *.o
		@rm -f core*
		@rm -f valout

test:	
		make clean
		make bridge
		bridge 5 3
		bridge 10 3
		bridge 20 3
		bridge 20 4
