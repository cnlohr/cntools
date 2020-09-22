#include <stdio.h>

#define CNFG_IMPLEMENTATION
#include <CNFG.h>

#define SCREEN_W 640
#define SCREEN_H 480

int advance;
void HandleKey( int keycode, int bDown ) { if( bDown && keycode == 'a' ) advance = 1; }
void HandleButton( int x, int y, int button, int bDown ){}
void HandleMotion( int x, int y, int mask ){}
void HandleDestroy(){}

#define BRESEN_W 320
#define BRESEN_H 240

int bx0, by0, bx1, by1;
void BresenPixel( int x, int y )
{
	if( x < 0 || x >= BRESEN_W || y < 0 || y >= BRESEN_H )
	{
		fprintf( stderr, "ERROR: PIXEL BAD: %d %d From %d %d -> %d %d\n", x, y, bx0, by0, bx1, by1 );
		exit( 0);
	}
	else
	{
		CNFGTackPixel( x + BRESEN_W/2, y+ BRESEN_H/2 );
		//CNFGTackPixel( x, y );
	}
}

void BresenhamDrawLineWithClip( int x0, int y0, int x1, int y1 )
{
#define FIXEDPOINT 256
#define FIXEDPOINTD2 (FIXEDPOINT/2)
	printf( "\n" );
			printf( "%d %d -> %d %d\n", x0, y0, x1, y1 );


	int dx = (x1-x0);
	int dy = (y1-y0);
	int sdx = (dx>0)?1:-1;
	int sdy = (dy>0)?1:-1;
	int x = x0;
	int y = y0;

	int yerr = FIXEDPOINTD2;
	int xerr = FIXEDPOINTD2;
	int yerrdiv = ( dx * sdx );  //dy, but always positive.
	int xerrdiv = ( dy * sdy );  //dx, but always positive.
	int yerrnumerator;
	int xerrnumerator;


	if( yerrdiv > 0 )
	{
		yerrnumerator = ((dy * sdy) * FIXEDPOINT + yerrdiv/2) / yerrdiv;
		int dxA = 0;
		if( x < 0 )
		{
			dxA = 0 - x;
			x = 0;
		}
		if( x > BRESEN_W-1 )
		{
			dxA = (x - (BRESEN_W-1));
			x = BRESEN_W-1;
		}
		if( dxA )
		{
			int yn = (yerrnumerator * dxA)*sdy;
			yn += FIXEDPOINTD2 * sdy; //This "feels" right
			y += yn/FIXEDPOINT;
		}

		if( sdy > 0 )
		{
			if( y > BRESEN_H-1 ) return;
			if( y > y1 ) return;
		}
		else
		{
			if( y < 0 ) return;
			if( y < y1 ) return;
		}
	}

	if( xerrdiv > 0 )
	{
		xerrnumerator = ((dx * sdx) * FIXEDPOINT + xerrdiv/2 ) / xerrdiv;
		int dyA = 0;	
		if( y < 0 )
		{
			dyA = 0 - y;
			y = 0;
		}
		if( y > BRESEN_H-1 )
		{
			dyA = y - (BRESEN_H-1);
			y = BRESEN_H-1;
		}
		if( dyA )
		{
			int xn = (xerrnumerator*dyA)*sdx;
			xn += FIXEDPOINTD2 * sdx; //This "feels" right.
			x += xn/FIXEDPOINT;
		}

		if( sdx > 0 )
		{
			if( x > BRESEN_W-1 ) return;
			if( x > x1 ) return;
		}
		else if( dx < 0 )
		{
			if( x < 0 ) return;
			if( x < x1 ) return;
		}
	}

	//This code checks both for vertical/horizontal line errors
	// as well as to see if we've created a bad situation in the code above.
	if( dx == 0 )
	{
		if( x < 0 ) return;
		if( x > BRESEN_W-1 ) return;
	}
	if( dy == 0 )
	{
		if( y < 0 ) return;
		if( y > BRESEN_H-1 ) return;
	}

	//We put the checks above to check this, in case we have a situation where
	// we have a 0-length line outside of the viewable area.  If that happened,
	// we would have aborted before hitting this code.
	if( dx == 0 && dy == 0 )
	{
		BresenPixel( x0, y0 );
		return;
	}

	if( xerrdiv > yerrdiv )
	{
		//Force clip end coordinate.
		//NOTE: We have an extra check within the inner loop, to avoid complicated math here.
		if( y1 < 0 ) y1 = 0;
		if( y1 > BRESEN_H-1) y1 = BRESEN_H-1;

		for( ; y != y1; y+=sdy )
		{
			BresenPixel( x, y );
			xerr += xerrnumerator;
			while( xerr >= FIXEDPOINT )
			{
				x += sdx;
				//Theoretically, we could math this so that in the end-coordinate clip stage
				//to make sure this condition just could never be hit, however, that is very
				//difficult to guarantee under all situations and may have weird edge cases.
				//So, I've decided to stick this here.
				if( x < 0 || x >= BRESEN_W-1 ) goto abortline;
				BresenPixel( x, y );
				xerr -= FIXEDPOINT;
			}
		}
		BresenPixel( x, y );
	}
	else
	{
		//Force clip end coordinate.
		//NOTE: We have an extra check within the inner loop, to avoid complicated math here.
		if( x1 < 0 ) x1 = 0;
		if( x1 > BRESEN_W-1) x1 = BRESEN_W-1;

		for( ; x != x1; x+=sdx )
		{
			BresenPixel( x, y );
			yerr += yerrnumerator;
			while( yerr >= FIXEDPOINT )
			{
				y += sdy;
				//Theoretically, we could math this so that in the end-coordinate clip stage
				//to make sure this condition just could never be hit, however, that is very
				//difficult to guarantee under all situations and may have weird edge cases.
				//So, I've decided to stick this here.
				if( y < 0 || y >= BRESEN_H-1 ) goto abortline;
				BresenPixel( x, y );
				yerr -= FIXEDPOINT;
			}
		}
		BresenPixel( x, y );
	}
abortline:
	;
	printf( "\n" );
}

