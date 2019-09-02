all : testtree cpptest


cpptest : cpptest.cpp
	g++ -o $@ $^ -O3 -g

testtree : testtree.c testtree2.c
	gcc -o $@ $^ -O2 -g -pg

clean :
	rm -rf *.o *~ testtree cpptest

