/*
	====================================================================
	This module acts as a replacement for the Borland mouse and keyboard
	functions in the win32 environment.

    In WIN32, the keyboard and optionally the mouse, generate events
	that are decoded and stored in buffers.  The bioskey() and
	getmousestat() functions then retrieve these events from their
	respective buffers.  Since the mouse is optional, bioskey() checks
	for and buffers mouse events, and so it must be called for
	getmousestat() to function.

    key codes are compatible with the values in keys.h.  For ASCII keys
	the low byte of the return value is the ASCII code.  For non-ASCII
	keys, the low byte is zero and the high byte is the scan code.
	--------------------------------------------------------------------
	Rev    Date		By		Description
	-----  --------	-----	--------------------------------------------
		   8/10/22	srg		enables mouse input in init_con.
	-----  --------	-----	--------------------------------------------
		   12/3/20	SRG		Added scrollwindow().
	-----  --------	-----	--------------------------------------------
		   11/20/20 SRG		Added second init_con() that controls window
							and buffer sizes.
	-----  --------	-----	--------------------------------------------
		   3/4/19	SRG		restore_screen now correctly sets restore
							buffer size for restore.
	-----  --------	-----	--------------------------------------------
			4/30/18	SRG		Corrected VS2017 warnings (added typecast).
	-----  --------	-----	--------------------------------------------
			12/7/16	SRG		save_screen()/resore_screen() put the cursor
							in its original location.
	-----  --------	-----	--------------------------------------------
		   5/30/12	SRG		Corrected filtering of F5 key, which wasn't
							being passed to applications.
	-----  --------	-----	--------------------------------------------
		   5/10/12	SRG		Added gotoxyb which accepts coordinates in
							Borland format (relative to 1).  Also added
							bioskey( 2 ) that returns the state of the
							shift and control keys.
	-----  --------	-----	--------------------------------------------
		   10/17/11	SRG		Corrected mouse bug.  Calls to getmousestat
							when no windows mouse events are pending
							return the last mouse state, not 0.
	-----  --------	-----	--------------------------------------------
		   10/12/11 SRG		Added a 3-argument hilight from scan32.
							Also, getmousestat args can be NULL, and
							added save_screen() and restore_screen.
    -----  --------	-----	--------------------------------------------
	       07/26/11	SRG		build from pieces of various win32 programs.
	====================================================================
*/

//#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <ctype.h>

#include "win32con.h"
#include "ASCII.h"


#include "win32trace.h"


//	Globals to speed things up

void							*conout	= GetStdHandle( STD_OUTPUT_HANDLE );	// handle to stdout
void							*conin = GetStdHandle( STD_INPUT_HANDLE );		// & stdin
COORD							size = { 80, 2000 };							// desired dimensions
CONSOLE_SCREEN_BUFFER_INFO		text;											// screen dimensions


void init_con( int width, int height )
{
	int			i, j;
	SMALL_RECT	r = { 0, 0, (short) width - 1, (short) 39 };
	COORD		size = { (short) width, (short) height };
//	DWORD		list[ 50 ];

//	i = SetConsoleDisplayMode( conout, CONSOLE_WINDOWED_MODE, &size );
//	if ( !i ) TRACE( (char *) "Can't set mode %d (%s)\n", GetLastError( ), strerror( GetLastError( ) ) );
//	i = GetConsoleProcessList( list, 50 );
//	if ( !i )
//		TRACE( (char *) "Can't get process list\n" );
//	else
//	{
//		TRACE( (char *) "Process IDs: " );
//		for ( j = 0; j < i; j ++ ) TRACE( (char *) "%d%s", list[ j ], ( j < i - 1 ) ? ", " : "\n" );
//	}

	i = GetConsoleScreenBufferInfo( conout, &text );
	size.X = max( size.X, text.dwSize.X );
	size.Y = max( size.Y, text.dwSize.Y );
	i = SetConsoleScreenBufferSize( conout, size );								//	set the buffer size we need
	if ( !i ) printf( "init_con: %s\n", strerror( i = GetLastError( ) ) );
	j = SetConsoleWindowInfo( conout, true, &r );								//	set the actual screen size
	if ( i && !j )
		TRACE( (char *) "Can't create console: %d (%s)\n", GetLastError( ), strerror( GetLastError( ) ) );
//		i = SetConsoleScreenBufferSize( conout, size );							//	if the window to bigger than the buffer, we'll now fix the buffer size
		SetConsoleMode( conin, ENABLE_MOUSE_INPUT );
}


