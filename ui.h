

typedef enum
{

	UI_STATION= 0 ,
	UI_ISSUE ,
	UI_PART_NO,
	UI_ISSUE_ACK,
	UI_CLEAR_ISSUE,
	UI_PASSWORD,
	UI_ADMIN_ACTIVITY,
	UI_BRK_QUA_MS,
	UI_ISSUE_RESOLVE,
	UI_WAITING,
	UI_VERIFY,
	UI_SET_MAX_STATIONS,
	UI_SET_MAX_CODES
	

}UI_STATE;


typedef enum
{

	UI_WAITFOR_RAISING_ISSUE = 0 ,
	UI_WAITFOR_ACKNOWLEDGING_ISSUE ,
	UI_WAITFOR_RESOLVING_ISSUE ,
	UI_WAITFOR_CLEARING_ISSUE ,
	UI_WAITFOR_CLEARING_ALL_ISSUES
	

}UI_WAITING_STATE;



	
	
enum
{
	UI_MSG_STATION = 0,
	UI_MSG_ISSUE,
	UI_MSG_PASSWORD,
	UI_MSG_ADMIN_ACTIVITY,
	UI_MSG_CLEAR_ISSUES,
	UI_MSG_WAITING,
	UI_MSG_VERIFY

	
};
		

enum
{
	ISSUE_0 = 0,
	ISSUE_1 ,
	ISSUE_2,
	ISSUE_3
};

extern const rom UINT8 *UI_MSG[];

void UI_init(UINT8 maxCode, UINT8 maxStation);
void UI_task(void);
void UI_setState( UI_STATE state);
UI_STATE UI_GetState(void);
void UI_setWaitingState( UI_WAITING_STATE state);
