/*
******************************
*Company:Lumitek
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



static TCP_CONN_STATUS g_tcpConnStatus = TCP_NONE_CONNECT;
static struct espconn serverConnHandle;
static struct _esp_tcp serverTcpHandle;


void USER_FUNC lum_sendTcpData(U8* socketData, U8 dataLen)
{
	lumDebug("lum_sendTcpData  g_tcpConnStatus=%d\n", g_tcpConnStatus);
	if(g_tcpConnStatus == TCP_BALANCE_CONNECTED || g_tcpConnStatus == TCP_SERVER_CONNECTED)
	{
		espconn_sent(&serverConnHandle, socketData, dataLen);
	}
}


static void USER_FUNC lum_tcpRecvCallback(void *arg, char *pusrdata, unsigned short length)
{
	lumDebug("lum_tcpRecvCallback g_tcpConnStatus=%d\n", g_tcpConnStatus);

	lum_sockRecvData(pusrdata, length, MSG_FROM_TCP, TCP_NULL_IP);
}


static void USER_FUNC lum_tcpSentCallback(void *arg)
{
    struct espconn *pespconn = arg;

    lumDebug("lum_tcpSentCallback\n");
}


static void USER_FUNC lum_tcpReconnectCallback(void *arg, sint8 err)
{
    struct espconn *pespconn = (struct espconn *)arg;

	lumDebug("lum_tcpReconnectCallback err =%d g_tcpConnStatus=%d\n", err, g_tcpConnStatus);
	if(g_tcpConnStatus == TCP_BALANCE_CONNECTING || g_tcpConnStatus == TCP_BALANCE_CONNECTED)
	{
		lum_connBalanceServer();
	}
	else
	{
		lum_clearServerAesKey();
		lum_connServer();
	}
}


static void USER_FUNC lum_tcpConnectCallback(void *arg)
{
    struct espconn *pespconn = arg;


	lumDebug("lum_tcpConnectCallback g_tcpConnStatus=%d\n", g_tcpConnStatus);
    espconn_regist_recvcb(pespconn, lum_tcpRecvCallback);
    espconn_regist_sentcb(pespconn, lum_tcpSentCallback);
   // user_esp_platform_sent(pespconn);
   if(g_tcpConnStatus == TCP_BALANCE_CONNECTING)
   {
		lum_sendLocalTaskMessage(MSG_CMD_GET_SERVER_ADDR, NULL, 0);
		g_tcpConnStatus = TCP_BALANCE_CONNECTED;
   }
   else
   {
   		lum_sendLocalTaskMessage(MSG_CMD_REQUST_CONNECT, NULL, 0);
   		g_tcpConnStatus = TCP_SERVER_CONNECTED;
   }
}


static void USER_FUNC lum_tcpSocketInit(U16 port, U32 ipAddr)
{
	U8* tmp;


	tmp = (U8*)&ipAddr;
	lumDebug("ipAddr=%d.%d.%d.%d port=%d\n", tmp[0], tmp[1], tmp[2], tmp[3], port);
	
	serverConnHandle.proto.tcp = &serverTcpHandle;
	serverConnHandle.type = ESPCONN_TCP;
	serverConnHandle.state = ESPCONN_NONE;

	serverConnHandle.proto.tcp->remote_port = port;
	os_memcpy(serverConnHandle.proto.tcp->remote_ip, &ipAddr, SOCKET_IP_LEN);

	espconn_regist_connectcb(&serverConnHandle, lum_tcpConnectCallback);
	espconn_regist_reconcb(&serverConnHandle, lum_tcpReconnectCallback);
	espconn_connect(&serverConnHandle);
}


static void USER_FUNC lum_disconnectTcpServer(void)
{
	espconn_disconnect(&serverConnHandle);
	espconn_delete(&serverConnHandle);
}


void USER_FUNC lum_connBalanceServer(void)
{
	if(g_tcpConnStatus != TCP_NONE_CONNECT)
	{
		lum_disconnectTcpServer();
	}
	lum_tcpSocketInit(TCP_SOCKET_PORT, ipaddr_addr(TCP_SERVER_IP));
	g_tcpConnStatus = TCP_BALANCE_CONNECTING;
}


void USER_FUNC lum_connServer(void)
{
	SOCKET_ADDR socketAddr;


	lum_disconnectTcpServer();
	lum_getServerAddr(&socketAddr);
	lum_tcpSocketInit(socketAddr.port, socketAddr.ipAddr);
	g_tcpConnStatus = TCP_SERVER_CONNECTING;
}