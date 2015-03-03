/*
******************************
*Company:Lumlink
*Data:2015-01-26
*Author:Meiyusong
******************************
*/

#ifndef __LUMLINK_UDP_SOCKET_H__
#define __LUMLINK_UDP_SOCKET_H__


void USER_FUNC lum_udpSocketInit(void);
BOOL USER_FUNC lum_sendUdpData(U8* socketData, U8 dataLen, U32 ipData);


#endif
