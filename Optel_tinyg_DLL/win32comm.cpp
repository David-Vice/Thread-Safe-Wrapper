// ==============================================================================================================
//	CAUTION: ARRAYS ARE NO LONGER INDEXED BY THE COM PORT NUMBER!
// --------------------------------------------------------------------------------------------------------------
//	Provide easy to use serial communication functions for WIN32.
//	Note that WIN32 supports COM port values beyond 9.  I have yet to
//	see a PC that has one, but know they can exist.  There's no reason
//	this module can't be modified to handle these cases, but no
//	compelling reason as of today to do so.
// --------------------------------------------------------------------------------------------------------------
// Rev		Date		By		Description
// -----	--------	------	---------------------------------------------------------------------------------
//			5/3/2022	SRG		When trying to connect with an Arduino Nano, the command:
//								powershell -WindowStyle Normal -command get-wmiobject win32_serialport >x.txt
//								doesn't list COM5 (connected to the NANO) as an option, it only lists COM1.
//								But the mode command lists both, so findserialports is modified.
//								In addition to not being listed by powershell, the Arduino Nano would repetedly
//								disconnect (in the charin()) function, because portprams[] had not been
//								initialized by openport().  I added code to update portprams from the parameters
//								retrieved in openport.
// -----	--------	------	---------------------------------------------------------------------------------
//			7/9/2021	SRG		Added get-wmiobject port detection (from osmometer).
// -----	--------	------	---------------------------------------------------------------------------------
//			6/9/2020	SRG		cmdio (any flavor) doesn't send the terminating carriage return, it must be
//								included as part of the command.  several instances of clock() - st > 10 were
//								being fooled into believing the port was disconnected.  10 is now replaced with
//								PDTIMEOUT which is now 50.
// -----	--------	------	---------------------------------------------------------------------------------
//			3/24/2020	SRG		After never having any sort of problems opening a serial interface, I was unable
//								to open an Arduino (Feather M0) on the new home computer.  After a little web
//								searching I ran across a reference to prepending "\\?\" to the path to eliminate
//								the MAX_PATH length limit.  Although I was trying to open COM12, clearly I'm no
//								where near that limit.  Just the same, I changed my prefix from "\\.\" to "\\?\"
//								and COM12 opened.  The next day I tried opening COM10 (a serial adaptor) only to
//								have that fail due to an error in the prefix check.  Ports under 10 don't need
//								the prefix, those above 9 do.  The code wasn't prefixing COM10.  The open/close
//								and disconnect states of the code have gotten confused.  A port is opened (e.g.
//								openfile) when the caller attempts to access it and closed when it disconnects.
//								By adding trace statements, I see the port being closed multiple times without
//								being reopened when talking with an unconnected serial adapter.
//								outstates was a subset of portparams and so is eliminated.
// -----	--------	------	---------------------------------------------------------------------------------
//			3/1/19		SRG		Correction to RTS & DTR being switched false during normal polling.  Removed
//								conflicting variable pstate in pf32, pstate is now set in openport() to prevent
//								charin() detecting disconnected port and reinitializing it.  Setcomsig now uses
//								Set/GetCommState() to update RTS/DTR signals.
// -----	--------	------	---------------------------------------------------------------------------------
//			2/6/19		SRG		Added isconnected() to relay the connection state of a USB serial port back to
//								the caller.
// -----	--------	------	---------------------------------------------------------------------------------
//			12/1/18		SRG		Added cmdiof, for communication in a full-duplex system e.g. the device echos
//								back send characters.
//								For the RL_Timer, uploads are very slow because the PC's Sleep and clock()
//								functions can't accurately measure a millisecond, which is the delay needed between
//								outgoing characters to prevent overflowing the PIC's UART at 115,200 BAUD.  So,
//								we run the system full-duplex and wait for the echo-back of our outgoing
//								characters.
// -----	--------	------	---------------------------------------------------------------------------------
//			6/5/18		SRG		Modified charin to restore port parameters on reconnect.  OK, this is getting a
//								little twisted.  When a port whose baud rate is not 9600 is disconnected then
//								reconnects, it is reinitialized at 9600 BAUD and no longer communicates.  So,
//								pinit[] is introduced.  pinit[x] is true when setcomprm is called for port x.
//								openport will not update portprams[] when pinit[] is true, so the disconnected
//								port will be reinitialized at it current rate instead of the default.
// -----	--------	------	---------------------------------------------------------------------------------
//			4/20/18		SRG		Modified outcoms(str,n) to return count
//								of characters sent.
// -----	--------	------	---------------------------------------
//			1/9/2018	SRG		added getline().
//								The issue of the 12/15 correction is
//								still live.  It has created a new
//								problem.
// -----  --------  ------ --------------------------------------------
//		  12/15/17	SRG		Correction to conflict created when program
//							terminate calls closeports() from multiple
//							places.
// -----  --------  ------ --------------------------------------------
//		  5/17/17	SRG		Corrections from VS2012 code analysis.
// -----  --------  ------ --------------------------------------------
//		  1/19/17	SRG		Correction in charin() when readfile doesn't
//							read a character.  Corrected misuse of
//							openport - its argument is a com port #
//							minus 1, so selport is invalid!  Both
//							charin functions trap and recover from
//							virtual COM port disconnect/reconnect.
// -----  --------  ------ --------------------------------------------
//		  2/27/16	SRG		Added getcomports and cmdio with delims.
// -----  --------  ------ --------------------------------------------
//		  6/28/15	SRG		Added getport which returns the currently
//							selected port (e.g. COM port #).
// -----  --------  ------ --------------------------------------------
//		  4/7/15	SRG		Added getcomsig from named port.  Defined
//							RS232INSIGS, RS232OUTSIGS and RS232SIGS.
// -----  --------  ------ --------------------------------------------
//		  4/6/15	SRG		Added signal names array portsignames[].
// -----  --------  ------ --------------------------------------------
//		  9/25/14	SRG		Communication channel is flushed when
//							opened.
// -----  --------  ------ --------------------------------------------
//		  9/16/14	SRG		Fixed a problem caused by windows 7 update
//							that has changed the size of the COMMCONFIG
//							structure from 50 bytes to 52 causing
//							GetCommConfig() to fail.
// -----  --------  ------ --------------------------------------------
//		  7/21/14	SRG		Fixed bug in closeports() that didn't set
//							openedports to 0 after closing them all.
//							This in turn caused reopening ports to
//							fail, which is usually undetected by the
//							caller (since many callers don't bother to
//							check open results).
// -----  --------  ------ --------------------------------------------
//		  7/9/14	SRG		Fixed several bugs in openport.  portsnames
//							(and portname) were too short.
// -----  --------  ------ --------------------------------------------
//		  5/25/14	SRG	   NUMCOMPORT has changed usage: it is now the
//						   number of open com ports, not the highest
//						   numbered one.  Any legal com port for a
//						   given system is supported.
// -----  --------  ------ --------------------------------------------
//		  8/11/13	SRG	   Integrated & tested HAI block communication.
// -----  --------  ------ --------------------------------------------
//		  7/25/13	SRG	   Correction for incorrect block time out
//						   caused by calling getcomprm() with the wrong
//						   port selected within getchunk.
// -----  --------  ------ --------------------------------------------
//		  7/19/13	SRG	   Implemented timeout detection in block
//						   receive routines.
// -----  --------  ------ --------------------------------------------
//		  7/09/13	SRG	   Several openport() bugs repaired.
//						   Character I/O is still incompatible with
//						   with block I/O.  Conditional compiling
//						   determines I/O type.
// -----  --------  ------ --------------------------------------------
//		  6/17/13	SRG	   This code tested under windows 7 home
//						   premimum SP1 on a Toshiba Satellite L755D
//						   AMD A6-3420M APU with Radeon HD Graphics
//						   (1.5 GHz), 64-bit (although the program is
//						   32-bit.
//						   The main app allocates 5x5,000,000 buffers,
//						   transmits one of them, then simulteneously
//						   receives on 4 COM ports using two EasySync
//						   USB2-H-6002-M RS-422 adaptors with all four
//						   receivers wired to the TX pair.
//						   Rates above 5MBAUD didn't seem to work (I
//						   tried 8M and 10M).  
// -----  --------  ------ --------------------------------------------
//		  6/3/13	SRG	   COM ports above 10 are addressed with names
//						   of the form: \\.\COMx.  In a string literal,
//						   each '\' is represented as "\\", so a string
//						   for com10 is: "\\\\.\\COM10".  This from MSDN.
//
//						   added anyrx() & getcomprop & outblock.
//
//						   Correct a bug that returned wrong values if
//						   getbyte() was called without receive data
//						   pending.
// -----  --------  ------ --------------------------------------------
//		  4/12/13	SRG	   Known bug: If a USB serial port is opened,
//						   and the attached device renumerates,
//						   subsequent communication fails.  Forthermore,
//						   attempts to (re)open the device (when the
//						   device is closed then reopened) fail.  I
//						   haven't figured out a solution yet.
// -----  --------  ------ --------------------------------------------
//	J	  5/30/12	SRG	   Support for break and comm error detection.
//						   Errors are returned as a super-set of the
//						   old com?drv format.
//						   Correction to RTS/DTR control.  DTR was
//						   inoperative!
//						   Caution: setcomprm() also sets RTS/DTR as
//						   well as all the other comm parameters.
//						   getcomprm() is modified to update the
//						   RTS/DTR states.
// -----  --------  ------ --------------------------------------------
//	I	  3/06/12	SRG	   Corrected a major bug that prevents the use
//						   of multiple ports in an application.
//						   openport wasn't updating selport if the port
//						   was already opened, so an app couldn't flip
//						   between ports.
// -----  --------  ------ --------------------------------------------
//	H	  3/5/12	SRG	   Simplecomm revised to show non-printable
//						   stuff as TTY codes (like NUL, STX, etc.),
//						   termination code can be any function/alt key
//						   code as defined in keys.h.
// -----  --------  ------ --------------------------------------------
//	G	  7/25/11	SRG	   getcommsig() is more compatible with the old
//						   DOS version in that the output signal states
//						   DTR & RTS are now also returned.
//						   This is accomplished by doing a bit merge of
//						   the "remembered" output bit states (which
//						   initialized to their *assumed* values on
//						   port open) with the retrieved input
//						   bit states.  So, if the op-system sets up
//						   COM ports differently in future OS versions
//						   these could be wrong.  But, their will
//						   correct themselves once the output signal
//						   states are set (by the caller).
//						   DTR, RTS are outputs, CTS, DSR, RI & RLSD
//						   are inputs.
//						   The sendbreak() function, available in the
//						   original DOS version is now implemented.
// -----  --------  ------ --------------------------------------------
//  F     6/27/11	SRG	   simplecomm modified so F1 toggles RTS.
// -----  --------  ------ --------------------------------------------
//	E	  05/18/11	SRG	   simplecomm is modified so termcode is not
//						   sent to the serial port.
// -----  --------  ------ --------------------------------------------
//	D	  12/08/10  SRG	   closeports no longer static because it must
//						   be explicitly called if a console control
//						   handler is assigned by calling
//						   SetConsoleCtrlHandler() in main().
// -----  --------  ------ --------------------------------------------
//	C	  3/16/10   SRG	   The function named getcomstat() that is
//						   defined in PCCOMM.C but missing from this
//						   module can be mapped to getcomsig(), but
//						   is more than likely an error (in the calling
//						   code) since getcomstat only returns the
//						   output bit states of LOOP, RTS & DTR (PC
//						   outputs).

//						   Bug in charin and setcomsig corrected.
// -----  --------  ------ --------------------------------------------
//	 B	  01/30/10	SRG	   Fixed a glitch that didn't set the com port
//						   timeout if the port opened and
//						   closeportsflag was already true!
// -----  --------  ------ --------------------------------------------
//	 A	  01/23/10	SRG	   Added code in openport to detect non RS-232
//						   devices.  When such a device is opened, and
//						   charin() is subsequently called, the
//						   ReadFile() call hangs.  Openport() now fails
//						   unless the opened device is an RS-232 port.
//						   The readahead variable is now an ARRAY since
//						   each port has a read ahead buffer.  Duh!
// -----  --------  ------ --------------------------------------------
//        02/04/09  SRG    Converted from PCCOMM.C.
// ====================================================================

//#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <winNT.h>
#include <stdio.h>
#include "time.h"
#include <conio.h>

#include "win32comm.h"
#include "keys.h"
#include "stristr.h"
#include "Win32Trace.h"
#include "crtitical.h"



//	As of 5/25/14 the indexes are now sequentially allocated from
//	ports, portinit, readahead and outstates.  Port names are built
//	as needed

		int					selport = -1;										// which port (index) is active -- default to none (for getport())
static	volatile int		openedports = 0;									// # of registered COM ports (the ports may not be open if they have been disconnected)
static	int					closeportsflag = 0;									// to tell us we're registered closeports
static	DWORD				error_mask = EV_BREAK | EV_ERR;						// the communication errors we handle
static	DWORD				rx_error_mask = CE_BREAK | CE_FRAME | CE_OVERRUN | CE_RXOVER | CE_RXPARITY;
static	volatile bool		closing = false;


DCB		defaultsettings =
{
	sizeof( defaultsettings ),													// sizeof(DCB)
	9600,																		// current baud rate 
	1,																			// binary mode, no EOF check
	0,																			// enable parity checking
	0,																			// CTS output flow control
	0,																			// DSR output flow control
	0,																			// DTR flow control type
	0,																			// DSR sensitivity
	0,																			// XOFF continues Tx
	0,																			// XON/XOFF out flow control
	0,																			// XON/XOFF in flow control
	0,																			// enable error replacement 
	0,																			// enable null stripping 
	0,																			// RTS flow control 
	1,		// 6/30/21															// abort reads/writes on error
	0,																			// reserved 
	0,																			// not currently used 
	0,																			// transmit XON threshold 
	0,																			// transmit XOFF threshold 
	8,																			// number of bits/byte, 4-8
	0,																			// parity: 0-4=no,odd,even,mark,space 
	0,																			// stop bits: 0,1,2 = 1, 1.5, 2 
	0,																			// Tx and Rx XON character 
	0,																			// Tx and Rx XOFF character 
	0,																			// error replacement character 
	0,																			// end of input character 
	0,																			// received event character 
	0
};																				// reserved; do not use 


//	Modem input signal names, in bit order from 0.

