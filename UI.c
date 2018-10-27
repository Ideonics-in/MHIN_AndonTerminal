#include "board.h"
#include "config.h"
#include "Keypad.h"
#include "lcd.h"
#include "string.h"
#include "ui.h"
#include "app.h"
#include "stdlib.h"


typedef struct _UI
{
	UI_STATE state;
	UI_STATE prevState;
	UINT8 buffer[MAX_INPUT_CHARS+1];
	UINT8 bufferIndex;
	UINT8 prevcode;
	UINT8 keyIndex;
	UINT8 input[MAX_INPUT_CHARS+1];
	UINT8 inputIndex;
	UINT8 messageTimeout;
	UINT8 maxCode;
	UINT8 maxStation;
}UI;

const rom UINT8 *UI_WAITING_MSG_PREFIX[]=
{
	"RAISING ISSUE   ",
	"ACKNOWLEDGING   ",
	"RESOLVING       ",
	"CLEARING ISSUE  ",
	"CLEARING ISSUES "
};

const rom UINT8 *UI_VERIFY_MSG_PREFIX[]=
{
	"ISSUE STILL OPEN",
	"CODE INVALID    ",
	"STATION INVALID "
};

const rom UINT8 *UI_MAX_MSG_PREFIX[]=
{
	"MAX STATIONS:",
	"MAX CODES:",

};


UINT8 WaitingMessage[35];

const rom UINT8 *UI_MSG[]=
		{"STATION:",
		"CODE:",
		"PASSWORD:",
		"ADMIN ACTIVITY:",
		"CLEAR ISSUES",
		"PLEASE WAIT",
		"PLEASE VERIFY"

		};


const rom UINT8 keyMap[MAX_KEYS ][MAX_CHAR_PER_KEY] = 
{
	{'1','-','1','-','1'},{'2','A','B','C','2'},{'3','D','E','F','3'},{'\x0B','\x0B','\x0B','\x0B','\x0B'},
	{'4','G','H','I','4'},{'5','J','K','L','5'},{'6','M','N','O','6'},{'\x0C','\x0C','\x0C','\x0C','\x0C'},
	{'7','P','Q','R','S'},{'8','T','U','V','8'},{'9','W','X','Y','Z'},{'\x08','\x08','\x08','\x08','\x08'},
	{'*','*','*','*','*'},{'0',' ','0',' ','0'},{'\x13','\x13','\x13','\x13','\x13'},{'\x0A','\x0A','\x0A','\x0A','\x0A'}
};




#pragma idata UI_DATA
UI ui = {0,0,{0},0,0xFF,0,0};
OpenIssue openIssue={{0},-1};
OpenIssue ackIssue={{0},-1};
//#pragma idata



UINT8 mapKey(UINT8 scancode, UINT8 duration);
UINT8 getStation(void);
void getData(void);
UINT8 getCode(void);
void clearUIBuffer(void);
void putUImsg(UINT8 msgIndex);
void setUImsg( UINT8 msgIndex );
void clearUIInput(void);
void showUImsg( UINT8* msg );


void UI_init(UINT8 maxCode, UINT8 maxStation)
{

	LCD_setBackSpace('\x08');	//Indicates LCD driver "\x08" is the symbol for backspace

	setUImsg(UI_MSG_STATION);

	ui.maxCode = maxCode;
	ui.maxStation = maxStation;

	clearUIBuffer();
	clearUIInput();
}


UI_STATE UI_GetState()
{
	return ui.state;
}


