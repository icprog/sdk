/*
******************************
*Company:Lumlink
*Data:2015-03-01
*Author:Meiyusong
******************************
*/


#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"


#include "lumlink/lumCommon.h"
#include "lumlink/lumSocketAes.h"
#include "lumlink/lumMessageTask.h"
#include "lumlink/lumPlatform.h"
#include "lumlink/lumTcpSocket.h"
#include "lumlink/lumSendList.h"




static SEND_LIST_HEADER g_sendListHeader;
static BOOL	g_bDelayingAfterSend;


static void USER_FUNC lum_resendTimerCallback(void *arg)
{
	lum_sendLocalTaskMessage(MSG_CMD_LOCAL_CHECK_SEND_LIST, NULL, 0);
	if(g_bDelayingAfterSend)
	{
		g_bDelayingAfterSend = FALSE;
	}
}


static void USER_FUNC lum_initResendTimer(BOOL bDelayingAfterSend)
{
	static os_timer_t reSendTimerFd;
	U32 timerGap;


	if(g_bDelayingAfterSend && !bDelayingAfterSend)
	{
		return;
	}
	if(bDelayingAfterSend)
	{
		g_bDelayingAfterSend = TRUE;
		timerGap = NEXT_SOCKET_SEND_DELAY;
	}
	else
	{
		timerGap = RESEND_TIMER_GAP;
	}
	os_timer_disarm(&reSendTimerFd);
	os_timer_setfn(&reSendTimerFd, (os_timer_func_t *)lum_resendTimerCallback, NULL);
	os_timer_arm(&reSendTimerFd, timerGap, 0);
}


void USER_FUNC lum_sendListInit(void)
{
	g_sendListHeader.firstNodePtr = NULL;
	g_sendListHeader.noteCount = 0;

	lum_initResendTimer(FALSE);
	g_bDelayingAfterSend = FALSE;
}


static void USER_FUNC lum_insertSendListNode(SEND_NODE* pNode)
{
	SEND_LIST_HEADER* pListHeader = &g_sendListHeader;
	SEND_NODE* curNode;


	curNode = pListHeader->firstNodePtr;
	if(curNode == NULL)
	{
		pListHeader->firstNodePtr = pNode;
		pNode->pNodeNext = NULL;
	}
	else
	{
		while(curNode->pNodeNext != NULL)
		{
			curNode = curNode->pNodeNext;
		}
		curNode->pNodeNext = pNode;
		pNode->pNodeNext = NULL;
	}
	pListHeader->noteCount++;
	//lumDebug("lum_insertSendListNode cmdData=0x%x sn=0x%04X noteCount=%d\n", pNode->nodeBody.cmdData, pNode->nodeBody.snIndex, pListHeader->noteCount);
}


static void USER_FUNC lum_freeSendNodeMemory(SEND_NODE* pNode)
{
	if(pNode->nodeBody.pData != NULL)
	{
		lum_free(pNode->nodeBody.pData);
		pNode->nodeBody.pData = NULL;
	}
	lum_free(pNode);
}


static BOOL USER_FUNC lum_deleteSendListNode(SEND_NODE* pNode)
{
	SEND_LIST_HEADER* pListHeader = &g_sendListHeader;
	SEND_NODE* pCurNode;
	BOOL ret = FALSE;


	pCurNode = pListHeader->firstNodePtr;
	if(pCurNode == NULL || pNode == NULL)
	{
		lumDebug("deleteListNode error no node to delete\n");
		return FALSE;
	}
	if(pCurNode == pNode)
	{
		pListHeader->firstNodePtr = pNode->pNodeNext;
		ret = TRUE;
	}
	else
	{
		while(pCurNode->pNodeNext != NULL)
		{
			if(pCurNode->pNodeNext == pNode)
			{
				pCurNode->pNodeNext = pCurNode->pNodeNext->pNodeNext;
				ret = TRUE;
				break;
			}
			pCurNode = pCurNode->pNodeNext;
		}
	}
	if(ret)
	{
		pListHeader->noteCount--;
		lum_freeSendNodeMemory(pNode);
	}
	else
	{
		lumDebug("deleteSendListNode not found \n");
	}

	//lumDebug("lum_deleteSendListNode cmdData=0x%x snIndex=0x%04X noteCount=%d\n", pNode->nodeBody.cmdData, pNode->nodeBody.snIndex, pListHeader->noteCount);
	return ret;
}


BOOL USER_FUNC lum_deleteRequstSendNode(U8* pSocketDataRecv)
{
	SEND_NODE* pCurNode = g_sendListHeader.firstNodePtr;
	SCOKET_HERADER_INFO* pSocketHeader;
	BOOL ret = FALSE;


	pSocketHeader = (SCOKET_HERADER_INFO*)pSocketDataRecv;
	if(pSocketDataRecv == NULL)
	{
		return ret;
	}
	while(pCurNode != NULL)
	{
		if(pCurNode->nodeBody.snIndex == pSocketHeader->snIndex && pCurNode->nodeBody.cmdData == (U16)pSocketDataRecv[SOCKET_HEADER_LEN])
		{
			lum_deleteSendListNode(pCurNode);
			ret = TRUE;
			break;
		}
		pCurNode = pCurNode->pNodeNext;
	}
	return ret;
}



