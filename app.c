
/*
*------------------------------------------------------------------------------
* Include Files
*------------------------------------------------------------------------------
*/
#include "stdlib.h"
#include "config.h"
#include "board.h"
#include "timer.h"
#include "communication.h"
#include "app.h"
#include "keypad.h"
#include "lcd.h"
#include <string.h>
#include "eep.h"
#include "ui.h"
#include "communication.h"

//#define SIMULATION
/*
*------------------------------------------------------------------------------
* Structures
*------------------------------------------------------------------------------
*/
typedef struct _ISSUE
{

	UINT8 data[MAX_KEYPAD_ENTRIES + 1];
	UINT8 state;
	UINT8 status;

}ISSUE;


																			//Object to store data of raised issue

typedef struct _LOG
{
	UINT8 prevIndex;
	UINT8 index;
	UINT8 entries[MAX_LOG_ENTRIES][LOG_BUFF_SIZE];
}LOG;																			//Object to store log entries





typedef struct _APP
{
	APP_STATE state;
	UINT32 preAppTime,curAppTime;
	UINT8 stationCount[2];
	UINT8 password[5];

	ISSUE issues[MAX_ISSUES];
	UINT8 currentIssue;
	UINT8 prevIssue;

	UINT8 waitingIssue;
	UINT8 openIssue;
	INT8 buzzerTimeout;
}APP;																			//This object contains all the varibles used in this application



typedef enum 
{
	ISSUE_ENTRY_DATA_ADDR = 0,
	ISSUE_STATE = MAX_KEYPAD_ENTRIES + 1,
	ISSUE_ENTRY_SIZE = sizeof(ISSUE)

};

/*
*------------------------------------------------------------------------------
* Variables
*------------------------------------------------------------------------------
*/
#pragma idata APP_DATA
APP ias = {0};

UINT8 LogBuf[50];
UINT8 PingBuf[25];
#pragma idata

#pragma idata LOG_DATA
LOG log = {0};
#pragma idata


/*------------------------------------------------------------------------------
* Private Functions
*------------------------------------------------------------------------------
*/

static int updateLog(far UINT8 *data,UINT8 status);



void SendIssueState(UINT8* data,UINT8 state);
UINT8 getStatusLog(far UINT8** logBuff);

UINT8 APP_comCallBack( far UINT8 *rxPacket, far UINT8* txCode,far UINT8** txPacket);


/*
*------------------------------------------------------------------------------
* void APP-init(void)
*------------------------------------------------------------------------------
*/

void APP_init(void)
{

	UINT16 i;
	UINT8 j,*ptr;
	UINT8 ackStatus = 0;
	UINT8 grn =0 ,org = 0,red = 0,buz = 0; 
	UINT8 size;
	size = ISSUE_ENTRY_SIZE;

#ifdef __FACTORY_CONFIGURATION__

	ias.state = ISSUE_CLOSED;
												//No. of critical issues should be 0 initially
	ias.preAppTime = 0;
	ias.curAppTime = 0;
	ias.currentIssue = 0;
	ias.prevIssue = 0;
	log.index = 0;
	ias.logonStatus = 0;
	updateIndication(1,0,0,0);
	for(i = 0; i < 256; i++)
	{
		Write_b_eep(i,0);
		Busy_eep();
		ClrWdt();

	}

#else

	ias.preAppTime = 0;
	ias.curAppTime = 0;
	ias.currentIssue = 0;
	ias.prevIssue = 0;
	log.index = 0;

	for( i = 0; i < MAX_ISSUES; i++)
	{
		ptr = (UINT8*) &ias.issues[i];
		for( j = 0 ; j < ISSUE_ENTRY_SIZE; j++)
		{
			Busy_eep();
			ClrWdt();
			*(ptr+j) = Read_b_eep((i*ISSUE_ENTRY_SIZE)+j );

			Busy_eep();
			ClrWdt();

		}


	}




	ias.password[0] = '1';
	ias.password[1] = '3';
	ias.password[2] = '1';
	ias.password[3] = '3';
	ias.password[4] = '\0';

	
	Busy_eep();
	ClrWdt();

#endif

COM_init(CMD_SOP , CMD_EOP ,RESP_SOP , RESP_EOP , APP_comCallBack);

ias.state = ACTIVE;

}