void UI_task(void)
{

	UINT8 keypressed = 0xFF;
	UINT8 i;
	UINT8 duration, scancode;
	UINT8 uimsg;

	if(ui.state == UI_VERIFY)
	{
		--ui.messageTimeout;
		if( ui.messageTimeout == 0)
		{


			setUImsg(UI_MSG_STATION);
			clearUIBuffer();
			clearUIInput();
			ui.state = UI_STATION;
				
			
		}
		return;
	}

	if(KEYPAD_read(&scancode, &duration) == FALSE)			//Check whether key has been pressed
	{
		return;
	}

	
	keypressed = mapKey(scancode,duration);				//Map the key

	if( keypressed == 0xFF)
	{
		return;
	}


	switch(ui.state)
	{
		case UI_STATION:

		if( keypressed == '\x08')
		{
			if(ui.bufferIndex > 0 )
			{
				LCD_putChar(keypressed);
				ui.bufferIndex--;
				if( ui.inputIndex > 0 )
					ui.inputIndex--;
			}

		}

		else if( keypressed == '\x0A')
		{
			UINT8 stn = 0;
			if(ui.bufferIndex > 0)
			{
				stn = getStation();
				if((stn > ui.maxStation) || (stn < 1) )		//check for station validity
				{
					strcpypgm2ram(WaitingMessage,UI_VERIFY_MSG_PREFIX[2]);
					strcatpgm2ram(WaitingMessage,UI_MSG[UI_MSG_VERIFY]);
					clearUIBuffer();
					clearUIInput();
					ui.messageTimeout = 10;
					showUImsg(WaitingMessage);
					ui.state = UI_VERIFY;
				}
				else
				{
					setUImsg(UI_MSG_ISSUE);
					clearUIBuffer();
					ui.state = UI_ISSUE;
				}
				

			}
		}

		else if( keypressed == '\x13')
		{


			setUImsg(UI_MSG_PASSWORD);

			clearUIBuffer();

			ui.state = UI_PASSWORD;
				
			
		}

		else if( keypressed == '\x0C')
		{
			APP_getAcknowledgedIssue(&ackIssue);
			if(ackIssue.ID != - 1)
			{
				showUImsg(ackIssue.tag);
				clearUIBuffer();
				clearUIInput();
				ui.state= UI_ISSUE_RESOLVE;
			}
		}	

		else if( keypressed == '\x0B')
		{
			APP_getOpenIssue(&openIssue);
			if(openIssue.ID != - 1)
			{
				showUImsg(openIssue.tag);
				clearUIBuffer();
				clearUIInput();
				ui.state= UI_ISSUE_ACK;
			}
		}		
		else
		{
			ui.buffer[ui.bufferIndex] = keypressed;
			LCD_putChar(ui.buffer[ui.bufferIndex]);
			ui.bufferIndex++;
		}

		break;

		case UI_ISSUE:

		if( keypressed == '\x08')
		{
			if(ui.bufferIndex > 0 )
			{
				LCD_putChar(keypressed);
				ui.bufferIndex--;

			}
		
			else
			{
				
				setUImsg(UI_MSG_STATION);
				clearUIBuffer();
				clearUIInput();
				ui.state = UI_STATION;
			}

		}

		else if( keypressed == '\x0A')
		{
			UINT8 code= 0;
			if(ui.bufferIndex > 0)
			{
				code = getCode();
				if((code > ui.maxCode) || (code < 1) )		//check for code validity
				{
					strcpypgm2ram(WaitingMessage,UI_VERIFY_MSG_PREFIX[1]);
					strcatpgm2ram(WaitingMessage,UI_MSG[UI_MSG_VERIFY]);
					clearUIBuffer();
					clearUIInput();
					ui.messageTimeout = 10;
					showUImsg(WaitingMessage);
					ui.state = UI_VERIFY;
				}
				else
				{
					getStation();
					ui.input[ui.inputIndex] = '\0';
					if(!APP_raiseIssues( ui.input))
					{
						strcpypgm2ram(WaitingMessage,UI_VERIFY_MSG_PREFIX[0]);
						strcatpgm2ram(WaitingMessage,UI_MSG[UI_MSG_VERIFY]);
						clearUIBuffer();
						clearUIInput();
						ui.messageTimeout = 10;
						showUImsg(WaitingMessage);
						ui.state = UI_VERIFY;
						return;
					}
					
					
					strcpypgm2ram(WaitingMessage,UI_WAITING_MSG_PREFIX[0]);
					strcatpgm2ram(WaitingMessage,UI_MSG[UI_MSG_WAITING]);
	
					showUImsg(WaitingMessage);
					clearUIBuffer();
					clearUIInput();
					ui.state = UI_WAITING;
					ui.prevcode = 0xFF;
					ui.keyIndex = 0;
				}
			}
		}
		else
		{
			ui.buffer[ui.bufferIndex] = keypressed;
			LCD_putChar(ui.buffer[ui.bufferIndex]);
			ui.bufferIndex++;
		}

		
		break;


		case UI_ISSUE_ACK:

		if( keypressed == '\x08')
		{
			setUImsg(UI_MSG_STATION);
			clearUIBuffer();
			clearUIInput();
			ui.state = UI_STATION;
		}

		else if( keypressed == '\x0B')
		{
			APP_getOpenIssue(&openIssue);
			if( openIssue.ID != -1 )
			{
				showUImsg(openIssue.tag);
			}

		}
		else if( keypressed == '\x0A')
		{
			APP_acknowledgeIssues(openIssue.ID);
			openIssue.ID = -1;
			strcpypgm2ram(WaitingMessage,UI_WAITING_MSG_PREFIX[1]);
			strcatpgm2ram(WaitingMessage,UI_MSG[UI_MSG_WAITING]);

			showUImsg(WaitingMessage);	
			clearUIBuffer();
			clearUIInput();
			ui.state = UI_WAITING;

		}

		break;



	case UI_ISSUE_RESOLVE:

		if( keypressed == '\x08')
		{
			setUImsg(UI_MSG_STATION);
			clearUIBuffer();
			clearUIInput();
			ui.state = UI_STATION;
		}

		else if( keypressed == '\x0C')
		{
			APP_getAcknowledgedIssue(&ackIssue);
			if( ackIssue.ID != -1 )
			{
				showUImsg(ackIssue.tag);
			}

		}
		else if( keypressed == '\x0A')
		{
			APP_resolveIssues(ackIssue.ID);
			ackIssue.ID = -1;
			
			strcpypgm2ram(WaitingMessage,UI_WAITING_MSG_PREFIX[2]);
			strcatpgm2ram(WaitingMessage,UI_MSG[UI_MSG_WAITING]);
			showUImsg(WaitingMessage);

			clearUIBuffer();
			clearUIInput();
			ui.state = UI_WAITING;

		}

		break;

	

		case UI_CLEAR_ISSUE:

		if( keypressed == '\x08')
		{
			setUImsg(UI_MSG_STATION);
			clearUIBuffer();
			ui.state = UI_STATION;

		}

		else if( keypressed == '\x0A')
		{
			APP_clearIssues();
			clearUIBuffer();
			clearUIInput();
			UI_setState(UI_STATION);
		}

		break;

		


		case UI_PASSWORD:
		if( keypressed == '\x08')
		{
		
				setUImsg(UI_MSG_STATION);
				clearUIBuffer();
				clearUIInput();
				ui.state = UI_STATION;
		
		

		}

		else if( keypressed == '\x0A')
		{
			BOOL result = FALSE;
			ui.buffer[ui.bufferIndex] = '\0';
	
		
			 
			result = APP_checkPassword(ui.buffer);	
			if( result == TRUE )
			{
				setUImsg(UI_MSG_ADMIN_ACTIVITY);
				clearUIBuffer();
				clearUIInput();
				ui.state = UI_ADMIN_ACTIVITY;
			}
			else
			{
				setUImsg(UI_MSG_STATION);
				clearUIBuffer();
				clearUIInput();
				ui.state = UI_STATION;
			}
		
		
		}

		else 
		{
			ui.buffer[ui.bufferIndex] = keypressed;
			LCD_putChar('*');
			ui.bufferIndex++;
		}
			
		break;		

		case UI_ADMIN_ACTIVITY:

		if( keypressed == '\x08')
		{
			setUImsg(UI_MSG_STATION);
			clearUIBuffer();
			clearUIInput();
			ui.state = UI_STATION;
			
		}

		else if( keypressed == '0')
		{
			setUImsg(UI_MSG_CLEAR_ISSUES);
			clearUIBuffer();
			clearUIInput();
			ui.state = UI_CLEAR_ISSUE;
		}

		else if( keypressed == '1')
		{
			char tmp[5];
			strcpypgm2ram(WaitingMessage,UI_MAX_MSG_PREFIX[0]);
			strcat(WaitingMessage,itoa(ui.maxStation,tmp));
			showUImsg(WaitingMessage);
			LCD_SetCursor(1,13);
		
			clearUIBuffer();
			clearUIInput();
			ui.state = UI_SET_MAX_STATIONS;
		}


		else if( keypressed == '2')
		{
			char tmp[5];
			strcpypgm2ram(WaitingMessage,UI_MAX_MSG_PREFIX[1]);
			strcat(WaitingMessage,itoa(ui.maxCode,tmp));
			showUImsg(WaitingMessage);
			LCD_SetCursor(1,10);

			clearUIBuffer();
			clearUIInput();
			ui.state = UI_SET_MAX_CODES;
		}


		break;


		case UI_SET_MAX_STATIONS:

		if( keypressed == '\x08')
		{
			if(ui.bufferIndex > 0 )
			{
				LCD_putChar(keypressed);
				ui.bufferIndex--;

			}
		
			else
			{
				
				setUImsg(UI_MSG_STATION);
				clearUIBuffer();
				clearUIInput();
				ui.state = UI_STATION;
			}


		}

		else if( keypressed == '\x0A')
		{
			UINT8 stations = 0;
			if(ui.bufferIndex > 0)
			{
				stations = getStation();		
				APP_SetMaxStations(stations);
				ui.maxStation = stations;
				setUImsg(UI_MSG_STATION);
				clearUIBuffer();
				clearUIInput();
				ui.state = UI_STATION;
			}
				

			
		}

		else
		{
			ui.buffer[ui.bufferIndex] = keypressed;
			LCD_putChar(ui.buffer[ui.bufferIndex]);
			ui.bufferIndex++;
		}

		break;


		case UI_SET_MAX_CODES:

		if( keypressed == '\x08')
		{
			if(ui.bufferIndex > 0 )
			{
				LCD_putChar(keypressed);
				ui.bufferIndex--;

			}
		
			else
			{
				
				setUImsg(UI_MSG_STATION);
				clearUIBuffer();
				clearUIInput();
				ui.state = UI_STATION;
			}


		}

		else if( keypressed == '\x0A')
		{
			UINT8 codes = 0;
			if(ui.bufferIndex > 0)
			{
				codes = getStation();		
				APP_SetMaxCodes(codes);
				ui.maxCode = codes;
				setUImsg(UI_MSG_STATION);
				clearUIBuffer();
				clearUIInput();
				ui.state = UI_STATION;
			}
				

			
		}

		else
		{
			ui.buffer[ui.bufferIndex] = keypressed;
			LCD_putChar(ui.buffer[ui.bufferIndex]);
			ui.bufferIndex++;
		}

		break;


		case UI_WAITING:
		if( keypressed == '\x13')
		{


			setUImsg(UI_MSG_PASSWORD);

			clearUIBuffer();

			ui.state = UI_PASSWORD;
				
			
		}

		break;
	

		default:
		break;


	}



}


