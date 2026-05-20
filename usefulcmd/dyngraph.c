//Make it so we don't need to include any other C files in our build.
#define CNFG_IMPLEMENTATION

#include "rawdraw_sf.h"
#include <float.h>
#include <math.h>

void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) { }
int HandleDestroy() { return 0; }

int unique_ranges;
int fixed_range_min_set;
int fixed_range_max_set;
double fixed_range_min;
double fixed_range_max;

double SimpleReadNumber( char ** number_ptr, double defaultNumber )
{
	if( !number_ptr ) return defaultNumber;

	const char * number = *number_ptr;
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
		if( nc == 0 ) return 0;
		else if( nc == 'x' ) radix = 16;
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

#define MAX_SETS 8

uint32_t palette[MAX_SETS] = {
	0xa5c266ff,
	0x628dfcff,
	0xcba08dff,
	0xc38ae7ff,
	0x54d8a6ff,
	0x2fd9ffff,
	0x9fc4e5ff,
	0xb3b3b3ff,
};

#define MAX_PTS 8192

double data[MAX_PTS][MAX_SETS];
int    fields_count[MAX_PTS];

int main( int argc, char ** argv )
{
	int opt;
    while ((opt = getopt(argc, argv, "un:x:")) != -1) {
        switch (opt) {
        case 'u':
            unique_ranges = 1;
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
            fprintf(stderr, "Usage: %s [-u (unique range)] [-n min] [-x max] name\n", argv[0]);
			return -1;
        }
    }

	CNFGSetup( "dyngraph", 1024, 768 );

	int head = 0;

	while(CNFGHandleInput())
	{
		char sline[1024];
		char * l = fgets( sline, sizeof(sline), stdin);
		if( !l ) break;

		char * lp;

		int fields = 0;
		do
		{
			lp = l;
			data[head][fields] = SimpleReadNumber( &l, 0 );
		} while( l != lp && fields++ < MAX_SETS );
		fields_count[head] = fields;

		CNFGBGColor = 0x000010ff; //Dark Blue Background

		short w, h;
		CNFGClearFrame();
		CNFGGetDimensions( &w, &h );

		//Change color to white.
		CNFGColor( 0xffffffff );

		int margin_x = 100;
		int margin_y = 20;
		int margin_w = 20;
		int margin_h = 20;

		int tw = w - margin_x - margin_w;
		int th = h - margin_y - margin_h;

		int tx, n, f;
		double tmin[MAX_SETS];
		double tmax[MAX_SETS];
		double tavgs[MAX_SETS] = { 0 };
		double tcnts[MAX_SETS] = { 0 };

		for( n = 0; n < MAX_SETS; n++ )
		{
			tmin[n] = DBL_MAX;
			tmax[n] = DBL_MIN;
		}

		int maxf = 0;

		for( tx = 0; tx < tw; tx++ )
		{
			int eh = ((((head - tx) % MAX_PTS ) + MAX_PTS ) % MAX_PTS );
			int fc = fields_count[eh];
			if( fc > maxf ) maxf = fc;

			for( f = 0; f < fc; f++ )
			{
				double d = data[eh][f];
				if( d < tmin[f] ) tmin[f] = d;
				if( d > tmax[f] ) tmax[f] = d;
				tavgs[f] += d;
				tcnts[f] ++;
			}
		}

		for( f = 0; f < maxf; f++ )
		{
			tavgs[f] /= tcnts[f];
		}

		double tstds[MAX_SETS] = { 0 };

		double gmin = DBL_MAX;
		double gmax = DBL_MIN;

		for( n = 0; n < MAX_SETS; n++ )
		{
			if( fixed_range_min_set ) tmin[n] = fixed_range_min;
			if( fixed_range_max_set ) tmax[n] = fixed_range_max;
			if( tmin[n] < gmin ) gmin = tmin[n];
			if( tmax[n] > gmax ) gmax = tmax[n];
		}

		if( !unique_ranges )
		{
			for( n = 0; n < MAX_SETS; n++ )
			{
				tmin[n] = gmin;
				tmax[n] = gmax;
			}
		}

		double base = th - margin_h;
		double invrange[MAX_SETS];
		for( n = 0; n < MAX_SETS; n++ )
		{
			invrange[n] = 1./(tmax[n]-tmin[n]);
		}

		double tlasts[MAX_SETS];
		int    has_tlast[MAX_SETS] = { 0 };


		for( f = 0; f < MAX_SETS; f++ )
		{

			CNFGColor( palette[f] );
			
			for( tx = 0; tx < tw; tx++ )
			{
				int eh = ((((head - tx) % MAX_PTS ) + MAX_PTS ) % MAX_PTS );
				int fc = fields_count[eh];
				int x = tx + margin_x;
				if( fc <= f ) continue;

				if( !has_tlast[ f ] )
				{
					double d = data[eh][f];
					double y = h - margin_h - th * ((d - tmin[f]) * invrange[f]);
					tlasts[ f ] = y;
					has_tlast[ f ] = 1;
				}

				double lasty = tlasts[f];
				double d = data[eh][f];
				double y = h - margin_h - th * ((d - tmin[f]) * invrange[f]);
				CNFGTackSegment( x, lasty, x+1, y ); 
				tlasts[f] = y;

				tstds[f] += (d - tavgs[f])*(d - tavgs[f]);
			}
		}

		CNFGColor( 0xffffffff );

		if( !unique_ranges )
		{
			CNFGPenX = 1; CNFGPenY = margin_y;
			char cts[512];
			snprintf( cts, sizeof(cts)-1, "%.3f", gmax );
			CNFGDrawText( cts, 3 );

			CNFGPenX = 1; CNFGPenY = h - margin_h;
			snprintf( cts, sizeof(cts)-1, "%.3f", gmin );
			CNFGDrawText( cts, 3 );

			CNFGPenX = 1; CNFGPenY = 1;
			snprintf( cts, sizeof(cts)-1, "Range: %.3f", gmax-gmin );
			CNFGDrawText( cts, 3 );
		}

		for( f = 0; f < maxf; f++ )
		{
			char cts[512];
			CNFGColor( palette[f] );

			tstds[f] = sqrt( tstds[f] ) / tcnts[f];

			CNFGPenX = w - 400; CNFGPenY = h - 200 + f * 14;
			snprintf( cts, sizeof(cts)-1, "%d: AVG: %.3f STD: %.3f R%%: %.3f", f, tavgs[f], tstds[f], tstds[f] * 100 / tavgs[f] );
			CNFGDrawText( cts, 3 );
		}


		//Display the image and wait for time to display next frame.
		CNFGSwapBuffers();

		head = (head+1) % MAX_PTS;		
	}
}
