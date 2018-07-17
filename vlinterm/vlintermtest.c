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

#define MAX_CSI_PARAMS 8

struct TermStructure
{
	int pipes[3];
	uint8_t * text_buffer;
	uint8_t * attrib_buffer;
	uint8_t * color_buffer;
	int current_color;
	int current_attributes;

	int savex, savey;
	int curx, cury;
	int charx, chary;
	int echo;

	int csistate[MAX_CSI_PARAMS];
	int whichcsi;
	int escapestart; // [ or ]
	int escapestate;
	int dec_priv_csi;

	int dec_keypad_mode;
	int dec_private_mode;
	int dec_mode; // ECMA-48 Mode Switches (Need to implement 4 (insert mode) as well as Auto-follow LF, VT, FF with CR (TODO)

	int osc_command_place;
	char osc_command[128];


	int scroll_top;
	int scroll_bottom;

	//This is here so we can handle information from stdout as well as stderr.
	struct LaunchInfo
	{
		struct TermStructure * ts;
		int watchvar;
	} lis[2]; 

	og_mutex_t screen_mutex;
};

struct TermStructure ts;

void BufferSet( struct TermStructure * ts, int start, int value, int length )
{
	int valuec = ts->current_color;
	int valueatt = ts->current_attributes;
	if( length == 1 )
	{
		ts->text_buffer[start] = value;
		ts->color_buffer[start] = ts->current_color;
		ts->attrib_buffer[start] = ts->current_attributes;
	}
	else
	{
		memset( ts->text_buffer + start, value, length );
		memset( ts->attrib_buffer + start, valueatt, length );
		memset( ts->color_buffer + start, valuec, length );
	}
}

void BufferCopy( struct TermStructure * ts, int dest, int src, int length )
{
	if( src < 0 ) { fprintf( stderr, "invalid buffer copy [0]\n" ); } 
	if( dest < 0 ) { fprintf( stderr, "invalid buffer copy [1]\n" ); } 
	if( src + length > ts->charx*ts->chary ) { fprintf( stderr, "invalid buffer copy [2]\n" ); } 
	if( dest + length > ts->charx*ts->chary ) { fprintf( stderr, "invalid buffer copy [3]\n" ); } 

	memcpy( ts->text_buffer + dest,   ts->text_buffer + src, length );
	memcpy( ts->attrib_buffer + dest, ts->attrib_buffer + src, length );
	memcpy( ts->color_buffer + dest,  ts->color_buffer + src, length );
}

void ResetTerminal( struct TermStructure * ts )
{
	ts->dec_private_mode = 1<<25; //Enable cursor.
	ts->dec_mode = 0;

	ts->curx = ts->cury = 0;
	ts->echo = 0;
	ts->escapestate = 0;

	ts->current_color = 7;
	ts->current_attributes = 0;

	ts->scroll_top = -1;
	ts->scroll_bottom = -1;

	BufferSet( ts, 0, 0, ts->charx * ts->chary );
}

void FeedbackTerminal( struct TermStructure * ts, uint8_t * data, int len )
{
	write( ts->pipes[0], data, len );
}

