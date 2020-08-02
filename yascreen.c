// $Id: yascreen.c,v 1.79 2020/08/02 02:01:25 bbonev Exp $
//
// Copyright © 2015-2020 Boian Bonev (bbonev@ipacct.com) {{{
//
// This file is part of yascreen - yet another screen library.
//
// yascreen is free software: you can redistribute it and/or mowdify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// yascreen is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with yascreen.  If not, see <http://www.gnu.org/licenses/>.
// }}}

// {{{ includes

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <time.h>
#include <wchar.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <yascreen.h>

// }}}

// {{{ definitions

#define mymax(a,b) (((a)>(b))?(a):(b))
#define mymin(a,b) (((a)<(b))?(a):(b))

#define ESC "\x1b"

// size of string that can be stored immediately w/o allocation
#define PSIZE (sizeof(char *))
// step to allocate key buffer
#define KEYSTEP (4096/sizeof(int))
// default timeout before escape is returned
#define YAS_DEFAULT_ESCTO 300

// check if a given value is a valid simple color value
#define YAS_ISCOLOR(c) ((c)>=8&&(c)<=15)
// check if a given value is a valid extended color value
#define YAS_ISXCOLOR(c) ((c)&0x100)

#define YAS_STORAGE 0x80000000 // data is longer than PSIZE and is stored in allocated memory
#define YAS_TOUCHED 0x40000000 // there are changes in this line, update cannot skip it
#define YAS_INTERNAL (YAS_STORAGE|YAS_TOUCHED)

#define TELNET_EOSN 240 // 0xf0 // end of subnegotiation
#define TELNET_NOP 241 // 0xf1 // NOP
#define TELNET_SYNCH 242 // 0xf2 // SYNCH
#define TELNET_NVTBRK 243 //0xf3 // NVTBRK
#define TELNET_IP 244 // 0xf4 // IP
#define TELNET_AO 245 // 0xf5 // AO
#define TELNET_AYT 246 //0xf6 // AYT are you there
#define TELNET_EC 247 // 0xf7 // EC
#define TELNET_EL 248 // 0xf8 // EL
#define TELNET_GOA 249 // 0xf9 // go ahead
#define TELNET_SOSN 250 // 0xfa // start of subnegotiation
#define TELNET_WILL 251 // 0xfb // will
#define TELNET_WONT 252 // 0xfc // wont
#define TELNET_DO 253 // 0xfd // do
#define TELNET_DONT 254 // 0xfe // dont
#define TELNET_IAC 255 // 0xff // telnet protocol escape code (IAC)
#define TELNET_NOOP 0x100 // telnet protocol handler have eaten a byte w/o yielding any result
#define TELNET_SIZE 0x101 // telnet protocol handler have detected screen size change notification

#define TELNET_NAWS 31 // 0x1f // negotiate about window size

// data is kept as utf8, including its combining chars
// if it fits in PSZIE, it is in d, with 0 terminating char
// if the char at previous position requires 2 columns, current char should be empty
// after initialization all chars are set to ' ' (0x20)
typedef struct _cell {
	uint32_t style; // color, style and storage type
	union {
		char *p;
		char d[PSIZE];
	};
} cell;

typedef enum { // ansi sequence state machine
	ST_NORM, // normal input, check for ESC
	ST_ENTER, // eat LF/NUL after CR
	ST_ESC, // escape sequence
	ST_ESC_SQ, // escape [ sequence
	ST_ESC_SQ_D, // escape [ digit sequence
	ST_ESC_O, // escape O sequence
} yas_k_state;

typedef enum { // telnet sequence state machine
	T_NORM, // expect regular byte or IAC
	T_IAC, // telnet IAC, expect option
	T_IAC_O, // telnet IAC option, expect opt code
	T_IAC_SB, // telnet IAC extended, expect IAC
	T_IAC_SE, // telnet IAC extended, expect SE
} yas_t_state;

typedef enum { // utf8 sequence state machine
	U_NORM, // expect single byte or leading byte
	U_L2C1, // expect 1/1 continuation byte
	U_L3C1, // expect 1/2 continuation bytes
	U_L3C2, // expect 2/2 continuation bytes
	U_L4C1, // expect 1/3 continuation bytes
	U_L4C2, // expect 2/3 continuation bytes
	U_L4C3, // expect 3/3 continuation bytes
	U_L5C1, // expect 1/4 continuation bytes
	U_L5C2, // expect 2/4 continuation bytes
	U_L5C3, // expect 3/4 continuation bytes
	U_L5C4, // expect 4/4 continuation bytes
	U_L6C1, // expect 1/5 continuation bytes
	U_L6C2, // expect 2/5 continuation bytes
	U_L6C3, // expect 3/5 continuation bytes
	U_L6C4, // expect 4/5 continuation bytes
	U_L6C5, // expect 5/5 continuation bytes
} yas_u_state;

struct _yascreen {
	int sx,sy; // size of screen
	ssize_t (*outcb)(yascreen *s,const void *data,size_t len); // output callback
	cell *mem; // memory state
	cell *scr; // screen state
	struct termios *tsstack; // saved terminal state
	int tssize; // number of items in the stack
	int escto; // single ESC key timeout
	int keysize; // saved key storage size
	int keycnt; // saved key count
	int *keys; // saved key array
	unsigned char ansibuf[20]; // buffer for escape sequence parsing
	unsigned char ansipos; // next byte will go in this pos
	unsigned char sosnbuf[20]; // buffer for telnet SOSN options parsing
	unsigned char sosnpos; // next byte will go in this pos
	unsigned char utf[3]; // buffer for utf8 sequence parsing; last byte is not put here, its never zero terminated
						// must be increased to 4 or 5 if some day unicode permits 5 or 6 byte sequences
	int64_t escts; // single ESC key timestamp
	yas_k_state state; // input parser state
	yas_t_state tstate; // telnet parser state
	yas_u_state ustate; // utf8 parser state
	int cursorx; // position to place cursor on update
	int cursory; // position to place cursor on update
	int scrx; // last reported screen size
	int scry; // last reported screen size
	uint8_t haveansi:1; // we do have a reported screen size from ansi sequence
	uint8_t havenaws:1; // we do have a reported screen size from telent naws
	uint8_t istelnet:1; // do process telnet sequences
	uint8_t cursor:1; // last cursor state
	uint8_t redraw:1; // flag to redraw from scratch
	int hint; // user defined hint (scalar)
	void *phint; // user defined hint (pointer)
	uint8_t outb[256]; // buffered output
	uint16_t outp; // position in outb
};
// }}}

static inline int64_t mytime() { // {{{
	struct timespec ts;
	int64_t res;

	clock_gettime(CLOCK_MONOTONIC,&ts);
	res=ts.tv_sec*1000;
	res+=ts.tv_nsec/1000000;
	return res;
} // }}}

static inline ssize_t out(yascreen *s __attribute__((unused)),const void *buf,size_t len) { // {{{
	size_t olen=len;

repeat:
	if (len) {
		if (sizeof s->outb-s->outp>=len) {
			memcpy(s->outb+s->outp,buf,len);
			s->outp+=len;
		} else {
			size_t brem=sizeof s->outb-s->outp;
			size_t wr;

			memcpy(s->outb+s->outp,buf,brem);
			s->outp+=brem;
			buf+=brem;
			len-=brem;
			wr=write(STDOUT_FILENO,s->outb,s->outp);
			if (wr<=0) // error
				return wr;
			if (wr==s->outp) {
				s->outp-=wr;
				goto repeat;
			}
			// wr>0 and wr<s->outp
			memmove(s->outb,s->outb+wr,s->outp-wr);
			s->outp-=wr;
			goto repeat;
		}
	}
	if (!olen&&s->outp) { // flush is requested
		size_t wr=write(STDOUT_FILENO,s->outb,s->outp);

		if (wr==s->outp)
			s->outp=0;
		else
			if (wr>0) {
				memmove(s->outb,s->outb+wr,s->outp-wr);
				s->outp-=wr;
			}
	}
	return olen;
} // }}}

static inline void outs(yascreen *s,const char *str) { // {{{
	ssize_t (*o)(yascreen *s,const void *buf,size_t len);
	size_t len;

	if (!s)
		return;
	if (!str)
		return;

	len=strlen(str);
	o=s->outcb?s->outcb:out;
	while (len) {
		ssize_t r=o(s,str,len);

		if (r>=0) {
			len-=r;
			str+=r;
		} else
			break;
	}
} // }}}

static inline void outf(yascreen *s,const char *format,...) __attribute__((format(printf,2,3))); // {{{
// }}}