//	11/20/2020 set both window and buffer sizes

void init_con( int wwidth, int wheight, int bwidth, int bheight )
{
	int			i, j;
	SMALL_RECT	r = { 0, 0, (short) wwidth - 1, (short) wheight - 1 };
	COORD		size = { (short) bwidth, (short) bheight };

	i = SetConsoleScreenBufferSize( conout, size );								//	set the buffer size we need
	j = SetConsoleWindowInfo( conout, true, &r );								//	set the actual screen size
	if ( !i && j )
		i = SetConsoleScreenBufferSize( conout, size );							//	if the window to bigger than the buffer, we'll now fix the buffer size
	GetConsoleScreenBufferInfo( conout, &text );
}


//	how mouse events are stored

typedef struct
{
	short			x, y;				// mouse location
	unsigned long	bstate;				// button state
} mclick;								// how we store mouse info


//	Define the keyboard buffer

#define MAX_KBSIZE	100					// size of the keyboard buffer
#define MAX_MSIZE	500					// size of the mouse buffer


unsigned short	kbuf[ MAX_KBSIZE ];		// keyboard buffer
mclick			mbuf[ MAX_MSIZE ];		// the mouse buffer

int				kip = 0, kop = 0;		// keyboard buffer indexes
int				mip = 0, mop = 0;		// mouse buffer indexes
int				shift_state = 0;		// bioskey(2) returned shift state


void do_windows_events( void )
{
	unsigned long	events, eread;
	INPUT_RECORD	inrec;

	while ( GetNumberOfConsoleInputEvents( conin, &events ) && events > 0 )
	{
		if ( ReadConsoleInput( conin, &inrec, 1, &eread ) && eread == 1 )
		{
			switch( inrec.EventType )
			{
			case MOUSE_EVENT:
				//	process mouse events

				mbuf[ mip ].bstate = inrec.Event.MouseEvent.dwButtonState;
				mbuf[ mip ].x = inrec.Event.MouseEvent.dwMousePosition.X;
				mbuf[ mip ].y = inrec.Event.MouseEvent.dwMousePosition.Y;
				if ( ++ mip >= MAX_MSIZE ) mip = 0;
				break;

			case KEY_EVENT:
				kbuf[ kip ] = inrec.Event.KeyEvent.uChar.AsciiChar;							//	get the ASCII keycode
				shift_state = inrec.Event.KeyEvent.dwControlKeyState;						//	and the shift/control key states

				//	If the ASCII code is 0, then it's some sort of function or shift key

				if ( !kbuf[ kip ] )
				{
					kbuf[ kip ] = ( inrec.Event.KeyEvent.wVirtualScanCode & 0xFF ) << 8;	// get the function key code and format it like Borland used to (high byte is the code, low byte is zero

					//	This is a little different than Borlan's key handler.
					//	We'll get events when the "shift" keys are pressed
					//	that don't represent key presses, but instead
					//	represent key state changes.  Since the system keeps
					//	track of the shift keys states for us
					//	(in inrec.Event.KeyEvent.dwControlKeyState),
					//	we just need to catch these states and not return them to
					//	the app.

					switch ( kbuf[ kip ] )
					{
					case 0x3800:
					case 0x1D00:
					case 0x2A00:
					case 0x3600:
					case 0x3A00:
						goto no_save;
					}
				} // a function key was pressed

				if ( inrec.Event.KeyEvent.bKeyDown )
				{
					if ( inrec.Event.KeyEvent.dwControlKeyState & ( LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED ) )
					{
						//	If it's an ALT key, format it the way Borland does

						kbuf[ kip ] = inrec.Event.KeyEvent.wVirtualScanCode << 8;
						if ( ++ kip >= sizeof( kbuf ) / sizeof( kbuf[ 0 ] ) ) kip = 0;
					}
					else
						if ( ++ kip >= sizeof( kbuf ) / sizeof( kbuf[ 0 ] ) ) kip = 0;
				}
no_save: ;
				break;
			} // switch
		} // a pending event
	} // while there's unprocessed keyboard input
}