char *portsignames[ 8 ] =
{
	(char *) "DTR",																//	bit 0- output
	(char *) "RTS",																//		1- output
	(char *) "B2",
	(char *) "B3",
	(char *) "CTS",																//		4
	(char *) "DSR",
	(char *) "RI",
	(char *) "CD"
};


//	Port names are COM1 ... COM9, then \\.\COM10, \\.\COM11, ...
//	As a literal it's "\\\\.\\COM10" ...

char portnames[ NUMCOMPORT ][ 15 ] =											// names of the ports we can simultaneously open
{
	"", "", "", "", "", "", "", "", "", ""
};

volatile HANDLE portinit[ NUMCOMPORT ] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };	// COM port handles
int readahead[ NUMCOMPORT ] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };		// read ahead character (to implement charin)
int portnumbers[ NUMCOMPORT ] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };		// numbers of the opened ports
DCB portprams[ NUMCOMPORT ];													//	parameters for each port
bool pstate[ NUMCOMPORT ] = { false, false, false, false, false, false, false, false, false, false };	//	port disconnect states
bool pinit[ NUMCOMPORT ] = { false, false, false, false, false, false, false, false, false, false };	//	port parameters have been changed

#ifdef	BLOCKIO
OVERLAPPED rolap[ NUMCOMPORT ];																			// these are used by charin and getbyte

typedef void (CALLBACK *cbptr)( DWORD error, DWORD transfered, LPOVERLAPPED olap );
cbptr	xyz[];										// callback pointer for each port
OVERLAPPED xolap[ NUMCOMPORT ];																			// these by outchunk
static clock_t timeout[ NUMCOMPORT ];																	// how long, in clock() units, to wait for the current block
volatile static clock_t ctimeout[ NUMCOMPORT ];																	// clock() value when the port operation has timed out
static DCB psetting[ NUMCOMPORT ];																		// individual port settings

typedef struct
{
	OVERLAPPED		olap;																				// ptr to this port's overlapped structures
	unsigned char	*buf;																				// ptr to caller's buffer start location
	ULONG			bufsiz;																				// how many there are
}	enh_olap_t;


typedef struct
{
	enh_olap_t		*eolap;																				// an array of these one for each chunk
	ULONG			numeolap;																			// # allocated
	ULONG			chunksrecv;																			// how many have been read so far
	ULONG			chunksreturned;																		// next chunk # to report as ready to F.G.
}	chunk_rec_t;


volatile chunk_rec_t	colap[ NUMCOMPORT ];															// for chunk read operations
#endif


//	All diagnostics 6/11/13

#undef	RS232_DIAGS

#ifdef	RS232_DIAGS

unsigned long	portpoll[ NUMCOMPORT ];
unsigned long	rxport[ NUMCOMPORT ];
clock_t			rxt[ NUMCOMPORT ][ 10000 ];
clock_t			*rxtp[ NUMCOMPORT ] =
{
	&rxt[ 0 ][ 0 ],	&rxt[ 1 ][ 0 ],	&rxt[ 2 ][ 0 ],	&rxt[ 3 ][ 0 ],
	&rxt[ 4 ][ 0 ],	&rxt[ 5 ][ 0 ], &rxt[ 6 ][ 0 ],	&rxt[ 7 ][ 0 ],
	&rxt[ 8 ][ 0 ],	&rxt[ 9 ][ 0 ]
};
#endif


// Close all open com ports.

//	12/08/10 -- This function must be explicitly called if a windows control handler
//				is assigned by calling SetConsoleCtrlHandler() (because it explicitly
//				disables the registered atexit() functions).  If a console control
//				handler isn't established, then closeports need not be explicitly called
//				because we've registered closeports() as an exit function.


void closeports( void )
{
	if ( closing ) return;
	closing = true;

	for ( int i = 0; i < openedports; i ++ )
	{
		if ( portinit[ i ] != NULL )
		{
			// TODO: terminate any io operations
			CloseHandle( portinit[ i ] );
			portinit[ i ] = NULL;												// 7/9/13 signal the port is closed
			readahead[ i ] = -1;
			portnumbers[ i ] = -1;
			pinit[ i ] = false;
#ifdef BLOCKIO
			if ( colap[ i ].eolap != NULL ) delete colap[ i ].eolap;
			colap[ i ].eolap = NULL;
			colap[ i ].numeolap = 0;
			colap[ i ].chunksrecv = 0;
			colap[ i ].chunksreturned = 0;
#endif
		}
	}
	openedports = 0;						// 7/21/14 no ports are opened!
	closing = false;
}


int closeport( int port )
{
	int		i;

	if ( port < 0 || port > MAXCOMPORTNUMBER ) return( ERROR_BAD_PORT );
	if ( closing ) return( BUSY );

	for ( i = 0; i < openedports && portnumbers[ i ] != port + 1; i ++ ) ;		//	8/8/16 compare #s instead of strings

	if ( i >= NUMCOMPORT ) return( ERROR_TOO_MANY );

	if ( i < openedports )
	{
		closing = true;

		//	Close port i, if it's open

		if ( portinit[ i ] != NULL )
		{
			//	Port I is open, close it

			CloseHandle( portinit[ i ] );
			portinit[ i ] = NULL;												// 7/9/13 signal the port is closed
			readahead[ i ] = -1;
			portnumbers[ i ] = -1;
			pinit[ i ] = false;
		}

		//	Now we must update openedports

		openedports = 0;

		for ( i = 0; i < NUMCOMPORT; i ++ )
		{
			if ( portinit[ i ] != NULL ) openedports ++;
		}

		closing = false;
		return( NOERROR );
	}

	return( ERROR_BAD_PORT );													//	this port isn't open, and we're not going to open it
}


// open a serial port for I/O.  0 <= port < NUMCOMPORT.  0 -> COM-1, 1 -> COM-2, ...
// Returns error value (from GetLastError()) on error
// else 0 and port ready for access.
// Does nothing if the port is already open

int openport( int port )
{
	DCB				params;

	//	9/16/14 -- pack some extra bytes around COMMCONFIG to
	//	allow for the Windows reported size (3d parameter of GetCommConfig()).
	//	And initialize the size field.

	struct
	{
		COMMCONFIG	cfg;
		char		extra[ 10 ];
	} cfg  = { sizeof( COMMCONFIG ) };

	DWORD			scfg = sizeof( cfg );
	char			portname[ 15 ];
	int				i;

	if ( port >= MAXCOMPORTNUMBER ) return( ERROR_BAD_PORT );

	//	Formulate the Windows device name

	if ( port < 10 - 1 )														//	3/25/2020 added -1 (COM10 is the transition)
	{
		sprintf( portname, "COM%d", port + 1 );
	}
	else
	{
		sprintf( portname, "\\\\?\\" "COM%d", port + 1 );						//	3/24/2020-- this is what works on win10!
	}

	//	Scan the opened port list to see if the requested port is already open

	for ( i = 0; i < openedports && portnumbers[ i ] != port + 1; i ++ ) ;		//	8/8/16 compare #s instead of string

	if ( i >= NUMCOMPORT ) return( ERROR_TOO_MANY );

	if ( i < openedports )
	{
		//	We found the port in the list.  Is it open, or was it closed due to an error?

		selport = i;															//	the port is in the list, select it

		if ( portinit[ i ] != NULL )
		{
			return( 0 );														//	the port is open
		}

		//	The requested port is in the list but was closed
	}

	strcpy( portnames[ i ], portname );

	// This port isn't yet open, do it now

	readahead[ i ] = -1;
//	outstates[ i ] = 0;															//	3/4/19 keep states

#ifdef RS232_DIAGS																// 6/11/13 diags
	portpoll[ i ] = rxport[ i ] = 0;
#endif

#ifdef BLOCKIO
	portinit[ i ] = CreateFile( portname, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED /* 0 */, NULL );
#else
	wchar_t wPortName[20];
	MultiByteToWideChar(CP_UTF8, 0, portname, -1, wPortName, 20);
	portinit[i] = CreateFile(wPortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
#endif
	if ( portinit[ i ] == INVALID_HANDLE_VALUE )
	{
		//	This happens if a USB COM port renumerates while the port is open,
		//	even if we close and reopen the port.  Or, if there's no port (of course).

		portinit[ i ] = NULL;													//	7/9/13 signal the port is closed
		return( GetLastError() );
	}

	//	9/16/2014 -- this call failed after windows 7 updates:
	//	KB2985461, 2982378, 2978092, 2977728, 2977629, 2973312,
	//	KB2972211, 2952664, 2889836, 890830, 2972216, and KB2894854.
	//	12/11/14-- this call hangs the work computer using the short
	//	USB-RS-232 cable.

	if ( !GetCommConfig( portinit[ i ], &cfg.cfg, &scfg ) )
	{
		DWORD err = GetLastError();												//	3/25/20

		if ( portinit[ i ] != NULL )
		{
			CloseHandle( portinit[ i ] );										// 7/9/13 close the port
			portinit[ i ] = NULL;												// & signal the port is closed
		}
		return( err );
	}

	if ( cfg.cfg.dwProviderSubType != PST_RS232 )
	{
		if ( portinit[ i ] != NULL )
		{
			CloseHandle( portinit[ i ] );										// 7/9/13 close the port
			portinit[ i ] = NULL;												// 7/9/13 signal the port is closed
		}
		return( ERROR_NOT_A_COM_PORT );
	}

	if ( !GetCommState( portinit[ i ], &params ) )
	{
		DWORD err = GetLastError();												//	3/25/20

		if ( portinit[ i ] != NULL )
		{
			CloseHandle( portinit[ i ] );										// 7/9/13 close the port
			portinit[ i ] = NULL;												// 7/9/13 signal the port is closed
		}
		return( err );
	}

	// As a default, we'll setup for 9600, N, 8, 1, all handshake off.

	if ( !pinit[ i ] )
	{
		//	If the port hasn't been initialized, set default parameters

		memcpy( &params, &defaultsettings, sizeof( params ) );					//	6/5/18 -- use default settings

		if ( !SetCommState( portinit[ i ], &params ) )
		{
			DWORD err = GetLastError();											//	3/25/2020
			if ( portinit[ i ] != NULL )
			{
				CloseHandle( portinit[ i ] );									// 7/9/13 close the port
				portinit[ i ] = NULL;											// 7/9/13 signal the port is closed
			}
			return( err );
		}
	}
	else
	{
		//	The port had been open and has reconnected, so reinitialize it using
		//	the last set parameters.

		if ( !SetCommState( portinit[ i ], &portprams[ i ] ) )
		{
			DWORD	err = GetLastError();										//	3/25/20

			if ( portinit[ i ] != NULL )
			{
				CloseHandle( portinit[ i ] );									// 7/9/13 close the port
				portinit[ i ] = NULL;											// 7/9/13 signal the port is closed
			}
			return( err );
		}
	}

#ifdef BLOCKIO
	memcpy( &psetting[ i ], &params, sizeof( psetting[ i ] ) );
#endif

	// The port's open and ready for use.  Register closeports for execution on
	// exit, unless we've already done so.

	portnumbers[ i ] = port + 1;												//	remember which COM port number this is.

	if ( !closeportsflag )
	{
		atexit( closeports );													// setup to auto close the ports on exit
		closeportsflag = 1;														// and signal we've done it
	}

	// Set the port timeout values -- no timeouts, reads return immediately

	COMMTIMEOUTS timeouts;

	if ( !GetCommTimeouts( portinit[ i ], &timeouts ) )
	{
		DWORD	err = GetLastError();											//	3/25/20

		if ( portinit[ i ] != NULL )
		{
			CloseHandle( portinit[ i ] );										// 7/9/13 close the port
			portinit[ i ] = NULL;												// 7/9/13 signal the port is closed
		}
		return( err );
	}

#ifndef	BLOCKIO
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 10;		//	2/6/19 was 0
	timeouts.WriteTotalTimeoutMultiplier = 10;		//	same here
#else
	//	Blockio time-outs: these time-outs are not compatible with charin()/getbyte()
	timeouts.ReadIntervalTimeout = 100;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.ReadTotalTimeoutMultiplier = 1;
	timeouts.WriteTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
#endif

	if ( !SetCommTimeouts( portinit[ i ], &timeouts ) )
	{
		DWORD	err = GetLastError();											//	3/25/20

		if ( portinit[ i ] != NULL )
		{
			CloseHandle( portinit[ i ] );										// 7/9/13 close the port
			portinit[ i ] = NULL;												// & signal the port is closed
		}
		return( err );
	}

	//	Enable error & break detection
/*	is this preventing us from detecting comm errors?
	if ( !SetCommMask( portinit[ i ], error_mask ) )
	{
		DWORD	err = GetLastError();											//	3/25/20

		if ( portinit[ i ] )
		{
			CloseHandle( portinit[ i ] );										// 7/9/13 close the port
			portinit[ i ] = NULL;												// & signal the port is closed
		}
		return( err );
	}
*/
	FlushFileBuffers( portinit[ i ] );											//	clear any pending data

/*
	//	7/6/21 -- will this fix the Arduino mega2560 r3 garbage character problem?
	//	This disabled the serial driver from putting modem signal changes in the serial data stream.

	UCHAR	lpInBuffer[ 1 ] = { 0 };
	DWORD	lpBytesReturned = 0;

	if (
			DeviceIoControl(
								portinit[ i ],					// handle to device
								IOCTL_SERIAL_LSRMST_INSERT,		// dwIoControlCode
								(LPVOID) lpInBuffer,			// input buffer 
								(DWORD) sizeof( UCHAR ),		// size of input buffer 
								NULL,							// lpOutBuffer
								0,								// nOutBufferSize
								(LPDWORD) lpBytesReturned,		// number of bytes returned
								(LPOVERLAPPED) NULL				// OVERLAPPED structure
							)
		)
		TRACE( "DeviceIOControl Success %d\n", lpBytesReturned );
	else
	{
		TRACE( "DeviceIOControl failed %d\n", lpBytesReturned );
		TRACE( "Error %d (%s)\n", GetLastError( ), strerror( GetLastError( ) ) );
	}
*/
	selport = i;																// remember which one's active
	memcpy( &portprams[ selport ], &params, sizeof( portprams[ selport ] ) );	//	5/3/2022 -- save retrieved port parameters
	if ( i + 1 > openedports ) openedports = i + 1;
	return( 0 );
}


// pick the specified port for I/O
//	Port is 0..n-1, 0-> COM1.
//	returns non-zero error code.

int portselect( int port )
{
	return( openport( port ) );
}


// pick the 'next' port

int otherport( void )
{
	return( openport( portnumbers[ ( ( selport + 1 ) % openedports ) ] - 1 ) );
}


//	Return the currently selected COM port # or -1

int getport( void )
{
	if ( selport >= 0 )
		return( portnumbers[ selport ] );
	else
		return( -1 );
}


//	True if selected port remains open

BOOL isconnected( void )
{
	if ( selport < 0 || selport >= NUMCOMPORT ) return( ERROR_BAD_PORT );
	return( pstate[ selport ] );
}


BOOL isconnected( int port )
{
	int		i;

	if ( port < 0 || port > MAXCOMPORTNUMBER ) return( ERROR_BAD_PORT );

	for ( i = 0; i < openedports && portnumbers[ i ] != port + 1; i ++ ) ;		//	8/8/16 compare #s instead of strings

	if ( i >= NUMCOMPORT ) return( ERROR_TOO_MANY );

	if ( i < openedports )
	{
		return( pstate[ i ] );
	}

	return( ERROR_BAD_PORT );													//	this port isn't open, and we're not going to open it
}



// return # characters pending or -1 if the port has disappeared.

// In the win32 environment, we can't know how many characters are in the
// buffer, or even if there are any without trying to read them, so we set
// the timeout value to zero, and try reading.  If the function returns
// no data, then we assume there's none received.  If we get a character, we
// save it in readahead[] since the caller is just asking if data is available,
// not for the value.  Getbyte(), checks readahead for pre-read data before
// calling the input function.

//	1/29/2017 adapts to PC speed so port disconnect is checked about 1/sec

//	3/26/2020 uses clock to pace port disconnect checking

#pragma warning(disable:4390)													//	disable warning for empty statement

int charin( void )
{
	unsigned char		c;														// 3/16/10 -- this must be unsigned else readahead[]=c can evaluate negative
	unsigned long		read;
	DWORD				errors;
	COMSTAT				cs;
	int					i;
	DCB					cfg;
//	static int			dtimer = 0;
//	static int			dtmax = 100000;											//	dtimer limit (initial value)
//	static time_t		ltime = clock(), dt, st;								//	time when dtimer hit dtmax
	static time_t		lastcheck = clock(), st;								//	timer for checking port disconnect & misc
//	static bool			hitlimit = false;										//	time has hit dtmax

	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 )
		goto no_port;															//	Port isn't opened and trying to do so fails
	else
	{
		//	Port is opened or successfully opened just now

		if ( !pstate[ selport ] )
		{
			pstate[ selport ] = true;
			portprams[ selport ].fAbortOnError = 1;
			setcomprm( &portprams[ selport ] );									//	reset parameters
			TRACE( (char *) "Port config\n" );
		}
	}

	if ( !portinit[ selport ] ) return( 0 );									//	this is a caller error!?!
	if ( readahead[ selport ] >= 0 ) return( 1 );								// return the character previously read

/*	use the system's time keeping ability, no timing loops!!
	if ( ++ dtimer > dtmax )
	{
		if ( !hitlimit )
		{
			hitlimit = true;
			ltime = clock();
		}
		else
		{
			//	We've hit the limit before so clock()-ltime is valid.  Adjust dtmax to make
			//	the value 990 to 1100 ticks.

			dt = clock() - ltime;												//	time since last dtmax
			ltime = clock();

			if ( dt < 990 )
			{
//				TRACE( "measured %d, ", dt );
//				TRACE( "dtmax too short %ld\n", dtmax );
				dtmax += 1000;
			}
			else
			{
				if ( dt > 1100 )
				{
//					TRACE( "measured %d, ", dt );
//					TRACE( "dtmax too long %ld\n", dtmax );
					dtmax -= 1000;
				}
				else
				{
					//	right on the money, leave dtmax alone
//					TRACE( "measured %d, ", dt );
//					TRACE( "Perfect %ld\n", dtmax );
				}
			}
		}

		dtimer = 0;
*/
	if ( clock() - lastcheck > 200 )											//	check 5 times/sec
	{
		lastcheck = clock();
		st = clock();
		if ( !GetCommState( portinit[ selport ], &cfg ) || clock() - st > PDTIMEOUT )
			goto no_port;
		st = clock();
		if ( !SetCommState( portinit[ selport ], &portprams[ selport ] ) || clock() - st > PDTIMEOUT )	//	put them back -- this generates an error if the port has been disconnected
			goto no_port;
	}

	// there is no character in the read ahead buffer, so we'll try
	// reading one from the port.  If we read one, we'll put it there then
	// signal the caller data is available.

	if ( ( i = ReadFile( portinit[ selport ], &c, 1, &read, NULL ) ) != 0 )
	{
		if ( read )
		{
#ifdef	RS232_DIAGS
			*rxtp[ selport ] = clock();
			( rxtp[ selport ] ) ++;
#endif
			if ( readahead[ selport ] < 0 )
			{
				readahead[ selport ] = c;
//				if ( c > 127 )
//					TRACE( "[%02X]", c );
//				else
//					TRACE( "%c", c );
			}
			else
			{
				TRACE( (char *) "readahead lost (COM-%d)\n", portnumbers[ selport ] );
				readahead[ selport ] = c;
			}

			DWORD	ms;

			if ( GetCommModemStatus( portinit[ selport ], &ms ) )
				if ( ms )
					;
			//TRACE( (char *) "State: %02X\n", ms & 0xFF );

			if ( ClearCommError( portinit[ selport ], &errors, &cs ) )
			{
				if ( errors ) TRACE( (char *) "COMM ERR %0X\n", errors );
				errors &= rx_error_mask;
				readahead[ selport ] |= errors << 8;
			}
			else
				TRACE( (char *) "Clear comm error failed\n" );

			return( 1 );
		}
	}
	else
	{
		//	1/19/17 ReadFile has failed.  Probably because the serial port is disconnected.
		//	Close the port and signal it's closed.

no_port:
		TRACE( (char *) "no port\n" );
		if ( pstate[ selport ] )
		{
			if ( ClearCommError( portinit[ selport ], &errors, &cs ) && errors & CE_FRAME )
			{
				TRACE( (char *) "frame\n" );
				readahead[ selport ] = CE_FRAME << 8;
				return( 0 );
			}
			else
				TRACE( (char *) "Clearcommerror failed\n" );

			TRACE( (char *) "Port closed @ %d\n", clock() );
			pstate[ selport ] = false;
			while ( closing ) ;
			if ( selport >= 0 && selport < openedports && portinit[ selport ] != NULL )
			{
				CloseHandle( portinit[ selport ] );
				portinit[ selport ] = NULL;
			}
		}
		return( -1 );
	}
	return( 0 );
}

