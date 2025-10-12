all : test_fft

test_fft : test_fft.c
	gcc -O2 -g -o $@ $^ -lm -Wall

genrotations : genrotations.c
	gcc -O3 -o  $@ $^ -lm

clean :
	rm -rf test_fft genrotations