static inline void outf(yascreen *s,const char *format,...) { // {{{
	va_list ap;
	char *ns;
	int size;

	if (!s)
		return;
	if (!format)
		return;

	va_start(ap,format);
	size=vasprintf(&ns,format,ap);
	va_end(ap);

	if (size==-1) // some error, nothing more to do
		return;

	outs(s,ns);

	free(ns);
} // }}}

inline void yascreen_set_hint_i(yascreen *s,int hint) { // {{{
	if (!s)
		return;
	s->hint=hint;
} // }}}

inline int yascreen_get_hint_i(yascreen *s) { // {{{
	if (!s)
		return 0;
	return s->hint;
} // }}}

inline void yascreen_set_hint_p(yascreen *s,void *hint) { // {{{
	if (!s)
		return;
	s->phint=hint;
} // }}}

inline void *yascreen_get_hint_p(yascreen *s) { // {{{
	if (!s)
		return NULL;
	return s->phint;
} // }}}

static char myver[]="\0Yet another screen library (https://github.com/bbonev/yascreen) $Revision: 1.79 $\n\n"; // {{{
// }}}

inline const char *yascreen_ver(void) { // {{{
	return myver;
} // }}}

inline yascreen *yascreen_init(int sx,int sy) { // {{{
	yascreen *s;
	int i;

	if (myver[0]==0) { // reformat the static version string
		char *rev=strstr(myver+1,"$Revision: ");
		int vermaj,vermin;

		if (rev) {
			sscanf(rev+strlen("$Revision: "),"%d.%d",&vermaj,&vermin);
			vermaj+=vermin/100;
			vermin=vermin%100;
			memmove(myver,myver+1,strlen(myver+1)+1);
			snprintf(rev-1,sizeof myver-(rev-1-myver),"%d.%02d\n\n",vermaj,vermin);
		}
	}

	if (sx<0||sy<0)
		return NULL;

	s=(yascreen *)calloc(1,sizeof *s);
	if (!s)
		return NULL;

	s->state=ST_NORM;
	s->tstate=T_NORM;
	s->ustate=U_NORM;
	s->escto=YAS_DEFAULT_ESCTO;
	s->cursor=1;
	s->cursorx=0;
	s->cursory=0;
	if (!s->outcb&&isatty(STDOUT_FILENO)) { // output is a terminal
		s->tsstack=(struct termios *)calloc(1,sizeof(struct termios));
		if (!s->tsstack) {
			free(s);
			return NULL;
		}
		s->tssize=1;
		tcgetattr(STDOUT_FILENO,s->tsstack);
		if (!sx||!sy) {
			struct winsize ws={0};

			if (!ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws)) {
				if (!sx)
					sx=ws.ws_col;
				if (!sy)
					sy=ws.ws_row;
			}
		}
	}
	if (sx<=0||sy<=0) {
		if (s->tsstack)
			free(s->tsstack);
		free(s);
		return NULL;
	}
	s->sx=sx;
	s->sy=sy;

	s->keys=(int *)calloc(KEYSTEP,sizeof(int));
	if (!s->keys) {
		if (s->tsstack)
			free(s->tsstack);
		free(s);
		return NULL;
	}
	s->keysize=KEYSTEP;
	s->mem=(cell *)calloc(sx*sy,sizeof(cell));
	s->scr=(cell *)calloc(sx*sy,sizeof(cell));
	if (!s->mem||!s->scr) {
		if (s->mem)
			free(s->mem);
		if (s->scr)
			free(s->scr);
		if (s->tsstack)
			free(s->tsstack);
		if (s->keys)
			free(s->keys);
		free(s);
		return NULL;
	}

	for (i=0;i<sx*sy;i++)
		strncpy(s->mem[i].d," ",sizeof s->mem[i].d);
	// leave scr empty, so that on first refresh everything is redrawn
	s->redraw=1;
	return s;
} // }}}

inline int yascreen_setout(yascreen *s,ssize_t (*out)(yascreen *s,const void *data,size_t len)) { // {{{
	if (!s)
		return -1;

	s->outcb=out;
	s->redraw=1;
	return 0;
} // }}}

inline void yascreen_set_telnet(yascreen *s,int on) { // {{{
	if (!s)
		return;
	s->istelnet=!!on;
} // }}}

inline void yascreen_init_telnet(yascreen *s) { // {{{
	if (!s)
		return;

	if (s->istelnet)
		outs(s,
			"\xff\xfb\x03" // will(251) suppress go ahead
			"\xff\xfb\x01" // will(251) echo
			"\xff\xfd\x03" // do(253) suppress go ahead
			"\xff\xfd\x01" // do(253) echo
			"\xff\xfb\x1f" // will(251) negotiate terminal size
			"\xff\xfd\x1f" // do(253) negotiate terminal size
		);
	else
		outs(s,
			"\xff\xfc\x03" // wont(251) suppress go ahead
			"\xff\xfc\x01" // wont(251) echo
			"\xff\xfe\x03" // dont(253) suppress go ahead
			"\xff\xfe\x01" // dont(253) echo
			"\xff\xfc\x1f" // wont(251) negotiate terminal size
			"\xff\xfe\x1f" // dont(253) negotiate terminal size
		);
} // }}}

inline int yascreen_resize(yascreen *s,int sx,int sy) { // {{{
	cell *mem,*scr;
	int i;

	if (!s)
		return -1;

	if (sx<0||sy<0)
		return -1;

	if (!sx||!sy)
		if (!s->outcb&&isatty(STDOUT_FILENO)) {
			struct winsize ws={0};

			if (!ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws)) {
				if (!sx)
					sx=ws.ws_col;
				if (!sy)
					sy=ws.ws_row;
			}
		}

	if (sx<=0||sy<=0)
		return -1;

	if (s->sx==sx&&s->sy==sy)
		return 0;

	for (i=0;i<s->sx*s->sy;i++) { // free old allocated data and set for reusage
		if (s->mem[i].style&YAS_STORAGE)
			free(s->mem[i].p);
		if (s->scr[i].style&YAS_STORAGE)
			free(s->scr[i].p);
		if (i<sx*sy) {
			s->mem[i].style=s->scr[i].style=0;
			s->mem[i].d[0]=' ';
			s->scr[i].d[0]=0;
		}
	}
	if (sx*sy>s->sx*s->sy) { // allocate bigger buffer
		mem=(cell *)realloc(s->mem,sx*sy*sizeof(cell));
		if (!mem)
			return -1;
		s->mem=mem;
		scr=(cell *)realloc(s->scr,sx*sy*sizeof(cell));
		if (!scr)
			return -1;
		s->scr=scr;
		for (i=s->sx*s->sy;i<sx*sy;i++) { // initialize the rest of the area
			s->mem[i].style=s->scr[i].style=0;
			s->mem[i].d[0]=' ';
			s->scr[i].d[0]=0;
		}
	}
	s->redraw=1;
	s->sx=sx;
	s->sy=sy;

	return 0;
} // }}}

inline void yascreen_free(yascreen *s) { // {{{
	int i;

	if (!s)
		return;

	if (!s->mem||!s->scr) { // error condition that will happen only if mem is corrupt
		if (s->mem)
			free(s->mem);
		if (s->scr)
			free(s->scr);
		if (s->tsstack)
			free(s->tsstack);
		if (s->keys)
			free(s->keys);
		free(s); // most probably will crash, because there is no way to have s partally initialized
		return;
	}
	for (i=0;i<s->sx*s->sy;i++) {
		if (s->mem[i].style&YAS_STORAGE)
			free(s->mem[i].p);
		if (s->scr[i].style&YAS_STORAGE)
			free(s->scr[i].p);
	}
	free(s->mem);
	free(s->scr);
	if (s->tsstack)
		free(s->tsstack);
	if (s->keys)
		free(s->keys);
	outs(s,ESC"[0m");
	free(s);
} // }}}

