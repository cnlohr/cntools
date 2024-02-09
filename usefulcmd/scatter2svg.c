#include <stdio.h>
#include <string.h>

#define MAXRECORDS 32768
#define MAXCOLUMNS 16
#define BUFFERLEN  8192

float Data[MAXRECORDS][MAXCOLUMNS];
float RangeMin[MAXCOLUMNS];
float RangeMax[MAXCOLUMNS];
int MaxAt[MAXCOLUMNS];
int MinAt[MAXCOLUMNS];
float tnan;

const char * Colors[] = { "Grey", "Red", "Green", "Blue", "dimgray", "Purple", "turquoise", "crimson", "goldenrod", "midnightblue", "lightpink", "mediumspringgreen", "darkorchid", "gold", "maroon", "seagreen", "slateblue" };

int main()
{
	int x = 0;
	int y = 0;
	int maxx = 0;
	int maxy = 0;

	memset( Data, 0xff, sizeof( Data ) );
	memset( &tnan, 0xff, sizeof( tnan ) );

	for( x = 0; x < MAXCOLUMNS; x++ )
	{
		RangeMin[x] = 1e20;
		RangeMax[x] = -1e20;
	}

	x = 0;
	y = 0;

	while( !feof( stdin ) )
	{
		char ToParse[BUFFERLEN];
		char c, done = 0;
		int pl = 0;
		int notnumeric = 0;
		float value;

		while( (c = getchar()) != EOF && !done )
		{
			if( c == '\n' || c == ',' )
			{
				ToParse[pl] = 0;
				break;
			}

			if( ( c > '9' || c < '0' ) && ( c != ' ' ) && ( c != '-' ) && ( c != '.' ) )
				notnumeric = 1;

			ToParse[pl++] = c;
			if( pl >= BUFFERLEN - 1 )
			{
				fprintf( stderr, "ERROR: Buffer Overflow.\n" );
				pl = 0;
			}
		}

		ToParse[pl] = 0;

		if( notnumeric && pl )
		{
			Data[y][x] = tnan;
		}
		else
		{
			value = 0;
			if( sscanf( ToParse, "%g", &value ) == 1 )
			{
				Data[y][x] = value;
				if( value < RangeMin[x] )
				{
					RangeMin[x] = value;
					MinAt[x] = y;
				}
				if( value > RangeMax[x] )
				{
					RangeMax[x] = value;
					MaxAt[x] = y;
				}
			}
		}

		if( c == '\n' )
		{
			if( x > maxx ) maxx = x;
			x = 0;
			y++;
		}
		if( c == ',' )
			x++;

		if( x == MAXCOLUMNS )
		{
			fprintf( stderr, "ERROR: Too man columns.\n" );
			x = 0;
		}
		if( y == MAXRECORDS )
		{
			fprintf( stderr, "ERROR: Too many records.\n" );
			y--;
		}
	}

	if( x > 0 )
		maxy = y+1;
	else
		maxy = y;

	for( x = 0; x <= maxx; x++ )
	{
		fprintf( stderr, "Range: %f (%d) - %f (%d)\n", RangeMin[x], MaxAt[x], RangeMax[x], MaxAt[x] );
	}

	//Ok, we have our data loaded
	float RangeyMax = -1e20;
	float RangeyMin = 1e20;
	float RangexMax = RangeMax[0];
	float RangexMin = RangeMin[0];

	//Phase 2.
	//For the way we're displaying the data, we want all of the output columns to be bonded on the same scale.
	for( x = 1; x < MAXCOLUMNS; x++ )
	{
		if( RangeMin[x] < RangeyMin )
			RangeyMin = RangeMin[x];
		if( RangeMax[x] > RangeyMax )
			RangeyMax = RangeMax[x];
	}

	RangeyMax += 0.001;
	RangexMax += 0.001;

	float cmx = 16;
	float cmy = 11;
	float ppc = 50;

	puts( "<?xml version=\"1.0\" standalone=\"no\"?>" );
	puts( "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"" );
	puts( "  \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\" >" );
	printf( "<svg width=\"%fcm\" height=\"%fcm\" viewBox=\"0 0 %f %f\"\n", cmx, cmy, cmx * ppc, cmy * ppc );
 	puts( "    xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">" );
	puts( " <desc></desc>" );

	fprintf( stderr, "%d, %d\n", maxx, maxy );
	fprintf( stderr, "(%f-%f),(%f-%f)\n", RangexMin, RangexMax, RangeyMin, RangeyMax );

	for( y = 0; y < maxy; y++ )
	{
		float tvx = Data[y][0];
		for( x = 1; x < maxx+1; x++ )
		{
			const char * color = Colors[x];
			float tvy = Data[y][x];
			if( (!(tvy < RangeyMax)) || (!(tvy > RangeyMin)) ) continue;

			float scx = ((tvx - RangexMin) / (RangexMax - RangexMin)) * cmx;
			float scy = ((tvy - RangeyMin) / (RangeyMax - RangeyMin)) * cmy;

			printf( " <circle cx=\"%fcm\" cy=\"%fcm\" r=\"1\" stroke=\"%s\" stroke-width=\"3.000000\" opacity=\"1.000000\" />\n", scx, scy, color );
		}
	}
	puts( "</svg>" );
	return 0;
}
