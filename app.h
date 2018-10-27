#ifndef _APP_H_
#define _APP_H_


/*
*----------------------------------------------------------------------------------------------------------------
*	MACROS
*-----------------------------------------------------------------------------------------------------------------
*/

//#define __FACTORY_CONFIGURATION__


/*
*----------------------------------------------------------------------------------------------------------------
*	Enumerations
*-----------------------------------------------------------------------------------------------------------------
*/
typedef enum 
{
	OFF,
	ON
}INDICATOR_STATE;

typedef enum _ISSUE_TYPE
{
ISSUE_CLOSED = 0,
	ISSUE_RAISED = 'R',
	ISSUE_ACKNOWLEDGED = 'A',
	ISSUE_RESOLVED = 'S',
}ISSUE_STATE;



typedef enum _APP_PARAM
{
	MAX_KEYPAD_ENTRIES = 8,
	MAX_ISSUES = 64,
	MAX_DEPARTMENTS = 10,
	MAX_LOG_ENTRIES = 64,
	LOG_BUFF_SIZE = MAX_KEYPAD_ENTRIES+1

}APP_PARAM;

typedef enum _LOGDATA
{
	HW_TMEOUT = 10,
	APP_TIMEOUT = 1000,
	TIMESTAMP_UPDATE_VALUE = (APP_TIMEOUT/HW_TMEOUT)
}LOGDATA;

typedef enum
{
	ACTIVE = 1,
	WAITING 
		

}APP_STATE;

enum
{
	CMD_UPDATE_STATUS = 0x80,
	CMD_CLOSE_ISSUE = 0x81,
	CMD_PING = 0x82,
	CMD_CLEAR_ISSUE = 0x83,
	CMD_NO_ISSUE_FOUND = 0x84,
	CMD_CLEAR_ALL_ISSUES = 0x85

	

	
};

typedef struct _OpenIssue
{
	UINT8 tag[32];
	INT8 ID;
}OpenIssue;

extern void APP_init(void);
extern void APP_task(void);

BOOL APP_raiseIssues(far UINT8* data);
void APP_acknowledgeIssues(UINT8 ID);
void APP_resolveIssues(UINT8 id);
void APP_clearIssues(void);
BOOL APP_checkPassword(UINT8 *password);
BOOL APP_login(UINT8 *password,UINT8 *data);
BOOL APP_logout(UINT8 *password,UINT8 *data);
void APP_getOpenIssue(OpenIssue *);
void APP_getAcknowledgedIssue(far OpenIssue *openIssue);
void APP_SetMaxStations(UINT8 stations);
void APP_SetMaxCodes(UINT8 codes);


#endif