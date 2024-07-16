TARGETS:=slowpipe hex2eng scatter2svg chexdump synczcat prependtime

all : $(TARGETS)

prependtime : prependtime.o
	gcc -o $@ $^

chexdump : chexdump.o
	gcc -o $@ $^

slowpipe : slowpipe.o
	gcc -o $@ $^

hex2eng : hex2eng.o
	gcc -o $@ $^

scatter2svg : scatter2svg.o
	gcc -o $@ $^

synczcat : synczcat.c
	gcc -O2 -o $@ $^ -lz -s

install :
	sudo cp $(TARGETS) /usr/local/bin/

clean :
	rm -rf $(TARGETS) *.o *~
