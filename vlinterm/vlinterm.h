#ifndef _VLINTERM_H
#define _VLINTERM_H

#include <stdint.h>
#include <os_generic.h>



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

void EmitChar( struct TermStructure * ts, int crx );
void ResetTerminal( struct TermStructure * ts );
int FeedbackTerminal( struct TermStructure * ts, uint8_t * data, int len );

//You must implement this.
void HandleOSCCommand( struct TermStructure * ts, int parameter, const char * value );

#endif

