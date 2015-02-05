/*
******************************
*Company:Lumlink
*Data:2015-02-04
*Author:Meiyusong
******************************
*/

#ifndef __LUMLINK_TIME_DATE__H__
#define __LUMLINK_TIME_DATE__H__

#define DIFF_SEC_1900_1970         			(2208988800UL)
#define SEC_2015_01_01_00_00_00				(1420041600UL)
#define SYNC_SYSTEM_TIME_TIMER_GAP			(3600000UL)
#define SYNC_NETWORK_TIME_TIMER_SUCC_GAP	(7200000UL)
#define SYNC_NETWORK_TIME_TIMER_FAILD_GAP	(10000UL)
#define TIME_RECONNECT_TIMER_GAP			(10000)


typedef struct
{
	U32 lastUTCTime;  // network UTC time
	U32 lastSystemTime; //get time from system
}TIME_DATE_INFO;



void USER_FUNC lum_initSystemTime(void);
void USER_FUNC lum_initNetworkTime(void);
U32 USER_FUNC lum_getSystemTime(void);

#endif

