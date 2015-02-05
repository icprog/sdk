/*
******************************
*Company:Lumlink
*Data:2015-01-26
*Author:Meiyusong
******************************
*/

#ifndef __LUMLINK_COMMON_H__
#define __LUMLINK_COMMON_H__


//socket port define
#define UDP_SOCKET_PORT		18530
#define TCP_SOCKET_PORT		17531
//lumitek.bugull.com:17531 ==> 122.227.164.112
#define TCP_SERVER_IP		"122.227.164.112"

//get UTC date info
#define TCP_DATE_PORT		37
#define TCP_DATE_IP			"128.138.140.44"


//aes key define
#define DEFAULT_AES_KEY		"1234567890abcdef"
#define DEFAULT_AES_IV		"1234567890abcdef"
#define AES_KEY_LEN			16

//Modual info
#define HW_VERSION			"HW_V1.01"
#define SW_VERSION			"1.01"
#define DEFAULT_MODUAL_NAME		"Lumitek switch"


//device save data define
#define DEVICE_CONFIG_SIZE (sizeof(DEVICE_CONFIG_DATA))

//socket header data
#define SOCKET_HEADER_PV			0x01
#define SOCKET_HEADER_RESERVED		0x00
#define SOCKET_HEADER_DEVICE_TYPE	0xD1
#define SOCKET_HEADER_FACTORY_CODE	0xF1
#define SOCKET_HEADER_LICENSE_DATA	0xB421	//Correct data is  0x21B4

//sw flag
#define LUMITEK_SW_FLAG					0xDCBA

//Heart beat interval
#define MAX_HEARTBEAT_INTERVAL			20
#define MIN_HEARTBEAT_INTERVAL			10


//other data define define
#define SOCKEY_MAX_DATA_LEN				256
#define SOCKET_OPEN_DATA_LEN			sizeof(SOCKET_HEADER_OPEN)
#define SOCKET_SECRET_DATA_OFFSET		sizeof(SOCKET_HEADER_OPEN)
#define SOCKET_DATA_OFFSET				(sizeof(SCOKET_HERADER_INFO)+1)
#define SOCKET_HEADER_LEN				sizeof(SCOKET_HERADER_INFO)
#define DEVICE_MAC_LEN					6
#define SOCKET_IP_LEN					4
#define SOCKET_MAC_ADDR_OFFSET			2
#define DEVICE_NAME_LEN					20


#define MAX_ALARM_COUNT				32
#define MAX_ABSENCE_COUNT			10
#define MAX_COUNTDOWN_COUNT			1


//Heart beat interval
#define MAX_HEARTBEAT_INTERVAL			20
#define MIN_HEARTBEAT_INTERVAL			10
#define DEFAULT_HEARTBEAT_INTERVAL		20

#define INVALID_SERVER_ADDR			0xFFFFFFFFU
#define INVALID_SERVER_PORT			0xFFFF
#define INVALID_SN_NUM				0xFFFF
#define TCP_NULL_IP					0


#define USER_FUNC	ICACHE_FLASH_ATTR
#define USER_CONFIG_DATA_FLASH_SECTOR	0x3C


#define ntohs(_n)  ((U16)((((_n) & 0xff) << 8) | (((_n) >> 8) & 0xff)))
#define ntohl(_n)  ((U32)( (((_n) & 0xff) << 24) | (((_n) & 0xff00) << 8) | (((_n) >> 8)  & 0xff00) | (((_n) >> 24) & 0xff) ))
#define htons(_n) ntohs(_n)
#define htonl(_n) ntohl(_n)

#define ESP_DEBUG
#ifdef ESP_DEBUG
#define lumDebug os_printf
#define lumError os_printf
#else
#define lumDebug
#define lumError

#endif

typedef enum
{
	MSG_LOCAL_EVENT	= 0,
	MSG_FROM_UDP	= 1,
	MSG_FROM_TCP	= 2
} MSG_ORIGIN;



typedef struct
{
	U8	reserved0:1;
	U8	bReback:1;
	U8	bLocked:1;
	U8	reserved3:1;
	U8	reserved4:1;
	U8	reserved5:1;
	U8	bEncrypt:1;
	U8	reserved7:1;
} SOCKET_HEADER_FLAG;


typedef struct
{
	U8	pv;
	SOCKET_HEADER_FLAG	flag;
	U8	mac[DEVICE_MAC_LEN];
	U8	dataLen;
} SOCKET_HEADER_OPEN;



