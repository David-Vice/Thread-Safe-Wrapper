/*
   ====================================================================
   IBM-PC/XT/AT Function key codes as returned by bioskey().
   ====================================================================
   Rev    Date      By     Description
   -----  --------  -----  --------------------------------------------
		  5/7/20    SRG    Added F11, F12, AF11, AF12, SF11, SF12 and
							CAF1-CAF12.
   -----  --------  -----  --------------------------------------------
		  02/14/08  SRG    Added shifted PgUp and PgDn codes.
   -----  --------  -----  --------------------------------------------
		  10/30/00  DK/sg  Validated OK per SVP 02120_A
   -----  --------  -----  --------------------------------------------
   2.0    1/9/99    SRG    Added common ASCII keys, verified by def.
   -----  --------  -----  --------------------------------------------
   1.0    3/24/98   RJG/sg Verified OK.
   -----  --------  -----  --------------------------------------------
		  06/26/92  SRG    Added Ctrl-F1 to F10.
   -----  --------  -----  --------------------------------------------
		  04/08/92  SRG    Added a few more.
   -----  --------  -----  --------------------------------------------
		  12/19/1987 SRG   Original.
   --------------------------------------------------------------------
   ====================================================================
*/


#define UA            0x4800  /* up */
#define LA            0x4B00  /* left */
#define CLA           0x7300  /* ctrl-left */
#define RA            0x4D00  /* right */
#define CRA           0x7400  /* ctrl-right */
#define DA            0x5000  /* down */
#define PU            0x4900  /* page up */
#define PD            0x5100  /* page down */
#define END           0x4F00  /* end */
#define INS           0x5200  /* insert */
#define DEL           0x5300  /* delete */
#define HOME          0x4700  /* home */
#define CHOME         0x7700  /* ctrl-home */
#define CEND          0x7500  /* ctrl-end */
#define CPU           0x8400  /* ctrl-PgUp */
#define CPD           0x7600  /* ctrl-PgDn */
#define SUA           0x4838  /* shift-up */
#define SLA           0x4B34  /* shift-left */
#define SRA           0x4D36  /* shift-right */
#define SDA           0x5032  /* shift-down */
#define SPU           0x4939  /* shift-PgUp */
#define SPD           0x5133  /* shift-PgDn */

#define F1            0x3B00  /* Function keys */
#define F2            0x3C00
#define F3            0x3D00
#define F4            0x3E00
#define F5            0x3F00
#define F6            0x4000
#define F7            0x4100
#define F8            0x4200
#define F9            0x4300
#define F10           0x4400
#define F11           0x5700
#define F12           0x5800


#define SF1           0x5400  /* Shifted function keys */
#define SF2           0x5500
#define SF3           0x5600
#define SF4           0x5700
#define SF5           0x5800
#define SF6           0x5900
#define SF7           0x5A00
#define SF8           0x5B00
#define SF9           0x5C00
#define SF10          0x5D00
#define SF11          0x5700
#define SF12          0x5800

#define AF1           0x6800  /* Alt Function keys */
#define AF2           0x6900
#define AF3           0x6A00
#define AF4           0x6B00
#define AF5           0x6C00
#define AF6           0x6D00
#define AF7           0x6E00
#define AF8           0x6F00
#define AF9           0x7000
#define AF10          0x7100
#define AF11          0x5700
#define AF12          0x5800

#define CF1           0x5E00  /* Control Function keys */
#define CF2           0x5F00
#define CF3           0x6000
#define CF4           0x6100
#define CF5           0x6200
#define CF6           0x6300
#define CF7           0x6400
#define CF8           0x6500
#define CF9           0x6600
#define CF10          0x6700
#define CF11          0x5700
#define CF12          0x5800

#define CAF1		0x6800			//  Control-Alt function keys
#define CAF2		0x6900			//	these don't exist as far as I can tell
#define CAF3		0x6A00
#define CAF4		0x6B00
#define CAF5		0x6C00
#define CAF6		0x6D00
#define CAF7		0x6E00
#define CAF8		0x6F00
#define CAF9		0x7000
#define CAF10		0x7100
#define CAF11		0x5700
#define CAF12		0x5800


#define AA            0x1E00  /* Alt Keys */
#define AB            0x3000
#define AC            0x2E00
#define AD            0x2000
#define AE            0x1200
#define AF            0x2100
#define AG            0x2200
#define AH            0x2300
#define AI            0x1700
#define AJ            0x2400
#define AK            0x2500
#define AL            0x2600
#define AM            0x3200
#define AN            0x3100
#define AO            0x1800
#define AP            0x1900
#define AQ            0x1000
#define AR            0x1300
#define AS            0x1F00
#define AT            0x1400
#define AU            0x1600
#define AV            0x2F00
#define AW            0x1100
#define AX            0x2D00
#define AY            0x1500
#define AZ            0x2C00

/*	11/21/19 these interfere with Windows stuff.
#define A0            0x8100
#define A1            0x7800
#define A2            0x7900
#define A3            0x8000
#define A4            0x8100
#define A5            0x8200
#define A6            0x8300
#define A7            0x8400
#define A8            0x8500
#define A9            0x8600
*/

// 1/9/99 Common ASCII codes

#define LF            0xA
#define CR            0xD
#ifndef BS
#define BS            0x8														//	also defined in ASCII.h
#endif
#define	ACK				6
#define	NAK				0x15
#define	TAB				9
#define DL            0x7F
#define ESC			  0x1B
