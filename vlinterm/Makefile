all : vlintermtest

RAWDRAW:=../rawdraw/CNFGXDriver.c ../rawdraw/CNFGFunctions.c
CFLAGS:=-I..  -I../rawdraw -O0 -g

vlintermtest : vlinterm.c vlintermtest.c $(RAWDRAW)
	gcc -o $@ $^ $(CFLAGS) -lX11 -lpthread -lutil

clean :
	rm -rf *.o *~ vlintermtest
