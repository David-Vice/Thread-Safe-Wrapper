// ======================================================================================================
//	Optel TinyG DLL.  This DLL provides easy interface to any motor controller running the public domain
//	TinyG firmware which allows the embedded device to process standard gcode commands.  Specifically,
//	this code controls the TinyG V8 board available from Synthetos:
//	https://synthetos.myshopify.com/checkouts/c/04192daed30ec0dc41d55511b60cfc2e/information

//	See: https://github.com/synthetos/TinyG/wiki for technical details.
// ======================================================================================================
// Rev		Date		By		Description
// ------	--------- ------	-------------------------------------------------------------------------
// 1.0		9/19/15		SRG		Original
// ======================================================================================================

#include <stdio.h>
#include <Windows.h>
#include <math.h>
#include <time.h>
#include "critical.h"

#include "optel_tinyg_dll.h"
#include "optel_tinyg_api.h"
#include "win32comm.h"
#include "stristr.h"
CRITICAL_SECTION cmdio_critical_section;
BOOL create_ports() {
	int ports[100];
	int k = 0, j = 0, i = 0, l = 0;
	char* p, buf[500];
	DCB			prm = { sizeof(prm),		// sizeof(DCB)
						115200,				// current baud rate 
						1,					// binary mode, no EOF check
						0,					// enable parity checking
						0,					// CTS output flow control
						0,					// DSR output flow control
						0,					// DTR flow control type
						0,					// DSR sensitivity
						0,					// XOFF continues Tx
						0,					// XON/XOFF out flow control
						0,					// XON/XOFF in flow control
						0,					// enable error replacement 
						0,					// enable null stripping 
						0,					// RTS flow control 
						0,					// abort reads/writes on error
						0,					// reserved 
						0,					// not currently used 
						0,					// transmit XON threshold 
						0,					// transmit XOFF threshold 
						8,					// number of bits/byte, 4-8
						0,					// parity: 0-4=no,odd,even,mark,space 
						0,					// stop bits: 0,1,2 = 1, 1.5, 2 
						0,					// Tx and Rx XON character 
						0,					// Tx and Rx XOFF character 
						0,					// error replacement character 
						0,					// end of input character 
						0,					// received event character 
						0 };				// reserved; do not use 
	if ((i = findserialports(ports)) > 0)
	{
		k = 0;															//	count of FTDI ports

		for (j = 0; j < i; j++)
		{
			if ((p = (char*)getportinfo((char*)"manufacturer", ports[j])) != NULL)
			{
				if (stristr(p, (char*)"FTDI") == p)
				{
					k++;
					l = ports[j] - 1;
				}
			}
		}

		if (!k)
		{
		noport:
			printf("Sorry, I can't find a TinyG controller to connect with\n");
			LeaveCriticalSection(&cmdio_critical_section);
			return FALSE;
		}

		if (k > 1)
		{
			printf("There are multiple FTDI serial ports, please unplug the ones not connected to TinyG\n");
			LeaveCriticalSection(&cmdio_critical_section);
			return FALSE;
		}
	}
	else
		goto noport;

	int i;
	if ((i = portselect(l))) {
		LeaveCriticalSection(&cmdio_critical_section);
		return FALSE;
	}
	if (setcomprm(&prm))
	{
		printf("Can't configure COM-%d\n", l + 1);
		LeaveCriticalSection(&cmdio_critical_section);
		return FALSE;
	}

	if (!cmdio((char*)" \r", 10 * CLOCKS_PER_SEC, buf, sizeof(buf), (char*)"\xA"))
	{
		printf("Is TinyG running on COM-%d?\n", l + 1);
		LeaveCriticalSection(&cmdio_critical_section);
		return FALSE;
	}

	printf("Found TinyG on COM-%d: %s\n", l + 1, buf);

	//	check tinyg configuration

	if (strchr(buf, '{') != NULL)
	{
		//	JSON report mode is on, turn it off.
		if (!cmdio((char*)"$ej=0\r", CLOCKS_PER_SEC, buf, sizeof(buf), (char*)"\xA"))
		{
			printf("Can't disable JSON reporting\n");
			LeaveCriticalSection(&cmdio_critical_section);
			return FALSE;
		}
		else
			printf("JSON reports off (text reports on)\n");
	}
	printf("Hi from Optel_tinyg_DLL , V%.3lf, %02d/%02d/%04d\n", TG_VERSION, RELMO, RELDA, RELYR);
	LeaveCriticalSection(&cmdio_critical_section);
	return TRUE;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    static		HMODULE module = NULL;
	int			i, j, k, l;
	char		*p, buf[ 500 ];
	DCB			prm = { sizeof( prm ),		// sizeof(DCB)
						115200,				// current baud rate 
						1,					// binary mode, no EOF check
						0,					// enable parity checking
						0,					// CTS output flow control
						0,					// DSR output flow control
						0,					// DTR flow control type
						0,					// DSR sensitivity
						0,					// XOFF continues Tx
						0,					// XON/XOFF out flow control
						0,					// XON/XOFF in flow control
						0,					// enable error replacement 
						0,					// enable null stripping 
						0,					// RTS flow control 
						0,					// abort reads/writes on error
						0,					// reserved 
						0,					// not currently used 
						0,					// transmit XON threshold 
						0,					// transmit XOFF threshold 
						8,					// number of bits/byte, 4-8
						0,					// parity: 0-4=no,odd,even,mark,space 
						0,					// stop bits: 0,1,2 = 1, 1.5, 2 
						0,					// Tx and Rx XON character 
						0,					// Tx and Rx XOFF character 
						0,					// error replacement character 
						0,					// end of input character 
						0,					// received event character 
						0 };				// reserved; do not use 


    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		InitializeCriticalSection(&cmdio_critical_section);
		if ( module == NULL )
		{
			module = hModule;
			
			int ports[ 100 ];

			if ( ( i = findserialports( ports ) ) > 0 )
			{
				k = 0;															//	count of FTDI ports

				for ( j = 0; j < i; j ++ )
				{
					if ( ( p = (char *) getportinfo( (char *) "manufacturer", ports[ j ] ) ) != NULL )
					{
						if ( stristr( p, (char *) "FTDI" ) == p )
						{
							k ++;
							l = ports[ j ] - 1;
						}
					}
				}

				if ( !k )
				{
noport:
					printf( "Sorry, I can't find a TinyG controller to connect with\n" );
					return FALSE;
				}

				if ( k > 1 )
				{
					printf( "There are multiple FTDI serial ports, please unplug the ones not connected to TinyG\n" );
					return FALSE;
				}
			}
			else
				goto noport;

			int i;
			if ( ( i = portselect( l ) ) ) return FALSE;

			if ( setcomprm( &prm ) )
			{
				printf( "Can't configure COM-%d\n", l + 1 );
				return FALSE;
			}

			if ( !cmdio( (char *) " \r", 10 * CLOCKS_PER_SEC, buf, sizeof( buf ), (char *) "\xA" ) )
			{
				printf( "Is TinyG running on COM-%d?\n", l + 1 );
				return FALSE;
			}

			printf( "Found TinyG on COM-%d: %s\n", l + 1, buf );

			//	check tinyg configuration

			if ( strchr( buf, '{' ) != NULL )
			{
				//	JSON report mode is on, turn it off.
				if ( !cmdio( (char *) "$ej=0\r", CLOCKS_PER_SEC, buf, sizeof( buf ), (char *) "\xA" ) )
				{
					printf( "Can't disable JSON reporting\n" );
					return FALSE;
				}
				else
					printf( "JSON reports off (text reports on)\n" );
			}
			printf( "Hi from Optel_tinyg_DLL %d, V%.3lf, %02d/%02d/%04d\n", ul_reason_for_call, TG_VERSION, RELMO, RELDA, RELYR );
			return TRUE;
		}
		else
		{
			printf( "Optel_TinyG_DLL: Can't share COM port resources\n" );
			return FALSE;
		}
        break;

    case DLL_THREAD_ATTACH:
		return TRUE;
		break;

    case DLL_THREAD_DETACH:
		return TRUE;
		break;

    case DLL_PROCESS_DETACH:
        if ( hModule == module )
        {
            //  Close all operations & free all variables

			printf( "Bye from Optel_TinyG_DLL\n" );
			closeports( );
			return TRUE;
		}
//		else we are not the detach target
		DeleteCriticalSection(&cmdio_critical_section);
		break;
    }
	return FALSE;
}


