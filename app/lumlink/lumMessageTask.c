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



static void USER_FUNC lum_replyFoundDevice(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
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
	
	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);
}



/********************************************************************************
Request:|62|
Response:|62|H-Len|H-Ver|S-Len|S-Ver|N-Len|Name|

参数说明：
H-Len：1-Byte，硬件版本号长度
H-Ver：X-Byte，硬件版本号
S-Len：1-Byte，软件版本号长度
S-Ver：X-Byte，软件版本号
N-Len：1-Byte，设备别名长度
Name：X-Byte，设备别名

********************************************************************************/
static void USER_FUNC lum_replyGetDeviceInfo(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	U8 deviceNameResp[100];
	U8 dataLen;
	DEVICE_NAME_DATA* pNameData;
	CREATE_SOCKET_DATA createData;
	U16 index = 0;


	os_memset(deviceNameResp, 0, sizeof(deviceNameResp));
	os_memset(&createData, 0, sizeof(CREATE_SOCKET_DATA));

	//Fill CMD
	deviceNameResp[index] = MSG_CMD_QUARY_MODULE_INFO;
	index += 1;

	//HW version lenth
	dataLen = os_strlen(HW_VERSION);
	deviceNameResp[index] = dataLen;
	index += 1;

	//HW version data
	os_memcpy((deviceNameResp+index), HW_VERSION, dataLen);
	index += dataLen;

	//SW version lenth
	dataLen = os_strlen(SW_VERSION);
	deviceNameResp[index] = dataLen;
	index += 1;

	//SW version data
	os_memcpy((deviceNameResp+index), SW_VERSION, dataLen);
	index += dataLen;

	//Device name lenth
	pNameData = lum_getDeviceName();
	deviceNameResp[index] = pNameData->nameLen;
	index += 1;

	//Device name data
	os_memcpy((deviceNameResp + index), pNameData->nameData, pNameData->nameLen);
	index += pNameData->nameLen;

	lumDebug("name=%s, nameLen=%d\n", pNameData->nameData, pNameData->nameLen);

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen =index;
	createData.bodyData = deviceNameResp;
	
	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);

}



/********************************************************************************
Request:		| 63 | N-Len | Name |
Response:	| 63 | Result |

********************************************************************************/
static void USER_FUNC lum_rebackSetDeviceName(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	U8 deviceNameResp[10];
	DEVICE_NAME_DATA nameData;
	CREATE_SOCKET_DATA createData;
	U16 index = 0;


	os_memset(deviceNameResp, 0, sizeof(deviceNameResp));
	os_memset(&nameData, 0, sizeof(DEVICE_NAME_DATA));
	os_memset(&createData, 0, sizeof(CREATE_SOCKET_DATA));

	//Set device name
	nameData.nameLen = pSocketDataRecv[SOCKET_DATA_OFFSET];
	nameData.nameLen = (nameData.nameLen > (DEVICE_NAME_LEN - 2))?(DEVICE_NAME_LEN - 2):nameData.nameLen;
	os_memcpy(nameData.nameData, (pSocketDataRecv + SOCKET_HEADER_LEN + 2), nameData.nameLen);
	lum_setDeviceName(&nameData);
	lumDebug("Set device name = %s\n", nameData.nameData);

	//Set reback socket body
	deviceNameResp[index] = MSG_CMD_SET_MODULE_NAME;
	index += 1;
	deviceNameResp[index] = REBACK_SUCCESS_MESSAGE;
	index += 1;

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen = index;
	createData.bodyData = deviceNameResp;
	
	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);
}