//	Here's the original Borland C keyboard function
//	cmd=2 --> return the shift key state
//	cmd=1 --> tests for pending keyboard key presses
//	cmd=0 --> retrieves keyboard input.  Blocks if no key has been pressed.
//	Function codes are formatted compatible with codes defined in keys.h.

int	bioskey( int cmd )
{
	static int		button_state = 0;
	int				kreturn;

	//	Start by checking for windows keyboard input events.

wait_for_keys:

	do_windows_events();

	switch( cmd )
	{
	case 0:
		//	The caller is asking for input.  If there is some in the buffer, return it, otherwise
		//	wait for some.

		if ( kip != kop )					// if keys have been pressed
		{
			kreturn = kbuf[ kop ];			// take the oldest key code
			if ( ++ kop >= sizeof( kbuf ) / sizeof( kbuf[ 0 ] ) ) kop = 0;	// update the output pointer
			return( kreturn );				// return the key code
		}
		goto wait_for_keys;					// else wait for some

	case 1:
		//	The caller is asking for keyboard status.  Return true if there's a key pending
		// if the input pointer <> the output pointer, there are key codes to return

		return( kip != kop );

	case 2:
		//	The caller is asking for the states of the shift/control keys.
		//	The format is defined in wincon.h ( example: LEFT_ALT_PRESSED ).

		return( shift_state );

	} // switch( cmd )
	return( 0 );
}


//	For a WIN32C program, input from the console includes both keyboard and mouse
//	events.  The two input types are no longer retrieved by seperate events but rather
//	both are obtained by ReadConsoleInput().  Mouse input must be enabled using
//	SetConsoleMode(), so keyboard only input can be obtained, but not mouse only input.
//	In the Borland model, getmousestat retrieves mouse events and bioskey keyboard.

//	Unlike the Borland version, coordinates are relative to 0, 0.

int gotoxy( int x, int y )
{
	COORD	loc = { (short) x, (short) y };

	return( SetConsoleCursorPosition( conout, loc ) );
}

//	Unlike the Borland version, coordinates are relative to 0, 0.

int wherex( void )
{
	CONSOLE_SCREEN_BUFFER_INFO	cur;

	GetConsoleScreenBufferInfo( conout, &cur );
	return( cur.dwCursorPosition.X );
}

//	Unlike the Borland version, coordinates are relative to 0, 0.

int wherey( void )
{
	CONSOLE_SCREEN_BUFFER_INFO	cur;

	GetConsoleScreenBufferInfo( conout, &cur );
	return( cur.dwCursorPosition.Y );
}

//	Here are three Borland equivalent routines
//	With coordinates relative to 1.

int gotoxyb( int x, int y )
{
	return( gotoxy( x - 1, y - 1 ) );
}

int wherexb( void )
{
	return( wherex() + 1 );
}

int whereyb( void )
{
	return( wherey() + 1 );
}


void textcolor( WORD color )
{
	SetConsoleTextAttribute( conout, color );
}


void clrscr( void )
{
	unsigned long	ch;
	COORD			zero = { 0, 0 };
	int				i;

	gotoxy( 0, 0 );
	i = FillConsoleOutputCharacter( conout, ' ', text.dwSize.X * text.dwSize.Y, zero, &ch );
	i = FillConsoleOutputAttribute( conout, FG_WHITE, text.dwSize.X * text.dwSize.Y, zero, &ch );
	gotoxy( 0, 0 );
}


void clreol( void )
{
	unsigned long	ch;
	COORD			zero = { (short) wherex(), (short) wherey() };

	FillConsoleOutputCharacter( conout, ' ', text.srWindow.Right - zero.X, zero, &ch );		//	text.dwSize.X,
	FillConsoleOutputAttribute( conout, FG_WHITE, text.srWindow.Right - zero.X, zero, &ch );	//	text.dwSize.X
	gotoxy( zero.X, zero.Y );
}


void clrn( int n )
{
	unsigned long	ch;
	COORD			zero = { (short) wherex(), (short) wherey() };

	FillConsoleOutputCharacter( conout, ' ', n, zero, &ch );
	FillConsoleOutputAttribute( conout, FG_WHITE, n, zero, &ch );
	gotoxy( zero.X, zero.Y );
}


