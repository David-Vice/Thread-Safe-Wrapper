/*
	====================================================================
	This module acts as a replacement for the Borland conio module that
	provides access to screen I/O functions in the win32 environment.
	--------------------------------------------------------------------
	Rev    Date		By		Description
	-----  --------	-----	--------------------------------------------
		   12/3/20	SRG		Added scrollwindow().
	-----  --------	-----	--------------------------------------------
		   11/20/20 SRG		Added second init_con() that controls window
							and buffer sizes.
	-----  --------	-----	--------------------------------------------
		   9/11/15	SRG		Added conio keyboard functions _kbhit, getch.
	-----  --------	-----	--------------------------------------------
		   10/12/11 SRG		Added a 3-argument hilight from scan32.
	-----  --------	-----	--------------------------------------------
	       07/26/11	SRG		build from pieces of various win32 programs.
	====================================================================
*/


//	Screen color codes

#ifdef GRAY
#undef GRAY
#endif
#ifdef WHITE
#undef WHITE
#endif

#define FG_LIGHTRED ( FOREGROUND_INTENSITY | FOREGROUND_RED )
#define FG_LIGHTCYAN ( FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY )
#define FG_CYAN ( FOREGROUND_GREEN | FOREGROUND_BLUE )
#define FG_LIGHTGREEN ( FOREGROUND_INTENSITY | FOREGROUND_GREEN )
#define FG_GREEN ( FOREGROUND_GREEN )
#define FG_YELLOW ( FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY )
#define FG_BLACK ( 0 )
#define FG_GRAY ( FOREGROUND_INTENSITY )
#define FG_WHITE ( FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY )

#define BG_LIGHTRED ( BACKGROUND_INTENSITY | BACKGROUND_RED )
#define BG_LIGHTCYAN ( BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY )
#define BG_CYAN ( BACKGROUND_GREEN | BACKGROUND_BLUE )
#define BG_LIGHTGREEN ( BACKGROUND_INTENSITY | BACKGROUND_GREEN )
#define BG_GREEN ( BACKGROUND_GREEN )
#define BG_YELLOW ( BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY )
#define BG_BLACK ( FG_BLACK )
#define BG_GRAY ( BACKGROUND_INTENSITY )
#define BG_WHITE ( BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_INTENSITY )

#define	_NORMALCURSOR			1
#define _NOCURSOR				0


extern void								*conout;		// handle to stdout
extern void								*conin;			// & stdin
extern CONSOLE_SCREEN_BUFFER_INFO		text;			// screen dimensions


//	Call this first

void init_con( int width, int height );
void init_con( int wwidth, int wheight, int bwidth, int bheight );


//	For a WIN32C program, input from the console includes both keyboard and mouse
//	events.  The two input types are no longer retrieved by seperate events but rather
//	both are obtained by ReadConsoleInput().  Mouse input must be enabled using
//	SetConsoleMode(), so keyboard only input can be obtained, but not mouse only input.
//	In the Borland model, getmousestat retrieves mouse events and bioskey keyboard.

//	Unlike the Borland version, coordinates are relative to 0, 0.

int gotoxy( int x, int y );

//	Unlike the Borland version, coordinates are relative to 0, 0.

int wherex( void );

//	Unlike the Borland version, coordinates are relative to 0, 0.

int wherey( void );

//	Here are three Borland replacements

int gotoxyb( int x, int y );
int wherexb( void );
int whereyb( void );


void textcolor( WORD color );

void clrscr( void );

void clreol( void );
void clrn( int n );						//	clear n characters from the cursor
void _setcursortype( int ctype );
void cursor( BOOL cursoron );			// this replaces the cursor.c routine

unsigned char getchr( void );

//	Return the attributes at the cursor

WORD getatr( void );

void hilight( char endcode, WORD color );
void hilight( char tcode, WORD hicolor, WORD bgcolor );
void hilight( COORD start, COORD end, WORD hicolor );

void setatr( WORD color );


//	Set the character at the cursor, doesn't change the current position's attributes

void setchr( CHAR chr );


//	Retrieve mouse events.

unsigned long getmousestat( int *x, int *y );

//	Retrieve keyboard keys.

int	bioskey( int cmd );


//	Ask a question, input a response.
//	Returns NULL if ESC pressed, else a pointer to the entry.

char *input( const char *msg );

//	Save and restore the screen content

int save_screen( void );
int restore_screen( void );

//	Highlight the screen field starting at the cursor up to but not including any of the characters
//	in c.

int hilight1( unsigned char *c, char foregroundcolor, char backgroundcolor );
int hilight1( unsigned char *c, WORD color );


//	Ask a yes/no question.  Returns true on "y".

int	ask( char *msg );


bool getn( char *msg, int low, int high, int *value );


//	Scroll the window region scroll

void scrollwindow( SMALL_RECT scroll );

