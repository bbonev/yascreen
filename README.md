YASCREEN 3 "September 30, 2020" yascreen "User-Manual"
==================================================

# NAME
  yascreen - Yet Another Screen Library (curses replacement for daemons and embedded apps)

# SYNOPSIS

  `#include <yascreen.h>`

# DESCRIPTION

## Main features

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
- there is no autoconf - there is no need to have one
- clean API with opaque private data, usable from C/C++
- easy cross compilation setup (by setting CC, AR, STRIP and RANLIB)

Current development is done on Linux, with additional testing on OpenBSD/FreeBSD; other platforms may need minimal fixes.

On \*BSD a `gmake` is required to build.

## Architecture

yascreen uses an opaque data structure, allocated by the library and dynamically resized when needed - `yascreen_init(int sx, int sy)` / `yascreen_resize(yascreen *s, int sx, int sy)`. An application may specify (0,0) for both calls to let yascreen detect the size or use a fixed size.

There are two modes of operation - telnet protocol over socket or running in terminal. For sockets the event loop would typically be handled outside of the library while for terminals a built-in event loop may be used.

Modes of operation can be modified at runtime.

For terminal use signal handling (`SIGWINCH`) should always be handled by the application.

## Example initialization for terminal and handling of SIGWINCH


```c
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

	…
	// option 1

	// input is handled in external event loop and fed to yascreen via yascreen_feed
	if (FD_ISSET(STDIN_FILENO,&r)&&sizeof c==read(STDIN_FILENO,&c,sizeof c))
		yascreen_feed(s,c); // pump state machine with bytestream

	// keys are processed only when available without delay/blocking
	while ((ch=yascreen_getch_nowait(s))!=-1) {
		// handle processed keys
	}

	…
	// option 2

	// input is handled by yascreen and key or -1 is returned not longer than TIMEOUT ms
	// note: if screen update is based on this, keypresses will force it
	while ((ch=yascreen_getch_to(s,TIMEOUT))!=-1) {
		// handle processed keys
	}

	…
	// option 3

	// input is handled by yascreen and the following call will block until a key is pressed
	if ((ch=yascreen_getch(s))!=-1) {
		// handle processed key
	}
}
```



For sockets input is handled like option 1 in the example above and yascreen needs to be provided with a callback for output.

In multiprocess mode daemons where `stdin`/`stdout` are redirected to a socket the same model from the example above can be used. Obviously SIGWINCH will work only for terminals and for sockets the screen size can get to be known either via telnet or ASNI sequences.

## Example initialization for socket with external event loop and telnet sequences processing


