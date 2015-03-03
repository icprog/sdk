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
#include "lumlink/lumGpio.h"
#include "lumlink/lumSendList.h"




static os_event_t g_messageTaskQueue[MESSAGE_TASK_QUEUE_LEN];
static os_timer_t g_heartBeatTimer;



BOOL USER_FUNC lum_sendLocalTaskMessage(U16 cmdData, U8* msgData, U8 dataLen)
{
	MSG_BODY* messageBody;
	U16 msgBodyLen;
	U8* dataBody;


	msgBodyLen = sizeof(MSG_BODY)+1;
	messageBody = (MSG_BODY*)lum_malloc((U32)msgBodyLen);
	if(messageBody == NULL)
	{
		return FALSE;
	}
	os_memset(messageBody, 0, msgBodyLen);

	if(msgData != NULL && dataLen != 0)
	{
		dataBody = (U8*)lum_malloc((U32)(dataLen+1));
		if(dataBody == NULL)
		{
			lum_free(messageBody);
			return FALSE;
		}
		os_memset(dataBody, 0, (dataLen+1));
		os_memcpy(dataBody, msgData, dataLen);
	}
	else
	{
		dataBody = NULL;
	}

	messageBody->cmdData = cmdData;
	messageBody->dataLen = dataLen;
	messageBody->msgOrigin = MSG_LOCAL_EVENT;
	messageBody->pData = dataBody;
	messageBody->socketIp = TCP_NULL_IP;

	lum_postTaskMessage((U32)cmdData, (U32)messageBody);
	return TRUE;
}


MSG_BODY* USER_FUNC lum_createTaskMessage(U8* socketData, U32 ipAddr, MSG_ORIGIN socketFrom)
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
	messageBody->socketIp = ipAddr;
	return messageBody;

}


void USER_FUNC lum_sockRecvData(S8* recvData, U16 dataLen, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	if(!lum_checkRevcSocket((U8*)recvData, (U8)dataLen))
	{
		return;
	}
	else
	{
		U8* pDecryptData;
		MSG_BODY* messageBody;
		BOOL bDecryptSuccess;
		U8 decryptLen;


		pDecryptData = (U8*)lum_malloc(dataLen+1);
		if(pDecryptData == NULL)
		{
			lumDebug("malloc socket recvData faild length=%d socketFrom=%d\n", dataLen, socketFrom);
			return;
		}
		os_memset(pDecryptData, 0, (dataLen+1));
		bDecryptSuccess = lum_getRecvSocketData((U8*)recvData, pDecryptData, socketFrom, &decryptLen);
		if(!bDecryptSuccess)
		{
			lum_free(pDecryptData);
			return;
		}
#ifdef LUM_SHOW_SOCKET_DATA
		lum_showHexData(lum_showSendType(socketFrom, FALSE, pDecryptData[SOCKET_HEADER_LEN]), pDecryptData, decryptLen);
#endif
		messageBody = lum_createTaskMessage(pDecryptData, ipAddr, socketFrom);
		if(messageBody == NULL)
		{
			lum_free(pDecryptData);
		}
		else
		{
			lum_postTaskMessage((U32)messageBody->cmdData, (U32)messageBody);
		}
		//防止多条命令连续来时，密钥错位
		if(messageBody->cmdData == MSG_CMD_REQUST_CONNECT)
		{
			lum_setServerAesKey(pDecryptData + SOCKET_HEADER_LEN + 2); //set AES key Immediately
		}
	}

}


