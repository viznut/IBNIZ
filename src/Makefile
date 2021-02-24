# For normal builds; remove -DX11 -lX11 from flags if you don't have X11
#CC=gcc
EXE=ibniz

CPPFLAGS=-DX11
LDFLAGS=`sdl-config --libs` -lX11 -s
CFLAGS=`sdl-config --cflags`

all: ibniz

# For win32 builds using mingw32 (you'll probably need to modify these)
#CC=i586-mingw32msvc-gcc
#EXE=ibniz.exe
#FLAGS=-L./SDL-1.2.14/lib -I./SDL-1.2.14/include -static -lmingw32 SDL-1.2.14/lib/libSDL.a SDL-1.2.14/lib/libSDLmain.a -mwindows -lwinmm
#all: ibniz.exe

clean:
	rm -f *.o *~ ibniz vmtest ibniz.exe whole.c

package: clean
	cd .. && cp -R src ibniz-1.18 && tar czf ibniz-1.18.tar.gz ibniz-1.18

winexe: clean
	cp * ../winbuild && cd ../winbuild && make -f Makefile.win

#$(EXE): whole.c
#	$(CC) -s -O3 -ffast-math -fwhole-program whole.c -o $(EXE) $(FLAGS) -lm

#whole.c: vm_slow.c ui_sdl.c clipboard.c texts.i font.i vm.h ibniz.h
#	cat ui_sdl.c vm_slow.c clipboard.c > whole.c

$(EXE): ui_sdl.o vm_slow.o clipboard.o compiler.o
	$(CC) -Os ui_sdl.o vm_slow.o clipboard.o compiler.o -o $(EXE) $(LDFLAGS) -lm

ui_sdl.o: ui_sdl.c ibniz.h font.i vm.h texts.i vm.h
	$(CC) -c -Os ui_sdl.c -o ui_sdl.o $(CFLAGS) $(CPPFLAGS)

clipboard.o: clipboard.c ibniz.h
	$(CC) -c -Os clipboard.c -o clipboard.o $(CFLAGS) $(CPPFLAGS)

compiler.o: compiler.c ibniz.h vm.h
	$(CC) -c -Os compiler.c -o compiler.o $(CFLAGS) $(CPPFLAGS)

vm_slow.o: vm_slow.c ibniz.h vm.h
	$(CC) -c -O3 vm_slow.c -o vm_slow.o $(CFLAGS) $(CPPFLAGS)

font.i: font.pl
	perl font.pl > font.i

runtest: vmtest
	./vmtest

vmtest: vm_test.c vm_slow.c
	gcc vm_test.c vm_slow.o -o vmtest -lm

ibniz2c: ibniz2c.c compiler.c gen_c.c
	gcc -DIBNIZ2C -Os compiler.c ibniz2c.c -o ibniz2c -lm

install: ${EXE}
	install -d ${DESTDIR}/usr/bin
	install $<  ${DESTDIR}/usr/bin/
