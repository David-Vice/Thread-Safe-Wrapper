#pragma once
#include "optel_tinyg_dll.h"


#ifdef __cplusplus
extern "C" {
#endif

#define	MM			( 4 )														//	# motors supported

#ifdef EXPORTING_DLL
	extern __declspec( dllexport ) double  tg_version( void );						//	return DLL version as x.xxx
//	extern __declspec( dllexport ) const char *tg_mname[ MM ];						//	motor names
	extern __declspec( dllexport ) bool tg_getpos( double pos[ MM ] );				//	retrieve current motor positions
	extern __declspec( dllexport ) bool tg_home( bool home[ MM ], int tosec );		//	home specified motors, wait up to tosec seconds
	extern __declspec( dllexport ) bool tg_move( bool move[ MM ], double pos[ MM ], int tosec );	//	move named motors to pos, wait up to tosec
	extern __declspec( dllexport ) bool tg_getranges( tg_range_t mrange[ MM ] );	//	retrieve all motor ranges
	extern __declspec( dllexport ) void tg_comm( char *msg );						//	activate interactive communication mode (w/TinyG)
	extern __declspec( dllexport ) BOOL tg_open_ports();						    //	open ports
	extern __declspec( dllexport ) void tg_close_ports();						    //	close ports

#else
	extern __declspec( dllimport ) double  tg_version( void );						//	return DLL version as x.xxx
	extern __declspec( dllimport ) const char *tg_mname[ MM ];						//	motor names
	extern __declspec( dllimport ) bool tg_getpos( double pos[ MM ] );				//	retrieve current motor positions
	extern __declspec( dllimport ) bool tg_home( bool home[ MM ], int tosec );		//	home specified motors, wait up to tosec seconds
	extern __declspec( dllimport ) bool tg_move( bool move[ MM ], double pos[ MM ], int tosec );	//	move named motors to pos, wait up to tosec
	extern __declspec( dllimport ) bool tg_getranges( tg_range_t mrange[ MM ] );	//	retrieve all motor ranges
	extern __declspec( dllimport ) void tg_comm( char *msg );						//	activate interactive communication mode (w/TinyG)
	extern __declspec( dllimport ) BOOL tg_open_ports();						    //	open ports
	extern __declspec( dllimport ) void tg_close_ports();						    //	close ports
#endif

#ifdef	__cplusplus
}
#endif

