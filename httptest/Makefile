all : httptest single_file_test

HTTP:=../http/cnhttp.c ../http/http_bsd.c ../http/mfs.c ../http/sha1.c

CFLAGS:=-I../http -g
LDFLAGS:=-lm

httptest : $(HTTP) httptest.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

single_file_http.c : $(HTTP) httptest.c
	echo "//single_file_http.c base from https://github.com/cntools/httptest." > $@
	echo "//scroll to bottom for implementation." >> $@
	echo "" >> $@
	echo "#define CUSTOM_HTTPHEADER_CODE 	PushString(\"Access-Control-Allow-Origin: *\\\\r\\\\n\");" >> $@
	echo "" >> $@
	cat ../http/sha1.h | grep -v "include \"" >> $@
	cat ../http/mfs.h | grep -v "include \"" >> $@
	cat ../http/cnhttp.h | grep -v "include \"" >> $@
	cat ../http/http_bsd.h | grep -v "include \"" >> $@
	cat $^ | grep -v "include \"" >> $@

single_file_test : single_file_http.c
	gcc -o $@ $^ -Os -s -flto -ffunction-sections -fdata-sections -Wl,-relax,--gc-sections $(LDFLAGS)

clean :
	rm -rf *.o *~ httptest single_file_http.c single_file_test