```c
yascreen *s;

s=yascreen_init(80,25); // there is no guarantee that screen size detection is supported on the remote end
yascreen_setout(s,output_cb); // set callback for output
yascreen_set_telnet(s,1); // enable processing of telnet sequences
yascreen_init_telnet(s); // try to negotiate telnet options (regardless if telnet processing is enabled)
yascreen_reqsize(s); // request initial screen size
for (;;) { // main loop
	…
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

API Reference
=============

Predefined constants and Helper macros
--------------------------------------


Internally style is kept into bitfields in a single integer variable - that includes foreground/background colors, style modifiers (bold, italic, underline, inverse and blink.

**Style codes**

| Name        | Function  |
|-------------|-----------|
|`YAS_ITALIC` | italic    |
|`YAS_UNDERL` | underline |
|`YAS_STRIKE` | stikeout  |
|`YAS_INVERSE`| inverse   |
|`YAS_BOLD`   | bold      |
|`YAS_BLINK`  | blink     |


**Color codes**

| Name           | Color                       |
|----------------|-----------------------------|
|`YAS_BLACK`     | black                       |
|`YAS_RED`       | red                         |
|`YAS_GREEN`     | green                       |
|`YAS_YELLOW`    | yellow                      |
|`YAS_BLUE`      | blue                        |
|`YAS_MAGENTA`   | magenta                     |
|`YAS_CYAN`      | cyan                        |
|`YAS_WHITE`     | white                       |
|`YAS_FGCOLORDEF`| default terminal foreground |
|`YAS_BGCOLORDEF`| default terminal background |

**Helper macros**

| Name                 | Description                                         |
|----------------------|-----------------------------------------------------|
|`YAS_FG(attribute)`   | extract foreground color                            |
|`YAS_BG(attribute)`   | extract background color                            |
|`YAS_FGCOLOR(color)`  | shift simple color value into foreground color      |
|`YAS_BGCOLOR(color)`  | shift simple color value into background color      |
|`YAS_FGXCOLOR(color)` | shift 256 palette color value into foreground color |
|`YAS_BGXCOLOR(color)` | shift 256 palette color value into background color |

All of the above can be or'ed into attribute, provided that the bits for foreground/background color are all zeroes.

**Key codes**

- Special, generated internally

| Name             | Value | Description           |
|------------------|------:|-----------------------|
|`YAS_K_NONE`      |    -1 | no key is available; in time limited mode means that the time limit expired |
|`YAS_SCREEN_SIZE` | 0x800 | notification for screen size change (may come because of telnet or ANSI sequence) |
|`YAS_TELNET_SIZE` | 0x801 | notification for screen size change; duplicates the above, may be used to differentiate how screen size change event was generated |

- Normal keys

| Name             | Value | Description           |
|------------------|------:|-----------------------|
|`YAS_K_NUL`       | 0x00  | Nul; on some terminals Ctrl-2 |
|`YAS_K_C_A`       | 0x01  | Ctrl-A |
|`YAS_K_C_B`       | 0x02  | Ctrl-B |
|`YAS_K_C_C`       | 0x03  | Ctrl-C |
|`YAS_K_C_D`       | 0x04  | Ctrl-D |
|`YAS_K_C_E`       | 0x05  | Ctrl-E |
|`YAS_K_C_F`       | 0x06  | Ctrl-F |
|`YAS_K_C_G`       | 0x07  | Ctrl-G |
|`YAS_K_C_H`       | 0x08  | Ctrl-H; depends on terminal see `YAS_K_BSP`, `YAS_K_C_8` |
|`YAS_K_C_I`       | 0x09  | Ctrl-I |
|`YAS_K_TAB`       | 0x09  | Tab |
|`YAS_K_C_J`       | 0x0a  | Ctrl-J |
|`YAS_K_C_K`       | 0x0b  | Ctrl-K |
|`YAS_K_C_L`       | 0x0c  | Ctrl-L |
|`YAS_K_C_M`       | 0x0d  | Enter, Return, Ctrl-M; see below |
|`YAS_K_RET`       | 0x0d  | Enter, Return, Ctrl-M; see above |
|`YAS_K_C_N`       | 0x0e  | Ctrl-N |
|`YAS_K_C_O`       | 0x0f  | Ctrl-O |
|`YAS_K_C_P`       | 0x10  | Ctrl-O |
|`YAS_K_C_Q`       | 0x11  | Ctrl-Q |
|`YAS_K_C_R`       | 0x12  | Ctrl-R |
|`YAS_K_C_S`       | 0x13  | Ctrl-S |
|`YAS_K_C_T`       | 0x14  | Ctrl-T |
|`YAS_K_C_U`       | 0x15  | Ctrl-U |
|`YAS_K_C_V`       | 0x16  | Ctrl-V |
|`YAS_K_C_W`       | 0x17  | Ctrl-W |
|`YAS_K_C_X`       | 0x18  | Ctrl-X |
|`YAS_K_C_Y`       | 0x19  | Ctrl-Y |
|`YAS_K_C_Z`       | 0x1a  | Ctrl-Z |
|`YAS_K_ESC`       | 0x1b  | Esc, Ctrl-3; see below; All ANSI sequences start with Esc, this is return after a timeout or double Esc |
|`YAS_K_C_3`       | 0x1b  | Esc, Ctrl-3; see above; All ANSI sequences start with Esc, this is return after a timeout or double Esc |
|`YAS_K_C_4`       | 0x1c  | Ctrl-4 |
|`YAS_K_C_5`       | 0x1d  | Ctrl-5 |
|`YAS_K_C_6`       | 0x1e  | Ctrl-6 |
|`YAS_K_C_7`       | 0x1f  | Ctrl-7 |
|`YAS_K_SPACE`     | 0x20  | Space |
|`YAS_K_EXCL`      | 0x21  | ! |
|`YAS_K_DQUOT`     | 0x22  | " |
|`YAS_K_HASH`      | 0x23  | # |
|`YAS_K_POUND`     | 0x24  | $ |
|`YAS_K_PERC`      | 0x25  | % |
|`YAS_K_AND`       | 0x26  | Ampersand |
|`YAS_K_QUOT`      | 0x27  | ' |
|`YAS_K_OBRA`      | 0x28  | ( |
|`YAS_K_CBRA`      | 0x29  | ) |
|`YAS_K_STAR`      | 0x2a  | * |
|`YAS_K_PLUS`      | 0x2b  | + |
|`YAS_K_COMMA`     | 0x2c  | , |
|`YAS_K_MINUS`     | 0x2d  | - |
|`YAS_K_DOT`       | 0x2e  | . |
|`YAS_K_SLASH`     | 0x2f  | / |
|`YAS_K_0`         | 0x30  | 0 |
|`YAS_K_1`         | 0x31  | 1 |
|`YAS_K_2`         | 0x32  | 2 |
|`YAS_K_3`         | 0x33  | 3 |
|`YAS_K_4`         | 0x34  | 4 |
|`YAS_K_5`         | 0x35  | 5 |
|`YAS_K_6`         | 0x36  | 6 |
|`YAS_K_7`         | 0x37  | 7 |
|`YAS_K_8`         | 0x38  | 8 |
|`YAS_K_9`         | 0x39  | 9 |
|`YAS_K_COLON`     | 0x3a  | : |
|`YAS_K_SEMI`      | 0x3b  | ; |
|`YAS_K_LT`        | 0x3c  | < |
|`YAS_K_EQ`        | 0x3d  | `=` |
|`YAS_K_GT`        | 0x3e  | > |
|`YAS_K_QUEST`     | 0x3f  | ? |
|`YAS_K_AT`        | 0x40  | @ |
|`YAS_K_A`         | 0x41  | A |
|`YAS_K_B`         | 0x42  | B |
|`YAS_K_C`         | 0x43  | C |
|`YAS_K_D`         | 0x44  | D |
|`YAS_K_E`         | 0x45  | E |
|`YAS_K_F`         | 0x46  | F |
|`YAS_K_G`         | 0x47  | G |
|`YAS_K_H`         | 0x48  | H |
|`YAS_K_I`         | 0x49  | I |
|`YAS_K_J`         | 0x4a  | J |
|`YAS_K_K`         | 0x4b  | K |
|`YAS_K_L`         | 0x4c  | L |
|`YAS_K_M`         | 0x4d  | M |
|`YAS_K_N`         | 0x4e  | N |
|`YAS_K_O`         | 0x4f  | O |
|`YAS_K_P`         | 0x50  | P |
|`YAS_K_Q`         | 0x51  | Q |
|`YAS_K_R`         | 0x52  | R |
|`YAS_K_S`         | 0x53  | S |
|`YAS_K_T`         | 0x54  | T |
|`YAS_K_U`         | 0x55  | U |
|`YAS_K_V`         | 0x56  | V |
|`YAS_K_W`         | 0x57  | W |
|`YAS_K_X`         | 0x58  | X |
|`YAS_K_Y`         | 0x59  | Y |
|`YAS_K_Z`         | 0x5a  | Z |
|`YAS_K_OSQ`       | 0x5b  | [ |
|`YAS_K_BSLASH`    | 0x5c  | `\` |
|`YAS_K_CSQ`       | 0x5d  | ] |
|`YAS_K_CARRET`    | 0x5e  | ^ |
|`YAS_K_USCORE`    | 0x5f  | `_` |
|`YAS_K_BTICK`     | 0x60  | \` |
|`YAS_K_a`         | 0x61  | a |
|`YAS_K_b`         | 0x62  | b |
|`YAS_K_c`         | 0x63  | c |
|`YAS_K_d`         | 0x64  | d |
|`YAS_K_e`         | 0x65  | e |
|`YAS_K_f`         | 0x66  | f |
|`YAS_K_g`         | 0x67  | g |
|`YAS_K_h`         | 0x68  | h |
|`YAS_K_i`         | 0x69  | i |
|`YAS_K_j`         | 0x6a  | j |
|`YAS_K_k`         | 0x6b  | k |
|`YAS_K_l`         | 0x6c  | l |
|`YAS_K_m`         | 0x6d  | m |
|`YAS_K_n`         | 0x6e  | n |
|`YAS_K_o`         | 0x6f  | o |
|`YAS_K_p`         | 0x70  | p |
|`YAS_K_q`         | 0x71  | q |
|`YAS_K_r`         | 0x72  | r |
|`YAS_K_s`         | 0x73  | s |
|`YAS_K_t`         | 0x74  | t |
|`YAS_K_u`         | 0x75  | u |
|`YAS_K_v`         | 0x76  | v |
|`YAS_K_w`         | 0x77  | w |
|`YAS_K_x`         | 0x78  | x |
|`YAS_K_y`         | 0x79  | y |
|`YAS_K_z`         | 0x7a  | z |
|`YAS_K_OCUR`      | 0x7b  | { |
|`YAS_K_PIPE`      | 0x7c  | \| |
|`YAS_K_CCUR`      | 0x7d  | } |
|`YAS_K_TLD`       | 0x7e  | Tilde |
|`YAS_K_C_8`       | 0x7f  | Backspace; see below; depends on terminal see `YAS_K_C_H` |
|`YAS_K_BSP`       | 0x7f  | Backspace; see below; depends on terminal see `YAS_K_C_H` |

- Extended keys, parsed from ANSI sequences

| Name             | Value | Description           |
|------------------|------:|-----------------------|
| `YAS_K_F1`       | 0x100 | F1 |
| `YAS_K_F2`       | 0x101 | F2 |
| `YAS_K_F3`       | 0x102 | F3 |
| `YAS_K_F4`       | 0x103 | F4 |
| `YAS_K_F5`       | 0x104 | F5 |
| `YAS_K_F6`       | 0x105 | F6 |
| `YAS_K_F7`       | 0x106 | F7 |
| `YAS_K_F8`       | 0x107 | F8 |
| `YAS_K_F9`       | 0x108 | F9 |
| `YAS_K_F10`      | 0x109 | F10 |
| `YAS_K_F11`      | 0x10a | F11 |
| `YAS_K_F12`      | 0x10b | F12 |
| `YAS_K_S_F1`     | 0x10c | Shift-F1 |
| `YAS_K_S_F2`     | 0x10d | Shift-F2 |
| `YAS_K_S_F3`     | 0x10e | Shift-F3 |
| `YAS_K_S_F4`     | 0x10f | Shift-F4 |
| `YAS_K_S_F5`     | 0x110 | Shift-F5 |
| `YAS_K_S_F6`     | 0x111 | Shift-F6 |
| `YAS_K_S_F7`     | 0x112 | Shift-F7 |
| `YAS_K_S_F8`     | 0x113 | Shift-F8 |
| `YAS_K_S_F9`     | 0x114 | Shift-F9 |
| `YAS_K_S_F10`    | 0x115 | Shift-F10 |
| `YAS_K_S_F11`    | 0x116 | Shift-F11 |
| `YAS_K_S_F12`    | 0x117 | Shift-F12 |
| `YAS_K_C_F1`     | 0x118 | Ctrl-F1 |
| `YAS_K_C_F2`     | 0x119 | Ctrl-F2 |
| `YAS_K_C_F3`     | 0x11a | Ctrl-F3 |
| `YAS_K_C_F4`     | 0x11b | Ctrl-F4 |
| `YAS_K_C_F5`     | 0x11c | Ctrl-F5 |
| `YAS_K_C_F6`     | 0x11d | Ctrl-F6 |
| `YAS_K_C_F7`     | 0x11e | Ctrl-F7 |
| `YAS_K_C_F8`     | 0x11f | Ctrl-F8 |
| `YAS_K_C_F9`     | 0x120 | Ctrl-F9 |
| `YAS_K_C_F10`    | 0x121 | Ctrl-F10 |
| `YAS_K_C_F11`    | 0x122 | Ctrl-F11 |
| `YAS_K_C_F12`    | 0x123 | Ctrl-F12 |
| `YAS_K_A_F1`     | 0x124 | Alt-F1 |
| `YAS_K_A_F2`     | 0x125 | Alt-F2 |
| `YAS_K_A_F3`     | 0x126 | Alt-F3 |
| `YAS_K_A_F4`     | 0x127 | Alt-F4 |
| `YAS_K_A_F5`     | 0x128 | Alt-F5 |
| `YAS_K_A_F6`     | 0x129 | Alt-F6 |
| `YAS_K_A_F7`     | 0x12a | Alt-F7 |
| `YAS_K_A_F8`     | 0x12b | Alt-F8 |
| `YAS_K_A_F9`     | 0x12c | Alt-F9 |
| `YAS_K_A_F10`    | 0x12d | Alt-F10 |
| `YAS_K_A_F11`    | 0x12e | Alt-F11 |
| `YAS_K_A_F12`    | 0x12f | Alt-F12 |
| `YAS_K_LEFT`     | 0x130 | Left |
| `YAS_K_UP`       | 0x131 | Up |
| `YAS_K_DOWN`     | 0x132 | Down |
| `YAS_K_RIGHT`    | 0x133 | Right |
| `YAS_K_HOME`     | 0x134 | Home |
| `YAS_K_END`      | 0x135 | End |
| `YAS_K_PGUP`     | 0x136 | PageUp |
| `YAS_K_PGDN`     | 0x137 | PageDown |
| `YAS_K_INS`      | 0x138 | Insert |
| `YAS_K_DEL`      | 0x139 | Delete |
| `YAS_K_C_LEFT`   | 0x13a | Ctrl-Left |
| `YAS_K_C_UP`     | 0x13b | Ctrl-Up |
| `YAS_K_C_DOWN`   | 0x13c | Ctrl-Down |
| `YAS_K_C_RIGHT`  | 0x13d | Ctrl-Right |

- Alt-<letter>

These codes are generated by a helper macro - `YAS_K_ALT(keycode)`.

| Name             | Description           |
|------------------|-----------------------|
| `YAS_K_A_BT`     | Alt-Backtick |
| `YAS_K_A_1`      | Alt-1 |
| `YAS_K_A_2`      | Alt-2 |
| `YAS_K_A_3`      | Alt-3 |
| `YAS_K_A_4`      | Alt-4 |
| `YAS_K_A_5`      | Alt-5 |
| `YAS_K_A_6`      | Alt-6 |
| `YAS_K_A_7`      | Alt-7 |
| `YAS_K_A_8`      | Alt-8 |
| `YAS_K_A_9`      | Alt-9 |
| `YAS_K_A_0`      | Alt-0 |
| `YAS_K_A_MINUS`  | Alt-Minus |
| `YAS_K_A_EQ`     | Alt-= |
| `YAS_K_A_BSP`    | Alt-Backspace |
| `YAS_K_A_TLD`    | Alt-Tilde |
| `YAS_K_A_EXCL`   | Alt-! |
| `YAS_K_A_AT`     | Alt-@ |
| `YAS_K_A_HASH`   | Alt-# |
| `YAS_K_A_POUND`  | Alt-$ |
| `YAS_K_A_PERC`   | Alt-% |
| `YAS_K_A_CARRET` | Alt-^ |
| `YAS_K_A_AND`    | Alt-Ampersand |
| `YAS_K_A_STAR`   | Alt-\* |
| `YAS_K_A_OBRA`   | Alt-( |
| `YAS_K_A_CBRA`   | Alt-) |
| `YAS_K_A_UND`    | Alt-_ |
| `YAS_K_A_PLUS`   | Alt-+ |
| `YAS_K_A_a`      | Alt-a |
| `YAS_K_A_b`      | Alt-b |
| `YAS_K_A_c`      | Alt-c |
| `YAS_K_A_d`      | Alt-d |
| `YAS_K_A_e`      | Alt-e |
| `YAS_K_A_f`      | Alt-f |
| `YAS_K_A_g`      | Alt-g |
| `YAS_K_A_h`      | Alt-h |
| `YAS_K_A_i`      | Alt-i |
| `YAS_K_A_j`      | Alt-j |
| `YAS_K_A_k`      | Alt-k |
| `YAS_K_A_l`      | Alt-l |
| `YAS_K_A_m`      | Alt-m |
| `YAS_K_A_n`      | Alt-n |
| `YAS_K_A_o`      | Alt-o |
| `YAS_K_A_p`      | Alt-p |
| `YAS_K_A_q`      | Alt-q |
| `YAS_K_A_r`      | Alt-r |
| `YAS_K_A_s`      | Alt-s |
| `YAS_K_A_t`      | Alt-t |
| `YAS_K_A_u`      | Alt-u |
| `YAS_K_A_v`      | Alt-v |
| `YAS_K_A_w`      | Alt-w |
| `YAS_K_A_x`      | Alt-x |
| `YAS_K_A_y`      | Alt-y |
| `YAS_K_A_z`      | Alt-z |


Functions
---------

All functions in the API work with a pointer to an opaque `yascreen` structure.

The structure is allocated internally in the library by `yascreen_init` and it is the job of the user program to keep track of it.

The library is thread safe, as long as each `struct yascreen` object is accessed by a single thread.



### `inline yascreen *yascreen_init(int sx,int sy);`

allocate and initialize screen data
output defaults to stdout
in case output is a terminal and initial size is (0,0), the screen size is autodetected

in case of error, returns `NULL`

### `inline const char *yascreen_ver(void);`

returns a string with the library version

### `inline int yascreen_setout(yascreen *s,ssize_t (*out)(yascreen *s,const void *data,size_t len));`

set callback that handles output
if out=NULL, the output goes to `stdout`

the callback may implement internal buffering, a flush is signalled by calling `out` with len=0

### `inline void yascreen_set_telnet(yascreen *s,int on);`

enable (on is non-zero) or disable (on=0) telnet sequence processing

### `inline void yascreen_init_telnet(yascreen *s);`

depending on telnet sequence processing, sends a set of telnet initialization sequences

### `inline int yascreen_resize(yascreen *s,int sx,int sy);`

resize screen
should redraw afterwards
since allocation is involved, this may fail and return -1

### `inline void yascreen_free(yascreen *s);`

finish the lifecycle of `struct yascreen` - all internally allocated memory is freed

### `inline void yascreen_term_save(yascreen *s);`

save current terminal state on top of state stack

### `inline void yascreen_term_restore(yascreen *s);`

restore previously saved terminal state from top of state stack

### `inline void yascreen_term_push(yascreen *s);`

push current terminal state to state stack

### `inline void yascreen_term_pop(yascreen *s);`

pop and restore previously saved terminal state from state stack

### `inline void yascreen_term_set(yascreen *s,int mode);`

set terminal for proper screen operation

### `mode` is a bitmask, containing one of

| Name             | Value | Description           |
|------------------|------:|-----------------------|
| `YAS_NOBUFF`     |     1 | turn off canonical mode (disable `ICANON` and `IEXTEN`) |
| `YAS_NOSIGN`     |     2 | disable `ISIG` |
| `YAS_NOECHO`     |     4 | disable local echo (`ECHO`) |
| `YAS_ONLCR`      |     8 | `ONLCR`\|`OPOST` |

### `inline int yascreen_printxy(yascreen *s,int x,int y,uint32_t attr,const char *format,...) __attribute__((format(printf,5,6)));`
### `inline int yascreen_putsxy(yascreen *s,int x,int y,uint32_t attr,const char *str);`

print at position, if data exceeds buffer, then it gets truncated

### `inline int yascreen_printxyu(yascreen *s,int x,int y,uint32_t attr,const char *format,...) __attribute__((format(printf,5,6)));`
### `inline int yascreen_putsxyu(yascreen *s,int x,int y,uint32_t attr,const char *str);`

print at position, if data exceeds buffer, then it gets truncated
screen is immediately updated

### `inline int yascreen_update(yascreen *s);`

sync memory state to screen
since allocation is involved, this may fail and return -1

### `inline void yascreen_redraw(yascreen *s);`

set next update to be a full redraw

### `inline void yascreen_clear_mem(yascreen *s,uint32_t attr);`

clear memory buffer
all cells in the screen are set to `Space`, using `attr` for colors and style

### `inline void yascreen_cursor(yascreen *s,int on);`

hide (`on`=0) or show (`on` is non-zero) cusror
screen is updated immediately

### `inline void yascreen_cursor_xy(yascreen *s,int x,int y);`

set cursor position
screen is updated immediately

### `inline void yascreen_altbuf(yascreen *s,int on);`

switch between regular and alternative buffer
screen is updated immediately

### `inline void yascreen_clear(yascreen *s);`

clear real screen, no change to memory buffers

### `inline void yascreen_clearln(yascreen *s);`

clear current line, no change to memory buffers

### `inline void yascreen_update_attr(yascreen *s,uint32_t oattr,uint32_t nattr);`

apply difference between two attrs and output the optimized ANSI sequence to switch from `oattr` to `nattr`
if `oattr`=0xffffffff, the full ANSI sequence will be generated
no change to memory buffers

### `yascreen_set_attr(s,attr)`

reset all attrs and set specific one (`attr`)

### `inline int yascreen_print(yascreen *s,const char *format,...) __attribute__((format(printf,2,3)));`
### `inline int yascreen_write(yascreen *s,const char *str,int len);`
### `inline int yascreen_puts(yascreen *s,const char *str);`
### `inline const char *yascreen_clearln_s(yascreen *s);`

print in line mode

### `inline int yascreen_sx(yascreen *s);`

get current x size

### `inline int yascreen_sy(yascreen *s);`

get current y size

### `inline int yascreen_x(yascreen *s);`

get current x

### `inline int yascreen_y(yascreen *s);`

get current y

### `inline void yascreen_esc_to(yascreen *s,int timeout);`

set timeout for single ESC key press

### `inline void yascreen_ckto(yascreen *s);`

in case of external event loop, this call will check for single ESC key
should be called regularly enough so that the above specified timeout is not extended too much
if not called often enough then single ESC will be yielded after longer timeout
if not called at all then single ESC will be yielded with next key press

### `inline int yascreen_getch_to(yascreen *s,int timeout);`

wait for a key, return ASCII or extended keycode, wait no more than timeout in milliseconds

### `yascreen_getch(s)`

get a key without timeout
this calls `yascreen_getch_to(s,0)`
zero timeout=wait forever

### `yascreen_getch_nowait(s)`

get a key, if available, return immediately
this calls `yascreen_getch_to(s,-1)`
negative timeout=do not wait

### `inline void yascreen_ungetch(yascreen *s,int key);`

put back key value in key buffer
the internal key buffer is dynamically allocated, hence there is no limit of how many key codes may be put back, but in case of memory allocation failure, the error will not be reported and the key will not be put into the buffer

### `inline void yascreen_pushch(yascreen *s,int key);`

push key value at end of key buffer
similar to `yascreen_ungetch` but the `key` code will be returned after all other key codes currently in the buffer
the internal key buffer is dynamically allocated, hence there is no limit of how many key codes may be put back, but in case of memory allocation failure, the error will not be reported and the key will not be put into the buffer

### `inline void yascreen_feed(yascreen *s,unsigned char c);`

feed key sequence state machine with byte stream
this is useful to implement external event loop and read key codes by `yascreen_getch_nowait` until it returns -1

### `inline int yascreen_peekch(yascreen *s);`

peek for key without removing it from input queue

### `inline void yascreen_getsize(yascreen *s,int *sx,int *sy);`

get last reported screen size
set both to 0 if there is none
this will yield valid result after `YAS_SCREEN_SIZE` is returned as keypress

### `inline void yascreen_reqsize(yascreen *s);`

request terminal to report its size via ANSI sequence

### `inline void yascreen_set_hint_i(yascreen *s,int hint);`
### `inline int yascreen_get_hint_i(yascreen *s);`
### `inline void yascreen_set_hint_p(yascreen *s,void *hint);`
### `inline void *yascreen_get_hint_p(yascreen *s);`

get/set opaque hint values
integer and pointer hints are stored separately and both can be used at the same time

these are useful to link the `yascreen` instance to user program data

for example a single output callback may output to socket or a terminal, depending on the hint values
