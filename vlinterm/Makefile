all : vlintermtest

RAWDRAW:=../rawdraw/CNFGXDriver.c ../rawdraw/CNFGFunctions.c
CFLAGS:=-I..  -I../rawdraw -g #-Os -s -m32

vlintermtest : vlinterm.c vlintermtest.c $(RAWDRAW)
	gcc -o $@ $^ $(CFLAGS) -lX11 -lpthread -lutil
	size -A $@

clean :
	rm -rf *.o *~ vlintermtest
