// $Id: yascreen.h,v 1.45 2022/12/19 04:17:19 bbonev Exp $
//
// Copyright © 2015-2020 Boian Bonev (bbonev@ipacct.com) {{{
//
// SPDX-License-Identifer: LGPL-3.0-or-later
//
// This file is part of yascreen - yet another screen library.
//
// yascreen is free software, released under the terms of GNU Lesser General Public License v3.0 or later
// }}}

#ifndef ___YASCREEN_H___
#define ___YASCREEN_H___

#include <unistd.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// extract colors from combined style attribute
#define YAS_FG(s) ((s)&0x1ff)
#define YAS_BG(s) (((s)>>9)&0x1ff)
// bit masks for different styles
#define YAS_ITALIC (1u<<(2*9+0))
#define YAS_UNDERL (1u<<(2*9+1))
#define YAS_STRIKE (1u<<(2*9+2))
#define YAS_INVERSE (1u<<(2*9+3))
#define YAS_BOLD (1u<<(2*9+4))
#define YAS_BLINK (1u<<(2*9+5))
// values representing default terminal's colors
#define YAS_FGCOLORDEF 0
#define YAS_BGCOLORDEF 0
// construnct simple colors to be ored into attribute
#define YAS_FGCOLOR(c) ((((c)&0x7)|8)<<0)
#define YAS_BGCOLOR(c) ((((c)&0x7)|8)<<9)
// construnct 256 extended colors to be ored into attribute
#define YAS_FGXCOLOR(c) ((((c)&0xff)|0x100)<<0)
#define YAS_BGXCOLOR(c) ((((c)&0xff)|0x100)<<9)

// simple color table
#define YAS_BLACK 0
#define YAS_RED 1
#define YAS_GREEN 2
#define YAS_YELLOW 3
#define YAS_BLUE 4
#define YAS_MAGENTA 5
#define YAS_CYAN 6
#define YAS_WHITE 7

// input modes
#define YAS_NOBUFF 1
#define YAS_NOSIGN 2
#define YAS_NOECHO 4
#define YAS_ONLCR 8

#define YAS_K_ALT(code) (((code)&0xff)+0xf0101)

#define YAS_IS_CC(code) ((code)>=0xf0000&&(code)<=0xffffd)

