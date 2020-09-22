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
	int dx = (x1-x0);
	int dy = (y1-y0);
	int sdx = (dx>0)?1:-1;
	int sdy = (dy>0)?1:-1;
	int x = x0;
	int y = y0;

	if( dx == 0 )
	{
		if( x0 < 0 || x0 > BRESEN_W - 1 ) return;
		if( y < 0 ) y = 0;
		if( y > BRESEN_H - 1 )  y = BRESEN_H - 1;
		if( y1 < 0 ) y1 = 0;
		if( y1 > BRESEN_H - 1 )  y1 = BRESEN_H - 1;

		while( y != y1 )
		{
			BresenPixel( x0, y );
			y += sdy;
		} 
		BresenPixel( x0, y );
		return;
	}

	int yerr = 128;
	int yerrnumerator = (dy * sdy) * 256 / ( dx * sdx );

	printf( "Entering: (%d %d) -> (%d %d)\n", x, y, x1, y1 );
	if( x < 0 )
	{
		int dx = 0 - x;
		y += (yerrnumerator * dx)/256;
		x = 0;
	}

	if( x > BRESEN_W-1 )
	{
		int dx = x - (BRESEN_W-1);
		y -= (yerrnumerator * dx)/256;
		x = BRESEN_W-1;
	}

	if( x1 < 0 ) x1 = 0;
	if( x1 > BRESEN_W-1) x1 = BRESEN_W-1;

	printf( "LEAVING: %d %d -> %d %d (%d %d)\n", x, y, x1, y1, sdx, sdy );

	for( ; x != x1; x+=sdx )
	{
		BresenPixel( x, y );
		yerr += yerrnumerator;
		while( yerr > 255 )
		{
			y += sdy;
			BresenPixel( x, y );
			yerr -= 256;
		}
	}
	BresenPixel( x, y );



#if 0
    x0 = (OLED_WIDTH - 1) - x0;
    y0 = (OLED_HEIGHT - 1) - y0;
    x1 = (OLED_WIDTH - 1) - x1;
    y1 = (OLED_HEIGHT - 1) - y1;

	int deltax = x1 - x0;
	int deltay = y1 - y0;
	int error = 0;
	int x;
	int sy = LABS(deltay);
	int ysg = (y0>y1)?-1:1;
	int y = y0;

	if( x0 < 0 && x1 < 0 ) return;
	if( x0 >= OLED_WIDTH && x1 >= OLED_WIDTH ) return;

    fbChanges = true;

	//My own spin on bresenham's.
	if( deltax == 0 )
	{
		if( y1 == y0 )
		{
			if( y >= 0 && y < OLED_HEIGHT && x1 >= 0 && x1 < OLED_WIDTH )
				drawPixelFastWhite( x1, y );
			return;
		}

		y = y0;
		if( y < 0 ) y = 0;
		if( y >= OLED_HEIGHT ) y = OLED_HEIGHT - 1;
		if( y1 < 0 ) y = 0;
		if( y1 >= OLED_HEIGHT ) y = OLED_HEIGHT - 1;
		for( ; y != y1; y+=ysg )
			drawPixelFastWhite( x1, y );
		return;
	}

	int deltaerr = (deltay * 256) / deltax;
	int sx = LABS(deltax);
	deltaerr = LABS(deltaerr);
	int xsg = (x0>x1)?-1:1;

	//TODO: Compute early-outs.  Them we can forego checks.
	//ysg is always minimum y component.  Something like this:
	if( y < 0 )
	{
		if( ysg < 0 ) return;
		int yz = 0 - y;
		x0 += (deltax * yz) / sy;
		y = 0;
	}
	if( y >= OLED_HEIGHT )
	{
		if( ysg > 0 ) return;
		int yz = OLED_HEIGHT - y + 1;
		x0 += (deltax * yz) / sy;
		y = OLED_HEIGHT-1;
	}
	if( x0 < 0 )
	{
		if( xsg < 0 ) return;
		int xz = 0 - x0;
		y += (deltay * xz) / sx;
		x0 = 0;
	}
	if( x0 >= OLED_WIDTH )
	{
		if( xsg < 0 ) return;
		int xz = OLED_WIDTH - x0 + 1;
		y += (deltay * xz) / sx;
		x0 = OLED_WIDTH-1;
	}

	if( y1 < 0 )
	{
		if( ysg < 0 ) return;
		int yz = 0 - y1;
		x1 += (deltax * yz) / sy;
		y = 0;
	}
	if( y1 >= OLED_HEIGHT )
	{
		if( ysg > 0 ) return;
		int yz = OLED_HEIGHT - y1 + 1;
		x1 += (deltax * yz) / sy;
		y1 = OLED_HEIGHT-1;
	}
	if( x1 < 0 )
	{
		if( xsg < 0 ) return;
		int xz = 0 - x1;
		y1 += (deltay * xz) / sx;
		x1 = 0;
	}
	if( x1 >= OLED_WIDTH )
	{
		if( xsg < 0 ) return;
		int xz = OLED_WIDTH - x1 + 1;
		y1 += (deltay * xz) / sx;
		x1 = OLED_WIDTH-1;
	}

	for( x = x0; x != x1; x+=xsg )
	{
		drawPixelFastWhite(x,y);
		error = error + deltaerr;
		while( error >= 128 )
		{
			y += ysg;
			drawPixelFastWhite(x, y);
			error = error - 256;
		}
	}
	drawPixelFastWhite(x1,y1);
