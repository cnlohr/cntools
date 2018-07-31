#define _GNU_SOURCE
#include <stdlib.h>
#include <CNFGFunctions.h>
#include <stdio.h>
#include <unistd.h>
#include <os_generic.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include "vlinterm.h"

int spawn_process_with_pts( const char * execparam, char * const argv[], int * pid );

#define INIT_CHARX  80
#define INIT_CHARY  25
#define CHAR_DOUBLE 0   //Set to 1 to double size, or 0 for normal size.

int charset_w, charset_h, font_w, font_h;
uint32_t * font;

int LoadFont(const char * fontfile)
{

	FILE * f = fopen( fontfile, "rb" );
	int c = 0, i;
	int format;
	if( !f ) { fprintf( stderr, "Error: cannot open font file %s\n", fontfile ); return -1; }
	if( (c = fgetc(f)) != 'P' ) { fprintf( stderr, "Error: Cannot parse first line of font file [%d].\n", c ); return -2; } 
	format = fgetc(f);
	fgetc(f);
	while( (c = getc(f)) != EOF ) { if( c == '\n' ) break; }	//Comment
	if( fscanf( f, "%d %d\n", &charset_w, &charset_h ) != 2 ) { fprintf( stderr, "Error: No size in pgm.\n" ); return -3; }
	while( (c = getc(f)) != EOF ) if( c == '\n' ) break;	//255
	if( (charset_w & 0x0f) || (charset_h & 0x0f) ) { fprintf( stderr, "Error: charset must be divisible by 16 in both dimensions.\n" ); return -4; }
	font = malloc( charset_w * charset_h * 4 );
	for( i = 0; i < charset_w * charset_h; i++ ) { 
		if( format == '5' )
		{
			c = getc(f);
			font[i] = c | (c<<8) | (c<<16);
		}
		else
		{
			fprintf( stderr, "Error: unsupported font image format.  Must be P5\n" );
			return -5;
		}
	}
	font_w = charset_w / 16;
	font_h = charset_h / 16;
	fclose( f );

	return 0;
}

struct TermStructure ts;



void * rxthread( void * v )
{
	struct TermStructure * ts = v;

    while( 1 )
	{
		uint8_t rxdata[1024+1];
		int rx = read( ts->ptspipe, rxdata, 1024 );
		if( rx < 0 ) break;
		int i;
		for( i = 0; i < rx; i++ )
		{
			char crx = rxdata[i];
			EmitChar( ts, crx );
		}
	}
	fprintf( stderr, "Error: Terminal pipe died\n" ); 
	close( ts->ptspipe );
	exit( 0 );
	return 0;
}


void HandleOSCCommand( struct TermStructure * ts, int parameter, const char * value )
{
	printf( "OSC Command: %d: %s\n", parameter, value );
}

void HandleBell( struct TermStructure * ts )
{
	printf( "BELL\n" );
}


extern int g_x_do_key_decode;

void HandleKey( int keycode, int bDown )
{
	static int shift_held;
	static int ctrl_held;
	static int alt_held;

	if( keycode == 65506 )
	{
		shift_held = bDown;
	}
	else if( keycode == 65507 )
	{
		ctrl_held = bDown;
	} 
	else if( keycode == 65513 )
	{
		alt_held = bDown;
	}
	else if( bDown )
	{
		int len = 0;
		const char * str = 0;
		char cc[3] = { keycode };

		if( keycode == 65293 || keycode == 65421 )
		{
			//Enter key: Be careful with these...
			cc[0] = '\x0d';
			len = 1;
			if( ts.dec_private_mode & (1<<20) )
			{
				cc[1] = '\x0a';
				len = 2;
			}
			str = cc;
		}
		else if( keycode >= 255 )
		{
			struct KeyLooup
			{
				unsigned short key;
				short stringlen;
				const char * string;
			};
			const struct KeyLooup keys[] = {
				{ 65288, 1, "\x08" },
				{ 65289, 1, "\x09" },
				{ 65307, 1, "\x1b" },
				{ 65362, 3, "\x1b[A" },  //Up
				{ 65364, 3, "\x1b[B" },  //Down
				{ 65361, 3, "\x1b[D" },  //Left
				{ 65363, 3, "\x1b[C" },  //Right
				{ 65366, 4, "\x1b[6~" }, //PgDn
				{ 65365, 4, "\x1b[5~" }, //PgUp
				{ 65367, 3, "\x1b[F" },  //Home
				{ 65360, 3, "\x1b[H" },  //End
				{ 65535, 4, "\x1b[3~" }, //Del
				{ 65379, 4, "\x1b[4~" }, //Ins
				{ 255, 0, "" },
			};
			int i;
			for( i = 0; i < sizeof(keys)/sizeof(keys[0]); i++ )
			{
				if( keys[i].key == keycode )
				{
					len = keys[i].stringlen;
					str = keys[i].string;
					break;
				}
			}
			if( i == sizeof(keys)/sizeof(keys[0]) ) fprintf( stderr, "Unmapped key %d\n", keycode );
		} else
		{
			extern int g_x_global_key_state;
			extern int g_x_global_shift_key;
			if( (g_x_global_key_state & 2) && !(g_x_global_key_state & 2) )
			{
				if( keycode >= 'a' && keycode <= 'z' )
					keycode = g_x_global_shift_key;
			}
			else if( g_x_global_key_state & 1 )
			{
				keycode = g_x_global_shift_key;
			}
			else if( ctrl_held )
			{
				if( keycode == 'c' )
				{
					kill( tcgetpgrp(ts.ptspipe), SIGINT );
					return;
				}
				else if( keycode >= 'a' && keycode <= 'z' )
				{
					keycode = keycode - 'a' + 1;
				}
				else if( keycode == ']' )
				{
					keycode = 0x1d;
				}
			}
			cc[0] = keycode;
			str = cc;
			len = 1;
		}

		int i;
		for( i = 0; i < len; i++ )
		{
			FeedbackTerminal( &ts, str + i, 1 );
			if( ts.echo ) EmitChar( &ts, str[i] );
		}
	}
}

