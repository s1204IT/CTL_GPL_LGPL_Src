/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
--                                                                            --
--                    CedarX Multimedia Framework                             --
--                                                                            --
--          the Multimedia Framework for Linux/Android System                 --
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                         Softwinner Products.                               --
--                                                                            --
--                   (C) COPYRIGHT 2011 SOFTWINNER PRODUCTS                   --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
*******************************************************************************/
#include <linux/module.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/sched.h>

#define LOG_TAG "rt_message"


#include "rt_message.h"
#include "rt_common.h"

static int TMessageDeepCopyMessage(message_t *pDesMsg, message_t *pSrcMsg)
{
	pDesMsg->command = pSrcMsg->command;
	pDesMsg->para0 = pSrcMsg->para0;
	pDesMsg->para1 = pSrcMsg->para1;
	pDesMsg->mpData = NULL;
	pDesMsg->mDataSize = 0;
	if (pSrcMsg->mpData && pSrcMsg->mDataSize > 0) {
		pDesMsg->mpData = kmalloc(pSrcMsg->mDataSize, GFP_KERNEL);
		if (pDesMsg->mpData) {
			pDesMsg->mDataSize = pSrcMsg->mDataSize;
			memcpy(pDesMsg->mpData, pSrcMsg->mpData, pSrcMsg->mDataSize);
		} else {
			RT_LOGE(" fatal error! kmalloc MessageData fail!");
			return -1;
		}
	}
	pDesMsg->pReply = pSrcMsg->pReply;
	return 0;
}

/**
  used for get_message(), so move data and reply to dstMsg.
*/
static int TMessageSetMessage(message_t *pDesMsg, message_t *pSrcMsg)
{
	pDesMsg->command = pSrcMsg->command;
	pDesMsg->para0 = pSrcMsg->para0;
	pDesMsg->para1 = pSrcMsg->para1;
	pDesMsg->mpData = pSrcMsg->mpData;
	pDesMsg->mDataSize = pSrcMsg->mDataSize;
	pDesMsg->pReply = pSrcMsg->pReply;
	return 0;
}

static int TMessageIncreaseIdleMessageList(message_queue_t *pThiz)
{
	int ret = 0;
	message_t   *pMsg;
	int i;
	for (i = 0; i < MAX_MESSAGE_ELEMENTS; i++) {
		pMsg = kmalloc(sizeof(message_t), GFP_KERNEL);
		if (NULL == pMsg) {
			RT_LOGE(" fatal error! kmalloc fail");
			ret = -1;
			break;
		}
		list_add_tail(&pMsg->mList, &pThiz->mIdleMessageList);
	}
	return ret;
}

MessageReply *ConstructMessageReply(void)
{
	MessageReply *pReply = (MessageReply *)kmalloc(sizeof(MessageReply), GFP_KERNEL);
	if (NULL == pReply) {
		RT_LOGE("fatal error! malloc fail.");
	}
	memset(pReply, 0, sizeof(MessageReply));
	int ret = rt_sem_init(&pReply->ReplySem, 0);
	if (ret != 0) {
		RT_LOGE("fatal error! rt sem ini fail:%d", ret);
	}
	return pReply;
}

void DestructMessageReply(MessageReply *pReply)
{
	if (pReply) {
		if (pReply->pReplyExtra) {
			kfree(pReply->pReplyExtra);
			pReply->pReplyExtra = NULL;
			pReply->nReplyExtraSize = 0;
		}
		rt_sem_deinit(&pReply->ReplySem);
		kfree(pReply);
	} else {
		RT_LOGE("fatal error! reply is null");
	}
}

int InitMessage(message_t *pMsg)
{
    memset(pMsg, 0, sizeof(message_t));
    return 0;
}

int message_create(message_queue_t *msg_queue)
{
	RT_LOGI("msg create\n");

	mutex_init(&msg_queue->mutex);
	msg_queue->mWaitMessageFlag = 0;
	init_waitqueue_head(&msg_queue->wait_msg);

	INIT_LIST_HEAD(&msg_queue->mIdleMessageList);
	INIT_LIST_HEAD(&msg_queue->mReadyMessageList);
	if (0 != TMessageIncreaseIdleMessageList(msg_queue)) {
		goto _err1;
	}
	msg_queue->message_count = 0;

	return 0;
_err1:
	return -1;
}


void message_destroy(message_queue_t *msg_queue)
{
	int cnt = 0;

	mutex_lock(&msg_queue->mutex);
	if (!list_empty(&msg_queue->mReadyMessageList)) {
		message_t *pEntry, *pTmp;
		list_for_each_entry_safe(pEntry, pTmp, &msg_queue->mReadyMessageList, mList) {
			RT_LOGD(" msg destroy: cmd[%x]", pEntry->command);
			if (pEntry->mpData) {
				kfree(pEntry->mpData);
				pEntry->mpData = NULL;
			}
			pEntry->mDataSize = 0;
			if (pEntry->pReply) {
				RT_LOGE("fatal error! MessageReply[%p] is not destroy? It should be destroyed out. check code!", pEntry->pReply);
				DestructMessageReply(pEntry->pReply);
				pEntry->pReply = NULL;
			}
			list_move_tail(&pEntry->mList, &msg_queue->mIdleMessageList);
			msg_queue->message_count--;
		}
	}
	if (msg_queue->message_count != 0) {
		RT_LOGE(" fatal error! msg count[%d]!=0", msg_queue->message_count);
	}
	if (!list_empty(&msg_queue->mIdleMessageList)) {
		message_t   *pEntry, *pTmp;
		list_for_each_entry_safe(pEntry, pTmp, &msg_queue->mIdleMessageList, mList) {
			list_del(&pEntry->mList);
			kfree(pEntry);
			cnt++;
		}
	}
	INIT_LIST_HEAD(&msg_queue->mIdleMessageList);
	INIT_LIST_HEAD(&msg_queue->mReadyMessageList);
	mutex_unlock(&msg_queue->mutex);
}