static BOOL USER_FUNC lum_createSendSocket(U8* oriSocketData, CREATE_SOCKET_DATA* pCreateData, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	U8* sendData;
	U8 socketLen;
	SCOKET_HERADER_INFO* pSocketinfo;


	pCreateData->keyType = lum_getSocketAesKeyType(socketFrom, pCreateData->bEncrypt);

	if(pCreateData->bReback == 1)
	{
		pSocketinfo = (SCOKET_HERADER_INFO*)oriSocketData;
		pCreateData->snIndex = pSocketinfo->snIndex;
	}
	else
	{
		pCreateData->snIndex = lum_getSocketSn(TRUE);
		pCreateData->snIndex = htons(pCreateData->snIndex);
	}

	sendData = lum_createSendSocketData(pCreateData, &socketLen, socketFrom);

	if(sendData == NULL)
	{
		return FALSE;
	}
	else
	{
		SEND_NODE_DATA sendNode;


		sendNode.bReback = pCreateData->bReback;
		sendNode.cmdData = pCreateData->bodyData[0];
		sendNode.dataLen = socketLen;
		sendNode.msgOrigin = socketFrom;
		sendNode.nextSendTime = 0;
		sendNode.pData = sendData;
		sendNode.sendCount = 0;
		sendNode.snIndex = pCreateData->snIndex;
		sendNode.socketIp = ipAddr;
		
		lum_addSendDataToNode(&sendNode);
		//lum_checkSendList();
	}
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

	//lumDebug("name=%s, nameLen=%d\n", pNameData->nameData, pNameData->nameLen);

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
		lum_setSwitchStatus(SWITCH_OPEN);
	}
	else //Close
	{
		lum_setSwitchStatus(SWITCH_CLOSE);
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
	if(lum_getSwitchStatus() == SWITCH_OPEN) //Open
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



/********************************************************************************
UserRequest:		| 41 |
Server Response:	| 41 | IP Address | Port |

********************************************************************************/
static void USER_FUNC lum_GetServerAddr(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	U8 data;
	CREATE_SOCKET_DATA createData;

	data = MSG_CMD_GET_SERVER_ADDR;

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 0;
	createData.bodyLen = 1;
	createData.bodyData = &data;

	lum_createSendSocket(pSocketDataRecv, &createData, MSG_FROM_TCP, ipAddr);
}



static void USER_FUNC lum_replyGetServerAddr(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	SOCKET_ADDR socketAddr;
	U8* tmp;

	os_memcpy(&socketAddr.ipAddr, (pSocketDataRecv + SOCKET_DATA_OFFSET), SOCKET_IP_LEN);
	os_memcpy(&socketAddr.port, (pSocketDataRecv + SOCKET_DATA_OFFSET + SOCKET_IP_LEN), 2);
	socketAddr.port = ntohs(socketAddr.port);
	lum_setServerAddr(&socketAddr);
	lum_disconnectBalanceServer();

	tmp = (U8*)&socketAddr.ipAddr;
	lumDebug("server ip=%d.%d.%d.%d  prot=%d\n", tmp[0], tmp[1], tmp[2], tmp[3], socketAddr.port);

	lum_deleteRequstSendNode(pSocketDataRecv);
}


/********************************************************************************
User Request:		| 42 |
Server Response:	| 42 | Key Len | Key |

********************************************************************************/
static void USER_FUNC lum_requstConnServer(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	U8 data;
	CREATE_SOCKET_DATA createData;

	data = MSG_CMD_REQUST_CONNECT;

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 0;
	createData.bodyLen = 1;
	createData.bodyData = &data;

	lum_createSendSocket(pSocketDataRecv, &createData, MSG_FROM_TCP, ipAddr);
}


static void USER_FUNC lum_replyRequstConnServer(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	//U8* pAesKey;
	//U8 keyLen;


	//pAesKey = pSocketDataRecv + SOCKET_HEADER_LEN + 2; //cmd+len
	//lum_setServerAesKey(pAesKey);
	//lumDebug("Keylen=%d AesKey=%s\n", pSocketDataRecv[SOCKET_DATA_OFFSET], pAesKey);
	//收到命令立刻设置，防止多条命令同时到达，密钥错位 (设置密钥提前)
	lum_sendLocalTaskMessage(MSG_CMD_HEART_BEAT, NULL, 0);
	lum_deleteRequstSendNode(pSocketDataRecv);
}



/********************************************************************************
Request:|61|
Response:|61|Interval|
Interval：2-Byte

********************************************************************************/
static void USER_FUNC lum_replyUdpHeartBeat(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	U8 heartBeatResp[10];
	U16 intervalData = 0;
	CREATE_SOCKET_DATA createData;
	U16 index = 0;


	os_memset(heartBeatResp, 0, sizeof(heartBeatResp));

	//Fill CMD
	heartBeatResp[index] = MSG_CMD_HEART_BEAT;
	index += 1;

	//Fill Interval
	intervalData = lum_getRandomNumber(MIN_HEARTBEAT_INTERVAL, MAX_HEARTBEAT_INTERVAL);
	intervalData = htons(intervalData);
	os_memcpy(heartBeatResp+index, &intervalData, 2);
	index += 2;

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen = index;
	createData.bodyData = heartBeatResp;

	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);
}