// key codes
// first part are 0x00-0x7f and these map directly to ASCII characters, also 1:1 in wchar_t
// second part are also directly representable in wchar_t, in the 0xf0000-0xffffd reserved range
typedef enum {
	YAS_K_NONE=0xf0000, // no key available
	YAS_K_NUL=0x00,
	YAS_K_C_A=0x01,
	YAS_K_C_B=0x02,
	YAS_K_C_C=0x03,
	YAS_K_C_D=0x04,
	YAS_K_C_E=0x05,
	YAS_K_C_F=0x06,
	YAS_K_C_G=0x07,
	YAS_K_C_H=0x08,
	YAS_K_C_I=0x09,
	YAS_K_TAB=0x09,
	YAS_K_C_J=0x0a,
	YAS_K_C_K=0x0b,
	YAS_K_C_L=0x0c,
	YAS_K_C_M=0x0d,
	YAS_K_RET=0x0d,
	YAS_K_C_N=0x0e,
	YAS_K_C_O=0x0f,
	YAS_K_C_P=0x10,
	YAS_K_C_Q=0x11,
	YAS_K_C_R=0x12,
	YAS_K_C_S=0x13,
	YAS_K_C_T=0x14,
	YAS_K_C_U=0x15,
	YAS_K_C_V=0x16,
	YAS_K_C_W=0x17,
	YAS_K_C_X=0x18,
	YAS_K_C_Y=0x19,
	YAS_K_C_Z=0x1a,
	YAS_K_ESC=0x1b,
	YAS_K_C_3=0x1b,
	YAS_K_C_4=0x1c,
	YAS_K_C_5=0x1d,
	YAS_K_C_6=0x1e,
	YAS_K_C_7=0x1f,
	YAS_K_SPACE=0x20, // ' '
	YAS_K_EXCL=0x21, // '!'
	YAS_K_DQUOT=0x22, // '"'
	YAS_K_HASH=0x23, // '#'
	YAS_K_POUND=0x24, // '$'
	YAS_K_PERC=0x25, // '%'
	YAS_K_AND=0x26, // '&'
	YAS_K_QUOT=0x27, // '\''
	YAS_K_OBRA=0x28, // '('
	YAS_K_CBRA=0x29, // ')'
	YAS_K_STAR=0x2a, // '*'
	YAS_K_PLUS=0x2b, // '+'
	YAS_K_COMMA=0x2c, // ','
	YAS_K_MINUS=0x2d, // '-'
	YAS_K_DOT=0x2e, // '.'
	YAS_K_SLASH=0x2f, // '/'
	YAS_K_0=0x30, // '0'
	YAS_K_1=0x31, // '1'
	YAS_K_2=0x32, // '2'
	YAS_K_3=0x33, // '3'
	YAS_K_4=0x34, // '4'
	YAS_K_5=0x35, // '5'
	YAS_K_6=0x36, // '6'
	YAS_K_7=0x37, // '7'
	YAS_K_8=0x38, // '8'
	YAS_K_9=0x39, // '9'
	YAS_K_COLON=0x3a, // ':'
	YAS_K_SEMI=0x3b, // ';'
	YAS_K_LT=0x3c, // '<'
	YAS_K_EQ=0x3d, // '='
	YAS_K_GT=0x3e, // '>'
	YAS_K_QUEST=0x3f, // '?'
	YAS_K_AT=0x40, // '@'
	YAS_K_A=0x41, // 'A'
	YAS_K_B=0x42, // 'B'
	YAS_K_C=0x43, // 'C'
	YAS_K_D=0x44, // 'D'
	YAS_K_E=0x45, // 'E'
	YAS_K_F=0x46, // 'F'
	YAS_K_G=0x47, // 'G'
	YAS_K_H=0x48, // 'H'
	YAS_K_I=0x49, // 'I'
	YAS_K_J=0x4a, // 'J'
	YAS_K_K=0x4b, // 'K'
	YAS_K_L=0x4c, // 'L'
	YAS_K_M=0x4d, // 'M'
	YAS_K_N=0x4e, // 'N'
	YAS_K_O=0x4f, // 'O'
	YAS_K_P=0x50, // 'P'
	YAS_K_Q=0x51, // 'Q'
	YAS_K_R=0x52, // 'R'
	YAS_K_S=0x53, // 'S'
	YAS_K_T=0x54, // 'T'
	YAS_K_U=0x55, // 'U'
	YAS_K_V=0x56, // 'V'
	YAS_K_W=0x57, // 'W'
	YAS_K_X=0x58, // 'X'
	YAS_K_Y=0x59, // 'Y'
	YAS_K_Z=0x5a, // 'Z'
	YAS_K_OSQ=0x5b, // '['
	YAS_K_BSLASH=0x5c, // '\\'
	YAS_K_CSQ=0x5d, // ']'
	YAS_K_CARRET=0x5e, // '^'
	YAS_K_USCORE=0x5f, // '_'
	YAS_K_BTICK=0x60, // '`'
	YAS_K_a=0x61, // 'a'
	YAS_K_b=0x62, // 'b'
	YAS_K_c=0x63, // 'c'
	YAS_K_d=0x64, // 'd'
	YAS_K_e=0x65, // 'e'
	YAS_K_f=0x66, // 'f'
	YAS_K_g=0x67, // 'g'
	YAS_K_h=0x68, // 'h'
	YAS_K_i=0x69, // 'i'
	YAS_K_j=0x6a, // 'j'
	YAS_K_k=0x6b, // 'k'
	YAS_K_l=0x6c, // 'l'
	YAS_K_m=0x6d, // 'm'
	YAS_K_n=0x6e, // 'n'
	YAS_K_o=0x6f, // 'o'
	YAS_K_p=0x70, // 'p'
	YAS_K_q=0x71, // 'q'
	YAS_K_r=0x72, // 'r'
	YAS_K_s=0x73, // 's'
	YAS_K_t=0x74, // 't'
	YAS_K_u=0x75, // 'u'
	YAS_K_v=0x76, // 'v'
	YAS_K_w=0x77, // 'w'
	YAS_K_x=0x78, // 'x'
	YAS_K_y=0x79, // 'y'
	YAS_K_z=0x7a, // 'z'
	YAS_K_OCUR=0x7b, // '{'
	YAS_K_PIPE=0x7c, // '|'
	YAS_K_CCUR=0x7d, // '}'
	YAS_K_TLD=0x7e, // '~'
	YAS_K_C_8=0x7f,
	YAS_K_BSP=0x7f,
	// extended keys, send as escape sequences
	// function keys with ALT/CTRL/SHIFT
	YAS_K_F1=0xf0001,
	YAS_K_F2=0xf0002,
	YAS_K_F3=0xf0003,
	YAS_K_F4=0xf0004,
	YAS_K_F5=0xf0005,
	YAS_K_F6=0xf0006,
	YAS_K_F7=0xf0007,
	YAS_K_F8=0xf0008,
	YAS_K_F9=0xf0009,
	YAS_K_F10=0xf000a,
	YAS_K_F11=0xf000b,
	YAS_K_F12=0xf000c,
	YAS_K_S_F1=0xf000d,
	YAS_K_S_F2=0xf000e,
	YAS_K_S_F3=0xf000f,
	YAS_K_S_F4=0xf0010,
	YAS_K_S_F5=0xf0011,
	YAS_K_S_F6=0xf0012,
	YAS_K_S_F7=0xf0013,
	YAS_K_S_F8=0xf0014,
	YAS_K_S_F9=0xf0015,
	YAS_K_S_F10=0xf0016,
	YAS_K_S_F11=0xf0017,
	YAS_K_S_F12=0xf0018,
	YAS_K_C_F1=0xf0019,
	YAS_K_C_F2=0xf001a,
	YAS_K_C_F3=0xf001b,
	YAS_K_C_F4=0xf001c,
	YAS_K_C_F5=0xf001d,
	YAS_K_C_F6=0xf001e,
	YAS_K_C_F7=0xf001f,
	YAS_K_C_F8=0xf0020,
	YAS_K_C_F9=0xf0021,
	YAS_K_C_F10=0xf0022,
	YAS_K_C_F11=0xf0023,
	YAS_K_C_F12=0xf0024,
	YAS_K_A_F1=0xf0025,
	YAS_K_A_F2=0xf0026,
	YAS_K_A_F3=0xf0027,
	YAS_K_A_F4=0xf0028,
	YAS_K_A_F5=0xf0029,
	YAS_K_A_F6=0xf002a,
	YAS_K_A_F7=0xf002b,
	YAS_K_A_F8=0xf002c,
	YAS_K_A_F9=0xf002d,
	YAS_K_A_F10=0xf002e,
	YAS_K_A_F11=0xf002f,
	YAS_K_A_F12=0xf0030,
	YAS_K_LEFT=0xf0031,
	YAS_K_UP=0xf0032,
	YAS_K_DOWN=0xf0033,
	YAS_K_RIGHT=0xf0034,
	YAS_K_HOME=0xf0035,
	YAS_K_END=0xf0036,
	YAS_K_PGUP=0xf0037,
	YAS_K_PGDN=0xf0038,
	YAS_K_INS=0xf0039,
	YAS_K_DEL=0xf003a,
	YAS_K_C_LEFT=0xf003b,
	YAS_K_C_UP=0xf003c,
	YAS_K_C_DOWN=0xf003d,
	YAS_K_C_RIGHT=0xf003e,
	YAS_K_S_LEFT=0xf003f,
	YAS_K_S_UP=0xf0040,
	YAS_K_S_DOWN=0xf0041,
	YAS_K_S_RIGHT=0xf0042,
	// ALT+letter
	YAS_K_A_BT=YAS_K_ALT('`'),
	YAS_K_A_1=YAS_K_ALT('1'),
	YAS_K_A_2=YAS_K_ALT('2'),
	YAS_K_A_3=YAS_K_ALT('3'),
	YAS_K_A_4=YAS_K_ALT('4'),
	YAS_K_A_5=YAS_K_ALT('5'),
	YAS_K_A_6=YAS_K_ALT('6'),
	YAS_K_A_7=YAS_K_ALT('7'),
	YAS_K_A_8=YAS_K_ALT('8'),
	YAS_K_A_9=YAS_K_ALT('9'),
	YAS_K_A_0=YAS_K_ALT('0'),
	YAS_K_A_MINUS=YAS_K_ALT('-'),
	YAS_K_A_EQ=YAS_K_ALT('='),
	YAS_K_A_BSP=YAS_K_ALT(0x7f),
	YAS_K_A_TLD=YAS_K_ALT('~'),
	YAS_K_A_EXCL=YAS_K_ALT('!'),
	YAS_K_A_AT=YAS_K_ALT('@'),
	YAS_K_A_HASH=YAS_K_ALT('#'),
	YAS_K_A_POUND=YAS_K_ALT('$'),
	YAS_K_A_PERC=YAS_K_ALT('%'),
	YAS_K_A_CARRET=YAS_K_ALT('^'),
	YAS_K_A_AND=YAS_K_ALT('&'),
	YAS_K_A_STAR=YAS_K_ALT('*'),
	YAS_K_A_OBRA=YAS_K_ALT('('),
	YAS_K_A_CBRA=YAS_K_ALT(')'),
	YAS_K_A_UND=YAS_K_ALT('_'),
	YAS_K_A_PLUS=YAS_K_ALT('+'),
	YAS_K_A_a=YAS_K_ALT('a'),
	YAS_K_A_b=YAS_K_ALT('b'),
	YAS_K_A_c=YAS_K_ALT('c'),
	YAS_K_A_d=YAS_K_ALT('d'),
	YAS_K_A_e=YAS_K_ALT('e'),
	YAS_K_A_f=YAS_K_ALT('f'),
	YAS_K_A_g=YAS_K_ALT('g'),
	YAS_K_A_h=YAS_K_ALT('h'),
	YAS_K_A_i=YAS_K_ALT('i'),
	YAS_K_A_j=YAS_K_ALT('j'),
	YAS_K_A_k=YAS_K_ALT('k'),
	YAS_K_A_l=YAS_K_ALT('l'),
	YAS_K_A_m=YAS_K_ALT('m'),
	YAS_K_A_n=YAS_K_ALT('n'),
	YAS_K_A_o=YAS_K_ALT('o'),
	YAS_K_A_p=YAS_K_ALT('p'),
	YAS_K_A_q=YAS_K_ALT('q'),
	YAS_K_A_r=YAS_K_ALT('r'),
	YAS_K_A_s=YAS_K_ALT('s'),
	YAS_K_A_t=YAS_K_ALT('t'),
	YAS_K_A_u=YAS_K_ALT('u'),
	YAS_K_A_v=YAS_K_ALT('v'),
	YAS_K_A_w=YAS_K_ALT('w'),
	YAS_K_A_x=YAS_K_ALT('x'),
	YAS_K_A_y=YAS_K_ALT('y'),
	YAS_K_A_z=YAS_K_ALT('z'),
	YAS_SCREEN_SIZE=0xf0701,
	YAS_TELNET_SIZE=0xf0702,
} yas_keys;