#endif




#if 0
	//Figure out which slope is greater.
	if( adx > ady )
	{
		//X's slope is higher, so we should 
	}
	else
	{
		//Y's slope is higher.
	}

	int i;
	if( y1 < y0 )
	{
		i = y0; y0 = y1; y1 = i;
		i = x0; x0 = x1; x1 = i;
	}
	//y1 >= y0

	int remlimit = (dx+1)/2; //round up

	rem = (x-x0) * dy % dx;
	y = y0 + (x-x0) * dy / dx;
	if (rem >= remlimit)
	{
	rem-=dx;
	y+=1;
	}
#endif
}

int main()
{
	CNFGSetup( "bresenham", SCREEN_W, SCREEN_H );
	while(1)
	{
		CNFGBGColor = 0x101010;
		CNFGClearFrame();
		int i;
		CNFGColor( 0xffffff );
		CNFGTackSegment( BRESEN_W/2, BRESEN_H/2, SCREEN_W - BRESEN_W/2, BRESEN_H/2 );
		CNFGTackSegment( SCREEN_W - BRESEN_W/2, BRESEN_H/2, SCREEN_W - BRESEN_W/2, SCREEN_H - BRESEN_H/2 );
		CNFGTackSegment( SCREEN_W - BRESEN_W/2, SCREEN_H - BRESEN_H/2, BRESEN_W/2, SCREEN_H - BRESEN_H/2 );
		CNFGTackSegment( BRESEN_W/2, SCREEN_H - BRESEN_H/2, BRESEN_W/2, BRESEN_H/2 );
		for( i = 0; i < 1; i++ )
		{
			int maxx = SCREEN_W;//BRESEN_W;
			int maxy = SCREEN_H;//BRESEN_H;

			int x0 = rand()%maxx;
			int y0 = rand()%maxy;
			int x1 = rand()%maxx;
			int y1 = rand()%maxy;

			CNFGColor( 0xff00ff );
			CNFGTackSegment( x0, y0, x1, y1 );
			CNFGTackSegment( x0+1, y0, x1+1, y1 );
			CNFGTackSegment( x0, y0+1, x1, y1+1 );
			CNFGTackSegment( x0+1, y0+1, x1+1, y1+1 );

			CNFGColor( 0x00ff00 );
			BresenhamDrawLineWithClip( x0 - BRESEN_W/2, y0 - BRESEN_H/2, x1 - BRESEN_W/2, y1 - BRESEN_H/2 );
			printf( "%d %d -> %d %d\n", x0, y0, x1, y1 );
		}
		CNFGSwapBuffers();
		while( !advance )
			CNFGHandleInput();
		advance = 0;
	}
}

