all : testtree cpptest


cpptest : cpptest.cpp
	g++ -o $@ $^ -flto -O3 -g

testtree : testtree.c testtree2.c
	gcc -o $@ $^ -flto -O3 -g

clean :
	rm -rf *.o *~ testtree cpptest

