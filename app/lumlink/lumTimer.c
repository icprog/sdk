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


static BOOL g_absenceRunning = FALSE;
static U16	g_nextAbsenceMinute = INVALUE_ABSENCE_MINUTE;
static U8 g_lastCheckMinute = 0xFF;


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
	return g_absenceRunning;
}


static void USER_FUNC lum_compareAlarm(U8 index, TIME_DATA_INFO* pCurTime, U16 curMinute)
{
	ALARM_DATA_INFO* pAlarmInfo;
	U8 compareWeek;
	U16 checkStartMinute = 0;
	U16 checkStopMinute = 0;


	pAlarmInfo = lum_getAlarmData(index);
	if(pAlarmInfo->repeatData.bActive == EVENT_INCATIVE)
	{
		return;
	}

	lumDebug("Alarm index=%d startHour=%d startMinute=%d stopHour=%d stopMinute=%d\n", index, pAlarmInfo->startHour, pAlarmInfo->startMinute, pAlarmInfo->stopHour, pAlarmInfo->stopMinute);
	checkStartMinute = pAlarmInfo->startHour*60 + pAlarmInfo->startMinute;
	checkStopMinute = pAlarmInfo->stopHour*60 + pAlarmInfo->stopMinute;
	lumDebug("Alarm curMinute=%d checkStartMinute=%d checkStopMinute=%d\n", curMinute, checkStartMinute, checkStopMinute);

	//compareWeek = *(U8*)(&pAlarmInfo->repeatData));
	os_memcpy(&compareWeek, &pAlarmInfo->repeatData, sizeof(ALARM_REPEAT_DATA));
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


static void USER_FUNC lum_checkAlarm(TIME_DATA_INFO* pCurTime, U16 curMinute)
{
	U8 i;

	for(i=0; i<MAX_ALARM_COUNT; i++)
	{
		lum_compareAlarm(i, pCurTime, curMinute);
	}
}



static ABSENXE_CHECK_STATUS USER_FUNC lum_compareAbsence(U8 index, TIME_DATA_INFO* pCurTime, U16 curMinute)
{
	U16 endMinute;
	U16 startMunite;
	U8 compareWeek;
	ASBENCE_DATA_INFO* pAbenceInfo;
	BOOL withinPeriod = FALSE;
	ABSENXE_CHECK_STATUS checkStatus = OUTOF_ABSENCE;;


	pAbenceInfo =  lum_getAbsenceData(index);
	if(pAbenceInfo->repeatData.bActive == EVENT_INCATIVE)
	{
		return checkStatus;
	}

	lumDebug("Absence index=%d, startHour=%d startMinute=%d endHour=%d endMinute=%d\n", index, pAbenceInfo->startHour, pAbenceInfo->startMinute, pAbenceInfo->endHour, pAbenceInfo->endMinute);
	os_memcpy(&compareWeek, &pAbenceInfo->repeatData, sizeof(ALARM_REPEAT_DATA));
	startMunite = pAbenceInfo->startHour*60 + pAbenceInfo->startMinute;
	endMinute = pAbenceInfo->endHour*60 + pAbenceInfo->endMinute;


	if(curMinute >= startMunite) //today
	{
		if(lum_compareWeekData(compareWeek, pCurTime->week))
		{
			if(curMinute <= endMinute || startMunite >= endMinute)
			{
				withinPeriod = TRUE;
			}
		}
	}
	else if(startMunite >= endMinute)//next day
	{
		compareWeek = ((compareWeek&0x40)>>6) | ((compareWeek&0x3F)<<1) | (compareWeek&0x80); //week add 1
		if(lum_compareWeekData(compareWeek, pCurTime->week))
		{
			if(curMinute <= endMinute)
			{
				withinPeriod = TRUE;
			}
		}
	}
	if(withinPeriod)
	{
		if(startMunite == curMinute)
		{
			checkStatus = EQUAL_START;
		}
		else if(curMinute == endMinute)
		{
			checkStatus = EQUAL_END;
		}
		else
		{
			checkStatus = WITHIN_ABSENCE;
		}
	}
	return checkStatus;
}


static void USER_FUNC lum_checkAbsence(TIME_DATA_INFO* pCurTime, U16 curMinute)
{
	U8 i;
	ABSENXE_CHECK_STATUS checkStatus;
	U16 randMinute;

	for(i=0; i<MAX_ABSENCE_COUNT; i++)
	{
		checkStatus |= lum_compareAbsence(i, pCurTime, curMinute);
	}
	if(checkStatus&EQUAL_START != 0)
	{
		if(!g_absenceRunning)
		{
			lum_setSwitchStatus(SWITCH_OPEN);
			randMinute = lum_getRandomNumber(MIN_ABSENCE_OPEN_INTERVAL, MAX_ABSENCE_OPEN_INTERVAL);
			g_nextAbsenceMinute = (curMinute + randMinute)%1440; //24*60
			g_absenceRunning = TRUE;
		}
	}
	else if(checkStatus&WITHIN_ABSENCE != 0)
	{
		//do nothing
	}
	else
	{
		if(g_absenceRunning)
		{
			lum_setSwitchStatus(SWITCH_CLOSE);
			g_nextAbsenceMinute = INVALUE_ABSENCE_MINUTE;
			g_absenceRunning = FALSE;
		}
	}

	if(g_absenceRunning && curMinute == g_nextAbsenceMinute)
	{
		SWITCH_STATUS curSwitchStatus;

		
		curSwitchStatus = lum_getSwitchStatus();
		if(curSwitchStatus == SWITCH_CLOSE)
		{
			randMinute = lum_getRandomNumber(MIN_ABSENCE_OPEN_INTERVAL, MAX_ABSENCE_OPEN_INTERVAL);
			lum_setSwitchStatus(SWITCH_OPEN);
		}
		else
		{
			randMinute = lum_getRandomNumber(MIN_ABSENCE_CLOSE_INTERVAL, MAX_ABSENCE_CLOSE_INTERVAL);
			lum_setSwitchStatus(SWITCH_CLOSE);
		}
		g_nextAbsenceMinute = (curMinute + randMinute)%1440; //24*60
	}
}


static void USER_FUNC lum_compareCountdown(U8 index, TIME_DATA_INFO* pCurTime)
{
	COUNTDOWN_DATA_INFO* pCountDownInfo;
	TIME_DATA_INFO countdownTime;
	SWITCH_STATUS action;


	pCountDownInfo = lum_getCountDownData(index);
	if(pCountDownInfo->flag.bActive == EVENT_INCATIVE)
	{
		return;
	}
	lum_gmtime(pCountDownInfo->count, &countdownTime);
	countdownTime.second = pCurTime->second;
	lumDebug("countdown index=%d %04d-%02d-%02d %02d:%02d:%02d\n", index, countdownTime.year, countdownTime.month+1, countdownTime.day,
		countdownTime.hour, countdownTime.minute, countdownTime.second);

	if(os_memcmp(&countdownTime, pCurTime, sizeof(TIME_DATA_INFO)) == 0)
	{
		COUNTDOWN_DATA_INFO countDownInfo;


		action = (pCountDownInfo->action == 1)?SWITCH_OPEN:SWITCH_CLOSE;
		lum_setSwitchStatus(action);

		os_memcpy(&countDownInfo, pCountDownInfo, sizeof(COUNTDOWN_DATA_INFO));
		countDownInfo.flag.bActive = EVENT_INCATIVE;
		lum_setCountDownData(&countDownInfo, index);
	}
}


static void USER_FUNC lum_checkCountdown(TIME_DATA_INFO* pCurTime)
{
	U8 i;


	for(i=0; i<MAX_COUNTDOWN_COUNT; i++)
	{
		lum_compareCountdown(i, pCurTime);
	}
}



static void USER_FUNC lum_checkTimer(TIME_DATA_INFO* pCurTime)
{
	U16 curMinute;


	curMinute = pCurTime->hour*60 + pCurTime->minute;
	lumDebug("checkTimer %04d-%02d-%02d %02d:%02d:%02d [%d]\n", pCurTime->year, pCurTime->month+1, pCurTime->day,
		pCurTime->hour, pCurTime->minute, pCurTime->second, pCurTime->week);

	lum_checkAlarm(pCurTime, curMinute);
	lum_checkAbsence(pCurTime, curMinute);
	lum_checkCountdown(pCurTime);
}


static void USER_FUNC lum_minuteCheckProtect(TIME_DATA_INFO* pCurTime)
{
	U8 tmpMinute;
	U8 totalMinute;
	U8 i;
	U32 totalSecond;
	U32 tmpSecond;
	TIME_DATA_INFO tmpTime;


	if(g_lastCheckMinute == 0xFF)
	{
		totalMinute = 1;
	}
	else
	{
		if(pCurTime->minute < g_lastCheckMinute)
		{
			tmpMinute = pCurTime->minute + 60;
		}
		totalMinute = tmpMinute - g_lastCheckMinute;
	}
	if(totalMinute > 10) //calibrate utc time
	{
		totalMinute = 1;
	}

	lumDebug("totalMinute=%d g_lastCheckMinute=%d\n", totalMinute, g_lastCheckMinute);
	if(totalMinute > 1)
	{
		totalSecond = lum_getSystemTime();
		for(i=1; i<=totalMinute; i++)
		{
			tmpSecond = totalSecond - (totalMinute-i)*60;
			lum_gmtime(tmpSecond, &tmpTime);
			lum_checkTimer(&tmpTime);
		}
	}
	else
	{
		lum_checkTimer(pCurTime);
	}
}


static void USER_FUNC lum_checkTimerCallback(void *arg)
{
	TIME_DATA_INFO curTime;
	U32 timerPeriod;


	lum_getGmtime(&curTime);
	if(curTime.year >= 2015)
	{
		lum_minuteCheckProtect(&curTime);
		g_lastCheckMinute = curTime.minute;
		timerPeriod = (70 - curTime.second)*1000;
	}
	else
	{
		timerPeriod = NOT_NTP_CHECK_TIMER_PERIOD;
	}
	lum_initTimer(timerPeriod);
}


void USER_FUNC lum_initTimer(U32 period)
{
	static os_timer_t checkTimerFd;

	
	os_timer_disarm(&checkTimerFd);
    os_timer_setfn(&checkTimerFd, (os_timer_func_t *)lum_checkTimerCallback, NULL);
    os_timer_arm(&checkTimerFd, period, 0);
}