double  tg_version( void )
{
    return( TG_VERSION );
}



const char *tg_mname[ MM ] =
{
	(const char *) "x",
	(const char *) "y",
	(const char *) "z",
	(const char *) "a",
};


//	Return 4 motor positions.

bool tg_getpos( double pos[ ] )
{
	char	buf[ 300 ];
	int		i = 0;
	int		retry;

	for ( retry = 0; retry < 3; retry ++ )
	{
		printf( "getpos(" );
		i = 0;

		if ( !cmdio( (char *) "?\r", CLOCKS_PER_SEC, buf, sizeof( buf ), (char *) "\xA" ) )
		{
			printf( "getpos: no reply\ngetpos(" );
			break;
		}

		//	X position:          0.000 mm
		//	Y position :         0.000 mm
		//	Z position :         0.000 mm
		//	A position :         0.000 deg
		//	Feed rate:           0.000 mm / min
		//	Velocity:            0.000 mm / min
		//	Units:            G21 - millimeter mode
		//	Coordinate system : G54 - coordinate system 1
		//	Distance mode : G90 - absolute distance mode
		//	Feed rate mode : G94 - units - per - minute mode( i.e.feedrate mode )
		//	Machine state : Stop
		//	tinyg [mm] ok >

		do
		{
			if ( i < 4 )
			{
				if ( tolower( *buf ) == *tg_mname[ i ] )
				{
					if ( sscanf( buf + 15, "%lf", pos + i ) != 1 )
					{
						//	convert error
						break;
					}
					else
					{
						printf( "%s%.3lf%s", tg_mname[ i ], *( pos + i ), ( i >= 3 ) ? ") OK\n" : "," );
						i ++;
					}
				}
				else
				{
					printf( "Wrong motor %s\n", buf );
					break;
				}
			}
			else
				if ( strstr( buf, "tinyg [mm" ) != NULL ) return( i >= 4 );
		}
		while ( cmdio( (char *) "", CLOCKS_PER_SEC, buf, sizeof( buf ), (char *) "\xA", false ) );
	}	//	retry

	return( false );
}