//struct size is mutiply of  item's max len
typedef struct
{
	SOCKET_HEADER_OPEN openData;
	U8	reserved;
	U16	snIndex;
	U8	deviceType;
	U8	factoryCode;
	U16	licenseData;
} SCOKET_HERADER_INFO;


typedef struct
{
	U8	nameLen;
	U8	nameData[DEVICE_NAME_LEN];
} DEVICE_NAME_DATA;


typedef struct
{
	U16 port;
	U32 ipAddr;
} SOCKET_ADDR;


typedef struct
{
	BOOL serverAesKeyValid;
	U8	serverKey[AES_KEY_LEN];
} AES_KEY_DATA;



typedef enum
{
	EVENT_INCATIVE = 0,
	EVENT_ACTIVE = 1
} ACTIVE_STATUS;


//ALARM data
typedef struct
{
	U8 monday:1;
	U8 tuesday:1;
	U8 wednesday:1;
	U8 thursday:1;
	U8 firday:1;
	U8 saturday:1;
	U8 sunday:1;
	U8 bActive:1;
} ALARM_REPEAT_DATA;


typedef struct
{
	ALARM_REPEAT_DATA repeatData;
	U8 startHour;
	U8 startMinute;
	U8 stopHour;
	U8 stopMinute;
	U8 reserved;
} ALARM_DATA_INFO;



typedef struct
{
	ALARM_REPEAT_DATA repeatData;
	U8 startHour;
	U8 startMinute;
	U8 endHour;
	U8 endMinute;
	U8 timeData;
} ASBENCE_DATA_INFO;


typedef struct
{
	U8 reserved0:1;
	U8 reserved1:1;
	U8 reserved2:1;
	U8 reserved3:1;
	U8 reserved4:1;
	U8 reserved5:1;
	U8 reserved6:1;
	U8 bActive:1;
} COUNTDOWN_FLAG;



typedef struct
{
	COUNTDOWN_FLAG flag;
	U8		action;
	U32		count;
} COUNTDOWN_DATA_INFO;



typedef struct
{
	U8	bLocked;	//used for check device be locked
	U8	swVersion;	//Used for upgrade check
	DEVICE_NAME_DATA deviceName;
	ALARM_DATA_INFO alarmData[MAX_ALARM_COUNT];
	ASBENCE_DATA_INFO absenceData[MAX_ABSENCE_COUNT];
	COUNTDOWN_DATA_INFO countDownData[MAX_COUNTDOWN_COUNT];
	U16 lumitekFlag;
} DEVICE_CONFIG_DATA;


typedef struct
{
	U8	macAddr[DEVICE_MAC_LEN];
	U32 ipAddr;
	AES_KEY_DATA keyData;
	U16 mallocCount;
	U16 socketSn;
	SOCKET_ADDR tcpServerAddr;
} GLOBAL_RUNING_DATA;


typedef struct
{
	DEVICE_CONFIG_DATA deviceConfigData;
	GLOBAL_RUNING_DATA globalData;

} GLOBAL_CONFIG_DATA;



BOOL USER_FUNC lum_checkRevcSocket(U8* recvData, U8 RecvLen);
void USER_FUNC lum_showHexData(S8* header, U8* data, U8 dataLen);
void USER_FUNC lum_getDeviceMac(U8* macAddr);

U8* USER_FUNC lum_malloc(U32 mllocSize);
void USER_FUNC lum_free(void* pData);

void USER_FUNC lum_globalConfigDataInit(void);
DEVICE_NAME_DATA* USER_FUNC lum_getDeviceName(void);
void USER_FUNC lum_setDeviceName(DEVICE_NAME_DATA* nameData);

U8 USER_FUNC lum_getDeviceLockStatus(void);
void USER_FUNC lum_setDeviceLockStatus(U8 lockStatus);

U16 USER_FUNC lum_getSocketSn(BOOL needIncrease);
void USER_FUNC lum_setServerAddr(SOCKET_ADDR* pSocketAddr);
void USER_FUNC lum_getServerAddr(SOCKET_ADDR* pSocketAddr);

void USER_FUNC lum_clearServerAesKey(void);
void USER_FUNC lum_setServerAesKey(U8* serverKey);
U8* USER_FUNC lum_getServerAesKey(U8* serverKey);
U8* USER_FUNC lum_showSendType(MSG_ORIGIN socketFrom, BOOL bSend, U8 cmd);

#endif