inline void yascreen_update_attr(yascreen *s,uint32_t oattr,uint32_t nattr) { // {{{
	if (!s)
		return;

	if (oattr==0xffffffff) {
		oattr=~nattr; // force setting all
		outs(s,ESC"0m");
	}

	if ((oattr&YAS_BOLD)!=(nattr&YAS_BOLD))
		outs(s,(nattr&YAS_BOLD)?ESC"[1m":ESC"[21m");
	if ((oattr&YAS_ITALIC)!=(nattr&YAS_ITALIC))
		outs(s,(nattr&YAS_ITALIC)?ESC"[3m":ESC"[23m");
	if ((oattr&YAS_UNDERL)!=(nattr&YAS_UNDERL))
		outs(s,(nattr&YAS_UNDERL)?ESC"[4m":ESC"[24m");
	if ((oattr&YAS_BLINK)!=(nattr&YAS_BLINK))
		outs(s,(nattr&YAS_BLINK)?ESC"[5m":ESC"[25m");
	if ((oattr&YAS_INVERSE)!=(nattr&YAS_INVERSE))
		outs(s,(nattr&YAS_INVERSE)?ESC"[7m":ESC"[27m");
	if ((oattr&YAS_STRIKE)!=(nattr&YAS_STRIKE))
		outs(s,(nattr&YAS_STRIKE)?ESC"[9m":ESC"[29m");
	if (YAS_FG(oattr)!=YAS_FG(nattr)) {
		if (YAS_ISXCOLOR(YAS_FG(nattr)))
			outf(s,ESC"[38;5;%dm",YAS_FG(nattr)-0x100);
		else {
			if (YAS_ISCOLOR(YAS_FG(nattr)))
				outf(s,ESC"[%dm",YAS_FG(nattr)-8+30);
			else
				outs(s,ESC"[39m");
		}
	}
	if (YAS_BG(oattr)!=YAS_BG(nattr)) {
		if (YAS_ISXCOLOR(YAS_BG(nattr)))
			outf(s,ESC"[48;5;%dm",YAS_BG(nattr)-0x100);
		else {
			if (YAS_ISCOLOR(YAS_BG(nattr)))
				outf(s,ESC"[%dm",YAS_BG(nattr)-8+40);
			else
				outs(s,ESC"[49m");
		}
	}
} // }}}

static inline int yascreen_update_range(yascreen *s,int y1,int y2) { // {{{
	int i,j,ob=0,redraw=0;
	char ra[]=ESC"[0m";
	uint32_t lsty=0,nsty;

	if (!s)
		return -1;

	if (s->redraw) {
		redraw=1;
		s->redraw=0;
		outf(s,ESC"[2J"ESC"[H%s",ra); // clear and position on topleft
		*ra=0;
	}

	y1=mymin(s->sy-1,mymax(0,y1));
	y2=mymin(s->sy,mymax(0,y2));

	for (j=y1;j<y2;j++) {
		int skip=1,cnt=0;

		if (!redraw&&!(s->mem[s->sx*j].style&YAS_TOUCHED)) // skip untouched lines
			continue;
		s->mem[s->sx*j].style&=~YAS_TOUCHED; // mark updated lines as not touched
		for (i=0;i<s->sx;i++) {
			int diff=redraw; // forced redraw

			if (!diff) // compare attributes
				diff=(s->mem[i+s->sx*j].style&~YAS_INTERNAL)!=(s->scr[i+s->sx*j].style&~YAS_INTERNAL);
			if (!diff) // compare content
				diff=!!strcmp((s->mem[i+s->sx*j].style&YAS_STORAGE)?s->mem[i+s->sx*j].p:s->mem[i+s->sx*j].d,(s->scr[i+s->sx*j].style&YAS_STORAGE)?s->scr[i+s->sx*j].p:s->scr[i+s->sx*j].d);

			if (diff||!skip) {
				if (skip) {
					outf(s,ESC"[%d;%dH%s",1+j,1+i,ra);
					*ra=0;
					skip=0;
				}
				if (diff) {
					if (cnt>7) {
						outf(s,ESC"[%d;%dH%s",1+j,1+i,ra);
						*ra=0;
						cnt=0;
					}
					while (cnt>=0) {
						nsty=s->mem[j*s->sx+i-cnt].style&~YAS_INTERNAL;
						if (lsty!=nsty) {
							yascreen_update_attr(s,lsty,nsty);
							lsty=nsty;
						}
						outs(s,(s->mem[j*s->sx+i-cnt].style&YAS_STORAGE)?s->mem[j*s->sx+i-cnt].p:s->mem[j*s->sx+i-cnt].d);
						cnt--;
					}
					cnt=0; // cycle above leaves cnt at -1
				} else
					cnt++;
			}
			if (s->scr[j*s->sx+i].style&YAS_STORAGE)
				free(s->scr[j*s->sx+i].p);
			s->scr[j*s->sx+i].style=s->mem[j*s->sx+i].style;
			if (s->mem[j*s->sx+i].style&YAS_STORAGE)
				s->scr[j*s->sx+i].p=strdup(s->mem[j*s->sx+i].p);
			else
				strncpy(s->scr[j*s->sx+i].d,s->mem[j*s->sx+i].d,sizeof s->scr[j*s->sx+i].d);
		}
	}
	if (s->cursor)
		outf(s,ESC"[%d;%dH",s->cursory+1,s->cursorx+1);

	out(s,"",0); // request a flush

	return ob;
} // }}}

inline int yascreen_update(yascreen *s) { // {{{
	if (!s)
		return -1;

	return yascreen_update_range(s,0,s->sy);
} // }}}

static inline void yascreen_putcw(yascreen *s,uint32_t attr,const char *str,int width) { // {{{
	if (!*str) // noop
		return;
	if (!str[1]) { // handle CR/LF
		switch (*str) {
			case '\n':
				s->cursory++;
				// fall through
			case '\r':
				s->cursorx=0;
				return;
		}
	}
	if (s->cursory<0||s->cursory>=s->sy)
		return;
	if (width&&s->cursorx>=0&&s->cursorx<s->sx&&s->cursorx+width<=s->sx) {
		int i;

		// normal char
		if (s->mem[s->cursorx+s->cursory*s->sx].style&YAS_STORAGE) {
			s->mem[s->cursorx+s->cursory*s->sx].style&=~YAS_STORAGE;
			free(s->mem[s->cursorx+s->cursory*s->sx].p);
			s->mem[s->cursorx+s->cursory*s->sx].p=0;
		}
		if (strlen(str)<PSIZE) {
			strncpy(s->mem[s->cursorx+s->cursory*s->sx].d,str,sizeof s->mem[s->cursorx+s->cursory*s->sx].d);
			s->mem[s->cursorx+s->cursory*s->sx].style=attr;
		} else {
			char *ts=strdup(str);

			if (!ts)
				return; // nothing more to do

			s->mem[s->cursorx+s->cursory*s->sx].p=ts;
			s->mem[s->cursorx+s->cursory*s->sx].style=YAS_STORAGE|attr;
		}
		s->mem[s->cursory*s->sx].style|=YAS_TOUCHED;

		s->cursorx++;
		for (i=1;i<width;i++) {
			if (s->cursorx<s->sx) {
				if (s->mem[s->cursorx+s->cursory*s->sx].style&YAS_STORAGE) {
					s->mem[s->cursorx+s->cursory*s->sx].style&=~YAS_STORAGE;
					free(s->mem[s->cursorx+s->cursory*s->sx].p);
					s->mem[s->cursorx+s->cursory*s->sx].p=0;
				}
				*s->mem[s->cursorx+s->cursory*s->sx].d=0;
				s->mem[s->cursorx+s->cursory*s->sx].style=attr;
			}
			s->cursorx++;
		}
		return;
	}
	if (s->cursorx<0&&s->cursorx+width>=0) { // wide character spanning left bound
		int x;

		for (x=0;x<s->cursorx+width;x++) { // zap spanned chars
			if (s->mem[x+s->cursory*s->sx].style&YAS_STORAGE) {
				s->mem[x+s->cursory*s->sx].style&=~YAS_STORAGE;
				free(s->mem[x+s->cursory*s->sx].p);
				s->mem[x+s->cursory*s->sx].p=0;
			}
			strncpy(s->mem[x+s->cursory*s->sx].d,"<",sizeof s->mem[x+s->cursory*s->sx].d);
			s->mem[x+s->cursory*s->sx].style=attr;
			s->mem[s->cursory*s->sx].style|=YAS_TOUCHED;
		}
		s->cursorx+=width;
		return;
	}
	if (s->cursorx<0||s->cursorx>=s->sx) { // noop for characters out of screen
		s->cursorx+=width;
		return;
	}
	if (!width&&s->cursorx==0) // nowhere to append - noop
		return;
	if (!width&&s->cursorx>0&&s->cursorx<=s->sx) { // combining char, add to previous
		int clen;
		char *ts;

		s->cursorx--;

		clen=strlen((s->mem[s->cursorx+s->cursory*s->sx].style&YAS_STORAGE)?s->mem[s->cursorx+s->cursory*s->sx].p:s->mem[s->cursorx+s->cursory*s->sx].d);
		if (clen+strlen(str)<PSIZE) { // new fits, so current should fit too and don't have YAS_STORAGE
			strncpy(s->mem[s->cursorx+s->cursory*s->sx].d+clen,str,sizeof s->mem[s->cursorx+s->cursory*s->sx].d-clen);
			s->mem[s->cursorx+s->cursory*s->sx].style=attr; // as a side effect combining chars set attr for main char
		} else {
			size_t tslen=clen+strlen(str)+1;

			ts=malloc(tslen);
			if (!ts) {
				s->cursorx++;
				return; // nothing more we could do
			}
			strcpy(ts,(s->mem[s->cursorx+s->cursory*s->sx].style&YAS_STORAGE)?s->mem[s->cursorx+s->cursory*s->sx].p:s->mem[s->cursorx+s->cursory*s->sx].d);
			strncpy(ts+clen,str,tslen-clen);
			if (s->mem[s->cursorx+s->cursory*s->sx].style&YAS_STORAGE)
				free(s->mem[s->cursorx+s->cursory*s->sx].p);
			s->mem[s->cursorx+s->cursory*s->sx].p=ts;
			s->mem[s->cursorx+s->cursory*s->sx].style=attr|YAS_STORAGE;
		}
		s->mem[s->cursory*s->sx].style|=YAS_TOUCHED;
		s->cursorx++;
	}
	if (!width) // noop
		return;
	if (s->cursorx+width>s->sx) { // wide character spanning right bound
		int x;

		for (x=s->cursorx;x<s->sx;x++) { // zap spanned chars
			if (s->mem[x+s->cursory*s->sx].style&YAS_STORAGE) {
				s->mem[x+s->cursory*s->sx].style&=~YAS_STORAGE;
				free(s->mem[x+s->cursory*s->sx].p);
				s->mem[x+s->cursory*s->sx].p=0;
			}
			strncpy(s->mem[x+s->cursory*s->sx].d,">",sizeof s->mem[x+s->cursory*s->sx].d);
			s->mem[x+s->cursory*s->sx].style=attr;
			s->mem[s->cursory*s->sx].style|=YAS_TOUCHED;
		}
		s->cursorx+=width;
		return;
	}
	return;
} // }}}