void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) { }
void HandleDestroy() { }


uint32_t TermColor( uint32_t incolor, int color, int attrib )
{
	//XXX TODO: Do something clever here to allow for cool glyphs.
	int actc;
	int in_inten = (incolor&0xff) + ( (incolor&0xff00)>>8 ) + (( incolor&0xff0000 )>> 16 );

	if( (in_inten > 384 ) ^ (!!(attrib&(1<<4))) )
	{
		actc = color & 0x0f;
	}
	else
	{
		actc = (color >> 4) & 0x0f;
	}

	int base_color = (attrib & 1)?0xff:0xaf;
	if( attrib & 1 ) base_color = 0xff;

	uint32_t outcolor = 0;// = incolor
	if( actc & 4 ) outcolor |= base_color;
	if( actc & 2 ) outcolor |= base_color<<8;
	if( actc & 1 ) outcolor |= base_color<<16;
//	if(  (attrib&(1<<4)) ) { outcolor = (in_inten > 384 )?0x0000ff:0xffff00; printf( "%d\n", actc ) ; }

	return outcolor | 0xff000000;
}

int main()
{
	if( LoadFont("ibm437.pgm") ) return -1;

	ts.screen_mutex = OGCreateMutex();
	ts.charx = INIT_CHARX;
	ts.chary = INIT_CHARY;
	ts.echo = 0;
	short w = ts.charx * font_w * (CHAR_DOUBLE+1);
	short h = ts.chary * font_h * (CHAR_DOUBLE+1);
	CNFGSetup( "Test terminal", w, h );

	uint32_t * framebuffer = malloc( ts.charx * ts.chary * font_w * font_h * 4 *(CHAR_DOUBLE+1)*(CHAR_DOUBLE+1));
	ts.termbuffer = 0;
	ResetTerminal( &ts );
	char * localargv[] = { "/bin/bash", 0 };
	ts.ptspipe = spawn_process_with_pts( "/bin/bash", localargv, &ts.pid );
	ResizeScreen( &ts, ts.charx, ts.chary );

	OGCreateThread( rxthread, (void*)&ts );

	usleep(10000);
//	FeedbackTerminal( &ts, "script /dev/null\n", 17 );
//	FeedbackTerminal( &ts, "export TERM=linux\n", 18 );

	int last_curx = 0, last_cury = 0;
	while(1)
	{
		int x, y;
		CNFGHandleInput();
		short screenx, screeny;
		CNFGGetDimensions( &screenx, &screeny );
		int newx = screenx/font_w;
		int newy = screeny/font_h;

		if( (newx != ts.charx || newy != ts.chary) && newy > 0 && newx > 0 )
		{
			ResizeScreen( &ts, newx, newy );
			w = ts.charx * font_w * (CHAR_DOUBLE+1);
			h = ts.chary * font_h * (CHAR_DOUBLE+1);
			framebuffer = realloc( framebuffer, w*h*4 );
			if( last_cury >= ts.chary ) last_cury = ts.chary-1;
		}

		if( last_curx != ts.curx || last_cury != ts.cury || ts.tainted )
		{
			OGLockMutex( ts.screen_mutex );
			uint32_t * tb = ts.termbuffer;
			ts.tainted = 0;
			if( last_curx < ts.charx && last_cury < ts.chary )
				tb[last_curx+last_cury*ts.charx] |= 1<<24;
			if( ts.curx < ts.charx && ts.cury < ts.chary )
				tb[ts.curx + ts.cury * ts.charx] |= 1<<24;
			last_curx = ts.curx;
			last_cury = ts.cury;

			for( y = 0; y < ts.chary; y++ )
			for( x = 0; x < ts.charx; x++ )
			{
				uint32_t v = tb[x+y*ts.charx];
				if( ! (v & (1<<24)) ) continue;
				tb[x+y*ts.charx] &= ~(1<<24);
				unsigned char c = v & 0xff;
				int color = v>>16;
				int attrib = v>>8;

				int cx, cy;
				int atlasx = c & 0x0f;
				int atlasy = c >> 4;
				uint32_t * fbo =   &framebuffer[x*font_w*(CHAR_DOUBLE+1) + y*font_h*w*(CHAR_DOUBLE+1)];
				uint32_t * start = &font[atlasx*font_w+atlasy*font_h*charset_w];
				int is_cursor = (x == ts.curx && y == ts.cury) && (ts.dec_private_mode & (1<<25));
		#if CHAR_DOUBLE==1
				for( cy = 0; cy < font_h; cy++ )
				{
					for( cx = 0; cx < font_w; cx++ )
					{
						fbo[cx*2+0] = fbo[cx*2+1] = (is_cursor?0xffffffff:0)^TermColor( start[cx], color, attrib );
					}
					fbo += w;
					for( cx = 0; cx < font_w; cx++ )
					{
						fbo[cx*2+0] = fbo[cx*2+1] = (is_cursor?0xffffffff:0)^TermColor( start[cx], color, attrib );
					}
					fbo += w;
					start += charset_w;
				}
		#else
				for( cy = 0; cy < font_h; cy++ )
				{
					for( cx = 0; cx < font_w; cx++ )
					{
						fbo[cx] = (is_cursor?0xffffffff:0)^TermColor( start[cx], color, attrib );
					}
					fbo += w;
					start += charset_w;
				}
		#endif
			}
			OGUnlockMutex( ts.screen_mutex );
			CNFGUpdateScreenWithBitmap( (unsigned long *)framebuffer, w, h );
		}
		else
		{
			static int waittoupdate;
			if( waittoupdate++ == 10 )
			{
				waittoupdate = 0;
				CNFGUpdateScreenWithBitmap( (unsigned long *)framebuffer, w, h );
			}
			
			
		}
		usleep(20000);
	//	CNFGSwapBuffers();
	}

}

















