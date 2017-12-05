all : cnhttptest

CNHTTPC:=http/cnhttp.c http/http_bsd.c http/mfs.c http/sha1.c
CFLAGS:=-Ihttp

cnhttptest : custom_http.c $(CNHTTPC)
	gcc $(CFLAGS) -o $@ $^
