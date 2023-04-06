//#include <windows.h>

#undef BLOCKIO
#undef RS232_DIAGS

#define NUMCOMPORT				10							// how many simultaneously opened ports we support
#define	MAXCOMPORTNUMBER		255							// highest COM port number we'll allow
#define ERROR_BAD_PORT			10001						// bad port # error
#define ERROR_NOT_A_COM_PORT	10002						// not a communication port
#define ERROR_TOO_MANY			10003						//	Too many open ports [this is an artifical limitation based on our static structure allocations]
#define	BUSY					10004						//	we're already busy closing a port
#define	NOERROR					0
#define	PDTIMEOUT				( 50 )						//	port disconnect timeout

// Modem control signals for setcomsig() & getcomsig()

//	Output signals (controlled by setcomsig() & now retrieved by getcomsig())
//	All signals are non-inverted, "1" means high >= 5V at the pin and "0" means
//	low <= -5V at the pin.

//	Pin #s are for the 25-pin serial port, 9-pin serial port pin #s are shown in ().

#define RTS	0x2												// request to send, pin 4 (7)
#define DTR	0x1												// data terminal ready, pin 20 (4)
#define	RS232OUTSIGS	( RTS | DTR )						//	output signals

//	Input signals (retrieved by getcomsig())

#define CTS 0x10											// clear to send, pin 5 (8)
#define DSR 0x20											// data set ready, pin 6 (6)
#define RI	0x40											// RI input, pin 22 (9)
#define RLSD 0x80											// CD/RLSD input, pin 8 (1)
#define	RS232INSIGS ( CTS | DSR | RI | RLSD )				//	input signals
#define	RS232SIGS ( RS232INSIGS | RS232OUTSIGS )			//	all the signals

//extern char		*portsignames[];							//	names of RS-232 port signals in status bit order starting w/0.
//extern int		selport;									//	selected port index

int openport( int port );									//	open a manually closed port
void closeports( void );									// close all open com ports
int closeport( int port );									//	close a single port
int otherport( void );										// pick the 'next' port, nonzero on error
int portselect( int port );									// pick a specific port, nonzero on error, port 0 => COM1
int getport( void );										// return the COM port # of the currently selected port or -1
int charin( void );											// return # characters pending
int charin( int port );										// return # characters pending on a different port
void outcom( char c );										// send c -> port
void outcome( char c );										//	c -> port in full-duplex system
void outcoms( char *str );									// send string to port
unsigned long outcoms( char *str, unsigned long n );		// send n characters of a string to port
void outcoms( char *str, int pacing );						//	send str with pacing ms between characters
void outcomsf( char *str );									//	send str in a full-duplex system
void outcomblock( unsigned char *block, int blocksize );	// write a block to the port, blocking
void waitxmitrdy( void );									// wait till transmitter is ready
DWORD rstcom( void );										// reset receive & transmit
unsigned getbyte( void );									// get next character & its receive status
BOOL getcomprm( DCB *params );								// get port parameters
BOOL setcomprm( DCB *params );								// set port parameters
void pramsetup( void );										// activate port setup menu

void simplecomm( unsigned termcode, bool halfduplex, char *msg );	// simple terminal program
void simplecomma( unsigned termcode, bool halfduplex, char *msg, bool autolf );	// with autolf control
int getcomsig( void );										// get active port's MODEM status
int getcomsig( int port );									// get a named port's MODEM status
BOOL setcomsig( int newsig );								// set port's MODEM control signals
BOOL setcomsig( int port, int newstat );					// 10/21/14 set a different port's signals
int readstr( long timeout, char *s, int maxlen );			// input string s (till cr) up to maxlen chars before timeout
BOOL getnt( long timeout, char *s, int maxlen );			// input maxlen chars into s
BOOL getntx( long timeout, unsigned char *s, int maxlen );	// getnt with the parity bit not cleared
BOOL waitfor( long timeout, char *matchstring );			// wait for a string in the input stream
BOOL waitfor( char *buf, int bufsiz, long timeout, char *block, int len );			// same as waitfor except match is a binary block of data
BOOL waitfor( char *bufr, int bufsiz, long timeout, char *ack, int acklen, char *nak, int naklen );	//	with both ACK and NAK replies
BOOL sendbreak( int breakon );								// send a break for howlong ticks
BOOL sendbreak_timed( int howlong );						// here's a new easier to use function
BOOL isconnected( void );									//	true if the current port remains open
BOOL isconnected( int port );								//	true if port remains open


