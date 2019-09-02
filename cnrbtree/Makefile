all : testtree

testtree : testtree.c testtree2.c
	tcc -fextended-identifiers -o $@ $^ -O3 -g

clean :
	rm -rf *.o *~ testtree

