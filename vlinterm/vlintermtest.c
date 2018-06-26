#include <CNFGFunctions.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <os_generic.h>
#include <string.h>
#include <fcntl.h>

int spawn_process_with_pipes( const char * execparam, char * const argv[], int pipefd[3] );



#define INIT_CHARX  80
#define INIT_CHARY  25
#define CHAR_DOUBLE 1   //Set to 1 to double size, or 0 for normal size.

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


struct TermStructure
{
	int pipes[3];
	uint8_t * text_buffer;
	int savex, savey;
	int curx, cury;
	int charx, chary;
	int echo;
	int csistate[2];
	int whichcsi;
	int escapestate;

	//This is here so we can handle information from stdout as well as stderr.
	struct LaunchInfo
	{
		struct TermStructure * ts;
		int watchvar;
	} lis[2]; 

	og_mutex_t screen_mutex;
};

struct TermStructure ts;

void ResetTerminal( struct TermStructure * ts )
{
	ts->curx = ts->cury = 0;
	ts->echo = 1;
	ts->escapestate = 0;
	memset( ts->text_buffer, 0, ts->charx * ts->chary );
}

void FeedbackTerminal( struct TermStructure * ts, uint8_t * data, int len )
{
	write( ts->pipes[0], data, len );
}

void EmitChar( struct TermStructure * ts, int crx )
{
	OGLockMutex( ts->screen_mutex );
	if( crx == '\r' ) { goto linefeed; }
	else if( crx == '\n' ) { goto newline; }
	else if( crx == 7 ) { /*beep*/ }
	else if( crx == 8 ) { if( ts->curx ) ts->curx--; /*backspace*/ }
	else if( crx == 9 ) {
			ts->curx = (ts->curx & 7) + 8;
			if( ts->curx >= ts->charx )
				ts->curx = ts->charx-1;
			/*tabstop*/
			}
	else if( crx == 0x18 || crx == 0x1a ) { ts->escapestate = 0; }
	else if( crx == 0x1b ) { ts->escapestate = 1; }
	else if( ts->escapestate )
	{
		if( ts->escapestate == 1 )
		{
			ts->escapestate = 0;
			switch( crx )
			{
				case 'c': ResetTerminal(ts); break;
				case 'D': goto linefeed;
				case 'E': goto newline;
				case 'H': break; //Set tabstop
				case 'M': if( ts->cury ) ts->cury--; break;
				case 'Z': FeedbackTerminal( ts, "\x1b[?6c", 5 ); break;
				case '7': ts->savex = ts->curx; ts->savey = ts->cury; break;
				case '8': ts->curx = ts->savex; ts->cury = ts->savey; break;
				case '[': ts->escapestate = 2;
					ts->csistate[0] = -1;
					ts->whichcsi = 0;
					break;
				case ')':
				case '%':
				case '?':
				case '(': ts->escapestate = 5; break;

				default: break;
			}
		}
		else if( ts->escapestate == 2 )
		{
			//A CSI control message.
			if( crx >= '0' && crx <= '9' )
			{
				if( ts->csistate[ts->whichcsi] < 0 ) ts->csistate[ts->whichcsi] = 0;
				ts->csistate[ts->whichcsi] *= 10;
				ts->csistate[ts->whichcsi] += crx - '0';
			}
			else
			{
				ts->escapestate = 0;
				int is_seq_default = ts->csistate[ts->whichcsi] < 0;
				if( is_seq_default ) ts->csistate[ts->whichcsi] = 1; //Default

				//printf( "CRX: %d %d %c\n", ts->csistate[0], ts->csistate[1], crx );
				switch( crx )
				{
				case 'F': ts->curx = 0;
				case 'A': ts->cury -= ts->csistate[0]; if( ts->cury < 0 ) ts->cury = 0; break;
				case 'E': ts->curx = 0;
				case 'B': ts->cury += ts->csistate[0]; if( ts->cury >= ts->chary ) ts->cury = ts->chary - 1; break;
				case 'C': ts->curx += ts->csistate[0]; if( ts->curx >= ts->charx ) ts->curx = ts->charx - 1; break;
				case 'D': ts->curx -= ts->csistate[0]; if( ts->curx < 0 ) ts->curx = 0; break;
				case 'G': ts->curx = ts->csistate[0] - 1; if( ts->curx < 0 ) ts->curx = 0; if( ts->curx >= ts->charx ) ts->curx = ts->charx-1;break;
				case 'd': ts->cury = ts->csistate[0] - 1; if( ts->cury < 0 ) ts->cury = 0; if( ts->cury >= ts->chary ) ts->cury = ts->chary-1; break;
				case ';': ts->escapestate = 2; ts->csistate[1] = -1; ts->whichcsi = 1; break;
				case 'H':
					if( ts->csistate[0] < 0 ) ts->csistate[0] = 1;
					if( ts->csistate[1] < 0 ) ts->csistate[1] = 1;
					ts->curx = ts->csistate[1] - 1;
					if( ts->curx < 0 ) ts->curx = 0; 
					ts->cury = ts->csistate[0] - 1;
					if( ts->cury < 0 ) ts->cury = 0; 
					break;
				case 'J':
					{
						int pos = ts->curx+ts->cury*ts->charx;
						int end = ts->charx * ts->chary;
						if( is_seq_default ) ts->csistate[0] = 0; 
						switch( ts->csistate[0] ) 
						{
							case 0:
								memset( &ts->text_buffer[pos], 0, end-pos-1 ); break;
							case 1:
								memset( &ts->text_buffer[0], 0, pos ); break;
							case 2:
								memset( &ts->text_buffer[0], 0, end-1 ); break;
						}
					}
					break; 
				case 'K':
					if( is_seq_default ) ts->csistate[0] = 0; 
					switch( ts->csistate[0] )
					{
					case 0:
						memset( &ts->text_buffer[ts->curx+ts->cury*ts->charx], 0, ts->charx-ts->curx ); break;
					case 1:	
						memset( &ts->text_buffer[ts->cury*ts->charx], 0, ts->curx-1 ); break;
					case 2:
						memset( &ts->text_buffer[ts->cury*ts->charx], 0, ts->charx ); break;
					}
					break;
				case 'P':
					{
						int pos = ts->curx+ts->cury*ts->charx;
						int remain = ts->charx - ts->curx - 1;
						if(  ts->csistate[0]  < remain ) remain =  ts->csistate[0] ;
						memset( &ts->text_buffer[pos], 0, remain);
					}
					break;
				default:
					fprintf( stderr, "Esape: %c\n", crx );
				}
			}
		}

		else if( ts->escapestate == 5 )
		{
			ts->escapestate ++;
		}
		else if( ts->escapestate == 6 )
		{
			ts->escapestate = 0;
		}
	}
	else
	{
		ts->text_buffer[ts->curx + ts->cury * ts->charx] = crx;
		ts->curx++;
		if( ts->curx >= ts->charx ) { ts->curx = 0; goto handle_newline; }
	}

	goto end;
linefeed:
	ts->curx = 0;
	goto handle_newline;
newline:
	ts->cury ++;
	ts->curx = 0;
	goto handle_newline;
handle_newline:
	if( ts->cury >= ts->chary )
	{
		//Must handle scrolling the screen up;
		int line;
		for( line = 1; line < ts->chary; line ++)
		{
			memcpy( &ts->text_buffer[(line-1)*ts->charx],
				&ts->text_buffer[(line)*ts->charx], ts->charx );
		}
		memset( &ts->text_buffer[(line-1)*ts->charx], 0, ts->charx );
		ts->cury--;
	}
end:
	OGUnlockMutex( ts->screen_mutex );
}



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
		//rxdata[rx] = 0;
		//printf( ":::%s:::", rxdata );
	}
	fprintf( stderr, "Error: Terminal pipe %d died\n", li->watchvar ); 
	return 0;
}