void _setcursortype( int ctype )
{
	CONSOLE_CURSOR_INFO		cur;
	int						i;

	if ( ctype == _NORMALCURSOR )
	{
		cur.bVisible = TRUE;
		cur.dwSize = 20;
	}
	else
	{
		cur.bVisible = FALSE;
		cur.dwSize = 1;
	}

	i = SetConsoleCursorInfo( conout, &cur );
}


//	Replaces cursor.c

void cursor( BOOL cursoron )
{
	_setcursortype( cursoron );
}



//	Return the character at the cursor

unsigned char getchr( void )
{
	CONSOLE_SCREEN_BUFFER_INFO	binfo;
	CHAR_INFO					cinfo;
	SMALL_RECT					r;
	COORD						c = { 1, 1 }, o = { 0, 0 };

	GetConsoleScreenBufferInfo( conout, &binfo );
	r.Left = r.Right = binfo.dwCursorPosition.X;
	r.Right = r.Left + 1;
	r.Top = r.Bottom = binfo.dwCursorPosition.Y;
	r.Bottom = r.Top + 1;
	ReadConsoleOutput( conout, &cinfo, c, o, &r );
	return( cinfo.Char.AsciiChar );
}


//	Return the attributes at the cursor

WORD getatr( void )
{
	CONSOLE_SCREEN_BUFFER_INFO	binfo;
	CHAR_INFO					cinfo;
	SMALL_RECT					r;
	COORD						c = { 1, 1 }, o = { 0, 0 };

	GetConsoleScreenBufferInfo( conout, &binfo );
	r.Left = binfo.dwCursorPosition.X;
	r.Right = r.Left + 1;
	r.Top = binfo.dwCursorPosition.Y;
	r.Bottom = r.Top + 1;
	ReadConsoleOutput( conout, &cinfo, c, o, &r );
	return( cinfo.Attributes & 0xFF );
}


void hilight( char endcode, WORD color )
{
	int	x = wherex(), y = wherey();
	int	i;
	char	c;


	for ( i = x; i < text.dwSize.X && ( c = getchr() ) != endcode; i ++ )
	{
		setatr( color );
		gotoxy( i + 1, y );
	}

	gotoxy( x, y );
}


//	Here's a second form of hilight that has seperate foreground and background color inputs

void hilight( char tcode, WORD hicolor, WORD bgcolor )
{
	CONSOLE_SCREEN_BUFFER_INFO	csr;
	char						och;
	unsigned long				chr;
	
	while ( GetConsoleScreenBufferInfo( conout, &csr ) &&
			ReadConsoleOutputCharacterA( conout, &och, 1, csr.dwCursorPosition, &chr ) && och != tcode &&
			FillConsoleOutputAttribute( conout, hicolor | bgcolor, 1, csr.dwCursorPosition, &chr ) )
	{
		csr.dwCursorPosition.X ++;
		gotoxy( csr.dwCursorPosition.X, csr.dwCursorPosition.Y );
	}
}


//	And a 3d form that accepts coordinates

void hilight( COORD start, COORD end, WORD hicolor )
{
	int		x, y;
	int		x0 = wherex( ), y0 = wherey( );

	for ( y = start.Y; y <= end.Y; y ++ )
	{
		for ( x = start.X; x <= end.X; x ++ )
		{
			gotoxy( x, y );
			setatr( hicolor );
		}
	}
	gotoxy( x0, y0 );
}


//	Set the attributes of the character at the cursor

void setatr( WORD color )
{
	CHAR_INFO	cbuf;
	COORD		one = { 1, 1 }, zero = { 0, 0 };
	SMALL_RECT	pos;
	
	pos.Left = pos.Right = (short) wherex();
	pos.Top = pos.Bottom = (short) wherey();

	cbuf.Char.AsciiChar = getchr();
	cbuf.Attributes = color;

	WriteConsoleOutput( conout, &cbuf, one, zero, &pos );
}


//	Set the character at the cursor, doesn't change the current position's attributes

void setchr( CHAR chr )
{
	CHAR_INFO	cbuf;
	COORD		one = { 1, 1 }, zero = { 0, 0 };
	SMALL_RECT	pos;
	
	pos.Left = pos.Right = (short) wherex();
	pos.Top = pos.Bottom = (short) wherey();

	cbuf.Char.AsciiChar = chr;
	cbuf.Attributes = getatr();

	WriteConsoleOutput( conout, &cbuf, one, zero, &pos );
}