void flush_message(message_queue_t *msg_queue)
{
	mutex_lock(&msg_queue->mutex);
	if (!list_empty(&msg_queue->mReadyMessageList)) {
		message_t   *pEntry, *pTmp;
		list_for_each_entry_safe(pEntry, pTmp, &msg_queue->mReadyMessageList, mList) {
			RT_LOGD(" msg destroy: cmd[%x]", pEntry->command);
			if (pEntry->mpData) {
				kfree(pEntry->mpData);
				pEntry->mpData = NULL;
			}
			pEntry->mDataSize = 0;
			if (pEntry->pReply) {
				RT_LOGW("Be careful! Destroy MessageReply[%p] when flush message, very dangerous!", pEntry->pReply);
				DestructMessageReply(pEntry->pReply);
				pEntry->pReply = NULL;
			}
			list_move_tail(&pEntry->mList, &msg_queue->mIdleMessageList);
			msg_queue->message_count--;
		}
	}
	if (msg_queue->message_count != 0) {
		RT_LOGE(" fatal error! msg count[%d]!=0", msg_queue->message_count);
	}
	mutex_unlock(&msg_queue->mutex);
}

/*******************************************************************************
Function name: put_message
Description:
    Do not accept mpData when call this function.
Parameters:

Return:

Time: 2015/3/6
*******************************************************************************/
int put_message(message_queue_t *msg_queue, message_t *msg_in)
{
	message_t message;
	memset(&message, 0, sizeof(message_t));
	message.command = msg_in->command;
	message.para0 = msg_in->para0;
	message.para1 = msg_in->para1;
	return putMessageWithData(msg_queue, &message);
}

int get_message(message_queue_t *msg_queue, message_t *msg_out)
{
	message_t   *pMessageEntry = NULL;

	mutex_lock(&msg_queue->mutex);

	if (list_empty(&msg_queue->mReadyMessageList)) {
		mutex_unlock(&msg_queue->mutex);
		return -1;
	}
	pMessageEntry = list_first_entry(&msg_queue->mReadyMessageList, message_t, mList);
	TMessageSetMessage(msg_out, pMessageEntry);
	list_move_tail(&pMessageEntry->mList, &msg_queue->mIdleMessageList);
	msg_queue->message_count--;

	mutex_unlock(&msg_queue->mutex);
	return 0;
}

int putMessageWithData(message_queue_t *msg_queue, message_t *msg_in)
{
	int ret = 0;
	message_t   *pMessageEntry = NULL;
	mutex_lock(&msg_queue->mutex);
	if (list_empty(&msg_queue->mIdleMessageList)) {
		RT_LOGW(" idleMessageList are all used, kmalloc more!");
		if (0 != TMessageIncreaseIdleMessageList(msg_queue)) {
			mutex_unlock(&msg_queue->mutex);
			return -1;
		}
	}
	pMessageEntry = list_first_entry(&msg_queue->mIdleMessageList, message_t, mList);
	if (0 == TMessageDeepCopyMessage(pMessageEntry, msg_in)) {
		list_move_tail(&pMessageEntry->mList, &msg_queue->mReadyMessageList);
		msg_queue->message_count++;
		RT_LOGI(" new msg command[%d], para[%d][%d]", pMessageEntry->command, pMessageEntry->para0, pMessageEntry->para1);
		if (msg_queue->mWaitMessageFlag) {
			wake_up(&msg_queue->wait_msg);
		}
	} else {
		ret = -1;
	}
    mutex_unlock(&msg_queue->mutex);
	return ret;
}

int get_message_count(message_queue_t *msg_queue)
{
	int message_count;

	mutex_lock(&msg_queue->mutex);
	message_count = msg_queue->message_count;
	mutex_unlock(&msg_queue->mutex);

	return message_count;
}

int TMessage_WaitQueueNotEmpty(message_queue_t *msg_queue, unsigned int timeout)
{
	int message_count;
	unsigned long time_jiffs = msecs_to_jiffies(timeout);
	int ret = 0;
	mutex_lock(&msg_queue->mutex);
	msg_queue->mWaitMessageFlag = 1;
	if (timeout <= 0) {
		while (list_empty(&msg_queue->mReadyMessageList)) {
			mutex_unlock(&msg_queue->mutex);
			wait_event(msg_queue->wait_msg, (msg_queue->message_count > 0));
			mutex_lock(&msg_queue->mutex);
		}
	} else {
		if (list_empty(&msg_queue->mReadyMessageList)) {
			mutex_unlock(&msg_queue->mutex);
			ret = wait_event_timeout(msg_queue->wait_msg, (msg_queue->message_count > 0), time_jiffs);
			mutex_lock(&msg_queue->mutex);
			if (0 == ret) {
				if (msg_queue->message_count != 0) {
					RT_LOGE("fatal error! wait event timeout, why message count[%d]!=0", msg_queue->message_count);
				}
			} else {
				if (0 == msg_queue->message_count) {
					RT_LOGE("fatal error! wait event success, remaining jiffies:%d, why message count==0?", ret);
				}
			}
		}
	}
	msg_queue->mWaitMessageFlag = 0;
	message_count = msg_queue->message_count;
	mutex_unlock(&msg_queue->mutex);

	return message_count;
}

