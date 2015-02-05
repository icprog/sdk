/*
******************************
*Company:Lumlink
*Data:2015-01-26
*Author:Meiyusong
******************************
*/

#ifndef __LUMLINK_SOCKET_AES__H__
#define __LUMLINK_SOCKET_AES__H__



typedef enum
{
    AES_ENCRYPTION,
    AES_DECRYPTION
}AES_DIRECTION;


typedef enum
{
	AES_KEY_DEFAULT,
	AES_KEY_SERVER,
	AES_KEY_OPEN
} AES_KEY_TYPE;

typedef struct
{
	U8 bEncrypt;
	U8 bReback;
	U16 snIndex;
	U16 bodyLen;
	AES_KEY_TYPE keyType;
	U8* bodyData;
} CREATE_SOCKET_DATA;


BOOL USER_FUNC lum_getAesKeyData(AES_KEY_TYPE keyType, U8* keyData);
AES_KEY_TYPE USER_FUNC lum_getSocketAesKeyType(MSG_ORIGIN msgOrigin, U8 bEncrypt);
BOOL USER_FUNC lum_getRecvSocketData(U8* recvData, U8* decryptData, MSG_ORIGIN socketFrom);
U8* USER_FUNC lum_createSendSocketData(CREATE_SOCKET_DATA* pCreateData, U8* socketLen, MSG_ORIGIN socketFrom);

#endif
