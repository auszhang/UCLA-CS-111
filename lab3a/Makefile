# NAME: Aidan Wolk, Christopher Aziz
# EMAIL: aidanwolk@g.ucla.edu, caziz@ucla.edu

# Handle wonky C++ compilation on SEASnet

ifneq (,$(findstring seas.ucla.edu,$(shell uname -n)))
CXX=g++ -Wl,--rpath=/usr/local/cs/lib64
else
CXX=g++
endif


lab3a: lab3a.cpp
	$(CXX) -Wall -Wextra -g $^ -o $@

.PHONY: dist
dist: lab3a-000000000.tar.gz

lab3a-000000000.tar.gz: lab3a.cpp Makefile README.md ext2_fs.h
	tar -czf $@ $^

.PHONY: clean
clean:
	-rm -f lab3a lab3a-000000000.tar.gz
