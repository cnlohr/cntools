#include "vlinterm.h"
#include <stdio.h>
#include <string.h>

//#define DEBUG_VLINTERM

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
	memset( ts->taint_buffer + start, 1, length );
	ts->tainted = 1;
}

void BufferCopy( struct TermStructure * ts, int dest, int src, int length )
{
	if( src < 0 ) { 
#ifdef DEBUG_VLINTERM
		fprintf( stderr, "invalid buffer copy [0]\n" );
#endif
		return;
	} 
	if( dest < 0 ) {
#ifdef DEBUG_VLINTERM
		fprintf( stderr, "invalid buffer copy [1]\n" ); 
#endif
		return;
	} 
	if( src + length > ts->charx*ts->chary )
	{
#ifdef DEBUG_VLINTERM
		fprintf( stderr, "invalid buffer copy [2]\n" );
#endif
		return;
	} 
	if( dest + length > ts->charx*ts->chary ) {
#ifdef DEBUG_VLINTERM
		fprintf( stderr, "invalid buffer copy [3]\n" );
#endif
		return;
	} 

	if( length < 0 )
	{
#ifdef DEBUG_VLINTERM
		fprintf( stderr, "Warning! Negative len copy\n" );
#endif
		return;
	}

	//Detect if we have overlapping regions, if so we must use a temporary buffer.
	if( ( dest > src && src + length >= dest ) ||
		( src > dest && dest + length >= src ) )
	{
		char temp[length];
		memcpy( temp,   ts->text_buffer + src, length );	
		memcpy( ts->text_buffer + dest, temp, length );	
		memcpy( temp,   ts->attrib_buffer + src, length );	
		memcpy( ts->attrib_buffer + dest, temp, length );	
		memcpy( temp,   ts->color_buffer + src, length );	
		memcpy( ts->color_buffer + dest, temp, length );	
	}
	else
	{
		memcpy( ts->text_buffer + dest,   ts->text_buffer + src, length );
		memcpy( ts->attrib_buffer + dest, ts->attrib_buffer + src, length );
		memcpy( ts->color_buffer + dest,  ts->color_buffer + src, length );
	}
	memset( ts->taint_buffer + dest, 1, length );
	ts->tainted = 1;
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

int FeedbackTerminal( struct TermStructure * ts, const uint8_t * data, int len )
{
	return write( ts->ptspipe, data, len );
}

void InsertBlankLines( struct TermStructure * ts, int l )
{
	if( l > ts->scroll_bottom - ts->cury ) l = ts->scroll_bottom - ts->cury;
	if( l > 0 )
	{
		int lines_need_to_move = ts->scroll_bottom - ts->cury - l;
		//printf( "PUSHING %d LINES / %d * %d\n", l, lines_need_to_move, ts->cury );
		if( lines_need_to_move > 0 ) 
			BufferCopy( ts, (l+ts->cury)*ts->charx, ts->cury*ts->charx, ts->charx * ( ts->scroll_bottom - l - ts->cury  ) );
		BufferSet( ts, ts->cury*ts->charx, 0, ts->charx * l );
	}
}

void HandleNewline( struct TermStructure * ts, int advance )
{
	if( advance )
	{
		//printf( "Handle newline\n" );
		InsertBlankLines( ts, 1 );
	}

	int top    = (ts->scroll_top<0)?0:(ts->scroll_top+1);
	int bottom = (ts->scroll_bottom<0)?ts->chary:(ts->scroll_bottom+1);
//	int bottom = ts->chary;
	if( ts->cury >= bottom )
	{
		//Must handle scrolling the screen up;
		BufferCopy( ts, 0, ts->charx, (bottom-top)*ts->charx );
		BufferSet( ts, (bottom-1)*ts->charx, 0, ts->charx );
		ts->cury--;
	}
}

void EmitChar( struct TermStructure * ts, int crx )
{

#ifdef DEBUG_VLINTERM
	fprintf( stderr, "(%d %d %c)", crx, ts->curx, crx );
#endif

	OGLockMutex( ts->screen_mutex );
	if( crx == '\r' ) { goto linefeed; }
	else if( crx == '\n' ) { goto newline; }
	else if( crx == 7 && ts->escapestate != 3 /* Make sure we're not in the OSC CSI */ ) { HandleBell( ts ); }
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
#ifdef DEBUG_VLINTERM
			fprintf( stderr, "{E: %d[%c] (%d %d %d %d)}", crx, crx, ts->curx, ts->cury, ts->savex, ts->savey );
#endif
			switch( crx )
			{
				case 'c': ResetTerminal(ts); break;
				case 'D': goto linefeed;
				case 'E': goto newline;
				//case 'H': break; //Set tabstop
				case 'M':	//Reverse linefeed.
					ts->cury--;
					if( ts->cury < ts->scroll_top )
					{
						BufferCopy( ts, ts->charx*(ts->scroll_top+1), ts->charx*ts->scroll_top, (ts->scroll_bottom - ts->scroll_top - 1) * ts->charx );
						BufferSet( ts, ts->charx*ts->scroll_top, 0, ts->charx );
						ts->cury = ts->scroll_top;
					} 
					break;
				//case 'Z': FeedbackTerminal( ts, "\x1b[?6c", 5 ); break;
				case '7': ts->savex = ts->curx; ts->savey = ts->cury; break;
				case '8': ts->curx = ts->savex; ts->cury = ts->savey; break;
				case '[': goto csi_state_start;
				case '=': ts->dec_keypad_mode = 1; break;
				case '>': ts->dec_keypad_mode = 2; break;
				case ']': goto csi_state_start;
				case '(': ts->escapestate = 4; break; //Start sequence defining G0 character set
				//case ')':
				//case '%':
				//case '(': ts->escapestate = 5; break;
				default: 
#ifdef DEBUG_VLINTERM
					fprintf( stderr, "UNHANDLED Esape: %c %d\n", crx, ts->whichcsi, ts->csistate[0] );
#endif
					break;
			}
		}
		else if( ts->escapestate == 2 )
		{
			if( crx == ';' && ts->escapestart == ']' ) //XXX This looks WRONG
			{
				//ESC ] ### ; For an OSC command.
#ifdef DEBUG_VLINTERM
				fprintf( stderr, "];command\n" );
#endif
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

#ifdef DEBUG_VLINTERM
				fprintf( stderr, "DEC PRIV CSI: '%c' %d %d / %d[%c]\n", crx, ts->csistate[0], ts->csistate[1], ts->dec_priv_csi, ts->dec_priv_csi );
#endif
				int set = -1;
				if( crx == 'c' )
				{
#ifdef DEBUG_VLINTERM
					fprintf( stderr, "Got a private [?c - not sure what to do with it.\n" );
#endif
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
						if( bit == 1000 ) //X11 mouse reporting.
							bit = 30;
						if( bit > 30 || bit < 0 )
						{
#ifdef DEBUG_VLINTERM
							fprintf( stderr, "Error: Unknown DEC Private type %d\n", bit );
#endif
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
#ifdef DEBUG_VLINTERM
						fprintf( stderr, "Error: Unknown DEC Private code ID %d (%c)\n", crx, crx );
#endif
					}
				}
			}
			else if( ts->escapestate == 4 )
			{
				//Charset escape sequence (Not implemented yet)
				ts->escapestate = 0;
			}
			else
			{
				ts->escapestate = 0;
				int is_seq_default = ts->csistate[ts->whichcsi] < 0;
				if( is_seq_default ) ts->csistate[ts->whichcsi] = 1; //Default

#ifdef DEBUG_VLINTERM
				if( crx != ';' ) fprintf( stderr, "CRX: %d %d  %d %d %c\n", ts->csistate[0], ts->csistate[1], ts->curx, ts->cury, crx );
#endif
				//This is the big list of CSI escapes.
				switch( crx )
				{
				case '@': 
				{
					//Insert specified # of characters.
					int chars = ts->csistate[0];
					BufferCopy( ts, ts->charx*ts->cury, ts->charx*ts->cury-chars, chars );
					BufferSet( ts, ts->charx*ts->cury + ts->charx, 0, chars ); 
					break;
				}
				case 'F': ts->curx = 0;
				case 'A': ts->cury -= ts->csistate[0]; break;
				case 'E': ts->curx = 0;
				case 'B': ts->cury += ts->csistate[0]; break; //CUD—Cursor Down
				case 'C': ts->curx += ts->csistate[0]; break; //CUF—Cursor Forward
				case 'D': ts->curx -= ts->csistate[0]; ts->curx = 0; break;
				case 'G': ts->curx = ts->csistate[0] - 1; break;
				case 'd': ts->cury = ts->csistate[0] - 1; break;
				case ';': ts->escapestate = 2; if( ts->whichcsi < MAX_CSI_PARAMS ) { ts->whichcsi++; ts->csistate[ts->whichcsi] = -1; } break;
				case 'h': ts->dec_mode |= 1<<ts->csistate[0]; break;
				case 'l': ts->dec_mode &= ~(1<<ts->csistate[0]); break;
				case 'f':
				case 'H':	//CUP - Cursor Position
					if( ts->csistate[0] <= 0 ) ts->csistate[0] = 1;
					if( ts->csistate[1] <= 0 ) ts->csistate[1] = 1;
					ts->curx = ts->csistate[1] - 1;
					ts->cury = ts->csistate[0] - 1;
					break;
				case 'J':	//DECSED - Selective Erase in Display
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
				case 'K':	//DECSEL - Selective Erase in Line
					if( is_seq_default ) ts->csistate[0] = 0; 
					switch( ts->csistate[0] )
					{
					case 0: BufferSet( ts, ts->curx+ts->cury*ts->charx, 0, ts->charx-ts->curx ); break;
					case 1:	BufferSet( ts, ts->cury*ts->charx, 0, ts->curx ); break;
					case 2:	BufferSet( ts, ts->cury*ts->charx, 0, ts->charx ); break;
					}
					break;
				case 'L': //Inset blank lines.
					{
						int l = ts->csistate[0];
						InsertBlankLines( ts, l );
					}
					break;
				case 'M': //Delete the specified number of lines.
				{
					if( ts->csistate[0] > 0 )
					{
						int lines = ts->csistate[0];
						int startcopy = ts->cury;
						int stopcopy = ts->scroll_bottom - lines;
						if( startcopy < ts->scroll_top ) startcopy = ts->scroll_top;						
						if( stopcopy > startcopy )
						{
							BufferCopy( ts, startcopy*ts->charx, (lines+startcopy)*ts->charx, ts->charx*(stopcopy-startcopy) );
							BufferSet( ts, stopcopy*ts->charx, 0, ts->charx * lines );
						}
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

						BufferCopy( ts, start, start+i+chars_to_del, nr_after_shift+1 );
						BufferSet( ts, start+nr_after_shift, 0, remain_in_line-nr_after_shift );
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
						else if( k == 27 ) { ts->current_attributes &= ~(1<<4); }
						else if( k >= 30 && k < 37 ) { ts->current_color = (ts->current_color&0xf0) | ( k - 30 ); }
						else if( k >= 40 && k < 47 ) { ts->current_color = (ts->current_color&0x0f) | ( ( k - 40 ) << 4 ); }
						else if( k == 38 ) { ts->current_attributes |= 1<<4; ts->current_color = (ts->current_color&0xf0) | 7; }
						else if( k == 39 ) { ts->current_attributes &= ~(1<<4); ts->current_color = (ts->current_color&0xf0) | 7; }
						else if( k == 49 ) { ts->current_color = (ts->current_color&0x0f) | (0<<4); }
					}
					break;
				}
				default:
#ifdef DEBUG_VLINTERM
					fprintf( stderr, "UNHANDLED CSI Esape: %c [%d]\n", crx, ts->csistate[0] );
#endif
					break;
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

		int top    = (ts->scroll_top<0)?0:ts->scroll_top;
		int bottom = (ts->scroll_bottom<0)?ts->chary:(ts->scroll_bottom+1);
		if( ts->cury < top ) ts->cury = top;
		if( ts->curx < 0 ) ts->curx = 0;
		if( ts->cury >= bottom ) ts->cury = bottom-1;
		if( ts->curx >= ts->charx ) ts->curx = ts->charx-1; //XXX DUBIOUS
	}
	else
	{
		//No escape sequence.  Just handle the character.

		/*if( ts->dec_private_mode & 2 ) //??? XXX Is this correct? 
		{
			if( ts->curx >= ts->charx ) { ts->curx = 0; ts->cury++; if( ts->cury >= ts->chary ) ts->cury = ts->chary-1; } 
			BufferSet( ts, ts->curx+ts->cury*ts->charx, crx, 1 );
			ts->curx++;
		}
		else
		{*/
			if( ts->curx >= ts->charx ) { ts->curx = 0; ts->cury++; HandleNewline( ts, 0 ); }
			BufferSet( ts, ts->curx+ts->cury*ts->charx, crx, 1 );
			ts->curx++;
		//}
#ifdef DEBUG_VLINTERM
		if( crx < 32 || crx < 0 ) fprintf( stderr, "C-X %d\n", (uint8_t)crx );
#endif
	}

	goto end;
linefeed:
	ts->curx = 0;
	HandleNewline( ts, 0 );
	goto end;

newline:
	ts->cury ++;
	ts->curx = 0;
	HandleNewline( ts, 1 );
	goto end;

csi_state_start:
	ts->escapestart = crx;
	ts->escapestate = 2;
	ts->csistate[0] = -1;
	ts->csistate[1] = -1;
	ts->dec_priv_csi = 0;
	ts->whichcsi = 0;
	goto end;
end:
	OGUnlockMutex( ts->screen_mutex );
}

