# NAME: Christopher Aziz
# EMAIL: caziz@ucla.edu
# ID: 000000000

CFLAGS = -Wall -Wextra -g
CC = gcc
STUDENT_ID = 000000000

build:
	$(CC) $(CFLAGS) -o lab1a lab1a.c
clean:
	rm -rf lab1a *.tar.gz *.dSYM
dist: clean
	tar -czvf lab1a-$(STUDENT_ID).tar.gz lab1a.c Makefile README