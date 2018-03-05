# NAME: Christopher Aziz
# EMAIL: caziz@ucla.edu

CFLAGS = -Wall -Wextra -g -pthread -lprofiler
CC = gcc
STUDENT_ID = 000000000

build: lab2_list

lab2_list: SortedList.c lab2_list.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf lab2_add lab2_list *.tar.gz

tests: build lab2_test.sh
	-./lab2_test.sh

graphs: tests
	./lab2_list.gp

profile: build
	LD_PRELOAD=/usr/lib64/libprofiler.so \
	CPUPROFILE=./raw.gperf \
	./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.out
	pprof --list=sync ./lab2_list ./raw.gperf >> profile.out
	rm -f ./raw.gperf

dist: build graphs
	tar -czvf lab2b-$(STUDENT_ID).tar.gz SortedList.h SortedList.c lab2_list.c lab2_test.sh lab2_list.gp *.png lab2b_list.csv Makefile README profile.out
