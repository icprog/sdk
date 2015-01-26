/*
******************************
*Company:Lumitek
*Data:2015-01-26
*Author:Meiyusong
******************************
*/


#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"

#include "user_interface.h"
#include "lumlink/lumCommon.h"
#include "lumlink/lumSocketAes.h"
#include "lumlink/aes.h"



static BOOL USER_FUNC lum_getAesKeyData(AES_KEY_TYPE keyType, U8* keyData)
{
	U8* tmpKeyData = NULL;
	BOOL needCopy = TRUE;


	if(keyType == AES_KEY_DEFAULT)
	{
		tmpKeyData = DEFAULT_AES_KEY;
	}
	else if(keyType == AES_KEY_SERVER)
	{
		tmpKeyData = DEFAULT_AES_KEY; //g_deviceConfig.globalData.keyData.serverKey;
	}
	else
	{
		needCopy = FALSE;
	}
	if(needCopy)
	{
		os_memcpy(keyData, tmpKeyData, AES_KEY_LEN);
	}
	return needCopy;
}


static BOOL USER_FUNC lum_setAesKey(AES_CTX* aesCtx, AES_KEY_TYPE keyType, AES_DIRECTION aesType)
{
	U8 aesKey[AES_KEY_LEN + 1];
	BOOL ret = TRUE;


	os_memset(aesKey, 0, sizeof(aesKey));
	if(!lum_getAesKeyData(keyType, aesKey))
	{
		ret = FALSE;
	}
	else
	{
		AES_set_key(aesCtx, aesKey, aesKey, AES_MODE_128);
		if(aesType == AES_DECRYPTION)
		{
			AES_convert_key(aesCtx);
		}
	}
	return ret;
}



static void USER_FUNC lum_PKCS5PaddingFillData(U8* inputData, U8* dataLen, AES_KEY_TYPE keyType)
{
	U8	fillData;

	if(keyType == AES_KEY_OPEN)
	{
		return;
	}
	fillData = AES_BLOCKSIZE - (*dataLen%AES_BLOCKSIZE);
	memset((inputData + *dataLen), fillData, fillData);
	*dataLen += fillData;
}



static void USER_FUNC lum_PKCS5PaddingRemoveData(U8* inputData, U8* dataLen, AES_KEY_TYPE keyType)
{
	U8	removeData = inputData[*dataLen - 1];

	if(keyType == AES_KEY_OPEN)
	{
		return;
	}
	if(removeData != 0)
	{
		os_memset((inputData + *dataLen - removeData), 0, removeData);
	}
	*dataLen -= removeData;
}


static BOOL USER_FUNC lum_AesEncryptSocketData(U8 *inData, U8* outData, U8* aesDataLen, AES_KEY_TYPE keyType)
{
	AES_CTX aesCtx;


	if(inData == NULL || outData == NULL)
	{
		lumError("meiyusong===> socketDataAesEncrypt input data Error \n");
		return FALSE;
	}
	if(keyType == AES_KEY_OPEN)
	{
		os_memcpy(outData, inData, *aesDataLen);
	}
	else
	{
		if(!lum_setAesKey(&aesCtx, keyType, AES_ENCRYPTION))
		{
			lumError("meiyusong===> Encrypt keyType Error keyType=%d\n", keyType);
			return FALSE;
		}
		lum_PKCS5PaddingFillData(inData, aesDataLen, keyType);
		AES_cbc_encrypt(&aesCtx, inData , outData, *aesDataLen);
	}
	return TRUE;
}


static USER_FUNC lum_AesDecryptSocketData(U8 *inData, U8* outData, U8* aesDataLen, AES_KEY_TYPE keyType)
{
	AES_CTX aesCtx;
	U8 dataLen = *aesDataLen;


	if(inData == NULL || outData == NULL)
	{
		lumError("socketDataAesDecrypt input data Error \n");
		return FALSE;
	}

	if(keyType == AES_KEY_OPEN)
	{
		os_memcpy(outData, inData, dataLen);
	}
	else
	{
		if(!lum_setAesKey(&aesCtx, keyType, AES_DECRYPTION))
		{
			return FALSE;
		}
		AES_cbc_decrypt(&aesCtx, inData, outData, dataLen);
		lum_PKCS5PaddingRemoveData(outData, aesDataLen, keyType);
	}
	return TRUE;
}


BOOL USER_FUNC lum_getRecvSocketData(U8* recvData, U8* decryptData, AES_KEY_TYPE keyType)
{
	U8 openDataLen = sizeof(SOCKET_HEADER_OPEN);
	SOCKET_HEADER_OPEN* pHeaderOpen;
	BOOL ret;

	pHeaderOpen = (SOCKET_HEADER_OPEN*)recvData;
	os_memcpy(decryptData, recvData, openDataLen);
	ret = lum_AesDecryptSocketData(recvData+openDataLen, decryptData+openDataLen, &pHeaderOpen->dataLen, keyType);
	return ret;
}


U8* USER_FUNC lum_createSendSocketData(CREATE_SOCKET_DATA* pCreateData, U8* socketLen)
{
	SCOKET_HERADER_INFO* pSocketHeader;
	U8 tmpData[SOCKEY_MAX_DATA_LEN];
	U8* pAesData = NULL;
	U8 aesDataLen;
	U16 mallocLen;
	U8 openDataLen = sizeof(SOCKET_HEADER_OPEN);


	os_memset(tmpData, 0, sizeof(tmpData));

	pSocketHeader = (SCOKET_HERADER_INFO*)tmpData;
	pSocketHeader->openData.pv = SOCKET_HEADER_PV;
	pSocketHeader->openData.flag.bEncrypt = pCreateData->bEncrypt;
	pSocketHeader->openData.flag.bReback = pCreateData->bReback;
	pSocketHeader->openData.flag.bLocked = 0;
	wifi_get_macaddr(SOFTAP_IF, pSocketHeader->openData.mac);
	aesDataLen = (pCreateData->bodyLen + SOCKET_HEADER_LEN - openDataLen);
	//pSocketHeader->openData.dataLen = (pCreateData->bodyLen + SOCKET_HEADER_LEN - openDataLen); //aesDataLen

	pSocketHeader->reserved = SOCKET_HEADER_RESERVED;
	pSocketHeader->snIndex = htons(pCreateData->snIndex);
	pSocketHeader->deviceType = SOCKET_HEADER_DEVICE_TYPE;
	pSocketHeader->factoryCode = SOCKET_HEADER_FACTORY_CODE;
	pSocketHeader->licenseData = SOCKET_HEADER_LICENSE_DATA;

	//fill body data
	os_memcpy(tmpData+SOCKET_HEADER_LEN, pCreateData->bodyData, pCreateData->bodyLen);

	mallocLen = SOCKET_HEADER_LEN + pCreateData->bodyLen + AES_BLOCKSIZE + 1;
	pAesData = (U8*)os_malloc(mallocLen);
	if(pAesData == NULL)
	{
		lumError("malloc pAesData space faild!");
		return NULL;
	}

	os_memset(pAesData, 0, mallocLen);
	os_memcpy(pAesData, tmpData, openDataLen);
	lum_AesEncryptSocketData(tmpData+openDataLen, pAesData+openDataLen, &aesDataLen, pCreateData->keyType);
	pSocketHeader = (SCOKET_HERADER_INFO*)pAesData;
	pSocketHeader->openData.dataLen = aesDataLen;
	*socketLen = aesDataLen + openDataLen;
	return pAesData;
}



