all : galoistest

galoistest : galoistest.c
	gcc -o $@ $^ -lm -g -O2

galoistest-check-all-skews-are-empty : galoistest-check-all-skews-are-empty.c
	gcc -o $@ $^ -lm -g -O2

clean :
	rm -rf *.o *~ galoistest

