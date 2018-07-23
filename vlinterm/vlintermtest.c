#include <CNFGFunctions.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <os_generic.h>
#include <string.h>
#include <fcntl.h>

#include "vlinterm.h"

int spawn_process_with_pipes( const char * execparam, char * const argv[], int pipefd[3] );

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
	while( (c = getc(f)) != EOF ) { printf( "%c", c ); if( c == '\n' ) break; }	//Comment
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

	return 0;
}

struct TermStructure ts;



void * rxthread( void * v )
{
	struct LaunchInfo * li = (struct LaunchInfo*)v; 
	struct TermStructure * ts = li->ts;

    while( 1 )
	{
		uint8_t rxdata[1024+1];
		int rx = read( ts->pipes[li->watchvar], rxdata, 1024 );
		if( rx < 0 ) break;
		int i;
		for( i = 0; i < rx; i++ )
		{
			char crx = rxdata[i];
			EmitChar( ts, crx );
		}
	}
	fprintf( stderr, "Error: Terminal pipe %d died\n", li->watchvar ); 
	return 0;
}


void HandleOSCCommand( struct TermStructure * ts, int parameter, const char * value )
{
	printf( "OSC Command: %d: %s\n", parameter, value );
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
		if( keycode == 65293 ) keycode = 10;
		if( keycode == 65288 ) keycode = 8;
		if( keycode == 65289 ) keycode = 9;
		if( keycode == 65362 ) //Up
		{			char cc[] = { 0x1b, '[', 'A' };	FeedbackTerminal( &ts, cc, 3 );		}
		else if( keycode == 65364 ) //Down
		{			char cc[] = { 0x1b, '[', 'B' };	FeedbackTerminal( &ts, cc, 3 );		}
		else if( keycode == 65361 ) //Left
		{			char cc[] = { 0x1b, '[', 'D' };	FeedbackTerminal( &ts, cc, 3 );		}
		else if( keycode == 65363 ) //Right
		{			char cc[] = { 0x1b, '[', 'C' };	FeedbackTerminal( &ts, cc, 3 );		}
		else if( keycode == 65366 ) //PgDn
		{			char cc[] = { 0x1b, '[', '6', '~' };	FeedbackTerminal( &ts, cc, 4 );		}
		else if( keycode == 65365 ) //PgUp
		{			char cc[] = { 0x1b, '[', '5', '~' };	FeedbackTerminal( &ts, cc, 4 );		}
		else if( keycode == 65367 ) //Home
		{			char cc[] = { 0x1b, '[', 'F' };	FeedbackTerminal( &ts, cc, 3 );		}
		else if( keycode == 65360 ) //End
		{			char cc[] = { 0x1b, '[', 'H' };	FeedbackTerminal( &ts, cc, 3 );		}
		else if( keycode  > 255 ) {
			fprintf( stderr, "%d\n", keycode );
			return;
		}
		else
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
			else if( ctrl_held && keycode >= 'a' && keycode <= 'z' )
			{
				keycode = keycode - 'a' + 1;
			}
			printf( "%d %d\n", keycode, g_x_global_shift_key );
			char cc[1] = { keycode };
			FeedbackTerminal( &ts, cc, 1 );
			if( ts.echo ) EmitChar( &ts, keycode );
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


	uint32_t * framebuffer = malloc( w * h * 4 ); 
	uint8_t * text_buffer = ts.text_buffer = malloc( ts.charx * ts.chary );
	unsigned char * attrib_buffer = ts.attrib_buffer = malloc( ts.charx * ts.chary );
	unsigned char * color_buffer  = ts.color_buffer =  malloc( ts.charx * ts.chary );

	ResetTerminal( &ts );
	sprintf( text_buffer, "Hello, world, how are you doing? 123 this is a continuation of the test.  I am going to write a lot of text here to see what can happen if I just keep typing to see where there are issues with the page breaks." );


	char * localargv[] = { "/bin/bash", 0 };
	spawn_process_with_pipes( "/bin/bash", localargv, ts.pipes );

	ts.lis[0].ts = &ts;
	ts.lis[1].ts = &ts;
	ts.lis[0].watchvar = 1;
	ts.lis[1].watchvar = 2;

	OGCreateThread( rxthread, (void*)&ts.lis[0] );
	OGCreateThread( rxthread, (void*)&ts.lis[1] );

	usleep(10000);
	FeedbackTerminal( &ts, "script /dev/null\n", 17 );
	FeedbackTerminal( &ts, "export TERM=linux\n", 18 );

	while(1)
	{
		int x, y;
		CNFGHandleInput();
		for( y = 0; y < ts.chary; y++ )
		for( x = 0; x < ts.charx; x++ )
		{
			unsigned char c = text_buffer[x+y*ts.charx];
			int color = ts.color_buffer[x+y*ts.charx];
			int attrib = ts.attrib_buffer[x+y*ts.charx];
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
		CNFGUpdateScreenWithBitmap( (unsigned long *)framebuffer, w, h );
		usleep(20000);
	//	CNFGSwapBuffers();
	}

}




















int spawn_process_with_pipes( const char * execparam, char * const argv[], int pipefd[3] )
{
	int pipeset[6];
  
	if( pipe(&pipeset[0]) ) return -1;
	if( pipe(&pipeset[2]) ) return -2;
	if( pipe(&pipeset[4]) ) return -3;
	
	int rforkv = fork();
	if (rforkv == 0)
	{
		// child
		close( pipeset[1] );
		close( pipeset[2] );
		close( pipeset[4] );
		dup2(pipeset[0], 0);
		dup2(pipeset[3], 1);
		dup2(pipeset[5], 2);
		close( pipeset[0] );
		close( pipeset[3] );
		close( pipeset[5] );
		execvp( execparam, argv );
	}
	else
	{
		// parent
		pipefd[0] = pipeset[1];
		pipefd[1] = pipeset[2];
		pipefd[2] = pipeset[4];
		close( pipeset[0] );
		close( pipeset[3] );
		close( pipeset[5] );
		return rforkv;
	}
}

