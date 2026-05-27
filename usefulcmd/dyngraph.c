//Make it so we don't need to include any other C files in our build.
#define CNFG_IMPLEMENTATION

#include "rawdraw_sf.h"
#include "os_generic.h"

#include <float.h>
#include <math.h>

int lastMouseX = -1, lastMouseY = -1;
int clicked = 0;
int bolded = -1;
int boldedPin = -1;
int suppress_stdout = 0;
int line_graph_mode_depth = 0;

void HandleKey( int keycode, int bDown ) {  }
void HandleButton( int x, int y, int button, int bDown )
{
	clicked = bDown;
	if( bDown )
		if( boldedPin < 0 )
			boldedPin = bolded;
		else
			boldedPin = -1;
}

void HandleMotion( int x, int y, int mask ) { lastMouseX = x; lastMouseY = y; }
int HandleDestroy() { return 0; }

int unique_ranges;
int fixed_range_min_set;
int fixed_range_max_set;
double fixed_range_min;
double fixed_range_max;

double SimpleReadNumber( char ** number_ptr, double defaultNumber )
{
	if( !number_ptr ) return defaultNumber;

	char * number = *number_ptr;
	if( !number || !number[0] )
	{
		*number_ptr = 0;
		return defaultNumber;
	}

	while( number[0] == ' ' || number[0] == '\t' ) number++;

	int radix = 10;
	double ret;
	if( number[0] == '0' )
	{
		char nc = number[1];
		number+=2;
		if( nc == 0 ) { *number_ptr = number-1; return 0; }
		else if( nc == 'x' ) radix = 16;
		else if( nc == ' ' || nc == '\t' || nc == ',' || nc == ';' ) { *number_ptr = number-1; return 0; }
		else if( nc == 'b' ) radix = 2;
		else { number--; radix = 8; }
	}
	char * endptr;
	if( radix != 10 )
	{
		ret = strtoll( number, &endptr, radix );
	}
	else
	{
		ret = strtod( number, &endptr );
	}
	if( endptr == number )
	{
		*number_ptr = endptr;
		return defaultNumber;
	}
	else
	{
		while( *endptr == ',' || *endptr == ';' ) endptr++;
		*number_ptr = endptr;
		return ret;
	}
}

#define MAX_SETS 512

uint32_t palette[MAX_SETS] = {
	0xa5c266ff,
	0x628dfcff,
	0xcba08dff,
	0xc38ae7ff,
	0x54d8a6ff,
	0x2fd9ffff,
	0x9fc4e5ff,
	0xb3b3b3ff,

	0x00488eff,
	0x00e49fff,
	0xffbb28ff,
	0xff8042ff,
	0x8884d8ff,
	0xa455a3ff,

	0x0000ffff,
	0xff0000ff,
	0x00ff00ff,
};

#define MAX_PTS 8192

double times[MAX_PTS];
double data[MAX_PTS][MAX_SETS];
int    fields_count[MAX_PTS];
double tmin[MAX_SETS];
double tmax[MAX_SETS];
double tavgs[MAX_SETS];
double tcnts[MAX_SETS];
double tstds[MAX_SETS];
double invrange[MAX_SETS];
double tlasts[MAX_SETS];
double frontmost[MAX_SETS];
double frontfiltered[MAX_SETS];
int    frontcounts[MAX_SETS];
int    has_tlast[MAX_SETS];