#pragma warning(default:4390)


// return # characters pending

// In the win32 environment, we can't know how many characters are in the
// buffer, or even if there are any without trying to read them, so we set
// the timeout value to zero, and try reading.  If the function returns
// no data, then we assume there's none received.  If we get a character, we
// save it in readahead[] since the caller is just asking if data is available,
// not for the value.  Getbyte(), checks readahead for pre-read data before
// calling the input function.

//	Use timekeeping facilities to pace port disconnect checking.

int charin( int port )
{
	unsigned char		c;														// 3/16/10 -- this must be unsigned else readahead[]=c can evaluate negative
	int					i, oldport = selport;									//	save the currently selected port
	unsigned long		read;
	DWORD				errors;	//, status;
	DCB					cfg;
//	static int			dtimer = 0;
	static time_t		lastcheck = clock();

	if ( portinit[ port ] == NULL && portinit[ selport ] && openport( port ) != 0 )	//	the specified port isn't open and we can't open it
	{
		if ( pstate[ port ] )
		{
			pstate[ port ] = false;
			TRACE( (char *) "Port closed @ %d\n", clock() );
		}
		selport = oldport;														//	select the original port
		return( 0 );															//	the port wasn't opened, and our attempt to do so failed!
	}
	else
	{
		if ( !pstate[ port ] )
		{
			pstate[ port ] = true;
			TRACE( (char *) "Reconnect @ %d (%d BAUD)\n", clock(), portprams[ selport ].BaudRate );
		}
	}

	if ( !portinit[ selport ] ) return( 0 );									//	this is a caller error!?!

	if ( readahead[ selport ] >= 0 )
	{
		selport = oldport;
		return( 1 );															// return the character previously read
	}

//	if ( ++ dtimer > 0 )
	if ( clock() - lastcheck > 200 )											//	check for disconnect 5 times/sec
	{
		lastcheck = clock();
//		dtimer = 0;

		if ( !GetCommState( portinit[ selport ], &cfg ) )						//	status was not used!
			goto no_port;

		if ( !SetCommState( portinit[ selport ], &portprams[ selport ] ) )		//	put them back -- this generates an error if the port has been disconnected
			goto no_port;
	}

	// there is no character in the read ahead buffer, so we'll try
	// reading one from the port.  If we read one, we'll put it there then
	// signal the caller data is available.

	if ( ( i = ReadFile( portinit[ selport ], &c, 1, &read, NULL ) ) != 0 )
 	{
		if ( read )
		{
#ifdef	RS232_DIAGS
			*rxtp[ selport ] = clock();
			( rxtp[ selport ] ) ++;
#endif
			readahead[ port ] = c;
			if ( ClearCommError( portinit[ port ], &errors, NULL ) && ( errors &= rx_error_mask ) )
			{
				readahead[ port ] |= errors << 8;
			}
			return( 1 );
		}
	}
	else
	{
		//	1/19/17 ReadFile has failed.  Probably because the serial port is disconnected.
		//	Close the port and signal it's closed.

no_port:
		pstate[ selport ] = false;

		if ( portinit[ selport ] != NULL )
		{
			CloseHandle( portinit[ selport ] );
			TRACE( (char *) "Closed COM%d\n", portnumbers[ selport ] );
			portinit[ selport ] = NULL;
		}
		selport = oldport;
		return( -1 );
	}
	return( 0 );
}


// Get next character.  If called when no character is available, we wait for one (e.g. the function blocks).
// Communication error information is returned as a super-set of my original PC communication routines.
//	See: CE_BREAK, CE_FRAME, CE_OVERRUN, CE_RXOVER, CE_RXPARITY.  These bit values are in the *upper-byte*
//	of the returned value.

unsigned getbyte( void )
{
	unsigned		c;
	clock_t			now;

	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 ) return( 0xFF00 );		//	if the port hasn't been opened, and our attempt to do so fails

	if ( !portinit[ selport ] ) return( 0 );									//	this is a caller error!?!

	if ( ( c = readahead[ selport ] ) >= 0 )
	{
getahead:
		readahead[ selport ] = -1;
		return( c );
	}
	else
		TRACE( (char *) "no char available\n" );

	// there is no character in the read ahead buffer, wait for one

	now = clock( );
	while ( clock() < now + CLOCKS_PER_SEC / 2 && ( c = charin() ) == 0 ) ;		// wait for char or error

	//	6/11/13 Correction for calls when no data is yet received returning the wrong value
	//	Don't think this is an issue, since I never have written code that does this,
	//	but who knows...

	if ( c < 0 ) return( 0xFF00 );			// return port init error
	goto getahead;							// else return the read ahead character
}


// Several read routines with various features.
// readstr inputs a string of characters up to a carriage return, full buffer or receive timeout.
// getnt inputs a fixed length string or stops on timeout.
// getntx is the same as getnt except it *doesn't* clear the parity bit of the receive data.
// waitfor examines the incoming stream for a specific string and returns when it's found or there's a timeout.


// input string s (till cr) up to maxlen chars before timeout.
// Returns true if string (a carriage return) is received before timeout.

int readstr( long timeout, char *s, int maxlen )
{
	clock_t		start;

	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 )
	{
		printf( "Can't open port.\n" );
		return( 0 );
	}

	if ( !portinit[ selport ] ) return( 0 );									//	this is a caller error!?!

	start = clock();
	*s = 0;

	while ( maxlen > 0 && abs( clock() - start ) < timeout )
    {
		if ( charin() )
        {
			*s = (char) ( getbyte() & 0x7F );
			if ( *s == 0xD )
            {
				*s = 0;
				return( 1 );
            }
			
			if ( *s != 0xA )
            {
				if ( -- maxlen ) s ++;
            }

			*s = 0;

			start = clock();
        }
    }
	return( 0 );
}


// input maxlen chars into s before timeout.
// Returns true if maxlen chars are received before timeout.


BOOL getnt( long timeout, char *s, int maxlen )
{
	clock_t marktm;
	
	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 ) return( 0 );

	if ( !portinit[ selport ] ) return( 0 );									//	this is a caller error!?!

	marktm = clock();
	
	while ( maxlen && abs( clock() - marktm ) < timeout )
    {
		if ( charin() )
        {
			*s++ = (char) ( getbyte() & 0x7F );
			marktm = clock();
			maxlen --;
        }
    }
	*s = 0;
	return( maxlen == 0 );
}



// Input maxlen chars into s before timeout.  Parity bit ISN'T cleared.
// Returns true if maxlen chars are received before timeout.

BOOL getntx( long timeout, unsigned char *s, int maxlen )
{
	clock_t marktm;
	
	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 ) return( 0 );

	if ( !portinit[ selport ] ) return( 0 );									//	this is a caller error!?!

	marktm = clock();
	
	while ( maxlen && abs( clock() - marktm ) < timeout )
    {
		if ( charin() )
        {
			*s++ = (unsigned char) ( getbyte() & 0xFF );
			marktm = clock();
			maxlen --;
        }
    }
	*s = 0;
	return( maxlen == 0 );
}


// #define	COMDEBUG

// Send a command to the port, then input a response into recvbuf (up to maxlen characters incl/null terminator).
// The response ends with an ACK (0x06) or NAK (0x15).
// Returns true if an ACK terminated response is received within timeout milliseconds.
//	2/1/17 corrected overwriting buffer when nothing received.
//	7/13/21 ACK/NAK no longer returned in buffer.

char	lastcommand[ 1000 ] = "";

BOOL cmdio( char *cmd, long timeout, char *recvbuf, int maxlen )
{
	clock_t			marktm;
	unsigned char	c;
	int				i;

	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 ) return( FALSE );

	if ( !portinit[ selport ] ) return( FALSE );								//	this is a caller error!?!

