#include <CNFGFunctions.h>
#include <stdlib.h>
#include <stdio.h>

#define INIT_CHARX  80
#define INIT_CHARY  34

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



void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) { }
void HandleDestroy() { }



int main()
{
	if( LoadFont("ibm437.pgm") ) return -1;

	int charx = INIT_CHARX;
	int chary = INIT_CHARY;
	short w = charx * font_w;
	short h = chary * font_h;
	CNFGSetup( "Test terminal", w, h );

	unsigned long * framebuffer = malloc( w * h * 4 ); 
	unsigned char * text_buffer = malloc( charx * chary );
	unsigned char * attrib_buffer = malloc( charx * chary );

	sprintf( text_buffer, "Hello, world, how are you doing?\n" );

	while(1)
	{
		int x, y;
		CNFGHandleInput();
		CNFGUpdateScreenWithBitmap( framebuffer, w, h );
		for( y = 0; y < chary; y++ )
		for( x = 0; x < charx; x++ )
		{
			unsigned char c = text_buffer[x+y*chary];
			int cx, cy;
			int atlasx = c & 0x0f;
			int atlasy = c >> 4;
			unsigned long * fbo = &framebuffer[x*font_w + y*font_h*w];
			unsigned long * start = &font[atlasx*font_w+atlasy*font_h*charset_w];
			for( cy = 0; cy < fonty; cy++ )
			{
				for( cx = 0; cx < fontx; cx++ )
				{
					fb[cx] = start[cx];
				}
				fb += w;
				start += charset_w;
			}
		}

		CNFGSwapBuffers();
	}
	
}


