
.PHONY: config.h

CC     = gcc
CFLAGS = -Wall -O2

all: zseal

config.h:
	./configure

zseal: zseal.c config.h
	$(CC) $(CFLAGS) zseal.c -o zseal

clean:
	/bin/rm -f zseal *~

