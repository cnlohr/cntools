all : bresenham-with-clip

bresenham-with-clip : bresenham-with-clip.c
	gcc -o $@ $^ -I../rawdraw -lX11

clean :
	rm -rf *.o *~

