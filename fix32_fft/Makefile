all : test_fft

test_fft : test_fft.c
	gcc -o $@ $^ -lm

testphasor : testphasor.c
	gcc -o $@ $^ -lm

genrotations : genrotations.c
	gcc -o  $@ $^ -lm

sinetablegen : sinetablegen.c
	gcc -o $@ $^ -lm

clean :
	rm -rf test_fft testphasor sinetablegen