int main( int argc, char ** argv )
{
	int opt;
    while ((opt = getopt(argc, argv, "l:sun:x:")) != -1) {
        switch (opt) {
		case 'l':
			char * oain = optarg;
			line_graph_mode_depth = SimpleReadNumber( &oain, 0.0 / 0.0 );
			if( line_graph_mode_depth != line_graph_mode_depth ) goto failure;
			break;
        case 'u':
            unique_ranges = 1;
            break;
		case 's':
			suppress_stdout = 1;
			break;
        case 'n':
		{
			fixed_range_min_set = 1;
			char * oain = optarg;
			fixed_range_min = SimpleReadNumber( &oain, 0.0 / 0.0 );
			if( fixed_range_min != fixed_range_min ) goto failure;
            break;
		}
        case 'x':
		{
			fixed_range_max_set = 1;
			char * oain = optarg;
			fixed_range_max = SimpleReadNumber( &oain, 0.0 / 0.0 );
			if( fixed_range_max != fixed_range_max ) goto failure;
            break;
		}
		failure:
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-u (unique range)] [-n min] [-x max] [-s (suppress output duplication)] [-l line-graph-mode-depth]\n", argv[0]);
			return -1;
        }
    }

	CNFGSetup( "dyngraph", 1024, 768 );

	int head = 0;
	int highest_sets = 0;

	while(CNFGHandleInput())
	{
		char sline[16384];
		char * l = fgets( sline, sizeof(sline), stdin);
		if( !l ) break;
		times[head] = OGGetAbsoluteTime();

		char * lp;

		int fields = 0;

		if( !suppress_stdout )
		{
			fwrite( l, 1, strlen( l ), stdout );
			fflush( stdout );
		}

		do
		{
			lp = l;
			double v = data[head][fields] = SimpleReadNumber( &l, 0.0/0.0 );
			if( v != v ) break;
		} while( l != lp && fields++ < MAX_SETS - 1 );
		fields_count[head] = fields;

		if( fields > highest_sets )
		{
			highest_sets = fields;
		}
		for( ; fields < MAX_SETS; fields++ )
		{
			data[head][fields] = 0.0/0.0;
		}

		CNFGBGColor = 0x000010ff; //Dark Blue Background

		short w, h;
		CNFGClearFrame();
		CNFGGetDimensions( &w, &h );


		int margin_x = line_graph_mode_depth ? 20 : 100;
		int margin_y = 20;
		int margin_w = 20;
		int margin_h = 20;

		int tw = w - margin_x - margin_w;
		int th = h - margin_y - margin_h;

		int tx, n, f;

		for( n = 0; n < highest_sets; n++ )
		{
			tmin[n] = DBL_MAX;
			tmax[n] = DBL_MIN;
			tavgs[n] = 0;
			tcnts[n] = 0;
			tstds[n] = 0;
		}

		int maxf = 0;

		int hist = line_graph_mode_depth ? line_graph_mode_depth : tw;

		double tdtime = 0;
		double otime = 0;
		int tdtimects = 0;

		for( tx = 0; tx < hist; tx++ )
		{
			int eh = ((((head - tx) % MAX_PTS ) + MAX_PTS ) % MAX_PTS );
			int fc = fields_count[eh];
			if( fc > maxf ) maxf = fc;
			int has_data = 0;

			for( f = 0; f < fc; f++ )
			{
				double d = data[eh][f];
				if( d != d ) continue;
				if( d < tmin[f] ) tmin[f] = d;
				if( d > tmax[f] ) tmax[f] = d;
				tavgs[f] += d;
				tcnts[f] ++;
				has_data = 1;
			}

			if( has_data )
			{
				double t = times[eh];
				if( otime == 0 )
				{
					otime = t;
				}
				else
				{
					tdtime += t - otime;
					otime = t;
					tdtimects++;
				}
			}
		}
		tdtime /= -tdtimects;

		for( f = 0; f < maxf; f++ )
		{
			tavgs[f] /= tcnts[f];
		}


		double gmin = DBL_MAX;
		double gmax = DBL_MIN;

		for( n = 0; n < highest_sets; n++ )
		{
			if( fixed_range_min_set ) tmin[n] = fixed_range_min;
			if( fixed_range_max_set ) tmax[n] = fixed_range_max;
			if( tmin[n] < gmin ) gmin = tmin[n];
			if( tmax[n] > gmax ) gmax = tmax[n];
		}

		if( !unique_ranges )
		{
			for( n = 0; n < highest_sets; n++ )
			{
				tmin[n] = gmin;
				tmax[n] = gmax;
			}
		}

		int lbold = bolded;
		bolded = -1;

		if( boldedPin >= 0 ) lbold = boldedPin;

		CNFGColor( 0x202020ff );

		CNFGTackSegment( margin_x, margin_y, w-margin_w, margin_y );
		CNFGTackSegment( margin_x, margin_y, margin_x, h-margin_h );
		CNFGTackSegment( margin_x, h-margin_h, w-margin_w, h-margin_h );
		CNFGTackSegment( w-margin_w, margin_y, w-margin_w, h-margin_h );

		//Change color to white.
		CNFGColor( 0xffffffff );

		double base = th - margin_h;
		for( n = 0; n < highest_sets; n++ )
		{
			invrange[n] = 1./(tmax[n]-tmin[n]);
			tlasts[n] = 0;
			frontmost[n] = 0;
			frontfiltered[n] = 0;
			frontcounts[n] = 0;
			has_tlast[n] = 0;
		}

		float fdistclosest = 5.0;

		if( line_graph_mode_depth )
		{
			int td;
			for( td = 0; td < line_graph_mode_depth; td++ )
			{
				int itd = line_graph_mode_depth - 1 - td;
				int eh = ((((head - itd) % MAX_PTS ) + MAX_PTS ) % MAX_PTS );
				int col = 255 - ((255*itd)/line_graph_mode_depth);
				CNFGColor( 0xff | (col<<8) | (col<<16) | (col<<24) );
				int lastx = 0;
				int lasty = 0;
				for( f = 0; f < highest_sets; f++ )
				{
					double lw = w - margin_x - margin_w-1;
					double x = margin_x + f * (lw/(highest_sets-1));

					double d = data[eh][f];
					double y = h - margin_h - th * ((d - tmin[f]) * invrange[f]);

					if( !has_tlast[ f ] )
					{
						tlasts[ f ] = y;
						has_tlast[ f ] = 1;
					}

					if( f > 0 )
					{
						CNFGTackSegment( lastx, lasty, x, y );
					}
					CNFGTackRectangle( x-2, y-2, x+2, y+2 );
					lastx = x;
					lasty = y;
				}
			}
		}
		else
		{
			// Regular time series mode
			for( f = 0; f < highest_sets; f++ )
			{
				int tline = lbold == f;
				if( tline ) CNFGSetLineWidth( 3 );
				CNFGColor( palette[f%(sizeof(palette)/sizeof(palette[0]))] );
				
				for( tx = 0; tx < tw; tx++ )
				{
					int eh = ((((head - tx) % MAX_PTS ) + MAX_PTS ) % MAX_PTS );
					int fc = fields_count[eh];
					int x = tx + margin_x;
					if( fc <= f ) continue;

					double d = data[eh][f];
					double y = h - margin_h - th * ((d - tmin[f]) * invrange[f]);

					if( !has_tlast[ f ] )
					{
						tlasts[ f ] = y;
						has_tlast[ f ] = 1;
					}

					double lasty = tlasts[f];
					CNFGTackSegment( x, lasty, x+1, y );

					float distsq = (lastMouseX-x)*(lastMouseX-x)+(lastMouseY-y)*(lastMouseY-y);
					if( distsq < fdistclosest )
					{
						fdistclosest = distsq;
						bolded = f;
					}

					tlasts[f] = y;

					if( frontcounts[f] < 10 )
					{
						if( frontcounts[f] == 0 )
							frontmost[f] = d;
						frontcounts[f]++;
						frontfiltered[f] += y;
					}

					tstds[f] += (d - tavgs[f])*(d - tavgs[f]);
				}
				if( tline ) CNFGSetLineWidth( 1 );
			}
		}

		CNFGColor( 0xffffffff );

		if( !unique_ranges )
		{

			for( int foregrounded = 0; foregrounded < 2; foregrounded++ )
			{
				CNFGSetLineWidth( 1 + (foregrounded?0:2) );
				CNFGColor( foregrounded ? 0xffffffff: 0x000000ff );

				CNFGPenX = margin_x + 2; CNFGPenY = 3;
				char cts[512];
				snprintf( cts, sizeof(cts)-1, "Max %.3f", gmax );
				CNFGDrawText( cts, 3 );

				CNFGPenX = margin_x + 2; CNFGPenY = h - margin_h + 3;
				snprintf( cts, sizeof(cts)-1, "Min %.3f", gmin );
				CNFGDrawText( cts, 3 );

				int tw, th;
				snprintf( cts, sizeof(cts)-1, "Range: %.3f Sets: %d", gmax-gmin, highest_sets );
				CNFGGetTextExtents( cts, &tw, &th, 3 );
				CNFGPenX = w - margin_w - 2 - tw; CNFGPenY = h - margin_h + 3;
				CNFGDrawText( cts, 3 );

				snprintf( cts, sizeof(cts)-1, "%.2f Hz", 1.0/tdtime );
				CNFGGetTextExtents( cts, &tw, &th, 3 );
				CNFGPenX = w - margin_w - 2 - tw; CNFGPenY = 3;
				CNFGDrawText( cts, 3 );
			}
		}

		for( f = 0; f < maxf; f++ )
			tstds[f] = sqrt( tstds[f] ) / tcnts[f];

		
		for( int foregrounded = 0; foregrounded < 2; foregrounded++ )
		{
			CNFGSetLineWidth( 1 + (foregrounded?0:2) );
			CNFGColor( foregrounded ? 0xffffffff: 0x000000ff );

			CNFGPenX = w - 340; CNFGPenY = h - (maxf+2)*16 - 10;
			CNFGDrawText( "Average", 3 );

			CNFGPenX = w - 208; CNFGPenY = h - (maxf+2)*16 - 10;
			CNFGDrawText( "Std", 3 );

			CNFGPenX = w - 128; CNFGPenY = h - (maxf+2)*16 - 10;
			CNFGDrawText( "R%", 3 );
		}

		if( line_graph_mode_depth )
		{
			// What would we put here for line graph mode?
		}
		else
		{
			for( f = 0; f < maxf; f++ )
			{
				for( int foregrounded = 0; foregrounded < 2; foregrounded++ )
				{
					int tline = lbold == f;
					if( tline ) CNFGSetLineWidth( 2 + (foregrounded?0:2) );
					else CNFGSetLineWidth(1+(foregrounded?0:2));

					char cts[512];
					CNFGColor( foregrounded ? palette[f%(sizeof(palette)/sizeof(palette[0]))] : 0x000000ff );

					CNFGPenX = w - 400; CNFGPenY = h - (maxf+1)*16 - 10 + f * 16;
					snprintf( cts, sizeof(cts)-1, "%2d:%12.3f%11.3f%8.3f", f, tavgs[f], tstds[f], tstds[f] * 100 / tavgs[f] );
					CNFGDrawText( cts, 3 );
					if( lastMouseX >= CNFGPenX && lastMouseY >= CNFGPenY && lastMouseY < CNFGPenY + 16 ) bolded = f;

					int tw, th;
					snprintf( cts, sizeof(cts)-1, "%.3f", frontmost[f] );
					CNFGGetTextExtents( cts, &tw, &th, 3 );
					CNFGPenX = margin_x - tw - 3; CNFGPenY = frontfiltered[f]/frontcounts[f] - 7;
					CNFGDrawText( cts, 3 );
					if( lastMouseX >= 0 && lastMouseX < margin_x && lastMouseY >= CNFGPenY && lastMouseY < CNFGPenY + 16 ) bolded = f;
				}
			}
		}

		CNFGSetLineWidth( 1 );

		//Display the image and wait for time to display next frame.
		CNFGSwapBuffers();

		head = (head+1) % MAX_PTS;		
	}
}
