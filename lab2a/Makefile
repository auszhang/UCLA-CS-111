# NAME: Christopher Aziz
# EMAIL: caziz@ucla.edu
# ID: 304806012

CFLAGS = -Wall -Wextra -g -pthread
CC = gcc
STUDENT_ID = 304806012

build: lab2_add lab2_list

lab2_add: lab2_add.c
	$(CC) $(CFLAGS) -o $@ $^

lab2_list: SortedList.c lab2_list.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf lab2_add lab2_list *.tar.gz

tests: build lab2_test.sh
	-./lab2_test.sh

graphs: tests
	./lab2_add.gp
	./lab2_list.gp

dist: build graphs
	tar -czvf lab2a-$(STUDENT_ID).tar.gz SortedList.h SortedList.c lab2_add.c lab2_list.c lab2_test.sh *.png lab2_add.csv lab2_list.csv Makefile README lab2_add.gp lab2_list.gp
