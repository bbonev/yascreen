#include <ctype.h>
#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include <yascreen.h>

#define SIZEX 20
#define SIZEY 20

inline void yascreen_dump(yascreen *s);

int main(void) {
	//uint32_t s1=YAS_ITALIC|YAS_FGCOLOR(1)|YAS_BGCOLOR(2); // red on green italic
	//uint32_t s2=YAS_UNDERL|YAS_FGCOLOR(2)|YAS_BGCOLOR(1); // green on red under
	//uint32_t s3=YAS_FGCOLORDEF|YAS_BGCOLORDEF; // default colors
	uint32_t s4=YAS_FGCOLOR(YAS_BLACK)|YAS_BGCOLOR(YAS_WHITE)|YAS_BLINK|YAS_ITALIC|YAS_BOLD; // distinct colors
	int i;
	// int j;
	int ccnt=0;
	int chrs[SIZEY];
	//FILE *f=fopen("/dev/null","w");

	yascreen *s=yascreen_init(SIZEX,SIZEY);

	setlocale(LC_ALL,"C.UTF-8");

	if (!s) {
		printf("couldn't init screen %dx%d\n",SIZEX,SIZEY);
		return 0;
	}

	#if 0 // test 1
	yascreen_cursor(s,0);

	printf("style: %08x fg: %d bg: %d\n",s1,YAS_FG(s1),YAS_BG(s1));
	yascreen_putsxy(s,0,0,s1,"abcdef日̀本aaaa");
	yascreen_putsxy(s,1,3,s2,"±µя̀́и́̀̂a̴̴̷̠̰̐");
	yascreen_putsxy(s,5,3,s1,"ź;wllllccc");
	//yascreen_dump(s);

	yascreen_update(s);
	sleep(1);
	yascreen_putsxy(s,0,1,s2,"abcdef日̀本aaaa");
	yascreen_putsxy(s,1,4,s1,"±µя̀́и́̀̂a̴̴̷̠̰̐");
	yascreen_putsxy(s,5,4,s2,"ź;wllllccc");
	yascreen_update(s);
	sleep(1);
	yascreen_putsxy(s,0,2,s3,"abcdef日̀本aaaa");
	yascreen_putsxy(s,1,5,s3,"±µя̀́и́̀̂a̴̴̷̠̰̐");
	yascreen_putsxy(s,5,5,s3,"ź;wllllccc");
	yascreen_update(s);
	sleep(1);
	yascreen_putsxy(s,0,0,s3,"                 ");
	yascreen_putsxy(s,0,1,s3,"                 ");
	yascreen_putsxy(s,0,2,s3,"                 ");
	yascreen_putsxy(s,0,3,s3,"                 ");
	yascreen_putsxy(s,0,4,s3,"                 ");
	yascreen_putsxy(s,0,5,s3,"                 ");
	yascreen_update(s);
	sleep(1);

	for (i=0;i<9;i++)
		for (j=0;j<9;j++) {
			uint32_t st=YAS_FGCOLOR(i)|YAS_BGCOLOR(j);

			if (i==8||j==8)
				st=0;
			yascreen_putsxy(s,i*2,j,st,"日̀");
		}
	yascreen_putsxy(s,0,9,0,"0123456789012345678901234567890");
	yascreen_update(s);
	sleep(2);

	yascreen_cursor(s,1);
	#endif

	#if 0 // test 2
	yascreen_putsxy(s,5,5,50,"abc");
	yascreen_update(s);
	sleep(1);
	yascreen_putsxy(s,6,5,50,"B");
	yascreen_update(s);
	#endif

	#if 1 // test 3
	yascreen_term_set(s,YAS_NOBUFF|YAS_NOSIGN|YAS_NOECHO);
	yascreen_cursor(s,0);
	yascreen_altbuf(s,1);
	yascreen_clear(s);
	yascreen_printxy(s,0,0,YAS_BLINK,"press 'q' to quit%*s",SIZEY,"");
	yascreen_printxy(s,0,1,YAS_BLINK,"press 't' to perform test%*s",SIZEY,"");
	yascreen_update(s);
	//yascreen_ungetch(s,'1');
	//yascreen_ungetch(s,'2');
	//yascreen_ungetch(s,'3');
	//yascreen_pushch(s,'a');
	//yascreen_pushch(s,'b');
	//yascreen_pushch(s,'c');
	for (;;) {
		int ch;

		ch=yascreen_getch(s);
		switch (ch) {
			case 't': // test for reuse
				yascreen_clear(s);
				yascreen_cursor(s,1);
				yascreen_altbuf(s,0);
				yascreen_term_restore(s);
				sleep(2);
				yascreen_term_set(s,YAS_NOBUFF|YAS_NOSIGN|YAS_NOECHO);
				yascreen_cursor(s,0);
				yascreen_altbuf(s,1);
				yascreen_redraw(s);
				yascreen_update(s);
				break;
			case 'q': // exit
				yascreen_clear(s);
				yascreen_cursor(s,1);
				yascreen_altbuf(s,0);
				yascreen_term_restore(s);
				yascreen_free(s);

				printf("sizeof wchar_t: %zu\n",sizeof(wchar_t));

				return 0;
			default:
				if (ccnt<SIZEY)
					chrs[ccnt++]=ch;
				else {
					memmove(chrs,chrs+1,sizeof(int)*(ccnt-1));
					chrs[SIZEY-1]=ch;
				}
				for (i=0;i<ccnt;i++)
					yascreen_printxy(s,0,i,s4,"%02d: %d [%02x] %c%*s",i,chrs[i],chrs[i],(chrs[i]<=255&&isprint(chrs[i]))?chrs[i]:'_',SIZEY,"");
				yascreen_update(s);
		}
	}
	#endif
}
