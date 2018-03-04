all : httptest

HTTP:=../http/cnhttp.c ../http/http_bsd.c ../http/mfs.c ../http/sha1.c

CFLAGS:=-I../http -g
LDFLAGS:=-lm

httptest : $(HTTP) httptest.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean :
	rm -rf *.o *~ httptest



