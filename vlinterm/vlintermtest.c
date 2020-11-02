#define _GNU_SOURCE
#include <stdlib.h>
#include <CNFG.h>
#include <stdio.h>
#include <unistd.h>
#include <os_generic.h>
#include <string.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include "vlinterm.h"

int spawn_process_with_pts( const char * execparam, char * const argv[], int * pid );

#define INIT_CHARX  80
#define INIT_CHARY  25

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

char NewWindowTitle[1024];
int UpdateWindowTitle = 0;

void HandleOSCCommand( struct TermStructure * ts, int parameter, const char * value )
{
	if( parameter == 0 ) UpdateWindowTitle = snprintf( NewWindowTitle, 1023, "%s", value );
	else
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

	if( keycode == 65506 || keycode == 65505 )
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
		if( shift_held && keycode == 65366 )
		{
			TermScroll( &ts, -ts.chary/2 );
		}
		else if( shift_held && keycode == 65365 )
		{
			TermScroll( &ts, ts.chary/2 );
		}
		else
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
				ts.scrollback = 0;
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
					{ 65362, 3, "\x1b[A" },  //Up
					{ 65364, 3, "\x1b[B" },  //Down
					{ 65361, 3, "\x1b[D" },  //Left
					{ 65363, 3, "\x1b[C" },  //Right
					{ 65288, 1, "\x08" },
					{ 65289, 1, "\x09" },
					{ 65307, 1, "\x1b" },
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
				if( i < 4 && ( ts.dec_private_mode & 2 ) ) 			//Handle DECCKM, not sure why but things like alsamixer require it.
				{
					cc[0] = '\x1b';
					cc[1] = 'O';
					cc[2] = str[2];
					str = cc;
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
					if( keycode >= 'a' && keycode <= 'z' )
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
}

void HandleButton( int x, int y, int button, int bDown )
{
	if( (button == 5 || button == 4) && bDown )
	{
		if( ts.dec_private_mode & (1<<29) )
		{
			char cc[3] = { '\x1b', '[', (button == 4)?'A':'B' };
			FeedbackTerminal( &ts, cc, 3 );
		}
		else
		{
			TermScroll( &ts, (button==5)?-1:1 );
		}
	}
	else if( ts.dec_private_mode & (1<<30) )
	{
		x /= font_w;
		y /= font_h;
		char cc[6] = { '\x1b', '[', 'M', (bDown?0x20:0x23)+button-1, x+0x21, y+0x21 };
		FeedbackTerminal( &ts, cc, 6 );
	}
}

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
		if( actc == 0 && (attrib&(1<<5)) ) actc = 8;
	}
	else
	{
		actc = (color >> 4) & 0x0f;
		if( actc == 0 && (attrib&(1<<6)) ) actc = 8;
	}

	int base_color = (attrib & 9)?0xff:0xaf;
	if( attrib & 1 ) base_color = 0xff; //??? This looks wrong?
	uint32_t outcolor = 0;// = incolor

	if( actc & 4 ) outcolor |= base_color;
	if( actc & 2 ) outcolor |= base_color<<8;
	if( actc & 1 ) outcolor |= base_color<<16;
	if( actc & 8 ) outcolor = 0x202020; //Color was set, but was a soft-black
	if( (attrib & 8) && !(actc & 7) ) outcolor = 0x606060;

	return outcolor | 0xff000000;
}

