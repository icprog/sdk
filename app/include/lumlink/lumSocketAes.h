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



BOOL lum_getRecvSocketData(U8* recvData, U8* decryptData, AES_KEY_TYPE keyType);
U8* lum_createSendSocketData(CREATE_SOCKET_DATA* pCreateData, U8* socketLen);

#endif
