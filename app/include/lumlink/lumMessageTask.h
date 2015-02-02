/*
******************************
*Company:Lumlink
*Data:2015-01-26
*Author:Meiyusong
******************************
*/

#ifndef __LUMLINK_MESSAGE_TASK_H__
#define __LUMLINK_MESSAGE_TASK_H__

#define MESSAGE_TASH_PRIO			0
#define MESSAGE_TASK_QUEUE_LEN		8


#define REBACK_SUCCESS_MESSAGE			0
#define REBACK_FAILD_MESSAGE			0xFF


typedef enum
{
	//Gpio data
	MSG_CMD_SET_GPIO_STATUS				= 0x01,
	MSG_CMD_GET_GPIO_STATUS				= 0x02,
	//Alarm data
	MSG_CMD_SET_ALARM_DATA				= 0x03,
	MSG_CMD_GET_ALARM_DATA				= 0x04,
	MSG_CMD_DELETE_ALARM_DATA			= 0x05,
	//Report data
	MSG_CMD_REPORT_GPIO_CHANGE			= 0x06,
	//MSG_CMD_REPORT_ALARM_CHANGE			= 0x07,
	//Against thief
	MSG_CMD_SET_ABSENCE_DATA			= 0x09,
	MSG_CMD_GET_ABSENCE_DATA			= 0x0A,
	MSG_CMD_DELETE_ABSENCE_DATA			= 0x0B,
	//stop watch
	MSG_CMD_SET_COUNDDOWN_DATA			= 0x0C,
	MSG_CMD_GET_COUNTDOWN_DATA			= 0x0D,
	MSG_CMD_DELETE_COUNTDOWN_DATA		= 0x0E,


	MSG_CMD_FOUND_DEVICE				= 0x23,
	MSG_CMD_LOCK_DEVICE					= 0x24,

	MSG_CMD_GET_SERVER_ADDR				= 0x41,
	MSG_CMD_REQUST_CONNECT				= 0x42,

	MSG_CMD_HEART_BEAT 					= 0x61,

	MSG_CMD_QUARY_MODULE_INFO			= 0x62,
	MSG_CMD_SET_MODULE_NAME				= 0x63,
	MSG_CMD_MODULE_UPGRADE				= 0x65,

	MSG_CMD_ENTER_SMART_LINK			= 0x66,

	//Local message start from 0xE1
	MSG_CMD_LOCAL_ENTER_SMARTLINK		= 0xFF01,
	MSG_CMD_LOCAL_GET_UTC_TIME			= 0xFF02,


	MSG_CMD_INVALID_EVENT				= 0xFFFF

} MESSAGE_CMD_TYPE;



typedef struct
{
	BOOL bReback;
	U16 cmdData;
	U16 snIndex;
	U16 dataLen;
	U8* pData;
	MSG_ORIGIN msgOrigin;
	U32 socketIp;
} MSG_BODY;


typedef struct
{
	U8 cmdCode;
	U8 IP[SOCKET_IP_LEN];
	U8 macAddr[DEVICE_MAC_LEN];
	U8 keyLen;
	U8 keyData[AES_KEY_LEN];
} CMD_FOUND_DEVIDE_RESP;


typedef struct
{
	U8 flag;
	U8 fre;   //固定为0x00
	U8 duty;   //输出高电平为0xFF，低电平为0x00
	U8 res;  //保留字节，固定为0xFF
} GPIO_STATUS;


void lum_messageTaskInit(void);
BOOL lum_sendLocalTaskMessage(U16 cmdData, U8* msgData, U8 dataLen);
void lum_postTaskMessage(U32 cmdMessage, U32 paraPointer);
MSG_BODY* lum_createTaskMessage(U8* socketData, U32 ipAddr, MSG_ORIGIN socketFrom);
void lum_sockRecvData(S8* recvData, U16 dataLen, MSG_ORIGIN socketFrom, U32 ipAddr);

#endif