UINT8 mapKey(UINT8 scancode, UINT8 duration)
{
	UINT8 keypressed = 0xFF;
	switch(ui.state)
	{

		case UI_STATION:
		keypressed = keyMap[scancode][0];
		
		if( (ui.bufferIndex >=2 ))
		{
		
			if( (keypressed != '\x08') && (keypressed !='\x0A') )
				keypressed = 0xFF;
		}
		else if( ui.bufferIndex > 0 && ui.bufferIndex )
		{
			if( ( keypressed == '*') ||(keypressed =='\x0B')|| (keypressed =='\x0C')
			|| (keypressed =='\x13'))
				keypressed = 0xFF;
		}
		else 
		{
			if( keypressed == '*') 
				keypressed = 0xFF;
		}

		break;




	

		case UI_ISSUE:

		keypressed = keyMap[scancode][0];

		if( (keypressed != '1') && 
			(keypressed != '2') && 
			(keypressed != '3') && 
			(keypressed != '4') && 
			(keypressed != '5') &&
			(keypressed != '6') &&
			(keypressed != '7') && 
			(keypressed != '8') &&
			(keypressed != '9') &&
			(keypressed != '0') &&
			(keypressed != '\x08')&& 
			(keypressed !='\x0A'))
			
			keypressed = 0xFF;

		break;

	


	
		case UI_ISSUE_ACK:

		keypressed = keyMap[scancode][0];

		if( (keypressed != '\x0A') && (keypressed != '\x08') && (keypressed != '\x0B') )
		{
			keypressed = 0xFF;
		}

		break;



		case UI_ISSUE_RESOLVE:

		keypressed = keyMap[scancode][0];

		if( (keypressed != '\x0A') && (keypressed != '\x08') && (keypressed != '\x0C') )
		{
			keypressed = 0xFF;
		}

		break;





		case UI_PASSWORD:
			keypressed = keyMap[scancode][0];
		break;



		case UI_CLEAR_ISSUE:
		keypressed = keyMap[scancode][0];

		if( (keypressed != '\x0A') && (keypressed != '\x08') )
		{
			keypressed = 0xFF;
		}

		break;

		case UI_ADMIN_ACTIVITY:
		keypressed  = keyMap[scancode][0];

		if( (keypressed != '0') && (keypressed != '1') && (keypressed != '2') && (keypressed !='\x08') && (keypressed != '\x0A') )
			keypressed = 0xFF;
		break;



		case UI_SET_MAX_STATIONS:
		keypressed = keyMap[scancode][0];
		
		if( (ui.bufferIndex >=2 ))
		{
		
			if( (keypressed != '\x08') && (keypressed !='\x0A') )
				keypressed = 0xFF;
		}
		else if( ui.bufferIndex > 0 && ui.bufferIndex )
		{
			if( ( keypressed == '*') ||(keypressed =='\x0B')|| (keypressed =='\x0C')
			|| (keypressed =='\x13'))
				keypressed = 0xFF;
		}
		else 
		{
			if( ( keypressed == '*') ||(keypressed =='\x0B')|| (keypressed =='\x0C')
			|| (keypressed =='\x13')) 
				keypressed = 0xFF;
		}

		break;

		case UI_SET_MAX_CODES:
		keypressed = keyMap[scancode][0];
		
		if( (ui.bufferIndex >=2 ))
		{
		
			if( (keypressed != '\x08') && (keypressed !='\x0A') )
				keypressed = 0xFF;
		}
		else if( ui.bufferIndex > 0 && ui.bufferIndex )
		{
			if( ( keypressed == '*') ||(keypressed =='\x0B')|| (keypressed =='\x0C')
			|| (keypressed =='\x13'))
				keypressed = 0xFF;
		}
		else 
		{
			if( ( keypressed == '*') ||(keypressed =='\x0B')|| (keypressed =='\x0C')
			|| (keypressed =='\x13')) 
				keypressed = 0xFF;
		}

		break;



		case UI_WAITING:
		
		keypressed = keyMap[scancode][0];

		if( (keypressed != '\x13'))
		{
			keypressed = 0xFF;
		}
		break;

		default:
		break;

	}

	return keypressed;
}