static void USER_FUNC lum_heartBeatTimerCallback(void *arg)
{
	lum_sendLocalTaskMessage(MSG_CMD_HEART_BEAT, NULL, 0);
}


static void USER_FUNC lum_setHeartBeatTimer(U16 intervel)
{
	U32 delayTime;


	delayTime = intervel*1000; //cover ms  to S
	os_timer_disarm(&g_heartBeatTimer);
	os_timer_setfn(&g_heartBeatTimer, (os_timer_func_t *)lum_heartBeatTimerCallback, NULL);
	os_timer_arm(&g_heartBeatTimer, delayTime, 0);
}




static void USER_FUNC lum_replyTcpHeartBeat(U8* pSocketDataRecv)
{
	U16 interval;


	os_memcpy(&interval, (pSocketDataRecv + SOCKET_DATA_OFFSET), 2);
	interval = ntohs(interval);
	lum_setHeartBeatTimer(interval);

	lum_deleteRequstSendNode(pSocketDataRecv);
}



static void USER_FUNC lum_requstTcpHeartBeat(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{

	U8 data;
	CREATE_SOCKET_DATA createData;

	data = MSG_CMD_HEART_BEAT;

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 0;
	createData.bodyLen = 1;
	createData.bodyData = &data;

	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);
	lum_setHeartBeatTimer(DEFAULT_TCP_HEARTBEAT_INTERVAL);
}


static void USER_FUNC lum_replyHeartBeat(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	if(socketFrom == MSG_LOCAL_EVENT)
	{
		lum_requstTcpHeartBeat(pSocketDataRecv, MSG_FROM_TCP, ipAddr);
	}
	else if(socketFrom == MSG_FROM_UDP)
	{
		lum_replyUdpHeartBeat(pSocketDataRecv, socketFrom, ipAddr);
	}
	else if(socketFrom == MSG_FROM_TCP)
	{
		lum_replyTcpHeartBeat(pSocketDataRecv);
	}
}


/********************************************************************************
Request:		| 66 |
Response:	| 66 | Result |

********************************************************************************/
static void USER_FUNC lum_replyEnterSmartLink(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	U8 enterSmartLinkResp[10];
	CREATE_SOCKET_DATA createData;
	U16 index = 0;
	U8 enterReson;


	os_memset(enterSmartLinkResp, 0, sizeof(enterSmartLinkResp));

	//Set reback socket body
	enterSmartLinkResp[index] = MSG_CMD_ENTER_SMART_LINK;
	index += 1;
	enterSmartLinkResp[index] = REBACK_SUCCESS_MESSAGE;
	index += 1;

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen = index;
	createData.bodyData = enterSmartLinkResp;

	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);
}


/********************************************************************************
Request:		| 06 | Pin |…|
Response:	| 06 | Pin |…|

********************************************************************************/
static U32 USER_FUNC lum_getBroadcastAddr(void)
{
	U32 ipAddr;
	struct ip_info ipconfig;


	wifi_get_ip_info(STATION_IF, &ipconfig);
	ipAddr = ipconfig.ip.addr | 0xFF000000;
	return ipAddr;
}


static void USER_FUNC lum_reportGpioChangeEvent(U8* pSocketDataRecv)
{
	U8 gpioChangeData[10];
	GPIO_STATUS* pGioStatus;
	CREATE_SOCKET_DATA createData;
	U16 index = 0;


	os_memset(gpioChangeData, 0, sizeof(gpioChangeData));


	//Set reback socket body
	gpioChangeData[index] = MSG_CMD_REPORT_GPIO_CHANGE;
	index += 1;

	pGioStatus = (GPIO_STATUS*)(gpioChangeData + index);
	pGioStatus->duty = (pSocketDataRecv[0] == 1)?0xFF:0;
	pGioStatus->res = 0xFF;
	index += sizeof(GPIO_STATUS);

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 0;
	createData.bodyLen = index;
	createData.bodyData = gpioChangeData;

	//send Socket
	lum_createSendSocket(pSocketDataRecv, &createData, MSG_FROM_UDP, lum_getBroadcastAddr());
	lum_createSendSocket(pSocketDataRecv, &createData, MSG_FROM_TCP, TCP_NULL_IP);
}


