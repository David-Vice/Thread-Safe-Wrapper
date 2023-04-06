//	============================================================================
//	A function that searches src for the first occurrance of dst,
//	independently of letter-case.  Like strstr but ignores case.
//	Returns ptr into src of the first occurance of dst.
//	============================================================================
//	Date		 By		Description
//	--------	----	--------------------------------------------------------
//	3/10/22		srg		Updated for VS2022 & renamed for the current convention.
//	--------	----	--------------------------------------------------------
//	11/7/99		SRG		Original
//	============================================================================

#include <string.h>
#include <ctype.h>

#include "stristr.h"

char *stristr( char *src, char *dst )
{
	char *p1, *p2, *p;

	if ( src == NULL || dst == NULL ) return( NULL );								//	neither string is NULL

	if ( !*src ) return( ( *src == *dst ) ? src : NULL );							//	nil src matches only nil dst

	if ( !*dst ) return( src );													//	nil dst matches all non-nil

	p = src;
	p1 = src;
	p2 = dst;

	do
	{
		if ( toupper( *p1 ) == toupper( *p2 ) )
		{
			p1 ++;
			p2 ++;
			if ( !*p2 ) return( p );
			if ( !*p1 ) return( NULL );
		}
		else
		{
			p1 -= ( p2 - dst );
			p1 ++;
			p2 = dst;
			p = p1;
		}
	}
	while ( *p );
	return( NULL );
}
