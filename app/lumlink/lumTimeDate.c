/*
******************************
*Company:Lumlink
*Data:2015-02-04
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
#include "lumlink/lumTimeDate.h"


static TIME_DATE_INFO g_timeDateInfo;
static os_timer_t g_systemTimeTimer;
static os_timer_t g_networkTimeTimer;
static os_timer_t g_getTimeTimer;

static BOOL g_getUTCSucc;

static struct espconn timeConnHandle;
static struct _esp_tcp timeTcpHandle;


static void USER_FUNC lum_getNetworkTime(void);
static void USER_FUNC lum_syncNetworkTimer(U32 timeGap);



static void USER_FUNC lum_syncNetworkTime(U32 networkTime)
{	
	g_timeDateInfo.lastUTCTime = networkTime - DIFF_SEC_1900_1970;
	g_timeDateInfo.lastSystemTime = system_get_time();
	lumDebug("============>network Time=%d\n", g_timeDateInfo.lastUTCTime);
	
}


static void USER_FUNC lum_timeSocketProtect(ETSTimerFunc pfunction)
{
	os_timer_disarm(&g_getTimeTimer);
    os_timer_setfn(&g_getTimeTimer, pfunction, NULL);
    os_timer_arm(&g_getTimeTimer, TIME_RECONNECT_TIMER_GAP, 0);
}



// set timeout while connect but not get time
static void USER_FUNC lum_disconnectTimerCallback(void *arg)
{
	espconn_disconnect(&timeConnHandle);
	if(!g_getUTCSucc)
	{
		lum_syncNetworkTimer(SYNC_NETWORK_TIME_TIMER_FAILD_GAP); //获取时间失败，减短等待时间
	}
}



static void USER_FUNC lum_timeRecvCallback(void *arg, char *pusrdata, unsigned short length)
{
	U32* tmp;
	U32 curUTC;


	tmp = (U32*)pusrdata;
	curUTC = ntohl(tmp[0]);
	lum_syncNetworkTime(curUTC);
	lum_timeSocketProtect(lum_disconnectTimerCallback); //收到数据10秒，如没有断开则强制断开
	g_getUTCSucc = TRUE;
}


static void USER_FUNC lum_timeSentCallback(void *arg)
{
    //struct espconn *pespconn = arg;

    lumDebug("lum_tcpSentCallback\n");
}


static void USER_FUNC lum_timeConnectCallback(void *arg)
{
	struct espconn *pespconn = arg;


	lumDebug("lum_timeConnectCallback\n");
	espconn_regist_recvcb(pespconn, lum_timeRecvCallback);
	espconn_regist_sentcb(pespconn, lum_timeSentCallback);

	lum_timeSocketProtect(lum_disconnectTimerCallback);//链接后10秒，如没有断开则强制断开
}


static void USER_FUNC lum_reconnectTimerCallback(void *arg)
{
	lum_getNetworkTime();
}


static void USER_FUNC lum_timeReconnectCallback(void *arg, sint8 err)
{
    //struct espconn *pespconn = (struct espconn *)arg;


	lumDebug("lum_timeReconnectCallback err=%d\n", err);
	if(g_getUTCSucc)
	{
		espconn_disconnect(&timeConnHandle);
	}
	else
	{
		lum_timeSocketProtect(lum_reconnectTimerCallback);
	}
}


static void USER_FUNC lum_timeDisconnectCallback(void *arg)
{
	struct espconn *pespconn = arg;

	
	lumDebug("lum_timeDisconnectCallback \n");
	os_timer_disarm(&g_getTimeTimer); //如已经断开，取消保护
	espconn_delete(pespconn);
}


static void USER_FUNC lum_getNetworkTime(void)
{
	U32 ipAddr;

	lumDebug("lum_getNetworkTime\n");
	os_memset(&timeConnHandle, 0, sizeof(struct espconn));
	os_memset(&timeTcpHandle, 0, sizeof(struct _esp_tcp));

	timeConnHandle.proto.tcp = &timeTcpHandle;
	timeConnHandle.type = ESPCONN_TCP;
	timeConnHandle.state = ESPCONN_NONE;

	timeConnHandle.proto.tcp->remote_port = TCP_DATE_PORT;
	timeConnHandle.proto.tcp->local_port = espconn_port();
	ipAddr = ipaddr_addr(TCP_DATE_IP);
	os_memcpy(timeConnHandle.proto.tcp->remote_ip, &ipAddr, SOCKET_IP_LEN);

	espconn_regist_connectcb(&timeConnHandle, lum_timeConnectCallback);
	espconn_regist_reconcb(&timeConnHandle, lum_timeReconnectCallback);
	espconn_regist_disconcb(&timeConnHandle, lum_timeDisconnectCallback);

	espconn_connect(&timeConnHandle);
	g_getUTCSucc = FALSE;
}


static void USER_FUNC lum_syncSystemTime(void)
{
	U32 curTimeUs;
	U32 curTimeSecond;


	curTimeUs = system_get_time();
	if(curTimeUs < g_timeDateInfo.lastSystemTime)
	{
		curTimeUs = 0xFFFFFFFF - g_timeDateInfo.lastSystemTime + curTimeUs;
	}
	else
	{
		curTimeUs = curTimeUs - g_timeDateInfo.lastSystemTime;
	}

	curTimeSecond = curTimeUs/1000000;  //us-->S
	g_timeDateInfo.lastUTCTime += curTimeSecond;
	g_timeDateInfo.lastSystemTime = curTimeUs;
}


static void USER_FUNC lum_syncSystemTimerCallback(void *arg)
{
	lum_syncSystemTime();
}


static void USER_FUNC lum_syncSystemTimer(void)
{
	os_timer_disarm(&g_systemTimeTimer);
    os_timer_setfn(&g_systemTimeTimer, (os_timer_func_t *)lum_syncSystemTimerCallback, NULL);
    os_timer_arm(&g_systemTimeTimer, SYNC_SYSTEM_TIME_TIMER_GAP, 1);
}


static void USER_FUNC lum_syncNetworkTimerCallback(void *arg)
{
	lum_syncNetworkTimer(SYNC_NETWORK_TIME_TIMER_SUCC_GAP);
	lum_getNetworkTime();
}


static void USER_FUNC lum_syncNetworkTimer(U32 timeGap)
{
	os_timer_disarm(&g_networkTimeTimer);
    os_timer_setfn(&g_networkTimeTimer, (os_timer_func_t *)lum_syncNetworkTimerCallback, NULL);
    os_timer_arm(&g_networkTimeTimer, timeGap, 0);
}


void USER_FUNC lum_initSystemTime(void)
{
	g_timeDateInfo.lastSystemTime = 0;
	g_timeDateInfo.lastUTCTime = SEC_2015_01_01_00_00_00;
	lum_syncSystemTimer();
}


void USER_FUNC lum_initNetworkTime(void)
{
	lum_syncNetworkTimer(10000);
}


U32 USER_FUNC lum_getSystemTime(void)
{
	U32 curTimeUs;
	U32 curTimeSecond;


	curTimeUs = system_get_time();
	if(curTimeUs < g_timeDateInfo.lastSystemTime)
	{
		curTimeUs = 0xFFFFFFFF - g_timeDateInfo.lastSystemTime + curTimeUs;
	}
	else
	{
		curTimeUs = curTimeUs - g_timeDateInfo.lastSystemTime;
	}
	curTimeSecond = curTimeUs/1000000;  //us-->S 
	return g_timeDateInfo.lastUTCTime + curTimeSecond;
}



