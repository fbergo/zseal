
CC     = gcc
CFLAGS = -Wall -O2

all: zseal

zseal: zseal.c
	$(CC) $(CFLAGS) zseal.c -o zseal

clean:
	/bin/rm -f zseal *~