struct _yascreen;
typedef struct _yascreen yascreen;

// allocate and initialize screen data
// output defaults to stdout
inline yascreen *yascreen_init(int sx,int sy);
// get library version as static string
inline const char *yascreen_ver(void);
// change output; if output is NULL, default is to stdout
inline int yascreen_setout(yascreen *s,ssize_t (*out)(yascreen *s,const void *data,size_t len));
// enable/disable handling of telnet protocol (disabled by default)
inline void yascreen_set_telnet(yascreen *s,int on);
// enable/disable handling of unicode input (enabled by default)
inline void yascreen_set_unicode(yascreen *s,int on);
// init remote telnet client
inline void yascreen_init_telnet(yascreen *s);
// resize screen; should redraw afterwards
// since allocation is involved, this may fail and return -1
inline int yascreen_resize(yascreen *s,int sx,int sy);
// free screen data
inline void yascreen_free(yascreen *s);

// save current terminal state on top of state stack
inline void yascreen_term_save(yascreen *s);
// restore previously saved terminal state from top of state stack
inline void yascreen_term_restore(yascreen *s);
// push current terminal state to state stack
inline void yascreen_term_push(yascreen *s);
// pop and restore previously saved terminal state from state stack
inline void yascreen_term_pop(yascreen *s);
// set terminal for proper screen operation
inline void yascreen_term_set(yascreen *s,int mode);

