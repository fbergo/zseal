
.PHONY: config.h

CC     = gcc
CFLAGS = -Wall -O2

all: zseal

x32: zseal.32

config.h:
	./configure

zseal: zseal.c config.h
	$(CC) $(CFLAGS) zseal.c -o zseal

zseal.32: zseal.c config.h
	$(CC) $(CFLAGS) -m32 zseal.c -o zseal.32

clean:
	/bin/rm -f zseal *~