void UI_setState( UI_STATE state)
{
	switch( state)
	{
		case UI_STATION:
			
			openIssue.ID = -1;
			ackIssue.ID = -1;
			setUImsg(UI_MSG_STATION);
			clearUIBuffer();
			clearUIInput();
			ui.state = UI_STATION;
			ui.prevcode = 0xFF;
			ui.keyIndex = 0;

		break;




	
		case UI_ISSUE:
			
			setUImsg(UI_MSG_ISSUE);
			clearUIBuffer();
			clearUIInput();
			ui.state = UI_ISSUE;

		break;




		default:
		break;
	}
}



void UI_setWaitingState( UI_WAITING_STATE state)
{
	strcpypgm2ram(WaitingMessage,UI_WAITING_MSG_PREFIX[0]);
	strcatpgm2ram(WaitingMessage,UI_MSG[UI_MSG_WAITING]);

	showUImsg(WaitingMessage);
	clearUIBuffer();
	clearUIInput();
	ui.state = UI_WAITING;
	ui.prevcode = 0xFF;
	ui.keyIndex = 0;
}


UINT8 getStation(void)
{
	UINT8 i,station = 0;

	if( ui.bufferIndex == 1 )
	{
		ui.input[ui.inputIndex] = '0';
		ui.inputIndex++;
		ui.input[ui.inputIndex] = ui.buffer[0];
		ui.inputIndex++;
	}

	else
	{
		ui.input[ui.inputIndex] = ui.buffer[0];
		ui.inputIndex++;
		ui.input[ui.inputIndex] = ui.buffer[1];
		ui.inputIndex++;
	}

	station = (ui.input[0]-'0')*10 + (ui.input[1]-'0');

	return station;
}

