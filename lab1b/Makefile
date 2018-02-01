# NAME: Christopher Aziz
# EMAIL: caziz@ucla.edu

CFLAGS = -Wall -Wextra -g -lz
CC = gcc
STUDENT_ID = 000000000

build: lab1b-client lab1b-server

lab1b-client: lab1b-client.c
	$(CC) $(CFLAGS) -o lab1b-client lab1b-client.c

lab1b-server: lab1b-server.c
	$(CC) $(CFLAGS) -o lab1b-server lab1b-server.c

clean:
	rm -rf lab1b-server lab1b-client *.tar.gz *.dSYM

dist: clean
	tar -czvf lab1b-$(STUDENT_ID).tar.gz lab1b-client.c lab1b-server.c Makefile README
