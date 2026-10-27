# IBNIZ

IBNIZ is a virtual machine designed for extremely compact low-level audiovisual programs. The leading design goal is usefulness as a platform for demoscene productions, glitch art and similar projects. Mainsteam software engineering aspects are considered totally irrelevant.

IBNIZ stands for Ideally Bare Numeric Impression giZmo. The name also refers to Gottfried Leibniz, the 17th-century polymath who, among all, invented binary arithmetic, built the first four-operation calculating machine, and believed that the world was designed with the principle that a minimal set of rules should yield a maximal diversity.

<a href="http://www.youtube.com/watch?feature=player_embedded&v=aKMrBaXJvMs
" target="_blank"><img src="http://img.youtube.com/vi/aKMrBaXJvMs/0.jpg" 
alt="Demo Video" width="240" height="180" border="10" /></a>

* [Demo video](https://www.youtube.com/watch?v=aKMrBaXJvMs)
* [Documentation](src/ibniz.txt)
* [Web site](http://viznut.fi/ibniz/)
* Community: `#countercomplex` @ IRCnet
* [Javascript implementation](http://ibniz.breizh-entropy.org/) ([git](https://github.com/asiekierka/ibnjs))
* [Another Javascript implementation](https://flupe.github.io/jibniz/) ([git](https://github.com/flupe/jibniz))

## Building

### Linux

Prerequisites:
* [SDL3](https://www.libsdl.org) development files (3.2 or later)
* `pkg-config`, a C compiler, Make, and Perl

Run these commands:
```
cd src
make
```

### MacOS

Prerequisites:
* Apple developer tools (installed with Xcode)
* [SDL3](https://www.libsdl.org) (3.2 or later) and `pkg-config`
  (via [Homebrew](https://brew.sh): `brew install sdl3 pkgconf`)

Run these commands:
```
cd src
make -f Makefile.osx
```

### Windows

Prerequisites:
* [MSYS2](https://www.msys2.org/) with a MinGW toolchain, Make, and Perl
* [SDL3](https://www.libsdl.org) development files (3.2 or later) and `pkg-config`
  for the same MinGW environment (for UCRT64, install
  `mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-sdl3 mingw-w64-ucrt-x86_64-pkgconf make perl`)

Run these commands in the matching MSYS2 MinGW shell:
```
cd src
make -f Makefile.win
```

Keep the matching `SDL3.dll` beside `ibniz.exe` or on `PATH`.

All makefiles use `pkg-config sdl3` to find SDL3. For a custom installation,
set `PKG_CONFIG_PATH` to its pkg-config directory. SDL 1.2 and SDL2 are not used.

### Tests

Run `make -C src runtest runuitest` for the VM tests and headless SDL3
rendering, audio, editor, clipboard, and input checks.