UINT8 getCode(void)
{
	UINT8 i,code = 0;

	if( ui.bufferIndex == 1 )
	{
		code =  ui.buffer[0] - '0';
	}

	else
	{
		code = (ui.buffer[0]-'0')*10 + (ui.buffer[1]-'0');
	}



	return code;
}




void getData(void)
{
	UINT8 i;

	for( i = 0; i< ui.bufferIndex; i++)
	{
		ui.input[ui.inputIndex] = ui.buffer[i];
		ui.inputIndex++;
		
	}
	ui.input[ui.inputIndex] = '\0';
	ui.inputIndex++;

	if( ui.inputIndex >= MAX_INPUT_CHARS )
		ui.inputIndex = 0;
}


void clearUIBuffer(void)
{
	memset(ui.buffer,0, MAX_INPUT_CHARS);
	ui.bufferIndex = 0;
	ui.keyIndex = 0;
	ui.prevcode = 0xFF;

}


void clearUIInput(void)
{
	memset((UINT8*)ui.input,0, MAX_INPUT_CHARS);
	ui.inputIndex = 0;
}




void showUImsg( UINT8* msg )
{
	UINT8 i;

	
	LCD_clear();

	i = 0;
	while( msg[i] != '\0')
	{
		LCD_putChar(msg[i]);
		i++;
	}
}




void setUImsg( UINT8 msgIndex )
{
	UINT8 i;

	const rom UINT8 *msg;

	msg = UI_MSG[msgIndex] ;

	LCD_clear();

	i = 0;
	while( msg[i] != '\0')
	{
		LCD_putChar(msg[i]);
		i++;
	}
}


void putUImsg(UINT8 msgIndex)
{
	UINT8 i;

	const rom UINT8 *msg;

	msg = UI_MSG[msgIndex] ;

	i = 0;
	while( msg[i] != '\0')
	{
		LCD_putChar(msg[i]);
		i++;
	}
}


		