#ifdef COMDEBUG
	strcpy_s( lastcommand, cmd );												//	save a diagnosic copy

	if ( charin( ) )
	{
		TRACE( (char *) "Unexpected RX: " );
		while ( charin( ) )
		{
			c = getbyte( );
			if ( c >= ' ' && c <= '~' )
				TRACE( (char *) "%c", c );
			else
				if ( c == '\r' || c == '\n' )
					TRACE( (char *) "\n" );
				else
					TRACE( (char *) "[%02X]", c );
		}
		TRACE( (char *) "\n" );
	}
#endif

	outcoms( cmd );																//	send the command

#ifdef COMDEBUG
	TRACE( (char *) "cmdio: " );
	for ( i = 0; cmd[ i ]; i ++ )
		if ( cmd[ i ] >= ' ' && cmd[ i ] <= '~' )
			TRACE( (char *) "%c", cmd[ i ] );
		else
			if ( cmd[ i ] == '\r' )
				TRACE( (char *) "\n" );
			else
				TRACE( (char *) "[%02X]", cmd[ i ] );
#endif

	marktm = clock();
	c = 0;
	clock_t	t;

	*recvbuf = 0;
	
	while ( maxlen > 1 && ( t = abs( clock() - marktm ) ) < timeout )
    {
		if ( ( i = charin( ) ) > 0 )
		{
			c = (unsigned char) ( getbyte( ) & 0xFF );
#ifdef COMDEBUG
			if ( c >= ' ' && c <= '~' )
				TRACE( (char *) "%c", cmd[ i ] );
			else
				if ( c == '\r' || c == '\n' )
					TRACE( (char *) "\n" );
				else
					TRACE( (char *) "[%02X]", cmd[ i ] );
#endif
			if ( c == 0x6 || c == 0x15 )
			{
//				*recvbuf ++ = c;												//	don't return ACK/NAK
				*recvbuf = 0;
#ifdef COMDEBUG
				TRACE( (char *) " %s\n", ( c == 6 ) ? "OK" : "NAK" );
#endif
				return( c == 0x6 );
			}
			else
			{
				*recvbuf ++ = c;
				*recvbuf = 0;
				maxlen --;
			}
			marktm = clock( );
		}
		else
		{
			if ( i < 0 )
			{
				TRACE( (char *) "Port disconnect\n" );
				return( FALSE );
			}
		}
    }
	if ( maxlen )
		TRACE( (char *) "cmdio timeout to \"%s\"\n", cmd );
	else
		TRACE( (char *) "RX buffer filled\n" );
	return( FALSE );
}


//	Save as cmdio with a callback function
//	2/1/17 corrected

BOOL cmdio( char *cmd, long timeout, char *recvbuf, int maxlen, void (*callback)( void ) )
{
	clock_t			marktm;
	unsigned char	c;
	int				i;

	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 ) return( FALSE );

	if ( !portinit[ selport ] ) return( 0 );									//	this is a caller error!?!

	outcoms( cmd );			// send the command

	marktm = clock();
	c = 0;

	*recvbuf = 0;
	
	while ( maxlen > 1 && abs( clock() - marktm ) < timeout )
    {
		if ( ( i = charin() ) > 0 )
        {
			c = (unsigned char) ( getbyte() & 0xFF );

			if ( c == 0x6 || c == 0x15 )
			{
				return( c == 0x6 );
			}
			else
			{
				*recvbuf ++ = c;
				*recvbuf = 0;
				maxlen --;
			}
			marktm = clock();
        }
		else
		{
			if ( i < 0 )
			{
				TRACE( ( char * ) "Port disconnect\n" );
				return( FALSE );
			}
			if ( callback != NULL ) callback( );
		}
    }
	TRACE( (char *) "cmdio timeout to %s\n", cmd );

	return( FALSE );
}


//	Same as cmdio with a list of input string delimiters
//	2/1/17 corrected

BOOL cmdio( char *cmd, long timeout, char *recvbuf, int maxlen, char *delims )
{
	clock_t			marktm;
	unsigned char	c;	//, retry = 2;
	int				i;

	if ( portinit[ selport ] == NULL && portinit[ selport ] && openport( portnumbers[ selport ] - 1 ) != 0 ) return( FALSE );

	if ( !portinit[ selport ] ) return( 0 );									//	this is a caller error!?!

	strcpy_s( lastcommand, cmd );												//	save a diagnosic copy

#ifdef COMDEBUG
	if ( charin( ) )
	{
		TRACE( (char *) "Unexpected RX: " );
		while ( charin( ) )
		{
			c = getbyte( );
			if ( c >= ' ' && c <= '~' )
				TRACE( (char *) "%c", c );
			else
				if ( c == '\r' || c == '\n' )
					TRACE( (char *) "\n" );
				else
					TRACE( (char *) "[%02X]", c );
		}
		TRACE( (char *) "\n" );
	}
#endif

	outcoms( cmd );																//	send the command all at once

#ifdef COMDEBUG
	TRACE( (char *) "cmdio: " );
	for ( i = 0; cmd[ i ]; i ++ )
		if ( cmd[ i ] >= ' ' && cmd[ i ] <= '~' )
			TRACE( (char *) "%c", cmd[ i ] );
		else
			if ( cmd[ i ] == '\r' )
				TRACE( (char *) "\n" );
			else
				TRACE( (char *) "[%02X]", cmd[ i ] );
#endif


	marktm = clock();
	c = 0;

	*recvbuf = 0;
	
	while ( maxlen > 1 && abs( clock() - marktm ) < timeout )
    {
		if ( ( i = charin( ) ) > 0 )
		{
			c = (unsigned char) ( getbyte( ) & 0xFF );

#ifdef COMDEBUG
			if ( c >= ' ' && c <= '~' )
				TRACE( (char *) "%c", c );
			else
				if ( c == '\n' )
					TRACE( (char *) "\n" );
				else
					TRACE( (char *) "[%02X]", c );
#endif
			if ( strchr( delims, c ) != NULL )
			{
				*recvbuf = 0;
#ifdef COMDEBUG
				TRACE( (char *) " %02X\n", c );
#endif
				return( TRUE );
			}
			else
			{
				*recvbuf ++ = c;
				*recvbuf = 0;
				maxlen --;
			}
			marktm = clock( );
		}
		else
			if ( i < 0 )
			{
				TRACE( ( char * ) "Port disconnect\n" );
				return( FALSE );
			}
    }
	TRACE( (char *) "cmdio timeout to %s\n", cmd );
	return( FALSE );
}


//	Same as cmdio with a list of input string delimiters & option to not clear input buffer
//	2/1/17 corrected

bool cmdio( char *cmd, long timeout, char *recvbuf, int maxlen, char *delims, bool clearbuf )
{
	EnterCriticalSection(&cmdio_critical_section);
	clock_t			marktm;
	unsigned char	c;	//, retry = 2;
	int				i;

	if (portinit[selport] == NULL && portinit[selport] && openport(portnumbers[selport] - 1) != 0)
	{
		LeaveCriticalSection(&cmdio_critical_section);
		return(FALSE);
	}

	if ( !portinit[ selport ] )									//	this is a caller error!?!
	{
		LeaveCriticalSection(&cmdio_critical_section);
		return(0);
	}

	strcpy_s( lastcommand, cmd );												//	save a diagnosic copy

#ifdef COMDEBUG
	if ( clearbuf && charin( ) )
	{
		TRACE( (char *) "Unexpected RX: " );
		while ( charin( ) )
		{
			c = getbyte( );
			if ( c >= ' ' && c <= '~' )
				TRACE( (char *) "%c", c );
			else
				if ( c == '\r' || c == '\n' )
					TRACE( (char *) "\n" );
				else
					TRACE( (char *) "[%02X]", c );
		}
		TRACE( (char *) "\n" );
	}
#endif

	outcoms( cmd );																//	send the command all at once

#ifdef COMDEBUG
	TRACE( (char *) "cmdio: " );
	for ( i = 0; cmd[ i ]; i ++ )
		if ( cmd[ i ] >= ' ' && cmd[ i ] <= '~' )
			TRACE( (char *) "%c", cmd[ i ] );
		else
			if ( cmd[ i ] == '\r' )
				TRACE( (char *) "\n" );
			else
				TRACE( (char *) "[%02X]", cmd[ i ] );
#endif


	marktm = clock( );
	c = 0;

	*recvbuf = 0;

	while ( maxlen > 1 && abs( clock( ) - marktm ) < timeout )
	{
		if ( ( i = charin( ) ) > 0 )
		{
			c = (unsigned char) ( getbyte( ) & 0xFF );

#ifdef COMDEBUG
			if ( c >= ' ' && c <= '~' )
				TRACE( (char *) "%c", c );
			else
				if ( c == '\n' )
					TRACE( (char *) "\n" );
				else
					TRACE( (char *) "[%02X]", c );
#endif
			if ( strchr( delims, c ) != NULL )
			{
				*recvbuf = 0;
#ifdef COMDEBUG
//				TRACE( (char *) " %02X\n", c );
#endif
				LeaveCriticalSection(&cmdio_critical_section);

				return( TRUE );
			}
			else
			{
				*recvbuf ++ = c;
				*recvbuf = 0;
				maxlen --;
			}
			marktm = clock( );
		}
		else
			if ( i < 0 )
			{
				TRACE( (char *) "Port disconnect\n" );
				LeaveCriticalSection(&cmdio_critical_section);
				return( FALSE );
			}
	}
	TRACE( (char *) "cmdio timeout to %s\n", cmd );
	LeaveCriticalSection(&cmdio_critical_section);
	return( FALSE );
}


BOOL cmdio( char *cmd, long timeout, char *recvbuf, int maxlen, int pacing )
{
	clock_t			marktm;
	unsigned char	c;
	int				i;

	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 ) return( FALSE );

	if ( !portinit[ selport ] ) return( 0 );									//	this is a caller error!?!

	strcpy_s( lastcommand, cmd );
	*recvbuf = 0;

	//	Check for unsolicited data

	if ( charin() )
	{
		TRACE( (char *) "Unsolicited:" );
		while ( charin() ) TRACE( (char *) " %02X", getbyte() );
		TRACE( (char *) "\n" );
	}

//	send the command with character pacing
//	Using sleep(1) sometimes works, but depending on system loading
//	can delay a lot longer than one ms.

	for ( i = 0; i < (int) strlen( cmd ); i ++ )
	{
		Sleep( pacing );
		outcom( cmd[ i ] );
		Sleep( pacing );
	}

	marktm = clock();
	c = 0;

	*recvbuf = 0;
	
	while ( maxlen > 1 && abs( clock() - marktm ) < timeout )
    {
		if ( ( i = charin( ) ) > 0 )
		{
			c = (unsigned char) ( getbyte( ) & 0xFF );

			if ( c == 0x6 || c == 0x15 )
			{
				*recvbuf ++ = c;
				*recvbuf = 0;
				return( c == 0x6 );
			}
			else
			{
				*recvbuf ++ = c;
				*recvbuf = 0;
				maxlen --;
			}
			marktm = clock( );
		}
		else
			if ( i < 0 )
			{
				TRACE( ( char * ) "Port disconnect\n" );
				return( FALSE );
			}
    }
	TRACE( (char *) "cmdio timeout to %s\n", cmd );

	return( FALSE );
}

//	This one echos received characters as we get them if echo is true.

BOOL cmdio( char *cmd, long timeout, char *recvbuf, int maxlen, bool echo )
{
	clock_t			marktm;
	unsigned char	c;
	int				i;

	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 ) return( FALSE );

	if ( !portinit[ selport ] ) return( FALSE );								//	this is a caller error!?!

#ifdef COMDEBUG
	strcpy_s( lastcommand, cmd );												//	save a diagnosic copy

	if ( charin( ) )
	{
		TRACE( (char *) "Unexpected RX: " );
		while ( charin( ) )
		{
			c = getbyte( );
			if ( c >= ' ' && c <= '~' )
				TRACE( (char *) "%c", c );
			else
				if ( c == '\r' || c == '\n' )
					TRACE( (char *) "\n" );
				else
					TRACE( (char *) "[%02X]", c );
		}
		TRACE( (char *) "\n" );
	}
#endif

	outcoms( cmd );																//	send the command

#ifdef COMDEBUG
	TRACE( (char *) "cmdio: " );
	for ( i = 0; cmd[ i ]; i ++ )
		if ( cmd[ i ] >= ' ' && cmd[ i ] <= '~' )
			TRACE( (char *) "%c", cmd[ i ] );
		else
			TRACE( (char *) "[%02X]", cmd[ i ] );
#endif

	marktm = clock( );
	c = 0;
	clock_t	t;

	*recvbuf = 0;

	while ( maxlen > 1 && ( t = abs( clock( ) - marktm ) ) < timeout )
	{
		if ( ( i = charin( ) ) > 0 )
		{
			c = (unsigned char) ( getbyte( ) & 0xFF );
#ifdef COMDEBUG
			TRACE( (char *) "[%02X(%c)]", c, c );
#endif
			if ( c == 0x6 || c == 0x15 )
			{
//				*recvbuf ++ = c;												//	don't return ACK/NAK
				*recvbuf = 0;
#ifdef COMDEBUG
				TRACE( (char *) " %s\n", ( c == 6 ) ? "OK" : "NAK" );
#endif
				return( c == 0x6 );
			}
			else
			{
				*recvbuf ++ = c;
				*recvbuf = 0;
				maxlen --;
				if ( echo ) printf( "%c", c );
			}
			marktm = clock( );
		}
		else
		{
			if ( i < 0 )
			{
				TRACE( (char *) "Port disconnect\n" );
				return( FALSE );
			}
		}
	}
	if ( maxlen )
		TRACE( (char *) "cmdio timeout to \"%s\"\n", cmd );
	else
		TRACE( (char *) "RX buffer filled\n" );
	return( FALSE );
}


//	cmdio for a full-duplex system (where transmitted characters are echoed back to us)

BOOL cmdiof( char *cmd, long timeout, char *recvbuf, int maxlen )
{
	clock_t			marktm;
	unsigned char	c;
	int				i;

	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 ) return( FALSE );

	if ( !portinit[ selport ] ) return( 0 );									//	this is a caller error!?!
	strcpy_s( lastcommand, cmd );

	if ( strlen( cmd ) && charin() )
	{
		while ( charin() ) getbyte();
	}