static void USER_FUNC lum_replyGpioChangeEvent(U8* pSocketDataRecv)
{
	lum_deleteRequstSendNode(pSocketDataRecv);
}


/********************************************************************************
Request:		| 03 |Pin_Num|Num|Flag|Start_Hour|Start_Min|Stop_Hour|Stop_min|Reserve|
Response:	| 03 |Result|


********************************************************************************/
static void USER_FUNC lum_replySetAlarmData(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	ALRAM_DATA* pAlarmData;
	U8 SetAlarmResp[10];
	CREATE_SOCKET_DATA createData;
	U16 index = 0;


	os_memset(SetAlarmResp, 0, sizeof(SetAlarmResp));

	//Save alarm data
	pAlarmData = (ALRAM_DATA*)(pSocketDataRecv + SOCKET_HEADER_LEN);
	lum_setAlarmData(&pAlarmData->alarmInfo, (pAlarmData->index - 1)); //pAlarmData->index from 1 to 32

	//Set reback socket body
	SetAlarmResp[index] = MSG_CMD_SET_ALARM_DATA;
	index += 1;
	SetAlarmResp[index] = REBACK_SUCCESS_MESSAGE;
	index += 1;

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen = index;
	createData.bodyData = SetAlarmResp;

	//send Socket
	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);
}




/********************************************************************************
Request: |04|Pin_Num|Num| ... |
Response:|04|Pin_Num|Num|Flag|Start_Hour|Start_Min|Stop_Hour|Stop_Min|Reserve|... |


********************************************************************************/
static U8 USER_FUNC lum_fillAlarmRebackData(U8* pdata, U8 alarmIndex)
{
	ALARM_DATA_INFO* pAlarmInfo;
	U8 index = 0;


	pAlarmInfo = lum_getAlarmData(alarmIndex - 1);
	if(pAlarmInfo == NULL)
	{
		return 0;
	}
	pdata[index] = alarmIndex; //num
	index += 1;
	os_memcpy((pdata+index), pAlarmInfo, sizeof(ALARM_DATA_INFO)); //flag
	index += sizeof(ALARM_DATA_INFO);
	return index;
}



static void USER_FUNC lum_replyGetAlarmData(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	U8 alarmIndex;
	U8 GetAlarmResp[250];  //(4+4)*MAX_ALARM_COUNT + 2+1
	CREATE_SOCKET_DATA createData;
	U16 index = 0;
	U8 i;


	os_memset(GetAlarmResp, 0, sizeof(GetAlarmResp));

	//Get data
	alarmIndex = pSocketDataRecv[SOCKET_HEADER_LEN + 2];

	//Set reback socket body
	GetAlarmResp[index] = MSG_CMD_GET_ALARM_DATA;
	index += 1;
	GetAlarmResp[index] = 0x0;
	index += 1;
	if(alarmIndex == 0)
	{
		for(i=1; i<=MAX_ALARM_COUNT; i++) // form 1 to MAX_ALARM_COUNT
		{
			index += lum_fillAlarmRebackData((GetAlarmResp + index), i);
		}
	}
	else
	{
		index += lum_fillAlarmRebackData((GetAlarmResp + index), alarmIndex);
	}

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen = index;
	createData.bodyData = GetAlarmResp;
	
	//send Socket
	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);
}




