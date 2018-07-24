#ifndef _VLINTERM_H
#define _VLINTERM_H

#include <stdint.h>
#include <os_generic.h>



#define MAX_CSI_PARAMS 8

struct TermStructure
{
	int ptspipe;
	uint8_t * text_buffer;
	uint8_t * attrib_buffer;
	uint8_t * color_buffer;
	uint8_t * taint_buffer;
	uint8_t tainted;
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

	int pid;

	int scroll_top;
	int scroll_bottom;

	og_mutex_t screen_mutex;
};

void EmitChar( struct TermStructure * ts, int crx );
void ResetTerminal( struct TermStructure * ts );
int FeedbackTerminal( struct TermStructure * ts, const uint8_t * data, int len );

//You must implement this.
void HandleOSCCommand( struct TermStructure * ts, int parameter, const char * value );
void HandleBell( struct TermStructure * ts );
#endif

