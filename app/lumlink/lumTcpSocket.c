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



static struct espconn serverConnHandle;
static struct _esp_tcp serverTcpHandle;


void USER_FUNC lum_sendTcpData(U8* socketData, U8 dataLen)
{
	espconn_sent(&serverConnHandle, socketData, dataLen);
}


static void USER_FUNC lum_tcpRecvCallback(void *arg, char *pusrdata, unsigned short length)
{
	lumDebug("lum_tcpRecvCallback\n");

	lum_sockRecvData(pusrdata, length, MSG_FROM_TCP, 0);
}

static void USER_FUNC lum_tcpSentCallback(void *arg)
{
    struct espconn *pespconn = arg;

    lumDebug("lum_tcpSentCallback\n");
}


static void USER_FUNC lum_tcpReconnectCallback(void *arg, sint8 err)
{
    struct espconn *pespconn = (struct espconn *)arg;

	lumDebug("lum_tcpReconnectCallback err =%d\n", err);
	lum_connBalanceServer();
}


static void USER_FUNC lum_tcpConnectCallback(void *arg)
{
    struct espconn *pespconn = arg;


	lumDebug("lum_tcpConnectCallback\n");
    espconn_regist_recvcb(pespconn, lum_tcpRecvCallback);
    espconn_regist_sentcb(pespconn, lum_tcpSentCallback);
   // user_esp_platform_sent(pespconn);
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


void USER_FUNC lum_connBalanceServer(void)
{
	lum_tcpSocketInit(TCP_SOCKET_PORT, ipaddr_addr(TCP_SERVER_IP));
}
