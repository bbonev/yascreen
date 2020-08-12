# yascreen

Yet Another Screen Library (curses replacement for daemons and embedded apps)
=============================================================================

Main features
- small footprint
- does not have external dependencies
- allows both internal and external event loop
- allows stdin/stdout or external input/output (can work over socket)
- supports basic set of telnet sequences, making it suitable for built-in terminal interfaces for daemons
- supports a limited set of input keystroke sequences
- fully unicode compatible (parts of this depend on wcwidth in libc)
- supports utf8 verification of input
- relies only on a limited subset of ansi/xterm ESC sequences, making it compatible with mostly all modern terminals (inspired by [linenoise](https://github.com/antirez/linenoise))
- there is no curses API and ancient terminal compatibility, hence less bloat
- there is no autoconf
- clean API with opaque private data, usable from C/C++
- easy cross compilation setup (by setting CC, AR, STRIP and RANLIB)

Current development is done on Linux, with additional testing on OpenBSD/FreeBSD; other platforms may need minimal fixes.

On \*BSD a `gmake` is required to build.

Initialization
==============

yascreen uses an opaque data structure, allocated by the library and dynamically resized when needed - `yascreen_init(int sx, int sy)`. An application may specify (0,0) to let yascreen detect the size or use a fixed size.

There are two modes of operation - telnet protocol over socket or running in terminal. For sockets the event loop would typically be handled outside of the library while for terminals a built-in event loop may be used.

Modes of operation can be modified at runtime.

For terminal use signal handling (`SIGWINCH`) should always be handled by the application.

# Example initialization for terminal and handling of SIGWINCH

```
yascreen *s;
int winch=0;

void sigwinch(int sign) {
	winch=1;
}

s=yascreen_init(0,0); // let yascreen get term size
yascreen_term_set(s,YAS_NOBUFF|YAS_NOSIGN|YAS_NOECHO);
signal(SIGWINCH,sigwinch);

for (;;) { // main loop
	if (winch) {
		winch=0;
		if (yascreen_resize(s,0,0))
			// handle a fatal error - no memory
		// get the new sizes and redraw
		newsizex=yascreen_sx(s);
		newsizey=yascreen_sy(s);
	}

	...
	// option 1

	// input is handled in external event loop and fed to yascreen via yascreen_feed
	if (FD_ISSET(STDIN_FILENO,&r)&&sizeof c==read(STDIN_FILENO,&c,sizeof c))
		yascreen_feed(s,c); // pump state machine with bytestream

	// keys are processed only when available without delay/blocking
	while ((ch=yascreen_getch_nowait(s))!=-1) {
		// handle processed keys
	}

	...
	// option 2

	// input is handled by yascreen and key or -1 is returned not longer than TIMEOUT ms
	// note: if screen update is based on this, keypresses will force it
	while ((ch=yascreen_getch_to(s,TIMEOUT))!=-1) {
		// handle processed keys
	}

	...
	// option 3

	// input is handled by yascreen and the following call will block until a key is pressed
	if ((ch=yascreen_getch(s))!=-1) {
		// handle processed key
	}
}

```

For sockets input is handled like option 1 in the example above and yascreen needs to be provided with a callback for output.

In multiprocess mode deamons where `stdin`/`stdout` are redirected to a socket the same model from the example above can be used. Obviously SIGWINCH will work only for terminals and for screen size either telnet or ASNI sequences should be used.

# Example initialization for socket with external event loop and telnet sequences processing

```
yascreen *s;

s=yascreen_init(80,25); // there is no guarantee that screen size detection is supported on the remote end
yascreen_setout(s,ouput_cb); // set callback for output
yascreen_set_telnet(s,1); // enable processing of telnet sequences
yascreen_init_telnet(s); // try to negotiate telnet options (regardless if telnet processing is enabled)
yascreen_reqsize(s); // request initial screen size
for (;;) { // main loop
	...
	yascreen_feed(s,c); // feed input from the socket to yascreen

	// keys are processed only when available without delay/blocking
	while ((ch=yascreen_getch_nowait(s))!=-1) {
		// handle processed keys

		// screen size change is reported as a special keypress code:
		if (ch==YAS_TELNET_SIZE||ch==YAS_SCREEN_SIZE) // screen size change reported via telnet or ANSI sequence
			// redraw
	}
}

```