// print at position, if data exceeds buffer, then it gets truncated
inline int yascreen_printxy(yascreen *s,int x,int y,uint32_t attr,const char *format,...) __attribute__((format(printf,5,6)));
inline int yascreen_putsxy(yascreen *s,int x,int y,uint32_t attr,const char *str);
// print at position, if data exceeds buffer, then it gets truncated; and update immediately
inline int yascreen_printxyu(yascreen *s,int x,int y,uint32_t attr,const char *format,...) __attribute__((format(printf,5,6)));
inline int yascreen_putsxyu(yascreen *s,int x,int y,uint32_t attr,const char *str);

// sync memory state to screen
// since allocation is involved, this may fail and return -1
inline int yascreen_update(yascreen *s);
// set next update to be a full redraw
inline void yascreen_redraw(yascreen *s);
// clear memory buffer
inline void yascreen_clear_mem(yascreen *s,uint32_t attr);

// calls suitable for line mode and init of fullscreen mode

// hide or show cusror; screen is updated immediately
inline void yascreen_cursor(yascreen *s,int on);
// set cursor position; screen is updated immediately
inline void yascreen_cursor_xy(yascreen *s,int x,int y);
// switch between regular and alternative buffer; screen is updated immediately
inline void yascreen_altbuf(yascreen *s,int on);
// clear real screen, no change to memory buffers
inline void yascreen_clear(yascreen *s);
// clear current line, no change to memory buffers
inline void yascreen_clearln(yascreen *s);
// apply difference between two attrs
inline void yascreen_update_attr(yascreen *s,uint32_t oattr,uint32_t nattr);
// reset all attrs and set specific one
#define yascreen_set_attr(s,attr) yascreen_update_attr(s,0xffffffff,attr)
// print in line mode
inline int yascreen_print(yascreen *s,const char *format,...) __attribute__((format(printf,2,3)));
inline int yascreen_write(yascreen *s,const char *str,int len);
inline int yascreen_puts(yascreen *s,const char *str);
// set if the above three calls should flush
inline void yascreen_line_flush(yascreen *s,int on);
// returns an escape sequence to clear line
inline const char *yascreen_clearln_s(yascreen *s);