//	send the command waiting for each character to echo back to us.

	for ( i = 0; i < (int) strlen( cmd ); i ++ )
	{
		outcom( cmd[ i ] );
		marktm = clock();
		c = cmd[ i ] + 1;
		while ( clock() < marktm + 500 )
		{
			if ( ( i = charin( ) ) > 0 )
			{
				c = (byte) ( getbyte( ) & 0xFF );
				if ( c == cmd[ i ] ) break;
			}
			else
				if ( i < 0 )
				{
					TRACE( ( char * ) "Port disconnect\n" );
					return( FALSE );
				}
		}
		if ( c != cmd[ i ] )
			return( FALSE );
	}

	marktm = clock();
	c = 0;

	*recvbuf = 0;

	while ( maxlen > 1 && abs( clock() - marktm ) < timeout )
    {
		if ( ( i = charin( ) ) > 0 )
		{
			c = (unsigned char) ( getbyte( ) & 0xFF );
			if ( c == 0x6 || c == 0x15 )
			{
				*recvbuf ++ = c;
				*recvbuf = 0;
				return( c == 0x6 );
			}
			else
			{
				*recvbuf ++ = c;
				*recvbuf = 0;
				maxlen --;
			}
			marktm = clock( );
		}
		else
			if ( i < 0 )
			{
				TRACE( ( char * ) "Port disconnect\n" );
				return( FALSE );
			}
    }
	TRACE( (char *) "cmdiof timeout\n" );
	return( FALSE );
}


//	A routine like cmdio that sends a command then inputs a cr/lf terminated line.
//	neither the CR or LF is returned in the return string.
//	Returns true if the linefeed is received within maxlen characters of the reply,
//	and the time between characters is less than timeout (milliseconds).

bool getline( char *cmd, time_t timeout, char *buf, int maxlen )
{
	clock_t			marktm;
	char			c;

	if ( !maxlen ) return( false );

	if ( portinit[ selport ] == NULL && portinit[ selport ] && openport( portnumbers[ selport ] - 1 ) != 0 ) return( false );

	if ( !portinit[ selport ] ) return( false );								//	this is a caller error!?!

	if ( cmd != NULL && *cmd ) outcoms( cmd );									// send the optional command

	marktm = clock();															//	mark start time
	c = 0;
	*buf = 0;																	//	delimit the output line

	while ( maxlen > 1 && abs( clock() - marktm ) < timeout )
	{
		if ( charin() )
		{
			c = (char) ( getbyte() & 0xFF );									//	get the received character

			switch( c )
			{
			case LF:
				return( true );

			case CR:
				break;

			default:
				*buf ++ = c;
				*buf = 0;
				maxlen --;
			}
			marktm = clock();
		}
	}
	return( false );
}




// wait for a character string or timeout.

// Although the method may seem awkward, its one of very few ways
// to insure a string like : xxyz is detected in an input string like xxxyz.
// (A normal character compare would compare the first two x's, then compare
// the third input x to y, detect a mismatch and start over, entire missing
// the input substring xxyz).

// Returns true if time out/error

BOOL waitfor( long timeout, char *str )
{
	char	*bufr;
	size_t	len;
	clock_t	mt;
	
	if ( portinit[ selport ] == NULL && portinit[ selport ] 
		&& openport( portnumbers[ selport ] - 1 ) != 0 ) return( 1 );
	
	if ( !portinit[ selport ] ) return( 0 );									//	this is a caller error!?!

	len = strlen( str );														// get input data length
	if ( ( bufr = (char *) malloc( len ) ) == NULL ) return( 1 );
	memset( bufr, 0, len );
	mt = clock();

	while ( abs( clock() - mt ) < timeout )
    {
		if ( charin() )
        {
			memmove( bufr, &bufr[ 1 ], len - 1 );								// ripple received data through
			bufr[ len - 1 ] = (char) ( getbyte() & 0xFF );						// recv char goes in last array pos
			if ( !memcmp( bufr, str, len ) )									// compare last 'len' received chars
            {
				free( bufr );													// if match
				return( 0 );
            }
			mt = clock();
        }
    }
	free( bufr );
	return( 1 );
}


// 2/1/17 -- wait for a block of len or timeout.

//	Returns true if time out/error
//	Inputs data into caller's buffer

BOOL waitfor( char *bufr, int bufsiz, long timeout, char *block, int len )
{
	char	*iptr = bufr, *optr = bufr;
	clock_t	mt;
	
	if ( portinit[ selport ] == NULL && portinit[ selport ]
		&& openport( portnumbers[ selport ] - 1 ) != 0 ) return( TRUE );
	
	if ( !portinit[ selport ] ) return( 0 );									//	this is a caller error!?!

	memset( bufr, 0, bufsiz );													//	null out the receive buffer
	mt = clock();																//	mark start time

	while ( abs( clock() - mt ) < timeout )
    {
		if ( charin() )
        {
			*iptr ++ = getbyte() & 0xFF;
			if ( (unsigned long long) ( iptr - optr ) >= (unsigned long long) bufsiz - (unsigned long long) len - 2 )
			{
				printf( "waitfor: buffer overflow!\n" );
				return( FALSE );
			}

			if ( iptr - optr >= len && !memcmp( block, optr, len ) )
			{
				//	match

				*optr = 0;														//	remove the match
				return( FALSE );
			}
			mt = clock();														//	reset mark time
		}
    }
	return( TRUE );																//	never got the string
}


// 2/1/17 -- wait for a block of len or timeout.
//	This one accepts both the ACKnowledge and NAK strings to reduce the wait time.
//	Returns true if time out/error
//	Inputs data into caller's buffer

BOOL waitfor( char *bufr, int bufsiz, long timeout, char *ack, int acklen, char *nak, int naklen )
{
	char	*iptr = bufr, *optr = bufr;
	clock_t	mt;
	
	if ( portinit[ selport ] == NULL && portinit[ selport ] 
		&& openport( portnumbers[ selport ] - 1 ) != 0 ) return( TRUE );
	
	if ( !portinit[ selport ] ) return( 0 );									//	this is a caller error!?!

	memset( bufr, 0, bufsiz );													//	null out the receive buffer
	mt = clock();																//	mark start time

	while ( abs( clock() - mt ) < timeout )
    {
		if ( charin() > 0 )
        {
			*iptr ++ = getbyte() & 0xFF;

			if ( (unsigned long long) ( iptr - optr ) >= (unsigned long long) bufsiz - max( acklen, naklen ) - 2 )
			{
				printf( "waitfor: buffer overflow!\n" );
				return( FALSE );
			}

			if ( iptr - optr >= acklen )
			{
				//	match

				if ( !memcmp( ack, optr, acklen ) )
				{
					*optr = 0;													//	remove the match
					return( FALSE );
				}
			}

			if ( iptr - optr >= naklen )
			{
				//	match

				if ( !memcmp( nak, optr, naklen ) )
				{
					*optr = 0;													//	remove the match
					return( TRUE );												//	failed
				}
				else
					optr ++;
			}

			mt = clock();														//	reset mark time
		}
    }
	return( TRUE );																//	never got the string
}


// Transmit routines

// send c -> port

void outcom( char c )
{
	unsigned long	l = 0;
	
	if ( portinit[ selport ] == NULL && portinit[ selport ] && openport( portnumbers[ selport ] - 1 ) != 0 ) return;

	if ( !portinit[ selport ] ) return;

	//	3/25/2020
	while ( !WriteFile( portinit[ selport ], &c, 1, &l, NULL ) || !l ) ;
}


// send c -> port for a full-duplex system, eat the echo back.

void outcome( char c )
{
	int				q;
	unsigned long	l;
	clock_t			t;

	if ( portinit[ selport ] == NULL && portinit[ selport ] && openport( portnumbers[ selport ] - 1 ) != 0 ) return;

	if ( !portinit[ selport ] ) return;

	do
	{
		l = 0;
	}
	while ( ( q = WriteFile( portinit[ selport ], &c, 1, &l, 0 ) ) != 0 && l < 1 );

	t = clock();
	while ( t < clock() + 100 )
	{
		if ( charin() )
		{
			q = getbyte();
			if ( q == c ) break;
		}
	}
}

// send string to port

void outcoms( char *str )
{
	unsigned long	l;
	char			*p = str;

	
	if ( portinit[ selport ] == NULL && portinit[ selport ] && openport( portnumbers[ selport ] - 1 ) != 0 ) return;
	if ( !portinit[ selport ] ) return;

	while ( WriteFile( portinit[ selport ], p, (DWORD) strlen( p ), &l, 0 ) != 0 && l < strlen( p ) )
		p += l;
}


// send string to port in full-duplex system (e.g. eat the echo back characters)

void outcomsf( char *str )
{
	char			*p = str;
	char			c;
	time_t			t;

	
	if ( portinit[ selport ] == NULL && portinit[ selport ] && openport( portnumbers[ selport ] - 1 ) != 0 ) return;
	if ( !portinit[ selport ] ) return;

	while ( *p )
	{
		outcom( *p );
		t = clock();
		while ( clock() < t + 100 )
		{
			if ( charin() )
			{
				c = (byte) ( getbyte() & 0xFF );
				if ( c == *p ) break;
			}
		}
		p ++;
	}
}


unsigned long outcoms( char *str, unsigned long n )
{
	unsigned long	l;
	
	if ( portinit[ selport ] == NULL && portinit[ selport ] && openport( portnumbers[ selport ] - 1 ) != 0 ) return( 0 );

	if ( !portinit[ selport ] ) return( 0 );									//	this is a caller error!?!

	return( ( WriteFile( portinit[ selport ], str, (DWORD) n, &l, 0 ) != 0 ) ? l : 0 );
}


void outcoms( char *str, int pacing )
{
	for ( unsigned i = 0; i < strlen( str ); i ++ )
	{
		Sleep( pacing );
		outcom( str[ i ] );
	}
}



void outcomblock( unsigned char *block, int blocksize )
{
	DWORD	bw;

	if ( portinit[ selport ] == NULL && portinit[ selport ] && openport( portnumbers[ selport ] - 1 ) != 0 ) return;

	if ( !portinit[ selport ] ) return;											//	this is a caller error!?!

	WriteFile( portinit[ selport ], block, (DWORD) blocksize, &bw, NULL );
}

// wait till transmitter is ready

void waitxmitrdy( void )
{
	DWORD events = EV_TXEMPTY;

	
	if ( portinit[ selport ] == NULL && portinit[ selport ] && openport( portnumbers[ selport ] - 1 ) != 0 ) return;

	if ( !portinit[ selport ] ) return;											//	this is a caller error!?!

	SetCommMask( portinit[ selport ], events );
	WaitCommEvent( portinit[ selport ], &events, NULL );
}



// reset receive & transmit
// returns true on error (with error code)

DWORD rstcom( void )
{
	DWORD	x;

	if ( portinit[ selport ] == NULL && portinit[ selport ] && openport( portnumbers[ selport ] - 1 ) != 0 ) return( GetLastError() );

	if ( !portinit[ selport ] ) return( 0 );									//	this is a caller error!?!

	if ( !ClearCommError( portinit[ selport ], &x, NULL ) ) return( GetLastError() );
	if ( !PurgeComm( portinit[ selport ], PURGE_TXCLEAR | PURGE_RXCLEAR ) ) return( GetLastError() );
	return( 0 );
}


// get port parameters
// Returns true on error
//	Windows doesn't update the com signal states, so we'll do it here.

int getcomprm( DCB *params )
{
	if ( portinit[ selport ] == NULL && portinit[ selport ] && openport( portnumbers[ selport ] - 1 ) != 0 ) return( GetLastError() );

	if ( !portinit[ selport ] ) return( 0 );									//	this is a caller error!?!

#ifdef BLOCKIO
	memcpy( &psetting[ selport ], params, sizeof( psetting[ selport ] ) );		// keep a local copy for block io timeouts
#else
	memcpy( params, &portprams[ selport ], sizeof( DCB ) );
#endif
	return( 0 );
}


// set port parameters
// Returns true on error

//	Caution: params contains fields for the states of RTS and DTR.  If you
//			 call this function to change baud rate, these signals are
//			 updated to those states.  So call getcomprm first which updates these fields
//			 to the current signal states.

//	Parity is: EVENPARITY, MARKPARITY, NOPARITY, ODDPARITY or SPACEPARITY (from DCB def in winbase.h (or windows.h))

BOOL setcomprm( DCB *params )
{
	if ( portinit[ selport ] == NULL && portinit[ selport ] && openport( portnumbers[ selport ] - 1 ) != 0 )
		return( true );															//	3/25/2020

	if ( !portinit[ selport ] ) return( true );									//	this is a caller error!?!
//TRACE( (char *) "5RTS %d\n", params -> fRtsControl );

	if ( !SetCommState( portinit[ selport ], params ) )
		return( true );															//	3/25/2020
#ifdef BLOCKIO
	else
		memcpy( &psetting[ selport ], params, sizeof( psetting[ selport ] ) );
#endif

	memcpy( &portprams[ selport ], params, sizeof( portprams[ 0 ] ) );			//	update saved settings
	pinit[ selport ] = true;													//	signal the user has initialized the port
	return( false );
}



// activate port setup menu

void pramsetup( void )
{

}




// Simple terminal program

#define	NONPRINT	32

char	*nonprint[ NONPRINT + 1 ] =
{
	(char *) "NUL[00]",
	(char *) "SOH[01]",
	(char *) "STX[02]",
	(char *) "ETX[03]",
	(char *) "EOT[04]",
	(char *) "ENQ[05]",
	(char *) "ACK[06]",
	(char *) "BEL[07]",
	(char *) "BS[08]",
	(char *) "TAB[09]",
	(char *) "LF[0A]",
	(char *) "VT[0B]",
	(char *) "FF[0C]",
	(char *) "CR[0D]",
	(char *) "SO[0E]",
	(char *) "SI[0F]",
	(char *) "DLE[10]",
	(char *) "DC1[11]",
	(char *) "DC2[12]",
	(char *) "DC3[13]",
	(char *) "DC4[14]",
	(char *) "NAK[15]",
	(char *) "SYN[16]",
	(char *) "ETB[17]",
	(char *) "CAN[18]",
	(char *) "EM[19]",
	(char *) "SUB[1A]",
	(char *) "ESC[1B]",
	(char *) "FS[1C]",
	(char *) "GS[1D]",
	(char *) "RS[1E]",
	(char *) "US[1F]",
	NULL
};

#pragma warning ( push )
#pragma warning ( disable : 4706 )				// disable assignment within conditional expression warning

//	This is now the lower-level function, see simplecomm() below.

