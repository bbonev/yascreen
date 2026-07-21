// $Id: yascreen_feed.c,v 1.11 2026/07/21 06:31:18 bbonev Exp $

// Copyright © 2015-2026 Boian Bonev (bbonev@ipacct.com) {{{
//
// SPDX-License-Identifer: LGPL-3.0-or-later
//
// This file is part of yascreen - yet another screen library.
//
// yascreen is free software, released under the terms of GNU Lesser General Public License v3.0 or later
// }}}

symver_V(V(yascreen_feed,V193),yascreen_feed,YASCREEN_193) // {{{
inline void V(yascreen_feed,V193)(yascreen *s,unsigned char c) {
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
			case TELNET_SIZE: // notify about screen size change w/o valid size data
				yascreen_pushch(s,YAS_TELNET_SIZE);
				return;
			case TELNET_SIZE_NAWS: // new valid screen size is received via NAWS
				yascreen_pushch(s,YAS_SCREEN_SIZE);
				return;
		}
	}

	switch (s->state) {
		case ST_ENTER:
			s->state=ST_NORM;
			if (c=='\n'||c==0) // ignore a single LF or NUL after CR
				break;
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
				if (!s->isunicode) { // do not process unicode sequences, push the byte as-is
					yascreen_pushch(s,c);
					break;
				}
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
				case 'A'...'N': // 'O' cannot be an alt-shift-letter, \eO is a key sequence prefix
				case 'P'...'Z':
				case ' ': // alt-space
				case 0x08: // alt-backspace (on terminals where backspace sends ^H)
				case 0x09: // alt-tab
				case 0x0d: // alt-enter
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
				case YAS_K_ESC:
					s->ansibuf[s->ansipos++]=c;
					s->state=ST_ESC_ESC;
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
				case 'a': // shift-up - \e[a
					yascreen_pushch(s,YAS_K_S_UP);
					s->state=ST_NORM;
					break;
				case 'b': // shift-down - \e[b
					yascreen_pushch(s,YAS_K_S_DOWN);
					s->state=ST_NORM;
					break;
				case 'c': // shift-right - \e[c
					yascreen_pushch(s,YAS_K_S_RIGHT);
					s->state=ST_NORM;
					break;
				case 'd': // shift-left - \e[d
					yascreen_pushch(s,YAS_K_S_LEFT);
					s->state=ST_NORM;
					break;
				case 'Z': // shift-tab - \e[Z
					yascreen_pushch(s,YAS_K_S_TAB);
					s->state=ST_NORM;
					break;
				case 'E': // keypad 5 - \e[E
				case 'G': // keypad 5 on linux console - \e[G
					yascreen_pushch(s,YAS_K_KP5);
					s->state=ST_NORM;
					break;
				case 'L': // insert on SCO console - \e[L
					yascreen_pushch(s,YAS_K_INS);
					s->state=ST_NORM;
					break;
				case '[': // linux console F1-F5 - \e[[A - \e[[E
					s->ansibuf[s->ansipos++]=c;
					s->state=ST_ESC_SQ_SQ;
					break;
				case '0'...'9':
					s->state=ST_ESC_SQ_D;
					s->ansibuf[s->ansipos++]=c;
					break;
				case 'M': // x10 mouse report - eat the 3 byte payload so it does not leak as keypresses
					s->state=ST_ESC_SQ_M;
					break;
				default:
					if (c>=0x20&&c<=0x3f) { // parameter/intermediate byte of an unsupported sequence - collect and discard
						s->state=ST_ESC_SQ_D;
						s->ansibuf[s->ansipos++]=c;
					} else // ignore unknown sequence
						s->state=ST_NORM;
					break;
			}
			break;
		case ST_ESC_SQ_M:
			s->ansipos++; // count the payload bytes, values are not used
			if (s->ansipos>=5) // \e[M plus 3 payload bytes
				s->state=ST_NORM;
			break;
		case ST_ESC_SQ_D:
			if (s->ansipos>=sizeof s->ansibuf-1) { // buffer overrun, ignore the sequence (keep 1 byte for NUL terminator)
				s->state=ST_NORM;
				break;
			}
			s->ansibuf[s->ansipos++]=c;
			if ((c>=0x40&&c<=0x7e)||c=='$') { // final char ('$' is rxvt shift-navigation, violates ecma-48)
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
						if (s->ansipos==5&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='0') // F9 - \e[20~
							yascreen_pushch(s,YAS_K_F9);
						if (s->ansipos==5&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='1') // F10 - \e[21~
							yascreen_pushch(s,YAS_K_F10);
						if (s->ansipos==5&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='3') // F11 - \e[23~
							yascreen_pushch(s,YAS_K_F11);
						if (s->ansipos==5&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='4') // F12 - \e[24~
							yascreen_pushch(s,YAS_K_F12);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='1'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='2') // shift-F1 \e[11;2~
							yascreen_pushch(s,YAS_K_S_F1);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='2'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='2') // shift-F2 \e[12;2~
							yascreen_pushch(s,YAS_K_S_F2);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='3'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='2') // shift-F3 \e[13;2~
							yascreen_pushch(s,YAS_K_S_F3);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='4'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='2') // shift-F4 \e[14;2~
							yascreen_pushch(s,YAS_K_S_F4);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='5'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='2') // shift-F5 \e[15;2~
							yascreen_pushch(s,YAS_K_S_F5);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='7'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='2') // shift-F6 \e[17;2~
							yascreen_pushch(s,YAS_K_S_F6);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='8'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='2') // shift-F7 \e[18;2~
							yascreen_pushch(s,YAS_K_S_F7);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='9'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='2') // shift-F8 \e[19;2~
							yascreen_pushch(s,YAS_K_S_F8);
						if (s->ansipos==7&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='0'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='2') // shift-F9 \e[20;2~
							yascreen_pushch(s,YAS_K_S_F9);
						if (s->ansipos==7&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='1'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='2') // shift-F10 \e[21;2~
							yascreen_pushch(s,YAS_K_S_F10);
						if (s->ansipos==7&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='3'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='2') // shift-F11 \e[23;2~
							yascreen_pushch(s,YAS_K_S_F11);
						if (s->ansipos==7&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='4'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='2') // shift-F12 \e[24;2~
							yascreen_pushch(s,YAS_K_S_F12);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='1'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='5') // ctrl-F1 \e[11;5~
							yascreen_pushch(s,YAS_K_C_F1);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='2'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='5') // ctrl-F2 \e[12;5~
							yascreen_pushch(s,YAS_K_C_F2);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='3'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='5') // ctrl-F3 \e[13;5~
							yascreen_pushch(s,YAS_K_C_F3);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='4'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='5') // ctrl-F4 \e[14;5~
							yascreen_pushch(s,YAS_K_C_F4);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='5'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='5') // ctrl-F5 \e[15;5~
							yascreen_pushch(s,YAS_K_C_F5);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='7'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='5') // ctrl-F6 \e[17;5~
							yascreen_pushch(s,YAS_K_C_F6);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='8'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='5') // ctrl-F7 \e[18;5~
							yascreen_pushch(s,YAS_K_C_F7);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='9'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='5') // ctrl-F8 \e[19;5~
							yascreen_pushch(s,YAS_K_C_F8);
						if (s->ansipos==7&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='0'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='5') // ctrl-F9 \e[20;5~
							yascreen_pushch(s,YAS_K_C_F9);
						if (s->ansipos==7&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='1'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='5') // ctrl-F10 \e[21;5~
							yascreen_pushch(s,YAS_K_C_F10);
						if (s->ansipos==7&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='3'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='5') // ctrl-F11 \e[23;5~
							yascreen_pushch(s,YAS_K_C_F11);
						if (s->ansipos==7&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='4'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='5') // ctrl-F12 \e[24;5~
							yascreen_pushch(s,YAS_K_C_F12);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='1'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='3') // alt-F1 \e[11;3~
							yascreen_pushch(s,YAS_K_A_F1);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='2'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='3') // alt-F2 \e[12;3~
							yascreen_pushch(s,YAS_K_A_F2);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='3'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='3') // alt-F3 \e[13;3~
							yascreen_pushch(s,YAS_K_A_F3);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='4'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='3') // alt-F4 \e[14;3~
							yascreen_pushch(s,YAS_K_A_F4);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='5'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='3') // alt-F5 \e[15;3~
							yascreen_pushch(s,YAS_K_A_F5);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='7'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='3') // alt-F6 \e[17;3~
							yascreen_pushch(s,YAS_K_A_F6);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='8'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='3') // alt-F7 \e[18;3~
							yascreen_pushch(s,YAS_K_A_F7);
						if (s->ansipos==7&&s->ansibuf[2]=='1'&&s->ansibuf[3]=='9'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='3') // alt-F8 \e[19;3~
							yascreen_pushch(s,YAS_K_A_F8);
						if (s->ansipos==7&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='0'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='3') // alt-F9 \e[20;3~
							yascreen_pushch(s,YAS_K_A_F9);
						if (s->ansipos==7&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='1'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='3') // alt-F10 \e[21;3~
							yascreen_pushch(s,YAS_K_A_F10);
						if (s->ansipos==7&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='3'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='3') // alt-F11 \e[23;3~
							yascreen_pushch(s,YAS_K_A_F11);
						if (s->ansipos==7&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='4'&&s->ansibuf[4]==';'&&s->ansibuf[5]=='3') // alt-F12 \e[24;3~
							yascreen_pushch(s,YAS_K_A_F12);
						if (s->ansipos==5&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='5') // F13 is shift-F1 - \e[25~
							yascreen_pushch(s,YAS_K_S_F1);
						if (s->ansipos==5&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='6') // F14 is shift-F2 - \e[26~
							yascreen_pushch(s,YAS_K_S_F2);
						if (s->ansipos==5&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='8') // F15 is shift-F3 - \e[28~
							yascreen_pushch(s,YAS_K_S_F3);
						if (s->ansipos==5&&s->ansibuf[2]=='2'&&s->ansibuf[3]=='9') // F16 is shift-F4 - \e[29~
							yascreen_pushch(s,YAS_K_S_F4);
						if (s->ansipos==5&&s->ansibuf[2]=='3'&&s->ansibuf[3]=='1') // F17 is shift-F5 - \e[31~
							yascreen_pushch(s,YAS_K_S_F5);
						if (s->ansipos==5&&s->ansibuf[2]=='3'&&s->ansibuf[3]=='2') // F18 is shift-F6 - \e[32~
							yascreen_pushch(s,YAS_K_S_F6);
						if (s->ansipos==5&&s->ansibuf[2]=='3'&&s->ansibuf[3]=='3') // F19 is shift-F7 - \e[33~
							yascreen_pushch(s,YAS_K_S_F7);
						if (s->ansipos==5&&s->ansibuf[2]=='3'&&s->ansibuf[3]=='4') // F20 is shift-F8 - \e[34~
							yascreen_pushch(s,YAS_K_S_F8);
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
						if (s->ansipos==6&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5') { // ctrl-ins/del/pgup/pgdn - \e[2;5~ \e[3;5~ \e[5;5~ \e[6;5~
							if (s->ansibuf[2]=='2')
								yascreen_pushch(s,YAS_K_C_INS);
							if (s->ansibuf[2]=='3')
								yascreen_pushch(s,YAS_K_C_DEL);
							if (s->ansibuf[2]=='5')
								yascreen_pushch(s,YAS_K_C_PGUP);
							if (s->ansibuf[2]=='6')
								yascreen_pushch(s,YAS_K_C_PGDN);
						}
						if (s->ansipos==6&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') { // shift-ins/del/pgup/pgdn - \e[2;2~ \e[3;2~ \e[5;2~ \e[6;2~
							if (s->ansibuf[2]=='2')
								yascreen_pushch(s,YAS_K_S_INS);
							if (s->ansibuf[2]=='3')
								yascreen_pushch(s,YAS_K_S_DEL);
							if (s->ansibuf[2]=='5')
								yascreen_pushch(s,YAS_K_S_PGUP);
							if (s->ansibuf[2]=='6')
								yascreen_pushch(s,YAS_K_S_PGDN);
						}
						if (s->ansipos==6&&s->ansibuf[3]==';'&&s->ansibuf[4]=='3') { // alt-ins/del/pgup/pgdn - \e[2;3~ \e[3;3~ \e[5;3~ \e[6;3~
							if (s->ansibuf[2]=='2')
								yascreen_pushch(s,YAS_K_A_INS);
							if (s->ansibuf[2]=='3')
								yascreen_pushch(s,YAS_K_A_DEL);
							if (s->ansibuf[2]=='5')
								yascreen_pushch(s,YAS_K_A_PGUP);
							if (s->ansibuf[2]=='6')
								yascreen_pushch(s,YAS_K_A_PGDN);
						}
						break;
					case '^': // rxvt ctrl-navigation
						if (s->ansipos==4&&s->ansibuf[2]=='2') // ctrl-ins - \e[2^
							yascreen_pushch(s,YAS_K_C_INS);
						if (s->ansipos==4&&s->ansibuf[2]=='3') // ctrl-del - \e[3^
							yascreen_pushch(s,YAS_K_C_DEL);
						if (s->ansipos==4&&s->ansibuf[2]=='5') // ctrl-pgup - \e[5^
							yascreen_pushch(s,YAS_K_C_PGUP);
						if (s->ansipos==4&&s->ansibuf[2]=='6') // ctrl-pgdn - \e[6^
							yascreen_pushch(s,YAS_K_C_PGDN);
						if (s->ansipos==4&&s->ansibuf[2]=='7') // ctrl-home - \e[7^
							yascreen_pushch(s,YAS_K_C_HOME);
						if (s->ansipos==4&&s->ansibuf[2]=='8') // ctrl-end - \e[8^
							yascreen_pushch(s,YAS_K_C_END);
						break;
					case '$': // rxvt shift-navigation
						if (s->ansipos==4&&s->ansibuf[2]=='2') // shift-ins - \e[2$
							yascreen_pushch(s,YAS_K_S_INS);
						if (s->ansipos==4&&s->ansibuf[2]=='3') // shift-del - \e[3$
							yascreen_pushch(s,YAS_K_S_DEL);
						if (s->ansipos==4&&s->ansibuf[2]=='5') // shift-pgup - \e[5$
							yascreen_pushch(s,YAS_K_S_PGUP);
						if (s->ansipos==4&&s->ansibuf[2]=='6') // shift-pgdn - \e[6$
							yascreen_pushch(s,YAS_K_S_PGDN);
						if (s->ansipos==4&&s->ansibuf[2]=='7') // shift-home - \e[7$
							yascreen_pushch(s,YAS_K_S_HOME);
						if (s->ansipos==4&&s->ansibuf[2]=='8') // shift-end - \e[8$
							yascreen_pushch(s,YAS_K_S_END);
						break;
					case 'P': // \e[1;mP - modified F1
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') // shift-F1 - \e[1;2P
							yascreen_pushch(s,YAS_K_S_F1);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5') // ctrl-F1 - \e[1;5P
							yascreen_pushch(s,YAS_K_C_F1);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='3') // alt-F1 - \e[1;3P
							yascreen_pushch(s,YAS_K_A_F1);
						break;
					case 'Q': // \e[1;mQ - modified F2
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') // shift-F2 - \e[1;2Q
							yascreen_pushch(s,YAS_K_S_F2);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5') // ctrl-F2 - \e[1;5Q
							yascreen_pushch(s,YAS_K_C_F2);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='3') // alt-F2 - \e[1;3Q
							yascreen_pushch(s,YAS_K_A_F2);
						break;
					case 'R': { // \e[n;mR - cursor position report, used for screen size detection
						int sx=0,sy=0;

						if (2==sscanf((char *)s->ansibuf+2,"%d;%dR",&sy,&sx)&&sx>10&&sy>3&&sx<=999&&sy<=999) { // ignore non-sane values
							s->scrx=sx;
							s->scry=sy;
							s->haveansi=1;
							yascreen_pushch(s,YAS_SCREEN_SIZE);
						} else if (!strcmp((char *)s->ansibuf+2,"1;2R")) // shift-F3 - \e[1;2R
							yascreen_pushch(s,YAS_K_S_F3);
						else if (!strcmp((char *)s->ansibuf+2,"1;5R")) // ctrl-F3 - \e[1;5R
							yascreen_pushch(s,YAS_K_C_F3);
						else if (!strcmp((char *)s->ansibuf+2,"1;3R")) // alt-F3 - \e[1;3R
							yascreen_pushch(s,YAS_K_A_F3);
						break;
					}
					case 'S': // \e[1;mS - modified F4
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') // shift-F4 - \e[1;2S
							yascreen_pushch(s,YAS_K_S_F4);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5') // ctrl-F4 - \e[1;5S
							yascreen_pushch(s,YAS_K_C_F4);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='3') // alt-F4 - \e[1;3S
							yascreen_pushch(s,YAS_K_A_F4);
						break;
					case 'H': // \e[1;mH - modified home
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5') // ctrl-home - \e[1;5H
							yascreen_pushch(s,YAS_K_C_HOME);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') // shift-home - \e[1;2H
							yascreen_pushch(s,YAS_K_S_HOME);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='3') // alt-home - \e[1;3H
							yascreen_pushch(s,YAS_K_A_HOME);
						break;
					case 'F': // \e[1;mF - modified end
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5') // ctrl-end - \e[1;5F
							yascreen_pushch(s,YAS_K_C_END);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') // shift-end - \e[1;2F
							yascreen_pushch(s,YAS_K_S_END);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='3') // alt-end - \e[1;3F
							yascreen_pushch(s,YAS_K_A_END);
						break;
					case 'Z': // \e[1;mZ or \e[Z with collected digits - shift-tab variants; treat any as shift-tab
						yascreen_pushch(s,YAS_K_S_TAB);
						break;
					case 'A':
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5') // ^up - \e[1;5A
							yascreen_pushch(s,YAS_K_C_UP);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') // shift-up - \e[1;2A
							yascreen_pushch(s,YAS_K_S_UP);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='3') // alt-up - \e[1;3A
							yascreen_pushch(s,YAS_K_A_UP);
						break;
					case 'B':
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5') // ^down - \e[1;5B
							yascreen_pushch(s,YAS_K_C_DOWN);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') // shift-down - \e[1;2B
							yascreen_pushch(s,YAS_K_S_DOWN);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='3') // alt-down - \e[1;3B
							yascreen_pushch(s,YAS_K_A_DOWN);
						break;
					case 'C':
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5') // ^right - \e[1;5C
							yascreen_pushch(s,YAS_K_C_RIGHT);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') // shift-right - \e[1;2C
							yascreen_pushch(s,YAS_K_S_RIGHT);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='3') // alt-right - \e[1;3C
							yascreen_pushch(s,YAS_K_A_RIGHT);
						break;
					case 'D':
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5') // ^left - \e[1;5D
							yascreen_pushch(s,YAS_K_C_LEFT);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') // shift-left - \e[1;2D
							yascreen_pushch(s,YAS_K_S_LEFT);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='3') // alt-left - \e[1;3D
							yascreen_pushch(s,YAS_K_A_LEFT);
						break;
				}
			}
			break;
		case ST_ESC_SQ_SQ:
			switch (c) {
				case 'A': // F1 \e[[A
					yascreen_pushch(s,YAS_K_F1);
					break;
				case 'B': // F2 \e[[B
					yascreen_pushch(s,YAS_K_F2);
					break;
				case 'C': // F3 \e[[C
					yascreen_pushch(s,YAS_K_F3);
					break;
				case 'D': // F4 \e[[D
					yascreen_pushch(s,YAS_K_F4);
					break;
				case 'E': // F5 \e[[E
					yascreen_pushch(s,YAS_K_F5);
					break;
			}
			s->state=ST_NORM;
			break;
		case ST_ESC_O:
			if (c>='0'&&c<='9') { // \eO<mod><key> - modified key, sent by some older xterm versions
				s->ansibuf[s->ansipos++]=c;
				s->state=ST_ESC_O_D;
				break;
			}
			switch (c) {
				case 'A': // up \eOA (application cursor mode)
					yascreen_pushch(s,YAS_K_UP);
					break;
				case 'B': // down \eOB
					yascreen_pushch(s,YAS_K_DOWN);
					break;
				case 'C': // right \eOC
					yascreen_pushch(s,YAS_K_RIGHT);
					break;
				case 'D': // left \eOD
					yascreen_pushch(s,YAS_K_LEFT);
					break;
				case 'M': // enter \eOM (application keypad mode)
					yascreen_pushch(s,YAS_K_RET);
					break;
				case 'E': // keypad 5 \eOE (application cursor mode)
					yascreen_pushch(s,YAS_K_KP5);
					break;
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
				// the F5-F12 codes below were captured from old versions of rxvt, xterm and the linux console
				// they follow the vt100 application keypad top two rows (7 8 9 - 4 5 6 ,) in physical order
				// the same codes mean keypad keys under DECKPAM, so do not reinterpret them without an application controlled mode
				case 'w': // F5 \eOw
					yascreen_pushch(s,YAS_K_F5);
					break;
				case 'x': // F6 \eOx
					yascreen_pushch(s,YAS_K_F6);
					break;
				case 'y': // F7 \eOy
					yascreen_pushch(s,YAS_K_F7);
					break;
				case 'm': // F8 \eOm
					yascreen_pushch(s,YAS_K_F8);
					break;
				case 't': // F9 \eOt
					yascreen_pushch(s,YAS_K_F9);
					break;
				case 'u': // F10 \eOu
					yascreen_pushch(s,YAS_K_F10);
					break;
				case 'v': // F11 \eOv
					yascreen_pushch(s,YAS_K_F11);
					break;
				case 'l': // F12 \eOl
					yascreen_pushch(s,YAS_K_F12);
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
		case ST_ESC_O_D: {
			unsigned char mod=0;

			if (s->ansipos>=sizeof s->ansibuf-1) { // buffer overrun, ignore the sequence (keep 1 byte for NUL terminator)
				s->state=ST_NORM;
				break;
			}
			s->ansibuf[s->ansipos++]=c;
			if ((c>='0'&&c<='9')||c==';') // collect modifier digits
				break;
			s->state=ST_NORM;
			if (s->ansipos==4) // \eO<mod><key> with single digit modifier
				mod=s->ansibuf[2];
			if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';') // \eO1;<mod><key> form
				mod=s->ansibuf[4];
			switch (c) {
				case 'P': // modified F1 - \eO2P \eO5P \eO3P
					if (mod=='2')
						yascreen_pushch(s,YAS_K_S_F1);
					if (mod=='5')
						yascreen_pushch(s,YAS_K_C_F1);
					if (mod=='3')
						yascreen_pushch(s,YAS_K_A_F1);
					break;
				case 'Q': // modified F2 - \eO2Q \eO5Q \eO3Q
					if (mod=='2')
						yascreen_pushch(s,YAS_K_S_F2);
					if (mod=='5')
						yascreen_pushch(s,YAS_K_C_F2);
					if (mod=='3')
						yascreen_pushch(s,YAS_K_A_F2);
					break;
				case 'R': // modified F3 - \eO2R \eO5R \eO3R
					if (mod=='2')
						yascreen_pushch(s,YAS_K_S_F3);
					if (mod=='5')
						yascreen_pushch(s,YAS_K_C_F3);
					if (mod=='3')
						yascreen_pushch(s,YAS_K_A_F3);
					break;
				case 'S': // modified F4 - \eO2S \eO5S \eO3S
					if (mod=='2')
						yascreen_pushch(s,YAS_K_S_F4);
					if (mod=='5')
						yascreen_pushch(s,YAS_K_C_F4);
					if (mod=='3')
						yascreen_pushch(s,YAS_K_A_F4);
					break;
				case 'A': // modified up - \eO2A \eO5A \eO3A
					if (mod=='2')
						yascreen_pushch(s,YAS_K_S_UP);
					if (mod=='5')
						yascreen_pushch(s,YAS_K_C_UP);
					if (mod=='3')
						yascreen_pushch(s,YAS_K_A_UP);
					break;
				case 'B': // modified down - \eO2B \eO5B \eO3B
					if (mod=='2')
						yascreen_pushch(s,YAS_K_S_DOWN);
					if (mod=='5')
						yascreen_pushch(s,YAS_K_C_DOWN);
					if (mod=='3')
						yascreen_pushch(s,YAS_K_A_DOWN);
					break;
				case 'C': // modified right - \eO2C \eO5C \eO3C
					if (mod=='2')
						yascreen_pushch(s,YAS_K_S_RIGHT);
					if (mod=='5')
						yascreen_pushch(s,YAS_K_C_RIGHT);
					if (mod=='3')
						yascreen_pushch(s,YAS_K_A_RIGHT);
					break;
				case 'D': // modified left - \eO2D \eO5D \eO3D
					if (mod=='2')
						yascreen_pushch(s,YAS_K_S_LEFT);
					if (mod=='5')
						yascreen_pushch(s,YAS_K_C_LEFT);
					if (mod=='3')
						yascreen_pushch(s,YAS_K_A_LEFT);
					break;
				case 'H': // modified home - \eO2H \eO5H \eO3H
					if (mod=='2')
						yascreen_pushch(s,YAS_K_S_HOME);
					if (mod=='5')
						yascreen_pushch(s,YAS_K_C_HOME);
					if (mod=='3')
						yascreen_pushch(s,YAS_K_A_HOME);
					break;
				case 'F': // modified end - \eO2F \eO5F \eO3F
					if (mod=='2')
						yascreen_pushch(s,YAS_K_S_END);
					if (mod=='5')
						yascreen_pushch(s,YAS_K_C_END);
					if (mod=='3')
						yascreen_pushch(s,YAS_K_A_END);
					break;
			}
			break;
		}
		case ST_ESC_ESC:
			if (s->ansipos>=sizeof s->ansibuf-1) { // buffer overrun, ignore the sequence (keep 1 byte for NUL terminator)
				s->state=ST_NORM;
				break;
			}
			s->ansibuf[s->ansipos++]=c;
			if (c>=0x40&&c<=0x7e&&c!=0x5b&&!(c==0x4f&&s->ansipos==3)) { // final char; 'O' right after \e\e starts an alt-SS3 sequence
				s->state=ST_NORM;
				s->ansibuf[s->ansipos]=0;
				switch (c) {
					case 'A':
						if (s->ansipos==4&&(s->ansibuf[2]=='['||s->ansibuf[2]=='O')) // alt-up - \e\e[A \e\eOA
							yascreen_pushch(s,YAS_K_A_UP);
						break;
					case 'B':
						if (s->ansipos==4&&(s->ansibuf[2]=='['||s->ansibuf[2]=='O')) // alt-down - \e\e[B \e\eOB
							yascreen_pushch(s,YAS_K_A_DOWN);
						break;
					case 'C':
						if (s->ansipos==4&&(s->ansibuf[2]=='['||s->ansibuf[2]=='O')) // alt-right - \e\e[C \e\eOC
							yascreen_pushch(s,YAS_K_A_RIGHT);
						break;
					case 'D':
						if (s->ansipos==4&&(s->ansibuf[2]=='['||s->ansibuf[2]=='O')) // alt-left - \e\e[D \e\eOD
							yascreen_pushch(s,YAS_K_A_LEFT);
						break;
					case 'H':
						if (s->ansipos==4&&(s->ansibuf[2]=='['||s->ansibuf[2]=='O')) // alt-home - \e\e[H \e\eOH
							yascreen_pushch(s,YAS_K_A_HOME);
						break;
					case 'F':
						if (s->ansipos==4&&(s->ansibuf[2]=='['||s->ansibuf[2]=='O')) // alt-end - \e\e[F \e\eOF
							yascreen_pushch(s,YAS_K_A_END);
						break;
					case 'P':
						if (s->ansipos==4&&s->ansibuf[2]=='O') // alt-F1 - \e\eOP
							yascreen_pushch(s,YAS_K_A_F1);
						break;
					case 'Q':
						if (s->ansipos==4&&s->ansibuf[2]=='O') // alt-F2 - \e\eOQ
							yascreen_pushch(s,YAS_K_A_F2);
						break;
					case 'R':
						if (s->ansipos==4&&s->ansibuf[2]=='O') // alt-F3 - \e\eOR
							yascreen_pushch(s,YAS_K_A_F3);
						break;
					case 'S':
						if (s->ansipos==4&&s->ansibuf[2]=='O') // alt-F4 - \e\eOS
							yascreen_pushch(s,YAS_K_A_F4);
						break;
					case '~':
						if (s->ansipos==5&&s->ansibuf[2]=='['&&(s->ansibuf[3]=='1'||s->ansibuf[3]=='7')) // alt-home - \e\e[1~ \e\e[7~
							yascreen_pushch(s,YAS_K_A_HOME);
						if (s->ansipos==5&&s->ansibuf[2]=='['&&(s->ansibuf[3]=='4'||s->ansibuf[3]=='8')) // alt-end - \e\e[4~ \e\e[8~
							yascreen_pushch(s,YAS_K_A_END);
						if (s->ansipos==5&&s->ansibuf[2]=='['&&s->ansibuf[3]=='2') // alt-ins - \e\e[2~
							yascreen_pushch(s,YAS_K_A_INS);
						if (s->ansipos==5&&s->ansibuf[2]=='['&&s->ansibuf[3]=='3') // alt-del - \e\e[3~
							yascreen_pushch(s,YAS_K_A_DEL);
						if (s->ansipos==5&&s->ansibuf[2]=='['&&s->ansibuf[3]=='5') // alt-pgup - \e\e[5~
							yascreen_pushch(s,YAS_K_A_PGUP);
						if (s->ansipos==5&&s->ansibuf[2]=='['&&s->ansibuf[3]=='6') // alt-pgdn - \e\e[6~
							yascreen_pushch(s,YAS_K_A_PGDN);
						break;
				}
			}
			break;
	}
} // }}}
