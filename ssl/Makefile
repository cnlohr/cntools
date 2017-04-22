all : clienttest

clienttest : cnsslclient.c cnhttpclient.c cnhttpcommon.c clienttest.c
	gcc -O0 -g -o $@ $^ -lssl -lcrypto

clean :
	rm -rf clienttest
