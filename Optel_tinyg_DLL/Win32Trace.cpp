//	==========================================================================================
//	This little utility replaces my beloved printf() statement available for MFC based
//	applications.
// ---------------------------------------------------------------------
// Rev    Date      By     Description
// -----  --------  -----  ---------------------------------------------
//		  10/23/12  SRG	   Fixed return value bug from __vsnTRACE.
// -----  --------  -----  ---------------------------------------------
//		  10/23/09	SRG	   Original after discovering the
//						   OutputDebugString in WIN32 applications.
//	==========================================================================================

//#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <windows.h>



int TRACE( char *fmt, ... )
{
	va_list	argptr;										// Argument list pointer
	char	*str;										// ptr to (output) string buffer
	size_t	alloced;									// how big the output buffer (str) is
	int		cnt;										// Result of TRACE for return


	//	Allocate space for the output.  We can't really know how much space to
	//	allocate, so we'll guess 10 times the length of the formatting
	//	string.

	alloced = 10 * strlen( fmt ) + 1;					// guess how much to allocate

	if ( ( str = (char *) malloc( alloced ) ) == NULL )
		return( 0 );									// if we run out of memory

	va_start( argptr, fmt );							// Initialize va_ functions

	while ( ( cnt = _vsnprintf( str, alloced - 1, fmt, argptr ) ) < 0 )
	{
		//	We converted the output and it filled our buffer,
		//	double the buffer's size

		alloced *= 2;

		if ( ( str = (char *) realloc( str, alloced ) ) == NULL )
		{
			return( 0 );
		}
	}

	va_end( argptr );									// Close va_ functions
	OutputDebugStringA( str );							// send to debug window
	free( str );										// we're done!
	return( cnt );
}