//	Send a command to the port, then input a response into recvbuf (up to maxlen characters incl/null terminator).
//	The response ends with an ACK (0x06) or NAK (0x15).
//	Returns true if an ACK terminated response is received within timeout milliseconds.
//	The second function calls a callback function (if it's not NULL) while waiting for receive characters
//	The third form waits for any char in delims and returns true
//	The fourth form allows controling the pacing of the outgoing message characters.
//	The fifth form prints incoming characters (except ACK/NAK) as they are received if echo is true.

BOOL cmdio( char *cmd, long timeout, char *recvbuf, int maxlen );
BOOL cmdio( char *cmd, long timeout, char *recvbuf, int maxlen, void (*callback)( void ) );
BOOL cmdio( char *cmd, long timeout, char *recvbuf, int maxlen, char *delims );
BOOL cmdio( char *cmd, long timeout, char *recvbuf, int maxlen, int pacing );
BOOL cmdio( char *cmd, long timeout, char *recvbuf, int maxlen, bool echo );
bool cmdio( char *cmd, long timeout, char *recvbuf, int maxlen, char *delims, bool clearbuf );

//	cmdio for a full-duplex system.

BOOL cmdiof( char *cmd, long timeout, char *recvbuf, int maxlen );

bool getline( char *cmd, time_t timeout, char *buf, int maxlen );

//	Check all opened com ports for receive data.
//	Returns the COM port # of received data 1..N, and updates rx with the data
//	otherwise it returns false (0).

int	anyrx( unsigned char *rx );

//	Return device capabilities in a COMMPROP structure.
//	returns 0 on success or error last error value.

int getcomprop( LPCOMMPROP p );

//	Return # of serial ports on this PC and their COM numbers.

int findserialports( int ports[] );



#ifdef	RS232_DIAGS
//	Print selected port's device capabilities (using a COMMPROP structure).
void printproperties( void );
#endif

//	Return a list of available COM ports on this PC.
//	Returns # ports and the first *nports entries
//	of comportlist are updated with their numbers.
//	If comportlist is NULL, nports is ignored and
//	the function returns the number of ports.

int getcomports( int *comportlist );
int findserialports( int ports[] );
const char *getportinfo( const char *field, int comport );						//	return info about field for comport
bool isaserialport( int comnumber );											//	true if comnumber is a serial port


//	#define BLOCKIO

#ifdef	BLOCKIO

//	Write a block of data of length size.
//	Returns 0 on error or # bytes written (which should equal size).
//	Wait until *most* of the data is transmitted.

unsigned long outblock( unsigned char *data, unsigned long size );


//	Write a block of data of length size.
//	Returns 0 on error or # bytes written (which should equal size).
//	Returns almost immediately.

unsigned long outchunk( unsigned char *data, unsigned long size );

BOOL chunksent( void );


//	Read a block of data from a port
//	Isn't compatible with charin()/getbyte().

int getchunk( int port, unsigned char *buf, int size, int bufsize );


//	Wait for all schedules transfers from all opened ports.

void waitallchunks( void );

//	Wait for new data on any of the ports getchunk was called.
//	Returns a pointer to the first unprocessed data, and sets
//	endofdata to the last byte of data + 1.
//	Returns NULL when all data on all getchunk ports is processed.
//	callfunc is a callback function that's periodically called while
//	waiting for data.

unsigned char *waitforchunks( unsigned char **endofdata, void (*callfunc)( void ) );

#endif


//	Debug stuff for now

#ifdef	RS232_DIAGS
extern clock_t			rxt[ NUMCOMPORT ][ 10000 ];
extern clock_t			*rxtp[ NUMCOMPORT ];
extern OVERLAPPED		rolap[ NUMCOMPORT ];
extern unsigned long	portpoll[ NUMCOMPORT ];
extern unsigned long	rxport[ NUMCOMPORT ];
#endif

/*
HANDLE  BeginEnumeratePorts( VOID );
BOOL EnumeratePortsNext( HANDLE DeviceInfoSet, LPTSTR lpBuffer );
BOOL  EndEnumeratePorts( HANDLE DeviceInfoSet );
HKEY GetDeviceKey( LPTSTR portName );
*/