/********************************************************************************
Request:		| 05 | Pin_num|Num |
Response:	| 05 | Result |

********************************************************************************/
static void USER_FUNC lum_replyDeleteAlarmData(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	U8 alarmIndex;
	U8 DeleteAlarmResp[10];
	CREATE_SOCKET_DATA createData;
	U16 index = 0;


	os_memset(DeleteAlarmResp, 0, sizeof(DeleteAlarmResp));

	alarmIndex = pSocketDataRecv[SOCKET_HEADER_LEN + 2];
	lum_deleteAlarmData((alarmIndex - 1), TRUE);

	//Set reback socket body
	DeleteAlarmResp[index] = MSG_CMD_DELETE_ALARM_DATA;
	index += 1;
	DeleteAlarmResp[index] = REBACK_SUCCESS_MESSAGE;
	index += 1;

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen = index;
	createData.bodyData = DeleteAlarmResp;

	//send Socket
	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);

}



/********************************************************************************
Request:		|09|Num|Flag|Start_hour| Start_min | Stop_hour |Stop_min|Time|
Response:	|09| Result |

********************************************************************************/
static void USER_FUNC lum_replySetAbsenceData(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	ASBENCE_DATA_INFO* pAbsenceInfo;
	U8 absenceIndex;
	U8 SetAbsenceResp[10];
	CREATE_SOCKET_DATA createData;
	U16 index = 0;


	os_memset(SetAbsenceResp, 0, sizeof(SetAbsenceResp));

	//Save absence data
	absenceIndex = pSocketDataRecv[SOCKET_DATA_OFFSET];
	pAbsenceInfo = (ASBENCE_DATA_INFO*)(pSocketDataRecv + SOCKET_HEADER_LEN + 2);
	lum_setAbsenceData(pAbsenceInfo, absenceIndex - 1);

	//Set reback socket body
	SetAbsenceResp[index] = MSG_CMD_SET_ABSENCE_DATA;
	index += 1;
	SetAbsenceResp[index] = REBACK_SUCCESS_MESSAGE;
	index += 1;

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen = index;
	createData.bodyData = SetAbsenceResp;

	//send Socket
	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);
}




/********************************************************************************
Request:		|0A |Num|
Response:	|0A|Num|Flag|Start_hour|Start_min| Stop_hour |Stop_min|Time|…|

********************************************************************************/
static void USER_FUNC lum_replyGetAbsenceData(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	U8 absenceIndex;
	ASBENCE_DATA_INFO* pAbsenceInfo;
	U8 GetAbsenceResp[100];  //(8)*MAX_ABSENCE_COUNT + 2+1
	CREATE_SOCKET_DATA createData;
	U16 index = 0;
	U8 i;


	os_memset(GetAbsenceResp, 0, sizeof(GetAbsenceResp));
	absenceIndex = pSocketDataRecv[SOCKET_DATA_OFFSET];

	//Set reback socket body
	GetAbsenceResp[index] = MSG_CMD_GET_ABSENCE_DATA;
	index = 1;

	if(absenceIndex == 0)
	{
		for(i=1; i<=MAX_ABSENCE_COUNT; i++)
		{
			pAbsenceInfo = lum_getAbsenceData(i - 1);
			//if(pAbsenceInfo->startHour == 0xFF)
			//{
			//	continue;
			//}
			GetAbsenceResp[index] = i; //Num
			index += 1;

			os_memcpy((GetAbsenceResp + index), pAbsenceInfo, sizeof(ASBENCE_DATA_INFO));
			index += sizeof(ASBENCE_DATA_INFO);
		}
	}
	else
	{
		pAbsenceInfo = lum_getAbsenceData(absenceIndex - 1);
		GetAbsenceResp[index] = absenceIndex; //Num
		index += 1;

		os_memcpy((GetAbsenceResp + index), pAbsenceInfo, sizeof(ASBENCE_DATA_INFO));
		index += sizeof(ASBENCE_DATA_INFO);
	}

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen = index;
	createData.bodyData = GetAbsenceResp;

	//send Socket
	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);
}



/********************************************************************************
Request:		| 0B |Num|
Response:	|0B|Result|

********************************************************************************/
static void USER_FUNC lum_replyDeleteAbsenceData(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	U8 absenceIndex;
	U8 DeleteAbsenceResp[10];
	CREATE_SOCKET_DATA createData;
	U16 index = 0;


	os_memset(DeleteAbsenceResp, 0, sizeof(DeleteAbsenceResp));

	absenceIndex = pSocketDataRecv[SOCKET_DATA_OFFSET];
	lum_deleteAbsenceData((absenceIndex - 1), TRUE);

	//Set reback socket body
	DeleteAbsenceResp[index] = MSG_CMD_DELETE_ABSENCE_DATA;
	index += 1;
	DeleteAbsenceResp[index] = REBACK_SUCCESS_MESSAGE;
	index += 1;

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen = index;
	createData.bodyData = DeleteAbsenceResp;

	//send Socket
	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);
}