/*
*------------------------------------------------------------------------------
* void APP-task(void)
*------------------------------------------------------------------------------
*/
void APP_task(void)
{
	UINT8 i,*ptr, data;

	UINT32 addr;

	ENTER_CRITICAL_SECTION();
	if( ias.state == ACTIVE)
	{
		EXIT_CRITICAL_SECTION();
		return;
	}
	EXIT_CRITICAL_SECTION();

	SendIssueState(ias.issues[ias.waitingIssue].data,ias.issues[ias.waitingIssue].state);
	/*
	while(ias.issues[ias.currentIssue].status == 0)
	{
		if(ias.currentIssue == ias.prevIssue)
			return;
		ias.currentIssue++;
		if(ias.currentIssue >= MAX_ISSUES)
			ias.currentIssue = 0;
	}
	SendIssueState(ias.issues[ias.currentIssue].data,ias.issues[ias.currentIssue].state);
	ias.prevIssue = ias.currentIssue;
	ias.currentIssue++;
	if(ias.currentIssue >= MAX_ISSUES)
		ias.currentIssue = 0;
	*/
	

}


void SendIssueState(UINT8* data,UINT8 state)
{
	UINT8 i = 0;

	while( *data != '\0')
	{
		LogBuf[i] = *data;
		i++;
		data++;
	}
	LogBuf[i++] = state;
	LogBuf[i]= '\0';

	COM_txString(LogBuf);

}

BOOL APP_raiseIssues(far UINT8* data)
{
	UINT8 i,j,issueIndex;
	UINT8 *ptr;



	for(i = 0; i < MAX_ISSUES; i++)									//if for any issue
		if(strcmp((INT8*)data,(INT8 *)ias.issues[i].data) == 0)		//input matches				
			if((ias.issues[i].state == ISSUE_RAISED) || (ias.issues[i].state == ISSUE_ACKNOWLEDGED ))				//and it is open
			{
				//UI_setState(UI_STATION);							//do nothing
				return FALSE;											
			}

	for( i = 0 ; i < MAX_ISSUES ;i++)
	{
		if( ias.issues[i].state == ISSUE_CLOSED )
		{
			ias.issues[i].state = ISSUE_RAISED;
			ias.issues[i].status = 1;
			strcpy((INT8*) ias.issues[i].data, (INT8*)data);

			ptr = (UINT8*)&ias.issues[i];
			for(j = 0; j < ISSUE_ENTRY_SIZE; j++)
			{
				Write_b_eep(j+(i*ISSUE_ENTRY_SIZE),*(ptr+j));
				Busy_eep();
				ClrWdt();
			}

			updateLog(data,'R');
			SendIssueState(ias.issues[i].data,ias.issues[i].state);
			ias.waitingIssue = i;
			ias.state = WAITING;
			appUpdateCount = 0;
			break;
		}
	}

	return TRUE;




}





void APP_getOpenIssue(far OpenIssue *openIssue)
{
	UINT8 i =openIssue->ID+1, j=0,k=0,l;
	UINT8 length;

	if( i >= MAX_ISSUES )
		i = 0;
	
	openIssue->ID = -1;

	for(l=0; l < MAX_ISSUES; l++,i++)
	{
		i = i%MAX_ISSUES;
		if( (ias.issues[i].state == ISSUE_RAISED) )
		{
			openIssue->tag[j++] = 'S';openIssue->tag[j++] = 'T';openIssue->tag[j++] = 'N';openIssue->tag[j++] = ':';
			openIssue->tag[j++] = ias.issues[i].data[k++];
			openIssue->tag[j++] =ias.issues[i].data[k++];
			openIssue->tag[j++] = '  ';
	
			openIssue->tag[j++] = 'C';openIssue->tag[j++] = 'O';openIssue->tag[j++] = 'D';openIssue->tag[j++] = 'E'; openIssue->tag[j++] = ':';
			openIssue->tag[j++] = ias.issues[i].data[k++];
			openIssue->tag[j++] =ias.issues[i].data[k++];
		
			openIssue->tag[j] = '\0';
			openIssue->ID = ias.openIssue = i;

			break;
		}

	}

}



void APP_acknowledgeIssues(UINT8 ID)
{
	UINT8 i;
	if(ias.issues[ID].state != ISSUE_RAISED) return;
	ias.issues[ID].state = ISSUE_ACKNOWLEDGED;
	ias.issues[ID].status = 1;

	updateLog(ias.issues[ID].data,ias.issues[ID].state );

	SendIssueState(ias.issues[ID].data,ias.issues[ID].state);
	ias.waitingIssue = ID;
	appUpdateCount = 0;
	ias.state = WAITING;

	Write_b_eep(((ID)*ISSUE_ENTRY_SIZE)+MAX_KEYPAD_ENTRIES + 1, ias.issues[ID].state);
	Busy_eep();
	ClrWdt();




			
	
}