//	Get the oldest mouse event.  Mouse events must be enabled using:
//		SetConsoleMode( conin, ENABLE_MOUSE_INPUT );
//	10/17/11 returns last reported mouse state if no windows mouse events
//	are available.

unsigned long getmousestat( int *x, int *y )
{
	unsigned long			mstat;
	static unsigned long	lmstat = 0;
	static int				lx = 0, ly = 0;

	do_windows_events();					// process keyboard/mouse events

	if ( mip == mop )
	{
		//	There are no new mouse events, so return the last recorded event
		if ( x != NULL ) *x = lx;
		if ( y != NULL ) *y = ly;
		return( lmstat );
	}

	if ( x != NULL ) *x = mbuf[ mop ].x;
	lx = mbuf[ mop ].x;
	if ( y != NULL ) *y = mbuf[ mop ].y;
	ly = mbuf[ mop ].y;
	lmstat = mstat = mbuf[ mop ].bstate;

	if ( ++ mop >= MAX_MSIZE ) mop = 0;		// bump the output pointer
	return( mstat );
}


CHAR_INFO	*scrbuf = NULL;					// ptr to screen save buffer
int			bufsize = 0;					// it's allocated size
SMALL_RECT	abssbuf;						// screen area that was saved
COORD		oldcursor;

//	Save the whole screen.
//	Note we just save the visible portion of the screen, not the allocated size.

//	Returns true on success.

int save_screen( void )
{
	CONSOLE_SCREEN_BUFFER_INFO	csr;
	COORD						start;
	long						bsize;
	void						*p;

	if ( GetConsoleScreenBufferInfo( conout, &csr ) )
	{
		oldcursor.X = wherex();
		oldcursor.Y = wherey();
		bsize = ( csr.srWindow.Right - csr.srWindow.Left + 1 ) * ( csr.srWindow.Bottom - csr.srWindow.Top + 1 ) * sizeof( CHAR_INFO );
//		bsize = csr.dwSize.X * csr.dwSize.Y * sizeof( CHAR_INFO );

		if ( bufsize > 0 && scrbuf != NULL )
			if ( ( p = realloc( scrbuf, bsize ) ) == NULL )
			{
				free( scrbuf );
				goto outmem;
			}
			else
				scrbuf = (CHAR_INFO *) p;
		else
			scrbuf = (CHAR_INFO *) malloc( bsize );

		if ( scrbuf != NULL )
		{
			start.X = csr.srWindow.Left;
			start.Y = csr.srWindow.Top;
			//	Note: by leaving csr.srWindow.Bottom & Right unchanged, we're only saving the visible part
			//	so accordingly, we'll change dwSize to reflect the visible part of the window.
			csr.dwSize.X = csr.srWindow.Right - csr.srWindow.Left + 1;
			csr.dwSize.Y = csr.srWindow.Bottom - csr.srWindow.Top + 1;
			abssbuf.Left = csr.srWindow.Left;
			abssbuf.Right = csr.srWindow.Right;
			abssbuf.Top = csr.srWindow.Top;
			abssbuf.Bottom = csr.srWindow.Bottom;

			if ( ReadConsoleOutput( conout, scrbuf, csr.dwSize, start, &csr.srWindow ) )
			{
				bufsize = bsize;
				return( 1 );	// success
			}
			else
			{
//				DWORD	x = GetLastError();
				bufsize = 0;
				free( scrbuf );
				scrbuf = NULL;
			}
		}
		else
		{
outmem:
			printf( "Out of memory\n" );
			return( 0 );
		}
	}
	return( 0 );	// failed
}


//	Restore the saved screen, including color attributes.
//	Returns true on success.

