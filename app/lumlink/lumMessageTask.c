/*
******************************
*Company:Lumitek
*Data:2015-01-26
*Author:Meiyusong
******************************
*/


#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"
#include "lumlink/lumCommon.h"
#include "lumlink/lumMessageTask.h"
#include "lumlink/lumSocketAes.h"



static os_event_t g_messageTaskQueue[MESSAGE_TASK_QUEUE_LEN];


static BOOL USER_FUNC lum_createSendSocket(U8* oriSocketData, CREATE_SOCKET_DATA* pCreateData, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	U8* sendData;
	U8 socketLen;
	SCOKET_HERADER_INFO* pSocketinfo;

	pCreateData->keyType = lum_getSocketAesKeyType(socketFrom, pCreateData->bEncrypt);

	pSocketinfo = (SCOKET_HERADER_INFO*)oriSocketData;
	if(pCreateData->bReback == 1)
	{
		pCreateData->snIndex = pSocketinfo->snIndex;
	}
	else
	{
		//
	}
	
	sendData = lum_createSendSocketData(pCreateData, &socketLen);
	if(sendData == NULL)
	{
		return FALSE;
	}
	if(socketFrom == MSG_FROM_UDP)
	{
		lum_sendUdpData(sendData, socketLen, ipAddr);
	}
	else if(socketFrom == MSG_FROM_TCP)
	{
		//send TCP socket
	}
	else
	{
		//Do nothing
	}
	lum_free(sendData);
	return TRUE;
}


/********************************************************************************

User Request: 		|23|Dev_MAC|
Device Response: 	|23|IP|MAC| Key-Len | Key |

IP:			4-Byte，设备局域网的MAC地址
MAC:		6-Byte，设备MAC地址
Key-Len:		1-Byte，通信密钥的长度
Key			X-Byte，通信密钥

********************************************************************************/
static void USER_FUNC lum_setFoundDeviceBody(CMD_FOUND_DEVIDE_RESP* pFoundDevResp)
{
	struct ip_info ipconfig;

	
	pFoundDevResp->cmdCode = MSG_CMD_FOUND_DEVICE;
    wifi_get_ip_info(STATION_IF, &ipconfig);
	os_memcpy(pFoundDevResp->IP, (U8*)&ipconfig.ip.addr, SOCKET_IP_LEN);
	pFoundDevResp->keyLen = AES_KEY_LEN;
	lum_getAesKeyData(AES_KEY_DEFAULT, pFoundDevResp->keyData);
	lum_getDeviceMac(pFoundDevResp->macAddr);
}



static void USER_FUNC lum_replyFoundDevice(U8* socketData, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	CMD_FOUND_DEVIDE_RESP	foundDevResp;
	CREATE_SOCKET_DATA		createData;


	os_memset(&foundDevResp, 0, sizeof(CMD_FOUND_DEVIDE_RESP));
	os_memset(&createData, 0, sizeof(CREATE_SOCKET_DATA));
	lum_setFoundDeviceBody(&foundDevResp);

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen = sizeof(CMD_FOUND_DEVIDE_RESP);
	createData.bodyData = (U8*)(&foundDevResp);
	
	lum_createSendSocket(socketData, &createData, socketFrom, ipAddr);
}




static void USER_FUNC lum_messageTask(os_event_t *e)
{
	MSG_BODY* messageBody;


	lumDebug("==> e->sig= 0x%X\n", e->sig);
	messageBody = (MSG_BODY*)e->par;
	
	switch(e->sig)
	{
	case MSG_CMD_FOUND_DEVICE:
		lum_replyFoundDevice(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	default:
		break;
	}
	
	if(messageBody != NULL)
	{
		if(messageBody->pData != NULL)
		{
			lum_free(messageBody->pData);
		}
		lum_free(messageBody);
	}
}


void USER_FUNC lum_postTaskMessage(U32 cmdMessage, U32 paraPointer)
{
	system_os_post(MESSAGE_TASH_PRIO, cmdMessage, paraPointer);
}

void USER_FUNC lum_messageTaskInit(void)
{
	system_os_task(lum_messageTask, MESSAGE_TASH_PRIO, g_messageTaskQueue, MESSAGE_TASK_QUEUE_LEN);
}


