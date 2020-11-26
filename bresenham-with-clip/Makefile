all : bresenham-with-clip

bresenham-with-clip : bresenham-with-clip.c
	gcc -o $@ $^ -I../rawdraw -lX11 -lm

clean :
	rm -rf *.o *~

