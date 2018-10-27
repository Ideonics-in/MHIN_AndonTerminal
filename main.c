
/*
*------------------------------------------------------------------------------
* main.c
*
* main application specific module.
*
* (C)2008 Sam's Logic.
*
* The copyright notice above does not evidence any
* actual or intended publication of such source code.
*
*------------------------------------------------------------------------------
*/

/*
*------------------------------------------------------------------------------
* File				: main.c
* Created by		: Sam
* Last changed by	: Sam
* Last changed		: 07/07/2009
*------------------------------------------------------------------------------
*
* Revision 0.0 07/07/2009 Sam
* Initial revision
*
*------------------------------------------------------------------------------
*/

/*
*------------------------------------------------------------------------------
* Include Files
*------------------------------------------------------------------------------
*/

#include "board.h"
#include <timers.h>			// Timer library functions
#include <delays.h>			// Dely library functions
#include <string.h>			// String library functions
#include "timer.h"			// Timer related functions
#include "uart.h"
#include "lcd.h"
#include "keypad.h"
#include "ui.h"
#include "communication.h"
#include "heartbeat.h"
#include "interruptvector.h"
#include "app.h"
#include "eep.h"
/*
*------------------------------------------------------------------------------
* Private Defines
*------------------------------------------------------------------------------
*/

/*
*------------------------------------------------------------------------------
* Processor config bits
*------------------------------------------------------------------------------
*/

#pragma config FOSC 	= INTIO67
#pragma config PLLCFG 	= OFF
#pragma config PRICLKEN = OFF
#pragma config FCMEN 	= OFF
#pragma config IESO 	= OFF
#pragma config PWRTEN   = OFF
#pragma config BOREN 	= OFF
#pragma config BORV 	= 285
#pragma config WDTEN    = OFF
#pragma config WDTPS 	= 512
#pragma config CCP2MX 	= PORTC1
#pragma config PBADEN 	= OFF

#pragma config MCLRE 	= EXTMCLR
#pragma config STVREN 	= OFF
#pragma config XINST 	= OFF
#pragma config LVP      = OFF
#pragma config CP0      = OFF
#pragma config CP1      = OFF
#pragma config CP2      = OFF
#pragma config CP3      = OFF
#pragma config CPB      = OFF
#pragma config CPD      = OFF
#pragma config WRT0     = OFF
#pragma config WRT1     = OFF
#pragma config WRT2     = OFF
#pragma config WRT3     = OFF
#pragma config WRTB     = OFF       
#pragma config WRTC     = OFF
#pragma config WRTD     = OFF
#pragma config EBTR0    = OFF
#pragma config EBTR1    = OFF
#pragma config EBTR2    = OFF
#pragma config EBTR3    = OFF
#pragma config EBTRB    = OFF


/*
*------------------------------------------------------------------------------
* Private Macros
*------------------------------------------------------------------------------
*/

/*
*------------------------------------------------------------------------------
* Private Data Types
*------------------------------------------------------------------------------
*/

/*
*------------------------------------------------------------------------------
* Public Variables
*------------------------------------------------------------------------------
*/

/*
*------------------------------------------------------------------------------
* Private Variables (static)
*------------------------------------------------------------------------------
*/

/*
*------------------------------------------------------------------------------
* Public Constants
*------------------------------------------------------------------------------
*/

/*
*------------------------------------------------------------------------------
* Private Constants (static)
*------------------------------------------------------------------------------
*/
enum
{
	TIMER0_RELOAD = (65536-20000)
};
/*
*------------------------------------------------------------------------------
* Private Function Prototypes (static)
*------------------------------------------------------------------------------
*/

/*
*------------------------------------------------------------------------------
* Public Functions
*------------------------------------------------------------------------------
*/

#define EEPROM_MAXCODE_INDEX (0x3F0)
#define EEPROM_MAXSTATION_INDEX (0x3F1)
/*
*------------------------------------------------------------------------------
* void main(void)

* Summary	: Application specifc main routine. Initializes all port and
*			: pheriperal and put the main task into an infinite loop.
*
* Input		: None
*
* Output	: None
*
*------------------------------------------------------------------------------
*/
void main(void)
{
	UINT8 i;
#ifdef KEYPAD_TEST
	UINT8 scancode, duration;
#endif

	UINT8 maxCode = 15, maxStation = 15;

	BOARD_init();
	TIMER0_init(TIMER0_RELOAD,0);
		
	LCD_init();


#ifdef TEST_LCD
	for(i = 0 ; i< 26; i++)
	{
		LCD_putChar(i+'A');
		DelayMs(200);
	}
	for(i = 0 ; i< 6; i++)
	{
		LCD_putChar(i+'0');
		DelayMs(200);
	}
	LCD_clear();
#endif


#ifdef TOWER_LAMP_TEST
	LAMP_GREEN = 1;
	DelayMs(1000);
	LAMP_GREEN = 0;

	LAMP_YELLOW = 1;
	DelayMs(1000);
	LAMP_YELLOW = 0;

	LAMP_RED = 1;
	DelayMs(1000);
	LAMP_RED = 0;


	BUZZER = 1;
	DelayMs(1000);
	BUZZER = 0;

#endif

	maxCode = Read_b_eep(EEPROM_MAX_CODES_INDEX);
	Busy_eep();
	ClrWdt();

	maxStation = Read_b_eep(EEPROM_MAX_STATIONS_INDEX);
	Busy_eep();
	ClrWdt();

	KEYPAD_init();
	


	UI_init(maxCode,maxStation);			

	APP_init();


#ifdef __UART_TEST__
	COM_txStr("IDEONICS ANDON TERMINAL ");
#endif	


	EnableInterrupts();

	//DelayMs(3000);
		

   	while(TRUE)
    {


		if( heartBeatCount >= 100 )
		{
			HB_task();
			heartBeatCount = 0;
		}


		if( keypadUpdateCount >= 8 )
		{
			KEYPAD_task();
			keypadUpdateCount = 0;
		}

		if( comUpdateCount >= 2 )
		{
			COM_task();
			comUpdateCount = 0;
		}

		//ClrWdt();

	if( appUpdateCount >= 250 )
	{
		APP_task();
		appUpdateCount = 0;
	}
	//ClrWdt();
	


#ifdef KEYPAD_TEST
		if( KEYPAD_read(&scancode, &duration) == TRUE )
			LCD_putChar(scancode);
#else
	if( uiUpdateCount >=40 )
	{
		
		UI_task();
		uiUpdateCount = 0;
	}
#endif


		ClrWdt();
				
    }
}




/*
*  End of main.c
*/