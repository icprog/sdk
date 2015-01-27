/*
******************************
*Company:Lumitek
*Data:2015-01-26
*Author:Meiyusong
******************************
*/


#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"


#include "user_interface.h"
#include "lumlink/lumCommon.h"
#include "lumlink/lumSocketAes.h"
#include "lumlink/lumMessageTask.h"




static struct espconn g_UDPespconn;


static MSG_BODY* USER_FUNC lum_createTaskMessage(U8* socketData, U32 ipAddr, MSG_ORIGIN socketFrom)
{
	MSG_BODY* messageBody;
	SCOKET_HERADER_INFO* socketHeader;
	U16 msgBodyLen;


	msgBodyLen = sizeof(MSG_BODY)+1;
	messageBody = (MSG_BODY*)lum_malloc((U32)msgBodyLen);
	if(messageBody == NULL)
	{
		return NULL;
	}
	os_memset(messageBody, 0, msgBodyLen);

	socketHeader = (SCOKET_HERADER_INFO*)socketData;
	messageBody->bReback = socketHeader->openData.flag.bReback;
	messageBody->cmdData = socketData[SOCKET_HEADER_LEN];
	messageBody->dataLen = socketHeader->openData.dataLen + SOCKET_OPEN_DATA_LEN;
	messageBody->msgOrigin = socketFrom;
	messageBody->pData = socketData;
	messageBody->snIndex = socketHeader->snIndex;
	messageBody->socketIp = socketFrom;
	return messageBody;
	
}


static void USER_FUNC lum_udpSocketRecv(void *arg, char *pusrdata, unsigned short length)
{
   // lumDebug("Go into lum_udpRecv recvLen=%d\n", length);
	if(!lum_checkRevcSocket((U8*)pusrdata, (U8)length))
	{
		return;
	}
	else
	{
		U8* pDecryptData;
		U32 ipAddr;
		MSG_BODY* messageBody;


		pDecryptData = (U8*)lum_malloc(length+1);
		if(pDecryptData == NULL)
		{
			lumDebug("malloc UDP recvData faild length=%d\n", length);
			return;
		}
		memset(pDecryptData, 0, (length+1));
		lum_getRecvSocketData((U8*)pusrdata, pDecryptData, MSG_FROM_UDP);
		lum_showHexData(pDecryptData, length);

		os_memcpy(&ipAddr, g_UDPespconn.proto.udp->remote_ip, SOCKET_IP_LEN);
		messageBody = lum_createTaskMessage(pDecryptData, ipAddr, MSG_FROM_UDP);
		if(messageBody == NULL)
		{
			lum_free(pDecryptData);
		}
		else
		{
			lum_postTaskMessage((U32)messageBody->cmdData, (U32)messageBody);
		}
	}
}


void USER_FUNC lum_sendUdpData(U8* socketData, U8 dataLen, U32 ipData)
{
	os_memcpy(g_UDPespconn.proto.udp->remote_ip, &ipData, SOCKET_IP_LEN);
	g_UDPespconn.proto.udp->remote_port = UDP_SOCKET_PORT;
	lum_showHexData(socketData, dataLen);
	espconn_sent(&g_UDPespconn, socketData, dataLen);
}


void USER_FUNC lum_udpSocketInit(void)
{
    g_UDPespconn.type = ESPCONN_UDP;
    g_UDPespconn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
    g_UDPespconn.proto.udp->local_port = UDP_SOCKET_PORT;
    espconn_regist_recvcb(&g_UDPespconn, lum_udpSocketRecv);
    espconn_create(&g_UDPespconn);
}




