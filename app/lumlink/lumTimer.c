/*
******************************
*Company:Lumlink
*Data:2015-02-07
*Author:Meiyusong
******************************
*/


#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"


#include "lumlink/lumCommon.h"
#include "lumlink/lumGpio.h"
#include "lumlink/lumTimeDate.h"
#include "lumlink/lumTimer.h"



static BOOL USER_FUNC lum_compareWeekData(U8 compareWeek, U8 curWeek)
{
	U8 tem;
	BOOL ret = FALSE;

	if((compareWeek&0x7F) == 0) // not repeat
	{
		ret = TRUE;
	}
	else
	{
		tem = ((compareWeek&0x3F)<<1)|((compareWeek&0x40)>>6);
		if((tem & (1<<curWeek)) != 0)
		{
			ret = TRUE;
		}
	}
	return ret;
}

static BOOL USER_FUNC lum_bAbsenceRunNow(void)
{
	return FALSE;
}


static void USER_FUNC lum_compareAlarm(U8 index, TIME_DATA_INFO* pCurTime)
{
	ALARM_DATA_INFO* pAlarmInfo;
	U8 compareWeek;
	U16 checkStartMinute = 0;
	U16 checkStopMinute = 0;
	U16 curMinute;
	BOOL needSave = FALSE;


	pAlarmInfo = lum_getAlarmData(index);
	if(pAlarmInfo->repeatData.bActive == EVENT_INCATIVE)
	{
		return;
	}

	checkStartMinute = pAlarmInfo->startHour*60 + pAlarmInfo->startMinute;
	checkStopMinute = pAlarmInfo->stopHour*60 + pAlarmInfo->stopMinute;
	curMinute = pCurTime->hour+60 + pCurTime->minute;

	//compareWeek = *(U8*)(&pAlarmInfo->repeatData));
	os_memcpy(&compareWeek, &pAlarmInfo->repeatData, 0);
	if(checkStartMinute == curMinute)
	{
		if(lum_compareWeekData(compareWeek, pCurTime->week))
		{
			if(!lum_bAbsenceRunNow())
			{
				lum_setSwitchStatus(SWITCH_OPEN);
			}
			if((compareWeek&0x7F) == 0)
			{
				//unActive alarm
			}
		}
	}
	if(checkStopMinute == curMinute)
	{
		if(checkStopMinute <= checkStartMinute)
		{
			compareWeek = ((compareWeek&0x40)>>6) | ((compareWeek&0x3F)<<1) | (compareWeek&0x80);
		}
		if(lum_compareWeekData(compareWeek, pCurTime->week))
		{
			if(!lum_bAbsenceRunNow())
			{
				lum_setSwitchStatus(SWITCH_CLOSE);
			}
			if((compareWeek&0x7F) == 0)
			{
				//unActive alarm
			}
		}
	}
}