int main()
{
	int fv = 0;
	CNFGSetup( "bresenham", SCREEN_W, SCREEN_H );
	while(1)
	{
		fv++;
		CNFGBGColor = 0x101010;
		CNFGClearFrame();
		int i;
		CNFGColor( 0xffffff );
		CNFGTackSegment( BRESEN_W/2, BRESEN_H/2, SCREEN_W - BRESEN_W/2, BRESEN_H/2 );
		CNFGTackSegment( SCREEN_W - BRESEN_W/2, BRESEN_H/2, SCREEN_W - BRESEN_W/2, SCREEN_H - BRESEN_H/2 );
		CNFGTackSegment( SCREEN_W - BRESEN_W/2, SCREEN_H - BRESEN_H/2, BRESEN_W/2, SCREEN_H - BRESEN_H/2 );
		CNFGTackSegment( BRESEN_W/2, SCREEN_H - BRESEN_H/2, BRESEN_W/2, BRESEN_H/2 );
		for( i = 0; i < 3; i++ )
		{
			int maxx = SCREEN_W;//BRESEN_W;
			int maxy = SCREEN_H;//BRESEN_H;
#if 1
			int x0 = rand()%maxx;
			int y0 = rand()%maxy;
			int x1 = rand()%maxx;
			int y1 = rand()%maxy;
#else
			int x0 = SCREEN_W/2;
			int y0 = SCREEN_H/2-10+fv;
			int x1 = SCREEN_W/2;
			int y1 = SCREEN_H/2;
#endif		
			CNFGColor( 0xff00ff );
			int dx, dy;
			for( dy = -1; dy <= 1; dy ++) for( dx = -1; dx <= 1; dx++ )
			{
				CNFGTackSegment( x0+dx, y0+dy, x1+dx, y1+dy );
			}

			CNFGColor( 0x00ff00 );
			BresenhamDrawLineWithClip( x0 - BRESEN_W/2, y0 - BRESEN_H/2, x1 - BRESEN_W/2, y1 - BRESEN_H/2 );
		}
		CNFGSwapBuffers();
	//	while( !advance )
			CNFGHandleInput();
		advance = 0;
	}
}