void APP_getAcknowledgedIssue(far OpenIssue *openIssue)
{
	UINT8 i =openIssue->ID+1, j=0,k=0,l;
	UINT8 length;

	if( i >= MAX_ISSUES )
		i = 0;
	
	openIssue->ID = -1;

	for(l=0; l < MAX_ISSUES; l++,i++)
	{
		i = i%MAX_ISSUES;
		if( (ias.issues[i].state == ISSUE_ACKNOWLEDGED))
		{
			openIssue->tag[j++] = 'S';openIssue->tag[j++] = 'T';openIssue->tag[j++] = 'N';openIssue->tag[j++] = ':';
	
			openIssue->tag[j++] = ias.issues[i].data[k++];
			openIssue->tag[j++] = ias.issues[i].data[k++];
			openIssue->tag[j++] = '  ';
	
			openIssue->tag[j++] = 'C';openIssue->tag[j++] = 'O';openIssue->tag[j++] = 'D';openIssue->tag[j++] = 'E'; openIssue->tag[j++] = ':';
			openIssue->tag[j++] = ias.issues[i].data[k++];
			openIssue->tag[j++] = ias.issues[i].data[k++];
		
	
			openIssue->tag[j] = '\0';
			openIssue->ID = ias.openIssue = i;
	
			break;
		}

	
	}

}



void APP_resolveIssues(UINT8 ID)
{
	UINT8 i;
	if(ias.issues[ID].state != ISSUE_ACKNOWLEDGED) return;
	ias.issues[ID].state = ISSUE_RESOLVED;
	ias.issues[ID].status = 1;

	updateLog(ias.issues[ID].data,ias.issues[ID].state );

	SendIssueState(ias.issues[ID].data,ias.issues[ID].state);
	ias.waitingIssue = ID;
	appUpdateCount = 0;
	ias.state = WAITING;

	Write_b_eep(((ID)*ISSUE_ENTRY_SIZE)+MAX_KEYPAD_ENTRIES + 1, ias.issues[ID].state);
	Busy_eep();
	ClrWdt();
	

}











/*---------------------------------------------------------------------------------------------------------------
*	UINT8 getStatusLog(UINT8** logBuff)
*
*----------------------------------------------------------------------------------------------------------------
*/

UINT8 getStatusLog(far UINT8** logBuff)
{
	UINT8 i;
	UINT8 length;
	if(log.prevIndex == log.index)
	{
		*logBuff = 0;
		 length = 0;
	}
	else
	{
		*logBuff = log.entries[log.prevIndex];
		length = strlen(log.entries[log.prevIndex]);
		log.prevIndex++;
		if( log.prevIndex >= MAX_LOG_ENTRIES )
			log.prevIndex = 0;
	}
	
	return(length);
}







/*---------------------------------------------------------------------------------------------------------------
*	void updateLog(void)
*----------------------------------------------------------------------------------------------------------------
*/
int updateLog(far UINT8 *data,UINT8 status)
{
	UINT8 i = 0;
	UINT8 entryIndex = 0;
	while( *data != '\0')
	{
		log.entries[log.index][i] = *data;
		i++;
		data++;
	}
	log.entries[log.index][i] = status;
	log.entries[log.index][i]= '\0';
	entryIndex = log.index;
	log.index++;
	if( log.index >= MAX_LOG_ENTRIES)
		log.index = 0;

	return entryIndex;
}











						

BOOL APP_checkPassword(UINT8 *password)
{

	if( strcmp(ias.password , password) )
		return FALSE;
	return TRUE;
}


void APP_SetMaxStations(UINT8 stations)
{

	Write_b_eep(EEPROM_MAX_STATIONS_INDEX, stations);
	Busy_eep();
	ClrWdt();
}

void APP_SetMaxCodes(UINT8 codes)
{

	Write_b_eep(EEPROM_MAX_CODES_INDEX, codes);
	Busy_eep();
	ClrWdt();
}



void APP_clearIssues()
{

	UINT16 i;


											
	ias.preAppTime = 0;
	ias.curAppTime = 0;
	ias.openIssue = 0;
	log.index = 0;
	log.prevIndex = 0;


	for( i = 0 ; i < MAX_ISSUES; i++)
	{
		ias.issues[i].state = ISSUE_CLOSED;
		ias.issues[i].status = 0;
		memset(ias.issues[i].data , 0 , MAX_KEYPAD_ENTRIES + 1);
		ClrWdt();
	}



	for(i = 0; i <  MAX_ISSUES; i++)
	{
		Write_b_eep(i*ISSUE_ENTRY_SIZE+ MAX_KEYPAD_ENTRIES + 1,0);
		Busy_eep();
		ClrWdt();

		Write_b_eep(i*ISSUE_ENTRY_SIZE+ MAX_KEYPAD_ENTRIES + 2,0);
		Busy_eep();
		ClrWdt();

	}


	
}