inline int yascreen_putsxy(yascreen *s,int x,int y,uint32_t attr,const char *str) { // {{{
	yas_u_state st=U_NORM;
	char utf[5]; // 4 byte sequence + 1 for terminating 0
	size_t i;

	if (!s)
		return EOF;
	if (attr&YAS_INTERNAL)
		return EOF;

	s->cursorx=x; // set cursor position to whatever is requested
	s->cursory=y;

	if (x>=s->sx||y>=s->sy) {
		s->cursorx=mymax(0,mymin(s->sx-1,s->cursorx)); // fixup position to be within screen
		s->cursory=mymax(0,mymin(s->sy-1,s->cursory));
		return 1; // somewhat successful print outside bounds
	}

	for (i=0;i<strlen(str);i++) {
		switch (st) {
			case U_NORM:
				if (str[i]&0x80) {
					if ((str[i]&0xc0)==0x80) // unexpected continuation byte
						break;
				startbyte:
					if ((str[i]&0xe0)==0xc0) { // 2 byte seq
						utf[0]=str[i];
						st=U_L2C1;
						break;
					}
					if ((str[i]&0xf0)==0xe0) { // 3 byte seq
						utf[0]=str[i];
						st=U_L3C1;
						break;
					}
					if ((str[i]&0xf8)==0xf0) { // 4 byte seq
						utf[0]=str[i];
						st=U_L4C1;
						break;
					}
					if ((str[i]&0xfc)==0xf8) { // 5 byte seq
						//utf[0]=str[i];
						st=U_L5C1;
						break;
					}
					if ((str[i]&0xfe)==0xfc) { // 6 byte seq
						//utf[0]=str[i];
						st=U_L6C1;
						break;
					}
					// pass 0xff and 0xfe - violates rfc
					utf[0]=str[i];
					utf[1]=0;
					yascreen_putcw(s,attr,utf,1); // assume width 1
					st=U_NORM; // in case we come from unexpected start byte
				} else {
					utf[0]=str[i];
					utf[1]=0;
					yascreen_putcw(s,attr,utf,1); // assume width 1
				}
				break;
			case U_L2C1:
				if ((str[i]&0xc0)==0x80) { // continuation byte
					wchar_t wc;

					utf[1]=str[i];
					utf[2]=0;

					wc=((utf[0]&0x1f)<<6)|(utf[1]&0x3f);
					yascreen_putcw(s,attr,utf,wcwidth(wc));
					st=U_NORM;
					break;
				}
				if (str[i]&0x80) // start another sequence
					goto startbyte;
				st=U_NORM; // normal byte kills current sequence and is processed
				utf[0]=str[i];
				utf[1]=0;
				yascreen_putcw(s,attr,utf,1); // assume width 1
				break;
			case U_L3C1:
				if ((str[i]&0xc0)==0x80) { // continuation byte
					utf[1]=str[i];
					st=U_L3C2;
					break;
				}
				if (str[i]&0x80) // start another sequence
					goto startbyte;
				st=U_NORM; // normal byte kills current sequence and is processed
				utf[0]=str[i];
				utf[1]=0;
				yascreen_putcw(s,attr,utf,1); // assume width 1
				break;
			case U_L3C2:
				if ((str[i]&0xc0)==0x80) { // continuation byte
					wchar_t wc;

					utf[2]=str[i];
					utf[3]=0;

					wc=((utf[0]&0x0f)<<12)|((utf[1]&0x3f)<<6)|(utf[2]&0x3f);
					yascreen_putcw(s,attr,utf,wcwidth(wc));
					st=U_NORM;
					break;
				}
				if (str[i]&0x80) // start another sequence
					goto startbyte;
				st=U_NORM; // normal byte kills current sequence and is processed
				utf[0]=str[i];
				utf[1]=0;
				yascreen_putcw(s,attr,utf,1); // assume width 1
				break;
			case U_L4C1:
				if ((str[i]&0xc0)==0x80) { // continuation byte
					utf[1]=str[i];
					st=U_L4C2;
					break;
				}
				if (str[i]&0x80) // start another sequence
					goto startbyte;
				st=U_NORM; // normal byte kills current sequence and is processed
				utf[0]=str[i];
				utf[1]=0;
				yascreen_putcw(s,attr,utf,1); // assume width 1
				break;
			case U_L4C2:
				if ((str[i]&0xc0)==0x80) { // continuation byte
					utf[2]=str[i];
					st=U_L4C3;
					break;
				}
				if (str[i]&0x80) // start another sequence
					goto startbyte;
				st=U_NORM; // normal byte kills current sequence and is processed
				utf[0]=str[i];
				utf[1]=0;
				yascreen_putcw(s,attr,utf,1); // assume width 1
				break;
			case U_L4C3:
				if ((str[i]&0xc0)==0x80) { // continuation byte
					wchar_t wc;

					utf[3]=str[i];
					utf[4]=0;

					wc=((utf[0]&0x07)<<18)|((utf[1]&0x3f)<<12)|((utf[2]&0x3f)<<6)|(utf[3]&0x3f);
					yascreen_putcw(s,attr,utf,wcwidth(wc));
					st=U_NORM;
					break;
				}
				if (str[i]&0x80) // start another sequence
					goto startbyte;
				st=U_NORM; // normal byte kills current sequence and is processed
				utf[0]=str[i];
				utf[1]=0;
				yascreen_putcw(s,attr,utf,1); // assume width 1
				break;
			case U_L5C1:
				if ((str[i]&0xc0)==0x80) { // continuation byte
					//utf[1]=str[i];
					st=U_L5C2;
					break;
				}
				if (str[i]&0x80) // start another sequence
					goto startbyte;
				st=U_NORM; // normal byte kills current sequence and is processed
				utf[0]=str[i];
				utf[1]=0;
				yascreen_putcw(s,attr,utf,1); // assume width 1
				break;
			case U_L5C2:
				if ((str[i]&0xc0)==0x80) { // continuation byte
					//utf[2]=str[i];
					st=U_L5C3;
					break;
				}
				if (str[i]&0x80) // start another sequence
					goto startbyte;
				st=U_NORM; // normal byte kills current sequence and is processed
				utf[0]=str[i];
				utf[1]=0;
				yascreen_putcw(s,attr,utf,1); // assume width 1
				break;
			case U_L5C3:
				if ((str[i]&0xc0)==0x80) { // continuation byte
					//utf[3]=str[i];
					st=U_L5C4;
					break;
				}
				if (str[i]&0x80) // start another sequence
					goto startbyte;
				st=U_NORM; // normal byte kills current sequence and is processed
				utf[0]=str[i];
				utf[1]=0;
				yascreen_putcw(s,attr,utf,1); // assume width 1
				break;
			case U_L5C4:
				if ((str[i]&0xc0)==0x80) { // continuation byte
					//utf[4]=str[i];
					//utf[5]=0;
					// ignore char by rfc
					st=U_NORM;
					break;
				}
				if (str[i]&0x80) // start another sequence
					goto startbyte;
				st=U_NORM; // normal byte kills current sequence and is processed
				utf[0]=str[i];
				utf[1]=0;
				yascreen_putcw(s,attr,utf,1); // assume width 1
				break;
			case U_L6C1:
				if ((str[i]&0xc0)==0x80) { // continuation byte
					//utf[1]=str[i];
					st=U_L6C2;
					break;
				}
				if (str[i]&0x80) // start another sequence
					goto startbyte;
				st=U_NORM; // normal byte kills current sequence and is processed
				utf[0]=str[i];
				utf[1]=0;
				yascreen_putcw(s,attr,utf,1); // assume width 1
				break;
			case U_L6C2:
				if ((str[i]&0xc0)==0x80) { // continuation byte
					//utf[2]=str[i];
					st=U_L6C3;
					break;
				}
				if (str[i]&0x80) // start another sequence
					goto startbyte;
				st=U_NORM; // normal byte kills current sequence and is processed
				utf[0]=str[i];
				utf[1]=0;
				yascreen_putcw(s,attr,utf,1); // assume width 1
				break;
			case U_L6C3:
				if ((str[i]&0xc0)==0x80) { // continuation byte
					//utf[3]=str[i];
					st=U_L6C4;
					break;
				}
				if (str[i]&0x80) // start another sequence
					goto startbyte;
				st=U_NORM; // normal byte kills current sequence and is processed
				utf[0]=str[i];
				utf[1]=0;
				yascreen_putcw(s,attr,utf,1); // assume width 1
				break;
			case U_L6C4:
				if ((str[i]&0xc0)==0x80) { // continuation byte
					//utf[4]=str[i];
					st=U_L6C5;
					break;
				}
				if (str[i]&0x80) // start another sequence
					goto startbyte;
				st=U_NORM; // normal byte kills current sequence and is processed
				utf[0]=str[i];
				utf[1]=0;
				yascreen_putcw(s,attr,utf,1); // assume width 1
				break;
			case U_L6C5:
				if ((str[i]&0xc0)==0x80) { // continuation byte
					//utf[5]=str[i];
					//utf[6]=0;
					// ignore char by rfc
					st=U_NORM;
					break;
				}
				if (str[i]&0x80) // start another sequence
					goto startbyte;
				st=U_NORM; // normal byte kills current sequence and is processed
				utf[0]=str[i];
				utf[1]=0;
				yascreen_putcw(s,attr,utf,1); // assume width 1
				break;
		}
	}
	s->cursorx=mymax(0,mymin(s->sx-1,s->cursorx)); // fixup position to be within screen
	s->cursory=mymax(0,mymin(s->sy-1,s->cursory));
	return 1;
} // }}}