void simplecomma( unsigned termcode, bool halfduplex, char *msg, bool autolf )
{
	unsigned int	c = termcode + 1;			// initialize to a non-termination value
	bool			binary = false;				// binary (show all codes in hex/ASCII)
	int				i = 0;

	printf( msg );
	if ( termcode < NONPRINT )
		printf( "Press %s to terminate, F1 toggle RTS, F6 toggle binary mode, F7 toggle auto LF.\n", nonprint[ termcode ] );
	else
		printf( "Press %02X to terminate, F1 to toggle RTS.\n", termcode );

	while ( c != termcode )
	{
		if ( _kbhit() )
		{
			if ( ( c = (unsigned char) _getch() ) )
			{
				if ( c != termcode ) outcom( (char) c );
				if ( halfduplex ) _putch( c );
			}
			else
			{
				//	This is a function code

				if ( _kbhit() )
				{
					c = (unsigned char) _getch() << 8;		// get the code and put into Borland bioskey form

					switch ( c )
					{
					case F1:
						//	F1 toggles RTS

						i ^= RTS;
						if ( i )
						{
							setcomsig( getcomsig() | RTS );
//printf( "RTS set: %04X (RTS is %04X)\n", getcomsig(), RTS );
						}

						else
						{
							setcomsig( getcomsig() & ~RTS );
//printf( "RTS low: %04X\n", getcomsig() );
						}
						printf( "RTS %s\n", ( i & RTS ) ? "On" : "Off" );
						break;

					case F6:
						binary = !binary;
						printf( "\n%s mode.\n", ( binary ) ? "Binary" : "ASCII" );
						break;

					case F7:
						autolf = !autolf;
						printf( "\n%s mode.\n", ( autolf ) ? "Auto LF" : "No LF" );
						break;

					default:
						printf( "[%04X]", c );
					} // switch(c)
				}
			}
		}
		
		if ( charin() > 0 )
		{
			i = getbyte() & 0xFF;

			if ( i < NONPRINT && ( binary || !( i == 0xD || i == 0xA || i == 0x9 ) ) )
				printf( "%s", nonprint[ i ] );
			else
			{
				if ( i > 0x7E )
					printf( "[%02X]", i );
				else
				{
					_putch( i );
					if ( i == CR && autolf ) _putch( '\n' );
				}
			}
		}
	}
}
#pragma warning( pop )	// put things back

//	This one is compatible with older module versions.

void simplecomm( unsigned termcode, BOOL halfduplex, char *msg )
{
	simplecomma( termcode, halfduplex, msg, FALSE );
}

// Retrieve MODEM status
//	Revised 7/2011 to be more compatible with the old Borland stuff by
//	returning not only the input signal states, but also the last recorded
//	output signal states.

int getcomsig( void )
{
	DWORD	status;

	
	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 ) return( GetLastError() );

	if ( !GetCommModemStatus( portinit[ selport ], &status ) ) return( GetLastError() );

	status &= ( CTS | DSR | RI | RLSD );	// mask the input bits

	if ( portprams[ selport ].fRtsControl != 0 ) status |= RTS;
	if ( portprams[ selport ].fDtrControl != 0 ) status |= DTR;
	return( status );
}


int getcomsig( int port )
{
	int	oldport = selport;
	int	result;

	portselect( port );
	result = getcomsig();
	selport = oldport;
	return( result );
}

// set port's MODEM control signals
// Returns error code ( > 0 ) if error else false.
// newstat is 0, RTS, DTR, or RTS | DTR
// 3/16/10 -- fixed logical AND to bit-wise AND
//	3/1/19 -- Rewrote using Set/GetCommState.

int setcomsig( int newstat )
{
	time_t	st = clock();

	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 )
		return( GetLastError() );

	if ( newstat & RTS )
		portprams[ selport ].fRtsControl = RTS_CONTROL_ENABLE;
	else
		portprams[ selport ].fRtsControl = RTS_CONTROL_DISABLE;

	if ( newstat & DTR )
		portprams[ selport ].fDtrControl = DTR_CONTROL_ENABLE;
	else
		portprams[ selport ].fDtrControl = DTR_CONTROL_DISABLE;

	if ( !SetCommState( portinit[ selport ], &portprams[ selport ] ) || clock() - st > PDTIMEOUT )	//	put them back -- this generates an error if the port has been disconnected
		return( GetLastError() );

	return( 0 );
}


//	10/21/14 -- set the communication signals of a different port
//	Returns true on error

BOOL setcomsig( int port, int newstat )
{
	int	oldport = selport;

	portselect( port );
	setcomsig( newstat );
	selport = oldport;
	return( FALSE );
}


// Here's the original Borland function
// Returns true on error

BOOL sendbreak( int breakon )
{
	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 ) return( GetLastError() );

	if ( breakon )
	{
		if ( !SetCommBreak( portinit[ selport ] ) ) return( GetLastError() );
	}
	else
	{
		if ( !ClearCommBreak( portinit[ selport ] ) ) return( GetLastError() );
	}

	return( 0 );
}


// Send a break for howlong ticks
// Returns true on error

BOOL sendbreak_timed( int howlong )
{
	clock_t	timer;
	
	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 ) return( GetLastError() );

	if ( !SetCommBreak( portinit[ selport ] ) ) return( GetLastError() );

	timer = clock();

	while ( abs( clock() - timer ) < howlong ) ;

	if ( !ClearCommBreak( portinit[ selport ] ) ) return( GetLastError() );

	return( 0 );
}


//	Check all opened com ports for receive data.
//	Returns the COM port # of received data 1..N, and updates rx with the data
//	otherwise it returns false (0).
//	Use a rotating priority scheme so one port doesn't hug all the action.

#ifdef	RS232_DIAGS
int lastnports[ 100 ];
int	lastrports[ 100 ];
int	lastri = 0;
int	lastni = 0;
#endif

int	anyrx( unsigned char *rx )
{
	int		oldport = selport;							// remember which port the app had selected

	for ( int i = ( oldport + 1 ) % NUMCOMPORT; i != oldport; i = ( i + 1 ) % NUMCOMPORT )
	{
		if ( portinit[ i ] != NULL )					// if the port 'i' is active
		{
#ifdef	RS232_DIAGS
			lastnports[ lastni ] = i;					// track the last 100 ports polled
			if ( ++ lastni >= 100 ) lastni = 0;
			portpoll[ i ] ++;
#endif
			selport = i;								// select it
			if ( charin() )
			{
#ifdef	RS232_DIAGS
				lastrports[ lastri ] = i;
				rxport[ i ] ++;
				if ( ++ lastri >= 100 ) lastri = 0;
#endif
				if ( rx != NULL )
					*rx = (unsigned char) ( getbyte() & 0xFF );	// get the received char
				else
					getbyte();							// else just toss it
				return( i + 1 );
			} // rx char pending
		} // this port is open
	} // for i
	selport = oldport;
	return( 0 );
}


//	Return a list of available COM ports on this PC.
//	Returns # ports and the first *nports entries
//	of comportlist are updated with their numbers.
//	If comportlist is NULL, nports is ignored and
//	the function returns the number of ports.
//	7/9/21-- unfortunately this function only finds
//	DOS supported serial ports, not USB to serial converters.
//	Use getserialports() below to find them all.

int getcomports( int *comportlist )
{
	char	*devices, *p;
	int		i, ports = 0, numdevice = 0;

	if ( ( devices = (char *) malloc( 65536 ) ) == NULL )
		return( 0 );															//	out of memory

	if ( ( i = QueryDosDeviceA( NULL, devices, 65536 ) ) == 0 )
	{
		//	Failed to get system info

		free( devices );
		return( 0 );
	}
	else
	{
		p = devices;

		while ( i && *p )
		{
			if ( !strncmp( p, "COM", 3 ) )
			{
				//	Found one

				ports ++;

				if ( comportlist != NULL )
				{
					if ( sscanf( p, "COM%d", comportlist ) == 1 )
					{
						comportlist ++;
					}
					else
					{
						free( devices );
						return( 0 );
					}
				}
			}

			numdevice ++;

			while ( *p ) p ++, i --;
			p ++;
			i --;
		}
	}

	TRACE( (char *) "Found %d COM ports out of %d system devices\n", ports, numdevice );
	free( devices );
	return( ports );
}


#ifdef	RS232_DIAGS

//	Print the selected port's capabilities from a COMMPROP structure.
//	The format is the same for settablebaud except multiple bits can
//	be set.  We'll just assume maxbaud has a single bit set.

static char *bauds( unsigned long d )
{
	static	char	buf[ 200 ] = "";

	char *rates[ 32 ] =
	{
		"75",			// x1
		"110",			// x2
		"135",			// x4
		"150",			// x8
		"300",			// x10
		"600",			// x20
		"1200",			// x40
		"1800",			// x80
		"2400",			// x100
		"4800",			// x200
		"7200",			// x400
		"9600",			// x800
		"14400",		// x1000
		"19200",		// x2000
		"38400",		// x4000
		"56000",		// x8000
		"128000",		// x10000
		"1152300",		// x20000
		"57600",		// x40000
		"?",			// x80000
		"?",			// x100000
		"?",			// x200000
		"?",			// x400000
		"?",			// x800000
		"?",			// x1000000
		"?",			// x2000000
		"?",			// x4000000
		"?",			// x8000000
		"Programmable",	// x10000000
		"",				// x20000000
		"",				// x40000000
		""				// x80000000
	};

	*buf = 0;
	for ( int i = 0; i < 32; i ++ )
	{
		if ( d & 1 )
		{
			if ( *buf ) strcat( buf, ", " );
			strcat( buf, rates[ i ] );
		}
		d >>= 1;
	}
	return( buf );
}


static char *subtype( DWORD val )
{
	typedef struct
	{
		int		id;
		char	*type;
	} typ_r;

	static typ_r typ[] =
	{
		{ 0x21,			"FAX" },
		{ 0x101,		"LAT protocol" },
		{ 6,			"MODEM" },
		{ 0x100,		"Unspecified network bridge" },
		{ 2,			"Parallel port" },
		{ 1,			"RS-232" },
		{ 3,			"RS-422" },
		{ 4,			"RS-423" },
		{ 5,			"RS-449" },
		{ 0x22,			"Scanner" },
		{ 0x102,		"TCP/IP Telnet" },
		{ 0,			"Unspecified" },
		{ 0x103,		"X.25 standard" },
		{ -1,			"" }
	};

	for ( int i = 0; typ[ i ].id >= 0; i ++ )
	{
		if ( typ[ i ].id == val ) return( typ[ i ].type );
	}

	return( "Unknown" );
}


static char *capabilities( DWORD val )
{
	static char	buf[ 100 ];

	typedef struct
	{
		int		id;
		char	*type;
	} typ_r;

	static typ_r typ[] =
	{
		{ 0x200,		"Spcl 16-bit" },
		{ 1,			"DTR/DSR" },
		{ 0x80,			"time-outs" },
		{ 8,			"Parity" },
		{ 4,			"RLSD" },
		{ 2,			"RTS/CTS" },
		{ 0x20,			"Set XON/XOFF" },
		{ 0x40,			"Spcl chars" },
		{ 0x10,			"XON/XOFF" },
		{ -1,			"" }
	};

	*buf = 0;

	for ( int i = 0; typ[ i ].id >= 0 && val; i ++ )
	{
		if ( val & typ[ i ].id )
		{
			if ( *buf ) strcat( buf, ", " );
			strcat( buf, typ[ i ].type );

			val &= ~typ[ i ].id;
		}
	}

	return( buf );
}


static char *settableparams( DWORD val )
{
	static char	buf[ 100 ];

	typedef struct
	{
		int		id;
		char	*type;
	} typ_r;

	static typ_r typ[] =
	{
		{ 1,			"Parity" },
		{ 2,			"BAUD" },
		{ 4,			"Data bits" },
		{ 8,			"Stop bits" },
		{ 0x10,			"flow control" },
		{ 0x20,			"Parity check" },
		{ 0x40,			"RLSD" },
		{ -1,			"" }
	};

	*buf = 0;
	for ( int i = 0; typ[ i ].id >= 0 && val; i ++ )
	{
		if ( val & typ[ i ].id )
		{
			if ( *buf ) strcat( buf, ", " );
			strcat( buf, typ[ i ].type );

			val &= ~typ[ i ].id;
		}
	}

	return( buf );
}


static char *settabledata( DWORD val )
{
	static char	buf[ 100 ];

	typedef struct
	{
		int		id;
		char	*type;
	} typ_r;

	static typ_r typ[] =
	{
		{ 1,			"5" },
		{ 2,			"6" },
		{ 4,			"7" },
		{ 8,			"8" },
		{ 0x10,			"16" },
		{ 0x20,			"16X" },
		{ -1,			"" }
	};

	*buf = 0;
	for ( int i = 0; typ[ i ].id >= 0 && val; i ++ )
	{
		if ( val & typ[ i ].id )
		{
			if ( *buf ) strcat( buf, ", " );
			strcat( buf, typ[ i ].type );

			val &= ~typ[ i ].id;
		}
	}

	return( buf );
}



static char *settablestopparity( DWORD val )
{
	static char	buf[ 100 ];

	typedef struct
	{
		int		id;
		char	*type;
	} typ_r;

	static typ_r typ[] =
	{
		{ 1,			"1S" },
		{ 2,			"1.5S" },
		{ 4,			"2S" },
		{ 8,			"8" },
		{ 0x100,		"No P" },
		{ 0x200,		"ODD" },
		{ 0x400,		"EVEN" },
		{ 0x800,		"MARK" },
		{ 0x1000,		"SPACE" },
		{ -1,			"" }
	};

	*buf = 0;
	for ( int i = 0; typ[ i ].id >= 0 && val; i ++ )
	{
		if ( val & typ[ i ].id )
		{
			if ( *buf ) strcat( buf, ", " );
			strcat( buf, typ[ i ].type );

			val &= ~typ[ i ].id;
		}
	}

	return( buf );
}

//	Return device capabilities in a COMMPROP structure.
//	returns 0 on success or error last error value.

int getcomprop( LPCOMMPROP p )
{
	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 ) return( GetLastError() );

	if ( !GetCommProperties( portinit[ selport ], p ) ) return( GetLastError() );
	return( 0 );
}

//	Retrieve and print the selected port's capabilities.