//	Home up to 4 motors.
//	True on success

bool tg_home( bool home[ MM ], int tosec )
{
	create_ports();
	char	buf[ 300 ];
	int		i = 0, j = 0;
	int		retry;
	double	pos[ MM ];

	for ( i = 0; i < 4; i ++ )													//	for each possible motor
	{
		if ( home[ i ] )														//	if we're to home it
		{
			for ( retry = 0; retry < 3; retry ++ )								//	try 3 times
			{
				printf( "home(%s) ", tg_mname[ i ] );

				//	Build the home command by listing the motors we've been asked to home.
				//printf(buf, "g28.2 %s0\r", tg_mname[i]);
				sprintf( buf, "g28.2 %s0\r", tg_mname[ i ] );

				//	Send the home command g29.2 axis0

				while ( cmdio( buf, CLOCKS_PER_SEC / 2, buf, sizeof( buf ), (char *) "\xA", false ) )
				{
					if ( strstr( buf, "stat:3" ) != NULL )
					{
						printf( "OK\n" );
						goto next_motor;
					}
					*buf = 0;													//	only send the command once
				}
			}	//	for retry
			closeports();
			return( false );													//	didn't home in 3 tries
		}	//	motor being homed
next_motor:;
	}	//	for each possible motor

	//	verify all homed motor positions are 0

	if ( tg_getpos( pos ) )
	{
		for ( i = 0; i < 4; i ++ )
		{
			if ( home[ i ] && fabs( pos[ i ] ) > 0.0001 )
			{
				printf( "%s isn't home\n", tg_mname[ i ] );
				closeports();
				return( false );
			}
		}
	}
	else {
		closeports();
		return(false);
	}
	closeports();
	return( true );
}


//	Move up to 4 motors to their specified positions
//	move is true if a motor is being moved.
//	values are in x, y, z, a order.
//	True on success.

