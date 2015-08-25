# yascreen
Yet Another Screen Library (curses replacement for daemons and embedded apps)

Main features
- small footprint
- does not have external dependencies
- allows both internal and external event loop
- allows stdin/stdout or external input/output (can work over socket)
- supports basic set of telnet sequences, so it is suitable for built-in terminal interfaces for daemons
- supports a limited set of input keystroke sequences, because I added only what in needed in my apps :)
- fully unicode compatible (parts of this depend on wcwidth in libc)
- supports utf8 verification of input
- relies only on a limited subset of ansi/xterm esc sequences, so it is compatible with mostly all modern terminals (inspired by [linenoise](https://github.com/antirez/linenoise))
- there is no curses API and ancient terminal compatibility, hence less bloat
- there is no autoconf
- clean API with opaque private data, usable from C/C++
- easy cross compilation setup (need to set CC, AR, STRIP and RANLIB)

Current development is done on Linux, with additional testing on OpenBSD/FreeBSD; other platforms may need minimal fixes.

