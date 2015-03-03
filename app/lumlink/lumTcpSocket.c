/*
******************************
*Company:Lumlink
*Data:2015-02-01
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
#include "lumlink/lumSocketAes.h"
#include "lumlink/lumMessageTask.h"
#include "lumlink/lumPlatform.h"
#include "lumlink/lumTcpSocket.h"



static os_timer_t g_tcpReconnectTimer;
static TCP_CONN_STATUS g_tcpConnStatus = TCP_NONE_CONNECT;
static struct espconn serverConnHandle;
static struct _esp_tcp serverTcpHandle;


BOOL USER_FUNC lum_sendTcpData(U8* socketData, U8 dataLen)
{
	BOOL ret;
	S8 sendResult = ESPCONN_TIMEOUT;

	//lumDebug("lum_sendTcpData  g_tcpConnStatus=%d\n", g_tcpConnStatus);
	if(g_tcpConnStatus == TCP_BALANCE_CONNECTED || g_tcpConnStatus == TCP_SERVER_CONNECTED)
	{
		sendResult = espconn_sent(&serverConnHandle, socketData, dataLen);
	}
	ret = (sendResult == ESPCONN_OK)?TRUE:FALSE;
	return ret;
}


static void USER_FUNC lum_tcpRecvCallback(void *arg, char *pusrdata, unsigned short length)
{
	lum_sockRecvData(pusrdata, length, MSG_FROM_TCP, TCP_NULL_IP);
}


static void USER_FUNC lum_tcpSentCallback(void *arg)
{
    //struct espconn *pespconn = arg;

    //lumDebug("lum_tcpSentCallback\n");
}

static void USER_FUNC lum_balanceReconnectTimerCallback(void *arg)
{
	lum_connBalanceServer();
}


static void USER_FUNC lum_balanceReconnectCallback(void *arg, sint8 err)
{
	lumDebug("lum_balanceReconnectCallback err =%d g_tcpConnStatus=%d\n", err, g_tcpConnStatus);
	g_tcpConnStatus = TCP_NONE_CONNECT;
	os_timer_disarm(&g_tcpReconnectTimer);
    os_timer_setfn(&g_tcpReconnectTimer, (os_timer_func_t *)lum_balanceReconnectTimerCallback, NULL);
    os_timer_arm(&g_tcpReconnectTimer, TCP_RECONNECT_TIMER_GAP, 0);
}


static void USER_FUNC lum_balanceConnectCallback(void *arg)
{
    struct espconn *pespconn = arg;


	lumDebug("lum_balanceConnectCallback g_tcpConnStatus=%d\n", g_tcpConnStatus);
    espconn_regist_recvcb(pespconn, lum_tcpRecvCallback);
    espconn_regist_sentcb(pespconn, lum_tcpSentCallback);

	lum_sendLocalTaskMessage(MSG_CMD_GET_SERVER_ADDR, NULL, 0);
	g_tcpConnStatus = TCP_BALANCE_CONNECTED;
}


static void USER_FUNC lum_balanceDisconnectCallback(void *arg)
{
	struct espconn *pespconn = arg;

	lumDebug("lum_balanceDisconnectCallback g_tcpConnStatus=%d\n", g_tcpConnStatus);
	g_tcpConnStatus = TCP_NONE_CONNECT;
	espconn_delete(pespconn);
	lum_connActualServer();
}


static void USER_FUNC lum_serverReconnectTimerCallback(void *arg)
{
	lum_connActualServer();
}


static void USER_FUNC lum_serverReconnectCallback(void *arg, sint8 err)
{
    struct espconn *pespconn = (struct espconn *)arg;


	lumDebug("lum_serverReconnectCallback g_tcpConnStatus=%d\n", g_tcpConnStatus);
	g_tcpConnStatus = TCP_NONE_CONNECT;
	os_timer_disarm(&g_tcpReconnectTimer);
    os_timer_setfn(&g_tcpReconnectTimer, (os_timer_func_t *)lum_serverReconnectTimerCallback, NULL);
    os_timer_arm(&g_tcpReconnectTimer, TCP_RECONNECT_TIMER_GAP, 0);
}


static void USER_FUNC lum_serverConnectCallback(void *arg)
{
    struct espconn *pespconn = arg;


	lumDebug("lum_serverConnectCallback g_tcpConnStatus=%d\n", g_tcpConnStatus);
    espconn_regist_recvcb(pespconn, lum_tcpRecvCallback);
    espconn_regist_sentcb(pespconn, lum_tcpSentCallback);

	lum_sendLocalTaskMessage(MSG_CMD_REQUST_CONNECT, NULL, 0);
	g_tcpConnStatus = TCP_SERVER_CONNECTED;
}


static void USER_FUNC lum_serverDisconnectCallback(void *arg)
{
	lumDebug("lum_serverDisconnectCallback g_tcpConnStatus=%d\n", g_tcpConnStatus);
	lum_connActualServer();
}


static void USER_FUNC lum_tcpSocketInit(U16 port, U32 ipAddr, BOOL bBalance)
{
	U8* tmp;


	tmp = (U8*)&ipAddr;
	lumDebug("ipAddr=%d.%d.%d.%d port=%d\n", tmp[0], tmp[1], tmp[2], tmp[3], port);
	
	os_memset(&serverConnHandle, 0, sizeof(struct espconn));
	os_memset(&serverTcpHandle, 0, sizeof(struct _esp_tcp));

	serverConnHandle.proto.tcp = &serverTcpHandle;
	serverConnHandle.type = ESPCONN_TCP;
	serverConnHandle.state = ESPCONN_NONE;

	serverConnHandle.proto.tcp->remote_port = port;
	serverConnHandle.proto.tcp->local_port = espconn_port();
	os_memcpy(serverConnHandle.proto.tcp->remote_ip, &ipAddr, SOCKET_IP_LEN);

	if(bBalance)
	{
		espconn_regist_connectcb(&serverConnHandle, lum_balanceConnectCallback);
		espconn_regist_reconcb(&serverConnHandle, lum_balanceReconnectCallback);
		espconn_regist_disconcb(&serverConnHandle, lum_balanceDisconnectCallback);
	}
	else
	{
		espconn_regist_connectcb(&serverConnHandle, lum_serverConnectCallback);
		espconn_regist_reconcb(&serverConnHandle, lum_serverReconnectCallback);
		espconn_regist_disconcb(&serverConnHandle, lum_serverDisconnectCallback);
	}
	espconn_connect(&serverConnHandle);
}


void USER_FUNC lum_disconnectBalanceServer(void)
{
	g_tcpConnStatus = TCP_NONE_CONNECT;
	espconn_disconnect(&serverConnHandle);
}


void USER_FUNC lum_connBalanceServer(void)
{
	g_tcpConnStatus = TCP_NONE_CONNECT;
	lum_tcpSocketInit(TCP_SOCKET_PORT, ipaddr_addr(TCP_SERVER_IP), TRUE);
}


void USER_FUNC lum_connActualServer(void)
{
	SOCKET_ADDR socketAddr;


	g_tcpConnStatus = TCP_NONE_CONNECT;
	lum_clearServerAesKey();
	lum_getServerAddr(&socketAddr);
	lum_tcpSocketInit(socketAddr.port, socketAddr.ipAddr, FALSE);
}


