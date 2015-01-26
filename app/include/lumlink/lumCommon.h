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
#define MAX_PING_DATA_COUNT	3
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
#define DEVICE_CONFIG_OFFSET_START 0x00
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
#define SOCKET_SECRET_DATA_OFFSET		sizeof(SOCKET_HEADER_OPEN)
#define SOCKET_DATA_OFFSET				sizeof(SCOKET_HERADER_INFO)
#define SOCKET_HEADER_LEN				sizeof(SCOKET_HERADER_INFO)
#define DEVICE_MAC_LEN					6
#define SOCKET_IP_LEN					4
#define SOCKET_MAC_ADDR_OFFSET			2
#define DEVICE_NAME_LEN					20


#define USER_FUNC	ICACHE_FLASH_ATTR

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
	AES_KEY_DEFAULT,
	AES_KEY_SERVER,
	AES_KEY_OPEN
} AES_KEY_TYPE;


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
	U8 bEncrypt;
	U8 bReback;
	U16 snIndex;
	U16 bodyLen;
	AES_KEY_TYPE keyType;
	U8* bodyData;
} CREATE_SOCKET_DATA;



#endif