/********************************************************************************
Request:		|0C|Num|Flag|Stop_time|Pin|
Response:	|0C|Result|

********************************************************************************/
static void USER_FUNC lum_replySetCountDownData(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	COUNTDOWN_DATA_INFO countDownData;
	U8 countDownIndex;
	GPIO_STATUS* pGpioStatus;
	U8* pData;
	U32 count;
	U8 SetcountDownResp[10];
	CREATE_SOCKET_DATA createData;
	U16 index = 0;


	os_memset(SetcountDownResp, 0, sizeof(SetcountDownResp));

	//Save countDown data
	pData = pSocketDataRecv + SOCKET_DATA_OFFSET;
	countDownIndex = pData[0];
	os_memcpy(&countDownData.flag, (pData + 1), sizeof(COUNTDOWN_FLAG));
	os_memcpy(&count, (pData + 2), sizeof(U32));
	countDownData.count = ntohl(count);
	pGpioStatus = (GPIO_STATUS*)(pData + 6);
	countDownData.action = (pGpioStatus->duty == 0xFF)?SWITCH_OPEN:SWITCH_CLOSE;
	lum_setCountDownData(&countDownData, (countDownIndex - 1));

	//Set reback socket body
	SetcountDownResp[index] = MSG_CMD_SET_COUNTDOWN_DATA;
	index += 1;
	SetcountDownResp[index] = REBACK_SUCCESS_MESSAGE;
	index += 1;

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen = index;
	createData.bodyData = SetcountDownResp;

	//send Socket
	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);
}




/********************************************************************************
Request:		|0D|Num|
Response:	|0D|Num|Flag|Stop_time|Pin|…|

********************************************************************************/
static U8 USER_FUNC fillCountDownRebackData(U8* pdata, U8 countDownIndex)
{

	COUNTDOWN_DATA_INFO* pCountDownData;
	GPIO_STATUS gpioStatus;
	U16 index = 0;
	U32 countNum;


	pCountDownData = lum_getCountDownData(countDownIndex);
	if(pCountDownData == NULL)
	{
		return index;
	}
	pdata[index] = countDownIndex + 1; //set Num
	index += 1;

	os_memcpy((pdata + index), &pCountDownData->flag, sizeof(COUNTDOWN_FLAG)); //set Flag
	index += sizeof(COUNTDOWN_FLAG);

	countNum = htonl(pCountDownData->count); // Set stop time
	os_memcpy((pdata + index), &countNum, sizeof(U32));
	index += sizeof(U32);

	os_memset(&gpioStatus, 0, sizeof(GPIO_STATUS));
	gpioStatus.duty = (pCountDownData->action == SWITCH_OPEN)?0xFF:0;
	gpioStatus.res = 0xFF;
	os_memcpy((pdata + index), &gpioStatus, sizeof(GPIO_STATUS)); //pin
	index += sizeof(GPIO_STATUS);

	return index;
}



static void USER_FUNC lum_replyGetCountDownData(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	U8 countDownIndex;
	U8 GetCountDownResp[20];
	CREATE_SOCKET_DATA createData;
	U16 index = 0;
	U8 i;


	os_memset(GetCountDownResp, 0, sizeof(GetCountDownResp));

	//Get data
	countDownIndex = pSocketDataRecv[SOCKET_DATA_OFFSET];

	//Set reback socket body
	GetCountDownResp[index] = MSG_CMD_GET_COUNTDOWN_DATA; //set CMD
	index += 1;
	if(countDownIndex == 0)
	{
		for (i=0; i<MAX_COUNTDOWN_COUNT; i++)
		{
			index += fillCountDownRebackData((GetCountDownResp + index), i);
		}
	}
	else
	{
		index += fillCountDownRebackData((GetCountDownResp + index), (countDownIndex - 1));
	}

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen = index;
	createData.bodyData = GetCountDownResp;

	//send Socket
	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);
}