inline int yascreen_putsxyu(yascreen *s,int x,int y,uint32_t attr,const char *str) { // {{{
	int rv,ny;

	if (!s)
		return EOF;

	rv=yascreen_putsxy(s,x,y,attr,str);
	if (rv==EOF)
		return rv;

	ny=s->cursory;
	yascreen_update_range(s,y,ny+1);

	return rv;
} // }}}

inline int yascreen_printxy(yascreen *s,int x,int y,uint32_t attr,const char *format,...) { // {{{
	va_list ap;
	char *ns;
	int size;
	int rv;

	if (!s)
		return -1;
	if (!format)
		return -1;

	va_start(ap,format);
	size=vasprintf(&ns,format,ap);
	va_end(ap);

	if (size==-1) // some error, nothing more to do
		return size;

	rv=yascreen_putsxy(s,x,y,attr,ns);

	free(ns);
	return rv;
} // }}}

inline int yascreen_printxyu(yascreen *s,int x,int y,uint32_t attr,const char *format,...) { // {{{
	va_list ap;
	char *ns;
	int size;
	int rv;

	if (!s)
		return -1;
	if (!format)
		return -1;

	va_start(ap,format);
	size=vasprintf(&ns,format,ap);
	va_end(ap);

	if (size==-1) // some error, nothing more to do
		return size;

	rv=yascreen_putsxyu(s,x,y,attr,ns);

	free(ns);
	return rv;
} // }}}

inline int yascreen_write(yascreen *s,const char *str,int len) { // {{{
	ssize_t (*o)(yascreen *s,const void *buf,size_t len);

	if (!s)
		return -1;
	if (!str)
		return -1;

	o=s->outcb?s->outcb:out;
	return o(s,str,len);
} // }}}

inline int yascreen_puts(yascreen *s,const char *str) { // {{{
	if (!s)
		return -1;

	outs(s,str);
	return 1;
} // }}}

inline int yascreen_print(yascreen *s,const char *format,...) { // {{{
	va_list ap;
	char *ns;
	int size;
	int rv;

	if (!s)
		return -1;
	if (!format)
		return -1;

	va_start(ap,format);
	size=vasprintf(&ns,format,ap);
	va_end(ap);

	if (size==-1) // some error, nothing more to do
		return size;

	rv=yascreen_puts(s,ns);

	free(ns);
	return rv;
} // }}}

inline const char *yascreen_clearln_s(yascreen *s __attribute__((unused))) { // {{{
	return ESC"[2K";
} // }}}

inline void yascreen_dump(yascreen *s) { // {{{
	int i,j;

	if (!s)
		return;

	printf("PSIZE: %zu\n",PSIZE);

	for (j=0;j<s->sy;j++)
		for (i=0;i<s->sx;i++)
			printf("x: %3d y: %3d len: %3zu attr: %08x s: %s\n",i,j,strlen((s->mem[i+s->sx*j].style&YAS_STORAGE)?s->mem[i+s->sx*j].p:s->mem[i+s->sx*j].d),s->mem[i+s->sx*j].style,(s->mem[i+s->sx*j].style&YAS_STORAGE)?s->mem[i+s->sx*j].p:s->mem[i+s->sx*j].d);
} // }}}

inline void yascreen_redraw(yascreen *s) { // {{{
	if (!s)
		return;

	s->redraw=1;
} // }}}

inline void yascreen_cursor(yascreen *s,int on) { // {{{
	if (!s)
		return;

	s->cursor=!!on;
	if (on)
		outs(s,ESC"[?25h"); // show cursor
	else
		outs(s,ESC"[?25l"); // hide cursor
	out(s,"",0); // request a flush
} // }}}

inline void yascreen_cursor_xy(yascreen *s,int x,int y) { // {{{
	if (!s)
		return;

	s->cursorx=mymin(mymax(x,0),s->sx-1);
	s->cursory=mymin(mymax(y,0),s->sy-1);
	outf(s,ESC"[%d;%dH",s->cursory+1,s->cursorx+1);
	out(s,"",0); // request a flush
} // }}}

inline void yascreen_altbuf(yascreen *s,int on) { // {{{
	if (!s)
		return;

	if (on)
		outs(s,ESC"[?1049h"); // go to alternative buffer
	else
		outs(s,ESC"[?1049l"); // go back to normal buffer
	out(s,"",0); // request a flush
} // }}}

inline void yascreen_clear(yascreen *s) { // {{{
	if (!s)
		return;

	outs(s,ESC"[0m"ESC"[2J"ESC"[H"); // reset attributes, clear screen and reset position
	out(s,"",0); // request a flush
} // }}}

inline void yascreen_clearln(yascreen *s) { // {{{
	if (!s)
		return;

	outs(s,yascreen_clearln_s(s)); // clear line
	out(s,"",0); // request a flush
} // }}}

inline void yascreen_term_save(yascreen *s) { // {{{
	if (!s)
		return;
	if (s->outcb)
		return;
	if (!isatty(STDOUT_FILENO))
		return;

	if (!s->tssize) { // no saved state, allocate new one
		s->tsstack=(struct termios *)calloc(1,sizeof(struct termios));
		if (!s->tsstack)
			return;
		s->tssize=1;
	}

	tcgetattr(STDOUT_FILENO,s->tsstack);
} // }}}

inline void yascreen_term_restore(yascreen *s) { // {{{
	if (!s)
		return;
	if (s->outcb)
		return;
	if (!isatty(STDOUT_FILENO))
		return;

	if (!s->tssize) // no saved state
		return;

	out(s,"",0); // request a flush
	tcsetattr(STDOUT_FILENO,TCSANOW,s->tsstack);
} // }}}

