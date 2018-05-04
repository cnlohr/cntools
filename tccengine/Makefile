all : example_host

CFLAGS:=-I..
LDFLAGS:=-lpthread -lm tcc/libtcc_$(shell uname -m).a -ldl 

example_host : example_host.c tccengine.c tccexports.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean :
	rm -rf *.o *~