void printproperties( void )
{
	COMMPROP	prop;

	if ( getcomprop( &prop ) ) return;

	printf( "COM-%d Properties:\n"
			"wPacketLength		: %d\n"
			"wPacketVersion		: %d\n"
			"dwServiceMask		: %d\n"
			"dwReserved1		: %d\n"
			"dwMaxTxQueue		: %d\n"
			"dwMaxRxQueue		: %d\n"
			"dwMaxBaud		: %s\n"
			"dwProvSubType		: %s\n"
			"dwProvCapabilities	: %s\n"
			"dwSettableParams	: %s\n",
			selport,
			prop.wPacketLength,
			prop.wPacketVersion,
			prop.dwServiceMask,
			prop.dwReserved1,
			prop.dwMaxTxQueue,
			prop.dwMaxRxQueue,
			bauds( prop.dwMaxBaud ),
			subtype( prop.dwProvSubType ),
			capabilities( prop.dwProvCapabilities ),
			settableparams( prop.dwSettableParams ) );

	printf( "dwSettableBaud		: %s\n"
			"wSettableData		: %s\n"
			"wSettableStopParity	: %s\n"
			"dwCurrentTxQueue	: %d\n"
			"dwCurrentRxQueue	: %d\n"
			"dwProvSpec1		: %d\n"
			"dwProvSpec2		: %d\n"
			"wcProvChar[1]		: %d\n",
			bauds( prop.dwSettableBaud ),
			settabledata( prop.wSettableData ),
			settablestopparity( prop.wSettableStopParity ),
			prop.dwCurrentTxQueue,
			prop.dwCurrentRxQueue,
			prop.dwProvSpec1,
			prop.dwProvSpec2,
			prop.wcProvChar[1] );
}

#endif

#ifdef	BLOCKIO

//	================================================================================
//	Overlapped I/O support routines.

//	These routines send and receive large blocks of data asynchronously, quueing the
//	operation and returning virtually immediately.  Routines are provided to
//	determine when the queued operations are complete.
//	================================================================================

//	Read completion callback routine is called when a block completes.


typedef void (CALLBACK *cbptr)( DWORD error, DWORD transfered, LPOVERLAPPED olap );

void readdone( _In_ DWORD port, _In_ DWORD error, _In_ DWORD transfered, _Inout_ LPOVERLAPPED olap )
{
	if ( olap == &xolap[ port ] )
	{
		//	This is the transmitter callback.
		return;
	}

	switch ( error )
	{
	case ERROR_SUCCESS:
		if ( transfered >= colap[ port ].eolap[ colap[ port ].chunksrecv ].bufsiz )
		{
			//	The chunk transfer is complete.

			//printf( "finished %d buffer %p len %ld\n", ( colap[ port ].eolap[ colap[ port ].chunksrecv ].buf - rxbuf[ 0 ] ) / rxbsiz, colap[ port ].eolap[ colap[ port ].chunksrecv ].buf, colap[ port ].eolap[ colap[ port ].chunksrecv ].bufsiz );
			colap[ port ].chunksrecv ++;
		}
		break;

	default:
//		printf( "error?" );
		;
	} // switch(error)
//printf( "exit\n" );
}


VOID CALLBACK readdone1( _In_ DWORD error, _In_ DWORD transfered, _Inout_ LPOVERLAPPED olap )
{
	readdone( 0, error, transfered, olap );
}

VOID CALLBACK readdone2( _In_ DWORD error, _In_ DWORD transfered, _Inout_ LPOVERLAPPED olap )
{
	readdone( 1, error, transfered, olap );
}

VOID CALLBACK readdone3( _In_ DWORD error, _In_ DWORD transfered, _Inout_ LPOVERLAPPED olap )
{
	readdone( 2, error, transfered, olap );
}

VOID CALLBACK readdone4( _In_ DWORD error, _In_ DWORD transfered, _Inout_ LPOVERLAPPED olap )
{
	readdone( 3, error, transfered, olap );
}

VOID CALLBACK readdone5( _In_ DWORD error, _In_ DWORD transfered, _Inout_ LPOVERLAPPED olap )
{
	readdone( 4, error, transfered, olap );
}

VOID CALLBACK readdone6( _In_ DWORD error, _In_ DWORD transfered, _Inout_ LPOVERLAPPED olap )
{
	readdone( 5, error, transfered, olap );
}

VOID CALLBACK readdone7( _In_ DWORD error, _In_ DWORD transfered, _Inout_ LPOVERLAPPED olap )
{
	readdone( 6, error, transfered, olap );
}

VOID CALLBACK readdone8( _In_ DWORD error, _In_ DWORD transfered, _Inout_ LPOVERLAPPED olap )
{
	readdone( 7, error, transfered, olap );
}

VOID CALLBACK readdone9( _In_ DWORD error, _In_ DWORD transfered, _Inout_ LPOVERLAPPED olap )
{
	readdone( 8, error, transfered, olap );
}

VOID CALLBACK readdone10( _In_ DWORD error, _In_ DWORD transfered, _Inout_ LPOVERLAPPED olap )
{
	readdone( 9, error, transfered, olap );
}


cbptr xyz[ NUMCOMPORT ] =
{
	readdone1, readdone2, readdone3, readdone4, readdone5,
	readdone6, readdone7, readdone8, readdone9, readdone10
};


//	Write a block of data of length size.

//	Returns 0 on error or # bytes written which should equal size.
//	This function blocks, that is, it returns only after almost all
//	of the data is written.

unsigned long outblock( unsigned char *data, unsigned long size )
{
	unsigned long wrote;

	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 ) return( 0 );

	return( WriteFile( portinit[ selport ], data, size, &wrote, NULL ) ? wrote : 0 );
}


//	Write a block of data of length size.

//	Returns 0 on error or # bytes written which should equal size.
//	This function does not block, that is, it returns almost immediately.
//	Use chunksent() to determine when data transmission is complete.

unsigned long outchunk( unsigned char *data, unsigned long size )
{
	if ( portinit[ selport ] == NULL && openport( portnumbers[ selport ] - 1 ) != 0 ) return( 0 );

	WriteFileEx( portinit[ selport ], data, size, &xolap[ selport ], xyz[ selport ] );
	return( size );
}


BOOL chunksent( void )
{
	return( xolap[ selport ].InternalHigh != 0 );
}


//	Read a block of data from a port.  This routine is used to
//	receive large, high-speed blocks of serial data.  It is not
//	compatible with charin()/getbyte() because it doesn't use readahead[].
//	The two schemes should not be alternately used by an application.

//	This routine breaks the incoming buffer into chunks (bufsize bytes in size)
//	and sets up requests to receive them, one after the other.  The companion
//	routines, waitallchunks() and waitforchunks(), reports the progress of the
//	transfer.

//	When size==bufsize, a single transfer is performed, so you don't get any
//	data until *all* the data is received.  By breaking large buffers into
//	smaller chunks, you'll get access to data quicker.

//	Returns # bytes setup to transfer, 0 if error.

int getchunk( int port, unsigned char *buf, int size, int bufsize )
{
	int				i, j, k, x;
	ULONG			n = size / bufsize + ( ( size % bufsize ) != 0 );					// how many transfers are needed
	
	if ( portinit[ port ] == NULL && openport( port ) != 0 ) return( GetLastError() );	// the port wasn't opened, and our attempt to do so failed!

	//	We allocate an array of OVERLAP structures to handle the transfers.

	if ( colap[ port ].eolap == NULL )
	{
		//	Allocate an array of eolap, one for each buffer chunk.

		colap[ port ].eolap = (enh_olap_t *) malloc( ( n + 1 ) * sizeof( enh_olap_t ) );
	}
	else
	{
		if ( colap[ port ].numeolap != n + 1 )
		{
			//	Reallocate the array

			colap[ port ].eolap = (enh_olap_t *) realloc( colap[ port ].eolap, ( n + 1 ) * sizeof( enh_olap_t ) );
		}
	}

	if ( colap[ port ].eolap == NULL )
	{
		//	Out of memory!
		return( 0 );
	}

	colap[ port ].numeolap = n + 1;														// how many are allocated
	colap[ port ].chunksrecv = 0;
	colap[ port ].chunksreturned = 0;
	memset( colap[ port ].eolap, 0, ( n + 1 ) * sizeof( enh_olap_t ) );					// init overlapped structures

	//	Set ctimeout to the clock() value when we will have waited too long for the receive block.
	//	Adjust the receive timeout to match the buffer size

	timeout[ port ] = (long) ( ( (long long) 3 * CLOCKS_PER_SEC * size * 10 ) / psetting[ port ].BaudRate );	// how long it should take to transfer size bytes @ BaudRate, in ms with 3x safety factor
	ctimeout[ port ] = clock() + timeout[ port ];										// the clock value when we've taken too long

	//	initialize the chunks

	for ( i = k = 0, x = size; x || k < 2; i ++ )										// for each buffer we're transferring
	{
		colap[ port ].eolap[ i ].buf = buf;												// where this data is going
		colap[ port ].eolap[ i ].bufsiz = min( x, bufsize );							// set this transfer size
//printf( "buffer %d @ %p %ld bytes: ", i, buf, colap[ port ].eolap[ i ].bufsiz );

		if ( colap[ port ].eolap[ i ].bufsiz )
		{
			colap[ port ].eolap[ i ].olap.hEvent = CreateEvent( 0, true, 0, NULL );		// create an event
			if (
				( j = ReadFileEx( portinit[ port ],
							colap[ port ].eolap[ i ].buf,
							colap[ port ].eolap[ i ].bufsiz,
							&colap[ port ].eolap[ i ].olap,
							xyz[ port ] ) )
				)
			{
//				printf( "getchunk: started chunk %d status 0x%X Event: %d\n", i, j, colap[ port ].eolap[ i ].olap.hEvent );
			}
			else
			{
				printf( "Chunk start error %d\n", i, strerror( GetLastError() ) );
				return( 0 );
			}
		} // not the last chunk (zero length buffer)
//		else
//			printf( "\n" );

		x -= colap[ port ].eolap[ i ].bufsiz;											// update remaining bytes
		buf += colap[ port ].eolap[ i ].bufsiz;											// where the next data goes
		k += !x;																		// k = count of zero buffer length eolaps
	}//	colap[ port ].eolap[ i ].buf = buf;												// this buffer value is retuned as the end of the last block
	return( size );
}



//	Wait for all chunks to finish, or for them to timeout.
//	Returns total bytes transferred on all channels on success
//	else 0.

void waitallchunks( void )
{
	int			i;
	BOOL		iorunning;

	do
	{
		iorunning = FALSE;		// assume all channels finished

		for ( i = 0; i < NUMCOMPORT; i ++ )
		{
			if ( portinit[ i ] != NULL )
			{
				//	This port in use

				if ( colap[ i ].chunksrecv < colap[ i ].numeolap )
				{
					//	This port transfer not complete
					
					if (
						HasOverlappedIoCompleted( &colap[ i ].eolap[ colap[ i ].chunksrecv ].olap ) &&
						colap[ i ].chunksrecv < colap[ i ].numeolap
						)
					{
//						startnextblock( i );					// start the next block transfer
						ctimeout[ i ] = clock() + timeout[ i ];	// reset the timer
					}	
					iorunning = TRUE;
				} // for each chunk of data
				else
				{
					//	Wait for the last transfer to complete!

					if (
							!HasOverlappedIoCompleted( &colap[ i ].eolap[ colap[ i ].chunksrecv - 1 ].olap ) ||
							colap[ i ].eolap[ colap[ i ].chunksrecv - 1 ].olap.InternalHigh < colap[ i ].eolap[ colap[ i ].chunksrecv - 1 ].bufsiz
						)
						iorunning = TRUE;
				}
			} // port in use
		} // for each COM port
	}
	while ( iorunning ) ;
}


//	Wait for new data on any of the ports getchunk was called.
//	Returns a pointer to the first unprocessed data, and sets
//	endofdata to the last byte of data (+ 1).

//	Returns NULL when all data on all getchunk ports is processed.

//	callfunc is a callback function that's periodically called while
//	waiting for data.

//	This routine is written to give multiple ports rotating priority so
//	a single channel doesn't hog all the CPU time, even at high BAUD rates.

unsigned char *waitforchunks( unsigned char **endofdata, void (*callfunc)( void ) )
{
	static int		port = 0;													// the last port we serviced
	BOOL			lastfin[ NUMCOMPORT ],										// flags to signal open ports have received all their data
					fin, first = TRUE;											// flags to implement serial port rotating priority
//	BOOL			queued;
	clock_t			t;
	int				i, j;
	unsigned char	*p;
	int				pctr;

//	if( _heapchk() != _HEAPOK ) DebugBreak();
	memset( lastfin, 0, sizeof( lastfin ) );
	for ( fin = FALSE; !fin; port = ( port + 1 ) % NUMCOMPORT )
	{
		pctr = 0;

		if ( portinit[ port ] != NULL && !lastfin[ port ] )
		{
			//	This port is in use, see if a chunk is available

			j = colap[ port ].chunksrecv;
			i = WaitForSingleObjectEx( colap[ port ].eolap[ j ].olap.hEvent, 0, TRUE );
//			i = SleepEx( 0, TRUE );		<< this works too
			switch ( i )
			{
			case WAIT_ABANDONED:
//				printf( "abandoned\n" );
				break;

			case WAIT_IO_COMPLETION:
//				printf( "%d i/o complete %ld\n", j , clock() );
				break;
				//0x000000C0L  The wait was ended by one or more user-mode asynchronous procedure calls (APC) queued to the thread.

			case WAIT_OBJECT_0:
				break;
				//0x00000000L  The state of the specified object is signaled. MEANS THE OPERATION HAS COMPLETED

			case WAIT_TIMEOUT:
				break;
				//0x00000102L  The time-out interval elapsed, and the object's state is nonsignaled.  The read isn't yet finished e.g. we're still waiting

			case WAIT_FAILED:
//				printf( "fail: %s\n", strerror( GetLastError() ) );
				break;

			default:
				printf( "unknown return value\n" );
			} // switch waitforsingleobjectex()

			if ( colap[ port ].chunksreturned < colap[ port ].chunksrecv )
			{
				p = colap[ port ].eolap[ colap[ port ].chunksreturned ].buf;				// pointer to filled buffer
				*endofdata = colap[ port ].eolap[ ++ colap[ port ].chunksreturned ].buf;	// pointer to end of filled buffer
				lastfin[ port ] = colap[ port ].chunksreturned >= colap[ port ].numeolap;	// if we've returned all the chunks from this port
				port = ( port + 1 ) % NUMCOMPORT;											// advance port so we don't hang on the same port
				return( p );																// return the buffer start
			}
			else
			{
				//	No data available, is there a time-out ?
				
				if ( ( t = clock() ) > ctimeout[ port ] )
				{
					//	Transfer time-out

//					printf( "timeout!\a\n" );

					if ( !CancelIo( portinit[ port ] ) )
					{
						printf( "Can't cancel I/O on COM-%d (%s)\a\n", port + 1, strerror( GetLastError() ) );
						return( NULL );
					}
					CloseHandle( portinit[ port ] );
					portinit[ port ] = NULL;			// 7/9/13 signal the port is closed
					if ( colap[ port ].eolap != NULL ) delete colap[ port ].eolap;
					colap[ port ].eolap = NULL;
					colap[ port ].numeolap = 0;
					colap[ port ].chunksrecv = 0;
					colap[ port ].chunksreturned = 0;
					return( NULL );
				}
			} // there's a comm time-out

			if ( first )
			{
				first = FALSE;
				fin = lastfin[ port ];
			}
			else
				fin &= lastfin[ port ];

			if ( callfunc != NULL ) callfunc();
		} // port is in use
	} // for each COM port

	return( NULL );
}