#ifdef DO_EXTRA_ST_WORK
#include <pwd.h>
#endif

int spawn_process_with_pts( const char * execparam, char * const argv[], int * pid )
{
	int r = getpt();
	if( r <= 0 ) return -1;
	if( grantpt(r) ) return -2;
	if( unlockpt( r ) ) return -3;
	char slavepath[64];
	if(ptsname_r(r, slavepath, sizeof(slavepath)) < 0)
	{
		return -4;
	}


	int rforkv = fork();
	if (rforkv == 0)
	{
		close( r ); //Close master
		close( 0 );
		close( 1 );
		close( 2 );
		r = open( slavepath, O_RDWR | O_NOCTTY ); //Why did the previous example have the O_NOCTTY flag?
		//if (ioctl(r, TIOCSCTTY, NULL) < 0)
		//	fprintf(stderr, "ioctl TIOCSCTTY failed: %s\n", strerror(errno));
		dup2( r, 0 );
		dup2( r, 1 );
		dup2( r, 2 );
		setsid();

		//From ST
		{
#ifdef DO_EXTRA_ST_WORK
			const struct passwd *pw;
			char *sh, *prog;

			errno = 0;
			if ((pw = getpwuid(getuid())) == NULL) {
				if (errno)
					fprintf( stderr, "getpwuid:%s\n", strerror(errno));
				else
					fprintf( stderr, "who are you?\n");
			}

			if ((sh = getenv("SHELL")) == NULL)
				sh = (pw->pw_shell[0]) ? pw->pw_shell : "bash";

			prog = sh;
			//DEFAULT(args, ((char *[]) {prog, NULL}));

			unsetenv("COLUMNS");
			unsetenv("LINES");
			unsetenv("TERMCAP");
			setenv("LOGNAME", pw->pw_name, 1);
			setenv("USER", pw->pw_name, 1);
			setenv("SHELL", sh, 1);
			setenv("HOME", pw->pw_dir, 1);
#endif
			setenv("TERM", "xterm", 1);
		}
		execvp( execparam, argv );
	}
	else
	{
		//https://stackoverflow.com/questions/13849582/sending-signal-to-a-forked-process-that-calls-exec
		//setpgid(rforkv, 0);   //Why is this mutually exclusive with setsid()?
		if( rforkv < 0 )
		{
			close( r );
			return -10;
		}
		*pid = rforkv;
		return r;
	}
}