void HandleKey( int keycode, int bDown )
{
	static int shift_held;

	if( keycode == 65506 )
	{
		shift_held = bDown;
	}

	if( bDown )
	{
		if( keycode == 65293 ) keycode = 10;
		else if( keycode  > 255 ) {
			fprintf( stderr, "%d\n", keycode );
			return;
		}

		char cc[1] = { keycode };
		FeedbackTerminal( &ts, cc, 1 );
		if( ts.echo ) EmitChar( &ts, keycode );
	}
}

void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) { }
void HandleDestroy() { }



int main()
{
	if( LoadFont("ibm437.pgm") ) return -1;

	ts.screen_mutex = OGCreateMutex();
	ts.charx = INIT_CHARX;
	ts.chary = INIT_CHARY;
	ts.echo = 1;
	short w = ts.charx * font_w * (CHAR_DOUBLE+1);
	short h = ts.chary * font_h * (CHAR_DOUBLE+1);
	CNFGSetup( "Test terminal", w, h );


	uint32_t * framebuffer = malloc( w * h * 4 ); 
	uint8_t * text_buffer = ts.text_buffer = malloc( ts.charx * ts.chary );
	unsigned char * attrib_buffer = malloc( ts.charx * ts.chary );

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

	while(1)
	{
		int x, y;
		CNFGHandleInput();
		for( y = 0; y < ts.chary; y++ )
		for( x = 0; x < ts.charx; x++ )
		{
			unsigned char c = text_buffer[x+y*ts.charx];
			int cx, cy;
			int atlasx = c & 0x0f;
			int atlasy = c >> 4;
			uint32_t * fbo =   &framebuffer[x*font_w*(CHAR_DOUBLE+1) + y*font_h*w*(CHAR_DOUBLE+1)];
			uint32_t * start = &font[atlasx*font_w+atlasy*font_h*charset_w];

	#ifdef CHAR_DOUBLE
			for( cy = 0; cy < font_h; cy++ )
			{
				for( cx = 0; cx < font_w; cx++ )
				{
					fbo[cx*2+0] = fbo[cx*2+1] = start[cx];
				}
				fbo += w;
				for( cx = 0; cx < font_w; cx++ )
				{
					fbo[cx*2+0] = fbo[cx*2+1] = start[cx];
				}
				fbo += w;
				start += charset_w;
			}
	#else
			for( cy = 0; cy < font_h; cy++ )
			{
				for( cx = 0; cx < font_w; cx++ )
				{
					fbo[cx] = start[cx];
				}
				fbo += w;
				start += charset_w;
			}
	#endif
		}
		CNFGUpdateScreenWithBitmap( (unsigned long *)framebuffer, w, h );
		CNFGSwapBuffers();
	}

}




















int spawn_process_with_pipes( const char * execparam, char * const argv[], int pipefd[3] )
{
	int pipeset[6];
  
	pipe(&pipeset[0]);
	pipe(&pipeset[2]);
	pipe(&pipeset[4]);
	
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

