EXE ?= ibniz
PKG_CONFIG ?= pkg-config

SDL_CFLAGS = $(shell $(PKG_CONFIG) --cflags sdl3)
SDL_LIBS = $(shell $(PKG_CONFIG) --libs sdl3)
CFLAGS ?= -O2
LDLIBS += -lm

all: $(EXE)

clean:
	rm -f *.o *~ ibniz vmtest uitest ibniz2c ibniz.exe whole.c

package: clean
	cd .. && cp -R src ibniz-1.18 && tar czf ibniz-1.18.tar.gz ibniz-1.18

winexe: clean
	cp * ../winbuild && cd ../winbuild && make -f Makefile.win

$(EXE): ui_sdl.o vm_slow.o clipboard.o compiler.o
	$(CC) $^ -o $@ $(LDFLAGS) $(SDL_LIBS) $(LDLIBS)

ui_sdl.o: ui_sdl.c ibniz.h font.i vm.h texts.i vm.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

clipboard.o: clipboard.c ibniz.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

compiler.o: compiler.c ibniz.h vm.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

vm_slow.o: vm_slow.c ibniz.h vm.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

font.i: font.pl
	perl font.pl > font.i

runtest: vmtest
	./vmtest

vmtest: vm_test.c vm_slow.o compiler.o ibniz.h vm.h
	$(CC) $(CPPFLAGS) $(CFLAGS) vm_test.c vm_slow.o compiler.o -o $@ $(LDFLAGS) $(LDLIBS)

uitest: ui_test.c ui_sdl.c vm_slow.o clipboard.o compiler.o ibniz.h vm.h texts.i font.i
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL_CFLAGS) ui_test.c vm_slow.o clipboard.o compiler.o -o $@ $(LDFLAGS) $(SDL_LIBS) $(LDLIBS)

runuitest: uitest
	SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy SDL_RENDER_DRIVER=software ./uitest

ibniz2c: ibniz2c.c compiler.c gen_c.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -DIBNIZ2C compiler.c ibniz2c.c -o $@ $(LDFLAGS) $(LDLIBS)

install: ${EXE}
	install -d ${DESTDIR}/usr/bin
	install $<  ${DESTDIR}/usr/bin/

.PHONY: all clean package winexe runtest runuitest install
