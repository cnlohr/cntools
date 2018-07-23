#include "vlinterm.h"
#include <stdio.h>
#include <string.h>

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

int FeedbackTerminal( struct TermStructure * ts, uint8_t * data, int len )
{
	return write( ts->pipes[0], data, len );
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
					fprintf( stderr, "UNHANDLED Esape: %c %d\n", crx, ts->whichcsi, ts->csistate[0] );
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

				//printf( "DEC PRIV CSI: '%c' %d %d / %d[%c]\n", crx, ts->csistate[0], ts->csistate[1], ts->dec_priv_csi, ts->dec_priv_csi );

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

				//if( crx != ';' ) printf( "CRX: %d %d  %d %d %c\n", ts->csistate[0], ts->csistate[1], ts->curx, ts->cury, crx );

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
					//Inset blank lines.  //XXX TODO: Double-check edge cases.
					{
						int l;
						if( ts->csistate[0] > 0 )
						{
							for( l = ts->chary-1; l >= ts->cury+ts->csistate[0]; l-- )
							{
								if( l > ts->scroll_bottom ) continue;
								if( l < ts->scroll_top ) continue;
								BufferCopy( ts, l*ts->charx, (l-ts->csistate[0])*ts->charx, ts->charx );
							}
							BufferSet( ts, ts->cury*ts->charx, 0, ts->charx * ts->csistate[0] );
						}
					}
					break;
				case 'M': //Delete the specified number of lines.
				{	//XXX TODO: Double-check edge cases.
					if( ts->csistate[0] > 0 )
					{
						int l;
						int lines = ts->csistate[0];
						for( l = ts->cury; l < ts->scroll_bottom - lines; l++ )
						{
							if( l > ts->scroll_bottom ) continue;
							if( l < ts->scroll_top ) continue;
							BufferCopy( ts, l*ts->charx, (l+lines)*ts->charx, ts->charx );
						}
						BufferSet( ts, l*ts->charx, 0, ts->charx * lines );
					}
					
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
				HandleOSCCommand( ts, ts->csistate[0], ts->osc_command );
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
		//if( crx < 32 || crx < 0 ) printf( "CRX %d\n", (uint8_t)crx );
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