int restore_screen( void )
{
	CONSOLE_SCREEN_BUFFER_INFO	csr;
	COORD						start = { 0, 0 };

	if ( scrbuf != NULL && bufsize && GetConsoleScreenBufferInfo( conout, &csr ) )
	{
		SetConsoleWindowInfo( conout, TRUE, &abssbuf );							// in case the app has scrolled the window, put it back like it was when we saved it!
		GetConsoleScreenBufferInfo( conout, &csr );								//	update for debugging
		//	Set the coordinates we're restoring

		csr.srWindow.Left = abssbuf.Left;
		csr.srWindow.Top = abssbuf.Top;
		csr.srWindow.Right = abssbuf.Right;
		csr.srWindow.Bottom = abssbuf.Bottom;
		csr.dwSize.X = ( csr.srWindow.Right - csr.srWindow.Left + 1 );			//	3/4/19 set correct size!
		csr.dwSize.Y = ( csr.srWindow.Bottom - csr.srWindow.Top + 1 );
		start.X = abssbuf.Left;
		start.Y = abssbuf.Top;

		//	Restore

		if ( WriteConsoleOutput( conout, scrbuf, csr.dwSize, start, &csr.srWindow ) )
		{
			//	Free our buffer

			free( scrbuf );
			scrbuf = NULL;
			bufsize = 0;
			gotoxy( oldcursor.X, oldcursor.Y );
			return( 1 );
		}
	}
	return( 0 );
}

//	Microsoft's strcmp doesn't work with IBM extended characters.

BOOL mystrchr( char *s, char c )
{
	for ( char *p = s; p != NULL && *p; p ++ )
	{
		if ( *p == c ) return( TRUE );
	}

	return( FALSE );
}

//	1/4/16 -- uses actual screen width

int hilight1( unsigned char *c, char foregroundcolor, char backgroundcolor )
{
  int			i, j, k;
  char			z;

  k = i = wherex();
  j = wherey();

  while ( ( k < text.dwSize.X ) && ( z = (char) getchr() ) != 0 && mystrchr( (char *) c, z ) == NULL )
  {
	  setatr( ( backgroundcolor << 4 ) + foregroundcolor );
      gotoxy( ( k = wherex() ) + 1, j );
  }
  gotoxy( i, j );
  return( k );
}


int hilight1( unsigned char *c, WORD color )
{
	int			i, j, k;
	char			z;

	k = i = wherex( );
	j = wherey( );

	while ( ( k < text.dwSize.X ) && ( z = (char) getchr( ) ) != 0 && mystrchr( (char *) c, z ) == NULL )
	{
		setatr( color );
		gotoxy( ( k = wherex( ) ) + 1, j );
	}
	gotoxy( i, j );
	return( k );
}

int	ask( char *msg )
{
	int		c = 0;

	printf( "%s", msg );

	do
	{
		;
	}
	while ( !_kbhit() || ( ( c = toupper( _getche() ) ) != 'Y' && c != 'N' && c != ESC ) );
	printf( "\n" );
	return( c == 'Y' );
}


char *input( const char *msg )
{
	static	char	buf[ 1000 ];
	char			*p = buf;
	int				c;

	printf( "%s", msg );

	c = *p = 0;

	do
	{
		if ( _kbhit() )
		{
			c = _getche();
			if ( isprint( c ) && p - buf < sizeof( buf ) - 1 )
			{
				*p ++ = c;
				*p = 0;
			}
			else
			{
				if ( c == BS && p > buf )
				{
					printf( " \x8" );
					-- p;
					*p = 0;
				}
			}
		}
	}
	while ( c != '\r' && c != ESC );
	printf( "\n" );
	return( ( c == '\r' ) ? buf : NULL );
}


//	Input a int between low and high
//	Returns true if value entered, false to cancel

bool getn( char *msg, int low, int high, int *value )
{
	char *p;

	do
	{
		if ( ( p = input( msg ) ) != NULL )										//	NULL if ESC, *p=0 if no entry
		{
			if ( *p && sscanf( p, "%d", value ) == 1 && *value >= low && *value <= high )
				return( true );													//	good entry
		}
	}
	while ( p != NULL );														//	while user hasn't hit ESC
	return( false );
}


//	12/3/2020 -- scroll a screen region

void scrollwindow( SMALL_RECT scroll )
{
	COORD		dest = { scroll.Left, scroll.Top };
	int			x = wherex( ), y = wherey( );
	CHAR_INFO	fill;
	BOOL		b;
	SMALL_RECT	clip = scroll;

	gotoxy( scroll.Left, scroll.Top );
	fill.Char.AsciiChar = (CHAR) ' ';
	fill.Attributes = FG_WHITE | BG_BLACK;
	scroll.Top ++;
	b = ScrollConsoleScreenBuffer( conout, &scroll, &clip, dest, &fill );
	gotoxy( x, y );
}


