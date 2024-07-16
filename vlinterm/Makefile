all : vlintermtest

RAWDRAW:=../rawdraw/CNFGXDriver.c ../rawdraw/CNFGFunctions.c
CFLAGS:=-I..  -I../rawdraw -Os -s

vlintermtest : vlinterm.c vlintermtest.c $(RAWDRAW)
	gcc -o $@ $^ $(CFLAGS) -lX11 -lpthread -lutil
	size -A $@

clean :
	rm -rf *.o *~ vlintermtest
