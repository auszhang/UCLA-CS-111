# NAME: Aidan Wolk, Christopher Aziz
# EMAIL: aidanwolk@g.ucla.edu, caziz@ucla.edu

# All this does is copy the Python source to another executable file
la3b: lab3b.py
	cp ./lab3b.py ./lab3b
	chmod +x ./lab3b

.PHONY: dist
dist: lab3b-000000000.tar.gz

lab3b-000000000.tar.gz: lab3b.py Makefile README.md
	tar -czf $@ $^

.PHONY: clean
clean:
	-rm -f lab3b lab3b-504756191.tar.gz
