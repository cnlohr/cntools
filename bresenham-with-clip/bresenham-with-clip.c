/*
	This is my algorithm for using Bresenham's line algorithm with clipping.
	This makes it possibel to speed along very quickly without having to check
	to see if the pixels you are writing are still within the framebuffer.

	This is the entire function and boostrap test.  Extract this function,
	BresenhamDrawLineWithClip and write your own BresenPixel. 

	Also, note that in some cases it may make sense to not use the BresenPixel
	function at all but rather update indices from within the inner loop.

	I'm confident there's a faster, or more efficient way to do this, but this
	is the balance of quality to speed to size that I've settled on.

	(C) 2020 <>< Charles Lohr, the following code may be used in any project
	for any purpose or fitness.  You may freely license the following code under
	the MIT/x11 or NewBSD Licenses.  I make NO WARRANTY regarding the quality,
	fitness or function of the following code.  It may have errors.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define CNFG_IMPLEMENTATION
#include <CNFG.h>

#define SCREEN_W 640
#define SCREEN_H 480


//Press 'A' to advance frames if in frame advance mode.
int advance;
void HandleKey( int keycode, int bDown ) { if( bDown && keycode == 'a' ) advance = 1; }
void HandleButton( int x, int y, int button, int bDown ){}
void HandleMotion( int x, int y, int mask ){}
void HandleDestroy(){}

#define BRESEN_W 320
#define BRESEN_H 240
#define MARGIN_X 200
#define MARGIN_Y 200

int bx0, by0, bx1, by1;
void drawPixelUnsafe( int x, int y )
{
	if( x < 0 || x >= BRESEN_W || y < 0 || y >= BRESEN_H )
	{
		fprintf( stderr, "ERROR: PIXEL BAD: %d %d From %d %d -> %d %d\n", x, y, bx0, by0, bx1, by1 );
		exit( 0);
	}
	else
	{
		CNFGTackPixel( x + MARGIN_X, y+ MARGIN_Y );
		//CNFGTackPixel( x, y );
	}
}

void BresenhamDrawLineWithClip( int x0, int y0, int x1, int y1 )
{
//Tune this as a function of the size of your viewing window, line accuracy, and worst-case scenario incoming lines.
//#define BRESEN_W OLED_WIDTH
//#define BRESEN_H OLED_HEIGHT
#define FIXEDPOINT 16
#define FIXEDPOINTD2 15
    int dx = (x1-x0);
    int dy = (y1-y0);
    int sdx = (dx>0)?1:-1;
    int sdy = (dy>0)?1:-1;
    int yerrdiv = ( dx * sdx );  //dy, but always positive.
    int xerrdiv = ( dy * sdy );  //dx, but always positive.
    int yerrnumerator = 0;
    int xerrnumerator = 0;
	int cx = x0;
	int cy = y0;

    if( cx < 0 && x1 < 0 ) return;
    if( cy < 0 && y1 < 0 ) return;
    if( cx >= BRESEN_W && x1 >= BRESEN_W ) return;
    if( cy >= BRESEN_H && y1 >= BRESEN_H ) return;

    //We put the checks above to check this, in case we have a situation where
    // we have a 0-length line outside of the viewable area.  If that happened,
    // we would have aborted before hitting this code.

    if( yerrdiv > 0 )
    {
        int dxA = 0;
        if( cx < 0 )
        {
            dxA = 0 - cx;
            cx = 0;
        }
        if( cx > BRESEN_W-1 )
        {
            dxA = (cx - (BRESEN_W-1));
            cx = BRESEN_W-1;
        }
        if( dxA || xerrdiv <= yerrdiv )
        {
            yerrnumerator = (((dy * sdy)<<16) + yerrdiv/2) / yerrdiv;
            if( dxA )
            {
                cy += (((yerrnumerator * dxA)) * sdy) >> FIXEDPOINT; //This "feels" right
                //Weird situation - if we cal, and now, both ends are out on the same side abort.
                if( cy < 0 && y1 < 0 ) return;
                if( cy > BRESEN_H-1 && y1 > BRESEN_H-1 ) return;
            }
        }
    }

    if( xerrdiv > 0 )
    {
        int dyA = 0;    
        if( cy < 0 )
        {
            dyA = 0 - cy;
            cy = 0;
        }
        if( cy > BRESEN_H-1 )
        {
            dyA = (cy - (BRESEN_H-1));
            cy = BRESEN_H-1;
        }
        if( dyA || xerrdiv > yerrdiv )
        {
            xerrnumerator = (((dx * sdx)<<16) + xerrdiv/2 ) / xerrdiv;
            if( dyA )
            {
                cx += (((xerrnumerator*dyA)) * sdx) >> FIXEDPOINT; //This "feels" right.
                //If we've come to discover the line is actually out of bounds, abort.
                if( cx < 0 && x1 < 0 ) return;
                if( cx > BRESEN_W-1 && x1 > BRESEN_W-1 ) return;
            }
        }
    }

    if( x1 == cx && y1 == cy )
    {
        drawPixelUnsafe( cx, cy );
        return;
    }

    //Make sure we haven't clamped the wrong way.
    //Also this checks for vertical/horizontal violations.
    if( dx > 0 )
    {
        if( cx > BRESEN_W-1 ) return;
        if( cx > x1 ) return;
    }
    else if( dx < 0 )
    {
        if( cx < 0 ) return;
        if( cx < x1 ) return;
    }

    if( dy > 0 )
    {
        if( cy > BRESEN_H-1 ) return;
        if( cy > y1 ) return;
    }
    else if( dy < 0 )
    {
        if( cy < 0 ) return;
        if( cy < y1 ) return;
    }

    //Force clip end coordinate.
    //NOTE: We have an extra check within the inner loop, to avoid complicated math here.
    //Theoretically, we could math this so that in the end-coordinate clip stage
    //to make sure this condition just could never be hit, however, that is very
    //difficult to guarantee under all situations and may have weird edge cases.
    //So, I've decided to stick this here.

    if( xerrdiv > yerrdiv )
    {
        int xerr = 1<<FIXEDPOINTD2;
        if( x1 < 0 ) x1 = 0;
        if( x1 > BRESEN_W-1) x1 = BRESEN_W-1;
        x1 += sdx; //Tricky - make sure the "next" mark we hit doesn't overflow.

        if( y1 < 0 ) y1 = 0;
        if( y1 > BRESEN_H-1 ) y1 = BRESEN_H-1;

        for( ; cy != y1; cy+=sdy )
        {
            drawPixelUnsafe( cx, cy );
            xerr += xerrnumerator;
            while( xerr >= (1<<FIXEDPOINT) )
            {
                cx += sdx;
                if( cx == x1 ) return;
                drawPixelUnsafe( cx, cy );
                xerr -= 1<<FIXEDPOINT;
            }
        }
        drawPixelUnsafe( cx, cy );
    }
    else
    {
        int yerr = 1<<FIXEDPOINTD2;

        if( y1 < 0 ) y1 = 0;
        if( y1 > BRESEN_H-1 ) y1 = BRESEN_H-1;
        y1 += sdy;        //Tricky: Make sure the NEXT mark we hit doens't overflow.

        if( x1 < 0 ) x1 = 0;
        if( x1 > BRESEN_W-1) x1 = BRESEN_W-1;

        for( ; cx != x1; cx+=sdx )
        {
            drawPixelUnsafe( cx, cy );
            yerr += yerrnumerator;
            while( yerr >= 1<<FIXEDPOINT )
            {
                cy += sdy;
                if( cy == y1 ) return;
                drawPixelUnsafe( cx, cy );
                yerr -= 1<<FIXEDPOINT;
            }
        }
        drawPixelUnsafe( cx, cy );
    }
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
		CNFGColor( 0x222244 );
		CNFGTackSegment( MARGIN_X, MARGIN_Y, MARGIN_X + BRESEN_W, MARGIN_Y );
		CNFGTackSegment( MARGIN_X + BRESEN_W, MARGIN_Y, MARGIN_X + BRESEN_W, MARGIN_Y + BRESEN_H );
		CNFGTackSegment( MARGIN_X + BRESEN_W, MARGIN_Y + BRESEN_H, MARGIN_X, MARGIN_Y + BRESEN_H );
		CNFGTackSegment( MARGIN_X, MARGIN_Y + BRESEN_H, MARGIN_X, MARGIN_Y );

		//CNFGTackSegment( BRESEN_W/2, BRESEN_H/2, SCREEN_W - BRESEN_W/2, BRESEN_H/2 );
		//CNFGTackSegment( SCREEN_W - BRESEN_W/2, BRESEN_H/2, SCREEN_W - BRESEN_W/2, SCREEN_H - BRESEN_H/2 );
		//CNFGTackSegment( SCREEN_W - BRESEN_W/2, SCREEN_H - BRESEN_H/2, BRESEN_W/2, SCREEN_H - BRESEN_H/2 );
		//CNFGTackSegment( BRESEN_W/2, SCREEN_H - BRESEN_H/2, BRESEN_W/2, BRESEN_H/2 );

		if( 0 )
		{
			CNFGColor( 0x00ff00 );
			//x0=-18452, y0=-15580, x1=128, y1=63
			//BresenhamDrawLineWithClip( -18452, -15580, 128, 63 );
			BresenhamDrawLineWithClip( -25656, 31744, 128, 0 );
		}
		else
		{
			for( i = 0; i < 1000; i++ )
			{
				int maxx = SCREEN_W*5 *(sin(fv/330.)+1);//BRESEN_W;
				int maxy = SCREEN_H*5 *(cos(fv/100.)+1);//BRESEN_H;
				int x = cos(fv/356.)*300+300;
				int y = cos(fv/432.)*300+300;
				if( maxx == 0 || maxy == 0 ) continue;
	#if 1
				int x0 = (rand()%maxx)-maxx/2+SCREEN_W/2+x;
				int y0 = (rand()%maxy)-maxy/2+SCREEN_H/2+y;
				int x1 = (rand()%maxx)-maxx/2+SCREEN_W/2+x;
				int y1 = (rand()%maxy)-maxy/2+SCREEN_H/2+y;
	#else
				int x0 = SCREEN_W/2;
				int y0 = SCREEN_H/2-10+fv;
				int x1 = SCREEN_W/2-200;
				int y1 = SCREEN_H/2;
	#endif		
	#if 0
				x0 = 100;
				y0 = 300;
				y1 = 400;
				x1 = (rand()%100) + 100;
	#endif
	#if 0
				x0 = 397+BRESEN_W/2; y0 = 1+BRESEN_H/2; x1 = 244+BRESEN_W/2; y1 = 0+BRESEN_H/2;
	#endif
				CNFGColor( 0xff00ff );
				int dx = 0, dy = 0;
				//for( dy = -1; dy <= 1; dy ++) for( dx = -1; dx <= 1; dx++ )
				{
					CNFGTackSegment( x0+dx, y0+dy, x1+dx, y1+dy );
				}

				CNFGColor( 0x00ff00 );
				BresenhamDrawLineWithClip( x0 - MARGIN_X, y0 - MARGIN_Y, x1 - MARGIN_X, y1 - MARGIN_Y );
			}
		}
		CNFGSwapBuffers();

		//Uncomment for frame advance mode.
		//while( !advance )
			CNFGHandleInput();
		advance = 0;
	}
}