int main( int argc, char ** argv )
{
	uint32_t * framebuffer = 0;
	int rawdrawtext = 0;
	int CHAR_DOUBLE = 0;

	{
		int c;
		opterr = 0;
		while ((c = getopt (argc, argv, "hdr")) != -1)
		switch (c)
		{
		case 'r':
			rawdrawtext = 1;
			break;
		case 'd':
			CHAR_DOUBLE = 1;
			break;
		case 'h':
		default:
			fprintf (stderr, "Options: -h, -r, -d\n" );
		    return -5;
		}
	}


	if( !rawdrawtext )
	{
		if( LoadFont("ibm437.pgm") ) return -1;
	}
	else
	{
		font_w = 6;
		font_h = 12;
	}

	ts.screen_mutex = OGCreateMutex();
	ts.charx = INIT_CHARX;
	ts.chary = INIT_CHARY;
	ts.echo = 0;
	ts.historyy = 1000;
	ts.termbuffer = 0;

	short w = ts.charx * font_w * (CHAR_DOUBLE+1);
	short h = ts.chary * font_h * (CHAR_DOUBLE+1);
	CNFGSetup( "vlinterm", w, h );

	//uint32_t * framebuffer = malloc( ts.charx * ts.chary * font_w * font_h * 4 *(CHAR_DOUBLE+1)*(CHAR_DOUBLE+1));
	ResetTerminal( &ts );
	char * localargv[] = { "/bin/bash", 0 };
	ts.ptspipe = spawn_process_with_pts( "/bin/bash", localargv, &ts.pid );
	ResizeScreen( &ts, ts.charx, ts.chary );
	OGCreateThread( rxthread, (void*)&ts );

	int last_scrollback = 0;
	int last_curx = 0, last_cury = 0;
	int force_taint = 0;

	while(1)
	{
		short screenx, screeny;
		CNFGGetDimensions( &screenx, &screeny );
		int x, y;
		CNFGHandleInput();
		int newx = screenx/font_w/(CHAR_DOUBLE+1);
		int newy = screeny/font_h/(CHAR_DOUBLE+1);


		if(UpdateWindowTitle)
		{
			CNFGChangeWindowTitle( NewWindowTitle );
			UpdateWindowTitle = 0;
		}

		if( ((newx != ts.charx || newy != ts.chary) && newy > 0 && newx > 0) || ( !framebuffer && !rawdrawtext ) )
		{
			ResizeScreen( &ts, newx, newy );
			w = ts.charx * font_w * (CHAR_DOUBLE+1);
			h = ts.chary * font_h * (CHAR_DOUBLE+1);
			if( !rawdrawtext )
			{
				framebuffer = realloc( framebuffer, w*h*4 );
				memset( framebuffer, 0, w*h*4 );
			}
			if( last_cury >= ts.chary ) last_cury = ts.chary-1;
		}

		int scrollback = ts.scrollback;
		if( scrollback >= ts.historyy-ts.chary ) scrollback = ts.historyy-ts.chary-1;
		if( scrollback < 0 ) scrollback = 0;
		int taint_all = force_taint || scrollback != last_scrollback;

		if( last_curx != ts.curx || last_cury != ts.cury || ts.tainted || taint_all )
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

			if( rawdrawtext )
			{
				CNFGClearFrame();
				for( y = 0; y < ts.chary; y++ )
				for( x = 0; x < ts.charx; x++ )
				{
					int ly = y - scrollback;

					uint32_t v = tb[x+ly*ts.charx];
					if( !taint_all && !(v & (1<<24)) ) continue;
					tb[x+ly*ts.charx] &= ~(1<<24);
					unsigned char c = v & 0xff;
					int color = v>>16;
					int attrib = v>>8;
					int is_cursor = (x == ts.curx && ly == ts.cury) && (ts.dec_private_mode & (1<<25));
					CNFGPenX = (CHAR_DOUBLE+1)*font_w*x;
					CNFGPenY = (CHAR_DOUBLE+1)*font_h*y;

					uint32_t colorb = TermColor( 0x00000000, color, attrib );
					uint32_t colorf = TermColor( 0xffffffff, color, attrib );

					CNFGColor( colorb );
					CNFGTackRectangle( CNFGPenX, CNFGPenY, CNFGPenX + (CHAR_DOUBLE+1)*font_w, CNFGPenY + (CHAR_DOUBLE+1)*font_h );
					if( is_cursor )
					{
						int x;
						for( x = 0; x <= font_w*(CHAR_DOUBLE+1); x+=2*(CHAR_DOUBLE+1) )
						{
							int y = x * font_h / font_w;
							int ix = font_w*(CHAR_DOUBLE+1) - x;
							int iy = font_h*(CHAR_DOUBLE+1) - y;
							CNFGTackSegment( CNFGPenX + 0, CNFGPenY + iy, CNFGPenX + x, CNFGPenY + 0 );
							CNFGTackSegment( CNFGPenX + x, CNFGPenY + 0, CNFGPenX + font_w*(CHAR_DOUBLE+1), CNFGPenY + y );
							CNFGTackSegment( CNFGPenX + font_w*(CHAR_DOUBLE+1), CNFGPenY + y, CNFGPenX + ix, CNFGPenY + font_h*(CHAR_DOUBLE+1) );
							CNFGTackSegment( CNFGPenX + ix, CNFGPenY + font_h*(CHAR_DOUBLE+1), CNFGPenX + 0, CNFGPenY + iy );
							//printf( "%d %d  %d %d\n", x, y, ix, iy );
						}
					}


					char ct[2];
					ct[0] = c;
					ct[1] = 0;
					CNFGColor( colorf );
					CNFGDrawText( ct, (CHAR_DOUBLE+1)*2 );
				}
				CNFGSwapBuffers();
				OGUnlockMutex( ts.screen_mutex );
			}
			else
			{
				for( y = 0; y < ts.chary; y++ )
				for( x = 0; x < ts.charx; x++ )
				{
					int ly = y - scrollback;

					uint32_t v = tb[x+ly*ts.charx];
					if( !taint_all && !(v & (1<<24)) ) continue;
					tb[x+ly*ts.charx] &= ~(1<<24);
					unsigned char c = v & 0xff;
					int color = v>>16;
					int attrib = v>>8;

					int cx, cy;
					int atlasx = c & 0x0f;
					int atlasy = c >> 4;
					uint32_t * fbo =   &framebuffer[x*font_w*(CHAR_DOUBLE+1) + y*font_h*w*(CHAR_DOUBLE+1)];
					uint32_t * start = &font[atlasx*font_w+atlasy*font_h*charset_w];
					int is_cursor = (x == ts.curx && ly == ts.cury) && (ts.dec_private_mode & (1<<25));

					//TODO: Major optimization - compute FG and BG color for glyph instead of pixel.
					//NOTE: This would then preclude greyscale fonts.
					if( CHAR_DOUBLE )
					{
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
					}
					else
					{
						for( cy = 0; cy < font_h; cy++ )
						{
							for( cx = 0; cx < font_w; cx++ )
							{
								fbo[cx] = (is_cursor?0xffffffff:0)^TermColor( start[cx], color, attrib );
							}
							fbo += w;
							start += charset_w;
						}
					}
				}
				OGUnlockMutex( ts.screen_mutex );
				CNFGUpdateScreenWithBitmap( (unsigned long *)framebuffer, w, h );
			}
		}
		else
		{
			static int waittoupdate;
			if( waittoupdate++ == 10 )
			{
				waittoupdate = 0;
				if( !rawdrawtext )
					CNFGUpdateScreenWithBitmap( (unsigned long *)framebuffer, w, h );
				else
					force_taint = 1;
			}
		}
		usleep(20000);
		last_scrollback = scrollback;
	//	CNFGSwapBuffers();
	}

}


