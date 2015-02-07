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
#define SEC_2015_01_01_00_00_00				(1420070400UL)
#define SYNC_SYSTEM_TIME_TIMER_GAP			(3600000UL)
#define SYNC_NETWORK_TIME_TIMER_SUCC_GAP	(3600000UL)
#define SYNC_NETWORK_TIME_TIMER_PROTECT_GAP	(10000UL)


typedef struct
{
	U32 lastUTCTime;  // network UTC time
	U32 lastSystemTime; //get time from system
} TIME_DATE_INFO;


typedef struct
{
	U16 year;		/* year. The number of years */
	U8 month;		/* month [0-11] */
	U8 day;			/* day of the month [1-31] */
	U8 week;		/* day of the week [0-6] 0-Sunday...6-Saturday */
	U8 hour;		/* hours [0-23] */
	U8 minute;		/* minutes [0-59] */
	U8 second;		/* seconds [0-59] */
} TIME_DATA_INFO;


void USER_FUNC lum_initSystemTime(void);
void USER_FUNC lum_initNetworkTime(void);
void USER_FUNC lum_getGmtime(TIME_DATA_INFO* timeInfo);
void USER_FUNC lum_getStringTime(S8* timeData);


#endif

