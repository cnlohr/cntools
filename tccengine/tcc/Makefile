## WARNING: If you build an ARM build, it will not be considered truly GPL-licensable!!!
##
## Tools you'll need:
## sudo apt-get install build-essential mingw-w64-i686-dev mingw-w64-x86-64-dev
##
all : libtcc_x86_64.a libtcc_i686.a include_tcc libtcc_win64.a libtcc_win32.a

.PHONY: incs

tinycc :
	git clone git://repo.or.cz/tinycc.git --depth=1

	#Picked a commit I know is good, slightly after 0.9.27
	#Consider getting  671dcace82debc4548029ff4fcc1a7f4db6d05e6


libtcc_i686.a : tinycc
	cd tinycc; CFLAGS=-m32 LDFLAGS=-m32 ./configure --cpu=i686; make clean all
	cp tinycc/libtcc.a ./libtcc_i686.a
	cp tinycc/libtcc1.a ./libtcc1_i686.a

libtcc_x86_64.a : tinycc
	cd tinycc; CFLAGS=-m64 LDFLAGS=-m64 ./configure --cpu=x86_64; make clean all
	cp tinycc/libtcc.a ./libtcc_x86_64.a
	cp tinycc/libtcc1.a ./libtcc1_x86_64.a

libtcc_win64.a : tinycc
	cd tinycc; CFLAGS=-m64 LDFLAGS=-m64 ./configure --cc=x86_64-w64-mingw32-gcc --config-mingw32 --enable-static --cpu=x86_64; make clean all
	cp tinycc/libtcc.a ./libtcc_win64.a
	cp tinycc/libtcc1.a ./libtcc1_win64.a

libtcc_win32.a : tinycc
	cd tinycc; CFLAGS=-m32 LDFLAGS=-m32 ./configure --cc=i686-w64-mingw32-gcc --config-mingw32 --enable-static --cpu=i386; make clean all
	cp tinycc/libtcc.a ./libtcc_win32.a
	cp tinycc/libtcc1.a ./libtcc1_win32.a


include_tcc : tinycc
	cp tinycc/include include_tcc -a
	cp tinycc/libtcc.h libtcc.h