inline void yascreen_term_push(yascreen *s) { // {{{
	struct termios *t;

	if (!s)
		return;
	if (s->outcb)
		return;
	if (!isatty(STDOUT_FILENO))
		return;

	t=(struct termios *)realloc(s->tsstack,(s->tssize+1)*sizeof(struct termios));
	if (!t)
		return;
	s->tsstack=t;
	s->tssize++;
	memmove(s->tsstack+1,s->tsstack,(s->tssize-1)*sizeof(struct termios));
	tcgetattr(STDOUT_FILENO,s->tsstack);
} // }}}

inline void yascreen_term_pop(yascreen *s) { // {{{
	if (!s)
		return;
	if (s->outcb)
		return;
	if (!isatty(STDOUT_FILENO))
		return;

	if (!s->tssize)
		return;

	tcsetattr(STDOUT_FILENO,TCSANOW,s->tsstack);
	if (s->tssize>1) {
		memmove(s->tsstack,s->tsstack+1,(s->tssize-1)*sizeof(struct termios));
		s->tssize--;
	}
} // }}}

inline void yascreen_term_set(yascreen *s,int mode) { // {{{
	struct termios t;

	if (!s)
		return;
	if (s->outcb)
		return;
	if (!isatty(STDOUT_FILENO))
		return;

	// get the terminal state
	tcgetattr(STDOUT_FILENO,&t);

	// turn off canonical mode
	if (mode&YAS_NOBUFF)
		t.c_lflag&=~(ICANON|IEXTEN);
	if (mode&YAS_NOSIGN)
		t.c_lflag&=~(ISIG);
	if (mode&YAS_NOECHO)
		t.c_lflag&=~(ECHO);
	// set input modes
	t.c_iflag&=~(BRKINT|ICRNL|INPCK|ISTRIP|IXON);
	// no post processing
	t.c_oflag&=~(OPOST|ONLCR|OCRNL);
	if (mode&YAS_ONLCR)
		t.c_oflag|=ONLCR|OPOST;
	// 8bit mode
	t.c_cflag|=CS8;
	// minimum of number input read.
	t.c_cc[VMIN]=1;
	// no timeout
	t.c_cc[VTIME]=0;

	tcsetattr(STDOUT_FILENO,TCSANOW,&t);
} // }}}

inline int yascreen_sx(yascreen *s) { // {{{
	if (!s)
		return -1;

	return s->sx;
} // }}}

inline int yascreen_sy(yascreen *s) { // {{{
	if (!s)
		return -1;

	return s->sy;
} // }}}

inline int yascreen_x(yascreen *s) { // {{{
	if (!s)
		return -1;

	return s->cursorx;
} // }}}

inline int yascreen_y(yascreen *s) { // {{{
	if (!s)
		return -1;

	return s->cursory;
} // }}}

inline void yascreen_ckto(yascreen *s) { // {{{
	if (!s)
		return;

	if (s->state==ST_ESC&&s->ansipos==1&&s->ansibuf[0]==YAS_K_ESC&&s->escto&&s->escts+s->escto<mytime()) {
		s->ansipos=0;
		s->state=ST_NORM;
		yascreen_pushch(s,YAS_K_ESC);
	}
} // }}}

static inline int yascreen_feed_telnet(yascreen *s,unsigned char c) { // {{{
	if (!s)
		return TELNET_NOOP;

	switch (s->tstate) {
		case T_NORM:
			if (c==TELNET_IAC) {
				s->tstate=T_IAC;
				return TELNET_NOOP;
			} else
				return c;
		case T_IAC:
			switch (c) { // naive, not so RFC compliant telnet protocol handling
				default: // treat as 2 byte sequence
				case TELNET_EOSN: // end of subnegotiation
				case TELNET_NOP: // NOP
				case TELNET_SYNCH: // SYNCH
				case TELNET_NVTBRK: // NVT BRK
				case TELNET_IP: // IP
				case TELNET_AO: // AO
				case TELNET_AYT: // AYT are you there
				case TELNET_EC: // EC
				case TELNET_EL: // EL
				case TELNET_GOA: // go ahead
					s->tstate=T_NORM;
					return TELNET_NOOP;
				case TELNET_SOSN: // start of subnegotiation
					s->sosnpos=0;
					s->tstate=T_IAC_SB;
					return TELNET_NOOP;
				case TELNET_WILL: // will
				case TELNET_WONT: // wont
				case TELNET_DO: // do
				case TELNET_DONT: // dont
					s->tstate=T_IAC_O; // treat as 3 bytes sequence
					return TELNET_NOOP;
				case TELNET_IAC: // escaped 255
					s->tstate=T_NORM;
					return TELNET_IAC;
			}
			break;
		case T_IAC_O:
			s->tstate=T_NORM;
			return TELNET_NOOP;
		case T_IAC_SB:
			if (c==TELNET_IAC)
				s->tstate=T_IAC_SE;
			else
				if (s->sosnpos<sizeof s->sosnbuf)
					s->sosnbuf[s->sosnpos++]=c;
			return TELNET_NOOP;
		case T_IAC_SE:
			switch (c) {
				case TELNET_EOSN: // try to redetect terminal size
					s->tstate=T_NORM;
					if (s->sosnpos==5&&s->sosnbuf[0]==TELNET_NAWS) { // get terminal size from NAWS
						uint16_t sx,sy;

						sx=(s->sosnbuf[1]<<8)+s->sosnbuf[2];
						sy=(s->sosnbuf[3]<<8)+s->sosnbuf[4];
						if (sx>10&&sy>3&&sx<=999&&sy<=999) { // ignore non-sane values
							s->scrx=sx;
							s->scry=sy;
							s->havenaws=1;
							yascreen_pushch(s,YAS_SCREEN_SIZE);
							return TELNET_NOOP;
						}
					}
					// fallback to the old way of redetecting screen size via ansi
					return TELNET_SIZE;
				case TELNET_IAC: // escaped 255
					if (s->sosnpos<sizeof s->sosnbuf)
						s->sosnbuf[s->sosnpos++]=c;
					s->tstate=T_IAC_SB;
					return TELNET_NOOP;
				default: // protocol error
					s->tstate=T_NORM;
					return TELNET_NOOP;
			}
			break;
	}
	return TELNET_NOOP;
} // }}}

