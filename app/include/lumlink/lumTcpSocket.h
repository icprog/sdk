/*
******************************
*Company:Lumlink
*Data:2015-02-01
*Author:Meiyusong
******************************
*/

#ifndef __LUMLINK_TCP_SOCKET_H__
#define __LUMLINK_TCP_SOCKET_H__


typedef enum
{
	TCP_NONE_CONNECT,
	TCP_BALANCE_CONNECTING,
	TCP_BALANCE_CONNECTED,
	TCP_BALANCE_DISCONNECT,
	TCP_SERVER_CONNECTING,
	TCP_SERVER_CONNECTED,
	TCP_SERVER_DISCONNECT,
}TCP_CONN_STATUS;


void USER_FUNC lum_connBalanceServer(void);
void USER_FUNC lum_sendTcpData(U8* socketData, U8 dataLen);
void USER_FUNC lum_connActualServer(void);


#endif