BOOL USER_FUNC lum_addSendDataToNode(SEND_NODE_DATA* pSendData)
{
	SEND_NODE* pSendNode;
	U32 mallocLen;


	if(pSendData->msgOrigin != MSG_FROM_UDP && pSendData->msgOrigin != MSG_FROM_TCP)
	{
		lumError("falid add to send node, sendData->msgOrigin=%d\n", pSendData->msgOrigin);
		return FALSE;
	}

	mallocLen = sizeof(SEND_NODE)+1;
	pSendNode = (SEND_NODE*)lum_malloc(mallocLen);
	if(pSendNode == NULL)
	{
		lumError("malloc error\n");
		return FALSE;
	}
	os_memset(pSendNode, 0, mallocLen);
	os_memcpy(&pSendNode->nodeBody, pSendData, sizeof(SEND_NODE_DATA));
	lum_insertSendListNode(pSendNode);
	lum_resendTimerCallback(NULL);
	return TRUE;
}



static BOOL USER_FUNC lum_needWaitSocketReback(U16 cmdData)
{
	BOOL ret = TRUE;


	switch(cmdData)
	{
	case MSG_CMD_REPORT_GPIO_CHANGE:
		ret = FALSE;
		break;

	default:
		break;
	}
	return ret;
}

void USER_FUNC lum_checkSendList(void)
{
	SEND_NODE* pCurNode = g_sendListHeader.firstNodePtr;
	SEND_NODE* pNodeNext;
	BOOL sendSuccess = FALSE;
	U32 curTime;
	BOOL needCheck = FALSE;;


	lum_initResendTimer(FALSE);
	curTime = lum_getSystemTime();
	//lumDebug("<<<========%d lum_checkSendList noteCount=%d mallocCount=%d g_bDelayingAfterSend=%d\n",
	//         curTime, g_sendListHeader.noteCount, lum_getMallocCount(), g_bDelayingAfterSend);
	while(pCurNode != NULL && !g_bDelayingAfterSend)
	{
		pNodeNext = pCurNode->pNodeNext;
		if(pCurNode->nodeBody.msgOrigin != MSG_FROM_UDP && pCurNode->nodeBody.msgOrigin != MSG_FROM_TCP)
		{
			lumError("Falid socketType=%d\n", pCurNode->nodeBody.msgOrigin);
			lum_deleteSendListNode(pCurNode);
			pCurNode = pNodeNext;
			continue;
		}

		if(pCurNode->nodeBody.nextSendTime == 0 || pCurNode->nodeBody.nextSendTime < curTime)
		{
			if(pCurNode->nodeBody.msgOrigin == MSG_FROM_UDP)
			{
				sendSuccess = lum_sendUdpData(pCurNode->nodeBody.pData, pCurNode->nodeBody.dataLen, pCurNode->nodeBody.socketIp);
			}
			else
			{
				sendSuccess = lum_sendTcpData(pCurNode->nodeBody.pData, pCurNode->nodeBody.dataLen);
			}
			lum_initResendTimer(TRUE);
			pCurNode->nodeBody.sendCount++;

#if 0
			lumDebug("====> cmdData=0x%X, origin=%d sendTimes=%d snIndex=0x%04X mallocCount=%d sendCount=%d noteCount=%d\n",
			         pCurNode->nodeBody.cmdData,
			         pCurNode->nodeBody.msgOrigin,
			         pCurNode->nodeBody.sendCount,
			         pCurNode->nodeBody.snIndex,
			         lum_getMallocCount(),
			         pCurNode->nodeBody.sendCount,
			         g_sendListHeader.noteCount);
#endif
			if(sendSuccess)
			{
				if(pCurNode->nodeBody.bReback == 0 && lum_needWaitSocketReback(pCurNode->nodeBody.cmdData))
				{
					needCheck = TRUE;
				}
				else
				{
					lum_deleteSendListNode(pCurNode);
				}
			}
			else //send faild
			{
				lumDebug("======>Send Faild\n");
				needCheck = TRUE;
			}
			if(needCheck)
			{
				if(pCurNode->nodeBody.sendCount >= MAX_RESEND_COUNT)
				{
					lum_deleteSendListNode(pCurNode);
				}
				else
				{
					pCurNode->nodeBody.nextSendTime = curTime + MAX_RESEND_INTERVAL;
				}
			}
		}
		else
		{
			//lumDebug("----> msgOrigin=%d cmdData=0x%X snIndex=0x%04X bReback=%d nextSendTime=%d sendCount=%d\n", pCurNode->nodeBody.msgOrigin,
			//         pCurNode->nodeBody.cmdData, pCurNode->nodeBody.snIndex, pCurNode->nodeBody.bReback, pCurNode->nodeBody.nextSendTime, pCurNode->nodeBody.sendCount);
		}
		pCurNode = pNodeNext;
	}
}