inline void yascreen_feed(yascreen *s,unsigned char c) { // {{{
	if (!s)
		return;

	yascreen_ckto(s);
	if (s->istelnet) { // process telnet codes
		int tc=yascreen_feed_telnet(s,c);

		switch (tc) {
			case 0x00 ... 0xff: // normal character
				c=(unsigned char)tc;
				break;
			default:
			case TELNET_NOOP: // byte is eaten w/o valid input
				return;
			case TELNET_SIZE: // notify about screen size change
				yascreen_pushch(s,YAS_TELNET_SIZE);
				return;
		}
	}

	switch (s->state) {
		case ST_ENTER:
			if (c=='\n'||c==0) // ignore LF or NUL after CR
				break;
			s->state=ST_NORM;
			// fall through
		case ST_NORM:
			if (c==YAS_K_ESC) { // handle esc sequences
				s->escts=mytime();
				s->ansipos=1;
				s->ansibuf[0]=c;
				s->state=ST_ESC;
			} else { // handle standard keys
				if (c=='\r') // shift state to ST_ENTER to eat optional LF/NUL after CR
					s->state=ST_ENTER;
				switch (s->ustate) {
					case U_NORM:
						if (c&0x80) {
							if ((c&0xc0)==0x80) // unexpected continuation byte - ignore
								break;
						startbyte:
							if ((c&0xe0)==0xc0) { // 2 byte seq
								s->utf[0]=c;
								s->ustate=U_L2C1;
								break;
							}
							if ((c&0xf0)==0xe0) { // 3 byte seq
								s->utf[0]=c;
								s->ustate=U_L3C1;
								break;
							}
							if ((c&0xf8)==0xf0) { // 4 byte seq
								s->utf[0]=c;
								s->ustate=U_L4C1;
								break;
							}
							if ((c&0xfc)==0xf8) { // 5 byte seq
								//s->utf[0]=c;
								s->ustate=U_L5C1;
								break;
							}
							if ((c&0xfe)==0xfc) { // 6 byte seq
								//s->utf[0]=c;
								s->ustate=U_L6C1;
								break;
							}
							// pass 0xff and 0xfe - violates rfc
							yascreen_pushch(s,c);
							s->ustate=U_NORM; // in case we come from unexpected start byte
						} else
							yascreen_pushch(s,c);
						break;
					case U_L2C1:
						if ((c&0xc0)==0x80) { // continuation byte
							yascreen_pushch(s,s->utf[0]);
							yascreen_pushch(s,c);
							s->ustate=U_NORM;
							break;
						}
						if (c&0x80) // start another sequence
							goto startbyte;
						s->ustate=U_NORM; // normal byte kills current sequence and is processed
						yascreen_pushch(s,c);
						break;
					case U_L3C1:
						if ((c&0xc0)==0x80) { // continuation byte
							s->utf[1]=c;
							s->ustate=U_L3C2;
							break;
						}
						if (c&0x80) // start another sequence
							goto startbyte;
						s->ustate=U_NORM; // normal byte kills current sequence and is processed
						yascreen_pushch(s,c);
						break;
					case U_L3C2:
						if ((c&0xc0)==0x80) { // continuation byte
							yascreen_pushch(s,s->utf[0]);
							yascreen_pushch(s,s->utf[1]);
							yascreen_pushch(s,c);
							s->ustate=U_NORM;
							break;
						}
						if (c&0x80) // start another sequence
							goto startbyte;
						s->ustate=U_NORM; // normal byte kills current sequence and is processed
						yascreen_pushch(s,c);
						break;
					case U_L4C1:
						if ((c&0xc0)==0x80) { // continuation byte
							s->utf[1]=c;
							s->ustate=U_L4C2;
							break;
						}
						if (c&0x80) // start another sequence
							goto startbyte;
						s->ustate=U_NORM; // normal byte kills current sequence and is processed
						yascreen_pushch(s,c);
						break;
					case U_L4C2:
						if ((c&0xc0)==0x80) { // continuation byte
							s->utf[2]=c;
							s->ustate=U_L4C3;
							break;
						}
						if (c&0x80) // start another sequence
							goto startbyte;
						s->ustate=U_NORM; // normal byte kills current sequence and is processed
						yascreen_pushch(s,c);
						break;
					case U_L4C3:
						if ((c&0xc0)==0x80) { // continuation byte
							yascreen_pushch(s,s->utf[0]);
							yascreen_pushch(s,s->utf[1]);
							yascreen_pushch(s,s->utf[2]);
							yascreen_pushch(s,c);
							s->ustate=U_NORM;
							break;
						}
						if (c&0x80) // start another sequence
							goto startbyte;
						s->ustate=U_NORM; // normal byte kills current sequence and is processed
						yascreen_pushch(s,c);
						break;
					case U_L5C1:
						if ((c&0xc0)==0x80) { // continuation byte
							//s->utf[1]=c;
							s->ustate=U_L5C2;
							break;
						}
						if (c&0x80) // start another sequence
							goto startbyte;
						s->ustate=U_NORM; // normal byte kills current sequence and is processed
						yascreen_pushch(s,c);
						break;
					case U_L5C2:
						if ((c&0xc0)==0x80) { // continuation byte
							//s->utf[2]=c;
							s->ustate=U_L5C3;
							break;
						}
						if (c&0x80) // start another sequence
							goto startbyte;
						s->ustate=U_NORM; // normal byte kills current sequence and is processed
						yascreen_pushch(s,c);
						break;
					case U_L5C3:
						if ((c&0xc0)==0x80) { // continuation byte
							//s->utf[3]=c;
							s->ustate=U_L5C4;
							break;
						}
						if (c&0x80) // start another sequence
							goto startbyte;
						s->ustate=U_NORM; // normal byte kills current sequence and is processed
						yascreen_pushch(s,c);
						break;
					case U_L5C4:
						if ((c&0xc0)==0x80) { // continuation byte
							//yascreen_pushch(s,s->utf[0]); // sequence is parsed but ignored
							//yascreen_pushch(s,s->utf[1]);
							//yascreen_pushch(s,s->utf[2]);
							//yascreen_pushch(s,s->utf[3]);
							//yascreen_pushch(s,c);
							s->ustate=U_NORM;
							break;
						}
						if (c&0x80) // start another sequence
							goto startbyte;
						s->ustate=U_NORM; // normal byte kills current sequence and is processed
						yascreen_pushch(s,c);
						break;
					case U_L6C1:
						if ((c&0xc0)==0x80) { // continuation byte
							//s->utf[1]=c;
							s->ustate=U_L6C2;
							break;
						}
						if (c&0x80) // start another sequence
							goto startbyte;
						s->ustate=U_NORM; // normal byte kills current sequence and is processed
						yascreen_pushch(s,c);
						break;
					case U_L6C2:
						if ((c&0xc0)==0x80) { // continuation byte
							//s->utf[2]=c;
							s->ustate=U_L6C3;
							break;
						}
						if (c&0x80) // start another sequence
							goto startbyte;
						s->ustate=U_NORM; // normal byte kills current sequence and is processed
						yascreen_pushch(s,c);
						break;
					case U_L6C3:
						if ((c&0xc0)==0x80) { // continuation byte
							//s->utf[3]=c;
							s->ustate=U_L6C4;
							break;
						}
						if (c&0x80) // start another sequence
							goto startbyte;
						s->ustate=U_NORM; // normal byte kills current sequence and is processed
						yascreen_pushch(s,c);
						break;
					case U_L6C4:
						if ((c&0xc0)==0x80) { // continuation byte
							//s->utf[3]=c;
							s->ustate=U_L6C5;
							break;
						}
						if (c&0x80) // start another sequence
							goto startbyte;
						s->ustate=U_NORM; // normal byte kills current sequence and is processed
						yascreen_pushch(s,c);
						break;
					case U_L6C5:
						if ((c&0xc0)==0x80) { // continuation byte
							//yascreen_pushch(s,s->utf[0]); // sequence is parsed but ignored
							//yascreen_pushch(s,s->utf[1]);
							//yascreen_pushch(s,s->utf[2]);
							//yascreen_pushch(s,s->utf[3]);
							//yascreen_pushch(s,s->utf[4]);
							//yascreen_pushch(s,c);
							s->ustate=U_NORM;
							break;
						}
						if (c&0x80) // start another sequence
							goto startbyte;
						s->ustate=U_NORM; // normal byte kills current sequence and is processed
						yascreen_pushch(s,c);
						break;
				}
			}
			break;
		case ST_ESC:
			switch (c) {
				case '`':
				case '-':
				case '=':
				case 0x7f:
				case '~':
				case '!':
				case '@':
				case '#':
				case '$':
				case '%':
				case '^':
				case '&':
				case '*':
				case '(':
				case ')':
				case '_':
				case '+':
				case ':':
				case ';':
				case '"':
				case '\'':
				case '{':
				case '}':
				case '|':
				case '\\':
				case ',':
				case '.':
				case '/':
				case '<':
				case '>':
				case '?':
				case '0'...'9':
				case 'a'...'z':
					yascreen_pushch(s,YAS_K_ALT(c));
					s->state=ST_NORM;
					break;
				case '[':
					s->ansibuf[s->ansipos++]=c;
					s->state=ST_ESC_SQ;
					break;
				case 'O':
					s->ansibuf[s->ansipos++]=c;
					s->state=ST_ESC_O;
					break;
				default: // ignore unknown sequence
					s->state=ST_NORM;
					break;
			}
			break;
		case ST_ESC_SQ:
			switch (c) {
				case 'A': // up
					yascreen_pushch(s,YAS_K_UP);
					s->state=ST_NORM;
					break;
				case 'B': // down
					yascreen_pushch(s,YAS_K_DOWN);
					s->state=ST_NORM;
					break;
				case 'C': // right
					yascreen_pushch(s,YAS_K_RIGHT);
					s->state=ST_NORM;
					break;
				case 'D': // left
					yascreen_pushch(s,YAS_K_LEFT);
					s->state=ST_NORM;
					break;
				case 'H': // home
					yascreen_pushch(s,YAS_K_HOME);
					s->state=ST_NORM;
					break;
				case 'F': // end
					yascreen_pushch(s,YAS_K_END);
					s->state=ST_NORM;
					break;
				case '0'...'9':
					s->state=ST_ESC_SQ_D;
					s->ansibuf[s->ansipos++]=c;
					break;
				default: // ignore unknown sequence
					s->state=ST_NORM;
					break;
			}
			break;
		case ST_ESC_SQ_D:
			if (s->ansipos>=sizeof s->ansibuf) { // buffer overrun, ignore the sequence
				s->state=ST_NORM;
				break;
			}
			s->ansibuf[s->ansipos++]=c;
			if (c>=0x40&&c<=0x7e) { // final char
				s->state=ST_NORM;
				s->ansibuf[s->ansipos]=0;
				switch (c) {
					case '~': // 0x7e
						if (s->ansipos==5&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='1') // F1 - \e[11~
							yascreen_pushch(s,YAS_K_F1);
						if (s->ansipos==5&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='2') // F2 - \e[12~
							yascreen_pushch(s,YAS_K_F2);
						if (s->ansipos==5&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='3') // F3 - \e[13~
							yascreen_pushch(s,YAS_K_F3);
						if (s->ansipos==5&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='4') // F4 - \e[14~
							yascreen_pushch(s,YAS_K_F4);
						if (s->ansipos==5&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='5') // F5 - \e[15~
							yascreen_pushch(s,YAS_K_F5);
						if (s->ansipos==5&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='7') // F6 - \e[17~
							yascreen_pushch(s,YAS_K_F6);
						if (s->ansipos==5&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='8') // F7 - \e[18~
							yascreen_pushch(s,YAS_K_F7);
						if (s->ansipos==5&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='9') // F8 - \e[19~
							yascreen_pushch(s,YAS_K_F8);
						if (s->ansipos==5&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='1') // F10 - \e[21~
							yascreen_pushch(s,YAS_K_F10);
						if (s->ansipos==5&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='3') // F11 - \e[23~
							yascreen_pushch(s,YAS_K_F11);
						if (s->ansipos==5&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='4') // F12 - \e[24~
							yascreen_pushch(s,YAS_K_F12);
						if (s->ansipos==4&&s->ansibuf[2]=='2') // insert - \e[2~
							yascreen_pushch(s,YAS_K_INS);
						if (s->ansipos==4&&s->ansibuf[2]=='3') // delete - \e[3~
							yascreen_pushch(s,YAS_K_DEL);
						if (s->ansipos==4&&s->ansibuf[2]=='5') // pgup - \e[5~
							yascreen_pushch(s,YAS_K_PGUP);
						if (s->ansipos==4&&s->ansibuf[2]=='6') // pgdn - \e[6~
							yascreen_pushch(s,YAS_K_PGDN);
						if (s->ansipos==4&&(s->ansibuf[2]=='1'||s->ansibuf[2]=='7')) // home - \e[1~ \e[7~
							yascreen_pushch(s,YAS_K_HOME);
						if (s->ansipos==4&&(s->ansibuf[2]=='4'||s->ansibuf[2]=='8')) // end - \e[4~ \e[8~
							yascreen_pushch(s,YAS_K_END);
						break;
					case 'R': { // \e[n;mR - cursor position report
						int sx,sy;

						sscanf((char *)s->ansibuf+2,"%d;%dR",&sy,&sx);
						if (sx>10&&sy>3&&sx<=999&&sy<=999) { // ignore non-sane values
							s->scrx=sx;
							s->scry=sy;
							s->haveansi=1;
							yascreen_pushch(s,YAS_SCREEN_SIZE);
						}
						break;
					}
					case 'A': // ^up - \e[1;5A
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5')
							yascreen_pushch(s,YAS_K_C_UP);
						break;
					case 'B': // ^down - \e[1;5B
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5')
							yascreen_pushch(s,YAS_K_C_DOWN);
						break;
					case 'C': // ^right - \e[1;5C
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5')
							yascreen_pushch(s,YAS_K_C_RIGHT);
						break;
					case 'D': // ^left - \e[1;5D
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5')
							yascreen_pushch(s,YAS_K_C_LEFT);
						break;
				}
			}
			break;
		case ST_ESC_O:
			switch (c) {
				case 'P': // F1 \eOP
					yascreen_pushch(s,YAS_K_F1);
					break;
				case 'Q': // F2 \eOQ
					yascreen_pushch(s,YAS_K_F2);
					break;
				case 'R': // F3 \eOR
					yascreen_pushch(s,YAS_K_F3);
					break;
				case 'S': // F4 \eOS
					yascreen_pushch(s,YAS_K_F4);
					break;
				case 'H': // home \eOH
					yascreen_pushch(s,YAS_K_HOME);
					break;
				case 'F': // end \eOF
					yascreen_pushch(s,YAS_K_END);
					break;
				case 'a': // ^up \eOa
					yascreen_pushch(s,YAS_K_C_UP);
					break;
				case 'b': // ^down \eOb
					yascreen_pushch(s,YAS_K_C_DOWN);
					break;
				case 'c': // ^right \eOc
					yascreen_pushch(s,YAS_K_C_RIGHT);
					break;
				case 'd': // ^left \eOd
					yascreen_pushch(s,YAS_K_C_LEFT);
					break;
			}
			s->state=ST_NORM;
			break;
	}
} // }}}

