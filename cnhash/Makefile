all : cnhashtest

cnhashtest :  cnhashtest.c cnhash.c
	gcc -o $@ $^ -g -O0

clean :
	rm -rf cnhashtest *.o *~

