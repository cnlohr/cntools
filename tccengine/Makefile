all : example_host

CFLAGS:=-I.. -I../cnrbtree -DTCCCRASH_STANDALONE -Itcc/tinycc
LDFLAGS:=-lpthread -lm tcc/libtcc_$(shell uname -m).a -ldl 

example_host : example_host.c tccengine.c tccexports.c tcccrash.c symbol_enumerator.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS) -rdynamic -O0 -g

clean :
	rm -rf *.o *~ example_host