inline int yascreen_getch_to(yascreen *s,int timeout) { // {{{
	int64_t toms=timeout*1000,tto;
	struct timeval to,*pto=&to;
	fd_set r;

	memset(&r,0,sizeof r); // make clang static analyzer happier (llvm bug #8920)
	if (!s)
		return -1;

	if (s->outcb) // we do not handle the input, so return immediately
		timeout=-1;

	if (timeout==0&&s->escto==0) // wait forever
		pto=NULL;
	else
		if (timeout<0) // return immediately
			toms=0;

	tto=s->escto?mymin(s->escto,timeout==0?s->escto:toms):toms;
	if (toms)
		toms-=tto; // remaining time to wait is in toms
	to.tv_sec=tto/1000;
	to.tv_usec=(tto%1000)*1000;

	for (;;) {
		yascreen_ckto(s); // check for esc timeout to return it as a key
		if (s->keycnt) { // check if we have stored key
			int key=s->keys[0];

			s->keycnt--;
			memmove(s->keys,s->keys+1,sizeof(int)*s->keycnt);
			return key;
		}
		if (s->outcb)
			return -1;
		if (STDOUT_FILENO<0)
			return -1;
		FD_ZERO(&r);
		FD_SET(STDOUT_FILENO,&r);
		if (-1!=select(STDOUT_FILENO+1,&r,NULL,NULL,pto)) {
			unsigned char c; // important to be unsigned, so codes>127 do not expand as negative int values

			if (FD_ISSET(STDOUT_FILENO,&r)&&sizeof c==read(STDOUT_FILENO,&c,sizeof c)) {
				yascreen_feed(s,c);
				continue; // check if feed has yielded a key
			}
		}
		if (pto&&(timeout>0||s->escto)&&to.tv_sec==0&&to.tv_usec==0) { // return because of timeout
			if (timeout<0) // nowait is set
				return -1;
			if (!toms&&timeout>0) // timeout is finished
				return -1;
			if (!toms)
				tto=s->escto;
			else
				tto=s->escto?mymin(s->escto,toms):toms;
			if (toms)
				toms-=tto; // remaining time to wait is in toms
			to.tv_sec=tto/1000;
			to.tv_usec=(tto%1000)*1000;
		}
	}
} // }}}

inline void yascreen_ungetch(yascreen *s,int key) { // {{{
	int *tk;

	if (!s)
		return;

	if (s->keysize<=s->keycnt) { // need to reallocate key storage
		int newsize=s->keysize+KEYSTEP;

		tk=(int *)realloc(s->keys,sizeof(int)*newsize);
		if (!tk)
			return;
		s->keys=tk;
		s->keysize=newsize;
	}

	memmove(s->keys+1,s->keys,sizeof(int)*s->keycnt);
	s->keys[0]=key;
	s->keycnt++;
} // }}}

inline void yascreen_pushch(yascreen *s,int key) { // {{{
	int *tk;

	if (!s)
		return;

	if (s->keysize<=s->keycnt) { // need to reallocate key storage
		int newsize=s->keysize+KEYSTEP;

		tk=(int *)realloc(s->keys,sizeof(int)*newsize);
		if (!tk)
			return;
		s->keys=tk;
		s->keysize=newsize;
	}

	s->keys[s->keycnt++]=key;
} // }}}

inline void yascreen_esc_to(yascreen *s,int timeout) { // {{{
	if (!s)
		return;

	s->escto=(timeout>=0)?timeout:YAS_DEFAULT_ESCTO;
} // }}}

inline int yascreen_peekch(yascreen *s) { // {{{
	int ch=yascreen_getch_nowait(s);

	if (ch!=-1)
		yascreen_ungetch(s,ch);
	return ch;
} // }}}

inline void yascreen_clear_mem(yascreen *s,uint32_t attr) { // {{{
	int i;

	if (!s)
		return;
	attr&=~YAS_STORAGE;

	for (i=0;i<s->sx*s->sy;i++) {
		if (s->mem[i].style&YAS_STORAGE)
			free(s->mem[i].p);
		s->mem[i].style=attr;
		strncpy(s->mem[i].d," ",sizeof s->mem[i].d);
	}
} // }}}

inline void yascreen_getsize(yascreen *s,int *sx,int *sy) { // {{{
	if (!s)
		return;
	if (sx)
		*sx=s->scrx;
	if (sy)
		*sy=s->scry;
} // }}}

inline void yascreen_reqsize(yascreen *s) { // {{{
	outs(s,ESC"[s"ESC"[999;999H"ESC"[6n"ESC"[u");
	out(s,"",0); // request a flush
} // }}}

