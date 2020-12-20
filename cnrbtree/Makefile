all : testtree cpptest


cpptest : cpptest.cpp
	g++ -o $@ $^ -O3 -flto -g

testtree : testtree.c testtree2.c
	gcc -o $@ $^ -O3 -flto -g

clean :
	rm -rf *.o *~ testtree cpptest

