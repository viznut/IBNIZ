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
* [SDL](https://www.libsdl.org) v1.2.x

Run these commands:
```
cd src
make
```

### MacOS

Prerequisites:
* Apple developer tools (installed with Xcode)
* [SDL](https://www.libsdl.org) v1.2.x (an easy way to get SDL is via [Homebrew](https://brew.sh) `brew install sdl`)

Run these commands:
```
cd src
make -f Makefile.osx
```

### Windows

Prerequisites:
* [MinGW](http://www.mingw.org)
* [SDL](https://www.libsdl.org) v1.2.x

Run these commands:
```
cd src
make -f Makefile.win
```
