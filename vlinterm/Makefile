all : vlintermtest

RAWDRAW:=../rawdraw/CNFGXDriver.c ../rawdraw/CNFGFunctions.c

vlintermtest : vlinterm.c vlintermtest.c $(RAWDRAW)
	gcc -o $@ $^ -I../rawdraw -lX11

clean :
	rm -rf *.o *~ vlintermtest