// get current x size
inline int yascreen_sx(yascreen *s);
// get current y size
inline int yascreen_sy(yascreen *s);
// get current x
inline int yascreen_x(yascreen *s);
// get current y
inline int yascreen_y(yascreen *s);

// keyboard input
// set timeout for single ESC key press
inline void yascreen_esc_to(yascreen *s,int timeout);
// in case of external event loop, this call will check for single ESC key
// should be called regularly enough so that the above specified timeout is not extended too much
// if not called often enough then single ESC will be yielded after longer timeout
// if not called at all then single ESC will be yielded with next key press
inline void yascreen_ckto(yascreen *s);
// wait for a key, return ASCII or extended keycode, wait no more than timeout in milliseconds
inline int yascreen_getch_to(yascreen *s,int timeout);
// zero timeout=wait forever
#define yascreen_getch(s) yascreen_getch_to(s,0)
// negative timeout=do not wait
#define yascreen_getch_nowait(s) yascreen_getch_to(s,-1)
// put back key value in key buffer
inline void yascreen_ungetch(yascreen *s,int key);
// push key value at end of key buffer
inline void yascreen_pushch(yascreen *s,int key);
// feed key sequence state machine with byte stream
// this is useful to implement external event loop and
// read key codes by yascreen_getch_nowait until it returns -1
inline void yascreen_feed(yascreen *s,unsigned char c);
// peek for key without removing it from input queue
inline int yascreen_peekch(yascreen *s);
// get last reported screen size; set both to 0 if there is none
// this will yield valid result after YAS_SCREEN_SIZE is returned as keypress
inline void yascreen_getsize(yascreen *s,int *sx,int *sy);
// request terminal to report its size
inline void yascreen_reqsize(yascreen *s);

// wchar_t input
// wait for a key, return wchar_t or extended keycode in 0xf0000-0xffffd range
// wait no more than timeout in milliseconds
inline wchar_t yascreen_getwch_to(yascreen *s,int timeout);
// zero timeout=wait forever
#define yascreen_getwch(s) yascreen_getwch_to(s,0)
// negative timeout=do not wait
#define yascreen_getwch_nowait(s) yascreen_getwch_to(s,-1)
// put back key value in key buffer
inline void yascreen_ungetwch(yascreen *s,wchar_t key);
// peek for key without removing it from input queue
inline wchar_t yascreen_peekwch(yascreen *s);

// hints api
inline void yascreen_set_hint_i(yascreen *s,int hint);
inline int yascreen_get_hint_i(yascreen *s);
inline void yascreen_set_hint_p(yascreen *s,void *hint);
inline void *yascreen_get_hint_p(yascreen *s);

#ifdef __cplusplus
}
#endif

#endif
