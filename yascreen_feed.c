// $Id: yascreen_feed.c,v 1.3 2023/02/02 00:06:00 bbonev Exp $

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
					case 'P': // \e[1;2P - shift-F1
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') // shift-F1 - \e[1;2P
							yascreen_pushch(s,YAS_K_S_F1);
						break;
					case 'Q': // \e[1;2Q - shift-F2
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') // shift-F2 - \e[1;2Q
							yascreen_pushch(s,YAS_K_S_F2);
						break;
					case 'R': { // \e[n;mR - cursor position report, used for screen size detection
						int sx,sy;

						sscanf((char *)s->ansibuf+2,"%d;%dR",&sy,&sx);
						if (sx>10&&sy>3&&sx<=999&&sy<=999) { // ignore non-sane values
							s->scrx=sx;
							s->scry=sy;
							s->haveansi=1;
							yascreen_pushch(s,YAS_SCREEN_SIZE);
						} else if (!strcmp((char *)s->ansibuf+2,"1;2R")) // shift-F3 - \e[1;2R
							yascreen_pushch(s,YAS_K_S_F3);
						break;
					}
					case 'S': // \e[1;2S - shift-F4
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') // shift-F4 - \e[1;2S
							yascreen_pushch(s,YAS_K_S_F4);
						break;
					case 'A':
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5') // ^up - \e[1;5A
							yascreen_pushch(s,YAS_K_C_UP);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') // shift-up - \e[1;2A
							yascreen_pushch(s,YAS_K_S_UP);
						break;
					case 'B':
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5') // ^down - \e[1;5B
							yascreen_pushch(s,YAS_K_C_DOWN);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') // shift-down - \e[1;2B
							yascreen_pushch(s,YAS_K_S_DOWN);
						break;
					case 'C':
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5') // ^right - \e[1;5C
							yascreen_pushch(s,YAS_K_C_RIGHT);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') // shift-right - \e[1;2C
							yascreen_pushch(s,YAS_K_S_RIGHT);
						break;
					case 'D':
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='5') // ^left - \e[1;5D
							yascreen_pushch(s,YAS_K_C_LEFT);
						if (s->ansipos==6&&s->ansibuf[2]=='1'&&s->ansibuf[3]==';'&&s->ansibuf[4]=='2') // shift-left - \e[1;2D
							yascreen_pushch(s,YAS_K_S_LEFT);
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
	}
} // }}}