void EmitChar( struct TermStructure * ts, int crx )
{
//	fprintf( stderr, "(%d %d %c)", crx, ts->curx, crx );
	OGLockMutex( ts->screen_mutex );
	if( crx == '\r' ) { goto linefeed; }
	else if( crx == '\n' ) { goto newline; }
	else if( crx == 7 && ts->escapestate != 3 /* Make sure we're not in the OSC CSI */ ) { /*beep*/ }
	else if( crx == 8 ) {
		if( ts->curx ) ts->curx--;
		//BufferSet( ts, ts->curx+ts->cury*ts->charx, 0, 1 );
	}
	else if( crx == 9 ) {
			ts->curx = (ts->curx & (~7)) + 8;
			if( ts->curx >= ts->charx )
				ts->curx = ts->charx-1;
			/*tabstop*/
			}
	else if( crx == 0x0f || crx == 0x0e ) { /*Activate other charsets*/ }
	else if( crx == 0x18 || crx == 0x1a ) { ts->escapestate = 0; }
	else if( crx == 0x1b ) { ts->escapestate = 1; }
	else if( crx == 0x9b ) { goto csi_state_start; }
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
				//case 'H': break; //Set tabstop
				case 'M': if( ts->cury ) ts->cury--; break;
				//case 'Z': FeedbackTerminal( ts, "\x1b[?6c", 5 ); break;
				case '7': ts->savex = ts->curx; ts->savey = ts->cury; break;
				case '8': ts->curx = ts->savex; ts->cury = ts->savey; break;
				case '[': goto csi_state_start;
				case '=': ts->dec_keypad_mode = 1; break;
				case '>': ts->dec_keypad_mode = 2; break;
				case ']': goto csi_state_start;
				//case ')':
				//case '%':
				//case '(': ts->escapestate = 5; break;
				default: 
					fprintf( stderr, "UNHANDLED Esape: %c\n", crx );
					break;
			}
		}
		else if( ts->escapestate == 2 )
		{
			if( crx == ';' && ts->escapestart == ']' ) //XXX This looks WRONG
			{
				//ESC ] ### ; For an OSC command.
				printf("];command\n" );
				ts->escapestate = 3;
				ts->osc_command_place = 0;
			}
			else if( crx == '?' && ts->whichcsi == 0 )
			{
				ts->dec_priv_csi = 1;
			}
			//A CSI control message.
			else if( crx >= '0' && crx <= '9' )
			{
				if( ts->csistate[ts->whichcsi] < 0 ) ts->csistate[ts->whichcsi] = 0;
				ts->csistate[ts->whichcsi] *= 10;
				ts->csistate[ts->whichcsi] += crx - '0';
			}
			else if( ts->dec_priv_csi )
			{
				ts->escapestate = 0;
				int is_seq_default = ts->csistate[ts->whichcsi] < 0;
				if( is_seq_default ) ts->csistate[ts->whichcsi] = 1; //Default

				printf( "DEC PRIV CSI: '%c' %d %d / %d[%c]\n", crx, ts->csistate[0], ts->csistate[1], ts->dec_priv_csi, ts->dec_priv_csi );

				int set = -1;
				if( crx == 'c' )
				{
					//What is this?!?
					//char sto[128];
					//int len = sprintf( sto, "\x1b[?1;2c,\"I am a VT100 with advanced video option\"\x1b\\" );
					//int len = sprintf( sto, "\x1b[?6c:\"I am a VT102\"\\" );
					//FeedbackTerminal( ts, sto, len );
				}
				else
				{
					if( crx == 'h' ) set = 1;
					else if( crx == 'l' ) set = 0;
					if( set >= 0 )
					{
						int bit = ts->csistate[ts->whichcsi];
						if( bit == 1000 )
							bit = 30;
						if( bit > 30 || bit < 0 )
						{
							fprintf( stderr, "Error: Unknown DEC Private type %d\n", bit );
						}
						else
						{
							if( set )
								ts->dec_private_mode |= 1<<bit;
							else
								ts->dec_private_mode &= ~(1<<bit); 
						}				
					}
					else
					{
						fprintf( stderr, "Error: Unknown DEC Private code ID %d (%c)\n", crx, crx );
					}
				}
			}
			else
			{
				ts->escapestate = 0;
				int is_seq_default = ts->csistate[ts->whichcsi] < 0;
				if( is_seq_default ) ts->csistate[ts->whichcsi] = 1; //Default

				if( crx != ';' )
				printf( "CRX: %d %d  %d %d %c\n", ts->csistate[0], ts->csistate[1], ts->curx, ts->cury, crx );

				//This is the big list of CSI escapes.
				switch( crx )
				{
				case 'F': ts->curx = 0;
				case 'A': ts->cury -= ts->csistate[0]; if( ts->cury < 0 ) ts->cury = 0; break;
				case 'E': ts->curx = 0;
				case 'B': ts->cury += ts->csistate[0]; if( ts->cury >= ts->chary ) ts->cury = ts->chary - 1; break; //CUD—Cursor Down
				case 'C': ts->curx += ts->csistate[0]; if( ts->curx >= ts->charx ) ts->curx = ts->charx - 1; break; //CUF—Cursor Forward
				case 'D': ts->curx -= ts->csistate[0]; if( ts->curx < 0 ) ts->curx = 0; break;
				case 'G': ts->curx = ts->csistate[0] - 1; if( ts->curx < 0 ) ts->curx = 0; if( ts->curx >= ts->charx ) ts->curx = ts->charx-1; break;
				case 'd': ts->cury = ts->csistate[0] - 1; if( ts->cury < 0 ) ts->cury = 0; if( ts->cury >= ts->chary ) ts->cury = ts->chary-1; break;
				case ';': ts->escapestate = 2; if( ts->whichcsi < MAX_CSI_PARAMS ) { ts->whichcsi++; ts->csistate[ts->whichcsi] = -1; } break;
				case 'h':
					ts->dec_mode |= 1<<ts->csistate[0];
					break;
				case 'l':
					ts->dec_mode &= ~(1<<ts->csistate[0]);
					break;
				case 'f':
				case 'H':	//CUP—Cursor Position
					if( ts->csistate[0] <= 0 ) ts->csistate[0] = 1;
					if( ts->csistate[1] <= 0 ) ts->csistate[1] = 1;
					ts->curx = ts->csistate[1] - 1;
					if( ts->curx < 0 ) ts->curx = 0; 
					ts->cury = ts->csistate[0] - 1;
					if( ts->cury < 0 ) ts->cury = 0; 
					break;
				case 'J':	//DECSED—Selective Erase in Display
					{
						int pos = ts->curx+ts->cury*ts->charx;
						int end = ts->charx * ts->chary;
						if( is_seq_default ) ts->csistate[0] = 0; 
						switch( ts->csistate[0] ) 
						{
							case 0:	BufferSet( ts, pos, 0, end-pos ); break;
							case 1:	BufferSet( ts, 0, 0, pos ); break;
							case 2:	case 3:
								BufferSet( ts, 0, 0, end ); break;
						}
						ts->curx = 0;
					}
					break; 
				case 'K':	//DECSEL—Selective Erase in Line
					if( is_seq_default ) ts->csistate[0] = 0; 
					switch( ts->csistate[0] )
					{
					case 0: BufferSet( ts, ts->curx+ts->cury*ts->charx, 0, ts->charx-ts->curx ); break;
					case 1:	BufferSet( ts, ts->cury*ts->charx, 0, ts->curx ); break;
					case 2:	BufferSet( ts, ts->cury*ts->charx, 0, ts->charx ); break;
					}
					break;
				case 'L':
					//Inset blank lines.
					{
						int l;
						if( ts->csistate[0] > 0 )
						{
							for( l = ts->chary-1; l >= ts->cury+ts->csistate[0]; l-- )
							{
								if( l > ts->scroll_bottom ) continue;
								if( l < ts->scroll_top ) continue;
								printf( "%d %d  (Copy %d to %d)\n", ts->cury, ts->csistate[0], l-ts->csistate[0], l );
								BufferCopy( ts, l*ts->charx, (l-ts->csistate[0])*ts->charx, ts->charx );
							}
							BufferSet( ts, ts->cury*ts->charx, 0, ts->charx * ts->csistate[0] );
						}
					}
					break;
				case 'M': //Delete the specified number of lines.
				{
					//XXX TODO Pick up here.
					if( ts->csistate[0] > 0 )
					{
						int lines = ts->csistate[0];
						for( l = ts->scroll_bottom-1; l >= ts->cury+lines; l-- )
						{
							if( l > ts->scroll_bottom ) continue;
							if( l < ts->scroll_top ) continue;
							printf( "%d %d  (Copy %d to %d)\n", ts->cury, ts->csistate[0], l-ts->csistate[0], l );
							BufferCopy( ts, l*ts->charx, (l-ts->csistate[0])*ts->charx, ts->charx );
						}
						BufferSet( ts, ts->cury*ts->charx, 0, ts->charx * lines );
					}

					printf( "%d %d   %d %d   %d %d   %d %d\n", ts->charx, ts->chary, ts->curx, ts->cury, ts->scroll_top, ts->scroll_bottom, lines, ts->cury-ts->scroll_bottom-lines );
					BufferCopy( ts, ts->charx * ts->cury, ts->charx * ( ts->cury + lines ), ts->charx*(ts->scroll_bottom-ts->cury-lines) );
					
					ts->curx = 0;
					break;
				}

				case 'P': //DCH "Delete Character" "
					{
						//"As characters are deleted, the remaining characters between the cursor and right margin move to the left."
						int chars_to_del = ts->csistate[0];
						int remain_in_line = ts->charx - ts->curx;
						if( chars_to_del > remain_in_line ) remain_in_line = remain_in_line;
						int start = ts->cury*ts->charx + ts->curx;
						int nr_after_shift = remain_in_line - chars_to_del-1;
						int i;
						printf( "%d %d %d\n", chars_to_del, nr_after_shift, remain_in_line );
						for( i = 0; i < remain_in_line; i++ )
						{
							if( i <= nr_after_shift )
							{
								ts->text_buffer[start+i] = ts->text_buffer[start+i+chars_to_del];
								ts->color_buffer[start+i] = ts->color_buffer[start+i+chars_to_del];
								ts->attrib_buffer[start+i] = ts->attrib_buffer[start+i+chars_to_del];
							}
							else
							{
								ts->text_buffer[start+i] = 0;
								ts->color_buffer[start+i] = ts->current_color;
								ts->attrib_buffer[start+i] = ts->current_attributes;
							}
						}
					}
					break;
				case 'r': //DECSTBM
					ts->scroll_top = ts->csistate[0]-1;
					ts->scroll_bottom = ts->csistate[1]-1;
					printf( "DECSTBM: %d %d\n", ts->scroll_bottom,  ts->scroll_top );
					ts->curx = 0;
					ts->cury = ts->scroll_top;
					break;
				case 'm':
				{
					//SGR (set attributes)
					int i;
					for( i = 0; i < ts->whichcsi+1; i++ )
					{
						int k = ts->csistate[i];
						if( is_seq_default ) k = 0; 
						if( k == 0 ) { ts->current_color = 7; ts->current_attributes = 0; }
						else if( k == 1 ) { ts->current_attributes |= 1<<0; }
						else if( k == 2 ) { ts->current_attributes |= 1<<1; }
						else if( k == 4 ) { ts->current_attributes |= 1<<2; }
						else if( k == 5 ) { ts->current_attributes |= 1<<3; }
						else if( k == 7 ) { ts->current_attributes |= 1<<4; }
						else if( k == 21 ) { ts->current_attributes &= ~(1<<0); }
						else if( k == 22 ) { ts->current_attributes &= ~(1<<1); }
						else if( k == 24 ) { ts->current_attributes &= ~(1<<2); }
						else if( k == 25 ) { ts->current_attributes &= ~(1<<3); }
						else if( k == 26 ) { ts->current_attributes &= ~(1<<4); }
						else if( k >= 30 && k < 37 ) { ts->current_color = (ts->current_color&0xf0) | ( k - 30 ); }
						else if( k >= 40 && k < 47 ) { ts->current_color = (ts->current_color&0x0f) | ( ( k - 40 ) << 4 ); }
						else if( k == 38 ) { ts->current_attributes |= 1<<4; ts->current_color = (ts->current_color&0xf0) | 7; }
						else if( k == 39 ) { ts->current_attributes &= ~(1<<4); ts->current_color = (ts->current_color&0xf0) | 7; }
						else if( k == 49 ) { ts->current_color = (ts->current_color&0x0f) | (0<<4); }
					}
					break;
				}
				default:
					fprintf( stderr, "UNHANDLED CSI Esape: %c [%d]\n", crx, ts->csistate[0] );
				}
			}
		}
		else if( ts->escapestate == 3 )
		{
			//Processing ESC ] (OSC)
			if( crx == 0x07 )
			{
				ts->osc_command[ts->osc_command_place] = 0;
				printf( "Got OSC Command: %d: %s\n", ts->csistate[0], ts->osc_command );
				ts->escapestate = 0;
			}
			else if( ts->osc_command_place < sizeof( ts->osc_command ) - 1 )
			{
				ts->osc_command[ts->osc_command_place++] = crx;
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
		if( ts->dec_private_mode & 2 ) //??? XXX Is this correct? 
		{
			if( ts->curx > ts->charx ) { ts->curx = 0; ts->cury++; if( ts->cury >= ts->chary ) ts->cury = ts->chary-1; } 
			BufferSet( ts, ts->curx+ts->cury*ts->charx, crx, 1 );
			ts->curx++;
		}

		{
			if( ts->curx >= ts->charx ) { ts->curx = 0; ts->cury++; goto handle_newline; }
			BufferSet( ts, ts->curx+ts->cury*ts->charx, crx, 1 );
			ts->curx++;
		}
		if( crx < 32 || crx < 0 ) printf( "CRX %d\n", (uint8_t)crx );
	}

	goto end;
linefeed:
	ts->curx = 0;
	goto handle_newline;
newline:
	ts->cury ++;
	ts->curx = 0;
	goto handle_newline;
csi_state_start:
	ts->escapestart = crx;
	ts->escapestate = 2;
	ts->csistate[0] = -1;
	ts->csistate[1] = -1;
	ts->dec_priv_csi = 0;
	ts->whichcsi = 0;
	goto end;
handle_newline:
	if( ts->cury >= ts->chary )
	{
		//Must handle scrolling the screen up;
		int line;
		for( line = 1; line < ts->chary; line ++)
		{
			BufferCopy( ts, (line-1)*ts->charx, (line)*ts->charx, ts->charx );
		}
		BufferSet( ts, (line-1)*ts->charx, 0, ts->charx );
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
	write( ts.pipes[0], "script /dev/null\n", 17 );
	write( ts.pipes[0], "export TERM=linux\n", 18 );

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
	//	CNFGSwapBuffers();
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