/********************************************************************************
Request:		|0E|Num|
Response:	|0E|Result||

********************************************************************************/
static void USER_FUNC lum_replyDeleteCountDownData(U8* pSocketDataRecv, MSG_ORIGIN socketFrom, U32 ipAddr)
{
	U8 countDownIndex;
	U8 DeleteCountDownResp[10];
	CREATE_SOCKET_DATA createData;
	U16 index = 0;


	os_memset(DeleteCountDownResp, 0, sizeof(DeleteCountDownResp));

	countDownIndex = pSocketDataRecv[SOCKET_DATA_OFFSET];
	lum_deleteCountDownData(countDownIndex -1);

	//Set reback socket body
	DeleteCountDownResp[index] = MSG_CMD_DELETE_COUNTDOWN_DATA;
	index += 1;
	DeleteCountDownResp[index] = REBACK_SUCCESS_MESSAGE;
	index += 1;

	//fill socket data
	createData.bEncrypt = 1;
	createData.bReback = 1;
	createData.bodyLen = index;
	createData.bodyData = DeleteCountDownResp;

	//send Socket
	lum_createSendSocket(pSocketDataRecv, &createData, socketFrom, ipAddr);
}


static void USER_FUNC lum_messageTask(os_event_t *e)
{
	MSG_BODY* messageBody;


	//lumDebug("e->sig= 0x%X\n", e->sig);
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

	case MSG_CMD_GET_SERVER_ADDR:
		if(messageBody->msgOrigin == MSG_LOCAL_EVENT)
		{
			lum_GetServerAddr(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		}
		else
		{
			lum_replyGetServerAddr(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		}
		break;

	case MSG_CMD_REQUST_CONNECT:
		if(messageBody->msgOrigin == MSG_LOCAL_EVENT)
		{
			lum_requstConnServer(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		}
		else
		{
			lum_replyRequstConnServer(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		}
		break;

	case MSG_CMD_HEART_BEAT:
		lum_replyHeartBeat(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	case MSG_CMD_ENTER_SMART_LINK:
		lum_replyEnterSmartLink(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	case MSG_CMD_REPORT_GPIO_CHANGE:
		if(messageBody->msgOrigin == MSG_LOCAL_EVENT)
		{
			lum_reportGpioChangeEvent(messageBody->pData);
		}
		else
		{
			lum_replyGpioChangeEvent(messageBody->pData);
		}
		break;

	//Alarm
	case MSG_CMD_SET_ALARM_DATA:
		lum_replySetAlarmData(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	case MSG_CMD_GET_ALARM_DATA:
		lum_replyGetAlarmData(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	case MSG_CMD_DELETE_ALARM_DATA:
		lum_replyDeleteAlarmData(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	//Absence
	case MSG_CMD_SET_ABSENCE_DATA:
		lum_replySetAbsenceData(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	case MSG_CMD_GET_ABSENCE_DATA:
		lum_replyGetAbsenceData(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	case MSG_CMD_DELETE_ABSENCE_DATA:
		lum_replyDeleteAbsenceData(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	//CountDown
	case MSG_CMD_SET_COUNTDOWN_DATA:
		lum_replySetCountDownData(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	case MSG_CMD_GET_COUNTDOWN_DATA:
		lum_replyGetCountDownData(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	case MSG_CMD_DELETE_COUNTDOWN_DATA:
		lum_replyDeleteCountDownData(messageBody->pData, messageBody->msgOrigin, messageBody->socketIp);
		break;

	case MSG_CMD_LOCAL_CHECK_SEND_LIST:
		lum_checkSendList();
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
	//lumDebug("lum_messageTask mallocCount=%d\n", lum_getMallocCount());
}


void USER_FUNC lum_postTaskMessage(U32 cmdMessage, U32 paraPointer)
{
	system_os_post(MESSAGE_TASH_PRIO, cmdMessage, paraPointer);
}

void USER_FUNC lum_messageTaskInit(void)
{
	system_os_task(lum_messageTask, MESSAGE_TASH_PRIO, g_messageTaskQueue, MESSAGE_TASK_QUEUE_LEN);
}