#endif

/*
//	6/10/2020-- this code allows for the retreval of serial port description text from the system which
//	identifies the hardware that implements the port.  Sometimes that matters!

#define MAX_NAME_PORTS 7
#define RegDisposition_OpenExisting (0x00000001)								// open key only if exists
#define CM_REGISTRY_HARDWARE        (0x00000000)

typedef WORD ( WINAPI *CM_Open_DevNode_Key )( DWORD, DWORD, DWORD, DWORD, ::PHKEY, DWORD );

HANDLE  BeginEnumeratePorts( VOID )
{
	BOOL guidTest = FALSE;
	DWORD RequiredSize = 0;
	HDEVINFO DeviceInfoSet;
	char	*buf;

	guidTest = SetupDiClassGuidsFromNameA( "Ports", 0, 0, &RequiredSize );
	if ( RequiredSize < 1 ) return NULL;
	buf = (char *) malloc( RequiredSize * sizeof( GUID ) );
	guidTest = SetupDiClassGuidsFromNameA( "Ports", (LPGUID) buf, RequiredSize * sizeof( GUID ), &RequiredSize );

	if ( !guidTest )return NULL;

	DeviceInfoSet = SetupDiGetClassDevs( (LPGUID) buf, NULL, NULL, DIGCF_PRESENT );
	if ( DeviceInfoSet == INVALID_HANDLE_VALUE ) return NULL;

	free( buf );

	return DeviceInfoSet;
}

BOOL EnumeratePortsNext( HANDLE DeviceInfoSet, LPTSTR lpBuffer )
{
	static CM_Open_DevNode_Key OpenDevNodeKey = NULL;
	static HINSTANCE CfgMan;

	int res1;
	char DevName[ MAX_NAME_PORTS ] = { 0 };
	static int numDev = 0;
	int numport;

	SP_DEVINFO_DATA DeviceInfoData = { 0 };
	DeviceInfoData.cbSize = sizeof( SP_DEVINFO_DATA );


	if ( !DeviceInfoSet || !lpBuffer )return -1;
	if ( !OpenDevNodeKey )
	{
		CfgMan = LoadLibrary( "cfgmgr32" );
		if ( !CfgMan ) return FALSE;
		OpenDevNodeKey = (CM_Open_DevNode_Key) GetProcAddress( CfgMan, "CM_Open_DevNode_Key" );

		if ( !OpenDevNodeKey )
		{
			FreeLibrary( CfgMan );
			return FALSE;
		}
	}

	while ( TRUE )
	{

		HKEY KeyDevice;
		DWORD len;
		res1 = SetupDiEnumDeviceInfo( DeviceInfoSet, numDev, &DeviceInfoData );

		if ( !res1 )
		{
			SetupDiDestroyDeviceInfoList( DeviceInfoSet );
			FreeLibrary( CfgMan );
			OpenDevNodeKey = NULL;
			return FALSE;
		}

		res1 = OpenDevNodeKey( DeviceInfoData.DevInst, KEY_QUERY_VALUE, 0, RegDisposition_OpenExisting, &KeyDevice, CM_REGISTRY_HARDWARE );
		if ( res1 != ERROR_SUCCESS )return NULL;
		len = MAX_NAME_PORTS;

		res1 = RegQueryValueEx(
			KeyDevice,															// handle of key to query
			"portname",															// address of name of value to query
			NULL,																// reserved
			NULL,																// address of buffer for value type
			(LPBYTE) DevName,													// address of data buffer
			&len																// address of data buffer size
		);

		RegCloseKey( KeyDevice );
		if ( res1 != ERROR_SUCCESS )return NULL;
		numDev++;
		if ( _memicmp( DevName, "com", 3 ) )continue;
		numport = atoi( DevName + 3 );
		if ( numport > 0 && numport <= 256 )
		{
			strcpy( lpBuffer, DevName );
			return TRUE;
		}

		FreeLibrary( CfgMan );
		OpenDevNodeKey = NULL;
		return FALSE;
	}
}


BOOL  EndEnumeratePorts( HANDLE DeviceInfoSet )
{
	if ( SetupDiDestroyDeviceInfoList( DeviceInfoSet ) )return TRUE;
	else return FALSE;
}


HKEY GetDeviceKey( LPTSTR portName )
{
	static HINSTANCE CfgMan;
	static CM_Open_DevNode_Key OpenDevNodeKey = NULL;
	static int numDev = 0;
	int res1;
	BOOL guidTest = FALSE;
	DWORD RequiredSize = 0;
	char* buf;
	HDEVINFO DeviceInfoSet;
	SP_DEVINFO_DATA DeviceInfoData = { 0 };
	char DevName[ MAX_NAME_PORTS ] = { 0 };

	guidTest = SetupDiClassGuidsFromNameA(
		"Ports", 0, 0, &RequiredSize );
	if ( RequiredSize < 1 )return NULL;

	buf = (char *) malloc( RequiredSize * sizeof( GUID ) );

	guidTest = SetupDiClassGuidsFromNameA(
		"Ports", (GUID *) &buf, RequiredSize * sizeof( GUID ), &RequiredSize );

	if ( !guidTest )return NULL;


	DeviceInfoSet = SetupDiGetClassDevs( (LPGUID) buf, NULL, NULL, DIGCF_PRESENT );
	if ( DeviceInfoSet == INVALID_HANDLE_VALUE )return NULL;

	free( buf );

	if ( !OpenDevNodeKey )
	{
		CfgMan = LoadLibrary( "cfgmgr32" );
		if ( !CfgMan )return NULL;
		OpenDevNodeKey =
			(CM_Open_DevNode_Key) GetProcAddress( CfgMan, "CM_Open_DevNode_Key" );
		if ( !OpenDevNodeKey )
		{
			FreeLibrary( CfgMan );
			return NULL;
		}
	}

	DeviceInfoData.cbSize = sizeof( SP_DEVINFO_DATA );

	while ( TRUE )
	{
		HKEY KeyDevice;
		DWORD len;
		res1 = SetupDiEnumDeviceInfo(
			DeviceInfoSet, numDev, &DeviceInfoData );

		if ( !res1 )
		{
			SetupDiDestroyDeviceInfoList( DeviceInfoSet );
			FreeLibrary( CfgMan );
			OpenDevNodeKey = NULL;
			return NULL;
		}

		res1 = OpenDevNodeKey( DeviceInfoData.DevInst, KEY_QUERY_VALUE | KEY_WRITE, 0,
			RegDisposition_OpenExisting, &KeyDevice, CM_REGISTRY_HARDWARE );
		if ( res1 != ERROR_SUCCESS )
		{
			SetupDiDestroyDeviceInfoList( DeviceInfoSet );
			FreeLibrary( CfgMan );
			OpenDevNodeKey = NULL;
			return NULL;
		}
		len = MAX_NAME_PORTS;

		res1 = RegQueryValueEx(
			KeyDevice,															// handle of key to query
			"portname",															// address of name of value to query
			NULL,																// reserved
			NULL,																// address of buffer for value type
			(LPBYTE) DevName,													// address of data buffer
			&len																// address of data buffer size
		);
		if ( res1 != ERROR_SUCCESS )
		{
			RegCloseKey( ERROR_SUCCESS );
			FreeLibrary( CfgMan );
			OpenDevNodeKey = NULL;
			return NULL;
		}
		if ( !_stricmp( DevName, portName ) )
		{
			FreeLibrary( CfgMan );
			OpenDevNodeKey = NULL;
			return KeyDevice;
		}
		numDev++;
	}
}

*/

//	Return the number of serial ports, and their COM port numbers.

int findserialports( int ports[] )
{
	FILE	*z = NULL;
	char	buf[ 200 ], *p;
	int		port = 0, numports = 0;

	system( "mode >x.txt" );
	if ( ( z = fopen( "x.txt", "rt" ) ) != NULL )
	{
		while ( fgets( buf, sizeof( buf ), z ) != NULL )
		{
			if ( ( p = strstr( buf, "device COM" ) ) != NULL )
			{
				port = atoi( p + 10 );
				ports[ numports ++ ] = port;
			}
		}
		fclose( z );
		_unlink( "x.txt" );
	}
	return( numports );
}


//	10/28/2022-- this function returns one of the fields associated with a serial port:
//	field can be any of the first headings, letter case doesn't matter:
//	The file consists of groups of these sections, one for each discovered device.

//	__GENUS: 2
//	__CLASS : Win32_PnPEntity
//	__SUPERCLASS : CIM_LogicalDevice
//	__DYNASTY : CIM_ManagedSystemElement
//	__RELPATH : Win32_PnPEntity.DeviceID = "FTDIBUS\\VID_0403+PID_6015+D30AOLGYA\\0000"
//	__PROPERTY_COUNT : 26
//	__DERIVATION : {CIM_LogicalDevice, CIM_LogicalElement, CIM_ManagedSystemElement}
//	__SERVER: SCOTT - PC
//	__NAMESPACE : root\cimv2
//	__PATH : \\SCOTT - PC\root\cimv2:Win32_PnPEntity.DeviceID = "FTDIBUS\\VID_0403+PID_6015+D30AOLGYA\\0000"
//	Availability :
//	Caption : USB Serial Port( COM5 )
//	ClassGuid : {4d36e978 - e325 - 11ce - bfc1 - 08002be10318}
//	CompatibleID:
//	ConfigManagerErrorCode: 0
//	ConfigManagerUserConfig : False
//	CreationClassName : Win32_PnPEntity
//	Description : USB Serial Port
//	DeviceID : FTDIBUS\VID_0403 + PID_6015 + D30AOLGYA\0000
//	ErrorCleared                :
//	ErrorDescription:
//	HardwareID: { FTDIBUS\COMPORT &VID_0403 &PID_6015 }
//	InstallDate:
//	LastErrorCode:
//	Manufacturer: FTDI
//	Name : USB Serial Port( COM5 )
//	PNPClass : Ports
//	PNPDeviceID : FTDIBUS\VID_0403 + PID_6015 + D30AOLGYA\0000
//	PowerManagementCapabilities :
//	PowerManagementSupported:
//	Present: True
//	Service : FTSER2K
//	Status : OK
//	StatusInfo :
//	SystemCreationClassName: Win32_ComputerSystem
//	SystemName : SCOTT - PC
//	PSComputerName : SCOTT - PC

//	field is the text of the field you're interested in.  Any of the headings above, "classguid" for example.
//	comport is the port number of the field to return.  5 = COM5 for example.
//	The function then returns a pointer to this specified field's value.
//	This function overwrites/erases x.txt!

const char *getportinfo( const char *field, int comport )
{
	FILE *f;
	char		buf[ 500 ];
	static char	rbuf[ 500 ] = "";
	char *p;
	bool		inrec = false;
	int			port = 0, i = 0;

	system( "powershell -WindowStyle Normal -command get-wmiobject win32_pnpentity >x.txt" );

	if ( ( f = fopen( "x.txt", "rt" ) ) != NULL && !setvbuf( f, NULL, _IOFBF, 30000 ) )
	{
		while ( fgets( buf, sizeof( buf ), f ) != NULL )
		{
			++ i;
			if ( ( p = strchr( buf, '\n' ) ) != 0 ) *p = 0;						//	delimit lines

			if ( *buf )
			{
				//	Not a blank line

				if ( strstr( buf, "Caption" ) == buf )
				{
					//	The caption line shows the COM port number

					if ( ( p = strstr( buf, "COM" ) ) != NULL && sscanf( p + 3, "%d", &port ) == 1 && port > 0 && port < 256 )
					{
						//	This is a valid caption line

						if ( port == comport )
						{
							//	This record belongs to the port we've been asked to process

							inrec = true;

							if ( *rbuf )
							{
								//	If the specified search field preceeds the Caption line,
								//	rbuf is already set to its value, close the file and return
								//	the requested info.  Otherwise continue reading with inrec
								//	true until we find the requested heading.
found:
								fclose( f );
								_unlink( "x.txt" );
								return( rbuf );
							}
						}
					}
				}

				if ( stristr( buf, (char *) field ) == buf )
				{
					//	Save the sought field

					if ( ( p = strchr( buf, ':' ) ) != NULL )
					{
						strcpy( rbuf, p + 2 );									//	copy the string value

						//	If this line contains the requested heading, and we're in the
						//	record for the requested COM port, then the heading appears
						//	after the 'Caption' line, and we're done otherwise wait for the
						//	caption line to know this value applies to the correct port.

						if ( inrec ) goto found;
					}
				}
			}
			else
			{
				//	A blank line signals a new record.

				inrec = false;													//	we're now in a different record
				*rbuf = 0;														//	clear any saved value
				port = 0;														//	and the active COM port
			}
		}	//	while fgets()

		fclose( f );
		_unlink( "x.txt" );
		return( NULL );															//	didn't find the key for comport
	}	//	output file found

	printf( "Cannot run powershell\n" );
	return( NULL );
}




//	Return true if COM<comnumber> is an attached serial port.

bool isaserialport( int comnumber )
{
	int		portlist[ 256 ], i, j;

	if ( ( i = findserialports( portlist ) ) > 0 )
	{
		for ( j = 0; j < i; j ++ ) if ( comnumber == portlist[ j ] ) return( true );
	}
	else
		TRACE( (char *) "getserialports failed\n" );

	return( false );
}