/********************************************************************************
User Request:		|24|dev_MAC|
Device Response:	|24|Result|

********************************************************************************/
static void USER_FUNC lum_replyLockDevice(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	U8 deviceLockResp[10];
	CREATE_SOCKET_DATA createData;
	U16 index = 0;
	U8 result;
	U8 macAddr[DEVICE_MAC_LEN];


	os_memset(deviceLockResp, 0, sizeof(deviceLockResp));
	lum_getDeviceMac(macAddr);
	
	//Lock device
	if(os_memcmp((pSocketDataRecv + SOCKET_DATA_OFFSET), macAddr, DEVICE_MAC_LEN) == 0)
	{
		lum_setDeviceLockStatus(1);
		result = REBACK_SUCCESS_MESSAGE;
	}
	else
	{
		result = REBACK_FAILD_MESSAGE;
	}

	//Fill reback body
	deviceLockResp[index] = MSG_CMD_LOCK_DEVICE;
	index += 1;

	deviceLockResp[index] = result;
	index += 1;

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen = index;
	createData.bodyData = deviceLockResp;
	
	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);
}


/********************************************************************************
Request:		| 01 | Pin |
Response:	| 01 | Pin|

********************************************************************************/
static void USER_FUNC lum_replySetGpioStatus(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	GPIO_STATUS* pGpioStatus;
	U8 gpioStatusResp[20];
	CREATE_SOCKET_DATA createData;
	U16 index = 0;


	os_memset(gpioStatusResp, 0, sizeof(gpioStatusResp));

	//set gpio status
	pGpioStatus = (GPIO_STATUS*)(pSocketDataRecv + SOCKET_DATA_OFFSET);
	if(pGpioStatus->duty == 0xFF) //Open
	{
		//setSwitchStatus(SWITCH_OPEN);
	}
	else //Close
	{
		//setSwitchStatus(SWITCH_CLOSE);
	}

	//Set reback socket body
	gpioStatusResp[index] = MSG_CMD_SET_GPIO_STATUS;
	index += 1;
	os_memcpy((gpioStatusResp + index), pGpioStatus, sizeof(GPIO_STATUS));
	index += sizeof(GPIO_STATUS);

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen = index;
	createData.bodyData = gpioStatusResp;
	
	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);
}


/********************************************************************************
Request:		| 02 | Pin |
Response:	| 02 | Pin|

********************************************************************************/
void USER_FUNC lum_replyGetGpioStatus(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	GPIO_STATUS* pGpioStatus;
	U8 gpioStatusResp[20];
	CREATE_SOCKET_DATA createData;
	U16 index = 0;


	os_memset(gpioStatusResp, 0, sizeof(gpioStatusResp));

	//Get gpio status
	pGpioStatus = (GPIO_STATUS*)(pSocketDataRecv + SOCKET_DATA_OFFSET);
	//if(getSwitchStatus()) //Open
	if(1)
	{
		pGpioStatus->duty = 0xFF;
	}
	else //Close
	{
		pGpioStatus->duty = 0x0;
	}
	pGpioStatus->res = 0xFF;

	//Set reback socket body
	gpioStatusResp[index] = MSG_CMD_GET_GPIO_STATUS;
	index += 1;
	os_memcpy((gpioStatusResp + index), pGpioStatus, sizeof(GPIO_STATUS));
	index += sizeof(GPIO_STATUS);

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen = index;
	createData.bodyData = gpioStatusResp;
	
	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);
}


static void USER_FUNC lum_messageTask(os_event_t *e)
{
	MSG_BODY* messageBody;


	lumDebug("e->sig= 0x%X\n", e->sig);
	messageBody = (MSG_BODY*)e->par;
	
	switch(e->sig)
	{
	case MSG_CMD_FOUND_DEVICE:
		lum_replyFoundDevice(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	case MSG_CMD_QUARY_MODULE_INFO:
		lum_replyGetDeviceInfo(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	case MSG_CMD_SET_MODULE_NAME:
		lum_rebackSetDeviceName(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	case MSG_CMD_LOCK_DEVICE:
		lum_replyLockDevice(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	case MSG_CMD_SET_GPIO_STATUS:
		lum_replySetGpioStatus(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	case MSG_CMD_GET_GPIO_STATUS:
		lum_replyGetGpioStatus(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
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