bool tg_move( bool move[ MM ], double pos[ MM ], int tosec )
{
	create_ports();
	char	buf[ 300 ],															//	tinyg command
			rbuf[ 300 ],														//	receive
			sbuf[ 300 ],														//	completion status
			*p, *q;

	double	motors[ MM ];

	int		retry;

	for ( retry = 0; retry < 3; retry ++ )
	{
		if ( !tg_getpos( motors ) )
		{
			printf( "Can't retrieve motor positions\n" );
			break;
		}

		printf( "mmnove(" );

		strcpy( buf, "g0 " );
		p = buf + 3;
		q = sbuf;
		for ( int i = 0; i < 4; i ++ )
		{
			if ( move[ i ] && motors[ i ] != pos[ i ] )
			{
				printf( "%s%s%.3lf", ( p > buf + 3 ) ? "," : "", tg_mname[ i ], pos[ i ] );

				if ( p != buf + 3 )
				{
					strcat( p, " " );
					p ++;
				}
				p += sprintf( p, "%s%.3lf", tg_mname[ i ], pos[ i ] );
				if ( q != sbuf )
				{
					strcat( sbuf, "," );
					q ++;
				}
				q += sprintf( q, "pos%s:%.3lf", tg_mname[ i ], pos[ i ] );		//	build the expected status reply
			}
		}	// for each possible motor

		if ( p != buf + 3 )
		{
			printf( ") " );
			strcat( p, "\r" );

			if ( !cmdio( buf, tosec * CLOCKS_PER_SEC, rbuf, sizeof( rbuf ), (char *) "\xA" ) )
			{
				printf( "Move command failed\n" );
				break;
			}
			if ( strstr( rbuf, "err" ) != NULL )
			{
				printf( "Move command failed (%s)\n", rbuf );
				break;
			}
			printf( "OK\n" );

			while ( cmdio( (char *) "", tosec * CLOCKS_PER_SEC, rbuf, sizeof( rbuf ), (char *) "\xA", false ) )
			{
				//	Process each line as we receive it
				//	lines contain posm1:<pos>,posm2:<pos>,...vel:<vel>,stat:<status>
				//	The final line contains all moving motors with their positions equal to pos[].

//				printf( "%s\n", rbuf );

				if (strstr(rbuf, sbuf) != NULL) {
					closeports();
					return(true);
				}//	all axis match expected positions

			}
			printf( "error\n" );
		}	//	any motor is being moved
		else {
			closeports();
			return(true);
		}//	no motors moving, success
	}	//	retry
	closeports();
	return( false );															//	didn't get proper status
}


const char *rmin = "$%stn\r";													//	request min range
const char *rmax = "$%stm\r";													//	and max range

bool tg_getranges( tg_range_t *mrange )
{
	create_ports();
	int		i, retry;
	char	buf[ 300 ], rbuf[ 300 ];

	for ( retry = 0; retry < 3; retry ++ )
	{
		for ( i = 0; i < 4; i ++ )
		{
			sprintf( buf, rmin, tg_mname[ i ] );								//	form command for the min range

			while ( cmdio( buf, CLOCKS_PER_SEC, rbuf, sizeof( rbuf ), (char *) "\xA", false ) )
			{
//				printf( "%s\n", rbuf );
				//	[xtn] x travel minimum            0.000 mm
				//	tinyg [mm] ok>

				if ( *buf )
				{
					if ( sscanf( rbuf + 25, "%lf", &mrange[ i ].min ) == 1 )
					{
					}
					else
						break;
					*buf = 0;
				}
				else
					if ( strstr( rbuf, "ok>" ) == NULL ) break;
			}

			//	do the same for max range

			sprintf( buf, rmax, tg_mname[ i ] );								//	form command for the max range

			while ( cmdio( buf, CLOCKS_PER_SEC, rbuf, sizeof( rbuf ), (char *) "\xA", false ) )
			{
//				printf( "%s\n", rbuf );
				//	[xtm] x travel minimum            0.000 mm
				//	tinyg [mm] ok>

				if ( *buf )
				{
					if ( sscanf( rbuf + 25, "%lf", &mrange[ i ].max ) == 1 )
					{
					}
					else
						break;
					*buf = 0;
				}
				else
					if ( strstr( rbuf, "ok>" ) == NULL ) break;
			}
		}	//	for each motor

		if ( i >= 4 )
		{
			printf( "Motor Ranges:\n" );
			for ( i = 0; i < 4; i ++ )
				printf( "%s\t%.3lf\t%.3lf\n", tg_mname[ i ], mrange[ i ].min, mrange[ i ].max );
			closeports();
			return( true );
		}
	}	//	for retry
	closeports();
	return( false );
}

void tg_comm( char *msg )
{
	simplecomma( 0x1B, true, msg, true );
}