UINT8 APP_comCallBack( far UINT8 *rxPacket, far UINT8* txCode,far UINT8** txPacket)
{


	UINT8 rxCode = rxPacket[0];
	UINT8 length = 0,i;

	switch( rxCode )
	{
		case CMD_UPDATE_STATUS:
		
			if(strcmp((INT8*)&rxPacket[1],
					(INT8 *)ias.issues[ias.waitingIssue].data) == 0)	//data matches	
			{
				if(ias.issues[ias.waitingIssue].status == 0 ) 
				{
					ias.waitingIssue = 0xFF;
					return 0xFF;				
				}

				ias.issues[ias.waitingIssue].status = 0;			//close status
				
				Write_b_eep((ias.waitingIssue*ISSUE_ENTRY_SIZE)+ MAX_KEYPAD_ENTRIES + 2,0);
				Busy_eep();
				ClrWdt();

				if(ias.issues[ias.waitingIssue].state == ISSUE_RESOLVED)		//if issue has been resolved
				{
					
					ias.issues[ias.waitingIssue].state = ISSUE_CLOSED;			//close issue

					Write_b_eep((ias.waitingIssue*ISSUE_ENTRY_SIZE)+ MAX_KEYPAD_ENTRIES + 1,0);
					Busy_eep();
					ClrWdt();
				}
				ias.waitingIssue = 0xFF;		//no issue in waiting state
				
			}

						
			length = 0xFF;
			*txCode = 	CMD_UPDATE_STATUS;
			ias.state = ACTIVE;
			UI_setState (UI_STATION);
			break;

	
		case CMD_CLEAR_ISSUE:
			UI_setWaitingState(UI_WAITFOR_CLEARING_ISSUE);
			for( i = 0 ; i < MAX_ISSUES; i++)
			{
					if(strcmp((INT8*)&rxPacket[1],
					(INT8 *)ias.issues[i].data) == 0)	//data matches	
					{
						ias.issues[i].state = ISSUE_CLOSED;			//close issue

						Write_b_eep((i*ISSUE_ENTRY_SIZE)+ MAX_KEYPAD_ENTRIES + 1,0);
						Busy_eep();
						ClrWdt();
						if( ias.waitingIssue == i )
						{
							ias.waitingIssue = 0xFF;
								if(ias.state == WAITING)
							{
								ias.state = ACTIVE;
								UI_setState (UI_STATION);
							}
						}
					
						*txCode = CMD_CLEAR_ISSUE;
						length = 0;
						return length;
					}
			}
			UI_setState (UI_STATION);
			*txCode = CMD_NO_ISSUE_FOUND;
			length = 0;
			break;



		case CMD_CLEAR_ALL_ISSUES:
			UI_setWaitingState(UI_WAITFOR_CLEARING_ALL_ISSUES);
			APP_clearIssues();
			*txCode = CMD_CLEAR_ALL_ISSUES;
			length = 0;
			ias.state = ACTIVE;
			UI_setState (UI_STATION);
			break;

	

		case CMD_PING:
		{
			UINT8 i,raised= 0,ack = 0,resolved = 0;
			UINT8 tmp[5];
			for( i = 0 ; i < MAX_ISSUES; i++)
			{
				if(ias.issues[i].state == ISSUE_RAISED)
					raised++;
				else if(ias.issues[i].state == ISSUE_ACKNOWLEDGED)
					ack++;
				else if(ias.issues[i].state == ISSUE_RESOLVED)
					resolved++;
				
			}
			PingBuf[0] = 0;
			strcat(PingBuf,"R:");
			strcat(PingBuf,itoa(raised,tmp));
			strcat(PingBuf,"|A:");
			strcat(PingBuf,itoa(ack,tmp));
			strcat(PingBuf,"|S:");
			strcat(PingBuf,itoa(resolved,tmp));
		
			length = strlen(PingBuf);
			*txPacket = PingBuf;
			*txCode = CMD_PING;
		}
			break;

		default:
			length = 0;
			*txCode = COM_RESP_INVALID_CMD;
			break;

	}

	return length;

}
	
		

		
	

