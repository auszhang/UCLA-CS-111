# NAME: Christopher Aziz
# EMAIL: caziz@ucla.edu

CFLAGS = -Wall -Wextra -lm -lmraa -g
TLSFLAGS = -lssl -lcrypto
CC = gcc
STUDENT_ID = 000000000

# build executable
build: lab4c_tcp lab4c_tls

lab4c_tcp: lab4c_tcp.c
	$(CC) $(CFLAGS) -o $@ $^

lab4c_tls: lab4c_tls.c
	$(CC) $(CFLAGS) $(TLSFLAGS) -o $@ $^

# delete all programs and output generated by the Makefile
clean:
	rm -rf lab4c_tls lab4c_tcp *.tar.gz

# build distribution tarball
dist: README Makefile lab4c_tcp.c lab4c_tls.c
	tar -czvf lab4c-$(STUDENT_ID).tar.gz $^
